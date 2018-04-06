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
           const TorrentPieceInformation &,
           const AllSellersGone &,
           std::chrono::duration<double> = std::chrono::duration<double>::zero());

    //// Connection level client events

    // Adds connection, and return the current number of connections
    uint addConnection(const ConnectionIdType &, const SendMessageOnConnectionCallbacks &);

    // Remove connection
    void removeConnection(const ConnectionIdType &);

    // Transition to BuyingState::sending_invitations
    void startDownloading(const Coin::Transaction & contractTx,
                          const PeerToStartDownloadInformationMap<ConnectionIdType> & peerToStartDownloadInformationMap);

    //// Connection level state machine events

    void peerAnnouncedModeAndTerms(const ConnectionIdType &, const protocol_statemachine::AnnouncedModeAndTerms &);
    void sellerHasJoined(const ConnectionIdType &);
    void sellerHasInterruptedContract(const ConnectionIdType &);
    void receivedFullPiece(const ConnectionIdType &, const protocol_wire::PieceData &);
    void remoteMessageOverflow(const ConnectionIdType &);
    void sellerCompletedSpeedTest(const ConnectionIdType &, bool);

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

    void sendInvitations () const;

    void maybeInviteSeller(detail::Connection<ConnectionIdType> *, protocol_statemachine::AnnouncedModeAndTerms) const;

    void resetIfAllSellersGone ();

    //// Assigning pieces

    // Tries to assign pieces to given seller
    int tryToAssignAndRequestPieces(detail::Seller<ConnectionIdType> &);

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

    // A valid piece was sent to us on given connection
    void validPieceReceivedOnConnection(detail::Seller<ConnectionIdType> &, int index);

    // An invalid piece was sent to us on given connection
    void invalidPieceReceivedOnConnection(detail::Seller<ConnectionIdType> &, int index);

    //// Members

    // Reference to core of session
    Session<ConnectionIdType> * _session;

    // Callback handlers
    RemovedConnectionCallbackHandler<ConnectionIdType> _removedConnection;
    FullPieceArrived<ConnectionIdType> _fullPieceArrived;
    SentPayment<ConnectionIdType> _sentPayment;
    AllSellersGone _allSellersGone;

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

    // When we started sending out invitations
    // (i.e. entered state StartedState::sending_invitations).
    // Is used to figure out when to start trying to build the contract
    std::chrono::high_resolution_clock::time_point _lastStartOfSendingInvitations;

    // Function that if defined will return the next piece that we should download
    PickNextPieceMethod<ConnectionIdType> _pickNextPieceMethod;

    // Maximum number of concurrent requests to send before waiting for piece responses
    // The optimum value depends on many factors. It is hardcoded to 4 for now.
    const int _maxConcurrentRequests;

    std::chrono::duration<double> _maxTimeToServicePiece;

    // Do we need to ask sellers to perform a speed test
    bool _speedTestPolicyEnabled;
    uint32_t _speedTestPolicyPayloadSize;
};

}
}
}

// Templated type defenitions
#include <protocol_session/detail/Buying.cpp>

#endif // JOYSTREAM_PROTOCOLSESSION_BUYING_HPP
