#include <nandroid/web.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static char* cwd;

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

static uv_loop_t* loop;

void fs_uv_on_read(uv_fs_t* req);
void fs_uv_on_open(uv_fs_t* req);
void fs_uv_on_close(uv_fs_t* req);

uv_fs_t open_req;
uv_fs_t read_req;
uv_fs_t close_req;

typedef struct{
  uv_stream_t* stream;
  size_t file_size;
  char* mime;
  uv_buf_t buff;
} http_serve_response;

static char*
get_mime(char* file_name){
  char* ext = strrchr(file_name, '.');
  ext += 1;

  if(strncmp(ext, "html", strlen(ext)) == 0){
    return "text/html";
  } else if(strncmp(ext, "json", strlen(ext)) == 0){
    return "application/json";
  } else{
    return "text/plain";
  }
}

static void
handle_serve_file(uv_stream_t* stream, struct _http_request* request) {
  char* path = malloc(strlen(cwd) + strlen(request->url) + 1);
  memcpy(path, cwd, strlen(cwd));
  memcpy(path + strlen(cwd), request->url, strlen(request->url));
  path[strlen(cwd) + strlen(request->url)] = '\0';

  FILE* f = fopen(path, "rb");
  if(f == NULL) abort();
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fclose(f);

  http_serve_response* resp = malloc(sizeof(http_serve_response));
  resp->stream = stream;
  resp->file_size = (size_t) size;
  resp->mime = get_mime(path);
  resp->buff.len = (size_t) size;
  resp->buff.base = malloc((size_t) size);

  open_req.data = resp;
  uv_fs_open(loop, &open_req, path, O_RDONLY, 0, &fs_uv_on_open);
}

void
fs_uv_on_open(uv_fs_t* req){
  http_serve_response* h_resp = ((http_serve_response*) req->data);
  if(req->result != -1){
    uv_fs_req_cleanup(req);
    uv_fs_read(loop, &read_req, (uv_file) req->result, &h_resp->buff, 1, -1, &fs_uv_on_read);
  } else{
    fprintf(stderr, "Error opening file\n");
    abort();
  }
}

#define NAN_RESPONSE_TEMPLATE \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: %s\r\n" \
  "\r\n" \
  "%s\n"

void
fs_uv_on_read(uv_fs_t* req){
  uv_fs_req_cleanup(req);
  if(req->result < 0){
    fprintf(stderr, "Read error\n");
    abort();
  } else if(req->result == 0){
    uv_fs_close(loop, &close_req, (uv_file) open_req.result, NULL);
  } else{
    uv_write_t* write = malloc(sizeof(uv_write_t));

    http_serve_response* r = ((http_serve_response*) (open_req.data));
    char* data = malloc(r->buff.len + 1);
    memcpy(data, r->buff.base, r->buff.len);
    data[r->buff.len] = '\0';

    size_t size = (strlen(NAN_RESPONSE_TEMPLATE) - 4) + strlen(r->mime) + strlen(data);
    r->buff.base = realloc(r->buff.base, size + 1);
    r->buff.len = size;
    snprintf(r->buff.base, size, NAN_RESPONSE_TEMPLATE, r->mime, data);
    r->buff.base[size] = '\0';

    uv_write(write, r->stream, &r->buff, 1, &on_uv_write_cb);
    uv_fs_close(loop, &close_req, (uv_file) open_req.result, &fs_uv_on_close);
  }
}

void
fs_uv_on_close(uv_fs_t* req){
  int result = (int) req->result;
  if(result == -1){
    fprintf(stderr, "Error closing file: %s\n", uv_strerror(result));
    abort();
  }

  uv_fs_req_cleanup(req);
}

static void
setup_api(http_router* router) {
  http_route version;
  http_route_init(&version, "/version");
  version.response_handler = &handle_api_version;

  router->routes[0] = version;
}

static void
setup_home(http_router* router){
  http_route index;
  http_route_init(&index, "index.html");
  index.response_handler = &handle_serve_file;

  router->routes[0] = index;
}

void
nan_http_server_route(nan_http_server* server) {
  loop = server->loop;
  cwd = malloc(1204);
  if (getcwd(cwd, 1024) == NULL) {
    perror("getcwd() error\n");
  }

  http_router api;
  http_router_init(&api, "/api");
  setup_api(&api);

  http_router home;
  http_router_init(&home, "/");
  setup_home(&home);

  server->routers[0] = api;
  server->routers[1] = home;
}
