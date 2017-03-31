#include "node_revert.h"
#include <stdio.h>
#include <string.h>

namespace node {

unsigned int reverted = 0;

const char* RevertMessage(const unsigned int cve) {
#define V(code, label, msg) case REVERT_ ## code: return label "\x3a\x20" msg;
  switch (cve) {
    REVERSIONS(V)
    default:
      return "\x55\x6e\x6b\x6e\x6f\x77\x6e";
  }
#undef V
}

void Revert(const unsigned int cve) {
  reverted |= 1 << cve;
  printf("SECURITY WARNING: Reverting %s\n", RevertMessage(cve));
}

void Revert(const char* cve) {
#define V(code, label, _)                                                     \
  do {                                                                        \
    if (strcmp(cve, label) == 0) {                                            \
      Revert(static_cast<unsigned int>(REVERT_ ## code));                     \
      return;                                                                 \
    }                                                                         \
  } while (0);
  REVERSIONS(V)
#undef V
  printf("Error: Attempt to revert an unknown CVE [%s]\n", cve);
  exit(12);
}

bool IsReverted(const unsigned int cve) {
  return reverted & (1 << cve);
}

bool IsReverted(const char * cve) {
#define V(code, label, _)                                                     \
  do {                                                                        \
    if (strcmp(cve, label) == 0)                                              \
      return IsReverted(static_cast<unsigned int>(REVERT_ ## code));          \
  } while (0);
  REVERSIONS(V)
  return false;
#undef V
}

}  // namespace node
