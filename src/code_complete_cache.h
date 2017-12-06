#pragma once

#include "language_server_api.h"

#include <optional.h>

#include <mutex>

using namespace std::experimental;
using std::experimental::nullopt;

// Cached completion information, so we can give fast completion results when
// the user erases a character. vscode will resend the completion request if
// that happens.
struct CodeCompleteCache {
  // NOTE: Make sure to access these variables under |WithLock|.
  optional<std::string> cached_path_;
  optional<lsPosition> cached_completion_position_;
  NonElidedVector<lsCompletionItem> cached_results_;

  std::mutex mutex_;

  void WithLock(std::function<void()> action);
  bool IsCacheValid(lsTextDocumentPositionParams position);
};