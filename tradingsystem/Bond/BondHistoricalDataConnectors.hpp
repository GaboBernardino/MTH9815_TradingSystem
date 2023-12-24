/**
* BondHistoricalDataService.hpp
*
* Derives from HistoricalDataService to handle bonds
*
* @author: Gabo Bernardino
*/

#ifndef BONDHISTORICALDATACONNECTORS_HPP
#define BONDHISTORICALDATACONNECTORS_HPP

#include <fstream>
#include "../historicaldataservice.hpp"
#include "../utils.hpp"

/**
* Historical data connector specialized for bond positions 
*/
class BondHistoricalPositionConnector : public Connector<Position<Bond>> {

public:
  // ctor
  BondHistoricalPositionConnector() = default;

  // Subscribe to a Service - this one is publish only tho
  virtual void Subscribe(const char* filename, const bool& header = false) override;
  
  // Publish data
  virtual void Publish(Position<Bond>& data) override;
};

/**
* Historical data connector specialized for bond risk
*/
class BondHistoricalRiskConnector : public Connector<PV01<Bond>> {

private:
  BondRiskService* bondRiskService_;

  // find the bucket the bond belongs to
  std::string _findBucket(const Bond& bond);

public:
  // ctor
  BondHistoricalRiskConnector(BondRiskService* _service);
  BondHistoricalRiskConnector() = default;

  // Subscribe to a Service - this one is publish only tho
  virtual void Subscribe(const char* filename, const bool& header = false) override;

  // Publish data
  virtual void Publish(PV01<Bond>& data) override;

  // Publish data for Bucketed risk
  void Publish(PV01<BucketedSector<Bond>>& data);
};

/**
* Historical data connector specialized for bond execution
*/
class BondHistoricalExecutionConnector : public Connector<ExecutionOrder<Bond>> {

public:
  // ctor
  BondHistoricalExecutionConnector() = default;

  // Subscribe to a Service - this one is publish only tho
  virtual void Subscribe(const char* filename, const bool& header = false) override;

  // Publish data
  virtual void Publish(ExecutionOrder<Bond>& data) override;
};

/**
* Historical data connector specialized for bond price streaming
*/
class BondHistoricalStreamingConnector : public Connector<PriceStream<Bond>> {

public:
  // ctor
  BondHistoricalStreamingConnector() = default;

  // Subscribe to a Service - this one is publish only tho
  virtual void Subscribe(const char* filename, const bool& header = false) override;

  // Publish data
  virtual void Publish(PriceStream<Bond>& data) override;
};

/**
* Historical data connector specialized for bond inquiries
*/
class BondHistoricalInquiryConnector : public Connector<Inquiry<Bond>> {

public:
  // ctor
  BondHistoricalInquiryConnector() = default;

  // Subscribe to a Service - this one is publish only tho
  virtual void Subscribe(const char* filename, const bool& header = false) override;

  // Publish data
  virtual void Publish(Inquiry<Bond>& data) override;
};

// ************************************************************************************************
// Implementations
// ************************************************************************************************

// POSITION
void BondHistoricalPositionConnector::Subscribe(const char* filename, const bool& header) {
  // they all get data from listeners
}

void BondHistoricalPositionConnector::Publish(Position<Bond>& data) {
  // extract information we need to output
  std::string bond_id = data.GetProduct().GetProductId();

  std::string book1 = "TRSY1", book2 = "TRSY2", book3 = "TRSY3";

  long pos1_long = data.GetPosition(book1), pos2_long = data.GetPosition(book2), pos3_long = data.GetPosition(book3);
  long aggregate_long = data.GetAggregatePosition();

  std::string pos1 = std::to_string(pos1_long), pos2 = std::to_string(pos2_long), pos3 = std::to_string(pos3_long);
  std::string aggregate = std::to_string(aggregate_long);

  // open file in append mode
  try {
    std::ofstream file;
    file.open("Data/positions.txt", ios::app);
    std::cout << PrintTimeStamp() << " Writing positions into 'positions.txt'..." << endl;
    // write into file
    file << PrintTimeStamp();
    file << "," << bond_id << ",TRSY1," << pos1 << ",TRSY2," << pos2 << ",TRSY3," << pos3;
    file << ",AGGREGATE," << aggregate << endl;
    file.close();
  }
  catch (std::exception& e) {
    std::cout << "An error occurred: " << e.what() << endl;
  }
}

// RISK
BondHistoricalRiskConnector::BondHistoricalRiskConnector(BondRiskService* _service) :
  bondRiskService_(_service) {}

void BondHistoricalRiskConnector::Subscribe(const char* filename, const bool& header) {
  // they all get data from listeners
}

void BondHistoricalRiskConnector::Publish(PV01<Bond>& data) {
  // extract information we need to output
  std::string bond_id = data.GetProduct().GetProductId();

  std::string pv01 = std::to_string(data.GetPV01());
  std::string qnt = std::to_string(data.GetQuantity());

  // open file in append mode
  try {
    std::ofstream file;
    file.open("Data/risk.txt", ios::app);
    std::cout << PrintTimeStamp() << " Writing PV01 into 'risk.txt'..." << endl;
    // write into file
    file << PrintTimeStamp();
    file << "," << bond_id << "," << pv01 << "," << qnt << endl;
    file.close();
  }
  catch (std::exception& e) {
    std::cout << "An error occurred: " << e.what() << endl;
  }

  // update bucketed risk and publish that as well
  std::string sector = _findBucket(data.GetProduct());
  bondRiskService_->UpdateBucketedRisk(sector);
  PV01<BucketedSector<Bond>> pv01bucket = bondRiskService_->GetBucketedRisk(sector);
  this->Publish(pv01bucket);
}

void BondHistoricalRiskConnector::Publish(PV01<BucketedSector<Bond>>& data) {
  // extract information we need to output
  std::string sector_id = data.GetProduct().GetName();

  std::string pv01 = std::to_string(data.GetPV01());
  std::string qnt = std::to_string(data.GetQuantity());

  // open file in append mode
  try {
    std::ofstream file;
    file.open("Data/risk.txt", ios::app);
    std::cout << PrintTimeStamp() << " Writing bucketed PV01 into 'risk.txt'..." << endl;
    // write into file
    file << PrintTimeStamp();
    file << "," << sector_id << "," << pv01 << "," << qnt << endl;
    file.close();
  }
  catch (std::exception& e) {
    std::cout << "An error occurred: " << e.what() << endl;
  }
}

std::string BondHistoricalRiskConnector::_findBucket(const Bond& bond) {
  // map sector name to vector of tickers in that sector
  std::unordered_map<std::string, std::vector<std::string>> pv_tickers = BucketMap();
  // find key (sector) corresponding to input bond
  std::string id = bond.GetProductId();
  for (const auto& [sector, tickers] : pv_tickers) {
    if (std::find(tickers.begin(), tickers.end(), id) != tickers.end()) {
      // found it!
      return sector;
    }
  }

  return "****************** ERROR ******************";
}

// EXECUTION
void BondHistoricalExecutionConnector::Subscribe(const char* filename, const bool& header) {
  // they all get data from listeners
}

void BondHistoricalExecutionConnector::Publish(ExecutionOrder<Bond>& data) {
  // extract information we need to output
  std::string bond_id = data.GetProduct().GetProductId();
  std::string order_id = data.GetOrderId();
  std::string side = (data.GetSide() == BID) ? "BID" : "OFFER";
  std::string is_child = (data.IsChildOrder()) ? "YES" : "NO";

  double price_dbl = data.GetPrice();
  std::string price = PriceToString(price_dbl);

  long visible = data.GetVisibleQuantity(), hidden = data.GetHiddenQuantity();
  std::string visible_qnt = std::to_string(visible), hidden_qnt = std::to_string(hidden);

  std::string order_type;
  OrderType type = data.GetOrderType();
  if (type == FOK) order_type = "FOK";
  else if (type == IOC) order_type = "IOC";
  else if (type == MARKET) order_type = "MARKET";
  else if (type == LIMIT) order_type = "LIMIT";
  else if (type == STOP) order_type = "STOP";

  // open file in append mode
  try {
    std::ofstream file;
    file.open("Data/executions.txt", ios::app);
    std::cout << PrintTimeStamp() << " Writing execution order into 'executions.txt'..." << endl;
    // write into file
    file << PrintTimeStamp();
    file << "," << bond_id << "," << side << "," << order_id << "," << order_type << ",";
    file << price << "," << visible_qnt << "," << hidden_qnt << "," << is_child << endl;
    file.close();
  }
  catch (std::exception& e) {
    std::cout << "An error occurred: " << e.what() << endl;
  }
}

// STREAMING
void BondHistoricalStreamingConnector::Subscribe(const char* filename, const bool& header) {
  // they all get data from listeners
}

void BondHistoricalStreamingConnector::Publish(PriceStream<Bond>& data) {
  // extract information we need to output
  std::string bond_id = data.GetProduct().GetProductId();

  PriceStreamOrder bidOrder = data.GetBidOrder(), offerOrder = data.GetOfferOrder();
  
  double bid_price_dbl = bidOrder.GetPrice(), offer_price_dbl = offerOrder.GetPrice();
  long bid_visible_long = bidOrder.GetVisibleQuantity(), bid_hidden_long = bidOrder.GetHiddenQuantity();
  long offer_visible_long = offerOrder.GetVisibleQuantity(), offer_hidden_long = offerOrder.GetHiddenQuantity();

  std::string bid_price = PriceToString(bid_price_dbl), offer_price = PriceToString(offer_price_dbl);
  std::string bid_visible = std::to_string(bid_visible_long), bid_hidden = std::to_string(bid_hidden_long);
  std::string offer_visible = std::to_string(offer_visible_long), offer_hidden = std::to_string(offer_hidden_long);

  // open file in append mode
  try {
    std::ofstream file;
    file.open("Data/streaming.txt", ios::app);
    std::cout << PrintTimeStamp() << " Writing price stream into 'streaming.txt'..." << endl;
    // write into file
    file << PrintTimeStamp();
    file << "," << bond_id << ",BID," << bid_price << "," << bid_visible << "," << bid_hidden << endl;
    file << PrintTimeStamp();
    file << "," << bond_id << ",OFFER," << offer_price << "," << offer_visible << "," << offer_hidden << endl;
    file.close();
  }
  catch (std::exception& e) {
    std::cout << "An error occurred: " << e.what() << endl;
  }
}

// INQUIRY
void BondHistoricalInquiryConnector::Subscribe(const char* filename, const bool& header) {
  // they all get data from listeners
}

void BondHistoricalInquiryConnector::Publish(Inquiry<Bond>& data) {
  // extract information we need to output
  std::string inquiry_id = data.GetInquiryId();
  std::string bond_id = data.GetProduct().GetProductId();
  std::string side = (data.GetSide() == SELL) ? "SELL" : "BUY";

  double price_dbl = data.GetPrice();
  std::string price = PriceToString(price_dbl);

  long qnt_long = data.GetQuantity();
  std::string qnt = std::to_string(qnt_long);

  std::string state;
  InquiryState inquiry_state = data.GetState();
  if (inquiry_state == RECEIVED) state = "RECEIVED";
  else if (inquiry_state == QUOTED) state = "QUOTED";
  else if (inquiry_state == DONE) state = "DONE";
  else if (inquiry_state == REJECTED) state = "REJECTED";
  else if (inquiry_state == CUSTOMER_REJECTED) state = "CUSTOMER_REJECTED";

  // open file in append mode
  try {
    std::ofstream file;
    file.open("Data/allinquiries.txt", ios::app);
    std::cout << PrintTimeStamp() << " Writing inquiries into 'allinquiries.txt'..." << endl;
    // write into file
    file << PrintTimeStamp();
    file << "," << inquiry_id << "," << bond_id << "," << side << "," << qnt << ",";
    file << price << "," << state << endl;
    file.close();
  }
  catch (std::exception& e) {
    std::cout << "An error occurred: " << e.what() << endl;
  }
}

#endif // !BONDHISTORICALDATASERVICE_HPP
