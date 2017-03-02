#include "ipc.h"

namespace {
  JsonMessage* as_message(char* ptr) {
    return reinterpret_cast<JsonMessage*>(ptr);
  }
}

const char* JsonMessage::payload() {
  return reinterpret_cast<const char*>(this) + sizeof(JsonMessage);
}

void JsonMessage::SetPayload(size_t payload_size, const char* payload) {
  char* payload_dest = reinterpret_cast<char*>(this) + sizeof(JsonMessage);
  this->payload_size = payload_size;
  memcpy(payload_dest, payload, payload_size);
}

IpcMessage_IsAlive::IpcMessage_IsAlive() {
  kind = JsonMessage::Kind::IsAlive;
}

void IpcMessage_IsAlive::Serialize(Writer& writer) {}

void IpcMessage_IsAlive::Deserialize(Reader& reader) {}

IpcMessage_ImportIndex::IpcMessage_ImportIndex() {
  kind = JsonMessage::Kind::ImportIndex;
}

void IpcMessage_ImportIndex::Serialize(Writer& writer) {
  writer.StartObject();
  ::Serialize(writer, "path", path);
  writer.EndObject();
}
void IpcMessage_ImportIndex::Deserialize(Reader& reader) {
  ::Deserialize(reader, "path", path);
}

IpcMessage_CreateIndex::IpcMessage_CreateIndex() {
  kind = JsonMessage::Kind::CreateIndex;
}

void IpcMessage_CreateIndex::Serialize(Writer& writer) {
  writer.StartObject();
  ::Serialize(writer, "path", path);
  ::Serialize(writer, "args", args);
  writer.EndObject();
}
void IpcMessage_CreateIndex::Deserialize(Reader& reader) {
  ::Deserialize(reader, "path", path);
  ::Deserialize(reader, "args", args);
}

IpcMessageQueue::IpcMessageQueue(const std::string& name) {
  local_block = new char[shmem_size];
  shared = CreatePlatformSharedMemory(name + "_memory");
  mutex = CreatePlatformMutex(name + "_mutex");
}

IpcMessageQueue::~IpcMessageQueue() {
  delete[] local_block;
}

void IpcMessageQueue::PushMessage(BaseIpcMessage* message) {
  rapidjson::StringBuffer output;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output);
  writer.SetFormatOptions(
    rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
  writer.SetIndent(' ', 2);
  message->Serialize(writer);

  size_t payload_size = strlen(output.GetString());
  assert(payload_size < shmem_size && "Increase shared memory size, payload will never fit");

  bool first = true;
  bool did_log = false;
  while (true) {
    using namespace std::chrono_literals;
    if (!first) {
      if (!did_log) {
        std::cout << "[info]: shmem full, waiting" << std::endl; // TODO: remove
        did_log = true;
      }
      std::this_thread::sleep_for(16ms);
    }
    first = false;

    std::unique_ptr<PlatformScopedMutexLock> lock = CreatePlatformScopedMutexLock(mutex.get());

    // Try again later when there is room in shared memory.
    if ((*shared->shared_bytes_used + sizeof(JsonMessage) + payload_size) >= shmem_size)
      continue;

    get_free_message()->kind = message->kind;
    get_free_message()->SetPayload(payload_size, output.GetString());

    *shared->shared_bytes_used += sizeof(JsonMessage) + get_free_message()->payload_size;
    assert(*shared->shared_bytes_used < shmem_size);
    get_free_message()->kind = JsonMessage::Kind::Invalid;
    break;
  }

}

std::vector<std::unique_ptr<BaseIpcMessage>> IpcMessageQueue::PopMessage() {
  size_t remaining_bytes = 0;
  // Move data from shared memory into a local buffer. Do this
  // before parsing the blocks so that other processes can begin
  // posting data as soon as possible.
  {
    std::unique_ptr<PlatformScopedMutexLock> lock = CreatePlatformScopedMutexLock(mutex.get());
    remaining_bytes = *shared->shared_bytes_used;

    memcpy(local_block, shared->shared_start, *shared->shared_bytes_used);
    *shared->shared_bytes_used = 0;
    get_free_message()->kind = JsonMessage::Kind::Invalid;
  }

  std::vector<std::unique_ptr<BaseIpcMessage>> result;

  char* message = local_block;
  while (remaining_bytes > 0) {
    std::unique_ptr<BaseIpcMessage> base_message;
    switch (as_message(message)->kind) {
    case JsonMessage::Kind::IsAlive:
      base_message = std::make_unique<IpcMessage_IsAlive>();
      break;
    case JsonMessage::Kind::CreateIndex:
      base_message = std::make_unique<IpcMessage_CreateIndex>();
      break;
    case JsonMessage::Kind::ImportIndex:
      base_message = std::make_unique<IpcMessage_ImportIndex>();
      break;
    default:
      assert(false);
    }

    rapidjson::Document document;
    document.Parse(as_message(message)->payload(), as_message(message)->payload_size);
    bool has_error = document.HasParseError();
    auto error = document.GetParseError();

    base_message->Deserialize(document);

    result.emplace_back(std::move(base_message));

    remaining_bytes -= sizeof(JsonMessage) + as_message(message)->payload_size;
    message = message + sizeof(JsonMessage) + as_message(message)->payload_size;
  }

  return result;
}