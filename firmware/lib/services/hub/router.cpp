#include "router.h"

namespace tama {

void MessageRouter::on(const std::string& type, IMessageHandler& handler) {
  handlers_[type] = &handler;
}

bool MessageRouter::dispatch(const Envelope& env) const {
  const auto it = handlers_.find(env.type);
  if (it == handlers_.end()) return false;
  it->second->handle(env);
  return true;
}

}  // namespace tama
