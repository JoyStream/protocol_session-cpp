/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, February 17 2016
 */

#include <protocol_session/detail/Seller.hpp>
#include <protocol_session/detail/Connection.hpp>

namespace joystream {
namespace protocol_session {
namespace detail {

    template <class ConnectionIdType>
    Seller<ConnectionIdType>::Seller() :
        _connection(nullptr),
        _numberOfPiecesAwaitingValidation(0) {
    }

    template <class ConnectionIdType>
    Seller<ConnectionIdType>::Seller(Connection<ConnectionIdType> * connection) :
        _connection(connection),
        _numberOfPiecesAwaitingValidation(0) {
    }

    template <class ConnectionIdType>
    int Seller<ConnectionIdType>::requestPiece(int i) {

        _piecesAwaitingArrival.push(i);

        assert(!isGone());

        // Send request
        _connection->processEvent(protocol_statemachine::event::RequestPiece(i));

        return _piecesAwaitingArrival.size();
    }

    template <class ConnectionIdType>
    int Seller<ConnectionIdType>::fullPieceArrived() {
        // Can't happen if there is no connection
        assert(!isGone());

        // Connection statemachine prevents overflows
        assert(_piecesAwaitingArrival.size() > 0);

        int index = _piecesAwaitingArrival.front();

        _piecesAwaitingArrival.pop();

        _numberOfPiecesAwaitingValidation++;

        return index;
    }

    template <class ConnectionIdType>
    void Seller<ConnectionIdType>::removed() {
        _connection = nullptr;
        _piecesAwaitingArrival = std::queue<int>();
        _numberOfPiecesAwaitingValidation = 0;
    }

    template <class ConnectionIdType>
    void Seller<ConnectionIdType>::pieceWasValid() {
        // After seller is removed it should ignore all piece validation results
        if(isGone()) return;

        if(_numberOfPiecesAwaitingValidation == 0)
          throw std::runtime_error("seller is not expecting piece validation result");

        _numberOfPiecesAwaitingValidation--;

        _connection->processEvent(protocol_statemachine::event::SendPayment());
    }

    template <class ConnectionIdType>
    void Seller<ConnectionIdType>::pieceWasInvalid() {
      // After seller is removed it should ignore all piece validation results
      if(isGone()) return;

      if(_numberOfPiecesAwaitingValidation == 0)
        throw std::runtime_error("seller is not expecting piece validation result");

      _numberOfPiecesAwaitingValidation--;

      // Trigger callback to session and terminate state machine
      // After seller is removed it is no longer responsible to handle validation results
      _connection->processEvent(protocol_statemachine::event::InvalidPieceReceived());
    }

    template <class ConnectionIdType>
    bool Seller<ConnectionIdType>::isPossiblyOwedPayment() const {
        return _piecesAwaitingArrival.size() > 0 || _numberOfPiecesAwaitingValidation > 0;
    }

    template <class ConnectionIdType>
    typename status::Seller<ConnectionIdType> Seller<ConnectionIdType>::status() const {
        return status::Seller<ConnectionIdType>(_connection->connectionId());
    }

    template <class ConnectionIdType>
    Connection<ConnectionIdType> * Seller<ConnectionIdType>::connection() const {
        return _connection;
    }

    template <class ConnectionIdType>
    std::queue<int> Seller<ConnectionIdType>::piecesAwaitingArrival() const {
      return _piecesAwaitingArrival;
    }

    template <class ConnectionIdType>
    int Seller<ConnectionIdType>::numberOfPiecesAwaitingValidation() const {
      return _numberOfPiecesAwaitingValidation;
    }
}
}
}
