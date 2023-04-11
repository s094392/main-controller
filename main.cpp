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
              queue<ForwardTask *> &queue) {
  a++;
  if (a % 2 == 0) {
    for (auto &worker : workers) {
      if (!queue.empty()) {
        if (worker.status == Worker::status::idle) {
          ForwardTask *task = queue.front();
          queue.pop();
          worker.status = Worker::status::running;
          send_model(worker.c, *task);
        }
      }
    }
  } else {
    for (int i = workers.size() - 1; i >= 0; i--) {
      if (!queue.empty()) {
        ForwardTask *task = queue.front();
        queue.pop();
        workers[i].status = Worker::status::running;
        send_model(workers[i].c, *task);
      }
    }
  }
};

int main() {
  redisContext *done = redis_init("REDISDONE");

  vector<Model> models;
  vector<Worker> workers = {Worker(0, "REDIS0"), Worker(1, "REDIS1")};
  vector<ForwardTask *> tasks;
  queue<ForwardTask *> queue;
  int n = get_models_from_json(models, "schema.json");

  ModelTask vgg160(&models[0]);
  ModelTask vgg161(&models[0]);
  vgg160.pos = 0;
  vgg161.pos = 1;

  for (auto &i : vgg160.tasks) {
    i.task_id = tasks.size();
    tasks.push_back(&i);
  }
  for (auto &i : vgg161.tasks) {
    i.task_id = tasks.size();
    tasks.push_back(&i);
  }

  queue.push(&vgg160.tasks[0]);
  queue.push(&vgg161.tasks[0]);

  schedule(workers, models, queue);

  while (true) {
    string result;
    redisReply *reply;
    reply = (redisReply *)redisCommand(done, "LPOP done");
    if (reply->type == REDIS_REPLY_STRING) {
      result = reply->str;
      cout << result << "\n";

      istringstream iss(result);
      freeReplyObject(reply);

      int task_id, worker_id;
      iss >> task_id >> worker_id;
      cout << task_id << " " << worker_id << "\n";
      auto task = tasks[task_id];
      int end = chrono::duration_cast<chrono::microseconds>(
                    chrono::system_clock::now().time_since_epoch())
                    .count();
      workers[worker_id].status = Worker::status::idle;
      task->status = Task::status::done;

      if (task->layer_id + 1 < task->model->size()) {
        queue.push(tasks[task_id + 1]);
      } else {
        cout << end - task->model_task->start_time << endl;
      }
    }
    if (queue.size()) {
      schedule(workers, models, queue);
    }
  }

  vector<int> batches = {64, 32, 16, 8, 4, 2, 1};

  return 0;
}
