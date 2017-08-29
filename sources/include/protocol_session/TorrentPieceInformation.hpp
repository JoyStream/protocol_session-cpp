/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, April 24 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_TORRENTPIECEINFORMATION_HPP
#define JOYSTREAM_PROTOCOLSESSION_TORRENTPIECEINFORMATION_HPP

#include <vector>
#include <stdint.h>
#include <protocol_session/PieceInformation.hpp>

namespace joystream {
namespace protocol_session {

class TorrentPieceInformation : public std::vector<PieceInformation> {
};

}
}

#endif // JOYSTREAM_PROTOCOLSESSION_TORRENTPIECEINFORMATION_HPP

