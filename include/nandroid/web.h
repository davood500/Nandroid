#ifndef NANDROID_WEB_H
#define NANDROID_WEB_H

#include <uv.h>
#include <http_parser.h>
#include "./web/http_server.h"
#include "./web/http_router.h"

typedef struct{
  http_router* routers[20];
  uv_loop_t* loop;
  sqlite3* database;
} nan_http_server;

void nan_http_server_init(nan_http_server* server);
void nan_http_server_route(nan_http_server* server);
void nan_http_server_run(nan_http_server* server);

#endif