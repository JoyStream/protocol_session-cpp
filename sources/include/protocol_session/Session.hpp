/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, February 9 2016
 */

#ifndef JOYSTREAM_PROTOCOLSESSION_SESSION_HPP
#define JOYSTREAM_PROTOCOLSESSION_SESSION_HPP

#include <protocol_session/detail/Connection.hpp>
#include <protocol_session/detail/Piece.hpp>
#include <protocol_session/Callbacks.hpp>
#include <protocol_session/SessionMode.hpp>
#include <protocol_session/SessionState.hpp>

#include <unordered_map>

// ConnectionIdType: Type for identifying connections.
// 1) must be possible to use as key in std::map
// 2) must have std::string toString(const & ConnectionIdType);

namespace joystream {
namespace protocol_wire {
    class Message;
}
namespace protocol_session {
namespace detail {

    template <class ConnectionIdType>
    class Selling;

    template <class ConnectionIdType>
    class Buying;

    template <class ConnectionIdType>
    class Observing;
}

    template <class ConnectionIdType>
    using PickNextPieceMethod = std::function<int(const std::vector<detail::Piece<ConnectionIdType>>*)>;

    class TorrentPieceInformation;

    template <class ConnectionIdType>
    class Session {

    public:

        /**
         * @brief Minimum funds required as buyer
         * @param terms Terms that buyer side will have
         * @param numberOfPieces Number of pieces to buy
         * @return minimum Funds (#satoshies) required for buyer, with given terms, to fund contract

        static int64_t minimumFundsRequiredAsBuyer(const protocol_wire::BuyerTerms & terms, int numberOfPieces);
         */

        Session();

        ~Session();

        // Delete to prevent copying of session
        Session(const Session&) = delete;
        void operator=(const Session&) = delete;

        //// Manage mode

        // Can only be called when mode is not set, or is in
        // a set mode different to the mode of the call.

        // Change session to observe mode
        void toObserveMode(const RemovedConnectionCallbackHandler<ConnectionIdType> &);

        // Change session to sell mode
        void toSellMode(const RemovedConnectionCallbackHandler<ConnectionIdType> &,
                        const LoadPieceForBuyer<ConnectionIdType> &,
                        const ClaimLastPayment<ConnectionIdType> &,
                        const AnchorAnnounced<ConnectionIdType> &,
                        const ReceivedValidPayment<ConnectionIdType> & receivedValidPayment,
                        const protocol_wire::SellerTerms &,
                        int);

        // Change session to buy mode
        void toBuyMode(const RemovedConnectionCallbackHandler<ConnectionIdType> &,
                       const FullPieceArrived<ConnectionIdType> &,
                       const SentPayment<ConnectionIdType> &,
                       const protocol_wire::BuyerTerms &,
                       const TorrentPieceInformation &,
                       const AllSellersGone &);

        /**
         * Warning: Do not call any of these operations
         * from the callbacks registered for each mode, or
         * when adding a connection.
         */

        //// Manage state

        // Can only be called when state has been set.

        void start();

        void stop();

        void pause();

        //// Common

        // Time out processing hook
        // NB: Later give some indication of how to set timescale for this call
        void tick();

        // Adds connection, and return the current number of connections
        uint addConnection(const ConnectionIdType &, const SendMessageOnConnectionCallbacks &);

        // Whether there is a connection with given id
        bool hasConnection(const ConnectionIdType &) const;

        // Remove connection if one exists with given id, otherwise returns false.
        bool removeConnection(const ConnectionIdType &);

        void disconnectSlowSellers(const std::chrono::duration<double> & limit);

        // *** TEMPORARY FIX ***

        /**
         * @brief Returns status of connection with given id
         * @return status of connection
         * @throws exception::SessionModeNotSetException if mode is not set
         * @throws exception::ConnectionDoesNotExist<ConnectionIdType> if connection does not exist which corresponds to @a id
         */
        status::Connection<ConnectionIdType> connectionStatus(const ConnectionIdType & id) const noexcept;

        // Get vector of all connection ids
        std::set<ConnectionIdType> connectionIds() const;

        // Process given message on given connection with given ID
        template<class M>
        void processMessageOnConnection(const ConnectionIdType &, const M &);

        //// Buying

        /**
         * @brief Start downloading from sellers.
         *
         * ...
         *
         * @param contractTx contract transaction
         * @param peerToStartDownloadInformationMap ...
         * @return void
         * @throws exception::StateIncompatibleOperation if @c\ state() != SessionState::started \@c
         * @throws exception::SessionModeNotSetException if @c\ mode() != SessionMode::buying \@c
         * @throws exception::NoLongerSendingInvitations if invitations are no longer being sent in buying mode (_state != BuyingState::sending_invitations)
         */
        void startDownloading(const Coin::Transaction & contractTx,
                              const PeerToStartDownloadInformationMap<ConnectionIdType> & peerToStartDownloadInformationMap,
                              const PickNextPieceMethod<ConnectionIdType> & pickNextPieceMethod);

        // Piece with given index has been downloaded, but not through
        // a regitered connection. Could be non-joystream peers, or something out of bounds.
        void pieceDownloaded(int);

        // Update terms
        void updateTerms(const protocol_wire::BuyerTerms &);

        //// Selling

        /**
         * @brief Start uploading.
         *
         * ...
         *
         * @param id identifier for connection to start uploading on
         * @param terms terms that buyer offered, which must be confirmed to still hold
         * @param contractKeyPair payment channel seller side commitment pair
         * @param finalPkHash payment channel seller side settlement pubkeyhash
         * @return void
         * @throws exception::SessionModeNotSetException if ...
         * @throws exception::ModeIncompatibleOperation if ...
         * @throws exception::StateIncompatibleOperation if ...
         * @throws exception::PeerNotReadyToStartUploading if ...
         */
        void startUploading(const ConnectionIdType & id,
                            const protocol_wire::BuyerTerms & terms,
                            const Coin::KeyPair & contractKeyPair,
                            const Coin::PubKeyHash & finalPkHash);

        // Data for given piece has been loaded
        void pieceLoaded(const protocol_wire::PieceData &, int);

        // Update terms when selling
        void updateTerms(const protocol_wire::SellerTerms &);

        //// Getters

        SessionState state() const;

        SessionMode mode() const;

        // Status of session
        status::Session<ConnectionIdType> status() const noexcept;

    private:

        // Session mode
        SessionMode _mode;

        // Current state of session
        SessionState _state;

        // Connections
        detail::ConnectionMap<ConnectionIdType> _connections;

        // When session was started
        time_t _started;

        //// Substates

        // Each pointer is != nullptr only when _mode corresponds

        // Observer
        detail::Observing<ConnectionIdType> * _observing;

        // Seller
        detail::Selling<ConnectionIdType> * _selling;

        // Buyer
        detail::Buying<ConnectionIdType> * _buying;

        friend class detail::Observing<ConnectionIdType>;
        friend class detail::Selling<ConnectionIdType>;
        friend class detail::Buying<ConnectionIdType>;

        //// Handle callbacks from connections

        void peerAnnouncedModeAndTerms(const ConnectionIdType &, const protocol_statemachine::AnnouncedModeAndTerms &);
        void invitedToOutdatedContract(const ConnectionIdType &);
        void invitedToJoinContract(const ConnectionIdType &);
        void contractPrepared(const ConnectionIdType &, uint64_t, const Coin::typesafeOutPoint &, const Coin::PublicKey &, const Coin::PubKeyHash &payorFinalPkHash);
        void pieceRequested(const ConnectionIdType &, int i);
        void invalidPieceRequested(const ConnectionIdType &);
        void paymentInterrupted(const ConnectionIdType &);
        void receivedValidPayment(const ConnectionIdType &, const Coin::Signature &);
        void receivedInvalidPayment(const ConnectionIdType &, const Coin::Signature &);
        void sellerHasJoined(const ConnectionIdType &);
        void sellerHasInterruptedContract(const ConnectionIdType &);
        void receivedFullPiece(const ConnectionIdType &, const protocol_wire::PieceData &);
        void remoteMessageOverflow(const ConnectionIdType &);
        void localMessageOverflow(const ConnectionIdType &);

        //// Utility routines

        // Creates a connection
        detail::Connection<ConnectionIdType> * createConnection(const ConnectionIdType & id, const SendMessageOnConnectionCallbacks &);

        // not sure, should we return connection pointer, or just id?
        std::vector<detail::Connection<ConnectionIdType> *> connectionsWithPeerInMode(protocol_statemachine::ModeAnnounced m);

        template <typename T>
        std::vector<detail::Connection<ConnectionIdType> *> connectionsInState() const;

        // Returns connection if present, otherwise throws exception
        // ConnectionDoesNotExist<ConnectionIdType>
        detail::Connection<ConnectionIdType> * get(const ConnectionIdType &) const;

        // Removes connection with given id from the connections map and deletes it and throws,
        // Returns iterator at next valid element
        typename detail::ConnectionMap<ConnectionIdType>::const_iterator destroyConnection(const ConnectionIdType &);

        // If possible, creates connection and adds to map
        detail::Connection<ConnectionIdType> * createAndAddConnection(const ConnectionIdType &, const SendMessageOnConnectionCallbacks &);
    };

}
}

// Templated type defenitions
#include <protocol_session/Session.cpp>

#endif // JOYSTREAM_PROTOCOLSESSION_SESSION_HPP
