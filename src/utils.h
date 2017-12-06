#pragma once

#include <optional.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

using std::experimental::nullopt;
using std::experimental::optional;

// Trim from start (in place)
void TrimStart(std::string& s);
// Trim from end (in place)
void TrimEnd(std::string& s);
// Trim from both ends (in place)
void Trim(std::string& s);

// Returns true if |value| starts/ends with |start| or |ending|.
bool StartsWith(const std::string& value, const std::string& start);
bool EndsWith(const std::string& value, const std::string& ending);
bool AnyStartsWith(const std::vector<std::string>& values,
                   const std::string& start);
bool StartsWithAny(const std::string& value,
                   const std::vector<std::string>& startings);
bool EndsWithAny(const std::string& value,
                 const std::vector<std::string>& endings);

std::string ReplaceAll(const std::string& source,
                       const std::string& from,
                       const std::string& to);

std::vector<std::string> SplitString(const std::string& str,
                                     const std::string& delimiter);

std::string LowerPathIfCaseInsensitive(const std::string& path);

template <typename TValues, typename TMap>
std::string StringJoinMap(const TValues& values, const TMap& map) {
  std::string result;
  bool first = true;
  for (auto& entry : values) {
    if (!first)
      result += ", ";
    first = false;
    result += map(entry);
  }
  return result;
}

template <typename TValues>
std::string StringJoin(const TValues& values) {
  return StringJoinMap(values, [](const std::string& entry) { return entry; });
}

// Finds all files in the given folder. This is recursive.
std::vector<std::string> GetFilesInFolder(std::string folder,
                                          bool recursive,
                                          bool add_folder_to_path);
void GetFilesInFolder(std::string folder,
                      bool recursive,
                      bool add_folder_to_path,
                      const std::function<void(const std::string&)>& handler);

// Ensures that |path| ends in a slash.
void EnsureEndsInSlash(std::string& path);

// Converts a file path to one that can be used as filename.
// e.g. foo/bar.c => foo_bar.c
std::string EscapeFileName(std::string path);

optional<std::string> ReadContent(const std::string& filename);
std::vector<std::string> ReadLinesWithEnding(std::string filename);
std::vector<std::string> ToLines(const std::string& content,
                                 bool trim_whitespace);

std::unordered_map<std::string, std::string> ParseTestExpectation(
    std::string filename);
void UpdateTestExpectation(const std::string& filename,
                           const std::string& expectation,
                           const std::string& actual);

void Fail(const std::string& message);

void WriteToFile(const std::string& filename, const std::string& content);

// note: this implementation does not disable this overload for array types
// See
// http://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique#Possible_Implementatiog
template <typename T, typename... Args>
std::unique_ptr<T> MakeUnique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
void AddRange(std::vector<T>* dest, const std::vector<T>& to_add) {
  for (const T& e : to_add)
    dest->push_back(e);
}

template <typename T>
void PushRange(std::queue<T>* dest, const std::vector<T>& to_add) {
  for (const T& e : to_add)
    dest->push(e);
}

template <typename T>
void RemoveRange(std::vector<T>* dest, const std::vector<T>& to_remove) {
  dest->erase(std::remove_if(dest->begin(), dest->end(),
                             [&](const T& t) {
                               // TODO: make to_remove a set?
                               return std::find(to_remove.begin(),
                                                to_remove.end(),
                                                t) != to_remove.end();
                             }),
              dest->end());
}

// http://stackoverflow.com/a/38140932
//
//  struct SomeHashKey {
//    std::string key1;
//    std::string key2;
//    bool key3;
//  };
//  MAKE_HASHABLE(SomeHashKey, t.key1, t.key2, t.key3)

inline void hash_combine(std::size_t& seed) {}

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  hash_combine(seed, rest...);
}

#define MAKE_HASHABLE(type, ...)                  \
  namespace std {                                 \
  template <>                                     \
  struct hash<type> {                             \
    std::size_t operator()(const type& t) const { \
      std::size_t ret = 0;                        \
      hash_combine(ret, __VA_ARGS__);             \
      return ret;                                 \
    }                                             \
  };                                              \
  }

#define MAKE_ENUM_HASHABLE(type)                  \
  namespace std {                                 \
  template <>                                     \
  struct hash<type> {                             \
    std::size_t operator()(const type& t) const { \
      return hash<int>()(static_cast<int>(t));    \
    }                                             \
  };                                              \
  }

float GetProcessMemoryUsedInMb();

std::string FormatMicroseconds(long long microseconds);
