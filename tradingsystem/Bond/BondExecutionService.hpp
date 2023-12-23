/**
* BondExecutionService.hpp
*
* Derives from ExecutionService to handle bonds
*
* @author: Gabo Bernardino
*/

#ifndef BONDEXECUTIONSERVICE_HPP
#define BONDEXECUTIONSERVICE_HPP

#include <array>
#include "../executionservice.hpp"
#include "BondTradeBookingService.hpp"
#include "BondAlgoExecutionService.hpp"


/**
 * Execution Service class specialized for bonds;
 * stores a vector of listeners and a map of strings -> execution orders
 * 
 * Gets data via a listener on BondAlgoExecutionService and communicates it
 * to TradeBooking listeners to book a trade
 */
class BondExecutionService : public ExecutionService<Bond> {
private:
  std::vector<ServiceListener<ExecutionOrder<Bond>>*> listeners_;
  std::unordered_map<std::string, ExecutionOrder<Bond>> orders_;

public:
  //ctor
  BondExecutionService();

  // Get data on our service given a key
  virtual ExecutionOrder<Bond>& GetData(std::string key) override;

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(ExecutionOrder<Bond>& data) override;

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<ExecutionOrder<Bond>>* listener) override;

  // Get all listeners on the Service.
  virtual const vector<ServiceListener<ExecutionOrder<Bond>>*>& GetListeners() const override;

  // Execute an order on a market
  virtual void ExecuteOrder(ExecutionOrder<Bond>& order, Market market) override;
};


/**
* Execution listener specialized for bonds
* gets execution order from algo and places it on BondTradeBookingService
*/
class BondExecutionListener : public ServiceListener<AlgoExecution<Bond>> {
private:
  BondExecutionService* bondExecService_;

  // keep count of market to alternate between them
  std::array<Market, 3> markets_;
  int counter_;

public:
  // ctor
  BondExecutionListener(BondExecutionService* _service);
  BondExecutionListener() = default;

  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(AlgoExecution<Bond>& data) override;

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(AlgoExecution<Bond>& data) override;

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(AlgoExecution<Bond>& data) override;
};


//*************************************************************************************************
// BondExecutionService implementations
//*************************************************************************************************
BondExecutionService::BondExecutionService() {
  orders_ = std::unordered_map<std::string, ExecutionOrder<Bond>>();
}

ExecutionOrder<Bond>& BondExecutionService::GetData(std::string key) {
  return orders_[key];
}

void BondExecutionService::OnMessage(ExecutionOrder<Bond>& data) {
  // not implemented for this service
}

void BondExecutionService::AddListener(ServiceListener<ExecutionOrder<Bond>>* listener) {
  listeners_.push_back(listener);
}

const vector<ServiceListener<ExecutionOrder<Bond>>*>& BondExecutionService::GetListeners() const {
  return listeners_;
}

void BondExecutionService::ExecuteOrder(ExecutionOrder<Bond>& order, Market market) {
  // add order to map
  std::string id = order.GetProduct().GetProductId();
  orders_[id] = order;

  // communicate order to trade listeners
  std::cout << "Communicating order " << id << " to TradeBooking Listeners..." << endl;
  for (auto l : listeners_) {
    l->ProcessAdd(order);
  }
}

//*************************************************************************************************
// BondExecutionListener implementations
//*************************************************************************************************
BondExecutionListener::BondExecutionListener(BondExecutionService* _service) :
  bondExecService_(_service)
{
  markets_ = std::array<Market, 3>{ BROKERTEC, ESPEED, CME };
  counter_ = 0;
}

void BondExecutionListener::ProcessAdd(AlgoExecution<Bond>& data) {
  // not implemented
}

void BondExecutionListener::ProcessRemove(AlgoExecution<Bond>& data) {
  // not implemented
}

void BondExecutionListener::ProcessUpdate(AlgoExecution<Bond>& data) {
  
  // get market to place the order on
  Market mkt = markets_[counter_];
  counter_++; counter_ %= 3;
  // get executon order from algo
  ExecutionOrder<Bond> order = data.GetOrder();

  bondExecService_->ExecuteOrder(order, mkt);
}


#endif // !BONDEXECUTIONSERVICE_HPP
