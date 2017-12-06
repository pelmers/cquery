#include "ipc_manager.h"

#include "language_server_api.h"
#include "query.h"

#include <sstream>

IpcManager* IpcManager::instance_ = nullptr;

// static
IpcManager* IpcManager::instance() {
  return instance_;
}

// static
void IpcManager::CreateInstance(MultiQueueWaiter* waiter) {
  instance_ = new IpcManager(waiter);
}

// static
void IpcManager::WriteStdout(IpcId id, lsBaseOutMessage& response) {
  std::ostringstream sstream;
  response.Write(sstream);

  StdoutMessage out;
  out.content = sstream.str();
  out.id = id;
  instance()->for_stdout.Enqueue(std::move(out));
}

IpcManager::IpcManager(MultiQueueWaiter* waiter)
    : for_stdout(waiter), for_querydb(waiter) {}

Index_Request::Index_Request(const std::string& path,
                             const std::vector<std::string>& args,
                             bool is_interactive,
                             optional<std::string> contents)
    : path(path),
      args(args),
      is_interactive(is_interactive),
      contents(contents) {}

Index_DoIdMap::Index_DoIdMap(std::unique_ptr<IndexFile> current,
                             PerformanceImportFile perf,
                             bool is_interactive,
                             bool write_to_disk)
    : current(std::move(current)),
      perf(perf),
      is_interactive(is_interactive),
      write_to_disk(write_to_disk) {
  assert(this->current);
}

Index_OnIdMapped::File::File(std::unique_ptr<IndexFile> file,
                             std::unique_ptr<IdMap> ids)
    : file(std::move(file)), ids(std::move(ids)) {}

Index_OnIdMapped::Index_OnIdMapped(PerformanceImportFile perf,
                                   bool is_interactive,
                                   bool write_to_disk)
    : perf(perf),
      is_interactive(is_interactive),
      write_to_disk(write_to_disk) {}

Index_OnIndexed::Index_OnIndexed(IndexUpdate& update,
                                 PerformanceImportFile perf)
    : update(update), perf(perf) {}

QueueManager::QueueManager(MultiQueueWaiter* waiter)
    : index_request(waiter),
      do_id_map(waiter),
      load_previous_index(waiter),
      on_id_mapped(waiter),
      on_indexed(waiter) {}

bool QueueManager::HasWork() {
  return !index_request.IsEmpty() || !do_id_map.IsEmpty() ||
         !load_previous_index.IsEmpty() || !on_id_mapped.IsEmpty() ||
         !on_indexed.IsEmpty();
}
