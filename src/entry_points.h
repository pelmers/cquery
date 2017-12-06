#pragma once

#include "config.h"
#include "file_consumer.h"
#include "import_manager.h"
#include "ipc_manager.h"
#include "project.h"
#include "semantic_highlight_symbol_cache.h"
#include "threaded_queue.h"
#include "timestamp_manager.h"
#include "work_thread.h"
#include "working_files.h"

// Contains declarations for some of the thread-main functions.

bool QueryDb_ImportMain(Config* config,
                        QueryDatabase* db,
                        ImportManager* import_manager,
                        QueueManager* queue,
                        SemanticHighlightSymbolCache* semantic_cache,
                        WorkingFiles* working_files);

WorkThread::Result IndexMain(Config* config,
                             FileConsumer::SharedState* file_consumer_shared,
                             TimestampManager* timestamp_manager,
                             ImportManager* import_manager,
                             Project* project,
                             WorkingFiles* working_files,
                             MultiQueueWaiter* waiter,
                             QueueManager* queue);