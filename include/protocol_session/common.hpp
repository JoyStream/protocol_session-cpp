/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, October 7 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_COMMON_HPP
#define JOYSTREAM_PROTOCOLSESSION_COMMON_HPP

#include <common/KeyPair.hpp>
#include <protocol_wire/protocol_wire.hpp>
#include <unordered_map>
#include <string>

namespace joystream {
namespace protocol_session {

template <class ConnectionIdType>
std::string IdToString(const ConnectionIdType &);


/**
 * This class is poorly named, and also is a bit weird, but just sticking with it for now.
 */
struct StartDownloadConnectionInformation {

    StartDownloadConnectionInformation(const protocol_wire::SellerTerms & sellerTerms,
                                       uint32_t index,
                                       int64_t value,
                                       const Coin::KeyPair & buyerContractKeyPair,
                                       const Coin::PubKeyHash & buyerFinalPkHash)
        : sellerTerms(sellerTerms)
        , index(index)
        , value(value)
        , buyerContractKeyPair(buyerContractKeyPair)
        , buyerFinalPkHash(buyerFinalPkHash) {}

    /// Seller

    // Terms which was the basis for the contract initation
    protocol_wire::SellerTerms sellerTerms;

    // Contract output index
    uint32_t index;

    // Contract output value
    int64_t value;

    /// Buyer

    // Contract output buyer multisig key
    Coin::KeyPair buyerContractKeyPair;

    // Payment/Refund buyer output
    Coin::PubKeyHash buyerFinalPkHash;
};

template <class ConnectionIdType>
using PeerToStartDownloadInformationMap = std::unordered_map<ConnectionIdType, StartDownloadConnectionInformation>;

/**
 * @brief The StartDownloadConnectionReadiness enum
 */
enum class PeerNotReadyToStartDownloadCause {
    connection_gone,
    connection_not_in_preparing_contract_state, // this can be due to peer, or an intervening action from us
    terms_expired
};

template <class ConnectionIdType>
using PeersNotReadyToStartDownloadingMap = std::unordered_map<ConnectionIdType, PeerNotReadyToStartDownloadCause>;

enum class PeerNotReadyToStartUploadingCause {
    connection_gone,
    connection_not_in_invited_state,
    terms_expired
};

}
}
#endif // JOYSTREAM_PROTOCOLSESSION_COMMON_HPP
