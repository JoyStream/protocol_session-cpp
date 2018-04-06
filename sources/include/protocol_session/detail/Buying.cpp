/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, April 9 2016
 */

#include <protocol_session/common.hpp>
#include <protocol_session/Session.hpp>
#include <protocol_session/TorrentPieceInformation.hpp>
#include <protocol_session/Exceptions.hpp>
#include <protocol_session/detail/Buying.hpp>
#include <protocol_session/detail/Selling.hpp>
#include <protocol_session/detail/Observing.hpp>
#include <common/Bitcoin.hpp> // BITCOIN_DUST_LIMIT
#include <common/P2SHAddress.hpp>

#include <numeric>

namespace joystream {
namespace protocol_session {
namespace detail {

    template <class ConnectionIdType>
    Buying<ConnectionIdType>::Buying(Session<ConnectionIdType> * session,
                                     const RemovedConnectionCallbackHandler<ConnectionIdType> & removedConnection,
                                     const FullPieceArrived<ConnectionIdType> & fullPieceArrived,
                                     const SentPayment<ConnectionIdType> & sentPayment,
                                     const protocol_wire::BuyerTerms & terms,
                                     const TorrentPieceInformation & information,
                                     const AllSellersGone & allSellersGone,
                                     std::chrono::duration<double> maxTimeToServicePiece)
        : _session(session)
        , _removedConnection(removedConnection)
        , _fullPieceArrived(fullPieceArrived)
        , _sentPayment(sentPayment)
        , _state(BuyingState::sending_invitations)
        , _terms(terms)
        , _numberOfMissingPieces(0)
        , _allSellersGone(allSellersGone)
        , _maxConcurrentRequests(4)
        , _maxTimeToServicePiece(maxTimeToServicePiece)
        , _speedTestPolicyEnabled(false)
        , _speedTestPolicyPayloadSize(500000) {
        //, _lastStartOfSendingInvitations(0) {

        // Setup pieces
        for(uint i = 0;i < information.size();i++) {

            PieceInformation p = information[i];

            _pieces.push_back(detail::Piece<ConnectionIdType>(i, p));

            if(!p.downloaded())
                _numberOfMissingPieces++;
        }

        // Notify any existing peers
        for(auto i : _session->_connections)
            (i.second)->processEvent(protocol_statemachine::event::BuyModeStarted(_terms));

        // If session is started, then set start time of this new mode
        if(_session->_state == SessionState::started)
            _lastStartOfSendingInvitations = std::chrono::high_resolution_clock::now();
    }

    template <class ConnectionIdType>
    uint Buying<ConnectionIdType>::addConnection(const ConnectionIdType & id, const SendMessageOnConnectionCallbacks & callbacks) {

        // Create connection
        detail::Connection<ConnectionIdType> * connection = _session->createAndAddConnection(id, callbacks);

        // Choose mode on connection
        connection->processEvent(protocol_statemachine::event::BuyModeStarted(_terms));

        return _session->_connections.size();
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::removeConnection(const ConnectionIdType & id) {

        // We explicitly check for stopped session, although checking for spesific connection existance
        // implicitly covers this case. It improves feedback to client.
        if(_session->_state == SessionState::stopped)
            throw exception::StateIncompatibleOperation("cannot remove connection while stopped, all connections are removed");

        if(!_session->hasConnection(id))
            throw exception::ConnectionDoesNotExist<ConnectionIdType>(id);

        removeConnection(id, DisconnectCause::client);
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::validPieceReceivedOnConnection(detail::Seller<ConnectionIdType> &seller, int index) {
        // Cannot happen when stopped, as there are no connections
        assert(_session->_state != SessionState::stopped);
        assert(_state == BuyingState::downloading);
        assert(index >= 0);
        assert(!seller.isGone());

        seller.pieceWasValid();

        auto connection = seller.connection();

        const paymentchannel::Payor & payor = connection->payor();

        _sentPayment(connection->connectionId(), payor.price(), payor.numberOfPaymentsMade(), payor.amountPaid(), index);

        tryToAssignAndRequestPieces(seller);
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::invalidPieceReceivedOnConnection(detail::Seller<ConnectionIdType> & seller, int index) {
        // Cannot happen when stopped, as there are no connections
        assert(_session->_state != SessionState::stopped);
        assert(_state == BuyingState::downloading);
        assert(index >= 0);
        assert(!seller.isGone());

        seller.pieceWasInvalid();

        removeConnection(seller.connection()->connectionId(), DisconnectCause::seller_sent_invalid_piece);
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::peerAnnouncedModeAndTerms(const ConnectionIdType & id, const protocol_statemachine::AnnouncedModeAndTerms & a) {

        assert(_session->_state != SessionState::stopped);
        assert(_session->hasConnection(id));

        detail::Connection<ConnectionIdType> * c = _session->get(id);

        //assert(c->announcedModeAndTermsFromPeer() == a);

        // If we are currently started and sending out invitations, then we may (re)invite
        // sellers with sufficiently good terms
        if(_session->_state == SessionState::started &&
           _state == BuyingState::sending_invitations) {

            // Check that this peer is seller,
            protocol_statemachine::ModeAnnounced m = a.modeAnnounced();

            assert(m != protocol_statemachine::ModeAnnounced::none);

            maybeInviteSeller(c, a);
        }
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::sellerHasJoined(const ConnectionIdType &) {

        // Cannot happen when stopped, as there are no connections
        assert(_session->_state != SessionState::stopped);
        //assert(_session->hasConnection(id));
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::sellerHasInterruptedContract(const ConnectionIdType & id) {

        // Cannot happen when stopped, as there are no connections
        assert(_session->_state != SessionState::stopped);
        assert(_session->hasConnection(id));

        // Remove connection
        removeConnection(id, DisconnectCause::seller_has_interrupted_contract);

        // Notify state machine about deletion
        throw protocol_statemachine::exception::StateMachineDeletedException();
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::receivedFullPiece(const ConnectionIdType & id, const protocol_wire::PieceData & p) {

        // Cannot happen when stopped, as there are no connections
        assert(_session->_state != SessionState::stopped);
        assert(_state == BuyingState::downloading);

        /**
         * Invariant _state == BuyingState::downloading
         * Is upheld by the fact that whenever a piece is assigned
         * a new piece, any peer which may have previosuyl have had it
         * assigned will ahve been removed, e.g. due to time out.
         */

        // Get seller corresponding to given id
        auto itr = _sellers.find(id);
        assert(itr != _sellers.end());

        detail::Seller<ConnectionIdType> & s = itr->second;

        // Update state and get expected piece index
        int index = s.fullPieceArrived();

        detail::Piece<ConnectionIdType> & piece = _pieces[index];

        piece.arrived();

        // Notify client - client should immediatly validate the piece and return result of validation
        bool wasValid = _fullPieceArrived(id, p, index);

        if (wasValid) {
          validPieceReceivedOnConnection(s, index);
        } else {
          invalidPieceReceivedOnConnection(s, index);
        }
    }

    template<class ConnectionIdType>
    void Buying<ConnectionIdType>::remoteMessageOverflow(const ConnectionIdType & id) {
      std::clog << "Error: remoteMessageOverflow from seller connection " << id << std::endl;
      removeConnection(id, DisconnectCause::seller_message_overflow);
    }

    template<class ConnectionIdType>
    void Buying<ConnectionIdType>::sellerCompletedSpeedTest(const ConnectionIdType & id, bool successful) {

      detail::Connection<ConnectionIdType> * c = _session->get(id);

      if (!successful) {
        // Remove connection
        removeConnection(id, DisconnectCause::seller_failed_speed_test);

        // Notify state machine about deletion
        throw protocol_statemachine::exception::StateMachineDeletedException();

      } else {
        // record completion time
        c->completedSpeedTest();
      }

    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::leavingState() {

        // Prepare sellers before we interrupt with new mode
        politeSellerCompensation();
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::start() {

        assert(_session->_state != SessionState::started);

        // Note starting time
        _lastStartOfSendingInvitations = std::chrono::high_resolution_clock::now();

        // Set client mode to started
        _session->_state = SessionState::started;
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::stop() {

        assert(_session->_state != SessionState::stopped);

        // Prepare sellers for closing connections
        politeSellerCompensation();

        // Clear sellers
        _sellers.clear();

        // Disconnect everyone:
        for(auto itr = _session->_connections.cbegin();itr != _session->_connections.cend();)
            itr = removeConnection(itr->first, DisconnectCause::client);

        // Update core session state
        _session->_state = SessionState::stopped;

        // Reset to sending_invitations state if download did not complete
        if (_state == BuyingState::downloading) {
            _state = BuyingState::sending_invitations;
        }
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::pause() {

        // We can only pause if presently started
        if(_session->_state == SessionState::paused ||
           _session->_state == SessionState::stopped)
            throw exception::StateIncompatibleOperation("cannot pause while already paused/stopped.");

        // Update state
        _session->_state = SessionState::paused;
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::tick() {

        // Only process if we are active
        if(_session->_state == SessionState::started) {

            // Disconnect timed out sellers
            // Allocate pieces if we are downloading
            // Reset state to allow restarting downloading after all sellers are gone
            if(_state == BuyingState::downloading) {

                for(auto mapping : _sellers) {

                    // Reference to seller
                    detail::Seller<ConnectionIdType> & s = mapping.second;

                    if (s.isGone()) continue;

                    // Disconnect if seller timed-out servicing request
                    if (s.servicingPieceHasTimedOut(_maxTimeToServicePiece)) {
                      removeConnection(s.connection()->connectionId(), DisconnectCause::seller_servicing_piece_has_timed_out);
                      continue;
                    }

                    // A seller may be waiting to be assigned a new piece
                    if(s.piecesAwaitingArrival().size() == 0) {

                        // This can happen when a seller has previously uploaded a valid piece,
                        // but there were no unassigned pieces at that time,
                        // however they become unassigned later as result of:
                        // * time out of old seller
                        // * seller interrupts contract by updating terms
                        // * seller sent an invalid piece

                        tryToAssignAndRequestPieces(s);
                    }
                }

                // If all sellers are gone, reset state
                resetIfAllSellersGone();
            }
        }
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::pieceDownloaded(int index) {

        assert(index >= 0);

        // We cannot apriori assert anything about piece state.
        detail::Piece<ConnectionIdType> & piece = _pieces[index];

        // If its not already, then mark piece as downloaded and
        // count towards missing piece count.
        // NB: There may be a seller currently sending us this piece,
        // or it may be in validation/storage, or even downloaded before.

        if(piece.state() != PieceState::downloaded) {

            _numberOfMissingPieces--;

            // This may be the last piece
            if(_numberOfMissingPieces == 0)
                _state = BuyingState::download_completed;
        }

        piece.downloaded();
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::updateTerms(const protocol_wire::BuyerTerms & terms) {

        // If we change terms when we are actually downloading,
        // then politely compensate sellers
        if(_state == BuyingState::downloading)
            politeSellerCompensation();

        // Notify existing peers
        for(auto itr : _session->_connections)
            itr.second->processEvent(protocol_statemachine::event::UpdateTerms<protocol_wire::BuyerTerms>(terms));

        // Set new terms
        _terms = terms;

        // If the download was not yet completed
        if(_state != BuyingState::download_completed) {

            // start over sending invitations
            _state = BuyingState::sending_invitations;

            // ditch any existing sellers
            _sellers.clear();

            sendInvitations();
        }
    }

    template <class ConnectionIdType>
    typename status::Buying<ConnectionIdType> Buying<ConnectionIdType>::status() const {

        // Generate statuses of all pieces
        std::vector<status::Piece<ConnectionIdType>> pieceStatuses;

        // SKIPPING DUE TO SPEED
        //for(auto piece : _pieces)
        //    pieceStatuses.push_back(piece.status());

        // Generate statuses of all sellers
        std::map<ConnectionIdType, status::Seller<ConnectionIdType>> sellerStatuses;

        for(auto mapping : _sellers) {
            // skip sellers that are no longer around
            if(mapping.second.isGone())
                continue;

            sellerStatuses.insert(std::make_pair(mapping.first, mapping.second.status()));
        }

        return status::Buying<ConnectionIdType>(_state,
                                                _terms,
                                                sellerStatuses,
                                                //_contractTx,
                                                pieceStatuses);
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::startDownloading(const Coin::Transaction & contractTx,
                                                    const PeerToStartDownloadInformationMap<ConnectionIdType> & peerToStartDownloadInformationMap) {

        std::clog << "Trying to start downloading." << std::endl;

        if(_state != BuyingState::sending_invitations)
            throw exception::NoLongerSendingInvitations();

        assert(_sellers.empty());
        //assert(!_contractTx.isiniliezd());

        /// Determine if announce contract

        PeersNotReadyToStartDownloadingMap<ConnectionIdType> peersNotReadyToStartDownloadingMap;

        for(auto m : peerToStartDownloadInformationMap) {

           auto id = m.first;

           auto it = _session->_connections.find(id);

           if(it == _session->_connections.cend()) {

               std::clog << IdToString(id) << " gone." << std::endl;

               peersNotReadyToStartDownloadingMap.insert(std::make_pair(id, PeerNotReadyToStartDownloadCause::connection_gone));

           } else if(!((it->second) -> template inState<protocol_statemachine::PreparingContract>())) {

               std::clog << IdToString(id) << " no longer in `PreparingContract` state." << std::endl;

               peersNotReadyToStartDownloadingMap.insert(std::make_pair(id, PeerNotReadyToStartDownloadCause::connection_not_in_preparing_contract_state));

           } else {

                // Check if terms are up to date
                protocol_statemachine::AnnouncedModeAndTerms a = it->second->announcedModeAndTermsFromPeer();

                // connectionsInState<protocol_statemachine::PreparingContract> =>
                assert(a.modeAnnounced() == protocol_statemachine::ModeAnnounced::sell);

                if(a.sellModeTerms() != m.second.sellerTerms)  {

                    std::clog << IdToString(id) << " terms expired." << std::endl;

                    peersNotReadyToStartDownloadingMap.insert(std::make_pair(id, PeerNotReadyToStartDownloadCause::terms_expired));

                } else {

                    std::clog << IdToString(id) << " ready." << std::endl;
                }

            }
        }

        if(!peersNotReadyToStartDownloadingMap.empty()) {

            std::clog << "Some peer(s) in bad state, contract could not be announced, prevents starting download. " << std::endl;

            throw exception::PeersNotAllReadyToStartDownload<ConnectionIdType>(peersNotReadyToStartDownloadingMap);
        }

        // store contruct?
        //_contractTx = contractTx;
        Coin::TransactionId txId(Coin::TransactionId::fromTx(contractTx));

        // Update state of sessions,
        // has to be done before starting to assign pieces to sellers
        _state = BuyingState::downloading;

        /// Try to announce to each prospective seller
        for(auto m : peerToStartDownloadInformationMap) {

            auto id = m.first;

            auto it = _session->_connections.find(id);

            // test above =>
            assert(it != _session->_connections.cend());

            auto c = it->second;

            // Create sellers
            _sellers[id] = detail::Seller<ConnectionIdType>(c);

            // Send message to peer
            StartDownloadConnectionInformation inf = m.second;

            c->processEvent(protocol_statemachine::event::ContractPrepared(Coin::typesafeOutPoint(txId, inf.index),
                                                                           inf.buyerContractKeyPair,
                                                                           inf.buyerFinalPkHash,
                                                                           inf.value));

            // Assign the first piece to this peer
            tryToAssignAndRequestPieces(_sellers[id]);
        }

        /////////////////////////

        std::cout << "Started downloading." << std::endl;
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::sendInvitations() const {

      assert(_session->_state == SessionState::started);
      assert(_state == BuyingState::sending_invitations);

      for(auto mapping : _session->_connections) {

          detail::Connection<ConnectionIdType> * c = mapping.second;

          // Check that this peer is seller,
          protocol_statemachine::AnnouncedModeAndTerms a = c->announcedModeAndTermsFromPeer();

          maybeInviteSeller(c, a);
      }
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::maybeInviteSeller(detail::Connection<ConnectionIdType> * c,
      protocol_statemachine::AnnouncedModeAndTerms a) const {

        assert(_session->_state == SessionState::started);
        assert(_state == BuyingState::sending_invitations);

        // Do not send invitations if peer is not announcing sell mode or has incompatible terms
        if(a.modeAnnounced() != protocol_statemachine::ModeAnnounced::sell || !_terms.satisfiedBy(a.sellModeTerms())) {
          return;
        }

        // Does seller need to complete a speed test to be invited
        if (_speedTestPolicyEnabled && !c->performedSpeedTest()) {
            c->startingSpeedTest(); // record starting time
            c->processEvent(protocol_statemachine::event::TestSellerSpeed(_speedTestPolicyPayloadSize));
            return;
        }

        // Seller has previously completed a speed test/or no speed test was required.. invite them
        c->processEvent(protocol_statemachine::event::InviteSeller());
        std::cout << "Invited: " << IdToString(c->connectionId()) << std::endl;
    }

    template <class ConnectionIdType>
    int Buying<ConnectionIdType>::tryToAssignAndRequestPieces(detail::Seller<ConnectionIdType> & s) {

        assert(_session->_state == SessionState::started);
        assert(_state == BuyingState::downloading);
        assert(!s.isGone());

        int totalNewRequests = 0;
        int concurrentRequests = s.piecesAwaitingArrival().size();

        while(concurrentRequests < _maxConcurrentRequests) {

          // Try to find index of next unassigned piece
          int pieceIndex;

          try {
              pieceIndex = this->_pickNextPieceMethod(&_pieces);
          } catch(const std::runtime_error & e) {
              // No unassigned piece was found
              break;
          }

          // Assign piece to seller
          _pieces[pieceIndex].assigned(s.connection()->connectionId());

          // Request piece from seller
          concurrentRequests = s.requestPiece(pieceIndex);

          totalNewRequests++;
        }

        return totalNewRequests;
    }

    template<class ConnectionIdType>
    typename detail::ConnectionMap<ConnectionIdType>::const_iterator Buying<ConnectionIdType>::removeConnection(const ConnectionIdType & id, DisconnectCause cause) {

        assert(_session->state() != SessionState::stopped);
        assert(_session->hasConnection(id));

        // If this is the connection of a seller,
        // we have to deal with that.
        auto itr = _sellers.find(id);

        if(itr != _sellers.cend()) {

            detail::Seller<ConnectionIdType> & s = itr->second;

            // It is possible to find a seller in the "gone" state that we have previoulsy removed.
            // This happens when the connection times out and is later re-establish. The connection will have the
            // same connection id (because it is the same peer)
            if(!s.isGone()) {
              // Remove
              removeSeller(s);
            }
        }

        // Destroy connection - important todo before notifying client
        auto it = _session->destroyConnection(id);

        // Notify client to remove connection
        _removedConnection(id, cause);

        return it;
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::removeSeller(detail::Seller<ConnectionIdType> & s) {
        if (s.isGone()) return;

        // we must be downloading or just finish downloading
        assert(_state == BuyingState::downloading || _state == BuyingState::download_completed);

        // If this seller has assigned piecees, then we must unassign them
        for(uint i = 0;i < _pieces.size();i++) {
            detail::Piece<ConnectionIdType> & piece = _pieces[i];

            if (piece.connectionId() != s.connection()->connectionId()) continue;

            // Deassign the piece
            piece.deAssign();
        }

        // Mark as seller as gone, but is not removed from _sellers map
        s.removed();
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::resetIfAllSellersGone () {

        assert(_state == BuyingState::downloading);
        assert(_session->_state == SessionState::started);

        // Find any seller that is not in gone state
        auto seller = find_if(_sellers.begin(), _sellers.end(), [] ( std::pair<ConnectionIdType, detail::Seller<ConnectionIdType>> mapping) {
          return !mapping.second.isGone();
        });

        // At least one seller is still connected
        if(seller != _sellers.cend()) return;

        std::cout << "All Sellers Are Gone" << std::endl;

        // Notify client
        _allSellersGone();

        // Transition to sending invitations state
        _state = BuyingState::sending_invitations;

        // Clear sellers
        _sellers.clear();

        // Send invitations to all connections
        sendInvitations();
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::politeSellerCompensation() {

        // Pay any seller we have requested a piece from which
        // has not been shown to be invalid, or even arrived.
        // NB: paying for only requested piece can lead to our payment
        // being dropped by peer state machine if it has not yet sent
        // the piece, but its worth trying.
        for(auto itr : _sellers) {

            detail::Seller<ConnectionIdType> & s = itr.second;

            if(s.isPossiblyOwedPayment()) {
              while(s.piecesAwaitingArrival().size() > 0) {
                s.fullPieceArrived();
              }

              while(s.numberOfPiecesAwaitingValidation() > 0) {
                s.pieceWasValid();
              }
            }
        }
    }

    template <class ConnectionIdType>
    protocol_wire::BuyerTerms Buying<ConnectionIdType>::terms() const {
        return _terms;
    }

    template <class ConnectionIdType>
    void Buying<ConnectionIdType>::setPickNextPieceMethod(const PickNextPieceMethod<ConnectionIdType> & pickNextPieceMethod) {
      _pickNextPieceMethod = pickNextPieceMethod;
    }

}
}
}
