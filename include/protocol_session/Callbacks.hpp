/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, April 25 2016
 */


#ifndef JOYSTREAM_PROTOCOLSESSION_CALLBACKS_HPP
#define JOYSTREAM_PROTOCOLSESSION_CALLBACKS_HPP

#include <protocol_session/common.hpp>
#include <common/P2SHAddress.hpp>
#include <common/P2PKHAddress.hpp>
#include <CoinCore/CoinNodeData.h>

//#include <protocol_statemachine/CBStateMachine.hpp> // send message callbacks

#include <functional>
#include <vector>

namespace Coin {
    class typesafeOutPoint;
    class PublicKey;
}

namespace joystream {
namespace paymentchannel {
    class Payee;
    class ContractTransactionBuilder;
}
namespace protocol_statemachine {
    struct Send;
}

namespace protocol_session {

// NB: In the future one can separete into two different callbacks,
// one for each mode, with its own set of DisconnectCauses.
enum class DisconnectCause {

    // session stops, removing connection
    client,

    //// buying

    seller_has_interrupted_contract,

    seller_servicing_piece_has_timed_out,

    seller_sent_invalid_piece,

    //// selling

    //buyer_invited_with_bad_terms,

    buyer_requested_invalid_piece,

    buyer_interrupted_payment,

    buyer_sent_invalid_payment
};

// Removal of a connection from the session: c++11 alias declaration
template <class ConnectionIdType>
using RemovedConnectionCallbackHandler = std::function<void(const ConnectionIdType &, DisconnectCause)>;

/**
// P2SH redeem script generator callback - generates script from a Coin::PublicKey
typedef std::function<uchar_vector(const Coin::PublicKey &)> P2SHScriptGeneratorFromPubKey;

// Generate a single key pair (for payment channel)
typedef std::function<Coin::KeyPair(const P2SHScriptGeneratorFromPubKey&, const uchar_vector&)> GenerateP2SHKeyPairCallbackHandler;

// Generate set of receive p2pkh addresses
typedef std::function<std::vector<Coin::P2PKHAddress>(int)> GenerateReceiveAddressesCallbackHandler;

// Generate set of change p2pkh addresses
typedef std::function<std::vector<Coin::P2PKHAddress>(int)> GenerateChangeAddressesCallbackHandler;
*/

// Send message callbacks
typedef protocol_statemachine::Send SendMessageOnConnectionCallbacks;

//// Buying

// Process arrival of a full piece, with given index over peer connection with given id
template <class ConnectionIdType>
using FullPieceArrived = std::function<void(const ConnectionIdType &, const protocol_wire::PieceData &, int)>;

// Buyer with givne connection id send a valid payment
template <class ConnectionIdType>
using SentPayment = std::function<void(const ConnectionIdType &,
                                       uint64_t paymentIncrement,
                                       uint64_t totalNumberOfPayments,
                                       uint64_t totalAmountPaid,
                                       int pieceIndex)>;

//// Selling

// Notification that buyer with given connection ids has one outstanding payment
//template <class ConnectionIdType>
//using HasOutstandingPayment = std::function<void(const ConnectionIdType &)>;

// Buyer with given connection id requires piece with given index
template <class ConnectionIdType>
using LoadPieceForBuyer = std::function<void(const ConnectionIdType &, int)>;

// Buyer with given connection id caused invitedToOutdatedContract event
//template <class ConnectionIdType>
//using StaleContractInvitation = std::function<void(const ConnectionIdType &)>;

// Buyer with given connection id requires piece with given index
template <class ConnectionIdType>
using ClaimLastPayment = std::function<void(const ConnectionIdType &, const joystream::paymentchannel::Payee &)>;

// Buyer with given connection id announced anchor
template <class ConnectionIdType>
using AnchorAnnounced = std::function<void(const ConnectionIdType &, uint64_t, const Coin::typesafeOutPoint &, const Coin::PublicKey &, const Coin::PubKeyHash &)>;

// Buyer with givne connection id send a valid payment
template <class ConnectionIdType>
using ReceivedValidPayment = std::function<void(const ConnectionIdType &,
                                                uint64_t paymentIncrement,
                                                uint64_t totalNumberOfPayments,
                                                uint64_t totalAmountPaid)>;

}
}

#endif // JOYSTREAM_PROTOCOLSESSION_CALLBACKS_HPP

