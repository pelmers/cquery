#include "project.h"
#include "query.h"
#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"
#include "include_complete.h"

#include <loguru.hpp>

namespace {
struct Ipc_CqueryStats : public RequestMessage<Ipc_CqueryStats> {
  const static IpcId kIpcId = IpcId::CqueryStats;
  lsRequestId id;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryStats, id);
REGISTER_IPC_MESSAGE(Ipc_CqueryStats);

struct CqueryStatsBody {
  std::string projectRoot;
  int files;
  int workingFiles;
  int queryFiles;
  int fileConsumerUsed;
  int symbols;
  int includes;
};
MAKE_REFLECT_STRUCT(CqueryStatsBody, projectRoot, files, workingFiles, queryFiles, fileConsumerUsed, symbols, includes);

struct Out_CqueryStats : public lsOutMessage<Out_CqueryStats> {
  lsRequestId id;
  CqueryStatsBody result;
};
MAKE_REFLECT_STRUCT(Out_CqueryStats, jsonrpc, id, result);

struct CqueryStatsHandler : BaseMessageHandler<Ipc_CqueryStats>{
  IpcId GetId() const override { return IpcId::CqueryStats; }

  void Run(Ipc_CqueryStats* request) override {
    LOG_S(INFO) << "Getting " << project->entries.size() << " stats";
    Out_CqueryStats out;
    out.id = request->id;
    out.result.projectRoot = config->projectRoot;
    out.result.files = project->entries.size();
    out.result.symbols = db->symbols.size();
    out.result.workingFiles = working_files->files.size();
    out.result.fileConsumerUsed = file_consumer_shared->used_files.size();
    out.result.includes = include_complete->completion_items.size();
    out.result.queryFiles = db->files.size();
    QueueManager::WriteStdout(IpcId::CqueryStats, out);
  }
};

REGISTER_MESSAGE_HANDLER(CqueryStatsHandler);
}  // namespace
