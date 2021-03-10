#pragma once

#include <cctype>
#include <tuple>

template <typename T> struct is_tuple : std::false_type {};
template <typename... T> struct is_tuple<std::tuple<T...>> : std::true_type {};

template <typename> struct num_of_template_params {
  static constexpr std::size_t size = 1;
};
template <typename... T> struct num_of_template_params<std::tuple<T...>> {
  static constexpr std::size_t size = sizeof...(T);
};

template <std::size_t N, typename A, typename... T> struct nth_type {
  static_assert(
      N <= sizeof...(T),
      "Invalid value of N for nth_type<>. Not enough template parameters");
  using Type = typename nth_type<N - 1, T...>::Type;
};
template <typename A, typename... T> struct nth_type<0, A, T...> {
  using Type = A;
};
