#ifndef NANDROID_NS_OBJECT_H
#define NANDROID_NS_OBJECT_H

#include "ns_common.h"

HEADER_BEGIN

#define FOR_EACH_VTYPE(V) \
  V(Fixnum) \
  V(Flonum) \
  V(Boolean) \
  V(Symbol) \
  V(Character) \
  V(String) \
  V(Pair) \
  V(Proc) \
  V(Macro) \
  V(Nil)

typedef enum{
  kIllegalType = 0,
#define DEFINE_VTYPE(name) k##name##Type,
  FOR_EACH_VTYPE(DEFINE_VTYPE)
#undef DEFINE_VTYPE
} ns_vtype;

typedef struct _ns_value{
  ns_vtype type;
  union{
    void* ns_ptr;
    uint16_t ns_character;
    struct{
      uint16_t* data;
      size_t size;
      size_t asize;
    } ns_string;
    struct{
      struct _ns_value* car;
      struct _ns_value* cdr;
    } ns_pair;
    uint64_t ns_fixnum;
    uint64_t ns_flonum;
    uint32_t ns_symbol;
    ns_bool boolean;
  } as;
} ns_value;

#define ns_set_value(val, attr, t, v)({ \
  (val)->as.attr = v; \
  (val)->type = t; \
})

#define ns_set_true(val) ns_set_value(val, boolean, kBooleanType, ns_true)
#define ns_set_false(val) ns_set_value(val, boolean, kBooleanType, ns_false)
#define ns_set_nil(val) ns_set_value(val, ns_ptr, kNilType, NULL)
#define ns_set_fixnum(val, value) ns_set_value(val, ns_fixnum, kFixnumType, value)
#define ns_set_flonum(val, value) ns_set_value(val, ns_flonum, kFlonumType, value)

HEADER_END

#endif