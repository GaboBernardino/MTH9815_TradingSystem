/**
* BondPricingService.hpp
* 
* Derives from PricingService to handle bonds
* 
* @author: Gabo Bernardino
*/

#ifndef BONDPRICINGSERVICE_HPP
#define BONDPRICINGSERVICE_HPP

#include <unordered_map>
#include <stdexcept>
#include <fstream>
#include "../pricingservice.hpp"
#include "../utils.hpp"


/**
* Pricing service class specialized for bonds;
* stores a vector of listeners and a map of strings -> price info
* 
* Gets data from `prices.txt` from a connector and communicates it
* to GUI and AlgoStream listeners
*/
class BondPricingService : public PricingService<Bond> {
private:
  std::vector<ServiceListener<Price<Bond>>*> listeners_;
  std::unordered_map<std::string, Price<Bond>> prices_;  // keyed on product id

public:
  // ctor
  BondPricingService();

  // Get data on our service given a key
  virtual Price<Bond>& GetData(std::string key) override;

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(Price<Bond>& data) override;

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<Price<Bond>>* listener) override;

  // Get all listeners on the Service.
  virtual const vector<ServiceListener<Price<Bond>>*>& GetListeners() const override;
};

/**
* Pricing connector class specialized for bonds;
* Reads from `prices.txt`, creates a Price object and sends it to the service
* Subscribe-only connector
*/
class BondPricingConnector : public PricingConnector<Bond> {
private:
  BondPricingService* bondPricingService_;
public:
  BondPricingConnector(BondPricingService* _service);
  BondPricingConnector() = default;

  // Subscribe to a Service
  virtual void Subscribe(const char* filename, const bool& header = true) override;

  // Publish data - this one is subscribe only tho
  virtual void Publish(Price<Bond>& data) override;
};


// ************************************************************************************************
// BondPricingService implementations
// ************************************************************************************************
BondPricingService::BondPricingService() {
  prices_ = std::unordered_map<std::string, Price<Bond>>();
}

Price<Bond>& BondPricingService::GetData(std::string key) {
  return prices_[key];
}

void BondPricingService::OnMessage(Price<Bond>& data) {
  // add data to the stored prices:
  std::string id = data.GetProduct().GetProductId();
  prices_[id] = data;

  // communicate new price to listeners
  std::cout << "Communicating price to GUI and AlgoStream Listeners..." << std::endl;
  for (auto l : listeners_) {
    l->ProcessAdd(data);
  }
}

void BondPricingService::AddListener(ServiceListener<Price<Bond>>* listener) {
  listeners_.push_back(listener);
}

const vector<ServiceListener<Price<Bond>>*>& BondPricingService::GetListeners() const {
  return listeners_;
}

// ************************************************************************************************
// BondPricingConnector implementations
// ************************************************************************************************
BondPricingConnector::BondPricingConnector(BondPricingService* _service) : 
  bondPricingService_(_service) {}

void BondPricingConnector::Subscribe(const char* filename, const bool& header) {
  std::string line;
  std::vector<std::string> row;  // to store output of string splitting
  Bond bond;  // bond object whose price will be created
  double bid, ask;

  try {
    std::ifstream in(filename);
    if (header) std::getline(in, line);  // skip header

    while (std::getline(in, line)) {
      // preprocess line string
      boost::algorithm::trim(line);
      // get id, bid, offer
      boost::algorithm::split(row, line, boost::algorithm::is_any_of(","));

      // create a bond object from the id
      bond = MakeBond(row[0]);
      
      // get price information
      bid = StringToPrice(row[1]);
      std::cout << std::endl << PrintTimeStamp();
      std::cout << " Bid price = " << bid << "; ";
      ask = StringToPrice(row[2]);
      std::cout << "Ask price = " << ask << std::endl;
      Price<Bond> price_obj(bond, 0.5 * (bid + ask), ask - bid);

      // communicate price to service
      bondPricingService_->OnMessage(price_obj);
    }
  } catch (std::exception& e) {
    std::cout << "An error occurred: " << e.what() << std::endl;
  }
}

void BondPricingConnector::Publish(Price<Bond>& data){
  // subscribe only
}

#endif // !BONDPRICINGSERVICE_HPP
