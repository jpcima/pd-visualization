#pragma once
#include <type_traits>
#include <stddef.h>
#include <stdint.h>

static constexpr unsigned msgmax = 512;
static constexpr unsigned channelmax = 4;
static constexpr unsigned sockbuf = 256 * 1024;

enum MessageTag {
  MessageTag_SampleRate,
  MessageTag_Frames,
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

enum VisuType {
  Visu_Waterfall,
  Visu_Spectrogram,
  Visu_Oscillogram,
  Visu_Transfer,
  Visu_Count,
  Visu_Default = Visu_Waterfall,
};

enum class VisuDftResolution {
  Medium,
  High,
};

#define PD_LAYOUT_CHECK(t)                                           \
  static_assert(std::is_standard_layout<t>() && !offsetof(t, x_obj), \
                "object layout check failed");

