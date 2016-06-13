#ifndef NANDROID_NS_VM_H
#define NANDROID_NS_VM_H

#include "ns_gc.h"
#include "ns_object.h"

typedef struct _ns_scope ns_scope;
typedef uint32_t ns_symbol;

typedef struct{
  // Garbage Collector
  ns_gc* gc;

  // Scope
  ns_scope* scope;

  ns_symbol sym_define;
} ns_vm;

void ns_vm_init(ns_vm* vm);

void ns_scope_push(ns_vm* vm);
void ns_scope_pop(ns_vm* vm);
void ns_scope_define(ns_vm* vm, ns_symbol symbol, ns_value* value);
void ns_scope_lookup(ns_vm* vm, ns_symbol symbol, ns_value** value);

#endif