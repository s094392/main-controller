#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <vector>

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
  std::string type;
};

class ForwardTask : Task {
public:
  Model *model;
  int layer_id;
  ForwardTask(Model *model, int layer_id) : model(model), layer_id(layer_id) {}
};

class ModelTask {
public:
  Model *model;
  std::vector<ForwardTask> tasks;
  ModelTask(Model *model) : model(model) {
    tasks.reserve(model->layer);
    for (int i = 0; i < model->layer; i++) {
      tasks.push_back(ForwardTask(model, i));
    }
  }
};

void send_model(redisContext *c, ForwardTask &task) {
  redisReply *reply;
  std::string cmd = "forward " + std::to_string(task.model->model_id) + " " +
                    std::to_string(task.layer_id);
  std::cout << cmd << std::endl;
  reply = (redisReply *)redisCommand(c, "RPUSH foo %s", cmd.c_str());
}

void send_model_task(redisContext *c, ModelTask &model_task) {
  for (auto &task : model_task.tasks) {
    send_model(c, task);
  }
}

void send_push(redisContext *c, int variable_id) {
  redisReply *reply;
  std::string cmd = "push " + std::to_string(variable_id);
  std::cout << cmd << std::endl;
  reply = (redisReply *)redisCommand(c, "RPUSH foo %s", cmd.c_str());
}

void send_pop(redisContext *c, int variable_id) {
  redisReply *reply;
  std::string cmd = "pop " + std::to_string(variable_id);
  std::cout << cmd << std::endl;
  reply = (redisReply *)redisCommand(c, "RPUSH foo %s", cmd.c_str());
}
