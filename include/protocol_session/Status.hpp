/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, April 21 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_STATUS
#define JOYSTREAM_PROTOCOLSESSION_STATUS

#include <protocol_session/PieceState.hpp>
#include <protocol_session/SellerState.hpp>
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

        CBStateMachine()
            : innerStateTypeIndex(typeid(protocol_statemachine::ChooseMode)) {}

        CBStateMachine(const std::type_index & innerStateTypeIndex,
                       const protocol_statemachine::AnnouncedModeAndTerms & announcedModeAndTermsFromPeer,
                       const paymentchannel::Payor & payor,
                       const paymentchannel::Payee & payee)
            : innerStateTypeIndex(innerStateTypeIndex)
            , announcedModeAndTermsFromPeer(announcedModeAndTermsFromPeer)
            , payor(payor)
            , payee(payee) {
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

    };

    template <class ConnectionIdType>
    struct Connection {

        Connection() {}

        Connection(const ConnectionIdType & connectionId,
                   const CBStateMachine & machine,
                   const std::queue<uint32_t> & downloadedValidPieces)
            : connectionId(connectionId)
            , machine(machine)
            , downloadedValidPieces(downloadedValidPieces) {
        }

        // Connection id
        ConnectionIdType connectionId;

        // State machine for this connection
        CBStateMachine machine;

        //// Buyer

        // Indexes of valid piecesm, in the order they were downloaded
        // NB: The reason this is not in Seller, is because
        // any peer can potentially provide valid pieces
        std::queue<uint32_t> downloadedValidPieces;

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

        Seller(SellerState state, ConnectionIdType connection)
            : state(state)
            , connection(connection) {
        }

        // State of this seller
        SellerState state;

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
