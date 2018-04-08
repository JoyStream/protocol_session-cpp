/**
 * Copyright (C) JoyStream - All Rights Reserved
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_SPEEDTESTPOLICY_HPP
#define JOYSTREAM_PROTOCOLSESSION_SPEEDTESTPOLICY_HPP

#include <chrono>

namespace joystream {
namespace protocol_session {

  class SpeedTestPolicy {
    public:
      SpeedTestPolicy();

      uint32_t payloadSize() const;
      uint32_t maxPayloadSize() const;
      std::chrono::seconds maxTimeToRespond() const;
      bool isEnabled() const;
      bool disconnectIfSlow() const;

      void setPayloadSize(uint32_t);
      void setMaxPayloadSize(uint32_t);
      void setMaxTimeToRespond(std::chrono::seconds);
      void enable();
      void disable();
      void setDisconnectIfSlow(bool);

    private:

      uint32_t _payloadSize;
      uint32_t _maxPayloadSize;
      std::chrono::seconds _maxTimeToRespond;
      bool _enabled;
      bool _disconnectIfSlow;

  };

}
}

#endif // JOYSTREAM_PROTOCOLSESSION_SPEEDTESTPOLICY_HPP
