/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, April 21 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_STATUS
#define JOYSTREAM_PROTOCOLSESSION_STATUS

#include <protocol_session/PieceState.hpp>
#include <protocol_session/BuyingState.hpp>
#include <protocol_session/SessionMode.hpp>
#include <protocol_session/SessionState.hpp>
#include <protocol_statemachine/protocol_statemachine.hpp>

#include <CoinCore/CoinNodeData.h> // Coin::Transaction

#include <queue>

namespace joystream {
namespace protocol_session {
namespace status {

    struct CBStateMachine {

        // CBStateMachine()
        //     : innerStateTypeIndex(typeid(protocol_statemachine::ChooseMode)) {}

        CBStateMachine(const std::type_index & innerStateTypeIndex,
                       const protocol_statemachine::AnnouncedModeAndTerms & announcedModeAndTermsFromPeer,
                       const paymentchannel::Payor & payor,
                       const paymentchannel::Payee & payee,
                       const boost::optional<std::chrono::milliseconds> latency)
            : innerStateTypeIndex(innerStateTypeIndex)
            , announcedModeAndTermsFromPeer(announcedModeAndTermsFromPeer)
            , payor(payor)
            , payee(payee)
            , latency(latency) {
        }

        // Type index of innermost currently active state
        std::type_index innerStateTypeIndex;

        //// Peer state

        protocol_statemachine::AnnouncedModeAndTerms announcedModeAndTermsFromPeer;

        //// Buyer Client state

        // Payor side of payment channel interaction
        paymentchannel::Payor payor;

        //// Seller Client state

        // Payee side of payment channel interaction
        paymentchannel::Payee payee;

        // Time it took to successfully deliver test payload
        boost::optional<std::chrono::milliseconds> latency;

    };

    template <class ConnectionIdType>
    struct Connection {

        Connection() {}

        Connection(const ConnectionIdType & connectionId,
                   const CBStateMachine & machine)
            : connectionId(connectionId)
            , machine(machine) {
        }

        // Connection id
        ConnectionIdType connectionId;

        // State machine for this connection
        CBStateMachine machine;

        //// Buyer

        //// Selling
    };

    template <class ConnectionIdType>
    struct Piece {

        Piece() {}

        Piece(int index, PieceState state, const ConnectionIdType & connectionId, unsigned int size)
            : index(index)
            , state(state)
            , connectionId(connectionId)
            , size(size) {}

        // Index of piece
        int index;

        // Piece state
        PieceState state;

        // Id of connectionto which piece is assigned, when _state == State::assigned_to_peer_for_download
        ConnectionIdType connectionId;

        // Byte length of piece (should be the same for all but last piece)
        unsigned int size;
    };

    template <class ConnectionIdType>
    struct Seller {

        Seller() {}

        Seller(ConnectionIdType connection)
            : connection(connection) {
        }

        // Connection identifier for seller
        ConnectionIdType connection;
    };

    template <class ConnectionIdType>
    struct Buying {

        Buying() {}

        Buying(const BuyingState state,
               const protocol_wire::BuyerTerms & terms,
               const std::map<ConnectionIdType, Seller<ConnectionIdType>> & sellers,
               //const Coin::Transaction & contractTx,
               const std::vector<Piece<ConnectionIdType>> & pieces)
            : state(state)
            , terms(terms)
            , sellers(sellers)
            //, contractTx(contractTx)
            , pieces(pieces) {
        }

        // State
        BuyingState state;

        // Terms for buying
        protocol_wire::BuyerTerms terms;

        // Maps connection identifier to connection
        std::map<ConnectionIdType, Seller<ConnectionIdType>> sellers;

        // Contract transaction id
        //Coin::Transaction contractTx;

        // Pieces in torrent file
        std::vector<Piece<ConnectionIdType>> pieces;
    };

    struct Selling {

        Selling() {}

        Selling(const protocol_wire::SellerTerms & terms)
            : terms(terms) {
        }

        // Terms for selling
        protocol_wire::SellerTerms terms;

    };

    template <class ConnectionIdType>
    struct Session {

        Session() {}

        Session(SessionMode mode, SessionState state, const Selling & selling, const Buying<ConnectionIdType>  & buying)
            : mode(mode)
            , state(state)
            , selling(selling)
            , buying(buying) {
        }

        // Session mode
        SessionMode mode;

        // Current state of session
        SessionState state;

        //// Substates

        // Seller
        Selling selling;

        // Buyer
        Buying<ConnectionIdType> buying;
    };

}
}
}

#endif // JOYSTREAM_PROTOCOLSESSION_STATUS
