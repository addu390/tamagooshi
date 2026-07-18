#include "commands.h"

namespace tama {

ClaudeCommands::ClaudeCommands(ILink& identity, ITelemetry& telemetry, const DeviceState& state)
    : identity_(identity), telemetry_(telemetry), state_(state) {}

std::string ClaudeCommands::handle(const std::string& cmd) {
  if (cmd == "status") {
    const Telemetry t = telemetry_.read();
    ClaudeStatus status;
    status.name = identity_.deviceName();
    status.secured = identity_.paired();
    status.batt_pct = t.batt_pct;
    status.mV = t.mV;
    status.mA = t.mA;
    status.usb = t.usb;
    status.up_secs = t.up_secs;
    status.heap = t.heap;
    status.approvals = state_.buddy.approvals;
    status.denials = state_.buddy.denials;
    return codec_.encodeStatus(status);
  }
  if (cmd == "unpair") {
    identity_.unpair();
    return codec_.encodeAck("unpair", true);
  }
  if (cmd == "name") {
    return codec_.encodeAck("name", true);
  }
  return {};
}

}  // namespace tama
