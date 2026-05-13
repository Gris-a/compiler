#pragma once

#include <variant>

template<typename... Ts> struct TTuple {};

template<typename T>
struct is_ttuple : std::false_type {};

template<typename... Ts>
struct is_ttuple<TTuple<Ts...>> : std::true_type {};

template<typename T>
concept ttuple = is_ttuple<T>::value;

template<ttuple... Tuples>
struct Concat;

template<>
struct Concat<> {
    using Result = TTuple<>;
};

template<ttuple T>
struct Concat<T> {
    using Result = T;
};

template<typename... Ts, typename... Us, ttuple... Rest>
struct Concat<TTuple<Ts...>, TTuple<Us...>, Rest...> {
    using Result = typename Concat<TTuple<Ts..., Us...>, Rest...>::Result;
};

namespace detail {
  template<ttuple Acc, ttuple Rest> struct PopImpl;

  template<ttuple Acc, typename Last>
  struct PopImpl<Acc, TTuple<Last>> {
      using Result = Acc;
  };
  
  template<typename... Acc, typename Head, typename... Tail>
  requires (sizeof...(Tail) > 0)
  struct PopImpl<TTuple<Acc...>, TTuple<Head, Tail...>> {
      using Result = typename PopImpl<TTuple<Acc..., Head>, TTuple<Tail...>>::Result;
  };
}

template<ttuple T>
struct Pop {
  using Result = detail::PopImpl<TTuple<>, T>::Result;
};

template<ttuple Tuple, typename T>
struct Contains;

template<typename Head, typename... Tail, typename T>
struct Contains<TTuple<Head, Tail...>, T> {
  static constexpr bool value = Contains<TTuple<Tail...>, T>::value;
};

template<typename Head, typename... Tail>
struct Contains<TTuple<Head, Tail...>, Head> {
  static constexpr bool value = true;
};

template<typename T>
struct Contains<TTuple<>, T> {
  static constexpr bool value = false;
};

template<ttuple T>
struct TTupleVariant;

template<typename... Ts>
struct TTupleVariant<TTuple<Ts...>> {
  using Result = std::variant<Ts...>;
};
