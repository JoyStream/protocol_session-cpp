/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, April 9 2016
 */

#ifndef JOYSTREAM_PROTOCOL_SELLING_HPP
#define JOYSTREAM_PROTOCOL_SELLING_HPP

#include <protocol_session/Session.hpp>
#include <protocol_wire/protocol_wire.hpp>

namespace Coin {
    class typesafeOutPoint;
    class Signature;
}

namespace joystream {
namespace protocol_session {
namespace detail {

template <class ConnectionIdType>
class Buying;

template <class ConnectionIdType>
class Observing;

template <class ConnectionIdType>
class Selling {

public:

    Selling(Session<ConnectionIdType> *,
            const RemovedConnectionCallbackHandler<ConnectionIdType> &,
            const LoadPieceForBuyer<ConnectionIdType> &,
            const ClaimLastPayment<ConnectionIdType> &,
            const AnchorAnnounced<ConnectionIdType> &,
            const ReceivedValidPayment<ConnectionIdType> &,
            const protocol_wire::SellerTerms &,
            int);

    //// Connection level client events

    // Adds connection, and return the current number of connections
    uint addConnection(const ConnectionIdType &, const SendMessageOnConnectionCallbacks &);

    // Connection with given id is to be removed
    void removeConnection(const ConnectionIdType &);

    // Start uploading
    void startUploading(const ConnectionIdType & id,
                        const protocol_wire::BuyerTerms & terms,
                        const Coin::KeyPair & contractKeyPair,
                        const Coin::PubKeyHash & finalPkHash);

    // Data for given piece has been loaded - arrival does not have to be in same order as request to load
    void pieceLoaded(const ConnectionIdType & id, const protocol_wire::PieceData &, int);

    //// Connection level state machine events

    void peerAnnouncedModeAndTerms(const ConnectionIdType &, const protocol_statemachine::AnnouncedModeAndTerms &);
    void invitedToOutdatedContract(const ConnectionIdType &);
    void invitedToJoinContract(const ConnectionIdType &);
    void contractPrepared(const ConnectionIdType &, uint64_t, const Coin::typesafeOutPoint &, const Coin::PublicKey &, const Coin::PubKeyHash &);
    void pieceRequested(const ConnectionIdType &, int);
    void invalidPieceRequested(const ConnectionIdType &);
    void paymentInterrupted(const ConnectionIdType &);
    void receivedValidPayment(const ConnectionIdType &, const Coin::Signature &);
    void receivedInvalidPayment(const ConnectionIdType &, const Coin::Signature &);

    //// Change mode

    void leavingState();

    //// Change state

    // Starts a stopped session by becoming fully operational
    void start();

    // Immediately closes all existing connections
    void stop();

    // Accepts new connections, but only advertises mode.
    void pause();

    //// Miscellenous

    // Time out processing hook
    // NB: Later give some indication of how to set timescale for this call
    void tick();

    // Update terms
    void updateTerms(const protocol_wire::SellerTerms &);

    // Status of state
    status::Selling status() const;

    //// Getters and setters

    protocol_wire::SellerTerms terms() const;

private:

    // Reference to core of session
    Session<ConnectionIdType> * _session;

    // Callback handlers
    RemovedConnectionCallbackHandler<ConnectionIdType> _removedConnection;
    LoadPieceForBuyer<ConnectionIdType> _loadPieceForBuyer;
    ClaimLastPayment<ConnectionIdType> _claimLastPayment;
    AnchorAnnounced<ConnectionIdType> _anchorAnnounced;
    ReceivedValidPayment<ConnectionIdType> _receivedValidPayment;

    // Terms for selling
    protocol_wire::SellerTerms _terms;

    // Maximum piece
    int _MAX_PIECE_INDEX;

    // Maximum number of pieces we will send before we defer sending pieces.
    // Requests are still accepted and will be honored after pending payments arrive.
    // The optimim value depends on many factors such as piece size and connection latency with a peer.
    // For now value is hardcoded to 4
    const int _maxOutstandingPayments;

    // To avoid wasting resources we limit the number of pieces that will be loaded but not sent to _maxPiecesToPreload
    // So the total number of pieces for we will try to load data for is _maxOutstandingPayments + _maxPiecesToPreload
    // The optimim value depends on many factors such as piece size and connection latency with a peer.
    // For now value is hardcoded to 2
    const int _maxPiecesToPreload;

    // Prepare given connection for deletion due to given cause, returns next valid iterator (e.g. end)
    typename detail::ConnectionMap<ConnectionIdType>::const_iterator removeConnection(const ConnectionIdType &, DisconnectCause);

    void tryToSendPieces(detail::Connection<ConnectionIdType> *);

    void tryToLoadPieces(detail::Connection<ConnectionIdType> *);

    // If at least one payment is made, then send claims notification
    void tryToClaimLastPayment(detail::Connection<ConnectionIdType> *);
};

}
}
}

// Templated type defenitions
#include <protocol_session/detail/Selling.cpp>

#endif // JOYSTREAM_PROTOCOL_SELLING_HPP
