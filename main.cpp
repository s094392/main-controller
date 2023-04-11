#include "actions.hpp"

#include "model.hpp"
#include <hiredis/hiredis.h>
#include <iostream>
#include <queue>
#include <string>
#include <unistd.h>
#include <vector>

redisContext *redis_init(std::string redis) {
  redisContext *c = redisConnect(getenv(redis.c_str()), 6379);

  if (c == NULL || c->err) {
    if (c) {
      std::cout << "Error: " << c->errstr;
    } else {
      std::cout << "Can't allocate redis context";
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

void schedule(vector<Worker> &workers, vector<Model> &models,
              queue<ForwardTask *> &queue) {
  ForwardTask *task = queue.front();
  queue.pop();
  for (auto &worker : workers) {
    if (worker.status == Worker::status::idle) {
      worker.status = Worker::status::running;
      send_model(worker.c, *task);
      return;
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

  send_create(workers[0].c, 16);

  ModelTask vgg160(&models[0]);

  for (auto &i : vgg160.tasks) {
    i.task_id = tasks.size();
    tasks.push_back(&i);
  }

  queue.push(tasks[0]);
  schedule(workers, models, queue);

  while (true) {
    std::string result;
    redisReply *reply;
    reply = (redisReply *)redisCommand(done, "BLPOP done 0");

    result = reply->element[1]->str;

    std::istringstream iss(result);
    freeReplyObject(reply);

    int task_id, worker_id;
    iss >> task_id >> worker_id;
    workers[worker_id].status = Worker::status::idle;
    auto task = tasks[task_id];
    task->status = Task::status::done;

    if (task->layer_id < task->model->size()) {
      queue.push(tasks[task_id + 1]);
    } else {
      cout << "done";
    }
    if (queue.size()) {
      schedule(workers, models, queue);
    }
  }

  // for (int t = 0; t < 10; t++) {
  //   send_create(workers[0].c, 20);
  //   send_model_task(workers[0].c, vgg160);
  // }

  vector<int> batches = {64, 32, 16, 8, 4, 2, 1};

  // for (auto &batch : batches) {
  //   for (int t = 0; t < 10; t++) {
  //     send_create(workers[0].c, batch);
  //     // send_create(c1, batch);
  //     send_model_task(workers[0].c, vgg160);
  //     // send_model_task(c1, vgg160);
  //   }

  //   // for (int t = 0; t < 10; t++) {
  //   //   send_create(c0, batch);
  //   //   for (int i = 0; i < vgg161.tasks.size(); i++) {
  //   //     if (i % 2 == 0) {
  //   //       send_model(c0, vgg160.tasks[i]);
  //   //       send_send(c0, c1, 0, 1);
  //   //     } else {
  //   //       send_model(c1, vgg161.tasks[i]);
  //   //       send_send(c1, c0, 1, 0);
  //   //     }
  //   //   }
  //   // }
  // }
  return 0;
}
