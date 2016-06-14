#ifndef NANDROID_HTTP_SERVER_H
#define NANDROID_HTTP_SERVER_H

#include <stddef.h>
#include <uv.h>
#include <http_parser.h>

typedef struct{
  char* field;
  char* value;
  size_t field_length;
  size_t value_length;
} http_header;

typedef struct _http_request{
  uv_write_t req;
  uv_stream_t stream;
  http_parser* parser;
  char* url;
  char* method;
  int headers_length;
  http_header headers[20];
  char* body;
  uv_buf_t buff;
} http_request;

void tcp_connection_new_cb(uv_stream_t* server, int status);
void on_uv_alloc_cb(uv_handle_t* handle, size_t size, uv_buf_t* buff);
void on_uv_read_cb(uv_stream_t* client, ssize_t nread, const uv_buf_t* buff);
void on_uv_write_cb(uv_write_t* write, int status);
void on_uv_close_cb(uv_handle_t* handle);
void on_uv_shutdown_cb(uv_shutdown_t* req, int status);

int http_message_begin_cb(http_parser* parser);
int http_url_cb(http_parser* parser, const char* chunk, size_t size);
int http_header_field_cb(http_parser* parser, const char* chunk, size_t size);
int http_header_value_cb(http_parser* parser, const char* chunk, size_t size);
int http_header_complete_cb(http_parser* parser);
int http_body_cb(http_parser* parser, const char* chunk, size_t size);
int http_message_complete_cb(http_parser* parser);

#endif