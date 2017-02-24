/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, April 29 2016
 */

#include <protocol_session/Session.hpp>
#include <protocol_session/detail/Observing.hpp>
#include <protocol_session/detail/Buying.hpp>
#include <protocol_session/detail/Selling.hpp>

namespace joystream {
namespace protocol_session {
namespace detail {

    template <class ConnectionIdType>
    Observing<ConnectionIdType>::Observing(Session<ConnectionIdType> * session,
                                           const RemovedConnectionCallbackHandler<ConnectionIdType> & removedConnection)
        : _session(session)
        , _removedConnection(removedConnection) {

        // Update terms for all connections
        for(auto itr : _session->_connections)
            (itr.second)->processEvent(joystream::protocol_statemachine::event::ObserveModeStarted());
    }

    template <class ConnectionIdType>
    uint Observing<ConnectionIdType>::addConnection(const ConnectionIdType & id, const SendMessageOnConnectionCallbacks &callbacks) {

        // Create connection
        detail::Connection<ConnectionIdType> * connection = _session->createAndAddConnection(id, callbacks);

        // Choose mode on connection
        connection->processEvent(protocol_statemachine::event::ObserveModeStarted());

        return _session->_connections.size();
    }

    template <class ConnectionIdType>
    void Observing<ConnectionIdType>::removeConnection(const ConnectionIdType & id) {

        if(_session->_state == SessionState::stopped)
            throw exception::StateIncompatibleOperation("cannot remove connection while session is stopped, all connections are removed.");

        if(!_session->hasConnection(id))
            throw exception::ConnectionDoesNotExist<ConnectionIdType>(id);

        _session->destroyConnection(id);

        // Notify client to remove connection
        _removedConnection(id, DisconnectCause::client);
    }

    template <class ConnectionIdType>
    void Observing<ConnectionIdType>::start() {

        // Update state
        _session->_state = SessionState::started;
    }

    template <class ConnectionIdType>
    void Observing<ConnectionIdType>::stop() {

        // We cant stop if we have already stopped
        if(_session->state() == SessionState::stopped)
            throw exception::StateIncompatibleOperation("cannot stop while already stopped.");

        // Disconnect everyone: iteration safe deletion
        for(auto itr = _session->_connections.cbegin(); itr != _session->_connections.cend();) {

            // Notify client to remove connection
            _removedConnection(itr->first, DisconnectCause::client);

            // Destroy connection: iterator made invalid here
            itr = _session->destroyConnection(itr->first);
        }

        // Update state
        _session->_state = SessionState::stopped;
    }

    template <class ConnectionIdType>
    void Observing<ConnectionIdType>::pause() {

        // Update state
        _session->_state = SessionState::paused;
    }

}
}
}
