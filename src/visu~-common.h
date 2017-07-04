#pragma once
#include <stdint.h>

static constexpr unsigned msgmax = 512;
static constexpr unsigned sockbuf = 256 * 1024;

enum MessageTag {
  MessageTag_SampleRate,
  MessageTag_Samples,
  MessageTag_Toggle,
};

struct MessageHeader {
  MessageTag tag;
  unsigned len;
  union {
    char c[0];
    int32_t i[0];
    uint32_t u[0];
    float f[0];
  };
};
