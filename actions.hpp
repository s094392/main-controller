#ifndef ACTIONS_HPP
#define ACTIONS_HPP

#include "model.hpp"
#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <vector>

void send_model(redisContext *c, ForwardTask &task) {
  redisReply *reply;
  std::string cmd = "forward " + std::to_string(task.model->id) + " " +
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

void send_send(redisContext *c0, redisContext *c1, int src, int dst) {
  redisReply *reply;
  std::string cmd = "send " + std::to_string(src) + " " + std::to_string(dst);
  std::cout << cmd << std::endl;
  reply = (redisReply *)redisCommand(c0, "RPUSH foo %s", cmd.c_str());

  cmd = "recv " + std::to_string(src) + " " + std::to_string(dst);
  std::cout << cmd << std::endl;
  reply = (redisReply *)redisCommand(c1, "RPUSH foo %s", cmd.c_str());
}

void send_create(redisContext *c, int batch_size) {
  redisReply *reply;
  std::string cmd = "create " + std::to_string(batch_size);
  std::cout << cmd << std::endl;
  reply = (redisReply *)redisCommand(c, "RPUSH foo %s", cmd.c_str());
}
#endif
