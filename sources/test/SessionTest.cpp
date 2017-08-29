/**
 * Copyright (C) JoyStream - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Bedeho Mender <bedeho.mender@gmail.com>, April 18 2016
 */

#include <SessionTest.hpp>

#include <thread>
#include <common/Seed.hpp>
#include <CoinCore/hdkeys.h>

namespace joystream {
namespace protocol_session {
    template<>
    std::string IdToString<ID>(const ID & s) {
        return std::to_string(s);
    }
}}

SessionTest::SessionTest()
    : //network(Coin::Network::testnet3)
    session(nullptr)
    , spy(nullptr) {
}

void SessionTest::init() {

    // Reset next key counter
    nextKey = 1;

    session = new Session<ID>();

    spy = new
    SessionSpy<ID>(
                /**
    [this](const P2SHScriptGeneratorFromPubKey & scriptGenerator, const uchar_vector &) -> Coin::KeyPair {

        Coin::PrivateKey sk = nextPrivateKey();

        scriptGenerator(sk.toPublicKey());

        return Coin::KeyPair(sk);
    },
    [this](int n) {

        std::vector<Coin::P2PKHAddress> addresses;

        for(int i = 0;i < n;i++){

            Coin::P2PKHAddress addr(this->network, nextPrivateKey().toPublicKey().toPubKeyHash());
            addresses.push_back(addr);
        }
        return addresses;

    },
    [this](int n) {

        std::vector<Coin::P2PKHAddress> addresses;

        for(int i = 0;i < n;i++){

            Coin::P2PKHAddress addr(this->network, nextPrivateKey().toPublicKey().toPubKeyHash());
            addresses.push_back(addr);
        }
        return addresses;

    },*/
    session);
}

void SessionTest::cleanup() {
    delete session;
    delete spy;

    session = nullptr;
    spy = nullptr;
}

Coin::PrivateKey SessionTest::nextPrivateKey() {

    std::stringstream s;
    s << std::hex << nextKey++;
    std::string hexInteger = s.str(); // MSB is at index 0

    // Make even length representation by potentially adding leading 0
    if(hexInteger.length() % 2 != 0)
        hexInteger = "0" + hexInteger;

    std::string finalHexRepresentation;

    if(hexInteger.length() > 2*Coin::Seed::length())
        throw std::runtime_error("privateKeyFromUInt: argument to big"); // not even going to truncate
    else if(hexInteger.length() < 2*Coin::Seed::length())
        finalHexRepresentation = std::string(2*Coin::Seed::length() - hexInteger.length(), '0') + hexInteger; // add suitable number of leading 0s

    assert(finalHexRepresentation.length() == 2*Coin::Seed::length());

    //std::cout << "Generate ---- " << finalHexRepresentation.length() << std::endl;

    Coin::Seed seed = Coin::Seed::fromRawHex(finalHexRepresentation);

    Coin::HDKeychain keyChain(seed.generateHDKeychain());

    return Coin::PrivateKey::fromRaw(keyChain.privkey());
}

paymentchannel::Payor SessionTest::getPayor(const protocol_wire::SellerTerms & sellerTerms,
                                     const protocol_wire::Ready & ready,
                                     const Coin::PrivateKey & payorContractSk,
                                     const Coin::PublicKey & payeeContractPk,
                                     const Coin::PubKeyHash & payeeFinalPkHash) {
   return paymentchannel::Payor(sellerTerms.minPrice(),
                                0,
                                ready.value(),
                                sellerTerms.settlementFee(),
                                Coin::RelativeLockTime::fromTimeUnits(sellerTerms.minLock()),
                                ready.anchor(),
                                Coin::KeyPair(payorContractSk),
                                ready.finalPkHash(),
                                payeeContractPk,
                                payeeFinalPkHash);
}

////

void SessionTest::basic() {
    //// (1) four peers join without announcing their mode
    addConnection(0);
    addConnection(1);
    addConnection(2);
    addConnection(3);

    // add same as before
    EXPECT_THROW(addConnection(0), joystream::protocol_session::exception::ConnectionAlreadyAddedException<ID>);

    //// (2) one drops out
    removeConnection(3);

    //// (3) the rest change modes
    session->processMessageOnConnection(0, protocol_wire::Observe());
    session->processMessageOnConnection(1, protocol_wire::Sell(protocol_wire::SellerTerms(1,2,3,4,5), 1337));
    session->processMessageOnConnection(2, protocol_wire::Buy(protocol_wire::BuyerTerms(10,20,30,40)));

    // Reset in case it is a buying session, where above terms warrant invite
    spy->reset();

    //// (4) pause
    pause();

    //// (5) new peer joins
    addConnection(4);

    //// (6) old peer updates terms
    session->processMessageOnConnection(0, protocol_wire::Observe());
    session->processMessageOnConnection(1, protocol_wire::Sell(protocol_wire::SellerTerms(33,44,66,22,32), 1338));
    session->processMessageOnConnection(2, protocol_wire::Buy(protocol_wire::BuyerTerms(100,200,300,400)));

    //// (7) stop
    stop();

    //// (8) start
    firstStart();
}

void SessionTest::addConnection(ID id) {
    ConnectionSpy<ID> * c = spy->addConnection(id);
    assertTermsSentToPeer(c);

    // Check the state of session
    EXPECT_TRUE(session->hasConnection(id));

    auto ids = session->connectionIds();
    auto itr = std::find(ids.cbegin(), ids.cend(), id);

    EXPECT_TRUE(itr != ids.cend());
    EXPECT_TRUE(spy->blankSession());

    spy->reset();
}

void SessionTest::removeConnection(ID id) {
    // remove peer
    session->removeConnection(id);

    // assert removal
    assertConnectionRemoved(id, DisconnectCause::client);

    // extra check that no other callback was made
    EXPECT_TRUE(spy->onlyCalledRemovedConnection());

    spy->reset();
}

void SessionTest::firstStart() {
    session->start();

    EXPECT_EQ(session->state(), SessionState::started);
    EXPECT_TRUE(spy->blankSession());

    // iterate peers and check that correct mode message had been sent to whatever
    for(auto mapping : spy->connectionSpies)
        assertTermsSentToPeer(mapping.second);

    spy->reset();
}

void SessionTest::stop() {
    assert(session->mode() != SessionMode::not_set);

    // Get all connections present before stop
    std::set<ID> presentBeforeStop = session->connectionIds();

    session->stop();

    EXPECT_EQ(session->state(), SessionState::stopped);

    // Check that removal callback was made for each
    std::set<ID> removed;
    for(auto f : spy->removedConnectionCallbackSlot)
        removed.insert(std::get<0>(f));

    EXPECT_EQ(removed, presentBeforeStop);

    spy->reset();
    spy->removeConnectionSpies();
}

void SessionTest::pause() {
    session->pause();
    EXPECT_EQ(session->state(), SessionState::paused);
    EXPECT_TRUE(spy->blank());
}

void SessionTest::toObserveMode() {
    spy->toMonitoredObserveMode();
    EXPECT_EQ(session->mode(), SessionMode::observing);
}

void SessionTest::toSellMode(const protocol_wire::SellerTerms & terms,
                      int maxPieceIndex) {
    spy->toMonitoredSellMode(terms, maxPieceIndex);
    EXPECT_EQ(session->mode(), SessionMode::selling);
}

void SessionTest::toBuyMode(const protocol_wire::BuyerTerms & terms,
                     const TorrentPieceInformation & information) {
    spy->toMonitoredBuyMode(terms,
                            information);
    EXPECT_EQ(session->mode(), SessionMode::buying);
}

void SessionTest::assertConnectionRemoved(ID expectedId, DisconnectCause expectedCause) const {
    // peer is gone
    EXPECT_TRUE(!session->hasConnection(expectedId));
    auto ids = session->connectionIds();
    auto itr = std::find(ids.cbegin(), ids.cend(), expectedId);
    EXPECT_TRUE(itr == ids.cend());

    // Iterate removal callbacks, in case multiple peers are removed at ones (e.g. stopping):
    // exactly one removal callback was made for given peer,
    // and with given cause
    bool callbackHasBeenMade = false;
    for(auto f : spy->removedConnectionCallbackSlot) {

        ID removedPeer;
        DisconnectCause cause;
        std::tie(removedPeer, cause) = f;

        // is this the given peer
        if(removedPeer == expectedId) {
            // is this correct cause
            if(cause == expectedCause) {
                if(callbackHasBeenMade)
                    FAIL() << "correct removedConnection callback made multiple times.";
                else
                    callbackHasBeenMade = true;
            } else
               FAIL() << "removedConnection callback made for peer with incorrect cause.";
        }
    }

    if(!callbackHasBeenMade)
        FAIL() << "No removedConnection callback made for peer.";

    // notice that we dont clear spy
}

void SessionTest::assertTermsSentToPeer(const ConnectionSpy<ID> *connection) const {

    // only one message should have been sent
    EXPECT_EQ((int)connection->callbackCallCount(), 1);

    // having a hard time comparing terms, as they cannot be recovered
    // from session easily

    switch(session->mode()) {
        case SessionMode::not_set: assert(false);
        case SessionMode::buying:
          EXPECT_EQ(connection->sendBuyCallbackSlot.size(), 1);
          //auto f = connection->sendBuyCallbackSlot.front();
          //auto m = std::get<0>(f); // buy message
          //howto compare sent buy message with terms in session?
          break;
        case SessionMode::selling:
          EXPECT_EQ(connection->sendSellCallbackSlot.size(), 1);
          break;
        case SessionMode::observing:
          EXPECT_EQ(connection->sendObserveCallbackSlot.size(), 1);
          break;
        default:
            assert(false);
    }
}

void SessionTest::assertTermsSentToAllPeers() const {

    for(auto mapping : spy->connectionSpies)
        assertTermsSentToPeer(mapping.second);
}

void SessionTest::assertFullPieceSent(ID peer, const protocol_wire::PieceData & data) const {
    ConnectionSpy<ID> * c = spy->connectionSpies.at(peer);

    EXPECT_EQ((int)c->sendFullPieceCallbackSlot.size(), 1);
    auto m2 = std::get<0>(c->sendFullPieceCallbackSlot.front());

    EXPECT_EQ(m2.pieceData(), data);
}

////

void SessionTest::addBuyerAndGoToReadyForPieceRequest(ID id, const protocol_wire::BuyerTerms & terms, const protocol_wire::Ready & ready, Coin::PublicKey & payeeContractPk, Coin::PubKeyHash & payeeFinalPkHash) {
    // peer joins
    addConnection(id);

    // peer announces buyer mode
    session->processMessageOnConnection(id, protocol_wire::Buy(terms));

    EXPECT_TRUE(spy->blank());

    // peer (buyer) sends invitation to terms which are good enough
    // NB: we assume the seller has only sent one terms message, which has 0 index
    session->processMessageOnConnection(id, protocol_wire::JoinContract(0));

    /// Seller joins contract
    Coin::KeyPair contract(nextPrivateKey());
    Coin::PublicKey finalPk(nextPrivateKey().toPublicKey());
    session->startUploading(id, terms, contract, finalPk.toPubKeyHash());

    // client joined contract
    ConnectionSpy<ID> * cSpy = spy->connectionSpies.at(id);

    EXPECT_EQ((int)cSpy->sendJoiningContractCallbackSlot.size(), 1);
    auto m2 = std::get<0>(cSpy->sendJoiningContractCallbackSlot.front());

    // copy out return values which payee sent
    payeeContractPk = m2.contractPk();
    payeeFinalPkHash = m2.finalPkHash();

    spy->reset();

    // peer (buyer) announces ready
    session->processMessageOnConnection(id, ready);

    // check that announcement callback was made
    EXPECT_TRUE(spy->onlyCalledAnchorAnnounced());
    EXPECT_EQ((int)spy->anchorAnnouncedCallbackSlot.size(), 1);

    ID announcedId;
    uint64_t value;
    Coin::typesafeOutPoint anchor;
    Coin::PublicKey contractPk;
    Coin::PubKeyHash finalPkHash;

    std::tie(announcedId, value, anchor, contractPk, finalPkHash) = spy->anchorAnnouncedCallbackSlot.front();

    EXPECT_EQ(announcedId, id);
    //FIXME: QCOMPARE(value, ready.value());
    EXPECT_EQ(anchor, ready.anchor());
    EXPECT_EQ(contractPk, ready.contractPk());
    EXPECT_EQ(finalPkHash, ready.finalPkHash());

    spy->reset();
}

void SessionTest::receiveValidFullPieceRequest(ID peer, int pieceIndexToRequest) {
    session->processMessageOnConnection(peer, protocol_wire::RequestFullPiece(pieceIndexToRequest));

    EXPECT_TRUE(spy->onlyCalledLoadPieceForBuyer());
    EXPECT_EQ((int)spy->loadPieceForBuyerCallbackSlot.size(), 1);

    ID id;
    int pieceIndexToLoad;

    std::tie(id, pieceIndexToLoad) = spy->loadPieceForBuyerCallbackSlot.front();
    EXPECT_EQ(id, peer);
    EXPECT_EQ(pieceIndexToRequest, pieceIndexToLoad);

    spy->reset();
}

void SessionTest::sendFullPiece(ID id, const protocol_wire::PieceData & data, int pieceIndex) {
    session->pieceLoaded(id, data, pieceIndex);

    EXPECT_TRUE(spy->blankSession());

    ConnectionSpy<ID> * c = spy->connectionSpies.at(id);

    EXPECT_EQ((int)c->sendFullPieceCallbackSlot.size(), 1);
    auto m2 = std::get<0>(c->sendFullPieceCallbackSlot.front());

    EXPECT_EQ(m2.pieceData(), data);

    spy->reset();
}

void SessionTest::exchangeDataForPayment(ID peer, uint N, paymentchannel::Payor & payor) {
    for(uint i = 0;i < N;i++) {
        // peer requests valid piece
        receiveValidFullPieceRequest(peer, i);

        // send message to peer
        protocol_wire::PieceData data;
        sendFullPiece(peer, data, i);

        // generate new payment
        Coin::Signature sig = payor.makePayment();

        // peer sends valid payment
        session->processMessageOnConnection(peer, protocol_wire::Payment(sig));

        EXPECT_TRUE(spy->blank());
    }
}

void SessionTest::add(SellerPeer & peer) {
    addConnection(peer.id);
    peer.spy = spy->connectionSpies.at(peer.id);
    session->processMessageOnConnection(peer.id, protocol_wire::Sell(peer.terms, peer.sellerTermsIndex));
}

void SessionTest::completeExchange(SellerPeer & peer) {
    ConnectionSpy<ID> * c = peer.spy;

    // is there a pending full piece request?
    if(c->callbackCallCount() == 0)
        return;

    int requestedPiece = 0;

    {
        EXPECT_EQ((int)c->sendRequestFullPieceCallbackSlot.size(), 1);
        auto m2 = std::get<0>(c->sendRequestFullPieceCallbackSlot.front());

        requestedPiece = m2.pieceIndex();

        // Remove message from slot
        c->sendRequestFullPieceCallbackSlot.clear();
    }

    // if so respond with some data
    protocol_wire::PieceData dataSent = protocol_wire::PieceData::fromHex("179017230471923470");
    session->processMessageOnConnection(peer.id, protocol_wire::FullPiece(dataSent));

    // check that session made client notification about arrival of piece
    EXPECT_EQ((int)spy->fullPieceArrivedCallbackSlot.size(), 1);

    {
        ID id;
        protocol_wire::PieceData receivedData;
        int index;
        std::tie(id, receivedData, index) = spy->fullPieceArrivedCallbackSlot.front();

        EXPECT_EQ(id, peer.id);
        EXPECT_EQ(receivedData, dataSent);
        EXPECT_EQ(index, requestedPiece);

        // tell session that it was valid
        session->validPieceReceivedOnConnection(peer.id, index);

        // clear call
        spy->fullPieceArrivedCallbackSlot.clear();
    }

    // check that a payment was sent to the peer
    // message should be: payment, full_piece_request
    {
        EXPECT_TRUE((int)c->sendPaymentCallbackSlot.size() > 0);
        auto m2 = std::get<0>(c->sendPaymentCallbackSlot.front());

        // validate payment
        peer.validatePayment(m2.sig());

        // Ditch payment message
        c->sendPaymentCallbackSlot.pop_front();
    }
}

bool SessionTest::hasPendingFullPieceRequest(const std::vector<SellerPeer> & peers) {
    bool has = false;

    for(auto p : peers)
        has = has || p.hasPendingFullPieceRequest();

    return has;
}

void SessionTest::takeSingleSellerToExchange(SellerPeer & peer) {
    // Add connection and announce seller terms
    add(peer);
    assertSellerInvited(peer);
    spy->reset();

    // Have peer join
    session->processMessageOnConnection(peer.id, peer.setJoiningContract());

    // no need to wait due to policy

    /// Start download

    // Setup
    auto v = { BuyerSellerRelationship(StartDownloadConnectionInformation(peer.terms,  0, 9999999, Coin::KeyPair(nextPrivateKey()), nextPrivateKey().toPublicKey().toPubKeyHash()), peer)};
    Coin::Transaction contractTx = simpleContract(v);
    peer.assertContractValidity(contractTx);
    PeerToStartDownloadInformationMap<ID> map = downloadInformationMap(v);

    // Start download
    session->startDownloading(contractTx, map);

    // make sure contract was announced to only relevant peers
    peer.contractAnnounced();

    ////////peer.assertContractValidity(tx);
}

/**
void Test::join(const SellerPeer & peer) {
    session->processMessageOnConnection(peer.id, peer.joinMessage());
}
*/

void SessionTest::assertSellerInvited(const SellerPeer & peer) {
    // client joined contract
    EXPECT_EQ((int)peer.spy->sendJoinContractCallbackSlot.size(), 1);
    auto m2 = std::get<0>(peer.spy->sendJoinContractCallbackSlot.front());

    EXPECT_EQ(m2.index(), peer.sellerTermsIndex);

}
