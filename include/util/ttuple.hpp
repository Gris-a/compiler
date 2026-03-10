#pragma once
#include <variant>

template<typename... Ts> struct TTuple {};

template<typename T>
concept ttuple = requires {
    []<typename... Us>(TTuple<Us...>){}(std::declval<T>());
};

template<ttuple LT, ttuple RT>
struct Concat;

template<typename... LTs, typename... RTs>
struct Concat<TTuple<LTs...>, TTuple<RTs...>> {
  using Result = TTuple<LTs..., RTs...>;
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
