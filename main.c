#include <nandroid/scheme/ns_gc.h>
#include <stdlib.h>
#include <stdio.h>
#include <nandroid/scheme/ns_vm.h>
#include <nandroid/scheme/ns_object.h>

int
main(int argc, char** argv){
  ns_vm vm;
  ns_vm_init(&vm);

  ns_value* ten = ns_gc_alloc(vm.gc, sizeof(ns_value));
  ten->type = kFixnumType;
  ten->as.ns_fixnum = 10;
  ns_scope_define(&vm, ns_hash("ten"), ten);

  ns_value* lookup = NULL;
  if(!ns_scope_lookup(&vm, ns_hash("ten"), &lookup)){
    fprintf(stderr, "Couldn't lookup 'ten'\n");
    abort();
  }

  switch(lookup->type){
    case kFixnumType:{
      printf("%lu\n", ten->as.ns_fixnum);
      break;
    }
    default:{
      printf("unknown\n");
      break;
    }
  }

  return 0;
}