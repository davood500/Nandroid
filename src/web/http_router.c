#include <nandroid/web/http_router.h>
#include <string.h>
#include <stdlib.h>

void
http_route_init(http_route* route, char* name){
  route->name = strdup(name);
  route->name_length = strlen(name);
}

void
http_router_init(http_router* router, char* name){
  router->name = strdup(name);
  router->name_length = strlen(name);
}

static int
strstrwith(char* pre, char* str){
  size_t lenpre = strlen(pre);
  size_t lenstr = strlen(str);
  return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

static char*
strsubstr(char* str, int start, int size){
  char* new_str = malloc((size_t) size);
  strncpy(new_str, &str[start], (size_t) size);
  return new_str;
}

http_route* http_router_find(http_router* router, char* name){
  char* route;
  if(strstrwith(router->name, name)){
    route = strsubstr(name, (int) router->name_length, (int) ((strlen(name) - router->name_length) + 1));
    route[strlen(name) - router->name_length] = '\0';
  } else{
    return NULL;
  }

  for(int i = 0; i < 20; i++){
    http_route* r = &router->routes[i];
    if(r != NULL){
      if(strncmp(r->name, route, sizeof(route)) == 0){
        free(route);
        return r;
      }
    }
  }

  free(route);
  return NULL;
}