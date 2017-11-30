#include "query_utils.h"

#include <climits>

namespace {

// Computes roughly how long |range| is.
int ComputeRangeSize(const Range& range) {
  if (range.start.line != range.end.line)
    return INT_MAX;
  return range.end.column - range.start.column;
}

}  // namespace

optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                                      const QueryTypeId& id) {
  QueryType& type = db->types[id.id];
  if (type.def)
    return type.def->definition_spelling;
  return nullopt;
}

optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                                      const QueryFuncId& id) {
  QueryFunc& func = db->funcs[id.id];
  if (func.def)
    return func.def->definition_spelling;
  return nullopt;
}

optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                                      const QueryVarId& id) {
  QueryVar& var = db->vars[id.id];
  if (var.def)
    return var.def->definition_spelling;
  return nullopt;
}

optional<QueryLocation> GetDefinitionSpellingOfSymbol(QueryDatabase* db,
                                                      const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      QueryType& type = db->types[symbol.idx];
      if (type.def)
        return type.def->definition_spelling;
      break;
    }
    case SymbolKind::Func: {
      QueryFunc& func = db->funcs[symbol.idx];
      if (func.def)
        return func.def->definition_spelling;
      break;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->vars[symbol.idx];
      if (var.def)
        return var.def->definition_spelling;
      break;
    }
    case SymbolKind::File:
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return nullopt;
}

optional<QueryLocation> GetDefinitionExtentOfSymbol(QueryDatabase* db,
                                                    const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      QueryType& type = db->types[symbol.idx];
      if (type.def)
        return type.def->definition_extent;
      break;
    }
    case SymbolKind::Func: {
      QueryFunc& func = db->funcs[symbol.idx];
      if (func.def)
        return func.def->definition_extent;
      break;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->vars[symbol.idx];
      if (var.def)
        return var.def->definition_extent;
      break;
    }
    case SymbolKind::File: {
      return QueryLocation(QueryFileId(symbol.idx),
                           Range(Position(1, 1), Position(1, 1)));
    }
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return nullopt;
}

std::string GetHoverForSymbol(QueryDatabase* db, const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      QueryType& type = db->types[symbol.idx];
      if (type.def)
        return type.def->detailed_name;
      break;
    }
    case SymbolKind::Func: {
      QueryFunc& func = db->funcs[symbol.idx];
      if (func.def)
        return func.def->detailed_name;
      break;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->vars[symbol.idx];
      if (var.def)
        return var.def->detailed_name;
      break;
    }
    case SymbolKind::File:
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return "";
}

optional<QueryFileId> GetDeclarationFileForSymbol(QueryDatabase* db,
                                                  const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      QueryType& type = db->types[symbol.idx];
      if (type.def && type.def->definition_spelling)
        return type.def->definition_spelling->path;
      break;
    }
    case SymbolKind::Func: {
      QueryFunc& func = db->funcs[symbol.idx];
      if (!func.declarations.empty())
        return func.declarations[0].path;
      if (func.def && func.def->definition_spelling)
        return func.def->definition_spelling->path;
      break;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->vars[symbol.idx];
      if (var.def && var.def->definition_spelling)
        return var.def->definition_spelling->path;
      break;
    }
    case SymbolKind::File: {
      return QueryFileId(symbol.idx);
    }
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return nullopt;
}

std::vector<QueryLocation> ToQueryLocation(
    QueryDatabase* db,
    const std::vector<QueryFuncRef>& refs) {
  std::vector<QueryLocation> locs;
  locs.reserve(refs.size());
  for (const QueryFuncRef& ref : refs)
    locs.push_back(ref.loc);
  return locs;
}
std::vector<QueryLocation> ToQueryLocation(
    QueryDatabase* db,
    const std::vector<QueryTypeId>& ids) {
  std::vector<QueryLocation> locs;
  locs.reserve(ids.size());
  for (const QueryTypeId& id : ids) {
    optional<QueryLocation> loc = GetDefinitionSpellingOfSymbol(db, id);
    if (loc)
      locs.push_back(loc.value());
  }
  return locs;
}
std::vector<QueryLocation> ToQueryLocation(
    QueryDatabase* db,
    const std::vector<QueryFuncId>& ids) {
  std::vector<QueryLocation> locs;
  locs.reserve(ids.size());
  for (const QueryFuncId& id : ids) {
    optional<QueryLocation> loc = GetDefinitionSpellingOfSymbol(db, id);
    if (loc)
      locs.push_back(loc.value());
  }
  return locs;
}
std::vector<QueryLocation> ToQueryLocation(QueryDatabase* db,
                                           const std::vector<QueryVarId>& ids) {
  std::vector<QueryLocation> locs;
  locs.reserve(ids.size());
  for (const QueryVarId& id : ids) {
    optional<QueryLocation> loc = GetDefinitionSpellingOfSymbol(db, id);
    if (loc)
      locs.push_back(loc.value());
  }
  return locs;
}

std::vector<QueryLocation> GetUsesOfSymbol(QueryDatabase* db,
                                           const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      QueryType& type = db->types[symbol.idx];
      return type.uses;
    }
    case SymbolKind::Func: {
      // TODO: the vector allocation could be avoided.
      QueryFunc& func = db->funcs[symbol.idx];
      std::vector<QueryLocation> result = ToQueryLocation(db, func.callers);
      AddRange(&result, func.declarations);
      if (func.def && func.def->definition_spelling)
        result.push_back(*func.def->definition_spelling);
      return result;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->vars[symbol.idx];
      return var.uses;
    }
    case SymbolKind::File:
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return {};
}

std::vector<QueryLocation> GetDeclarationsOfSymbolForGotoDefinition(
    QueryDatabase* db,
    const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      // Returning the definition spelling of a type is a hack (and is why the
      // function has the postfix `ForGotoDefintion`, but it lets the user
      // jump to the start of a type if clicking goto-definition on the same
      // type from within the type definition.
      QueryType& type = db->types[symbol.idx];
      if (type.def) {
        optional<QueryLocation> declaration = type.def->definition_spelling;
        if (declaration)
          return {*declaration};
      }
      break;
    }
    case SymbolKind::Func: {
      QueryFunc& func = db->funcs[symbol.idx];
      return func.declarations;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->vars[symbol.idx];
      if (var.def) {
        optional<QueryLocation> declaration = var.def->declaration;
        if (declaration)
          return {*declaration};
      }
      break;
    }
    default:
      break;
  }

  return {};
}

optional<QueryLocation> GetBaseDefinitionOrDeclarationSpelling(
    QueryDatabase* db,
    QueryFunc& func) {
  if (!func.def->base)
    return nullopt;
  QueryFunc& base = db->funcs[func.def->base->id];

  optional<QueryLocation> def;
  if (base.def)
    def = base.def->definition_spelling;
  if (!def && !base.declarations.empty())
    def = base.declarations[0];
  return def;
}

bool HasCallersOnSelfOrBaseOrDerived(QueryDatabase* db, QueryFunc& root) {
  // Check self.
  if (!root.callers.empty())
    return true;

  // Check for base calls.
  optional<QueryFuncId> func_id = root.def->base;
  while (func_id) {
    QueryFunc& func = db->funcs[func_id->id];
    if (!func.callers.empty())
      return true;
    if (!func.def)
      break;
    func_id = func.def->base;
  }

  // Check for derived calls.
  std::queue<QueryFuncId> queue;
  PushRange(&queue, root.derived);
  while (!queue.empty()) {
    QueryFunc& func = db->funcs[queue.front().id];
    queue.pop();
    if (!func.callers.empty())
      return true;
    PushRange(&queue, func.derived);
  }

  return false;
}

std::vector<QueryFuncRef> GetCallersForAllBaseFunctions(QueryDatabase* db,
                                                        QueryFunc& root) {
  std::vector<QueryFuncRef> callers;

  optional<QueryFuncId> func_id = root.def->base;
  while (func_id) {
    QueryFunc& func = db->funcs[func_id->id];
    AddRange(&callers, func.callers);

    if (!func.def)
      break;
    func_id = func.def->base;
  }

  return callers;
}

std::vector<QueryFuncRef> GetCallersForAllDerivedFunctions(QueryDatabase* db,
                                                           QueryFunc& root) {
  std::vector<QueryFuncRef> callers;

  std::queue<QueryFuncId> queue;
  PushRange(&queue, root.derived);

  while (!queue.empty()) {
    QueryFunc& func = db->funcs[queue.front().id];
    queue.pop();

    PushRange(&queue, func.derived);
    AddRange(&callers, func.callers);
  }

  return callers;
}

optional<lsPosition> GetLsPosition(WorkingFile* working_file,
                                   const Position& position) {
  if (!working_file)
    return lsPosition(position.line - 1, position.column - 1);

  optional<int> start = working_file->GetBufferLineFromIndexLine(position.line);
  if (!start)
    return nullopt;

  return lsPosition(*start - 1, position.column - 1);
}

optional<lsRange> GetLsRange(WorkingFile* working_file, const Range& location) {
  if (!working_file) {
    return lsRange(
        lsPosition(location.start.line - 1, location.start.column - 1),
        lsPosition(location.end.line - 1, location.end.column - 1));
  }

  optional<int> start =
      working_file->GetBufferLineFromIndexLine(location.start.line);
  optional<int> end =
      working_file->GetBufferLineFromIndexLine(location.end.line);
  if (!start || !end)
    return nullopt;

  // If remapping end fails (end can never be < start), just guess that the
  // final location didn't move. This only screws up the highlighted code
  // region if we guess wrong, so not a big deal.
  //
  // Remapping fails often in C++ since there are a lot of "};" at the end of
  // class/struct definitions.
  if (*end < *start)
    *end = *start + (location.end.line - location.start.line);

  return lsRange(lsPosition(*start - 1, location.start.column - 1),
                 lsPosition(*end - 1, location.end.column - 1));
}

lsDocumentUri GetLsDocumentUri(QueryDatabase* db,
                               QueryFileId file_id,
                               std::string* path) {
  QueryFile& file = db->files[file_id.id];
  if (file.def) {
    *path = file.def->path;
    return lsDocumentUri::FromPath(*path);
  } else {
    *path = "";
    return lsDocumentUri::FromPath("");
  }
}

lsDocumentUri GetLsDocumentUri(QueryDatabase* db, QueryFileId file_id) {
  QueryFile& file = db->files[file_id.id];
  if (file.def) {
    return lsDocumentUri::FromPath(file.def->path);
  } else {
    return lsDocumentUri::FromPath("");
  }
}

optional<lsLocation> GetLsLocation(QueryDatabase* db,
                                   WorkingFiles* working_files,
                                   const QueryLocation& location) {
  std::string path;
  lsDocumentUri uri = GetLsDocumentUri(db, location.path, &path);
  optional<lsRange> range =
      GetLsRange(working_files->GetFileByFilename(path), location.range);
  if (!range)
    return nullopt;
  return lsLocation(uri, *range);
}

NonElidedVector<lsLocation> GetLsLocations(
    QueryDatabase* db,
    WorkingFiles* working_files,
    const std::vector<QueryLocation>& locations) {
  std::unordered_set<lsLocation> unique_locations;
  for (const QueryLocation& query_location : locations) {
    optional<lsLocation> location =
        GetLsLocation(db, working_files, query_location);
    if (!location)
      continue;
    unique_locations.insert(*location);
  }

  NonElidedVector<lsLocation> result;
  result.reserve(unique_locations.size());
  result.assign(unique_locations.begin(), unique_locations.end());
  return result;
}

// Returns a symbol. The symbol will have *NOT* have a location assigned.
optional<lsSymbolInformation> GetSymbolInfo(QueryDatabase* db,
                                            WorkingFiles* working_files,
                                            SymbolIdx symbol) {
  switch (symbol.kind) {
    case SymbolKind::File: {
      QueryFile& file = db->files[symbol.idx];
      if (!file.def)
        return nullopt;

      lsSymbolInformation info;
      info.name = file.def->path;
      info.kind = lsSymbolKind::File;
      return info;
    }
    case SymbolKind::Type: {
      QueryType& type = db->types[symbol.idx];
      if (!type.def)
        return nullopt;

      lsSymbolInformation info;
      info.name = type.def->short_name;
      info.kind = lsSymbolKind::Class;
      return info;
    }
    case SymbolKind::Func: {
      QueryFunc& func = db->funcs[symbol.idx];
      if (!func.def)
        return nullopt;

      lsSymbolInformation info;
      info.name = func.def->short_name;
      info.kind = lsSymbolKind::Function;

      if (func.def->declaring_type.has_value()) {
        QueryType& container = db->types[func.def->declaring_type->id];
        if (container.def) {
          info.kind = lsSymbolKind::Method;
          info.containerName = container.def->short_name;
        }
      }

      return info;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->vars[symbol.idx];
      if (!var.def || var.def->is_local)
        return nullopt;

      lsSymbolInformation info;
      info.name += var.def->short_name;
      info.kind = lsSymbolKind::Variable;
      if (var.def->declaring_type.has_value()) {
        QueryTypeId& declaring_type = var.def->declaring_type.value();
        QueryType& type = db->types[declaring_type.id];
        if (type.def) {
          info.containerName = type.def->short_name;
        }
      }
      return info;
    }
    case SymbolKind::Invalid: {
      return nullopt;
    }
  };

  return nullopt;
}

void AddCodeLens(const char* singular,
                 const char* plural,
                 CommonCodeLensParams* common,
                 QueryLocation loc,
                 const std::vector<QueryLocation>& uses,
                 optional<QueryLocation> excluded,
                 bool force_display) {
  TCodeLens code_lens;
  optional<lsRange> range = GetLsRange(common->working_file, loc.range);
  if (!range)
    return;
  code_lens.range = *range;
  code_lens.command = lsCommand<lsCodeLensCommandArguments>();
  code_lens.command->command = "cquery.showReferences";
  code_lens.command->arguments.uri = GetLsDocumentUri(common->db, loc.path);
  code_lens.command->arguments.position = code_lens.range.start;

  // Add unique uses.
  std::unordered_set<lsLocation> unique_uses;
  for (const QueryLocation& use : uses) {
    if (excluded == use)
      continue;
    optional<lsLocation> location =
        GetLsLocation(common->db, common->working_files, use);
    if (!location)
      continue;
    unique_uses.insert(*location);
  }
  code_lens.command->arguments.locations.assign(unique_uses.begin(),
                                                unique_uses.end());

  // User visible label
  size_t num_usages = unique_uses.size();
  code_lens.command->title = std::to_string(num_usages) + " ";
  if (num_usages == 1)
    code_lens.command->title += singular;
  else
    code_lens.command->title += plural;

  if (force_display || unique_uses.size() > 0)
    common->result->push_back(code_lens);
}

lsWorkspaceEdit BuildWorkspaceEdit(QueryDatabase* db,
                                   WorkingFiles* working_files,
                                   const std::vector<QueryLocation>& locations,
                                   const std::string& new_text) {
  std::unordered_map<QueryFileId, lsTextDocumentEdit> path_to_edit;

  for (auto& location : locations) {
    optional<lsLocation> ls_location =
        GetLsLocation(db, working_files, location);
    if (!ls_location)
      continue;

    if (path_to_edit.find(location.path) == path_to_edit.end()) {
      path_to_edit[location.path] = lsTextDocumentEdit();

      QueryFile& file = db->files[location.path.id];
      if (!file.def)
        continue;

      const std::string& path = file.def->path;
      path_to_edit[location.path].textDocument.uri =
          lsDocumentUri::FromPath(path);

      WorkingFile* working_file = working_files->GetFileByFilename(path);
      if (working_file)
        path_to_edit[location.path].textDocument.version =
            working_file->version;
    }

    lsTextEdit edit;
    edit.range = ls_location->range;
    edit.newText = new_text;

    // vscode complains if we submit overlapping text edits.
    auto& edits = path_to_edit[location.path].edits;
    if (std::find(edits.begin(), edits.end(), edit) == edits.end())
      edits.push_back(edit);
  }

  lsWorkspaceEdit edit;
  for (const auto& changes : path_to_edit)
    edit.documentChanges.push_back(changes.second);
  return edit;
}

std::vector<SymbolRef> FindSymbolsAtLocation(WorkingFile* working_file,
                                             QueryFile* file,
                                             lsPosition position) {
  std::vector<SymbolRef> symbols;
  symbols.reserve(1);

  int target_line = position.line + 1;
  int target_column = position.character + 1;
  if (working_file) {
    optional<int> index_line =
        working_file->GetIndexLineFromBufferLine(target_line);
    if (index_line)
      target_line = *index_line;
  }

  for (const SymbolRef& ref : file->def->all_symbols) {
    if (ref.loc.range.Contains(target_line, target_column))
      symbols.push_back(ref);
  }

  // Order shorter ranges first, since they are more detailed/precise. This is
  // important for macros which generate code so that we can resolving the
  // macro argument takes priority over the entire macro body.
  //
  // Order functions before other types, which makes goto definition work
  // better on constructors.
  std::sort(symbols.begin(), symbols.end(),
            [](const SymbolRef& a, const SymbolRef& b) {
              int a_size = ComputeRangeSize(a.loc.range);
              int b_size = ComputeRangeSize(b.loc.range);

              if (a_size == b_size)
                return a.idx.kind != b.idx.kind &&
                       a.idx.kind == SymbolKind::Func;

              return a_size < b_size;
            });

  return symbols;
}

NonElidedVector<Out_CqueryTypeHierarchyTree::TypeEntry>
BuildParentInheritanceHierarchyForType(QueryDatabase* db,
                                       WorkingFiles* working_files,
                                       QueryTypeId root) {
  QueryType& root_type = db->types[root.id];
  if (!root_type.def)
    return {};

  NonElidedVector<Out_CqueryTypeHierarchyTree::TypeEntry> parent_entries;
  parent_entries.reserve(root_type.def->parents.size());

  for (QueryTypeId parent_id : root_type.def->parents) {
    QueryType& parent_type = db->types[parent_id.id];
    if (!parent_type.def)
      continue;

    Out_CqueryTypeHierarchyTree::TypeEntry parent_entry;
    parent_entry.name = parent_type.def->detailed_name;
    if (parent_type.def->definition_spelling)
      parent_entry.location = GetLsLocation(
          db, working_files, *parent_type.def->definition_spelling);
    parent_entry.children =
        BuildParentInheritanceHierarchyForType(db, working_files, parent_id);

    parent_entries.push_back(parent_entry);
  }

  return parent_entries;
}

optional<Out_CqueryTypeHierarchyTree::TypeEntry>
BuildInheritanceHierarchyForType(QueryDatabase* db,
                                 WorkingFiles* working_files,
                                 QueryTypeId root_id) {
  QueryType& root_type = db->types[root_id.id];
  if (!root_type.def)
    return nullopt;

  Out_CqueryTypeHierarchyTree::TypeEntry entry;

  // Name and location.
  entry.name = root_type.def->detailed_name;
  if (root_type.def->definition_spelling)
    entry.location =
        GetLsLocation(db, working_files, *root_type.def->definition_spelling);

  entry.children.reserve(root_type.derived.size());

  // Base types.
  Out_CqueryTypeHierarchyTree::TypeEntry base;
  base.name = "[[Base]]";
  base.location = entry.location;
  base.children =
      BuildParentInheritanceHierarchyForType(db, working_files, root_id);
  if (!base.children.empty())
    entry.children.push_back(base);

  // Add derived.
  for (QueryTypeId derived : root_type.derived) {
    auto derived_entry =
        BuildInheritanceHierarchyForType(db, working_files, derived);
    if (derived_entry)
      entry.children.push_back(*derived_entry);
  }

  return entry;
}

NonElidedVector<Out_CqueryTypeHierarchyTree::TypeEntry>
BuildParentInheritanceHierarchyForFunc(QueryDatabase* db,
                                       WorkingFiles* working_files,
                                       QueryFuncId root) {
  QueryFunc& root_func = db->funcs[root.id];
  if (!root_func.def || !root_func.def->base)
    return {};

  QueryFunc& parent_func = db->funcs[root_func.def->base->id];
  if (!parent_func.def)
    return {};

  Out_CqueryTypeHierarchyTree::TypeEntry parent_entry;
  parent_entry.name = parent_func.def->detailed_name;
  if (parent_func.def->definition_spelling)
    parent_entry.location =
        GetLsLocation(db, working_files, *parent_func.def->definition_spelling);
  parent_entry.children = BuildParentInheritanceHierarchyForFunc(
      db, working_files, *root_func.def->base);

  NonElidedVector<Out_CqueryTypeHierarchyTree::TypeEntry> entries;
  entries.push_back(parent_entry);
  return entries;
}

optional<Out_CqueryTypeHierarchyTree::TypeEntry>
BuildInheritanceHierarchyForFunc(QueryDatabase* db,
                                 WorkingFiles* working_files,
                                 QueryFuncId root_id) {
  QueryFunc& root_func = db->funcs[root_id.id];
  if (!root_func.def)
    return nullopt;

  Out_CqueryTypeHierarchyTree::TypeEntry entry;

  // Name and location.
  entry.name = root_func.def->detailed_name;
  if (root_func.def->definition_spelling)
    entry.location =
        GetLsLocation(db, working_files, *root_func.def->definition_spelling);

  entry.children.reserve(root_func.derived.size());

  // Base types.
  Out_CqueryTypeHierarchyTree::TypeEntry base;
  base.name = "[[Base]]";
  base.location = entry.location;
  base.children =
      BuildParentInheritanceHierarchyForFunc(db, working_files, root_id);
  if (!base.children.empty())
    entry.children.push_back(base);

  // Add derived.
  for (QueryFuncId derived : root_func.derived) {
    auto derived_entry =
        BuildInheritanceHierarchyForFunc(db, working_files, derived);
    if (derived_entry)
      entry.children.push_back(*derived_entry);
  }

  return entry;
}

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

void InsertSymbolIntoResult(QueryDatabase* db,
                            WorkingFiles* working_files,
                            SymbolIdx symbol,
                            std::vector<lsSymbolInformation>* result) {
  optional<lsSymbolInformation> info = GetSymbolInfo(db, working_files, symbol);
  if (!info)
    return;

  optional<QueryLocation> location = GetDefinitionExtentOfSymbol(db, symbol);
  if (!location) {
    auto decls = GetDeclarationsOfSymbolForGotoDefinition(db, symbol);
    if (decls.empty())
      return;
    location = decls[0];
  }

  optional<lsLocation> ls_location =
      GetLsLocation(db, working_files, *location);
  if (!ls_location)
    return;
  info->location = *ls_location;
  result->push_back(*info);
}
