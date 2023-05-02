#ifndef ACTIONS_HPP
#define ACTIONS_HPP

#include "model.hpp"
#include "utils.hpp"
#include <hiredis/hiredis.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>

using namespace std;

void _send_cmd(redisContext *c, string queue_name, char *cmd) {
  char buffer[50];
  sprintf(buffer, "RPUSH %s %%s", queue_name.c_str());
  redisReply *reply;
  // cout << cmd << endl;
  reply = (redisReply *)redisCommand(c, buffer, cmd);
}
#endif
