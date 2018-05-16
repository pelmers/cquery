#include "cache_manager.h"
#include "clang_complete.h"
#include "message_handler.h"
#include "project.h"
#include "queue_manager.h"
#include "timer.h"
#include "working_files.h"
#include "include_complete.h"

#include <loguru.hpp>

namespace {
MethodType kMethodType = "$cquery/addCompilationDb";

struct lsCqueryAddCompilationDbParams {
  std::string databaseDirectory;
};
MAKE_REFLECT_STRUCT(lsCqueryAddCompilationDbParams, databaseDirectory);

struct In_CqueryAddCompilationDb : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsCqueryAddCompilationDbParams params;
};
MAKE_REFLECT_STRUCT(In_CqueryAddCompilationDb, params);
REGISTER_IN_MESSAGE(In_CqueryAddCompilationDb);

struct Handler_CqueryAddCompilationDb
    : BaseMessageHandler<In_CqueryAddCompilationDb> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_CqueryAddCompilationDb* request) override {
    Timer time;
    project->Load(g_config->projectRoot, request->params.databaseDirectory);
    time.ResetAndPrint("[perf] Loaded compilation entries (" +
                       std::to_string(project->entries.size()) + " files)");

    time.Reset();
    project->Index(QueueManager::instance(), working_files, lsRequestId());
    time.ResetAndPrint(
        "[perf] Dispatched cquery/addCompilationDb index requests");

    clang_complete->FlushAllSessions();
    LOG_S(INFO) << "Flushed all clang complete sessions";
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CqueryAddCompilationDb);
}  // namespace
