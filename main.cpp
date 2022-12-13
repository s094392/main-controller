#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <vector>

const int vgg16_id = 0;
const int vgg16_layer = 37;

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

class Model {
public:
  std::string name;
  int model_id;
  int layer;
  Model(int model_id, std::string name, int layer)
      : model_id(model_id), name(name), layer(layer) {}
};

class Task {
public:
  Model *model;
  int layer_id;
  Task(Model *model, int layer_id) : model(model), layer_id(layer_id) {}
};

class ModelTask {
public:
  Model *model;
  std::vector<Task> tasks;
  ModelTask(Model *model) : model(model) {
    tasks.reserve(model->layer);
    for (int i = 0; i < model->layer; i++) {
      tasks.push_back(Task(model, i));
    }
  }
};

void send_model(Task &task, redisContext *c) {
  redisReply *reply;
  std::string cmd = std::to_string(task.model->model_id) + " " +
                    std::to_string(task.layer_id);
  std::cout << cmd << std::endl;
  reply = (redisReply *)redisCommand(c, "RPUSH foo %s", cmd.c_str());
}

void send_model_task(ModelTask &model_task, redisContext *c) {
  for (auto &task : model_task.tasks) {
    send_model(task, c);
  }
}

int main() {
  redisContext *c = redis_init("REDIS0");
  Model vgg16(0, "vgg16", vgg16_layer);
  ModelTask vgg160(&vgg16);
  send_model_task(vgg160, c);
  return 0;
}
