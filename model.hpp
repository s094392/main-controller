#ifndef MODEL_HPP
#define MODEL_HPP

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

#endif // !MODEL_HPP
