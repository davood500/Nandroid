cmake_minimum_required(VERSION 3.3)
project(Nandroid)

find_package(Threads)

include_directories(include)

add_executable(Nandroid main.c include/nandroid/common.h include/nandroid/scheme/ns_common.h include/nandroid/scheme/ns_gc.h src/scheme/ns_gc.c include/nandroid/scheme/ns_object.h include/nandroid/scheme/ns_vm.h src/scheme/ns_hash.c src/scheme/ns_vm.c include/nandroid/web.h include/nandroid/web/http_server.h src/web/http_server.c include/nandroid/web/http_router.h src/web/http_router.c src/web.c)
target_link_libraries(Nandroid uv)
target_link_libraries(Nandroid http_parser)
target_link_libraries(Nandroid sqlite3)
target_link_libraries(Nandroid ${CMAKE_THREAD_LIBS_INIT})