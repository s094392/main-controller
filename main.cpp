#include "actions.hpp"
#include "model.hpp"
#include "utils.hpp"

#include <chrono>
#include <hiredis/hiredis.h>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace std;

redisContext *c;
vector<bool> variables;
long scheduler_all = 0, scheduler_n = 0;
bool overhead = false;
int move_num0 = 0;
int move_num1 = 0;

redisContext *redis_init(string redis) {
  redisContext *c = redisConnect(getenv(redis.c_str()), 6379);
  if (c == NULL || c->err) {
    if (c) {
      cout << "Error: " << c->errstr;
    } else {
      cout << "Can't allocate redis context";
    }
  }
  return c;
}

class Creator {
public:
  int id;
  Creator(int id) : id(id) {}
  void send_task(int variable_id) {
    char cmd[10];
    sprintf(cmd, "%d", variable_id);
    _send_cmd(c, "creator", cmd);
  }

  void init_variables(int n) {
    for (int i = 0; i < n; i++) {
      send_task(i);
      redisReply *reply = (redisReply *)redisCommand(c, "BLPOP done 0");
      freeReplyObject(reply);
      variables[i] = true;
    }
  }
};

class Worker {
public:
  int id;
  enum status { idle, running } status;
  ForwardTask *task;
  Worker(int id) : id(id), status(status::idle) {}
  void send_task(ForwardTask &task) {
    this->task = &task;
    char cmd[50];
    sprintf(cmd, "forward %d %d %d %d", task.task_id, task.model.id,
            task.layer_id, task.model_task.pos);
    if (task.layer_id == 0) {
      task.model_task.start_time =
          chrono::duration_cast<chrono::microseconds>(
              chrono::steady_clock::now().time_since_epoch())
              .count();
    }
    _send_cmd(c, "worker" + to_string(id), cmd);
  }
};

int get_free_variable(int model_id) {
  for (int i = model_id * variables.size() / 2;
       i < variables.size() - (variables.size() / 2) * (1 - model_id); i++) {
    if (variables[i]) {
      variables[i] = false;
      return i;
    }
  }
  return -1;
}

void layerwise_fifo_scheduler(vector<Worker> &workers, vector<Model> &models,
                              vector<deque<ForwardTask *>> &queues) {
  for (auto &worker : workers) {
    if (worker.status == Worker::status::idle) {
      ForwardTask *task = nullptr;
      for (auto &queue : queues) {
        if (!queue.empty() && queue.back()->layer_id % 2 == worker.id % 2) {
          task = queue.back();
          queue.pop_back();
          break;
        }
      }
      if (task) {
        task->model_task.worker = worker.id;
        if (task->layer_id == 0) {
          task->model_task.pos = get_free_variable(task->model.id);
        }
        worker.status = Worker::status::running;
        worker.send_task(*task);
      }
    }
  }
}

bool moved = false;
bool moved0 = false;
int light = 0;

void layerwise_our_scheduler(vector<Worker> &workers, vector<Model> &models,
                             vector<deque<ForwardTask *>> &queues) {
  for (auto &worker : workers) {
    if (worker.status == Worker::status::idle) {
      ForwardTask *task = nullptr;
      if (moved0) {
        if (worker.id == 0) {
          if (!queues[worker.id].empty()) {
            task = queues[worker.id].back();
            queues[worker.id].pop_back();
            moved0 = false;
            // LOGs("release00");
          } else {
            task = queues[1 - worker.id].back();
            queues[1 - worker.id].pop_back();
            // LOGs(task->layer_id);
          }
        } else if (!queues[worker.id].empty()) {
          task = queues[worker.id].back();
          queues[worker.id].pop_back();
          if (task->layer_id + 1 == task->model.size()) {
            moved0 = false;
            // LOGs("release01");
          }
        }
      } else if (moved) {
        if (worker.id == 0 && !queues[worker.id].empty() &&
            queues[worker.id].back()->layer_id != 0) {
          task = queues[worker.id].back();
          queues[worker.id].pop_back();
          if (task->layer_id + 1 == task->model.size()) {
            // LOGs("release10", task->model.id);
            // cout << "rel1" << endl;
            ;
            moved = false;
          }
        } else {
          if (worker.id == 1 && !queues[1 - worker.id].empty() &&
              queues[1 - worker.id].back()->layer_id != 0) {
            task = queues[1 - worker.id].back();
            queues[1 - worker.id].pop_back();
            if (task->layer_id + 1 == task->model.size()) {
              // LOGs("release11");
              // cout << "rel2" << endl;
              ;
              moved = false;
            }
          }
        }
      } else {
        if (!queues[worker.id].empty()) {
          task = queues[worker.id].back();
          queues[worker.id].pop_back();
        } else if (worker.id == 1 && workers[0].status != Worker::idle &&
                   !queues[1 - worker.id].empty() &&
                   queues[1 - worker.id].back()->layer_id == 0) {
          move_num0++;
          // cout << "wow" << endl;
          // LOGs("wow");
          task = queues[1 - worker.id].back();
          if (task->model.size() > 1)
            moved = true;
          queues[1 - worker.id].pop_back();
        } else if (worker.id == 0 && !queues[1 - worker.id].empty() &&
                   queues[1 - worker.id].back()->layer_id == 0 &&
                   workers[1].status != Worker::idle) {
          // cout << "swow" << endl;
          move_num1++;
          task = queues[1 - worker.id].back();
          if (task->model.size() > 1)
            moved0 = true;
          queues[1 - worker.id].pop_back();
        }
      }
      if (task) {
        task->model_task.worker = worker.id;
        if (task->layer_id == 0) {
          task->model_task.pos = get_free_variable(task->model.id);
        }
        worker.status = Worker::status::running;
        worker.send_task(*task);
      }
    }
  }
}

void fixed_scheduler(vector<Worker> &workers, vector<Model> &models,
                     vector<deque<ForwardTask *>> &queues) {
  for (auto &worker : workers) {
    if (worker.status == Worker::status::idle) {
      ForwardTask *task = nullptr;
      if (!queues[worker.id].empty()) {
        task = queues[worker.id].back();
        queues[worker.id].pop_back();
      }
      if (task) {
        task->model_task.worker = worker.id;
        if (task->layer_id == 0) {
          task->model_task.pos = get_free_variable(task->model.id);
        }
        worker.status = Worker::status::running;
        worker.send_task(*task);
      }
    }
  }
}

void fifo_scheduler(vector<Worker> &workers, vector<Model> &models,
                    vector<deque<ForwardTask *>> &queues) {
  for (auto &worker : workers) {
    if (worker.status == Worker::status::idle) {
      ForwardTask *task = nullptr;
      if (!queues[1 - worker.id].empty() &&
          queues[1 - worker.id].back()->model_task.worker == worker.id) {
        task = queues[1 - worker.id].back();
        queues[1 - worker.id].pop_back();
      } else if (!queues[worker.id].empty() &&
                 (queues[worker.id].back()->model_task.worker == -1 ||
                  queues[worker.id].back()->model_task.worker == worker.id)) {
        task = queues[worker.id].back();
        queues[worker.id].pop_back();
      } else if (!queues[1 - worker.id].empty() &&
                 (queues[1 - worker.id].back()->model_task.worker == -1 ||
                  queues[1 - worker.id].back()->model_task.worker ==
                      worker.id)) {
        task = queues[1 - worker.id].back();
        queues[1 - worker.id].pop_back();
      }
      if (task) {
        task->model_task.worker = worker.id;
        if (task->layer_id == 0) {
          task->model_task.pos = get_free_variable(task->model.id);
        }
        worker.status = Worker::status::running;
        worker.send_task(*task);
      }
    }
  }
}

int allcall = 0;

void has(vector<Worker> &workers, vector<Model> &models,
         vector<deque<ForwardTask *>> &queues) {
  allcall++;
  if (workers[0].status == Worker::idle && workers[1].status == Worker::idle) {
    light++;
  }
  if (light > 20) {
    cout << "GGGG" << endl;
    fifo_scheduler(workers, models, queues);
  } else {
    layerwise_our_scheduler(workers, models, queues);
  }
}

void wait_workers(int n) {
  string result;
  redisReply *reply;
  while (n--) {
    reply = (redisReply *)redisCommand(c, "BLPOP initdone 0");
  }
}

Creator creator(0);

void summary(vector<Model> &models, vector<ModelTask> &model_tasks) {
  long long start = model_tasks.begin()->arrival_time;
  long long end = model_tasks.begin()->end_time;
  vector<int> task_num(models.size());
  vector<int> comp_time(models.size());
  vector<int> resp_time(models.size());

  for (auto &model_task : model_tasks) {
    start = min(start, model_task.start_time);
    end = max(end, model_task.end_time);

    task_num[model_task.model.id]++;
    comp_time[model_task.model.id] +=
        model_task.end_time - model_task.start_time;
    resp_time[model_task.model.id] +=
        model_task.end_time - model_task.arrival_time;
  }
  long long tail_tatency = end - start;
  cout << "Tail latency: " << tail_tatency << endl;
  for (int i = 0; i < models.size(); i++) {
    cout << "Model " << i << "\n";
    cout << "Model name: " << models[i].name << "\n";
    cout << "Comp time: " << comp_time[i] / task_num[i] << "\n";
    cout << "Resp time: " << resp_time[i] / task_num[i] << "\n";
  }
}

void loadgen(vector<Model> &models, vector<Worker> &workers,
             vector<reference_wrapper<ForwardTask>> &tasks,
             vector<ModelTask> &model_tasks,
             vector<deque<ForwardTask *>> &queues) {}

int main() {
  c = redis_init("REDIS");
  vector<Model> models;
  vector<Worker> workers = {Worker(0), Worker(1)};
  vector<reference_wrapper<ForwardTask>> tasks;
  vector<ModelTask> model_tasks;
  vector<deque<ForwardTask *>> queues(2);

  int n = get_models_from_json(models, "schema.json");
  for (auto &model : models) {
    cout << model.name << endl;
  }
  variables = vector<bool>(10, false);

  ifstream file;
  file.open("workload.txt");
  int tmp, model_id;

  int lambda0, lambda1;
  file >> lambda0 >> lambda1;
  while (file >> tmp >> model_id) {
    model_tasks.emplace_back(ModelTask(models[model_id], tmp));
  }

  cout << "model_tasks created" << endl;

  creator.init_variables(8);
  wait_workers(2);

  for (int i = 0; i < model_tasks.size(); i++) {
    model_tasks[i].create_tasks();
  }

  cout << "start" << endl;

  int now_task_id = 0;
  for (auto &model_task : model_tasks) {
    for (auto &i : model_task.tasks) {
      i.task_id = tasks.size();
      tasks.emplace_back(i);
    }
  }

  long long start_time = chrono::duration_cast<chrono::microseconds>(
                             chrono::steady_clock::now().time_since_epoch())
                             .count();
  int task_done = 0;

  while (true) {
    string result;
    redisReply *reply;
    if (now_task_id < model_tasks.size()) {
      ModelTask &model_task = model_tasks[now_task_id];
      long long now = chrono::duration_cast<chrono::microseconds>(
                          chrono::steady_clock::now().time_since_epoch())
                          .count();
      if (now > model_task.arrival_time + start_time) {
        queues[model_task.model.id].push_front(&model_task.tasks[0]);
        model_task.arrival_time = now;
        now_task_id++;
      }
    }

    reply = (redisReply *)redisCommand(c, "LPOP done");
    if (reply->type == REDIS_REPLY_STRING) {
      result = reply->str;

      istringstream iss(result);
      freeReplyObject(reply);

      int task_id, worker_id;
      iss >> task_id >> worker_id;
      if (worker_id == -1) {
        variables[task_id] = true;
      } else {
        auto task = tasks[task_id];
        task.get().model_task.end_time =
            chrono::duration_cast<chrono::microseconds>(
                chrono::steady_clock::now().time_since_epoch())
                .count();
        workers[worker_id].status = Worker::status::idle;
        task.get().status = Task::status::done;

        if (task.get().layer_id + 1 < task.get().model.size()) {
          queues[task.get().model.id].push_back(&tasks[task_id + 1].get());
        } else {
          task_done++;
          // cout << task.get().model.id << " " << task_done << endl;
          if (task_done == model_tasks.size()) {
            for (auto &model_task : model_tasks) {
              LOGs(model_task.arrival_time, model_task.start_time,
                   model_task.end_time,
                   model_task.end_time - model_task.start_time,
                   model_task.model.id);
            }
            cout << "move 0: " << move_num0 << ", move 1: " << move_num1
                 << endl;
            cout << "Lambda 0: " << lambda0 << ", lambda1: " << lambda1 << endl;
            if (overhead) {
              cout << "=" << scheduler_all << "/" << scheduler_n << endl;
            }
            break;
          }
          creator.send_task(task.get().model_task.pos);
        }
      }
    }

    long scheduler_start, scheduler_end;

    if (queues[0].size() || queues[1].size()) {
      if (overhead) {
        scheduler_start = chrono::duration_cast<chrono::microseconds>(
                              chrono::steady_clock::now().time_since_epoch())
                              .count();
      }

      // fixed_scheduler(workers, models, queues);
      // fifo_scheduler(workers, models, queues);
      // layerwise_our_scheduler(workers, models, queues);
      has(workers, models, queues);

      if (overhead) {
        scheduler_end = chrono::duration_cast<chrono::microseconds>(
                            chrono::steady_clock::now().time_since_epoch())
                            .count();
        scheduler_all += scheduler_end - scheduler_start;
        scheduler_n++;
      }
    }
  }
  return 0;
}
