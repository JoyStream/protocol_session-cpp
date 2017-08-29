/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, May 19 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_SELLERSTATE_HPP
#define JOYSTREAM_PROTOCOLSESSION_SELLERSTATE_HPP

namespace joystream {
namespace protocol_session {

    enum class SellerState {

        // Not been assigned piece
        waiting_to_be_assigned_piece,

        // We have requested a full piece, and we are waiting for it to arrive
        waiting_for_full_piece,

        // We are waiting for libtorrent to fire on_piece_pass() or on_piece_failed()
        // on a full piece which was recently received
        waiting_for_piece_validation_and_storage,

        // Peer is no longer connected
        gone
    };

}
}

#endif // JOYSTREAM_PROTOCOLSESSION_SELLERSTATE_HPP
