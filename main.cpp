#include "actions.hpp"
#include "model.hpp"

#include <chrono>
#include <hiredis/hiredis.h>
#include <iostream>
#include <queue>
#include <string>
#include <unistd.h>
#include <vector>

using namespace std;

redisContext *c;
vector<bool> variables;

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
    int n;
    if (variable_id < 4)
      n = 0;
    else
      n = 1;
    _send_cmd(c, ("creator" + to_string(n)).c_str(), cmd);
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
  Worker(int id) : id(id), status(status::idle) {}
  void send_task(ForwardTask &task) {
    char cmd[50];
    sprintf(cmd, "forward %d %d %d %d", task.task_id, task.model.id,
            task.layer_id, task.model_task.pos);
    if (task.layer_id == 0) {
      task.model_task.start_time =
          chrono::duration_cast<chrono::microseconds>(
              chrono::system_clock::now().time_since_epoch())
              .count();
    }
    _send_cmd(c, "worker" + to_string(id), cmd);
  }
};

int get_free_variable(int k) {
  for (int i = k; i < variables.size(); i++) {
    if (variables[i]) {
      variables[i] = false;
      return i;
    }
  }
  return -1;
}

void schedule(vector<Worker> &workers, vector<Model> &models,
              vector<deque<ForwardTask *>> &queues) {
  for (auto &worker : workers) {
    if (worker.status == Worker::status::idle) {
      ForwardTask *task = nullptr;
      if (!queues[worker.id].empty()) {
        task = queues[worker.id].back();
        queues[worker.id].pop_back();
      } else if (!queues[1 - worker.id].empty()) {
        // task = queues[1 - worker.id].back();
        // queues[1 - worker.id].pop_back();
      }
      if (task) {
        if (task->layer_id == 0) {
          if (worker.id == 0)
            task->model_task.pos = get_free_variable(0);
          else
            task->model_task.pos = get_free_variable(4);
        }
        worker.status = Worker::status::running;
        worker.send_task(*task);
      }
    }
  }
}

void wait_workers(int n) {
  string result;
  redisReply *reply;
  while (n--) {
    reply = (redisReply *)redisCommand(c, "BLPOP initdone 0");
  }
}

int main() {
  c = redis_init("REDIS");

  vector<Model> models;
  vector<Worker> workers = {Worker(0), Worker(1)};
  vector<reference_wrapper<ForwardTask>> tasks;
  vector<ModelTask> model_tasks;
  int n = get_models_from_json(models, "schema.json");
  vector<deque<ForwardTask *>> queues(n);
  variables = vector<bool>(8, false);

  int N = 10;
  Creator creator(0);
  creator.init_variables(8);

  for (int i = 0; i < N; i++) {
    model_tasks.emplace_back(ModelTask(models[1], i));
  }

  for (int i = 0; i < N; i++) {
    model_tasks[i].create_tasks();
  }

  for (auto &model_task : model_tasks) {
    for (auto &i : model_task.tasks) {
      i.task_id = tasks.size();
      tasks.emplace_back(i);
    }
    queues[model_task.model.id].push_front(&model_task.tasks[0]);
  }

  wait_workers(2);

  schedule(workers, models, queues);

  while (true) {
    string result;
    redisReply *reply;
    while (true) {
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
                  chrono::system_clock::now().time_since_epoch())
                  .count();
          workers[worker_id].status = Worker::status::idle;
          task.get().status = Task::status::done;

          if (task.get().layer_id + 1 < task.get().model.size()) {
            queues[task.get().model.id].push_back(&tasks[task_id + 1].get());
          } else {
            creator.send_task(task.get().model_task.pos);

            cout << task.get().model_task.start_time << " "
                 << task.get().model_task.end_time << " "
                 << task.get().model_task.end_time -
                        task.get().model_task.start_time
                 << " " << task.get().model.id << endl;
          }
        }
      } else {
        break;
      }
    }
    if (queues[0].size() || queues[1].size()) {
      schedule(workers, models, queues);
    }
  }
  return 0;
}
