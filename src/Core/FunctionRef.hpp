#pragma once

#include <memory>
#include <type_traits>
#include <utility>

namespace Hermes::core {
template <class Signature> class FunctionRef;

template <class Ret, class... Args> class FunctionRef<Ret(Args...)> {
public:
  FunctionRef() = delete;

  template <class Callable>
    requires(!std::is_same_v<std::remove_cvref_t<Callable>, FunctionRef> &&
             std::is_invocable_r_v<Ret, Callable &, Args...>)
  FunctionRef(Callable &callable) noexcept
      : m_Ptr(static_cast<void *>(std::addressof(callable))),
        m_Invoke(&Invoke<Callable>) {}

  Ret operator()(Args... args) const {
    return m_Invoke(m_Ptr, std::forward<Args>(args)...);
  }

private:
  void *m_Ptr;
  Ret (*m_Invoke)(void *, Args...);

  template <class Callable> static Ret Invoke(void *ptr, Args... args) {
    return (*static_cast<Callable *>(ptr))(std::forward<Args>(args)...);
  }
};
} // namespace Hermes::core
