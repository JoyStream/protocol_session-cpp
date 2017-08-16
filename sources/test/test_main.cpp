#include <gtest/gtest.h>

#include <SessionTest.hpp>
#include <SessionSpy.hpp>

using namespace joystream;
using namespace joystream::protocol_session;

#define TEST_PRIVATE_KEY "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"

TEST_F(SessionTest, observing)
{
    init();

    // to observe mode
    toObserveMode();

    // incorrectly try to go to same mode again
    EXPECT_THROW(toObserveMode(), exception::SessionAlreadyInThisMode);

    // Start session
    firstStart();

    // incorrectly try to start again
    EXPECT_THROW(firstStart(), exception::StateIncompatibleOperation);

    // Do basic tests
    basic();

    //// Do transitions to other modes: buy, back to observe, sell

    // Have some new peers join, without announcing mode,
    // and transition to buy mode
    addConnection(0);
    addConnection(1);

    // Go to buy mode
    toBuyMode(protocol_wire::BuyerTerms(),
              TorrentPieceInformation());

    // back to observe mode
    toObserveMode();

    // Go to sell mode
    toSellMode(protocol_wire::SellerTerms(),
               0);

    cleanup();
}

TEST_F(SessionTest, selling_basic)
{
    init();

    // to observe mode
    toSellMode(protocol_wire::SellerTerms(22, 134, 1, 88, 32), 0);

    // incorrectly try to go to same mode again
    EXPECT_THROW(toSellMode(protocol_wire::SellerTerms(), 0),
                             exception::SessionAlreadyInThisMode);

    // Start session
    firstStart();

    // incorrectly try to start again
    EXPECT_THROW(firstStart(), exception::StateIncompatibleOperation);

    // Do basic tests
    basic();

    //// Do transitions to other modes: buy, back to sell, observe

    // Have some new peers join, without announcing mode,
    // and transition to buy mode
    addConnection(0);
    addConnection(1);

    // Go to buy mode
    toBuyMode(protocol_wire::BuyerTerms(),
              TorrentPieceInformation());

    // back to sell mode
    toSellMode(protocol_wire::SellerTerms(22, 134, 1, 88, 32), 0);

    // Go to observe mode
    toObserveMode();

    cleanup();
}

TEST_F(SessionTest, selling)
{
    init();

    protocol_wire::SellerTerms sellerTerms(22, 134, 10, 88, 32);
    protocol_wire::BuyerTerms buyerTerms(24, 200, 2, 400);
    Coin::PrivateKey payorContractSk = Coin::PrivateKey::fromRawHex(TEST_PRIVATE_KEY);
    protocol_wire::Ready ready(1123,
                               Coin::typesafeOutPoint(Coin::TransactionId::fromRPCByteOrder(std::string("97a27e013e66bec6cb6704cfcaa5b62d4fc6894658f570ed7d15353835cf3547")), 55),
                               payorContractSk.toPublicKey(),
                               Coin::RedeemScriptHash::fromRawHash(uchar_vector("03a3fac91cac4a5c9ec870b444c4890ec7d68671")));

    ID peer = 0;
    uint numberOfExchangesWhileStarted = 5;

    assert(sellerTerms.satisfiedBy(buyerTerms));

    // back to sell mode
    toSellMode(sellerTerms, numberOfExchangesWhileStarted + 1);

    // Start session
    firstStart();

    // Add a buyer peer and take connection to state ReadyForPieceRequest
    Coin::PublicKey payeeContractPk;
    Coin::RedeemScriptHash payeeFinalScriptHash;
    addBuyerAndGoToReadyForPieceRequest(peer, buyerTerms, ready, payeeContractPk, payeeFinalScriptHash);

    // Do sequence of data for payments exchanges
    paymentchannel::Payor payor = getPayor(sellerTerms, ready, payorContractSk, payeeContractPk, payeeFinalScriptHash);

    exchangeDataForPayment(peer, numberOfExchangesWhileStarted, payor);

    // Receive a single request
    receiveValidFullPieceRequest(peer, numberOfExchangesWhileStarted);

    // Pause
    pause();

    // Load piece, without it being sent
    protocol_wire::PieceData data;
    session->pieceLoaded(peer, data, numberOfExchangesWhileStarted);

    EXPECT_TRUE(spy->blankSession());
    ConnectionSpy<ID> * c = spy->connectionSpies.at(peer);
    EXPECT_EQ((int)c->callbackCallCount(), 0);

    // Start up again
    session->start();

    // Make sure this results in piece being sent
    assertFullPieceSent(peer, data);
    spy->reset();

    // Update terms
    protocol_wire::SellerTerms newTerms;
    session->updateTerms(newTerms);

    // Check that peers were sent
    assertTermsSentToAllPeers();

    spy->reset();

    cleanup();
}

/**
TEST_F(SessionTest, selling_buyer_invited_with_bad_terms)
{
    protocol_wire::SellerTerms sellerTerms(22, 134, 10, 88, 32);
    protocol_wire::BuyerTerms badBuyerTerms;
    ID peer = 0;

    assert(!sellerTerms.satisfiedBy(badBuyerTerms));

    // back to sell mode
    toSellMode(sellerTerms, 0);

    // Start session
    firstStart();

    // Have some new peers join, without announcing mode,
    addConnection(peer);

    // peer announces buyer mode
    session->processMessageOnConnection(peer, protocol_wire::Buy(badBuyerTerms));

    EXPECT_TRUE(spy->blank());

    // peer (buyer) sends invitation to terms which are not good enough
    session->processMessageOnConnection(peer, protocol_wire::JoinContract(0));

    EXPECT_TRUE(spy->onlyCalledRemovedConnection());
    EXPECT_EQ((int)spy->removedConnectionCallbackSlot.size(), 1);

    ID id;
    DisconnectCause cause;
    std::tie(id, cause) = spy->removedConnectionCallbackSlot.front();
    EXPECT_EQ(id, peer);
    EXPECT_EQ(cause, DisconnectCause::buyer_invited_with_bad_terms);

    spy->reset();
}
*/

TEST_F(SessionTest, selling_buyer_requested_invalid_piece)
{
    init();

    int invalidPieceIndex = 1337;
    ID peer = 0;

    // back to sell mode
    toSellMode(protocol_wire::SellerTerms(), 5);

    // Start session
    firstStart();

    // Add a buyer peer and take connection to state ReadyForPieceRequest
    Coin::PublicKey payeeContractPk;
    Coin::RedeemScriptHash payeeFinalScriptHash;
    addBuyerAndGoToReadyForPieceRequest(peer, protocol_wire::BuyerTerms(), protocol_wire::Ready(), payeeContractPk, payeeFinalScriptHash);

    // peer requests invalid piece
    session->processMessageOnConnection(peer, protocol_wire::RequestFullPiece(invalidPieceIndex));

    EXPECT_TRUE(spy->onlyCalledRemovedConnection());

    EXPECT_EQ((int)spy->removedConnectionCallbackSlot.size(), 1);

    ID id;
    DisconnectCause cause;
    std::tie(id, cause) = spy->removedConnectionCallbackSlot.front();
    EXPECT_EQ(id, peer);
    EXPECT_EQ(cause, DisconnectCause::buyer_requested_invalid_piece);

    spy->reset();

    cleanup();
}

TEST_F(SessionTest, selling_buyer_interrupted_payment)
{
    init();

    int pieceIndex = 0;
    ID peer = 0;

    // back to sell mode
    toSellMode(protocol_wire::SellerTerms(), 5);

    // Start session
    firstStart();

    // Add a buyer peer and take connection to state ReadyForPieceRequest
    Coin::PublicKey payeeContractPk;
    Coin::RedeemScriptHash payeeFinalScriptHash;
    addBuyerAndGoToReadyForPieceRequest(peer, protocol_wire::BuyerTerms(), protocol_wire::Ready(), payeeContractPk, payeeFinalScriptHash);

    // peer requests valid piece
    receiveValidFullPieceRequest(peer, pieceIndex);

    // peer resends terms, which should cause payment interrupt
    session->processMessageOnConnection(peer, protocol_wire::Buy());

    // check that peer was removed due to this
    assertConnectionRemoved(peer, DisconnectCause::buyer_interrupted_payment);
    spy->reset();

    cleanup();
}

TEST_F(SessionTest, selling_buyer_sent_invalid_payment)
{
    init();

    int pieceIndex = 0;
    protocol_wire::PieceData data;
    ID peer = 0;

    // Must have valid public in ready message
    Coin::PrivateKey payorContractSk = Coin::PrivateKey::fromRawHex(TEST_PRIVATE_KEY);
    protocol_wire::Ready ready = protocol_wire::Ready(1123,
                                                      Coin::typesafeOutPoint(Coin::TransactionId::fromRPCByteOrder(std::string("97a27e013e66bec6cb6704cfcaa5b62d4fc6894658f570ed7d15353835cf3547")), 55),
                                                      payorContractSk.toPublicKey(),
                                                      Coin::RedeemScriptHash::fromRawHash(uchar_vector("03a3fac91cac4a5c9ec870b444c4890ec7d68671")));
    // back to sell mode
    toSellMode(protocol_wire::SellerTerms(), 5);

    // Start session
    firstStart();

    // Add a buyer peer and take connection to state ReadyForPieceRequest
    Coin::PublicKey payeeContractPk;
    Coin::RedeemScriptHash payeeFinalScriptHash;
    addBuyerAndGoToReadyForPieceRequest(peer, protocol_wire::BuyerTerms(), ready, payeeContractPk, payeeFinalScriptHash);

    // peer requests valid piece
    receiveValidFullPieceRequest(peer, pieceIndex);

    // send message to peer
    sendFullPiece(peer, data, pieceIndex);

    // peer sends invalid payment (blank signature)
    session->processMessageOnConnection(peer, protocol_wire::Payment());

    // check that peer was removed due to this
    assertConnectionRemoved(peer, DisconnectCause::buyer_sent_invalid_payment);
    spy->reset();

    cleanup();
}

TEST_F(SessionTest, selling_buyer_disappears)
{
    init();

    ID peer = 0;
    uint numberOfExchangesBeforeDisappearance = 5;

    protocol_wire::SellerTerms sellerTerms(22, 134, 10, 88, 32);
    protocol_wire::BuyerTerms buyerTerms(24, 200, 2, 400);
    Coin::PrivateKey payorContractSk = Coin::PrivateKey::fromRawHex(TEST_PRIVATE_KEY);
    protocol_wire::Ready ready(1123,
                               Coin::typesafeOutPoint(Coin::TransactionId::fromRPCByteOrder(std::string("97a27e013e66bec6cb6704cfcaa5b62d4fc6894658f570ed7d15353835cf3547")), 55),
                               payorContractSk.toPublicKey(),
                               Coin::RedeemScriptHash::fromRawHash(uchar_vector("03a3fac91cac4a5c9ec870b444c4890ec7d68671")));

    assert(sellerTerms.satisfiedBy(buyerTerms));

    // back to sell mode
    toSellMode(sellerTerms, 5);

    // Start session
    firstStart();

    // Add a buyer peer and take connection to state ReadyForPieceRequest
    Coin::PublicKey payeeContractPk;
    Coin::RedeemScriptHash payeeFinalScriptHash;
    addBuyerAndGoToReadyForPieceRequest(peer, buyerTerms, ready, payeeContractPk, payeeFinalScriptHash);

    // Do sequence of data for payments exchanges
    paymentchannel::Payor payor = getPayor(sellerTerms, ready, payorContractSk, payeeContractPk, payeeFinalScriptHash);

    exchangeDataForPayment(peer, numberOfExchangesBeforeDisappearance, payor);

    // connection is dropped: we can't use Test::removeConnection: does spy->reset, and asserts no other callback
    session->removeConnection(peer);

    // make sure we are notified about claiming last payment
    {
        ID removedPeer;
        joystream::paymentchannel::Payee payee;
        EXPECT_TRUE(spy->claimLastPaymentCallbackSlot.size() == 1);

        std::tie(removedPeer, payee) = spy->claimLastPaymentCallbackSlot.front();

        EXPECT_EQ(removedPeer, peer);
        EXPECT_EQ((uint)payee.numberOfPaymentsMade(), numberOfExchangesBeforeDisappearance);
    }

    // and that connecion is dropped from session
    assertConnectionRemoved(peer, DisconnectCause::client);

    cleanup();
}

TEST_F(SessionTest, buying_basic)
{
    init();

    // To buy mode
    toBuyMode(protocol_wire::BuyerTerms(), TorrentPieceInformation());

    // Incorrectly try to go to same mode again
    EXPECT_THROW(toBuyMode(protocol_wire::BuyerTerms(), TorrentPieceInformation()),
                             exception::SessionAlreadyInThisMode);

    // Start session
    firstStart();

    // incorrectly try to start again
    EXPECT_THROW(firstStart(), exception::StateIncompatibleOperation);

    // Do basic tests
    basic();

    //// Do transitions to other modes: observe, back to buy, sell

    // Have some new peers join, without announcing mode,
    // and transition to buy mode
    addConnection(0);
    addConnection(1);

    // Go to observe mode
    toObserveMode();

    // Back to buy mode
    toBuyMode(protocol_wire::BuyerTerms(), TorrentPieceInformation());

    cleanup();
}

TEST_F(SessionTest, buying)
{
    init();

    // min #sellers = 3
    protocol_wire::BuyerTerms buyerTerms(24, 200, 3, 400);
    SellerPeer first(0, protocol_wire::SellerTerms(22, 134, 10, 88, 32), 2075),
               second(1, protocol_wire::SellerTerms(4, 13, 11, 88, 32), 1234),
               third(2, protocol_wire::SellerTerms(1, 2, 3, 88, 32), 123),
               bad(3, protocol_wire::SellerTerms(),765);

    assert(buyerTerms.satisfiedBy(first.terms));
    assert(buyerTerms.satisfiedBy(second.terms));
    assert(buyerTerms.satisfiedBy(third.terms));
    assert(!buyerTerms.satisfiedBy(bad.terms));

    // To buy mode
    /**
    std::shared_ptr<Coin::UnspentOutput> utxo(
      new Coin::UnspentP2SHOutput(Coin::KeyPair::generate(),
                                  uchar_vector(),
                                  uchar_vector(),
                                  Coin::typesafeOutPoint(Coin::TransactionId::fromRPCByteOrder(std::string("97a27e013e66bec6cb6704cfcaa5b62d4fc6894658f570ed7d15353835cf3547")),
                                                         55),
                                  34561)
    );

    Coin::UnspentOutputSet funding({utxo});
    */

    // minTimeBeforeBuildingContract = 1
    // servicingPieceTimeOutLimit = 2

    uint totalNumberOfPieces = 30;
    TorrentPieceInformation information;
    for(uint i = 0;i < totalNumberOfPieces;i++)
        information.push_back(PieceInformation(0, (i % 2) == 0)); // every even index piece is missing

    toBuyMode(buyerTerms, information);

    // Start session
    firstStart();

    // Add connection and announce seller terms, which are good enough, and hence an invitation should arrive
    add(first);
    assertSellerInvited(first);

    // Add another seller with good terms
    add(second);
    assertSellerInvited(second);

    // Add another seller with bad terms
    add(bad);
    EXPECT_TRUE(spy->blank()); // nothing should happen

    // tick()
    session->tick();
    EXPECT_TRUE(spy->blank()); // no seller has joined, and there are to few (min 3)

    // Make the two good and one bad join
    session->processMessageOnConnection(first.id, first.setJoiningContract());
    session->processMessageOnConnection(second.id, second.setJoiningContract());
    session->processMessageOnConnection(bad.id, bad.setJoiningContract()); // not legal, should be ignored

    // tick()
    session->tick();
    EXPECT_TRUE(spy->blank()); // Nothing should happen, as min 3 good sellers are required

    // Make last good one join
    add(third);
    assertSellerInvited(third);
    spy->reset();

    session->processMessageOnConnection(third.id, third.setJoiningContract());

    // Nothing should happen, due to policy.minTimeBeforeBuildingContract() not having passed
    EXPECT_TRUE(spy->blank());

    /// Start download

    // Setup
    auto v = { BuyerSellerRelationship(StartDownloadConnectionInformation(first.terms,  0, 9999999, Coin::KeyPair(nextPrivateKey()), nextPrivateKey().toPublicKey().toPubKeyHash()), first),
               BuyerSellerRelationship(StartDownloadConnectionInformation(second.terms, 1, 2222222, Coin::KeyPair(nextPrivateKey()), nextPrivateKey().toPublicKey().toPubKeyHash()), second),
               BuyerSellerRelationship(StartDownloadConnectionInformation(third.terms,  2, 3333333, Coin::KeyPair(nextPrivateKey()), nextPrivateKey().toPublicKey().toPubKeyHash()), third)};

    Coin::Transaction contractTx = simpleContract(v);

    first.assertContractValidity(contractTx);
    second.assertContractValidity(contractTx);
    third.assertContractValidity(contractTx);

    PeerToStartDownloadInformationMap<ID> map = downloadInformationMap(v);

    // Start download
    session->startDownloading(contractTx, map);

    // make sure contract was announced to only relevant peers
    first.contractAnnounced();
    second.contractAnnounced();
    third.contractAnnounced();
    bad.assertNoContractAnnounced();

    // asserts for this? requires us to understand
    // _generateKeyPairs(numberOfSellers);
    // _generateP2PKHAddresses(numberOfSellers);

    // Service all piece requests from buyer until done
    do {
        // Attempts to complete a round of exchange for each peer
        completeExchange(first);
        completeExchange(second);
        completeExchange(third);
    } while(hasPendingFullPieceRequest({first, second, third}));

    // assert that state is done, all is good
    // can't access buying sub state!

    cleanup();
}

TEST_F(SessionTest, buying_seller_has_interrupted_contract)
{
    init();

    // min #sellers = 1
    protocol_wire::BuyerTerms buyerTerms(24, 200, 1, 400);

    SellerPeer first(0, protocol_wire::SellerTerms(22, 134, 10, 88, 32),5634);

    assert(buyerTerms.satisfiedBy(first.terms));

    /**
    // To buy mode
    std::shared_ptr<Coin::UnspentOutput> utxo(
      new Coin::UnspentP2SHOutput(Coin::KeyPair::generate(),
                                  uchar_vector(),
                                  uchar_vector(),
                                  Coin::typesafeOutPoint(Coin::TransactionId::fromRPCByteOrder(std::string("97a27e013e66bec6cb6704cfcaa5b62d4fc6894658f570ed7d15353835cf3547")),
                                                         55),
                                  34561)
    );

    Coin::UnspentOutputSet funding({utxo});
    */

    // minTimeBeforeBuildingContract = 0
    // servicingPieceTimeOutLimit = 2

    uint totalNumberOfPieces = 30;
    TorrentPieceInformation information;
    for(uint i = 0;i < totalNumberOfPieces;i++)
        information.push_back(PieceInformation(0, (i % 2) == 0)); // every even index piece is missing

    ////

    toBuyMode(buyerTerms, information);

    // Start session
    firstStart();

    // Take seller to exchange
    takeSingleSellerToExchange(first);

    // Service all piece requests from buyer until done
    completeExchange(first);

    EXPECT_TRUE(hasPendingFullPieceRequest({first}));

    spy->reset();

    // Peer interrupts contract by resending terms
    session->processMessageOnConnection(first.id, protocol_wire::Buy());

    // assert removal
    assertConnectionRemoved(first.id, DisconnectCause::seller_has_interrupted_contract);

    // extra check that no other callback was made
    EXPECT_TRUE(spy->onlyCalledRemovedConnection());

    spy->reset();

    cleanup();
}

TEST_F(SessionTest, buying_seller_sent_invalid_piece)
{
    init();

    // min #sellers = 1
    protocol_wire::BuyerTerms buyerTerms(24, 200, 1, 400);
    SellerPeer first(0, protocol_wire::SellerTerms(22, 134, 10, 88, 32), 543);

    assert(buyerTerms.satisfiedBy(first.terms));

    /**
    // To buy mode
    std::shared_ptr<Coin::UnspentOutput> utxo(
      new Coin::UnspentP2SHOutput(Coin::KeyPair::generate(),
                                  uchar_vector(),
                                  uchar_vector(),
                                  Coin::typesafeOutPoint(Coin::TransactionId::fromRPCByteOrder(std::string("97a27e013e66bec6cb6704cfcaa5b62d4fc6894658f570ed7d15353835cf3547")),
                                                         55),
                                  34561)
    );
    Coin::UnspentOutputSet funding({utxo});
    */

    // minTimeBeforeBuildingContract = 0
    // servicingPieceTimeOutLimit = 2

    uint totalNumberOfPieces = 30;
    TorrentPieceInformation information;
    for(uint i = 0;i < totalNumberOfPieces;i++)
        information.push_back(PieceInformation(0, (i % 2) == 0)); // every even index piece is missing

    ////
    toBuyMode(buyerTerms, information);

    // Start session
    firstStart();

    // Take seller to exchange
    takeSingleSellerToExchange(first);

    // Recover index of requested piece
    int requestedPiece;
    {
        ConnectionSpy<ID> * c = first.spy;
        EXPECT_EQ((int)c->sendRequestFullPieceCallbackSlot.size(), 1);
        auto m2 = std::get<0>(c->sendRequestFullPieceCallbackSlot.front());

        requestedPiece = m2.pieceIndex();
    }

    // Send piece
    session->processMessageOnConnection(first.id, protocol_wire::FullPiece());
    spy->reset();

    //
    session->invalidPieceReceivedOnConnection(first.id, requestedPiece);

    // assert removal
    assertConnectionRemoved(first.id, DisconnectCause::seller_sent_invalid_piece);

    // extra check that no other callback was made
    EXPECT_TRUE(spy->onlyCalledRemovedConnection());

    spy->reset();

    cleanup();
}


int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
