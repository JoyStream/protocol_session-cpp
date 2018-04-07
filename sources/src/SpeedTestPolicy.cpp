#include <protocol_session/SpeedTestPolicy.hpp>


namespace joystream {
namespace protocol_session {

  SpeedTestPolicy::SpeedTestPolicy() :
    _payloadSize(500000),
    _maxTimeToRespond(std::chrono::seconds(5)),
    _enabled(true),
    _maxPayloadSize(2000000) {

  }

  uint32_t SpeedTestPolicy::payloadSize() const {
    return _payloadSize;
  }

  uint32_t SpeedTestPolicy::maxPayloadSize() const {
    return _maxPayloadSize;
  }

  std::chrono::seconds SpeedTestPolicy::maxTimeToRespond() const {
    return _maxTimeToRespond;
  }

  bool SpeedTestPolicy::isEnabled() const {
    return _enabled;
  }

  void SpeedTestPolicy::setPayloadSize(uint32_t payloadSize) {
    _payloadSize = payloadSize;
  }

  void SpeedTestPolicy::setMaxPayloadSize(uint32_t payloadSize) {
    _maxPayloadSize = payloadSize;
  }

  void SpeedTestPolicy::setMaxTimeToRespond(std::chrono::seconds maxTimeToRespond) {
    _maxTimeToRespond = maxTimeToRespond;
  }

  void SpeedTestPolicy::enable() {
    _enabled = true;
  }

  void SpeedTestPolicy::disable() {
    _enabled = false;
  }
}
}
