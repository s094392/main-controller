#ifndef ACTIONS_HPP
#define ACTIONS_HPP

#include "model.hpp"
#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

void _send_cmd(redisContext *c, string cmd) {
  redisReply *reply;
  reply = (redisReply *)redisCommand(c, "RPUSH foo %s", cmd.c_str());
}

void send_model(redisContext *c, ForwardTask &task) {
  string cmd = "forward " + to_string(task.task_id) + " " +
               to_string(task.model->id) + " " + to_string(task.layer_id) +
               " " + to_string(task.model_task->pos);
  if (task.layer_id == 0) {
    task.model_task->start_time =
        chrono::duration_cast<chrono::microseconds>(
            chrono::system_clock::now().time_since_epoch())
            .count();
  }
  _send_cmd(c, cmd);
}

void send_model_task(redisContext *c, ModelTask &model_task) {
  for (auto &task : model_task.tasks) {
    send_model(c, task);
  }
}

void send_push(redisContext *c, int variable_id) {
  string cmd = "push " + to_string(variable_id);
  _send_cmd(c, cmd);
}

void send_pop(redisContext *c, int variable_id) {
  string cmd = "pop " + to_string(variable_id);
  _send_cmd(c, cmd);
}

void send_send(redisContext *c0, redisContext *c1, int src, int dst) {
  string cmd = "send " + to_string(src) + " " + to_string(dst);
  _send_cmd(c0, cmd);

  cmd = "recv " + to_string(src) + " " + to_string(dst);
  _send_cmd(c1, cmd);
}

void send_create(redisContext *c, int batch_size) {
  string cmd = "create " + to_string(batch_size);
  _send_cmd(c, cmd);
}
#endif
