/**
* BondPositionService.hpp
* 
* Derives from PositionService to handle bonds
* 
* @author: Gabo Bernardino
*/

#ifndef BONDPOSITIONSERVICE_HPP
#define BONDPOSITIONSERVICE_HPP

#include "../positionservice.hpp"
#include "BondRiskService.hpp"


/**
* Position service specialized for Bonds;
* stores a vector of listeners and a map of strings -> position info
* 
* Gets data from listener on TradeBookingService and communicates it
* to Risk Listeners and Historical Data Listeners
*/
class BondPositionService : public PositionService<Bond> {
private:
  std::vector<ServiceListener<Position<Bond>>*> listeners_;
  std::unordered_map<std::string, Position<Bond>> positions_;  // keyed on product id

public:
  // ctor
  BondPositionService();

  // Get data on our service given a key
  virtual Position<Bond>& GetData(std::string key) override;

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(Position<Bond>& data) override;

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<Position<Bond>>* listener) override;

  // Get all listeners on the Service.
  virtual const vector<ServiceListener<Position<Bond>>*>& GetListeners() const override;

  // Add a trade to the service
  virtual void AddTrade(Trade<Bond>& trade) override;
};

/**
* Postion listener specialized for bonds
* Sends trades from Position service to Risk service
*/
class BondPositionListener : public ServiceListener<Trade<Bond>> {
private:
  BondPositionService* bondPositionService_;

public:
  // ctor
  BondPositionListener(BondPositionService* _service);
  BondPositionListener() = default;

  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(Trade<Bond>& data) override;

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(Trade<Bond>& data) override;

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(Trade<Bond>& data) override;
};


// ************************************************************************************************
// BondPositionService implementations
// ************************************************************************************************
BondPositionService::BondPositionService() {
  vector<std::string> cusips{ "91282CJL6", "91282CJK8", "91282CJN2", "91282CJM4", "91282CJJ1", "912810TW8", "912810TV0" };
  
  for (std::string& id : cusips) positions_[id] = Position(MakeBond(id));
}

Position<Bond>& BondPositionService::GetData(std::string key) {
  return positions_[key];
}

void BondPositionService::OnMessage(Position<Bond>& data) {
  // not implemented for this service
}

void BondPositionService::AddListener(ServiceListener<Position<Bond>>* listener) {
  listeners_.push_back(listener);
}

const vector<ServiceListener<Position<Bond>>*>& BondPositionService::GetListeners() const {
  return listeners_;
}

void BondPositionService::AddTrade(Trade<Bond>& trade) {

  // get (current) position object to modify and communicate to listeners
  std::string id = trade.GetProduct().GetProductId();
  Position<Bond>& position_obj = positions_[id];
  // get trade size and direction
  long quantity = trade.GetQuantity();
  if (trade.GetSide() == SELL) quantity *= -1;
  
  // get book for the trade
  std::string book = trade.GetBook();

  // update current position before communicating to listeners
  position_obj.AddPosition(book, quantity);

  cout << "Added trade on book " << book << " for a quantity of " << quantity << endl;

  std::cout << "Communicating new position to Risk Lsteners" << std::endl;
  for (auto l : listeners_) {
    l->ProcessUpdate(position_obj);  // send position to risk listeners
    l->ProcessAdd(position_obj);  // this is for the historical data listener
  }
}


// ************************************************************************************************
// BondPositionListener implementations
// ************************************************************************************************
BondPositionListener::BondPositionListener(BondPositionService* _service) :
  bondPositionService_(_service) {}

void BondPositionListener::ProcessAdd(Trade<Bond>& data) {
  // not implemented
}

void BondPositionListener::ProcessRemove(Trade<Bond>& data) {
  // not implemented
}

void BondPositionListener::ProcessUpdate(Trade<Bond>& data) {
  bondPositionService_->AddTrade(data);
}


#endif // !BONDPOSITIONSERVICE_HPP
