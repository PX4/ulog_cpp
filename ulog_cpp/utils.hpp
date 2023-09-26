/****************************************************************************
 * Copyright (c) 2023 PX4 Development Team.
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/
#pragma once

namespace ulog_cpp {
/**
 * Just does a static cast, except for signed char, it interprets it as unsigned char
 * @tparam ReturnType The type to cast to
 * @tparam OriginType The type to cast from
 * @param v The value to cast
 * @return The casted value
 */
template <typename ReturnType, typename OriginType>
static inline ReturnType staticCastEnsureUnsignedChar(const OriginType& v)
{
  if constexpr (std::is_same_v<std::decay_t<OriginType>, char>) {
    return static_cast<ReturnType>(static_cast<unsigned char>(v));
  }
  return static_cast<ReturnType>(v);
}

/**
 * SFINAE helper to check if a type is a vector
 * @tparam T The type to check
 */
template <typename T>
struct is_vector : std::false_type {  // NOLINT(*-identifier-naming)
};
template <typename T>
struct is_vector<std::vector<T>> : std::true_type {
};

/**
 * SFINAE helper to check if a type is a string
 * @tparam T The type to check
 */
template <typename T>
struct is_string : std::is_same<std::decay_t<T>, std::string> {  // NOLINT(*-identifier-naming)
};

}  // namespace ulog_cpp
