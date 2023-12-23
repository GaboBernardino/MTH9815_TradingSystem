/**
* BondStreamingService.hpp
*
* Derives from StreamingService to handle bonds
*
* @author: Gabo Bernardino
*/

#ifndef BONDSTREAMINGSERVICE_HPP
#define BONDSTREAMINGSERVICE_HPP

#include "../streamingservice.hpp"
#include "BondTradeBookingService.hpp"
#include "BondAlgoStreamingService.hpp"


/**
 * Bond Streaming Service
 * uses a Connector to publish streams via socket into a separate process
 * which listens to the streams on the socket via its own Connector
 * and prints them when it receives them.
 * Keyed on product identifier.
 * Type T is the product type.
 */
class BondStreamingService : public StreamingService<Bond> {
private:
  std::vector<ServiceListener<PriceStream<Bond>>*> listeners_;
  std::unordered_map<std::string, PriceStream<Bond>> streams_;

public:
  //ctor
  BondStreamingService();

  // Get data on our service given a key
  virtual PriceStream<Bond>& GetData(std::string key) override;

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(PriceStream<Bond>& data) override;

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<PriceStream<Bond>>* listener) override;

  // Get all listeners on the Service.
  virtual const vector<ServiceListener<PriceStream<Bond>>*>& GetListeners() const override;

  // Publish two-way prices
  virtual void PublishPrice(PriceStream<Bond>& priceStream) override;
};


/**
* Streaming listener specialized for bonds
* Aggresses the top of the book, alternating between bid and offer
* and only aggressing when the spread is at its tightest
*/
class BondStreamingListener : public ServiceListener<AlgoStream<Bond>> {
private:
  BondStreamingService* bondStreamService_;

public:
  // ctor
  BondStreamingListener(BondStreamingService* _service);
  BondStreamingListener() = default;

  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(AlgoStream<Bond>& data) override;

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(AlgoStream<Bond>& data) override;

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(AlgoStream<Bond>& data) override;
};


//*************************************************************************************************
// BondStreamingService implementations
//*************************************************************************************************
BondStreamingService::BondStreamingService() {
  streams_ = std::unordered_map<std::string, PriceStream<Bond>>();
}

PriceStream<Bond>& BondStreamingService::GetData(std::string key) {
  return streams_[key];
}

void BondStreamingService::OnMessage(PriceStream<Bond>& data) {
  // not implemented for this service
}

void BondStreamingService::AddListener(ServiceListener<PriceStream<Bond>>* listener) {
  listeners_.push_back(listener);
}

const vector<ServiceListener<PriceStream<Bond>>*>& BondStreamingService::GetListeners() const {
  return listeners_;
}

void BondStreamingService::PublishPrice(PriceStream<Bond>& priceStream) {
  // add order to map
  std::string id = priceStream.GetProduct().GetProductId();
  streams_[id] = priceStream;

  // communicate order to listeners
  std::cout << "Communicating price stream for bond " << id << " to PriceStream Listeners..." << endl;
  for (auto l : listeners_) {
    l->ProcessAdd(priceStream);
  }
}

//*************************************************************************************************
// BondStreamingListener implementations
//*************************************************************************************************
BondStreamingListener::BondStreamingListener(BondStreamingService* _service) :
  bondStreamService_(_service) {}

void BondStreamingListener::ProcessAdd(AlgoStream<Bond>& data) {
  // not implemented
}

void BondStreamingListener::ProcessRemove(AlgoStream<Bond>& data) {
  // not implemented
}

void BondStreamingListener::ProcessUpdate(AlgoStream<Bond>& data) {
  
  // get price stream from algo
  PriceStream<Bond> price = data.GetPriceStream();

  bondStreamService_->PublishPrice(price);
}


#endif // !BONDStreamingSERVICE_HPP
