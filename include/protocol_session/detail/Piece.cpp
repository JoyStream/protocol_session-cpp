/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, February 5 2016
 */

#include <protocol_session/detail/Piece.hpp>
#include <protocol_session/PieceInformation.hpp>

namespace joystream {
namespace protocol_session {
namespace detail {

    template <class ConnectionIdType>
    Piece<ConnectionIdType>::Piece()
        : Piece(0, PieceState::unassigned, ConnectionIdType(), 0) {
    }

    template <class ConnectionIdType>
    Piece<ConnectionIdType>::Piece(int index, PieceState state, const ConnectionIdType & id, unsigned int size)
        : _index(index)
        , _state(state)
        , _connectionId(id)
        , _size(size) {
    }

    template <class ConnectionIdType>
    Piece<ConnectionIdType>::Piece(int index, const PieceInformation & p)
        : _index(index)
        , _state(p.downloaded() ? PieceState::downloaded : PieceState::unassigned)
        , _size(p.size()) {
    }

    template <class ConnectionIdType>
    void Piece<ConnectionIdType>::assigned(const ConnectionIdType & id) {

        assert(_state == PieceState::unassigned);

        _state = PieceState::being_downloaded;
        _connectionId = id;
    }

    template <class ConnectionIdType>
    void Piece<ConnectionIdType>::deAssign() {
        _state = PieceState::unassigned;
        _connectionId = ConnectionIdType();
    }

    template <class ConnectionIdType>
    void Piece<ConnectionIdType>::downloaded() {
        _state = PieceState::downloaded;
        _connectionId = ConnectionIdType();
    }

    template <class ConnectionIdType>
    int Piece<ConnectionIdType>::index() const {
        return _index;
    }

    template <class ConnectionIdType>
    void Piece<ConnectionIdType>::arrived() {
        _state = PieceState::being_validated_and_stored;
    }

    template <class ConnectionIdType>
    PieceState Piece<ConnectionIdType>::state() const {
        return _state;
    }

    template <class ConnectionIdType>
    ConnectionIdType Piece<ConnectionIdType>::connectionId() const {
        return _connectionId;
    }

    template <class ConnectionIdType>
    unsigned int Piece<ConnectionIdType>::size() const {
        return _size;
    }
}
}
}
