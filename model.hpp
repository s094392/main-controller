#ifndef MODEL_HPP
#define MODEL_HPP

#include "json.hpp"
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

using json = nlohmann::json;
using namespace std;

enum class op { conv, relu, maxpool, linear, flat };

class Layer {
public:
  op type;
  Layer(op type) : type(type){};
};

class Model {
public:
  int id;
  string name;
  Model(int id, string name) : id(id), name(name){};
  size_t size() { return layer_data.size(); }

  void add_conv_layer(int in_channels, int out_channels, int filter_size,
                      int stride, int padding) {
    this->layer_data.push_back(Layer(op::conv));
  }
  void add_relu_layer() { this->layer_data.push_back(Layer(op::relu)); }

  void add_maxpool_layer(int size, int stride) {
    this->layer_data.push_back(Layer(op::maxpool));
  }
  void add_linear_layer(int in_channels, int out_channels) {
    this->layer_data.push_back(Layer(op::linear));
  }
  void add_flat_layer() { this->layer_data.push_back(Layer(op::flat)); }

private:
  vector<Layer> layer_data;
};

class Task {
public:
  int task_id;
  enum status { todo, done } status;
  std::string type;
  Task() : status(status::todo) {}
};

class ModelTask;

class ForwardTask : public Task {
public:
  Model *model;
  ModelTask *model_task;
  int layer_id;
  ForwardTask(Model *model, int layer_id, ModelTask *model_task)
      : model(model), layer_id(layer_id), model_task(model_task), Task() {}
};

class ModelTask {
public:
  Model *model;
  int pos;
  int start_time;
  int end_time;
  std::vector<ForwardTask> tasks;
  ModelTask(Model *model) : model(model) {
    tasks.reserve(model->size());
    for (int i = 0; i < model->size(); i++) {
      tasks.push_back(ForwardTask(model, i, this));
    }
  }
};

int get_models_from_json(vector<Model> &models, string filename) {
  ifstream f(filename);
  json data = json::parse(f);
  for (int i = 0; i < data["Models"].size(); i++) {
    models.push_back(Model(i, data["Models"][i]["name"]));
    for (auto &layer : data["Models"][i]["layers"]) {
      if (layer["type"] == "conv") {
        models[i].add_conv_layer(
            layer["params"]["in_channels"], layer["params"]["out_channels"],
            layer["params"]["filter_size"], layer["params"]["stride"],
            layer["params"]["padding"]);
      } else if (layer["type"] == "relu") {
        models[i].add_relu_layer();
      } else if (layer["type"] == "maxpool") {
        models[i].add_maxpool_layer(layer["params"]["ksize"],
                                    layer["params"]["kstride"]);
      } else if (layer["type"] == "linear") {
        models[i].add_linear_layer(layer["params"]["in_channels"],
                                   layer["params"]["out_channels"]);
      } else if (layer["type"] == "flat") {
        models[i].add_flat_layer();
      }
    }
  }
  return models.size();
}

#endif // !MODEL_HPP
