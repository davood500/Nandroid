#include <nandroid/web.h>
#include <stdlib.h>
#include <string.h>
#include <nandroid/web/http_router.h>

#define JSON_RESPONSE \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: application/json\r\n" \
  "\r\n" \
  "%s\n"

static char* version = "{ \"version\": \"0.0.0.0\" }";

static void
handle_api_version(uv_stream_t* stream, http_request* request) {
  size_t len = (strlen(JSON_RESPONSE) - 2) + strlen(version);
  char* response = malloc(len + 1);
  snprintf(response, len, JSON_RESPONSE, version);
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

static void
send_json(uv_stream_t* stream, char* json){
  size_t len = (strlen(JSON_RESPONSE) - 2) + strlen(json);
  char* response = malloc(len + 1);
  snprintf(response, len, JSON_RESPONSE, json);
  response[len] = '\0';

  uv_buf_t buffer;
  buffer.base = malloc(strlen(response));
  buffer.len = strlen(response);
  strncpy(buffer.base, response, strlen(response));

  free(response);

  uv_write_t* write = malloc(sizeof(uv_write_t));
  uv_write(write, stream, &buffer, 1, &on_uv_write_cb);
}

#define SELECT_SETTING "SELECT * FROM settings WHERE name = '%s';"

static int
get_greeting_cb(void* arg, int argc, char** argv, char** colNames){
  char* format = "{ \"greeting\": \"%s\" }";
  char* resp = malloc(strlen(format) - 2 + strlen(argv[1]) + 1);
  snprintf(resp, strlen(format) - 2 + strlen(argv[1]), format, argv[1]);
  resp[strlen(format) - 2 + strlen(argv[1])] = '\0';
  send_json(((uv_stream_t*) arg), resp);
  free(resp);
  return 0;
}

static void
handle_get_greeting(uv_stream_t* stream, http_request* req){
  char* sql = malloc(strlen(SELECT_SETTING) - 2 + strlen("greeting") + 1);
  snprintf(sql, strlen(SELECT_SETTING) - 2 + strlen("greeting"), SELECT_SETTING, "greeting");
  sql[strlen(SELECT_SETTING) - 2 + strlen("greeting")] = '\0';

  int code;
  char* errMsg;
  if((code = sqlite3_exec(req->database, sql, &get_greeting_cb, stream, &errMsg)) != SQLITE_OK){
    free(sql);
    send_json(stream, "{ \"error\": \"executing sqlite lookup\" }");
    return;
  }

  free(sql);
}

#define SET_SETTING "UPDATE settings SET value = '%s' WHERE name = '%s';"

static void
handle_set_greeting(uv_stream_t* stream, http_request* req){
  size_t sql_len = strlen(SET_SETTING) + strlen(req->body) + strlen("greeting");
  char* sql = malloc(sql_len + 1);
  snprintf(sql, sql_len, SET_SETTING, req->body, "greeting");
  sql[sql_len] = '\0';

  printf("%s\n", sql);

  int code;
  char* errMsg;
  if((code = sqlite3_exec(req->database, sql, NULL, NULL, &errMsg)) != SQLITE_OK){
    free(sql);
    printf("%s\n", sqlite3_errmsg(req->database));
    send_json(stream, "{ \"error\": \"executing sqlite update\" }");
    return;
  }

  free(sql);
  send_json(stream, "{ \"status\": 200 }");
}

static void
setup_data(http_router* router){
  http_route* get_greeting = malloc(sizeof(http_route));
  http_route_init(get_greeting, "/get_greeting");
  get_greeting->response_handler = &handle_get_greeting;

  http_route* set_greeting = malloc(sizeof(http_route));
  http_route_init(set_greeting, "/set_greeting");
  set_greeting->response_handler = &handle_set_greeting;

  router->routes[0] = get_greeting;
  router->routes[1] = set_greeting;
}

void
nan_http_server_route(nan_http_server* server) {
  http_router* api = malloc(sizeof(http_router));
  http_router_init(api, "/api");
  setup_api(api);

  http_router* api_data = malloc(sizeof(http_router));
  http_router_init(api_data, "/api/data");
  setup_data(api_data);

  server->routers[0] = api;
  server->routers[1] = api_data;
}
