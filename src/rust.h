#include <cstdint>
#include <unistd.h>

extern "C" {
  uint32_t add(uint32_t lhs, uint32_t rhs);
  void* hello_new(void (*)(void*));
  void hello_set(void*, const uint16_t *, size_t);
  size_t hello_size(void*);
  const char* hello_get(void*);
  void hello_free(void *);
}
