#include <nandroid/web.h>
#include <stdlib.h>
#include <string.h>

static uv_loop_t* loop;
static uv_tcp_t server;

static http_parser_settings settings;
static nan_http_server* nan_server;

void
nan_http_server_init(nan_http_server* serv){
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
  req->buff = uv_buf_init(malloc(1024 * 1024), 1024 * 1024);

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

int
http_message_complete_cb(http_parser* parser){
  http_request* req = parser->data;

  for(int i = 0; i < 20; i++){
    http_router* router = &nan_server->routers[i];
    http_route* route = http_router_find(router, req->url);
    if(route != NULL){
      route->response_handler(&req->stream, req);
      return 0;
    }
  }

  uv_write_t* write = malloc(sizeof(uv_write_t));
  uv_buf_t buf = uv_buf_init(RESPONSE, sizeof(RESPONSE));
  uv_write(write, &req->stream, &buf, 1, &on_uv_write_cb);
  return 0;
}