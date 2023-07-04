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

enum class op { conv, relu, maxpool, linear, flat, avgpool, lstm, tf };

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

  void add_conv_layer() { this->layer_data.push_back(Layer(op::conv)); }
  void add_relu_layer() { this->layer_data.push_back(Layer(op::relu)); }
  void add_maxpool_layer() { this->layer_data.push_back(Layer(op::maxpool)); }
  void add_linear_layer() { this->layer_data.push_back(Layer(op::linear)); }
  void add_flat_layer() { this->layer_data.push_back(Layer(op::flat)); }
  void add_avgpool_layer() { this->layer_data.push_back(Layer(op::avgpool)); }
  void add_lstm_layer() { this->layer_data.push_back(Layer(op::lstm)); }
  void add_tf_layer() { this->layer_data.push_back(Layer(op::tf)); }

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
  Model &model;
  ModelTask &model_task;
  int layer_id;
  ForwardTask(Model &model, int layer_id, ModelTask &model_task)
      : model(model), layer_id(layer_id), model_task(model_task), Task() {}
};

class ModelTask {
public:
  Model &model;
  int pos;
  long long int arrival_time;
  long long int start_time;
  long long int end_time;
  int worker;
  std::vector<ForwardTask> tasks;
  ModelTask(Model &model, int arrival_time)
      : model(model), arrival_time(arrival_time) {
    worker = -1;
    tasks.reserve(model.size());
  }
  void create_tasks() {
    for (int i = 0; i < model.size(); i++) {
      tasks.emplace_back(ForwardTask(model, i, *this));
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
        models[i].add_conv_layer();
      } else if (layer["type"] == "relu") {
        models[i].add_relu_layer();
      } else if (layer["type"] == "maxpool") {
        models[i].add_maxpool_layer();
      } else if (layer["type"] == "linear") {
        models[i].add_linear_layer();
      } else if (layer["type"] == "flat") {
        models[i].add_flat_layer();
      } else if (layer["type"] == "avgpool") {
        models[i].add_avgpool_layer();
      } else if (layer["type"] == "lstm") {
        models[i].add_lstm_layer();
      } else if (layer["type"] == "tf") {
        models[i].add_tf_layer();
      }
    }
  }
  return models.size();
}

#endif // !MODEL_HPP
