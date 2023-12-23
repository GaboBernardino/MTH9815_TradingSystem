/**
* BondTradeBookingService.hpp
*
* Derives from TradeBookingService to handle bonds
*
* @author: Gabo Bernardino
*/

#ifndef BONDTRADEBOOKINGSERVICE_HPP
#define BONDTRADEBOOKINGSERVICE_HPP

#include <array>
#include <fstream>
#include <unordered_map>
#include "../tradebookingservice.hpp"
#include "../utils.hpp"
#include "BondPositionService.hpp"
#include "BondExecutionService.hpp"

/**
 * Trade Booking Service to book trades to a particular book.
 * Specialized for Bonds
 */
class BondTradeBookingService : public TradeBookingService<Bond> {
private:
  std::vector<ServiceListener<Trade<Bond>>*> listeners_;
  std::unordered_map<std::string, Trade<Bond>> trades_;  // keyed on trade id

public:
  // ctor
  BondTradeBookingService();

  // Get data on our service given a key
  virtual Trade<Bond>& GetData(std::string key) override;

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(Trade<Bond>& data) override;

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<Trade<Bond>>* listener) override;

  // Get all listeners on the Service.
  virtual const vector<ServiceListener<Trade<Bond>>*>& GetListeners() const override;

  // Book the trade
  virtual void BookTrade(Trade<Bond>& trade) override;
};

/**
* Trade booking listener specialized for bonds
* Sends trades from Booking service to Position service
*/
class BondTradeBookingListener : public ServiceListener<ExecutionOrder<Bond>> {
private:
  BondTradeBookingService* bondTradeBookingService_;

  // keep count of book to place the trade on
  std::array<std::string, 3> books_;
  int counter_;


public:
  // ctor
  BondTradeBookingListener(BondTradeBookingService* _service);
  BondTradeBookingListener() = default;

  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(ExecutionOrder<Bond>& data) override;

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(ExecutionOrder<Bond>& data) override;

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(ExecutionOrder<Bond>& data) override;
};


/**
* Trade booking connector class specialized for bonds;
* Subscribe-only connector
*/
class BondTradeBookingConnector : public Connector<Trade<Bond>> {
private:
  BondTradeBookingService* tradeBookingService_;
public:
  BondTradeBookingConnector(BondTradeBookingService* _service);
  BondTradeBookingConnector() = default;

  // Subscribe to a Service
  virtual void Subscribe(const char* filename, const bool& header = true) override;

  // Publish data - this one is subscribe only tho
  virtual void Publish(Trade<Bond>& data) override;
};


// ************************************************************************************************
// BondTradeBookingService implementations
// ************************************************************************************************
BondTradeBookingService::BondTradeBookingService() {
  trades_ = std::unordered_map<std::string, Trade<Bond>>();
}

Trade<Bond>& BondTradeBookingService::GetData(std::string key) {
  return trades_[key];
}

void BondTradeBookingService::OnMessage(Trade<Bond>& data) {
  // book the trade
  BookTrade(data);
}

void BondTradeBookingService::AddListener(ServiceListener<Trade<Bond>>* listener) {
  listeners_.push_back(listener);
}

const vector<ServiceListener<Trade<Bond>>*>& BondTradeBookingService::GetListeners() const {
  return listeners_;
}

void BondTradeBookingService::BookTrade(Trade<Bond>& trade) {
  // add data to the stored trades:
  std::string id = trade.GetTradeId();
  trades_[id] = trade;

  std::cout << "Communicating trade to Position Listeners" << std::endl;
  // communicate trade to position service
  for (auto l : listeners_) {
    l->ProcessUpdate(trade);
  }
}


// ************************************************************************************************
// BondTradeBookingConnector implementations
// ************************************************************************************************
BondTradeBookingConnector::BondTradeBookingConnector(BondTradeBookingService* _service) :
  tradeBookingService_(_service) {}

void BondTradeBookingConnector::Subscribe(const char* filename, const bool& header) {
  std::string line;
  std::vector<std::string> row;
  Bond bond;
  double trade_price;
  long trade_size;
  Side side;

  try {
    std::ifstream in(filename);
    if (header) std::getline(in, line);  // skip header
    
    while (std::getline(in, line)) {
      // get bond id, trade id, price, book, size and side
      boost::algorithm::split(row, line, boost::algorithm::is_any_of(","));

      // some items need preprocessing
      // create a bond object from the id:
      bond = MakeBond(row[0]);
      std::cout << "Bond: " << bond << std::endl;
      // compute price
      trade_price = StringToPrice(row[2]);
      // trade size:
      trade_size = std::stol(row[4]);
      // trade side:
      boost::algorithm::trim(row[5]);
      side = (row[5] == "BUY") ? BUY : SELL;

      Trade<Bond> trade_obj(bond, row[1], trade_price, row[3], trade_size, side);
      // communicate trade to service
      tradeBookingService_->OnMessage(trade_obj);

      std::cout << std::endl;
    }
  }
  catch (std::exception& e) {
    std::cout << "An error occurred: " << e.what() << std::endl;
  }
}

void BondTradeBookingConnector::Publish(Trade<Bond>& data) {
  // subscribe only
}


// ************************************************************************************************
// BondTradeBookingListener implementations
// ************************************************************************************************
BondTradeBookingListener::BondTradeBookingListener(BondTradeBookingService* _service) :
  bondTradeBookingService_(_service)
{
  books_ = std::array<std::string, 3>{"TRSY1", "TRSY2", "TRSY3"};
  counter_ = 0;
}

void BondTradeBookingListener::ProcessAdd(ExecutionOrder<Bond>& data) {
  // bond object:
  std::string id = data.GetProduct().GetProductId();
  Bond bond = MakeBond(id);
  //trade data:
  std::string trade_id = bond.GetTicker() + "57747FFC" + std::to_string(counter_);
  double price = data.GetPrice();
  std::string book = books_[counter_];
  counter_++; counter_ %= 3;
  long qnt = data.GetHiddenQuantity() + data.GetVisibleQuantity();
  Side side = (data.GetSide() == OFFER) ? BUY : SELL;

  Trade<Bond> trade_obj(bond, trade_id, price, book, qnt, side);
  bondTradeBookingService_->BookTrade(trade_obj);
}

void BondTradeBookingListener::ProcessRemove(ExecutionOrder<Bond>& data) {
  // not implemented
}

void BondTradeBookingListener::ProcessUpdate(ExecutionOrder<Bond>& data) {
  // not implemented
}

#endif
