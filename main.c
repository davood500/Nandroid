#include <nandroid/web.h>
#include <stdlib.h>

int main(int argc, char** argv){
  nan_http_server server;
  nan_http_server_init(&server);
  nan_http_server_route(&server);
  nan_http_server_run(&server);
}