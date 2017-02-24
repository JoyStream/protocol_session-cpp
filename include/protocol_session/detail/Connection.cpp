/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, February 9 2016
 */

#include <protocol_session/detail/Connection.hpp>
#include <protocol_session/Status.hpp>
#include <protocol_wire/protocol_wire.hpp>

namespace joystream {
namespace protocol_session {
namespace detail {

    template <class ConnectionIdType>
    Connection<ConnectionIdType>::Connection(const ConnectionIdType & connectionId,
                                             const protocol_statemachine::PeerAnnouncedMode & peerAnnouncedMode,
                                             const protocol_statemachine::InvitedToOutdatedContract & invitedToOutdatedContract,
                                             const protocol_statemachine::InvitedToJoinContract & invitedToJoinContract,
                                             const protocol_statemachine::Send & send,
                                             const protocol_statemachine::ContractIsReady & contractIsReady,
                                             const protocol_statemachine::PieceRequested & pieceRequested,
                                             const protocol_statemachine::InvalidPieceRequested & invalidPieceRequested,
                                             const protocol_statemachine::PeerInterruptedPayment & peerInterruptedPayment,
                                             const protocol_statemachine::ValidPayment & validPayment,
                                             const protocol_statemachine::InvalidPayment & invalidPayment,
                                             const protocol_statemachine::SellerJoined & sellerJoined,
                                             const protocol_statemachine::SellerInterruptedContract & sellerInterruptedContract,
                                             const protocol_statemachine::ReceivedFullPiece & receivedFullPiece)
        : _connectionId(connectionId)
        , _machine(peerAnnouncedMode,
                   invitedToOutdatedContract,
                   invitedToJoinContract,
                   send,
                   contractIsReady,
                   pieceRequested,
                   invalidPieceRequested,
                   peerInterruptedPayment,
                   validPayment,
                   invalidPayment,
                   sellerJoined,
                   sellerInterruptedContract,
                   receivedFullPiece,
                   0)
        , _loadedPiecePending(false) {

        // Initiating state machine
        _machine.initiate();
    }

    template <class ConnectionIdType>
    template <class M>
    void Connection<ConnectionIdType>::processMessage(const M & message) {
        processEvent(protocol_statemachine::event::Recv<M>(message));
    }

    template <class ConnectionIdType>
    void Connection<ConnectionIdType>::processEvent(const boost::statechart::event_base & e) {
        _machine.processEvent(e);
    }

    template <class ConnectionIdType>
    template<typename T>
    bool Connection<ConnectionIdType>::inState() const {
        return _machine. template inState<T>();
    }

    template <class ConnectionIdType>
    ConnectionIdType Connection<ConnectionIdType>::connectionId() const {
        return _connectionId;
    }

    template <class ConnectionIdType>
    protocol_statemachine::AnnouncedModeAndTerms Connection<ConnectionIdType>::announcedModeAndTermsFromPeer() const {
        return _machine.announcedModeAndTermsFromPeer();
    }

    template <class ConnectionIdType>
    paymentchannel::Payee Connection<ConnectionIdType>::payee() const {
        return _machine.payee();
    }

    template <class ConnectionIdType>
    paymentchannel::Payor Connection<ConnectionIdType>::payor() const {
        return _machine.payor();
    }

    template <class ConnectionIdType>
    int Connection<ConnectionIdType>::maxPieceIndex() const {
        return _machine.MAX_PIECE_INDEX();
    }

    template <class ConnectionIdType>
    void Connection<ConnectionIdType>::setMaxPieceIndex(int maxPieceIndex) {
        return _machine.setMAX_PIECE_INDEX(maxPieceIndex);
    }

    /**
    template <class ConnectionIdType>
    protocol_statemachine::CBStateMachine & Connection<ConnectionIdType>::machine() {
        return _machine;
    }
    */

    template <class ConnectionIdType>
    typename status::Connection<ConnectionIdType> Connection<ConnectionIdType>::status() const {
        return status::Connection<ConnectionIdType>(_connectionId,
                                                    status::CBStateMachine(_machine.getInnerStateTypeIndex(),
                                                                           _machine.announcedModeAndTermsFromPeer(),
                                                                           _machine.payor(),
                                                                           _machine.payee()),
                                                    _downloadedValidPieces);
    }

    template <class ConnectionIdType>
    bool Connection<ConnectionIdType>::loadedPiecePending() const {
        return _loadedPiecePending;
    }

    template <class ConnectionIdType>
    void Connection<ConnectionIdType>::setLoadedPiecePending(bool loadedPiecePending) {
        _loadedPiecePending = loadedPiecePending;
    }

    template <class ConnectionIdType>
    protocol_wire::PieceData Connection<ConnectionIdType>::loadedPieceData() const {
        return _loadedPieceData;
    }

    template <class ConnectionIdType>
    void Connection<ConnectionIdType>::setLoadedPieceData(const protocol_wire::PieceData & loadedPieceData) {
        _loadedPieceData = loadedPieceData;
    }

}
}
}
