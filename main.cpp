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
              deque<ForwardTask *> &queue) {
  a++;
  if (a % 2 == 0) {
    for (auto &worker : workers) {
      if (!queue.empty()) {
        if (worker.status == Worker::status::idle) {
          ForwardTask *task = queue.back();
          queue.pop_back();
          worker.status = Worker::status::running;
          send_model(worker.c, *task);
        }
      }
    }
  } else {
    for (int i = workers.size() - 1; i >= 0; i--) {
      if (!queue.empty()) {
        if (workers[i].status == Worker::status::idle) {
          ForwardTask *task = queue.back();
          queue.pop_back();
          workers[i].status = Worker::status::running;
          send_model(workers[i].c, *task);
        }
      }
    }
  }
};

int main() {
  redisContext *done = redis_init("REDISDONE");

  vector<Model> models;
  vector<Worker> workers = {Worker(0, "REDIS0"), Worker(1, "REDIS1")};
  vector<reference_wrapper<ForwardTask>> tasks;
  vector<ModelTask> model_tasks;
  deque<ForwardTask *> queue;
  int n = get_models_from_json(models, "schema.json");

  for (int i = 0; i < 100; i++) {
    model_tasks.emplace_back(ModelTask(models[i % 2], i % 10));
  }

  for (int i = 0; i < 100; i++) {
    model_tasks[i].create_tasks();
  }

  for (auto &model_task : model_tasks) {
    for (auto &i : model_task.tasks) {
      i.task_id = tasks.size();
      tasks.emplace_back(i);
    }
    queue.push_front(&model_task.tasks[0]);
  }

  schedule(workers, models, queue);

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
        int end = chrono::duration_cast<chrono::microseconds>(
                      chrono::system_clock::now().time_since_epoch())
                      .count();
        workers[worker_id].status = Worker::status::idle;
        task.get().status = Task::status::done;

        if (task.get().layer_id + 1 < task.get().model.size()) {
          queue.push_back(&tasks[task_id + 1].get());
        } else {
          cout << end - task.get().model_task.start_time << endl;
          task.get().model_task.end_time = end;
        }
      } else {
        break;
      }
    }
    if (queue.size()) {
      schedule(workers, models, queue);
    }
  }

  vector<int> batches = {64, 32, 16, 8, 4, 2, 1};

  return 0;
}
