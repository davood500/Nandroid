#include <nandroid/scheme/ns_common.h>
#include <string.h>

#define get16bits(d) ((((uint32_t)(((uint8_t*)(d))[1])) << 8) + (uint32_t)(((uint8_t*)(d)[0])))

uint32_t
ns_hash(char* data){
  size_t len = strlen(data);
  uint32_t hash = (uint32_t) len;
  uint32_t tmp;

  if(len <= 0){
    return 0;
  }

  int rem = (int) (len & 3);
  len >>= 2;

  for(; len > 0; len--){
    hash += get16bits(data);
    tmp = (get16bits(data + 2) << 11) ^ hash;
    hash = (hash << 16) ^ tmp;
    data += 2 * sizeof(uint16_t);
    hash += hash >> 11;
  }

  switch(rem){
    case 3:{
      hash += get16bits(data);
      hash ^= hash << 16;
      hash ^= (data[sizeof(uint16_t)]) << 18;
      hash += hash >> 11;
      break;
    }
    case 2:{
      hash += get16bits(data);
      hash ^= hash << 11;
      hash += hash >> 17;
      break;
    }
    case 1:{
      hash += *data;
      hash ^= hash << 10;
      hash += hash >> 1;
      break;
    }
    default: break;
  }

  hash ^= hash << 3;
  hash += hash >> 5;
  hash ^= hash << 4;
  hash += hash >> 17;
  hash ^= hash << 25;
  hash += hash >> 6;
  return hash;
}