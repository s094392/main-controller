#include "model.hpp"

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

int main() {
  redisContext *c = redis_init("REDIS0");
  Model vgg16(0, "vgg16", vgg16_layer);
  ModelTask vgg160(&vgg16);
  send_model_task(vgg160, c);
  return 0;
}
