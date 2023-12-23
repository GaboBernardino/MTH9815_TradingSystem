/**
* BondGUIService.hpp
*
* Defines a GUIService base class and derives a specialization for Bonds
*
* @author: Gabo Bernardino
*/

#ifndef BONDGUISERVICE_HPP
#define BONDGUISERVICE_HPP

#include "../utils.hpp"
#include "BondPricingService.hpp"
#include <chrono>

/**
* GUI connector class specialized for bonds;
* Publish-only connector
*/
class BondGUIConnector : Connector<Price<Bond>> {
private:
  BondPricingService* bondPricingService_;
  std::string fileName_;

public:
  BondGUIConnector(BondPricingService* _service, const std::string& file_name = "gui.txt");
  BondGUIConnector() = default;

  // Subscribe to a Service - this one is publish-only tho
  virtual void Subscribe(const char* filename, const bool& header = true) override;

  // Publish data
  virtual void Publish(Price<Bond>& data) override;
};

/**
 * GUI Service that listens to streaming prices
 * It should register a ServiceListener on the BondPricingService
 * which should notify back to the GUIService at that throttle interval 
 * Keyed on product identifier.
 * Type T is the product type.
 */
template <typename T>
class GUIService : public Service<std::string, Price<T>> {
public:
  // Add price to the service
  virtual void AddPrice(Price<T>& price) = 0;
};


class BondGUIService : public GUIService<Bond> {
private:
  std::vector<ServiceListener<Price<Bond>>*> listeners_;
  std::unordered_map<std::string, Price<Bond>> prices_;  // keyed on product id

  BondGUIConnector* guiConnector_;
  std::chrono::milliseconds throttle_;

public:
  //ctor
  BondGUIService(const int& throttle_interval);
  void SetConnector(BondGUIConnector* _gui_connector);

  // Get data on our service given a key
  virtual Price<Bond>& GetData(std::string key) override;

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(Price<Bond>& data) override;

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<Price<Bond>>* listener) override;

  // Get all listeners on the Service.
  virtual const vector<ServiceListener<Price<Bond>>*>& GetListeners() const override;

  // Add price to the service
  virtual void AddPrice(Price<Bond>& price) override;

  // Return length of throttle interval
  std::chrono::milliseconds GetThrottleInterval() const;
};


/**
* GUI Listener class that listens to prices
* and communicates them to a gui services which then
* sends them to a connector to publish them
*/
class BondGUIListener : public ServiceListener<Price<Bond>> {
private:
  BondGUIService* guiService_;

  std::chrono::system_clock::time_point start_;
  std::chrono::milliseconds throttle_;
  int counter_;  // keep count of read prices - only need to print the first 100

public:
  // ctor
  BondGUIListener(BondGUIService* _service);
  BondGUIListener() = default;

  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(Price<Bond>& data) override;

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(Price<Bond>& data) override;

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(Price<Bond>& data) override;
};

//*************************************************************************************************
// BondGUIService implementations
//*************************************************************************************************
BondGUIService::BondGUIService(const int& throttle_interval) :
  throttle_(throttle_interval)
{
  prices_ = std::unordered_map<std::string, Price<Bond>>();
}

void BondGUIService::SetConnector(BondGUIConnector* _gui_connector) {
  guiConnector_ = _gui_connector;
}

Price<Bond>& BondGUIService::GetData(std::string key) {
  return prices_[key];
}

void BondGUIService::OnMessage(Price<Bond>& data) {
  // not implemented for this service
}

void BondGUIService::AddListener(ServiceListener<Price<Bond>>* listener) {
  listeners_.push_back(listener);
}

const vector<ServiceListener<Price<Bond>>*>& BondGUIService::GetListeners() const {
  return listeners_;
}

void BondGUIService::AddPrice(Price<Bond>& price) {
  std::string id = price.GetProduct().GetProductId();
  prices_[id] = price;

  guiConnector_->Publish(price);  // publish data to the GUI output
}

std::chrono::milliseconds BondGUIService::GetThrottleInterval() const {
  return throttle_;
}

//*************************************************************************************************
// BondGUIConnector implementations
//*************************************************************************************************
BondGUIConnector::BondGUIConnector(BondPricingService* _service, const std::string& file_name) :
  bondPricingService_(_service), fileName_(file_name) {}

void BondGUIConnector::Subscribe(const char* filename, const bool& header) {
  // publish only
}

void BondGUIConnector::Publish(Price<Bond>& data) {
  // get data about the price
  std::string id = data.GetProduct().GetProductId();

  double mid = data.GetMid(), spread = data.GetBidOfferSpread();
  std::string bid_price = PriceToString(mid - 0.5 * spread);
  std::string offer_price = PriceToString(mid + 0.5 * spread);

  // open output file in append mode
  try {
    std::ofstream file;
    std::cout << PrintTimeStamp() << " Writing prices to 'gui.txt'..." << endl;
    file.open(fileName_, ios::app);
    // write into file
    file << PrintTimeStamp() << "," << id << "," << bid_price << "," << offer_price << std::endl;
    file.close();
  }
  catch (std::exception& e) {
    std::cout << "An error occurred: " << e.what() << endl;
  }
}

//*************************************************************************************************
// BondGUIConnector implementations
//*************************************************************************************************
BondGUIListener::BondGUIListener(BondGUIService* _service) :
  guiService_(_service), counter_(0)
{
  // get values for throttle
  throttle_ = guiService_->GetThrottleInterval();
  start_ = std::chrono::system_clock::now();
}

void BondGUIListener::ProcessAdd(Price<Bond>& data) {

  if (counter_ < 100 && std::chrono::system_clock::now() - start_ >= throttle_) {
    guiService_->AddPrice(data);
    // restart throttle and counter
    counter_++;
    start_ = std::chrono::system_clock::now();
  }
}

void BondGUIListener::ProcessRemove(Price<Bond>& data) {
  // not implemented
}

void BondGUIListener::ProcessUpdate(Price<Bond>& data) {
  // not implemented
}

#endif // !BONDGUISERVICE_HPP
