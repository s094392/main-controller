#include <hiredis/hiredis.h>
#include <iostream>
#include <string>

void serve(std::string redis) {
  redisContext *c = redisConnect(getenv(redis.c_str()), 6379);

  if (c == NULL || c->err) {
    if (c) {
      std::cout << "Error: " << c->errstr;
    } else {
      std::cout << "Can't allocate redis context";
    }
  }

  const int vgg16_layer = 37;

  redisReply *reply;

  for (int i = 0; i < vgg16_layer; i++) {
    std::string cmd = "0 " + std::to_string(i);
    std::cout << cmd << std::endl;
    reply = (redisReply *)redisCommand(c, "RPUSH foo %s", cmd.c_str());
  }
}

int main() {
  serve("REDIS0");
  serve("REDIS1");
  return 0;
}
