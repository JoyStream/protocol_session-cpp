/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, May 19 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_BUYINGSTATE_HPP
#define JOYSTREAM_PROTOCOLSESSION_BUYINGSTATE_HPP

namespace joystream {
namespace protocol_session {

    enum class BuyingState {

        // Inviting sellers:
        // Peers with good enough terms are invited, even when paused,
        sending_invitations,

        // Requesting and downloading pieces:
        // Contract has been announced and exchanges are ongoing.
        downloading,

        // Have all pieces
        download_completed
    };
}
}

#endif // JOYSTREAM_PROTOCOLSESSION_BUYINGSTATE_HPP

