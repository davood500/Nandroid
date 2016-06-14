#ifndef NANDROID_HTTP_ROUTER_H
#define NANDROID_HTTP_ROUTER_H

#include <stddef.h>
#include <uv.h>
#include "../common.h"

HEADER_BEGIN

typedef struct http_route{
  char* name;
  size_t name_length;
  void (*response_handler)(uv_stream_t* stream, struct _http_request* request);
} http_route;

void http_route_init(http_route* route, char* name);

typedef struct{
  char* name;
  size_t name_length;

  http_route routes[20];
} http_router;

void http_router_init(http_router* router, char* name);
http_route* http_router_find(http_router* router, char* route);

HEADER_END

#endif