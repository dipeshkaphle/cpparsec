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
