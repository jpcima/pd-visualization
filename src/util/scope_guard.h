#pragma once
#include <utility>

#define scope(kw) SCOPE_GUARD_##kw()

template <class F> class scope_guard {
  F f_;
 public:
  inline scope_guard(scope_guard<F> &&o): f_(std::move(o.f_)) {}
  inline scope_guard(F f): f_(std::move(f)) {}
  inline ~scope_guard() { f_(); }
};

#define SCOPE_GUARD_PP_CAT_I(x, y) x##y
#define SCOPE_GUARD_PP_CAT(x, y) SCOPE_GUARD_PP_CAT_I(x, y)
#define SCOPE_GUARD_PP_UNIQUE(x) SCOPE_GUARD_PP_CAT(x, __LINE__)
#define SCOPE_GUARD_exit()                                     \
  auto SCOPE_GUARD_PP_UNIQUE(_scope_exit_local) =              \
    ::scope_guard_detail::scope_guard_helper() << [&]

namespace scope_guard_detail {

struct scope_guard_helper {
  template <class F> inline scope_guard<F> operator<<(F &&f) {
    return scope_guard<F>(std::forward<F>(f));
  }
};

} // namespace scope_guard_detail
