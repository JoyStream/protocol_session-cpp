/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, May 9 2016
 */

#include "SessionSpy.hpp"

template <class ConnectionIdType>
SessionSpy<ConnectionIdType>::SessionSpy(Session<ConnectionIdType> * session)
    : _session(session) {
}

template <class ConnectionIdType>
SessionSpy<ConnectionIdType>::~SessionSpy() {
    removeConnectionSpies();
}

template <class ConnectionIdType>
void SessionSpy<ConnectionIdType>::toMonitoredObserveMode() {
    _session->toObserveMode(removedConnectionCallbackSlot.hook());
}

template <class ConnectionIdType>
void SessionSpy<ConnectionIdType>::toMonitoredSellMode(const protocol_wire::SellerTerms & terms,
                                                       int maxPieceIndex) {

    _session->toSellMode(removedConnectionCallbackSlot.hook(),
                         loadPieceForBuyerCallbackSlot.hook(),
                         claimLastPaymentCallbackSlot.hook(),
                         anchorAnnouncedCallbackSlot.hook(),
                         ReceivedValidPayment<ConnectionIdType>([](const ConnectionIdType &, uint64_t, uint64_t, uint64_t) -> void{}), // not bothering to add monitoring to this now, session will be deprecated
                         terms,
                         maxPieceIndex);

}

template <class ConnectionIdType>
void SessionSpy<ConnectionIdType>::toMonitoredBuyMode(const protocol_wire::BuyerTerms & terms,
                                                      const TorrentPieceInformation & information) {
    _session->toBuyMode(removedConnectionCallbackSlot.hook(),
                        fullPieceArrivedCallbackSlot.hook(),
                        SentPayment<ConnectionIdType>([](const ConnectionIdType &, uint64_t, uint64_t, uint64_t, int) -> void {}), // not bothering to add monitoring to this now, session will be deprecated
                        terms,
                        information);
}

template <class ConnectionIdType>
ConnectionSpy<ConnectionIdType> * SessionSpy<ConnectionIdType>::addConnection(const ConnectionIdType & id) {

    // Create spy for connection
    ConnectionSpy<ConnectionIdType> * spy = new ConnectionSpy<ConnectionIdType>(id);

    // Create connection, spied on by spy, and add to underlying session
    // NB*: may cause exception if connecton id already has been added, in which case spy leaks
    _session->addConnection(id, spy->callbacks);

    // Insert spy into mapping
    connectionSpies.insert(std::make_pair(id, spy));

    return spy;
}

template <class ConnectionIdType>
void SessionSpy<ConnectionIdType>::removeConnectionSpies() {

    for(auto mapping : connectionSpies)
        delete mapping.second;

    connectionSpies.clear();
}

template <class ConnectionIdType>
bool SessionSpy<ConnectionIdType>::blank() const {

    for(auto mapping : connectionSpies) {
        if(!mapping.second->blank())
            return false;
    }

    return blankSession();
}

template <class ConnectionIdType>
bool SessionSpy<ConnectionIdType>::blankSession() const {

            //// General
    return  removedConnectionCallbackSlot.empty() &&
            //// Buying
            fullPieceArrivedCallbackSlot.empty() &&
            //// Selling
            loadPieceForBuyerCallbackSlot.empty() &&
            claimLastPaymentCallbackSlot.empty() &&
            anchorAnnouncedCallbackSlot.empty();
}


template <class ConnectionIdType>
bool SessionSpy<ConnectionIdType>::blankConnectionExcept(const ConnectionIdType & id) const {

    for(auto mapping : connectionSpies) {

        // any connection, different from id, which is not blank, leads to false result
        if(mapping.first != id && !mapping.second->blank())
            return false;
    }

    return true;
}

template <class ConnectionIdType>
void SessionSpy<ConnectionIdType>::reset() {

    //// General
    removedConnectionCallbackSlot.clear();

    //// Buying
    fullPieceArrivedCallbackSlot.clear();

    //// Selling
    loadPieceForBuyerCallbackSlot.clear();
    claimLastPaymentCallbackSlot.clear();
    anchorAnnouncedCallbackSlot.clear();

    // Connection spies
    for(auto mapping : connectionSpies)
        mapping.second->reset();
}

template <class ConnectionIdType>
bool SessionSpy<ConnectionIdType>::onlyCalledRemovedConnection() const {
    return  //// General
            !removedConnectionCallbackSlot.empty() &&
            //// Buying
            fullPieceArrivedCallbackSlot.empty() &&
            //// Selling
            loadPieceForBuyerCallbackSlot.empty() &&
            claimLastPaymentCallbackSlot.empty() &&
            anchorAnnouncedCallbackSlot.empty();
}

template <class ConnectionIdType>
bool SessionSpy<ConnectionIdType>::onlyCalledBroadcastTransaction() const {
    return  //// General
            removedConnectionCallbackSlot.empty() &&
            //// Buying
            fullPieceArrivedCallbackSlot.empty() &&
            //// Selling
            loadPieceForBuyerCallbackSlot.empty() &&
            claimLastPaymentCallbackSlot.empty() &&
            anchorAnnouncedCallbackSlot.empty();
}

template <class ConnectionIdType>
bool SessionSpy<ConnectionIdType>::onlyCalledFullPieceArrived() const {
    return  //// General
            removedConnectionCallbackSlot.empty() &&
            //// Buying
            !fullPieceArrivedCallbackSlot.empty() &&
            //// Selling
            loadPieceForBuyerCallbackSlot.empty() &&
            claimLastPaymentCallbackSlot.empty() &&
            anchorAnnouncedCallbackSlot.empty();
}

template <class ConnectionIdType>
bool SessionSpy<ConnectionIdType>::onlyCalledLoadPieceForBuyer() const {
    return  //// General
            removedConnectionCallbackSlot.empty() &&
            //// Buying
            fullPieceArrivedCallbackSlot.empty() &&
            //// Selling
            !loadPieceForBuyerCallbackSlot.empty() &&
            claimLastPaymentCallbackSlot.empty() &&
            anchorAnnouncedCallbackSlot.empty();
}

template <class ConnectionIdType>
bool SessionSpy<ConnectionIdType>::onlyCalledClaimLastPayment() const {
    return  //// General
            removedConnectionCallbackSlot.empty() &&
            //// Buying
            fullPieceArrivedCallbackSlot.empty() &&
            //// Selling
            loadPieceForBuyerCallbackSlot.empty() &&
            !claimLastPaymentCallbackSlot.empty() &&
            anchorAnnouncedCallbackSlot.empty();
}

template <class ConnectionIdType>
bool SessionSpy<ConnectionIdType>::onlyCalledAnchorAnnounced() const {
    return  //// General
            removedConnectionCallbackSlot.empty() &&
            //// Buying
            fullPieceArrivedCallbackSlot.empty() &&
            //// Selling
            loadPieceForBuyerCallbackSlot.empty() &&
            claimLastPaymentCallbackSlot.empty() &&
            !anchorAnnouncedCallbackSlot.empty();
}
