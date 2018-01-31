/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, February 17 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_SELLER_HPP
#define JOYSTREAM_PROTOCOLSESSION_SELLER_HPP

#include <string>
#include <cstdlib>

namespace joystream {
namespace protocol_session {
namespace status {
    template <class ConnectionIdType>
    struct Seller;
}
namespace detail {

    template <class ConnectionIdType>
    class Connection;

    template <class ConnectionIdType>
    class Seller {

    public:

        Seller();

        Seller(Connection<ConnectionIdType> *);

        // Used to request a piece for from the peer, returns total number of pieces awaiting arrival
        // Returned value helps caller to determine wether to make additional requests
        int requestPiece(int i);

        std::queue<int> piecesAwaitingArrival() const;

        int numberOfPiecesAwaitingValidation() const;

        // Update state to reflect that a recently arrived full piece from this peer is being verified
        // We expect the pieces to arrive in same order they were requested. Returns the expected index of the piece
        // which arrived
        int fullPieceArrived();

        // Seller has been removed
        void removed();

        // Result of validating piece received from this seller
        void pieceWasValid();

        // Result of validating piece received from this seller
        void pieceWasInvalid();

        // Returns true ff there are any pieces pending arrival or waiting to be validated
        bool isPossiblyOwedPayment() const;

        // Status of seller
        status::Seller<ConnectionIdType> status() const;

        Connection<ConnectionIdType> * connection() const;

        bool isGone() const  { return _connection == nullptr; }

        bool servicingPieceHasTimedOut(const std::chrono::duration<double> &) const;

    private:

        // Connection identifier for seller
        Connection<ConnectionIdType> * _connection;

        // Pieces we are expecting from peer in order they were requested
        std::queue<int> _piecesAwaitingArrival;

        int _numberOfPiecesAwaitingValidation;

        // The earliest time the piece at the front of the queue is expected to arrive
        // This is effectively the time the first piece request is sent, and updated on arrival of a piece
        // This is used to determine if servicing the next piece has timed out.
        std::chrono::high_resolution_clock::time_point _frontPieceEarliestExpectedArrival;
    };

}
}
}

// Templated type defenitions
#include <protocol_session/detail/Seller.cpp>

#endif // JOYSTREAM_PROTOCOLSESSION_SELLER_HPP
