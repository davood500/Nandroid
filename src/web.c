#include <nandroid/web.h>
#include <stdlib.h>
#include <string.h>

#define NAN_VERSION \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: application/json\r\n" \
  "\r\n" \
  "%s\n"

static char* version = "{ \"version\": \"0.0.0.0\" }";

static void
handle_api_version(uv_stream_t* stream, struct _http_request* request) {
  size_t len = (strlen(NAN_VERSION) - 2) + strlen(version);
  char* response = malloc(len + 1);
  snprintf(response, len, NAN_VERSION, version);
  response[len] = '\0';

  uv_buf_t buffer;
  buffer.base = malloc(strlen(response));
  buffer.len = strlen(response);
  strncpy(buffer.base, response, strlen(response));

  free(response);

  uv_write_t* write = malloc(sizeof(uv_write_t));
  uv_write(write, stream, &buffer, 1, &on_uv_write_cb);
}

static void
setup_api(http_router* router) {
  http_route* version = malloc(sizeof(http_route));
  http_route_init(version, "/version");
  version->response_handler = &handle_api_version;

  router->routes[0] = version;
}

void
nan_http_server_route(nan_http_server* server) {
  http_router* api = malloc(sizeof(http_router));
  http_router_init(api, "/api");
  setup_api(api);

  server->routers[0] = api;
}
