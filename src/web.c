#include <nandroid/web.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <nandroid/web/http_router.h>

#define NAN_VERSION \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: application/json\r\n" \
  "\r\n" \
  "{ \"Nandroid Version\": \"0.0.0.0\" }\n"

static void
handle_api_version(uv_stream_t* stream, struct _http_request* request) {
  uv_write_t* write = malloc(sizeof(uv_write_t));
  uv_buf_t buff = uv_buf_init(NAN_VERSION, sizeof(NAN_VERSION));
  uv_write(write, stream, &buff, 1, on_uv_write_cb);
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
