#include "message_handler.h"
#include "query_utils.h"

namespace {
struct Ipc_CqueryCallTreeInitial
    : public IpcMessage<Ipc_CqueryCallTreeInitial> {
  const static IpcId kIpcId = IpcId::CqueryCallTreeInitial;
  lsRequestId id;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryCallTreeInitial, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryCallTreeInitial);

struct Ipc_CqueryCallTreeExpand : public IpcMessage<Ipc_CqueryCallTreeExpand> {
  struct Params {
    std::string usr;
  };
  const static IpcId kIpcId = IpcId::CqueryCallTreeExpand;
  lsRequestId id;
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryCallTreeExpand::Params, usr);
MAKE_REFLECT_STRUCT(Ipc_CqueryCallTreeExpand, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryCallTreeExpand);

struct Out_CqueryCallTree : public lsOutMessage<Out_CqueryCallTree> {
  enum class CallType { Direct = 0, Base = 1, Derived = 2 };
  struct CallEntry {
    std::string name;
    std::string usr;
    lsLocation location;
    bool hasCallers = true;
    CallType callType = CallType::Direct;
  };

  lsRequestId id;
  NonElidedVector<CallEntry> result;
};
MAKE_REFLECT_TYPE_PROXY(Out_CqueryCallTree::CallType, int);
MAKE_REFLECT_STRUCT(Out_CqueryCallTree::CallEntry,
                    name,
                    usr,
                    location,
                    hasCallers,
                    callType);
MAKE_REFLECT_STRUCT(Out_CqueryCallTree, jsonrpc, id, result);

NonElidedVector<Out_CqueryCallTree::CallEntry> BuildInitialCallTree(
    QueryDatabase* db,
    WorkingFiles* working_files,
    QueryFuncId root) {
  QueryFunc& root_func = db->funcs[root.id];
  if (!root_func.def || !root_func.def->definition_spelling)
    return {};
  optional<lsLocation> def_loc =
      GetLsLocation(db, working_files, *root_func.def->definition_spelling);
  if (!def_loc)
    return {};

  Out_CqueryCallTree::CallEntry entry;
  entry.name = root_func.def->short_name;
  entry.usr = root_func.def->usr;
  entry.location = *def_loc;
  entry.hasCallers = HasCallersOnSelfOrBaseOrDerived(db, root_func);
  NonElidedVector<Out_CqueryCallTree::CallEntry> result;
  result.push_back(entry);
  return result;
}

NonElidedVector<Out_CqueryCallTree::CallEntry> BuildExpandCallTree(
    QueryDatabase* db,
    WorkingFiles* working_files,
    QueryFuncId root) {
  QueryFunc& root_func = db->funcs[root.id];
  if (!root_func.def)
    return {};

  auto format_location =
      [&](const lsLocation& location,
          optional<QueryTypeId> declaring_type) -> std::string {
    std::string base;

    if (declaring_type) {
      QueryType type = db->types[declaring_type->id];
      if (type.def)
        base = type.def->detailed_name;
    }

    if (base.empty()) {
      base = location.uri.GetPath();
      size_t last_index = base.find_last_of('/');
      if (last_index != std::string::npos)
        base = base.substr(last_index + 1);
    }

    return base + ":" + std::to_string(location.range.start.line + 1);
  };

  NonElidedVector<Out_CqueryCallTree::CallEntry> result;
  std::unordered_set<QueryLocation> seen_locations;

  auto handle_caller = [&](QueryFuncRef caller,
                           Out_CqueryCallTree::CallType call_type) {
    optional<lsLocation> call_location =
        GetLsLocation(db, working_files, caller.loc);
    if (!call_location)
      return;

    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: REMOVE |seen_locations| once we fix the querydb update bugs
    // TODO: basically, querydb gets duplicate references inserted into it.
    if (!seen_locations.insert(caller.loc).second) {
      std::cerr << "!!!! FIXME DUPLICATE REFERENCE IN QUERYDB" << std::endl;
      return;
    }

    if (caller.has_id()) {
      QueryFunc& call_func = db->funcs[caller.id_.id];
      if (!call_func.def)
        return;

      Out_CqueryCallTree::CallEntry call_entry;
      call_entry.name =
          call_func.def->short_name + " (" +
          format_location(*call_location, call_func.def->declaring_type) + ")";
      call_entry.usr = call_func.def->usr;
      call_entry.location = *call_location;
      call_entry.hasCallers = HasCallersOnSelfOrBaseOrDerived(db, call_func);
      call_entry.callType = call_type;
      result.push_back(call_entry);
    } else {
      // TODO: See if we can do a better job here. Need more information from
      // the indexer.
      Out_CqueryCallTree::CallEntry call_entry;
      call_entry.name = "Likely Constructor";
      call_entry.usr = "no_usr";
      call_entry.location = *call_location;
      call_entry.hasCallers = false;
      call_entry.callType = call_type;
      result.push_back(call_entry);
    }
  };

  std::vector<QueryFuncRef> base_callers =
      GetCallersForAllBaseFunctions(db, root_func);
  std::vector<QueryFuncRef> derived_callers =
      GetCallersForAllDerivedFunctions(db, root_func);
  result.reserve(root_func.callers.size() + base_callers.size() +
                 derived_callers.size());

  for (QueryFuncRef caller : root_func.callers)
    handle_caller(caller, Out_CqueryCallTree::CallType::Direct);
  for (QueryFuncRef caller : base_callers) {
    // Do not show calls to the base function coming from this function.
    if (caller.id_ == root)
      continue;

    handle_caller(caller, Out_CqueryCallTree::CallType::Base);
  }
  for (QueryFuncRef caller : derived_callers)
    handle_caller(caller, Out_CqueryCallTree::CallType::Derived);

  return result;
}

struct CqueryCallTreeInitialHandler
    : BaseMessageHandler<Ipc_CqueryCallTreeInitial> {
  void Run(Ipc_CqueryCallTreeInitial* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_CqueryCallTree out;
    out.id = request->id;

    for (const SymbolRef& ref :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      if (ref.idx.kind == SymbolKind::Func) {
        out.result =
            BuildInitialCallTree(db, working_files, QueryFuncId(ref.idx.idx));
        break;
      }
    }

    IpcManager::WriteStdout(IpcId::CqueryCallTreeInitial, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryCallTreeInitialHandler);

struct CqueryCallTreeExpandHandler
    : BaseMessageHandler<Ipc_CqueryCallTreeExpand> {
  void Run(Ipc_CqueryCallTreeExpand* request) override {
    Out_CqueryCallTree out;
    out.id = request->id;

    auto func_id = db->usr_to_func.find(request->params.usr);
    if (func_id != db->usr_to_func.end())
      out.result = BuildExpandCallTree(db, working_files, func_id->second);

    IpcManager::WriteStdout(IpcId::CqueryCallTreeExpand, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryCallTreeExpandHandler);
}  // namespace