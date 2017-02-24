/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, May 19 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_PIECESTATE_HPP
#define JOYSTREAM_PROTOCOLSESSION_PIECESTATE_HPP

namespace joystream {
namespace protocol_session {

    enum class PieceState {

        // We do not have piece, and it has given assignment status
        unassigned,

        // Currently assigned to seller for being downloaded
        being_downloaded,

        // Currently assigned to seller, and is being validated and stored by client
        being_validated_and_stored,

        // Valid copy is stored
        downloaded
    };

}
}

#endif // JOYSTREAM_PROTOCOLSESSION_PIECESTATE_HPP

