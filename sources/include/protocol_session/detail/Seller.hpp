/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, February 17 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_SELLER_HPP
#define JOYSTREAM_PROTOCOLSESSION_SELLER_HPP

#include <protocol_session/SellerState.hpp>

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

        Seller(SellerState , Connection<ConnectionIdType> * , uint32_t);

        // When seller is not assigned a piece, this routine
        // can be used to assing piece to seller and send request to peer
        void requestPiece(int i);

        // Whether
        bool servicingPieceHasTimedOut(const std::chrono::duration<double> &) const;

        // Update state to reflect that a recently arrived full piece from this
        // seller is being processed (verified and
        void fullPieceArrived();

        // Seller has been removed
        void removed();

        // Result of validating piece received from this seller
        bool pieceWasValid();

        // Result of validating piece received from this seller
        void pieceWasInvalid();

        // Whether a piece was recently received
        bool isPossiblyOwedPayment() const;

        // Status of seller
        status::Seller<ConnectionIdType> status() const;

        // Getters
        SellerState state() const;

        Connection<ConnectionIdType> * connection() const;

        int indexOfAssignedPiece() const;

    private:

        // State of this seller
        SellerState _state;

        // Connection identifier for seller
        Connection<ConnectionIdType> * _connection;

        // When _state == State::waiting_for_full_piece,
        // waiting_for_piece_validation_and_storage
        int _indexOfAssignedPiece;

        // When last piece was assigned to this seller.
        // Is used to identify slow seller.
        std::chrono::high_resolution_clock::time_point _whenLastPieceAssigned;
    };

}
}
}

// Templated type defenitions
#include <protocol_session/detail/Seller.cpp>

#endif // JOYSTREAM_PROTOCOLSESSION_SELLER_HPP
