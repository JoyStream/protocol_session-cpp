/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, April 29 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_OBSERVING_HPP
#define JOYSTREAM_PROTOCOLSESSION_OBSERVING_HPP

#include <protocol_session/Session.hpp>

namespace joystream {
namespace protocol_session {
namespace detail {

template <class ConnectionIdType>
class Selling;

template <class ConnectionIdType>
class Buying;

template <class ConnectionIdType>
class Observing {

public:

    Observing(Session<ConnectionIdType> *,
              const RemovedConnectionCallbackHandler<ConnectionIdType> &);

    //// Connection level client events

    // Adds connection, and return the current number of connections
    uint addConnection(const ConnectionIdType &, const SendMessageOnConnectionCallbacks &);

    // Remove connection
    void removeConnection(const ConnectionIdType &);

    //// Change state

    // Starts a stopped session by becoming fully operational
    void start();

    // Immediately closes all existing connections
    void stop();

    // Pause session
    void pause();

private:

    //// Members

    // Reference to core of session
    Session<ConnectionIdType> * _session;

    // Callback handlers
    RemovedConnectionCallbackHandler<ConnectionIdType> _removedConnection;

};

}
}
}

// Templated type defenitions
#include <protocol_session/detail/Observing.cpp>
#endif // JOYSTREAM_PROTOCOLSESSION_OBSERVING_HPP
