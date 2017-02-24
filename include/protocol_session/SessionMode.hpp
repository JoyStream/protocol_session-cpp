/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, April 28 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_SESSIONMODE_HPP
#define JOYSTREAM_PROTOCOLSESSION_SESSIONMODE_HPP

namespace joystream {
namespace protocol_session {

    enum class SessionMode {
        not_set,
        buying,
        selling,
        observing
    };

}
}

#endif // JOYSTREAM_PROTOCOLSESSION_SESSIONMODE_HPP
