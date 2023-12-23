/**
* BondAlgoExecutionService.hpp
*
* Defines an AlgoExecutionService base class and derives a specialization for Bonds
*
* @author: Gabo Bernardino
*/

#ifndef BONDALGOEXECUTIONSERVICE_HPP
#define BONDALGOEXECUTIONSERVICE_HPP

#include "../executionservice.hpp"
#include "../marketdataservice.hpp"


template <typename T>
class AlgoExecution {

private:
  ExecutionOrder<T> order_;

public:
  AlgoExecution(const ExecutionOrder<T>& order);
  AlgoExecution() = default;

  const ExecutionOrder<T>& GetOrder() const;
};

//*************************************************************************************************
// AlgoExecution implementations
//*************************************************************************************************
template <typename T>
AlgoExecution<T>::AlgoExecution(const ExecutionOrder<T>& order) :
  order_(order) {}

template <typename T>
const ExecutionOrder<T>& AlgoExecution<T>::GetOrder() const {
  return order_;
}

/**
 * Algo Execution Service which registers a ServiceListener
 * on the BondMarketDataService and aggresses the top of the book
 * Keyed on product identifier.
 * Type T is the product type.
 */
template <typename T>
class AlgoExecutionService : public Service<std::string, AlgoExecution<T>> {
public:
  // Send an order to the book
  virtual void SendOrder(OrderBook<T>& orderBook) = 0;
};


class BondAlgoExecutionService : public AlgoExecutionService<Bond> {
private:
  std::vector<ServiceListener<AlgoExecution<Bond>>*> listeners_;
  std::unordered_map<std::string, AlgoExecution<Bond>> algo_execs_;  // keyed on product id/

  // keep track of which side of the book we are on (even -> BID, odd -> offer)
  long counter_;

public:
  //ctor
  BondAlgoExecutionService();

  // Get data on our service given a key
  virtual AlgoExecution<Bond>& GetData(std::string key) override;

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(AlgoExecution<Bond>& data) override;

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<AlgoExecution<Bond>>* listener) override;

  // Get all listeners on the Service.
  virtual const vector<ServiceListener<AlgoExecution<Bond>>*>& GetListeners() const override;

  // Send an order to the book
  virtual void SendOrder(OrderBook<Bond>& orderBook) override;
};

/**
* Algo execution listener specialized for bonds
* Aggresses the top of the book, alternating between bid and offer 
* and only aggressing when the spread is at its tightest
*/
class BondAlgoExecutionListener : public ServiceListener<OrderBook<Bond>> {
private:
  BondAlgoExecutionService* bondAlgoExecService_;

public:
  // ctor
  BondAlgoExecutionListener(BondAlgoExecutionService* _service);
  BondAlgoExecutionListener() = default;

  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(OrderBook<Bond>& data) override;

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(OrderBook<Bond>& data) override;

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(OrderBook<Bond>& data) override;
};


//*************************************************************************************************
// BondAlgoExecutionService implementations
//*************************************************************************************************
BondAlgoExecutionService::BondAlgoExecutionService() {
  algo_execs_ = std::unordered_map<std::string, AlgoExecution<Bond>>();
  counter_ = 0L;
}

AlgoExecution<Bond>& BondAlgoExecutionService::GetData(std::string key) {
  return algo_execs_[key];
}

void BondAlgoExecutionService::OnMessage(AlgoExecution<Bond>& data) {
  // not implemented for this service
}

void BondAlgoExecutionService::AddListener(ServiceListener<AlgoExecution<Bond>>* listener) {
  listeners_.push_back(listener);
}

const vector<ServiceListener<AlgoExecution<Bond>>*>& BondAlgoExecutionService::GetListeners() const {
  return listeners_;
}

void BondAlgoExecutionService::SendOrder(OrderBook<Bond>& orderBook) {
  
  std::string id = orderBook.GetProduct().GetProductId();  // product to trade
  // top of the book (both sides):
  BidOffer best = orderBook.GetBestBidOffer();

  // instructions: "only aggressing when the spread is at its tightest (i.e. 1/128th)"
  double minimum_spread = 1. / 128.;
  if (best.GetBidOrder().GetPrice() - best.GetOfferOrder().GetPrice() >= minimum_spread) {
    Bond bond = orderBook.GetProduct();
    
    std::string order_id = bond.GetTicker() + "57747FFC" + std::to_string(counter_);

    // order specifications - most of them will be hardcoded for simplicity
    long all_qnt, visible_qnt, hidden_qnt;  // quantities
    long divisor = 4;  // visible to hidden ratio of 1 to 3
    PricingSide side;

    if (counter_ % 2) {
      // even number of trades gone thru => BID
      side = BID;
      // get quantity
      all_qnt = best.GetBidOrder().GetQuantity();
      
    }
    else {
      // odd number of trades gone thru => OFFER
      side = OFFER;
      // compute quantites
      all_qnt = best.GetOfferOrder().GetQuantity();
    }
    
    // compute quantites
    visible_qnt = all_qnt / divisor;
    hidden_qnt = all_qnt - visible_qnt;
    
    // now handle the execution
    ExecutionOrder<Bond> order(bond, side, order_id, MARKET, 1., visible_qnt, hidden_qnt, "", false);
    // note that we are crossing the spread -> use market orders and dont worry about the price
    AlgoExecution<Bond> algo(order);
    algo_execs_[id] = algo;

    std::cout << "Communicating order " << order_id << " to Execution Listeners..." << std::endl;
    for (auto l : listeners_) {
      l->ProcessUpdate(algo);
    }
    
    counter_++;
  }
}



// ************************************************************************************************
// BondAlgoExecutionListener implementations
// ************************************************************************************************
BondAlgoExecutionListener::BondAlgoExecutionListener(BondAlgoExecutionService* _service) :
  bondAlgoExecService_(_service) {}

void BondAlgoExecutionListener::ProcessAdd(OrderBook<Bond>& data) {
  bondAlgoExecService_->SendOrder(data);
}

void BondAlgoExecutionListener::ProcessRemove(OrderBook<Bond>& data) {
  // not implemented
}

void BondAlgoExecutionListener::ProcessUpdate(OrderBook<Bond>& data) {
  // not implemented
}

#endif // !BONDALGOEXECUTIONSERVICE_HPP
