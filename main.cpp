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

class Worker {
public:
  redisContext *c;
  int id;
  enum status { idle, running } status;
  Worker(int id, string redis) : id(id), status(status::idle) {
    c = redis_init(redis.c_str());
  }
};

int a = 1;

void schedule(vector<Worker> &workers, vector<Model> &models,
              vector<deque<ForwardTask *>> &queues) {
  for (auto &worker : workers) {
    if (worker.status == Worker::status::idle) {
      ForwardTask *task = nullptr;
      if (!queues[worker.id].empty()) {
        task = queues[worker.id].back();
        queues[worker.id].pop_back();
      } else if (!queues[1 - worker.id].empty()) {
        task = queues[1 - worker.id].back();
        queues[1 - worker.id].pop_back();
      }
      if (task) {
        worker.status = Worker::status::running;
        send_model(worker.c, *task);
      }
    }
  }
}

int main() {
  redisContext *done = redis_init("REDISDONE");

  vector<Model> models;
  vector<Worker> workers = {Worker(0, "REDIS0"), Worker(1, "REDIS1")};
  vector<reference_wrapper<ForwardTask>> tasks;
  vector<ModelTask> model_tasks;
  int n = get_models_from_json(models, "schema.json");
  vector<deque<ForwardTask *>> queues(n);

  for (int i = 0; i < 2000; i++) {
    model_tasks.emplace_back(ModelTask(models[i % 2], i % 3 + (i % 2) * 3));
  }

  for (int i = 0; i < 2000; i++) {
    model_tasks[i].create_tasks();
  }

  for (auto &model_task : model_tasks) {
    for (auto &i : model_task.tasks) {
      i.task_id = tasks.size();
      tasks.emplace_back(i);
    }
    queues[model_task.model.id].push_front(&model_task.tasks[0]);
  }

  schedule(workers, models, queues);

  while (true) {
    string result;
    redisReply *reply;
    while (true) {
      reply = (redisReply *)redisCommand(done, "LPOP done");
      if (reply->type == REDIS_REPLY_STRING) {
        result = reply->str;

        istringstream iss(result);
        freeReplyObject(reply);

        int task_id, worker_id;
        iss >> task_id >> worker_id;
        auto task = tasks[task_id];
        long long int end = chrono::duration_cast<chrono::microseconds>(
                                chrono::system_clock::now().time_since_epoch())
                                .count();
        workers[worker_id].status = Worker::status::idle;
        task.get().status = Task::status::done;

        if (task.get().layer_id + 1 < task.get().model.size()) {
          queues[task.get().model.id].push_back(&tasks[task_id + 1].get());
        } else {
          // cout << end - task.get().model_task.start_time << endl;
          cout << task.get().model_task.start_time << " " << end << endl;
          task.get().model_task.end_time = end;
        }
      } else {
        break;
      }
    }
    if (queues[0].size() || queues[1].size()) {
      schedule(workers, models, queues);
    }
  }

  vector<int> batches = {64, 32, 16, 8, 4, 2, 1};

  return 0;
}
