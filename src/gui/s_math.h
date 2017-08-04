#pragma once
#include <cmath>

template <class R>
R clamp(R x, R xmin, R xmax) {
  return (x < xmin) ? xmin : (x > xmax) ? xmax : x;
}

template <class R>
R interp_linear(const R y[], R mu) {
  return y[0] * (1 - mu) + y[1] * mu;
}

template <class R>
R interp_catmull(const R y[], R mu) {
  R mu2 = mu * mu;
  R mu3 = mu2 * mu;
  R a[] = {- R(0.5) * y[0] + R(1.5) * y[1] - R(1.5) * y[2] + R(0.5) * y[3],
           y[0] - R(2.5) * y[1] + R(2) * y[2] - R(0.5) * y[3],
           - R(0.5) * y[0] + R(0.5) * y[2],
           y[1]};
  return a[0] * mu3 + a[1] * mu2 + a[2] * mu + a[3];
}

template <class R>
R window_nutall(R r) {
  R a[] = {0.355768, -0.487396, 0.144232, -0.012604};
  R p = r * 2 * R(M_PI);
  R w = a[0];
  for (unsigned i = 1; i < 4; ++i)
    w += a[i] * std::cos(i * p);
  return w;
}
