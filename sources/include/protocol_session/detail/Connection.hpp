/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, February 9 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_DETAIL_CONNECTION_HPP
#define JOYSTREAM_PROTOCOLSESSION_DETAIL_CONNECTION_HPP

#include <protocol_statemachine/protocol_statemachine.hpp>
#include <protocol_session/detail/PieceDeliveryPipeline.hpp>

#include <common/Network.hpp>
#include <queue>
#include <chrono>

namespace joystream {
namespace protocol_wire {
    class Message;
}
namespace protocol_session {
namespace status {
    template <class ConnectionIdType>
    struct Connection;
}
namespace detail {

    template <class ConnectionIdType>
    class Connection {

    public:

        Connection(const ConnectionIdType &,
                   const protocol_statemachine::PeerAnnouncedMode &,
                   const protocol_statemachine::InvitedToOutdatedContract &,
                   const protocol_statemachine::InvitedToJoinContract &,
                   const protocol_statemachine::Send &,
                   const protocol_statemachine::ContractIsReady &,
                   const protocol_statemachine::PieceRequested &,
                   const protocol_statemachine::InvalidPieceRequested &,
                   const protocol_statemachine::PeerInterruptedPayment &,
                   const protocol_statemachine::ValidPayment &,
                   const protocol_statemachine::InvalidPayment &,
                   const protocol_statemachine::SellerJoined &,
                   const protocol_statemachine::SellerInterruptedContract &,
                   const protocol_statemachine::ReceivedFullPiece &,
                   const protocol_statemachine::MessageOverflow &,
                   const protocol_statemachine::MessageOverflow &,
                   const protocol_statemachine::SellerCompletedSpeedTest &,
                   const protocol_statemachine::BuyerRequestedSpeedTest &,
                   Coin::Network network,
                   const std::function<std::chrono::high_resolution_clock::time_point()> &);

        // Processes given message
        template<class M>
        void processMessage(const M &);

        // Process given event
        void processEvent(const boost::statechart::event_base &);

        // Whether state machine is in given (T) inner state
        template<typename T>
        bool inState() const;

        // Id of given connection
        ConnectionIdType connectionId() const;

        // Peer terms announced
        protocol_statemachine::AnnouncedModeAndTerms announcedModeAndTermsFromPeer() const;

        // Payor in state machine: only used when selling
        paymentchannel::Payee payee() const;

        // Payee in state machine: only used when buying
        paymentchannel::Payor payor() const;

        // Connection state machine reference
        //protocol_statemachine::CBStateMachine & machine();

        // MAX_PIECE_INDEX
        int maxPieceIndex() const;
        void setMaxPieceIndex(int);

        // Statu of connection
        status::Connection<ConnectionIdType> status() const;

        PieceDeliveryPipeline & pieceDeliveryPipeline();

        void startingSpeedTest();
        void endingSpeedTest();
        bool hasStartedSpeedTest() const;
        bool hasCompletedSpeedTest() const;
        bool speedTestCompletedInLessThan(std::chrono::seconds);
        int32_t timeToDeliverTestPayload() const;

    private:

        // Connection id
        ConnectionIdType _connectionId;

        // State machine for this connection
        protocol_statemachine::CBStateMachine _machine;

        //// Buyer

        //// Selling
        PieceDeliveryPipeline _pieceDeliveryPipeline;

        //// Speed Testing - used by when buying and selling
        // buyer: records time when request to seller was sent
        // seller: records time when request arrived from buyer
        boost::optional<std::chrono::high_resolution_clock::time_point> _startedSpeedTestAt;

        // buyer: records time when the test payload was received from seller
        boost::optional<std::chrono::high_resolution_clock::time_point> _completedSpeedTestAt;

        std::function<std::chrono::high_resolution_clock::time_point()> _getTime;
    };

    template <class ConnectionIdType>
    using ConnectionMap = std::map<ConnectionIdType, Connection<ConnectionIdType> *>;

}
}
}

// Templated type defenitions
#include <protocol_session/detail/Connection.cpp>

#endif // JOYSTREAM_PROTOCOLSESSION_DETAIL_CONNECTION_HPP
