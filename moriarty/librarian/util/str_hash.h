
#ifndef MORIARTY_UTIL_STR_HASH_H_
#define MORIARTY_UTIL_STR_HASH_H_

#include <cstddef>
#include <string>
#include <string_view>

namespace moriarty {

// This allows std::string_view to be used as a key in the map on lookups.
// Usage: std::unordered_map<std::string, int, Hash, std::equal_to<>> map;
struct StrHash {
  using is_transparent = void;
  [[nodiscard]] size_t operator()(const std::string& txt) const {
    return std::hash<std::string>{}(txt);
  }
  [[nodiscard]] size_t operator()(std::string_view txt) const {
    return std::hash<std::string_view>{}(txt);
  }
  [[nodiscard]] size_t operator()(const char* txt) const {
    return std::hash<std::string_view>{}(txt);
  }
};

}  // namespace moriarty

#endif  // MORIARTY_UTIL_STR_HASH_H_
