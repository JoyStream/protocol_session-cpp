/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, February 5 2016
 */


#include <protocol_session/Session.hpp>
#include <protocol_session/Exceptions.hpp>
#include <protocol_session/detail/Buying.hpp>
#include <protocol_session/detail/Selling.hpp>
#include <protocol_session/detail/Observing.hpp>

#include <utility> // std::pair

namespace joystream {
namespace protocol_session {

    /**
    template <class ConnectionIdType>
    int64_t Session<ConnectionIdType>::minimumFundsRequiredAsBuyer(const protocol_wire::BuyerTerms & terms, int numberOfPieces) {

        int64_t maxSingleSellerRevenue = terms.maxPrice() * numberOfPieces;

        // Max total contract output
        int64_t outputPerContractOutput;

        if(maxSingleSellerRevenue > BITCOIN_DUST_LIMIT)
            outputPerContractOutput = maxSingleSellerRevenue;
        else
            outputPerContractOutput = BITCOIN_DUST_LIMIT;

        int64_t maxTotalOutput = outputPerContractOutput * terms.minNumberOfSellers();

        // Contract fee when *there is a change output*
        uint64_t contractTxFeeWithChangeOutput = paymentchannel::ContractTransactionBuilder::fee(terms.minNumberOfSellers(), true, terms.maxContractFeePerKb());

        return maxTotalOutput + contractTxFeeWithChangeOutput;
    }
    */

    template <class ConnectionIdType>
    Session<ConnectionIdType>::Session()
        : _mode(SessionMode::not_set)
        , _state(SessionState::stopped)
        , _observing(nullptr)
        , _selling(nullptr)
        , _buying(nullptr) {

        time(&_started);
    }

    template <class ConnectionIdType>
    Session<ConnectionIdType>::~Session() {

        if(_mode == SessionMode::not_set)
            return;

        if(_state != SessionState::stopped)
            stop();
    }

    template <class ConnectionIdType>
    void Session<ConnectionIdType>::toObserveMode(const RemovedConnectionCallbackHandler<ConnectionIdType> & removedConnection) {

        // Prepare for exiting current state
        switch(_mode) {

            case SessionMode::not_set:

                assert(_observing == nullptr && _buying == nullptr && _selling == nullptr);
                break;

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::SessionAlreadyInThisMode();

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                _buying->leavingState();
                delete _buying;
                _buying = nullptr;
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                _selling->leavingState();
                delete _selling;
                _selling = nullptr;
                break;

            default:
                assert(false);
        }

        // Change mode
        _mode = SessionMode::observing;

        // Create
        _observing = new detail::Observing<ConnectionIdType>(this,
                                                             removedConnection);
    }

    template <class ConnectionIdType>
    void Session<ConnectionIdType>::toSellMode(const RemovedConnectionCallbackHandler<ConnectionIdType> & removedConnection,
                                               const LoadPieceForBuyer<ConnectionIdType> & loadPieceForBuyer,
                                               const ClaimLastPayment<ConnectionIdType> & claimLastPayment,
                                               const AnchorAnnounced<ConnectionIdType> & anchorAnnounced,
                                               const ReceivedValidPayment<ConnectionIdType> & receivedValidPayment,
                                               const protocol_wire::SellerTerms & terms,
                                               int MAX_PIECE_INDEX) {

        // Prepare for exiting current state
        switch(_mode) {

            case SessionMode::not_set:

                assert(_observing == nullptr && _buying == nullptr && _selling == nullptr);
                break;

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                delete _observing;
                _observing = nullptr;
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                _buying->leavingState();
                delete _buying;
                _buying = nullptr;
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                throw exception::SessionAlreadyInThisMode();

            default:
                assert(false);
        }

        // Change mode
        _mode = SessionMode::selling;

        // Create and set selling state
        _selling = new detail::Selling<ConnectionIdType>(this,
                                                         removedConnection,
                                                         loadPieceForBuyer,
                                                         claimLastPayment,
                                                         anchorAnnounced,
                                                         receivedValidPayment,
                                                         terms,
                                                         MAX_PIECE_INDEX);
    }

    template <class ConnectionIdType>
    void Session<ConnectionIdType>::toBuyMode(const RemovedConnectionCallbackHandler<ConnectionIdType> & removedConnection,
                                              const FullPieceArrived<ConnectionIdType> & fullPieceArrived,
                                              const SentPayment<ConnectionIdType> & sentPayment,
                                              const protocol_wire::BuyerTerms & terms,
                                              const TorrentPieceInformation & information) {

        // Prepare for exiting current state
        switch(_mode) {

            case SessionMode::not_set:

                assert(_observing == nullptr && _buying == nullptr && _selling == nullptr);
                break;

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                delete _observing;
                _observing = nullptr;
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                throw exception::SessionAlreadyInThisMode();

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                _selling->leavingState();
                delete _selling;
                _selling = nullptr;
                break;

            default:
                assert(false);
        }

        // Change mode
        _mode = SessionMode::buying;

        // Create and set selling state
        _buying = new detail::Buying<ConnectionIdType>(this,
                                                       removedConnection,
                                                       fullPieceArrived,
                                                       sentPayment,
                                                       terms,
                                                       information);
    }

    template <class ConnectionIdType>
    void Session<ConnectionIdType>::start() {

        if(_state == SessionState::started)
            throw exception::StateIncompatibleOperation("cannot start an already started session.");

        switch(_mode) {

            case SessionMode::not_set:

                assert(_observing == nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::SessionModeNotSetException();

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                _observing->start();
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                _buying->start();
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                _selling->start();
                break;

            default:
                assert(false);
        }
    }

    template <class ConnectionIdType>
    void Session<ConnectionIdType>::stop() {

        if(_state == SessionState::stopped)
            throw exception::StateIncompatibleOperation("cannot stop an already stopped session.");

        switch(_mode) {

            case SessionMode::not_set:

                assert(_observing == nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::SessionModeNotSetException();

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                _observing->stop();
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                _buying->stop();
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                _selling->stop();
                break;

            default:
                assert(false);
        }

    }

    template <class ConnectionIdType>
    void Session<ConnectionIdType>::pause() {

        if(_state == SessionState::paused)
            throw exception::StateIncompatibleOperation("cannot pause and already paused session.");

        switch(_mode) {

            case SessionMode::not_set:

                assert(_observing == nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::SessionModeNotSetException();

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                _observing->pause();
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                _buying->pause();
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                _selling->pause();
                break;

            default:
                assert(false);
        }
    }

    template <class ConnectionIdType>
    void Session<ConnectionIdType>::tick() {

        switch(_mode) {

            case SessionMode::not_set:

                assert(_observing == nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::SessionModeNotSetException();

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                // Nothing to do for observing mode
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                _buying->tick();
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                _selling->tick();
                break;

            default:

                assert(false);
        }
    }

    template <class ConnectionIdType>
    uint Session<ConnectionIdType>::addConnection(const ConnectionIdType & id, const SendMessageOnConnectionCallbacks & callbacks) {

        switch(_mode) {

            case SessionMode::not_set:

                assert(_observing == nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::SessionModeNotSetException();

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                return _observing->addConnection(id, callbacks);

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                return _buying->addConnection(id, callbacks);

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                return _selling->addConnection(id, callbacks);

            default:

                assert(false);
        }
    }

    template<class ConnectionIdType>
    bool Session<ConnectionIdType>::hasConnection(const ConnectionIdType & id) const {

        // drop this
        //if(_mode == SessionMode::not_set)
        //    throw exception::SessionModeNotSetException();

        return _connections.find(id) != _connections.cend();
    }

    template<class ConnectionIdType>
    bool Session<ConnectionIdType>::removeConnection(const ConnectionIdType & id) {

        switch(_mode) {

            case SessionMode::not_set:

                assert(_observing == nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::SessionModeNotSetException();
                break;

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                _observing->removeConnection(id);
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                _buying->removeConnection(id);
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                _selling->removeConnection(id);
                break;

            default:
                assert(false);
        }

        return true;
    }

    template<class ConnectionIdType>
    status::Connection<ConnectionIdType> Session<ConnectionIdType>::connectionStatus(const ConnectionIdType & id) const noexcept {

        if(_mode == SessionMode::not_set)
            throw exception::SessionModeNotSetException();

        // Try to recover conneciton based on id
        auto it = _connections.find(id);

        if(it == _connections.cend())
            throw exception::ConnectionDoesNotExist<ConnectionIdType>(id);

        // Generate status for connection
        return (*it).second->status();
    }

    template<class ConnectionIdType>
    std::set<ConnectionIdType> Session<ConnectionIdType>::connectionIds() const {

        std::set<ConnectionIdType> ids;

        // Add ids of all connections
        for(auto mapping: _connections)
            ids.insert(mapping.first);

        return ids;
    }

    template<class ConnectionIdType>
    template<class M>
    void Session<ConnectionIdType>::processMessageOnConnection(const ConnectionIdType & id, const M &m) {

        if(_mode == SessionMode::not_set)
            throw exception::SessionModeNotSetException();

        // Get connection
        detail::Connection<ConnectionIdType> * c = get(id);

        // and, have it process the message
        c->processMessage(m);

        // ** DO NOT USE c **
        // MAY HAVE BEEN DELETED BY THE TIME
        // STACK TRACE RETRACTS, HENCE WE CANNOT USE IT.
    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::startDownloading(const Coin::Transaction & contractTx, const PeerToStartDownloadInformationMap<ConnectionIdType> & peerToStartDownloadInformationMap) {

        if(_state == SessionState::paused)
            throw exception::StateIncompatibleOperation("cannot start downloading on a paused session.");

        switch(_mode) {

            case SessionMode::not_set:

                assert(_observing == nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::SessionModeNotSetException();

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);

                _buying->startDownloading(contractTx, peerToStartDownloadInformationMap);
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            default:
                assert(false);
        }

    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::validPieceReceivedOnConnection(const ConnectionIdType & id, int index) {

        switch(_mode) {

            case SessionMode::not_set:

                assert(_observing == nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::SessionModeNotSetException();

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                _buying->validPieceReceivedOnConnection(id, index);
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            default:
                assert(false);
        }

    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::invalidPieceReceivedOnConnection(const ConnectionIdType & id, int index) {

        switch(_mode) {

            case SessionMode::not_set:

                throw exception::SessionModeNotSetException();

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                _buying->invalidPieceReceivedOnConnection(id, index);
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            default:
                assert(false);
        }

    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::pieceDownloaded(int index) {

        switch(_mode) {

            case SessionMode::not_set:

                throw exception::SessionModeNotSetException();

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                _buying->pieceDownloaded(index);
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            default:
                assert(false);
        }

    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::updateTerms(const protocol_wire::BuyerTerms & terms) {

        switch(_mode) {

            case SessionMode::not_set:

                throw exception::SessionModeNotSetException();

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                _buying->updateTerms(terms);
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            default:
                assert(false);
        }

    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::startUploading(const ConnectionIdType & id,
                                                   const protocol_wire::BuyerTerms & terms,
                                                   const Coin::KeyPair & contractKeyPair,
                                                   const Coin::PubKeyHash & finalPkHash) {

        switch(_mode) {

            case SessionMode::not_set:

                throw exception::SessionModeNotSetException();

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                _selling->startUploading(id,
                                         terms,
                                         contractKeyPair,
                                         finalPkHash);
                break;

            default:
                assert(false);
        }
    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::pieceLoaded(const ConnectionIdType & id, const protocol_wire::PieceData & data, int index) {

        switch(_mode) {

            case SessionMode::not_set:

                throw exception::SessionModeNotSetException();

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                _selling->pieceLoaded(id, data, index);
                break;

            default:
                assert(false);
        }
    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::updateTerms(const protocol_wire::SellerTerms & terms) {

        switch(_mode) {

            case SessionMode::not_set:

                throw exception::SessionModeNotSetException();

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                throw exception::ModeIncompatibleOperation();
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                _selling->updateTerms(terms);
                break;

            default:
                assert(false);
        }
    }

    template<class ConnectionIdType>
    SessionState Session<ConnectionIdType>::state() const {
        return _state;
    }

    template<class ConnectionIdType>
    SessionMode Session<ConnectionIdType>::mode() const {
        return _mode;
    }

    template<class ConnectionIdType>
    typename status::Session<ConnectionIdType> Session<ConnectionIdType>::status() const noexcept {

        return status::Session<ConnectionIdType>(_mode,
                                                 _state,
                                                 (_mode == SessionMode::selling ? _selling->status() : status::Selling()),
                                                 (_mode == SessionMode::buying ? _buying->status() : status::Buying<ConnectionIdType>()));
    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::peerAnnouncedModeAndTerms(const ConnectionIdType & id, const protocol_statemachine::AnnouncedModeAndTerms & a) {

        assert(hasConnection(id));

        switch(_mode) {

            case SessionMode::not_set:
                throw exception::SessionModeNotSetException();

            case SessionMode::observing:

                assert(_observing != nullptr && _buying == nullptr && _selling == nullptr);
                // Nothing to do
                break;

            case SessionMode::buying:

                assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);
                _buying->peerAnnouncedModeAndTerms(id, a);
                break;

            case SessionMode::selling:

                assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);
                _selling->peerAnnouncedModeAndTerms(id, a);
                break;

        default:
            assert(false);
        }

    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::invitedToOutdatedContract(const ConnectionIdType & id) {

        assert(hasConnection(id));
        assert(_mode == SessionMode::selling);
        assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);

        _selling->invitedToOutdatedContract(id);
    }

    template <class ConnectionIdType>
    void Session<ConnectionIdType>::invitedToJoinContract(const ConnectionIdType & id) {

        assert(hasConnection(id));
        assert(_mode == SessionMode::selling);
        assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);

        _selling->invitedToJoinContract(id);
    }

    template <class ConnectionIdType>
    void Session<ConnectionIdType>::contractPrepared(const ConnectionIdType & id, uint64_t value, const Coin::typesafeOutPoint & anchor, const Coin::PublicKey & payorContractPk, const Coin::PubKeyHash & payorFinalPkHash) {

        assert(hasConnection(id));
        assert(_mode == SessionMode::selling);
        assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);

        _selling->contractPrepared(id, value, anchor, payorContractPk, payorFinalPkHash);
    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::pieceRequested(const ConnectionIdType & id, int index) {

        assert(hasConnection(id));
        assert(_mode == SessionMode::selling);
        assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);

        _selling->pieceRequested(id, index);
    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::invalidPieceRequested(const ConnectionIdType & id) {

        assert(hasConnection(id));
        assert(_mode == SessionMode::selling);
        assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);

        _selling->invalidPieceRequested(id);
    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::paymentInterrupted(const ConnectionIdType & id) {

        assert(hasConnection(id));
        assert(_mode == SessionMode::selling);
        assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);

        _selling->paymentInterrupted(id);
    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::receivedValidPayment(const ConnectionIdType & id, const Coin::Signature & sig) {

        assert(hasConnection(id));
        assert(_mode == SessionMode::selling);
        assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);

        _selling->receivedValidPayment(id, sig);
    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::receivedInvalidPayment(const ConnectionIdType & id, const Coin::Signature & sig) {

        assert(hasConnection(id));
        assert(_mode == SessionMode::selling);
        assert(_observing == nullptr && _buying == nullptr && _selling != nullptr);

        _selling->receivedInvalidPayment(id, sig);
    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::sellerHasJoined(const ConnectionIdType & id) {

        assert(hasConnection(id));
        assert(_mode == SessionMode::buying);
        assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);

        _buying->sellerHasJoined(id);
    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::sellerHasInterruptedContract(const ConnectionIdType & id) {

        assert(hasConnection(id));
        assert(_mode == SessionMode::buying);
        assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);

        _buying->sellerHasInterruptedContract(id);
    }

    template<class ConnectionIdType>
    void Session<ConnectionIdType>::receivedFullPiece(const ConnectionIdType & id, const protocol_wire::PieceData & data) {

        assert(hasConnection(id));
        assert(_mode == SessionMode::buying);
        assert(_observing == nullptr && _buying != nullptr && _selling == nullptr);

        _buying->receivedFullPiece(id, data);
    }

    template<class ConnectionIdType>
    detail::Connection<ConnectionIdType> * Session<ConnectionIdType>::createConnection(const ConnectionIdType & id, const SendMessageOnConnectionCallbacks & sendMessageCallbacks) {

        return new detail::Connection<ConnectionIdType>(
        id,
        [this, id](const protocol_statemachine::AnnouncedModeAndTerms & a) { this->peerAnnouncedModeAndTerms(id, a); },
        [this, id](void) { this->invitedToOutdatedContract(id); },
        [this, id]() { this->invitedToJoinContract(id); },
        sendMessageCallbacks,
        [this, id](uint64_t value, const Coin::typesafeOutPoint & anchor, const Coin::PublicKey & payorContractPk, const Coin::PubKeyHash & payorFinalPkHash) { this->contractPrepared(id, value, anchor, payorContractPk, payorFinalPkHash); },
        [this, id](int i) { this->pieceRequested(id, i); },
        [this, id]() { this->invalidPieceRequested(id); },
        [this, id]() { this->paymentInterrupted(id); },
        [this, id](const Coin::Signature & s) { this->receivedValidPayment(id, s); },
        [this, id](const Coin::Signature & s) { this->receivedInvalidPayment(id, s); },
        [this, id]() { this->sellerHasJoined(id); },
        [this, id]() { this->sellerHasInterruptedContract(id); },
        [this, id](const protocol_wire::PieceData & p) { this->receivedFullPiece(id, p); });
    }

    template <class ConnectionIdType>
    std::vector<detail::Connection<ConnectionIdType> *> Session<ConnectionIdType>::connectionsWithPeerInMode(protocol_statemachine::ModeAnnounced m) {

        std::vector<detail::Connection<ConnectionIdType> *> matches;

        // Copy connection with peer in given mode into matches
        for(auto mapping: _connections) {

            const protocol_statemachine::CBStateMachine & machine = mapping.second->machine();

            if(machine.announcedModeAndTermsFromPeer().modeAnnounced() == m)
                matches.push_back(mapping.first);
        }

        return matches;
    }

    template <class ConnectionIdType>
    template <typename T>
    std::vector<detail::Connection<ConnectionIdType> *> Session<ConnectionIdType>::connectionsInState() const {

        std::vector<detail::Connection<ConnectionIdType> *> matches;

        // Add ids of all connections
        for(auto mapping: _connections) {

           detail::Connection<ConnectionIdType> * c = mapping.second;

            if(c-> template inState<T>())
                matches.push_back(c);
        }

        return matches;
    }

    template <class ConnectionIdType>
    detail::Connection<ConnectionIdType> * Session<ConnectionIdType>::get(const ConnectionIdType & id) const {

        auto itr = _connections.find(id);

        if(itr == _connections.cend())
            throw exception::ConnectionDoesNotExist<ConnectionIdType>(id);
        else
            return itr->second;
    }

    template <class ConnectionIdType>
    typename detail::ConnectionMap<ConnectionIdType>::const_iterator Session<ConnectionIdType>::destroyConnection(const ConnectionIdType & id) {

        // Get iterator pointing at connection in map
        auto itr = _connections.find(id);

        assert(itr != _connections.cend());

        // Delete connection
        delete (itr->second);

        // Delete from map and return iterator at next valid position (e.g. end)
        return _connections.erase(itr);
    }

    template <class ConnectionIdType>
    detail::Connection<ConnectionIdType> * Session<ConnectionIdType>::createAndAddConnection(const ConnectionIdType & id, const SendMessageOnConnectionCallbacks & callbacks) {

        // Do not accept new connection if session is stopped
        if(_state == SessionState::stopped)
            throw exception::StateIncompatibleOperation("cannot create connection while session is stopped.");

        // Check that connection is new, throw exception if not
        if(hasConnection(id))
            throw exception::ConnectionAlreadyAddedException<ConnectionIdType>(id);

        // Create a new connection
        detail::Connection<ConnectionIdType> * connection = createConnection(id, callbacks);

        // Add to map
        _connections.insert(std::make_pair(id, connection));

        return connection;
    }
}
}

