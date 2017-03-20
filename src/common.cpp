/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Lola Rigaut-LUczak <rllola80@gmail.com>, March 20 2017
 */

#include <protocol_session/common.hpp>


namespace joystream {
namespace protocol_session {

const char* PeerNotReadyToStartUploadingCauseToString(PeerNotReadyToStartUploadingCause cause) {
  switch (cause) {
    case PeerNotReadyToStartUploadingCause::connection_gone: return "Connection gone";
    case PeerNotReadyToStartUploadingCause::connection_not_in_invited_state: return "Connection not in invited state";
    case PeerNotReadyToStartUploadingCause::terms_expired: return "Terms expired";
  }
}

}
}
