#include <nandroid/scheme/ns_vm.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct _ns_scope_entry {
  struct _ns_scope_entry* next;
  struct _ns_scope_entry* prev;
  ns_value* value;
  ns_symbol key;
} ns_scope_entry;

struct _ns_scope{
  ns_scope* parent;
  ns_scope_entry* entries;
};

ns_scope* ns_scope_new(ns_scope* parent){
  ns_scope* scope = malloc(sizeof(ns_scope));
  scope->parent = parent;
  scope->entries = NULL;
  return scope;
}

void
ns_vm_init(ns_vm* vm){
  if((vm->gc = malloc(sizeof(ns_gc))) == NULL){
    fprintf(stderr, "Can't allocate gc...quitting\n");
    abort();
  }
  ns_gc_init(vm->gc);

  if((vm->scope = ns_scope_new(NULL)) == NULL){
    fprintf(stderr, "Can't allocate scope...quitting\n");
  }
}

void
ns_scope_push(ns_vm* vm){
  vm->scope = ns_scope_new(vm->scope);
}

void
ns_scope_pop(ns_vm* vm){
  if(vm->scope->parent == NULL){
    fprintf(stderr, "Scope underflow\n");
    abort();
  }

  ns_scope* scope = vm->scope;
  vm->scope = scope->parent;

  ns_scope_entry* entry = scope->entries;
  while(entry != NULL){
    ns_scope_entry* next = entry->next;
    ns_gc_del_ref(vm->gc, entry->value);
    free(entry);
    entry = next;
  }
  free(scope);
}

void
ns_scope_define(ns_vm* vm, ns_symbol sym, ns_value* value){
  ns_scope_entry* entry = vm->scope->entries;
  while(entry != NULL){
    if(entry->key == sym){
      ns_gc_del_ref(vm->gc, entry->value);
      entry->value = value;
      return;
    }

    entry = entry->next;
  }

  entry = malloc(sizeof(ns_scope_entry));
  entry->key = sym;
  entry->value = value;
  entry->next = vm->scope->entries;
  vm->scope->entries = entry;
  ns_gc_add_ref(vm->gc, value);
}

int
ns_scope_lookup(ns_vm* vm, ns_symbol sym, ns_value** value){
  ns_scope* scope = vm->scope;
  while(scope != NULL){
    ns_scope_entry* entry = scope->entries;
    while(entry != NULL){
      if(entry->key == sym){
        *value = entry->value;
        return 1;
      }

      entry = entry->next;
    }

    scope = scope->parent;
  }

  return 0;
}