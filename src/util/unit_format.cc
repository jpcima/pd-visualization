#include "unit_format.h"
#include <cstdio>

std::string unit_format(const char *fmt, double val) {
  struct unit_mapping {
    const char prefix;
    double factor;
  };

  const unit_mapping mappings[] = {
    {'u', 1e-6},
    {'m', 1e-3},
    {0, 1},
    {'k', 1e3},
    {'M', 1e6},
  };
  const unsigned nmappings = sizeof(mappings) / sizeof(mappings[0]);

  unsigned index = 0;
  while (index + 1 < nmappings && val >= mappings[index + 1].factor)
    ++index;

  char prefix = mappings[index].prefix;
  float factor = mappings[index].factor;

  char textbuf[64];
  unsigned len = std::sprintf(textbuf, fmt, val / factor);
  textbuf[len++] = ' ';
  textbuf[len++] = prefix;
  textbuf[len++] = 0;

  return std::string(textbuf, len);
}
