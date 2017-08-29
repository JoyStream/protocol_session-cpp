/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, February 5 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_DETAIL_PIECE_HPP
#define JOYSTREAM_PROTOCOLSESSION_DETAIL_PIECE_HPP

#include <protocol_session/PieceState.hpp>
#include <string>

namespace joystream {
namespace protocol_session {

class PieceInformation;

namespace detail {

    template <class ConnectionIdType>
    class Piece {

    public:

        Piece();
        Piece(int, PieceState, const ConnectionIdType &, unsigned int);
        Piece(int, const PieceInformation &);

        // Piece is assigned to connection with given id
        void assigned(const ConnectionIdType &);

        // Piece has been downloaded
        void downloaded();

        // Piece is no longer assigned to a connection
        void deAssign();

        // Piece has arrived, and is being stored and validated
        void arrived();

        // Getters
        int index() const;

        PieceState state() const;

        ConnectionIdType connectionId() const;

        unsigned int size() const;

    private:

        // Index of piece
        int _index;

        // Piece state
        PieceState _state;

        // Id of connectionto which piece is assigned, when _state == State::assigned_to_peer_for_download
        ConnectionIdType _connectionId;

        // Byte length of piece (should be the same for all but last piece)
        unsigned int _size;
    };

}
}
}

// Templated type defenitions
#include <protocol_session/detail/Piece.cpp>

#endif // JOYSTREAM_PROTOCOLSESSION_DETAIL_PIECE_HPP
