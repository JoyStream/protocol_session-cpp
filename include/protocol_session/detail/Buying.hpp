/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, April 9 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_BUYING_HPP
#define JOYSTREAM_PROTOCOLSESSION_BUYING_HPP

#include <protocol_session/Session.hpp>
#include <protocol_session/detail/Connection.hpp>
#include <protocol_session/Callbacks.hpp>
#include <protocol_session/BuyingState.hpp>
#include <protocol_session/detail/Piece.hpp>
#include <protocol_session/detail/Seller.hpp>
#include <protocol_wire/protocol_wire.hpp>
#include <CoinCore/CoinNodeData.h>

#include <vector>

namespace joystream {
namespace protocol_statemachine {
    class AnnouncedModeAndTerms;
}
namespace protocol_session {

class TorrentPieceInformation;
class ContractAnnouncement;
enum class StartDownloadConnectionReadiness;

namespace detail {

template <class ConnectionIdType>
class Selling;

template <class ConnectionIdType>
class Observing;

template <class ConnectionIdType>
class Buying {

public:

    Buying(Session<ConnectionIdType> *,
           const RemovedConnectionCallbackHandler<ConnectionIdType> &,
           const FullPieceArrived<ConnectionIdType> &,
           const SentPayment<ConnectionIdType> &,
           const protocol_wire::BuyerTerms &,
           const TorrentPieceInformation &);

    //// Connection level client events

    // Adds connection, and return the current number of connections
    uint addConnection(const ConnectionIdType &, const SendMessageOnConnectionCallbacks &);

    // Remove connection
    void removeConnection(const ConnectionIdType &);

    // Transition to BuyingState::sending_invitations
    void startDownloading(const Coin::Transaction & contractTx,
                          const PeerToStartDownloadInformationMap<ConnectionIdType> & peerToStartDownloadInformationMap);

    // A valid piece was sent too us on given connection
    void validPieceReceivedOnConnection(const ConnectionIdType &, int index);

    // An invalid piece was sent too us on given connection
    // Should not be called when session is stopped.
    void invalidPieceReceivedOnConnection(const ConnectionIdType &, int index);

    //// Connection level state machine events

    void peerAnnouncedModeAndTerms(const ConnectionIdType &, const protocol_statemachine::AnnouncedModeAndTerms &);
    void sellerHasJoined(const ConnectionIdType &);
    void sellerHasInterruptedContract(const ConnectionIdType &);
    void receivedFullPiece(const ConnectionIdType &, const protocol_wire::PieceData &);

    //// Change mode

    void leavingState();

    //// Change state

    // Starts a stopped session by becoming fully operational
    void start();

    // Immediately closes all existing connections
    void stop();

    // Pause session
    // Accepts new connections, but only advertises mode.
    // All existing connections are gracefully paused so that all
    // incoming messages can be ignored. In particular it
    // honors last pending payment, but issues no new piece requests.
    void pause();

    //// Miscellenous

    // Time out processing hook
    // NB: Later give some indication of how to set timescale for this call
    void tick();

    // Piece with given index has been downloaded, but not through
    // a regitered connection. Could be non-joystream peers, or something out of bounds.
    void pieceDownloaded(int);

    // Update terms
    void updateTerms(const protocol_wire::BuyerTerms &);

    // Status of Buying
    status::Buying<ConnectionIdType> status() const;

    protocol_wire::BuyerTerms terms() const;

    void setPickNextPieceMethod(const PickNextPieceMethod<ConnectionIdType> & pickNextPieceMethod);

private:

    //// Assigning pieces

    // Tries to assign an unassigned piece to given seller
    bool tryToAssignAndRequestPiece(detail::Seller<ConnectionIdType> &);

    //// Utility routines

    // Prepare given connection for deletion due to given cause
    // Returns iterator to next valid element
    typename detail::ConnectionMap<ConnectionIdType>::const_iterator removeConnection(const ConnectionIdType &, DisconnectCause);

    // Removes given seller
    void removeSeller(detail::Seller<ConnectionIdType> &);

    //
    void politeSellerCompensation();

    // Unguarded
    void _start();

    //// Members

    // Reference to core of session
    Session<ConnectionIdType> * _session;

    // Callback handlers
    RemovedConnectionCallbackHandler<ConnectionIdType> _removedConnection;
    FullPieceArrived<ConnectionIdType> _fullPieceArrived;
    SentPayment<ConnectionIdType> _sentPayment;

    // State
    BuyingState _state;

    // Terms for buying
    protocol_wire::BuyerTerms _terms;

    // Maps connection identifier to connection
    std::map<ConnectionIdType, detail::Seller<ConnectionIdType>> _sellers;

    // Contract transaction id
    // NB** Must be stored, as signatures are non-deterministic
    // contributions to the TxId, and hence discarding them
    // ***When segwit is enforced, this will no longer be neccessary.***
    //Coin::Transaction _contractTx;

    // Pieces in torrent file
    std::vector<detail::Piece<ConnectionIdType>> _pieces;

    // The number of pieces not yet downloaded.
    // Is used to detect when we are done.
    uint32_t _numberOfMissingPieces;

    // Indexes of pieces which were assigned, but then later
    // unassigned for some reason (e.g. seller left, timed out, etc).
    // They are prioritized when assiging new pieces.
    // NB: using std::deque over std::queue, since latter
    std::deque<uint32_t> _deAssignedPieces;

    // Index before which we should never assign a piece unless all pieces
    // with a greater index have been assigned. Following this constraint
    // results in in-order downloading of pieces, e.g. for media streaming.
    // Will typically also be reset of client desires to set streaming of media to
    // some midway point
    uint32_t _assignmentLowerBound;

    // When we started sending out invitations
    // (i.e. entered state StartedState::sending_invitations).
    // Is used to figure out when to start trying to build the contract
    std::chrono::high_resolution_clock::time_point _lastStartOfSendingInvitations;

    // Function that if defined will return the next piece that we should download
    PickNextPieceMethod<ConnectionIdType> _pickNextPieceMethod;
};

}
}
}

// Templated type defenitions
#include <protocol_session/detail/Buying.cpp>

#endif // JOYSTREAM_PROTOCOLSESSION_BUYING_HPP
