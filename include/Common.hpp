#pragma once

#include <memory>
#include <string>
#include <vector>

template <typename Type, typename Deleter = std::default_delete<Type>>
using Scope = std::unique_ptr<Type, Deleter>;

template <typename Type>
using Ref = std::shared_ptr<Type>;

template <typename Type>
using Weak = std::weak_ptr<Type>;

template <class Type, class... Types>
constexpr auto is(std::variant<Types...>& variant)
    -> decltype(std::holds_alternative<Type>(variant)) {
  return std::holds_alternative<Type>(variant);
}

template <class Type, class... Types>
constexpr auto is(const std::variant<Types...>& variant)
    -> decltype(std::holds_alternative<Type>(variant)) {
  return std::holds_alternative<Type>(variant);
}

template <class Type, class... Types>
constexpr auto as(std::variant<Types...>& variant)
    -> decltype(std::get<Type>(variant)) {
  return std::get<Type>(variant);
}

template <class Type, class... Types>
constexpr auto as(const std::variant<Types...>& variant)
    -> decltype(std::get<Type>(variant)) {
  return std::get<Type>(variant);
}
