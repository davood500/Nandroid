#include <nandroid/web.h>
#include <stdlib.h>

#define NAN_VERSION \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: application/json\r\n" \
  "\r\n" \
  "{ \"Nandroid Version\": \"0.0.0.0\" }\n"

static void
handle_api_version(uv_stream_t* stream, void on_uv_write_cb(uv_write_t* write, int status), struct _http_request* request){
  uv_write_t* write = malloc(sizeof(uv_write_t));
  uv_buf_t buff = uv_buf_init(NAN_VERSION, sizeof(NAN_VERSION));
  uv_write(write, stream, &buff, 1, on_uv_write_cb);
}

static void
run_web_panel(){

}

int main(int argc, char** argv){
  nan_http_server server;
  nan_http_server_init(&server);
  nan_http_server_route(&server);
  nan_http_server_run(&server);
}