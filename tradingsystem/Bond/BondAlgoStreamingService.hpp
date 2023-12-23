/**
* BondAlgoStreamingService.hpp
*
* Defines an AlgoStreamingService base class and derives a specialization for Bonds
*
* @author: Gabo Bernardino
*/

#ifndef BONDAGSTREAMINGSERVICE_HPP
#define BONDAGSTREAMINGSERVICE_HPP

#include "../streamingservice.hpp"
#include "../pricingservice.hpp"

template <typename T>
class AlgoStream {

private:
  PriceStream<T> priceStream_;

public:
  AlgoStream(const PriceStream<T>& _stream);
  AlgoStream() = default;

  const PriceStream<T>& GetPriceStream() const;
};

//*************************************************************************************************
// AlgoStream implementations
//*************************************************************************************************
template <typename T>
AlgoStream<T>::AlgoStream(const PriceStream<T>& _stream) :
  priceStream_(_stream) {}

template <typename T>
const PriceStream<T>& AlgoStream<T>::GetPriceStream() const {
  return priceStream_;
}


/**
 * Algo Streaming Service which registers a ServiceListener
 * on the BondPricingService and send the bid/offer prices to the BondStreamingService
 * via a ServiceListener and the PublishPrice() method
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class AlgoStreamingService : public Service<std::string, AlgoStream<T>> {

public:
  // Send bid/offer prices to BondStreamingService
  virtual void PublishPrice(const Price<Bond>& price) = 0;
};

class BondAlgoStreamingService : public AlgoStreamingService<Bond> {
private:
  std::vector<ServiceListener<AlgoStream<Bond>>*> listeners_;
  std::unordered_map<std::string, AlgoStream<Bond>> algo_streams_;

  // keep counter to decide size of orders
  long counter_;

public:
  //ctor
  BondAlgoStreamingService();

  // Get data on our service given a key
  virtual AlgoStream<Bond>& GetData(std::string key) override;

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(AlgoStream<Bond>& data) override;

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<AlgoStream<Bond>>* listener) override;

  // Get all listeners on the Service.
  virtual const vector<ServiceListener<AlgoStream<Bond>>*>& GetListeners() const override;

  // Send an order to the book
  virtual void PublishPrice(const Price<Bond>& price) override;
};

/**
* Algo streaming listener specialized for bonds
* Receives prices from a pricing service
*/
class BondAlgoStreamingListener : public ServiceListener<Price<Bond>> {
private:
  BondAlgoStreamingService* bondAlgoStreamService_;

public:
  // ctor
  BondAlgoStreamingListener(BondAlgoStreamingService* _service);
  BondAlgoStreamingListener() = default;

  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(Price<Bond>& data) override;

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(Price<Bond>& data) override;

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(Price<Bond>& data) override;
};


//*************************************************************************************************
// BondAlgoStreamingService implementations
//*************************************************************************************************
BondAlgoStreamingService::BondAlgoStreamingService() {
  algo_streams_ = std::unordered_map<std::string, AlgoStream<Bond>>();
  counter_ = 0L;
}

AlgoStream<Bond>& BondAlgoStreamingService::GetData(std::string key) {
  return algo_streams_[key];
}

void BondAlgoStreamingService::OnMessage(AlgoStream<Bond>& data) {
  // not implemented for this service
}

void BondAlgoStreamingService::AddListener(ServiceListener<AlgoStream<Bond>>* listener) {
  listeners_.push_back(listener);
}

const vector<ServiceListener<AlgoStream<Bond>>*>& BondAlgoStreamingService::GetListeners() const {
  return listeners_;
}

void BondAlgoStreamingService::PublishPrice(const Price<Bond>& price) {

  // get price information
  double mid = price.GetMid(), spread = price.GetBidOfferSpread();
  double bid_price = mid - 0.5 * spread, offer_price = mid + 0.5 * spread;

  // quantities - instructions:
  // Alternate visible sizes between 1000000 and 2000000 on subsequent updates for both sides.
  // Hidden size should be twice the visible size at all times.
  long visible_qnt = (counter_ % 2) ? 1000000 : 2000000;
  long hidden_qnt = 2 * visible_qnt;

  // create price stream order objcts
  PriceStreamOrder bid_order(bid_price, visible_qnt, hidden_qnt, BID);
  PriceStreamOrder offer_order(offer_price, visible_qnt, hidden_qnt, OFFER);

  Bond bond = price.GetProduct();

  // create the price stream and algo stream obects + add algo stream to map
  PriceStream<Bond> price_stream(bond, bid_order, offer_order);
  AlgoStream<Bond> stream_obj(price_stream);
  algo_streams_[bond.GetProductId()] = stream_obj;

  std::cout << "Communicating algo stream to StreamingListeners..." << std::endl;
  for (auto l : listeners_) {
    l->ProcessUpdate(stream_obj);
  }

  counter_++;
}


//*************************************************************************************************
// BondAlgoStreamingListener implementations
//*************************************************************************************************
BondAlgoStreamingListener::BondAlgoStreamingListener(BondAlgoStreamingService* _service) :
  bondAlgoStreamService_(_service) {}

void BondAlgoStreamingListener::ProcessAdd(Price<Bond>& data) {
  bondAlgoStreamService_->PublishPrice(data);
}

void BondAlgoStreamingListener::ProcessRemove(Price<Bond>& data) {
  // not implemented
}

void BondAlgoStreamingListener::ProcessUpdate(Price<Bond>& data) {
  // not implemented
}

#endif // !BONDAGSTREAMINGSERVICE_HPP
