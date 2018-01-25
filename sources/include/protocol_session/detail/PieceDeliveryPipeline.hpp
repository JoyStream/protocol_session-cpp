#ifndef JOYSTREAM_PROTOCOLSESSION_PIECEDEILIVERYPIPELINE_HPP
#define JOYSTREAM_PROTOCOLSESSION_PIECEDEILIVERYPIPELINE_HPP


#include <boost/variant.hpp>
#include <memory>
#include <deque>
#include <vector>

namespace joystream {
namespace protocol_wire {
    class PieceData;
}

namespace protocol_session {
namespace detail {

class PieceDeliveryPipeline {

public:
  PieceDeliveryPipeline ();

  int add(int index);

  int dataReady(int index, const protocol_wire::PieceData &);

  void paymentReceived();

  std::vector<int> getNextBatchToLoad(int maxLoaded);

  std::vector<protocol_wire::PieceData> getNextBatchToSend(int maxPendingPayments);

private:

  struct Piece {
    Piece (int i) : index(i) {}

    const int index;

    // Initial state of the Piece - before a request is made to load it
    struct NotRequested {};

    // A request was made to load the piece data
    struct Loading {};

    // Once the piece data is available it is ready to be sent
    struct ReadyToSend {
      protocol_wire::PieceData data;
    };

    // A piece remains in pipeline in this state until the next payment is received
    struct WaitingForPayment {};

    boost::variant<NotRequested, Loading, ReadyToSend, WaitingForPayment> state;

    template<typename T>
    bool inState() const {
      if(boost::get<T>(&state)) {
        return true;
      } else {
        return false;
      }
    }
  };

  // Double ended queue used (works better than a queue or vector) for both iteration
  // to update elements not at the begining or end,
  // and for fast efficient push/pop operations
  std::deque<Piece> _pipeline;
};


}
}
}

#endif
