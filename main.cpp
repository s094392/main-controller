#include "actions.hpp"

#include "model.hpp"
#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <unistd.h>
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

int main() {
  redisContext *c0 = redis_init("REDIS0");
  redisContext *c1 = redis_init("REDIS1");

  vector<Model> models;
  int n = get_models_from_json(models, "schema.json");
  ModelTask vgg160(&models[0]);
  ModelTask vgg161(&models[0]);

  for (int t = 0; t < 10; t++) {
    send_create(c0, 20);
    send_model_task(c0, vgg160);
  }

  vector<int> batches = {64, 32, 16, 8, 4, 2, 1};

  for (auto &batch : batches) {
    for (int t = 0; t < 10; t++) {
      send_create(c0, batch);
      // send_create(c1, batch);
      send_model_task(c0, vgg160);
      // send_model_task(c1, vgg160);
    }

    // for (int t = 0; t < 10; t++) {
    //   send_create(c0, batch);
    //   for (int i = 0; i < vgg161.tasks.size(); i++) {
    //     if (i % 2 == 0) {
    //       send_model(c0, vgg160.tasks[i]);
    //       send_send(c0, c1, 0, 1);
    //     } else {
    //       send_model(c1, vgg161.tasks[i]);
    //       send_send(c1, c0, 1, 0);
    //     }
    //   }
    // }
  }
  return 0;
}
