#include "message_handler.h"

namespace {
struct Ipc_TextDocumentDidClose : public IpcMessage<Ipc_TextDocumentDidClose> {
  struct Params {
    lsTextDocumentItem textDocument;
  };

  const static IpcId kIpcId = IpcId::TextDocumentDidClose;
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidClose::Params, textDocument);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidClose, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDidClose);

struct TextDocumentDidCloseHandler
    : BaseMessageHandler<Ipc_TextDocumentDidClose> {
  void Run(Ipc_TextDocumentDidClose* request) override {
    std::string path = request->params.textDocument.uri.GetPath();

    // Clear any diagnostics for the file.
    Out_TextDocumentPublishDiagnostics out;
    out.params.uri = request->params.textDocument.uri;
    IpcManager::WriteStdout(IpcId::TextDocumentPublishDiagnostics, out);

    // Remove internal state.
    working_files->OnClose(request->params.textDocument);
    clang_complete->NotifyClose(path);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentDidCloseHandler);
}  // namespace