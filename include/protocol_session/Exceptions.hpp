/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, April 24 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_EXCEPTIONS_HPP
#define JOYSTREAM_PROTOCOLSESSION_EXCEPTIONS_HPP

#include <protocol_session/common.hpp>

#include <stdexcept>
#include <string>
#include <vector>

namespace joystream {
namespace protocol_session {
namespace exception {

class CouldNotCompleteContract : public std::runtime_error {};

template <class ConnectionIdType>
class ConnectionAlreadyAddedException : public std::runtime_error {

public:

    ConnectionAlreadyAddedException(const ConnectionIdType & id)
        : std::runtime_error(std::string("Connection with id ") + IdToString<ConnectionIdType>(id) + std::string(" already added."))
        , _id(id) {
    }

    ConnectionIdType id() const {
        return _id;
    }

private:

    // Id of connection which has already been added
    ConnectionIdType _id;

};

class InvalidPieceIndexException : public std::runtime_error {

public:

    InvalidPieceIndexException(int expectedPieceIndex, int actualPieceIndex)
        : std::runtime_error("Expected piece index ") // + std::string(expectedPieceIndex))
        , _expectedPieceIndex(expectedPieceIndex)
        , _actualPieceIndex(actualPieceIndex) {
    }

    int expectedPieceIndex() const {
        return _expectedPieceIndex;
    }

    int actualPieceIndex() const {
        return _actualPieceIndex;
    }

private:

    int _expectedPieceIndex, _actualPieceIndex;

};

template <class ConnectionIdType>
class ConnectionDoesNotExist : public std::runtime_error {

public:

    ConnectionDoesNotExist(const ConnectionIdType & id)
        : std::runtime_error(std::string("Connection with id ") + IdToString<ConnectionIdType>(id) + std::string(" does not exist."))
         , _id(id) {
    }

    ConnectionIdType id() const {
        return _id;
    }

private:

    ConnectionIdType _id;

};

class StateIncompatibleOperation : public std::runtime_error {

public:

    StateIncompatibleOperation(const std::string & problemDescription)
        : std::runtime_error(std::string("Operation not compatible with current session state: ") + problemDescription) {
    }
};

class SessionModeNotSetException : public std::runtime_error {

public:

    SessionModeNotSetException()
        : std::runtime_error("Mode of session not yet set.") {
    }

};

class SessionAlreadyInThisMode : public std::runtime_error {

public:

    SessionAlreadyInThisMode()
        : std::runtime_error("Session is already in this mode.") {
    }
};

class ModeIncompatibleOperation : public std::runtime_error {

public:

    ModeIncompatibleOperation()
        : std::runtime_error("Operation is incompatible with current session mode.") {
    }
};

class IncorrectPieceIndex : public std::runtime_error {

public:

    IncorrectPieceIndex(int actual, int expected)
        : std::runtime_error(std::string("Provided piece index ") +
                             std::to_string(actual) +
                             std::string(", expected ") +
                             std::to_string(expected))
        , _actual(actual)
        , _expected(expected) {
    }

    int actual() const { return _actual; }
    int expected() const { return _expected; }

private:

    int _actual, _expected;
};

class NoLongerSendingInvitations : public std::runtime_error {

public:

    NoLongerSendingInvitations()
        : std::runtime_error("Session no longer sending invitations.") {
    }

};

template <class ConnectionIdType>
class PeersNotAllReadyToStartDownload : public std::runtime_error {

public:

    PeersNotAllReadyToStartDownload(const PeersNotReadyToStartDownloadingMap<ConnectionIdType> & map)
        : std::runtime_error("Not all peers ready to start download.")
        , peersNotReadyToStartDownloadingMap(map) {}

    PeersNotReadyToStartDownloadingMap<ConnectionIdType> peersNotReadyToStartDownloadingMap;
};

class PeerNotReadyToStartUploading : public std::runtime_error {

public:

    PeerNotReadyToStartUploading(PeerNotReadyToStartUploadingCause cause)
        : std::runtime_error("Peer is not ready to start uploading.")
        , peerNotReadyToStartUploadingCause(cause) {
    }

    PeerNotReadyToStartUploadingCause peerNotReadyToStartUploadingCause;
};

}
}
}

#endif // JOYSTREAM_PROTOCOLSESSION_EXCEPTIONS_HPP
