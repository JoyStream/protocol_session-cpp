#include <protocol_wire/PieceData.hpp>
#include <protocol_session/detail/PieceDeliveryPipeline.hpp>

#include <cassert>

namespace joystream {
namespace protocol_session {
namespace detail {

PieceDeliveryPipeline::PieceDeliveryPipeline () {

}

int PieceDeliveryPipeline::add(int index) {
  // There has to be a reasonable limit on how big the _pipeline can grow
  // This should be capped to the maximum number of payments that can be made on a paymnet channel
  // ignore all requests to add after this limit is reached. Alternatively we can have an internal counter
  // and cap the total number add operations allowed. Or just leave the responsibility to the user of the pipeline
  _pipeline.push_back(Piece(index));

  return _pipeline.size();
}

int PieceDeliveryPipeline::dataReady(int index, const protocol_wire::PieceData & data) {
  int piecesUpdated = 0;

  for (Piece &p : _pipeline) {
    // Save the piece data if piece is in Loading state
    // Save the piece data also if in NotRequested state, this allows us to avoid asking for reading the piece again.
    // We may not find any matching pieces in the pipeline if the call
    // was delayed, we recieve a polite payment, or just called in error. But we will not treat it as a critical error.
    //
    if (p.index == index && p.inState<Piece::Loading>()) {
      // Update the piece state and save the piece data
      auto readyToSend = Piece::ReadyToSend();

      readyToSend.data = data;

      p.state = readyToSend;

      piecesUpdated++;
      // we don't return.. there may be another piece in the pipeline for the same index
      // this is wasteful but allowed.
    }
  }

  // explain what it means for piecesUpdates == 0

  return piecesUpdated;
}

void PieceDeliveryPipeline::paymentReceived() {
  // Protocol state machine prevents overflow messages so this shouldn't happen
  if(_pipeline.size() == 0) {
    //throw std::runtime_error("cannot call paymentReceived on empty pipeline");
    // Calling payment received on an empty pipeline has no effect.
    return;
  }

  // We may get a payment for a piece which is not in WaitingForPayment. This is expected
  // when a buyer is doing a polite compensation before disconnectin the seller.

  // Piece at the front of the queue - remvove it no matter what state it is in.
  _pipeline.pop_front();
}

std::vector<int> PieceDeliveryPipeline::getNextBatchToLoad(int maxPiecesBeingServiced) {
  int n = 0;
  std::vector<int> pieces;

  for (Piece &p : _pipeline) {
    // We always try to service peices at the front of the queue in the order they were added
    // but we limit it so not to waste resources incase the buyer disappears.
    if (n++ > maxPiecesBeingServiced) break;

    // Piece should be waiting to be requested
    if(p.inState<Piece::NotRequested>()) {

      pieces.push_back(p.index);

      // Update the piece state
      p.state = Piece::Loading();
    }
  }

  return pieces;
}

std::vector<protocol_wire::PieceData>
  PieceDeliveryPipeline::getNextBatchToSend(int maxPiecesUnpaidFor) {
    int n = 0;
    std::vector<protocol_wire::PieceData> pieces;

    for (Piece &p : _pipeline) {
      // We will only tolerate having a maximum of maxPiecesUnpaidFor pieces at anytime be delivered
      // and not yet paid for. (a piece is popped of the front of the queue when a payment is received)
      if (n++ > maxPiecesUnpaidFor) break;

      // Abort as soon as we see a piece that is either Loading or NotRequested
      // because we need to send pieces in order they were requested
      if(p.inState<Piece::NotRequested>() || p.inState<Piece::Loading>())
        break;

      auto readyToSend = boost::get<Piece::ReadyToSend>(&p.state);

      // Piece should be ready to send
      if(readyToSend) {

        pieces.push_back(readyToSend->data);

        // Update the piece state
        p.state = Piece::WaitingForPayment();

      } else {
        assert(p.inState<Piece::WaitingForPayment>());
      }
    }

    return pieces;
}

}
}
}
