#pragma once

#include <memory>
#include <string>
#include <vector>

template <typename Type, typename Deleter = std::default_delete<Type>>
using Scope = std::unique_ptr<Type, Deleter>;

template <typename Type>
using Ref = std::shared_ptr<Type>;
