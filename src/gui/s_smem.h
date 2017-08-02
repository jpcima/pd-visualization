#pragma once
#include <memory>

class sample_memory {
 public:
  sample_memory() {}
  explicit sample_memory(unsigned size) { this->resize(size); }

  unsigned size() const { return bufsize_; }

  void resize(unsigned size) {
    buf_.reset(new float[2 * size]());
    bufsize_ = size;
  }

  void append(float sample) {
    buf_[writeindex_] = buf_[writeindex_ + bufsize_] = sample;
    writeindex_ = (writeindex_ + 1) % bufsize_;
  }

  const float *data() const {
    unsigned readindex = (writeindex_ - bufsize_) % bufsize_;
    return buf_.get() + readindex;
  }

 private:
  std::unique_ptr<float[]> buf_;
  unsigned bufsize_ = 0;
  unsigned writeindex_ = 0;
};
