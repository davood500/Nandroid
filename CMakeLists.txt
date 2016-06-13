cmake_minimum_required(VERSION 3.3)
project(Nandroid)

include_directories(include)

add_executable(Nandroid main.c include/nandroid/common.h include/nandroid/scheme/ns_common.h include/nandroid/scheme/ns_gc.h src/scheme/ns_gc.c include/nandroid/scheme/ns_object.h include/nandroid/scheme/ns_vm.h src/scheme/ns_hash.c)