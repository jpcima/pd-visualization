#pragma once
#include <algorithm>
#include <memory>

template <unsigned ChannelMax = 4>
struct frame {
  float samples[ChannelMax] = {};
};

template <unsigned ChannelMax = 4>
class sample_memory {
 public:
  typedef frame<ChannelMax> frame_type;

  sample_memory() {}
  explicit sample_memory(unsigned size) { this->resize(size); }

  unsigned size() const { return bufsize_; }

  void resize(unsigned size) {
    buf_.reset(new frame_type[2 * size]());
    bufsize_ = size;
    writeindex_ = 0;
  }

  void append(frame_type frame) {
    buf_[writeindex_] = buf_[writeindex_ + bufsize_] = frame;
    writeindex_ = (writeindex_ + 1) % bufsize_;
  }

  void append(const float samples[], unsigned nsamples) {
    frame_type frame;
    std::copy_n(samples, std::min(nsamples, ChannelMax), frame.samples);
    append(frame);
  }

  const frame_type *data() const {
    unsigned readindex = writeindex_;
    return buf_.get() + readindex;
  }

 private:
  std::unique_ptr<frame_type[]> buf_;
  unsigned bufsize_ = 0;
  unsigned writeindex_ = 0;
};
