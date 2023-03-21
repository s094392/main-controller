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
  Model vgg16(0, "vgg16", vgg16_layer);
  ModelTask vgg160(&vgg16);
  ModelTask vgg161(&vgg16);

  // send_model_task(c0, vgg160);

  for (int t = 0; t < 5; t++) {
    send_create(c0, 8);
    send_model_task(c0, vgg160);
  }
  for (int t = 0; t < 5; t++) {
    send_create(c0, 8);
    for (int i = 0; i < vgg161.tasks.size(); i++) {
      if (i % 2 == 0) {
        send_model(c0, vgg160.tasks[i]);
        send_send(c0, c1, 0, 1);
      } else {
        send_model(c1, vgg161.tasks[i]);
        send_send(c1, c0, 1, 0);
      }
    }
  }
  return 0;
}
