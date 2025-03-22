// Copyright 2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/internal/simple_pattern.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "src/util/debug_string.h"

namespace moriarty {
namespace moriarty_internal {

namespace {

bool IsNonNegativeChar(char ch) { return (ch >= 0 && ch <= 127); }

bool IsSpecialCharacter(char ch) {
  static constexpr std::string_view kSpecialCharacters = R"(\()[]{}^?*+-|)";
  return absl::StrContains(kSpecialCharacters, ch);
}

bool ValidCharSetRange(std::string_view range) {
  if (range.size() != 3) return false;
  if (range[1] != '-') return false;
  char a = range[0];
  char b = range[2];
  return (a <= b) && ((std::islower(a) && std::islower(b)) ||
                      (std::isupper(a) && std::isupper(b)) ||
                      (std::isdigit(a) && std::isdigit(b)));
}

}  // namespace

bool RepeatedCharSet::Add(char character) {
  if (!IsNonNegativeChar(character)) {
    throw std::invalid_argument(
        std::format("Invalid character in SimplePattern: {}",
                    librarian::DebugString(character)));
  }
  if (valid_chars_[character]) return false;
  valid_chars_[character] = true;
  return true;
}

void RepeatedCharSet::FlipValidCharacters() { valid_chars_.flip(); }

void RepeatedCharSet::SetRange(int64_t min, int64_t max) {
  if (min > max || max < 0) {
    throw std::invalid_argument(std::format(
        "Invalid range in SimplePattern: min = {}, max = {}", min, max));
  }
  min_ = std::max<int64_t>(0, min);
  max_ = max;
}

bool RepeatedCharSet::IsValidCharacter(char character) const {
  return IsNonNegativeChar(character) && valid_chars_[character];
}

std::optional<int64_t> RepeatedCharSet::LongestValidPrefix(
    std::string_view str) const {
  int64_t idx = 0;
  while (idx < str.size() && idx < max_ && IsValidCharacter(str[idx])) idx++;

  if (idx < min_) return std::nullopt;
  return idx;
}

int64_t RepeatedCharSet::MinLength() const { return min_; }

int64_t RepeatedCharSet::MaxLength() const { return max_; }

std::vector<char> RepeatedCharSet::ValidCharacters() const {
  std::vector<char> result;
  for (int c = 0; c < 128; c++)  // Use `int` over `char` to avoid overflow.
    if (valid_chars_[c]) result.push_back(c);
  return result;
}

int CharacterSetPrefixLength(std::string_view pattern) {
  if (pattern.empty())
    throw std::invalid_argument(
        "Cannot parse character set. Empty pattern given.");
  if (pattern[0] != '[') {
    if (IsSpecialCharacter(pattern[0])) {
      throw std::invalid_argument(std::format(
          "Unexpected special character found: `{}`. If you want to use this "
          "as a character, wrap it in square brackets. E.g., `[{{]` will "
          "accept a `{{` character.",
          pattern[0]));
    }
    return 1;  // Single character
  }

  // The end of the character set is either the first or the second ']' seen
  // (no character may be duplicated in a character set, so it cannot be the
  // 3rd or 4th, etc.). It is the second if '[' does not appear between the
  // first and the second.
  std::optional<int> close_index;
  for (int i = 1; i < pattern.size(); i++) {
    if (pattern[i] == ']') {
      if (close_index.has_value()) {
        close_index = i;
        break;
      }
      close_index = i;
    } else if (pattern[i] == '[') {
      if (close_index.has_value()) break;
    }
  }

  if (!close_index.has_value()) {
    throw std::invalid_argument(
        std::format("No ']' found to end character set. {}", pattern));
  }
  return *close_index + 1;
}

RepeatedCharSet ParseCharacterSetBody(std::string_view chars) {
  if (chars.empty())
    throw std::invalid_argument(
        "Empty character set. Use [ab] to match 'a' or 'b'.");

  RepeatedCharSet char_set;
  char_set.SetRange(1, 1);

  std::string_view original = chars;
  auto throw_duplicate_char = [original](char c) {
    throw std::invalid_argument(std::format("{} appears multiple times in [{}]",
                                            librarian::DebugString(c),
                                            original));
  };

  bool negation = false;
  if (chars[0] == '^') {
    chars.remove_prefix(1);
    if (chars.empty()) {
      if (!char_set.Add('^')) throw_duplicate_char('^');
      return char_set;
    }
    negation = true;
  }

  if (chars.back() == '-') {
    if (!char_set.Add('-')) throw_duplicate_char('-');
    chars.remove_suffix(1);
  }

  if (absl::StrContains(chars, '[') && absl::StrContains(chars, ']') &&
      chars.find('[') > chars.find(']')) {
    throw std::invalid_argument(
        "']' cannot come after '[' inside a character set");
  }

  for (int i = 0; i < chars.size(); i++) {
    if (ValidCharSetRange(chars.substr(i, 3))) {
      for (char c = chars[i]; c <= chars[i + 2]; c++) {
        if (!char_set.Add(c)) throw_duplicate_char(c);
      }
      i += 2;  // Handled chars[i + 1] and chars[i + 2].
      continue;
    }

    if (chars[i] == '-') {
      throw std::invalid_argument(
          "Invalid '-' in character set. Only works with "
          "[lowercase-lowercase], [uppercase-uppercase], [number-number]. If "
          "you want to include '-', it must be the last character in the set. "
          "(E.g., `[abe-]` will accept 'a' or 'b' or 'e' or '-')");
    }

    if (!char_set.Add(chars[i])) throw_duplicate_char(chars[i]);
  }

  if (negation) char_set.FlipValidCharacters();

  return char_set;
}

int RepetitionPrefixLength(std::string_view pattern) {
  if (pattern.empty()) return 0;
  if (pattern[0] == '?' || pattern[0] == '+' || pattern[0] == '*') return 1;
  if (pattern[0] != '{') return 0;

  int idx = pattern.find_first_of('}');
  if (idx == std::string_view::npos) {
    throw std::invalid_argument("No '}' found to end repetition block.");
  }

  return idx + 1;
}

RepetitionRange ParseRepetitionBody(std::string_view repetition) {
  if (repetition.empty()) return RepetitionRange({1, 1});
  if (repetition.size() == 1) {
    if (repetition[0] == '?') return RepetitionRange({0, 1});
    if (repetition[0] == '+')
      return RepetitionRange({1, std::numeric_limits<int64_t>::max()});
    if (repetition[0] == '*')
      return RepetitionRange({0, std::numeric_limits<int64_t>::max()});

    throw std::invalid_argument(
        std::format("Invalid repetition character: '{}'", repetition));
  }

  if (repetition.front() != '{' || repetition.back() != '}') {
    throw std::invalid_argument(std::format(
        "Expected {{ and }} around repetition block: '{}'", repetition));
  }
  repetition.remove_prefix(1);
  repetition.remove_suffix(1);

  std::string_view min_str = repetition;
  std::string_view max_str = repetition;
  if (absl::StrContains(repetition, ',')) {
    min_str = repetition.substr(0, repetition.find(','));
    max_str = repetition.substr(repetition.find(',') + 1);
  }

  RepetitionRange result = {0, std::numeric_limits<int64_t>::max()};
  if (!min_str.empty() && !absl::SimpleAtoi(min_str, &result.min_length)) {
    throw std::invalid_argument(
        std::format("Invalid min value in repetition: '{}'", repetition));
  }
  if (!max_str.empty() && !absl::SimpleAtoi(max_str, &result.max_length)) {
    throw std::invalid_argument(
        std::format("Invalid max value in repetition: '{}'", repetition));
  }

  return result;
}

PatternNode ParseRepeatedCharSetPrefix(std::string_view pattern) {
  int char_set_len = CharacterSetPrefixLength(pattern);
  std::string_view chars = pattern.substr(0, char_set_len);
  if (chars.size() >= 2 && chars.front() == '[' && chars.back() == ']') {
    chars = chars.substr(1, chars.size() - 2);
  }
  RepeatedCharSet char_set = ParseCharacterSetBody(chars);

  int repetition_len = RepetitionPrefixLength(pattern.substr(char_set_len));

  RepetitionRange repetition =
      ParseRepetitionBody(pattern.substr(char_set_len, repetition_len));

  char_set.SetRange(repetition.min_length, repetition.max_length);

  PatternNode result;
  result.repeated_character_set = std::move(char_set);
  result.pattern = pattern.substr(0, char_set_len + repetition_len);

  return result;
}

namespace {

PatternNode ParseAllOfNodeScopePrefix(std::string_view pattern) {
  // The `allof_node` holds the concatenated elements. E.g., "a*(b|c)d" will
  // store 3 elements in `allof_node` ("a*", "(b|c)", "d").
  PatternNode allof_node;
  allof_node.subpattern_type = PatternNode::SubpatternType::kAllOf;

  std::size_t idx = 0;
  while (idx < pattern.size() && pattern[idx] != '|' && pattern[idx] != ')') {
    if (pattern[idx] != '(') {
      PatternNode char_set = ParseRepeatedCharSetPrefix(pattern.substr(idx));
      allof_node.subpatterns.push_back(char_set);
      idx += char_set.pattern.size();
      continue;
    }

    PatternNode inner_scope =
        ParseScopePrefix(pattern.substr(idx + 1));  // +1 for '('
    std::size_t inner_size = inner_scope.pattern.size();
    if (idx + 1 + inner_size >= pattern.size() ||
        pattern[idx + 1 + inner_size] != ')') {
      throw std::invalid_argument(
          std::format("Invalid end of scope. Expected ')'. '{}'", pattern));
    }

    inner_scope.pattern = pattern.substr(idx, inner_size + 2);
    allof_node.subpatterns.push_back(std::move(inner_scope));
    idx += inner_size + 2;  // +2 for '(' and ')'
  }

  allof_node.pattern = pattern.substr(0, idx);
  return allof_node;
}

// Converts "\\" -> "\" and "\ " -> " ". And removes empty spaces (does not
// remove other whitespace characters).
std::string Sanitize(std::string_view pattern) {
  std::string sanitized_pattern;
  for (int i = 0; i < pattern.size(); i++) {
    if (pattern[i] == '\\') {
      if (i + 1 == pattern.size()) {
        throw std::invalid_argument(
            "Cannot have unescaped '\\' at the end of pattern.");
      }
      if (pattern[i + 1] != '\\' && pattern[i + 1] != ' ') {
        throw std::invalid_argument(
            std::format("Invalid escape character: \\ followed by {}",
                        librarian::DebugString(pattern[i + 1])));
      }
      absl::StrAppend(&sanitized_pattern, pattern.substr(i + 1, 1));
      i++;
      continue;
    }
    if (pattern[i] != ' ')
      absl::StrAppend(&sanitized_pattern, pattern.substr(i, 1));
  }
  return sanitized_pattern;
}

}  // namespace

PatternNode ParseScopePrefix(std::string_view pattern) {
  if (pattern.empty() || pattern[0] == ')') {
    throw std::invalid_argument("Attempting to parse empty scope.");
  }

  // The `anyof_node` holds the top-level options. E.g., "ab|c(d|e)|f" will
  // store 3 elements in `anyof_node` ("ab", "c(d|e)", "f").
  PatternNode anyof_node;
  anyof_node.subpattern_type = PatternNode::SubpatternType::kAnyOf;

  std::size_t idx = 0;
  while (idx < pattern.size() && pattern[idx] != ')') {
    if (pattern[idx] == '|') {
      if (idx == 0 || idx + 1 >= pattern.size() || pattern[idx + 1] == '|') {
        throw std::invalid_argument(
            "Empty or-block not allowed in pattern. (E.g., `a|c||b`)");
      }
      idx++;
    }
    PatternNode allof_node = ParseAllOfNodeScopePrefix(pattern.substr(idx));
    idx += allof_node.pattern.size();
    anyof_node.subpatterns.push_back(std::move(allof_node));
  }

  // If there is only one subpattern with `kAnyOf`, flatten it.
  if (anyof_node.subpatterns.size() == 1) return anyof_node.subpatterns[0];
  anyof_node.pattern = pattern.substr(0, idx);
  return anyof_node;
}

std::optional<int64_t> MatchesPrefixLength(const PatternNode& pattern_node,
                                           std::string_view str) {
  std::optional<int64_t> prefix_length =
      pattern_node.repeated_character_set.LongestValidPrefix(str);
  if (!prefix_length.has_value()) return std::nullopt;
  str.remove_prefix(*prefix_length);

  for (const PatternNode& subpattern : pattern_node.subpatterns) {
    std::optional<int64_t> subpattern_length =
        MatchesPrefixLength(subpattern, str);
    if (!subpattern_length) {
      if (pattern_node.subpattern_type == PatternNode::SubpatternType::kAllOf)
        return std::nullopt;
      continue;  // We are in a kAnyOf, so we don't *have* to match this.
    }

    *prefix_length += *subpattern_length;

    if (pattern_node.subpattern_type == PatternNode::SubpatternType::kAnyOf) {
      return prefix_length;
    }

    str.remove_prefix(*subpattern_length);
  }

  if (pattern_node.subpattern_type == PatternNode::SubpatternType::kAnyOf) {
    // We are in a kAnyOf, but we didn't match anything...
    return std::nullopt;
  }
  return prefix_length;
}

SimplePattern::SimplePattern(std::string pattern) {
  std::string sanitized_pattern = Sanitize(pattern);
  if (sanitized_pattern.empty()) {
    throw std::invalid_argument("SimplePattern may not be empty");
  }

  // Put the string into the SimplePattern immediately so that all
  // `string_view`s point to the correct memory.
  pattern_ = std::move(sanitized_pattern);
  try {
    pattern_node_ = ParseScopePrefix(pattern_);
  } catch (const std::invalid_argument& e) {
    throw std::invalid_argument(std::format(
        "Invalid SimplePattern: {}.\nError: {}", pattern_, e.what()));
  }

  if (pattern_node_.pattern != pattern_) {
    throw std::invalid_argument(
        absl::StrCat("Invalid pattern. Extra characters found: ",
                     pattern_.substr(pattern_node_.pattern.size())));
  }
}

std::string SimplePattern::Pattern() const { return pattern_; }

bool SimplePattern::Matches(std::string_view str) const {
  std::optional<int64_t> prefix_length =
      MatchesPrefixLength(pattern_node_, str);
  return prefix_length && *prefix_length == str.length();
}

namespace {

std::string GenerateRepeatedCharSet(
    const RepeatedCharSet& char_set,
    std::optional<std::string_view> restricted_alphabet,
    const SimplePattern::RandFn& rand) {
  if (char_set.MaxLength() == std::numeric_limits<int64_t>::max()) {
    throw std::runtime_error(
        "Cannot generate with `*` or `+` or massive lengths.");
  }
  int64_t len = rand(char_set.MinLength(), char_set.MaxLength());

  RepeatedCharSet restricted_char_set;
  if (restricted_alphabet.has_value()) {
    for (char c : *restricted_alphabet)
      (void)restricted_char_set.Add(c);  // Ignore duplicates here.
  } else {
    restricted_char_set.FlipValidCharacters();  // Allow all characters.
  }
  std::vector<char> valid_chars;
  for (char c : char_set.ValidCharacters()) {
    if (restricted_char_set.IsValidCharacter(c)) valid_chars.push_back(c);
  }

  if (valid_chars.empty()) {
    // No valid characters, so the only valid string is the empty string.
    if (char_set.MinLength() <= 0) return "";
    throw std::invalid_argument(
        "No valid characters for generation, but empty string is not "
        "allowed.");
  }

  std::string result;
  while (result.size() < len) {
    int64_t idx = rand(0, (int)valid_chars.size() - 1);
    result.push_back(valid_chars[idx]);
  }
  return std::string(result.begin(), result.end());
}

std::string GeneratePatternNode(
    const PatternNode& node,
    std::optional<std::string_view> restricted_alphabet,
    const SimplePattern::RandFn& rand) {
  std::string result = GenerateRepeatedCharSet(node.repeated_character_set,
                                               restricted_alphabet, rand);

  if (node.subpatterns.empty()) return result;

  if (node.subpattern_type == PatternNode::SubpatternType::kAnyOf) {
    int64_t idx = rand(0, (int)node.subpatterns.size() - 1);
    std::string subresult =
        GeneratePatternNode(node.subpatterns[idx], restricted_alphabet, rand);
    return absl::StrCat(result, subresult);
  }

  for (const PatternNode& subpattern : node.subpatterns) {
    std::string subresult =
        GeneratePatternNode(subpattern, restricted_alphabet, rand);
    absl::StrAppend(&result, subresult);
  }
  return result;
}

}  // namespace

std::string SimplePattern::Generate(const RandFn& rand) const {
  return GenerateWithRestrictions(/* restricted_alphabet = */ std::nullopt,
                                  rand);
}

std::string SimplePattern::GenerateWithRestrictions(
    std::optional<std::string_view> restricted_alphabet,
    const RandFn& rand) const {
  return GeneratePatternNode(pattern_node_, restricted_alphabet, rand);
}

}  // namespace moriarty_internal
}  // namespace moriarty
