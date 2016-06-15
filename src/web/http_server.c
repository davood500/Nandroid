#include <nandroid/web.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static uv_loop_t* loop;
static uv_tcp_t server;

static http_parser_settings settings;
static nan_http_server* nan_server;

static char cwd[1024];

#define DATABASE "/nandroid.sqlite"

void
nan_http_server_init(nan_http_server* serv){
  getcwd(cwd, 1024);
  char* database_path = malloc(strlen(cwd) + strlen(DATABASE) + 1);
  memcpy(database_path, cwd, strlen(cwd));
  memcpy(database_path + strlen(cwd), DATABASE, strlen(DATABASE));
  database_path[strlen(cwd) + strlen(DATABASE)] = '\0';
  if(sqlite3_open(database_path, &serv->database)){
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(serv->database));
    abort();
  }
  free(database_path);
  memcpy(&cwd[strlen(cwd)], "/www\0", 5);

  printf("%s\n", cwd);

  for(int i = 0; i < 20; i++){
    serv->routers[i] = NULL;
  }

  struct sockaddr_in addr;
  uv_ip4_addr("127.0.0.1", 8080, &addr);
  loop = uv_default_loop();
  serv->loop = loop;

  settings.on_url = &http_url_cb;
  settings.on_body = &http_body_cb;
  settings.on_header_field = &http_header_field_cb;
  settings.on_header_value = &http_header_value_cb;
  settings.on_headers_complete = &http_header_complete_cb;
  settings.on_message_begin = &http_message_begin_cb;
  settings.on_message_complete = &http_message_complete_cb;

  uv_tcp_init(loop, &server);
  uv_tcp_bind(&server, ((const struct sockaddr*) &addr), 0);

  int resp = uv_listen(((uv_stream_t*) &server), 128, &tcp_connection_new_cb);
  if(resp){
    fprintf(stderr, "Error on tcp listen: %s\n", uv_strerror(resp));
    abort();
  }
}

void
nan_http_server_run(nan_http_server* server){
  nan_server = server;
  uv_run(loop, UV_RUN_DEFAULT);
}

void
tcp_connection_new_cb(uv_stream_t* server, int status){
  http_request* req = malloc(sizeof(http_request));
  if(status == -1) {
    fprintf(stderr, "Error on connection: %s\n", uv_strerror(status));
    abort();
  }

  uv_tcp_init(loop, ((uv_tcp_t*) &req->stream));
  req->parser = malloc(sizeof(http_parser));
  req->stream.data = req;
  req->parser->data = req;
  req->database = nan_server->database;

  if(uv_accept(server, &req->stream) == 0){
    http_parser_init(req->parser, HTTP_REQUEST);
    uv_read_start(&req->stream, &on_uv_alloc_cb, &on_uv_read_cb);
  }
}

void
on_uv_alloc_cb(uv_handle_t* handle, size_t size, uv_buf_t* buff){
  *buff = uv_buf_init(malloc(size), (unsigned int) size);
}

void
on_uv_read_cb(uv_stream_t* client, ssize_t nread, const uv_buf_t* buff){
  if(nread > 0){
    http_request* req = client->data;
    size_t parsed = http_parser_execute(req->parser, &settings, buff->base, (size_t) nread);
    if(parsed < nread){
      printf("Closing\n");
      uv_close((uv_handle_t*) client, &on_uv_close_cb);
    }
  } else{
    if(nread == UV_EOF){
      // Fallthrough
    } else{
      fprintf(stderr, "read: %s\n", uv_strerror((int) nread));
    }

    uv_shutdown_t* shutdown_req = malloc(sizeof(uv_shutdown_t));
    uv_shutdown(shutdown_req, client, &on_uv_shutdown_cb);
  }

  free(buff->base);
}

void
on_uv_close_cb(uv_handle_t* handle){
  http_request* req = handle->data;
  free(req);
}

void
on_uv_shutdown_cb(uv_shutdown_t* req, int status){
  uv_close((uv_handle_t*) req->handle, &on_uv_close_cb);
  free(req);
}

int
http_message_begin_cb(http_parser* parser){
  http_request* req = parser->data;
  req->headers_length = 0;
  return 0;
}

int
http_url_cb(http_parser* parser, const char* chunk, size_t len){
  http_request* req = parser->data;
  req->url = malloc(len + 1);
  strncpy(req->url, chunk, len);
  req->url[len] = '\0';
  return 0;
}

int
http_header_field_cb(http_parser* parser, const char* chunk, size_t len){
  http_request* req = parser->data;
  http_header* header = &req->headers[req->headers_length];
  header->field = malloc(len + 1);
  header->field_length = len;
  strncpy(header->field, chunk, len);
  header->field[len] = '\0';
  return 0;
}

int
http_header_value_cb(http_parser* parser, const char* chunk, size_t len){
  http_request* req = parser->data;
  http_header* header = &req->headers[req->headers_length];
  header->value_length = len;
  header->value = malloc(len + 1);
  strncpy(header->value, chunk, len);
  header->value[len] = '\0';
  req->headers_length++;
  return 0;
}

int
http_header_complete_cb(http_parser* parser){
  http_request* req = parser->data;
  const char* method = http_method_str((enum http_method) parser->method);
  req->method = malloc(sizeof(method));
  strncpy(req->method, method, strlen(method));
  return 0;
}

int
http_body_cb(http_parser* parser, const char* chunk, size_t len){
  http_request* req = parser->data;
  req->body = malloc(len + 1);
  strncpy(req->body, chunk, len);
  req->body[len] = '\0';
  return 0;
}

void
on_uv_write_cb(uv_write_t* write, int status){
  uv_close((uv_handle_t*) write->handle, &on_uv_close_cb);
  free(write);
}

#define RESPONSE \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: text/html\r\n" \
  "\r\n" \
  "<html><body><p>Error cannot find resource</p></body></html>\n"

static char*
get_mime(char* file_name){
  char* ext = strrchr(file_name, '.');
  ext += 1;

  if(strncmp(ext, "html", strlen(ext)) == 0){
    return "text/html";
  } else if(strncmp(ext, "json", strlen(ext)) == 0){
    return "application/json";
  } else if(strncmp(ext, "js", strlen(ext)) == 0){
    return "application/javascript";
  } else{
    return "text/plain";
  }
}

#define NAN_RESPONSE_TEMPLATE \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: %s\r\n" \
  "\r\n" \
  "%s\n"

uv_fs_t open_req;
uv_fs_t read_req;
uv_fs_t close_req;

void uv_fs_on_open_cb(uv_fs_t* req);
void uv_fs_on_read_cb(uv_fs_t* req);
void uv_fs_on_close_cb(uv_fs_t* req);

typedef struct{
  long fsize;
  char* mime;
  http_request* request;
  uv_buf_t buffer;
} http_response;

static void
handle_serve_file(char* path, uv_stream_t* stream, http_request* request) {
  http_response* resp = malloc(sizeof(http_response));
  resp->request = request;
  resp->mime = strdup(get_mime(path));

  FILE* f = fopen(path, "rb");
  if(f == NULL) abort();
  fseek(f, 0, SEEK_END);
  resp->fsize = ftell(f);
  fclose(f);

  open_req.data = resp;
  uv_fs_open(loop, &open_req, path, O_RDONLY, S_IRUSR, &uv_fs_on_open_cb);
}

void
uv_fs_on_open_cb(uv_fs_t* req){
  int result = (int) req->result;
  if(result == -1){
    fprintf(stderr, "Error opening file: %s\n", uv_strerror(result));
  }

  http_response* resp = ((http_response*) open_req.data);
  resp->buffer = uv_buf_init(malloc((size_t) resp->fsize), (unsigned int) resp->fsize);

  uv_fs_req_cleanup(req);
  uv_fs_read(loop, &read_req, result, &resp->buffer, 1, -1, &uv_fs_on_read_cb);
}

void
uv_fs_on_read_cb(uv_fs_t* req){
  int result = (int) req->result;
  if(result == -1){
    fprintf(stderr, "Error reading file: %s\n", uv_strerror(result));
  }

  http_response* resp = ((http_response*) open_req.data);

  uv_fs_req_cleanup(req);

  size_t len = (strlen(NAN_RESPONSE_TEMPLATE) - 4) + strlen(resp->mime) + resp->buffer.len;
  char* response = malloc(len + 1);
  snprintf(response, len, NAN_RESPONSE_TEMPLATE, resp->mime, resp->buffer);
  response[len] = '\0';

  uv_buf_t buffer;
  buffer.base = malloc(strlen(response));
  buffer.len = strlen(response);
  strncpy(buffer.base, response, strlen(response));

  free(response);

  uv_write_t* write = malloc(sizeof(uv_write_t));
  uv_write(write, &resp->request->stream, &buffer, 1, &on_uv_write_cb);
  uv_fs_close(loop, &close_req, (uv_file) open_req.result, &uv_fs_on_close_cb);
}

void
uv_fs_on_close_cb(uv_fs_t* req){
  int result = (int) req->result;
  if(result == -1){
    fprintf(stderr, "Error closing file: %s\n", uv_strerror(result));
  }

  uv_fs_req_cleanup(req);
}

int
http_message_complete_cb(http_parser* parser){
  http_request* req = parser->data;

  if((strlen(req->url) == 1) && (strncmp(req->url, "/", 1) == 0)){
    char* path = malloc(strlen(cwd) + strlen("/index.html") + 1);
    memcpy(path, cwd, strlen(cwd));
    memcpy(path + strlen(cwd), "/index.html", strlen("/index.html"));
    path[strlen(cwd) + strlen("/index.html")] = '\0';

    handle_serve_file(path, &req->stream, req);
    free(path);
    return 0;
  }

  for(int i = 0; i < 20; i++){
    http_router* router = nan_server->routers[i];
    if(router != NULL){
      http_route* route = http_router_find(router, req->url);
      if(route != NULL){
        route->response_handler(&req->stream, req);
        return 0;
      }
    }
  }

  char* path = malloc(strlen(cwd) + strlen(req->url) + 1);
  memcpy(path, cwd, strlen(cwd));
  memcpy(path + strlen(cwd), req->url, strlen(req->url));
  path[strlen(cwd) + strlen(req->url)] = '\0';

  printf("Serving: %s\n", req->url);

  if(access(path, F_OK) != -1){
    handle_serve_file(path, &req->stream, req);
  } else{
    uv_write_t* write = malloc(sizeof(uv_write_t));
    uv_buf_t buf = uv_buf_init(RESPONSE, sizeof(RESPONSE));
    uv_write(write, &req->stream, &buf, 1, &on_uv_write_cb);
  }

  free(path);
  return 0;
}