/**
* BondInquiryService.hpp
*
* Derives from InquiryService to handle bonds
*
* @author: Gabo Bernardino
*/

#ifndef BONDINQUIRYSERVICE_HPP
#define BONDINQUIRYSERVICE_HPP

#include "boost/algorithm/string.hpp"
#include "../utils.hpp"
#include "../inquiryservice.hpp"
#include "../products.hpp"

/**
 * Bond inquiry service specialized for bonds;
 * stores a vector of listeners and a map of strings -> inquiry
 * also syores a pointer to a connector which  it uses to publish
 * quotes for received inquiries
 * 
 * Gets data from `inquiries.txt` via a connector and sends it back
 * to the connector to publish
 * Also communicates the data to InquiryListeners and HistoricalData listeners
 */
class BondInquiryService : public InquiryService<Bond> {
private:
  std::vector<ServiceListener<Inquiry<Bond>>*> listeners_;
  std::unordered_map<std::string, Inquiry<Bond>> inquiries_;

  Connector<Inquiry<Bond>>* bondInquiryConnector_;

public:
  //ctor
  BondInquiryService();
  void SetConnector(Connector<Inquiry<Bond>>* _connector);

  // Get data on our service given a key
  virtual Inquiry<Bond>& GetData(std::string key) override;

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(Inquiry<Bond>& data) override;

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<Inquiry<Bond>>* listener) override;

  // Get all listeners on the Service.
  virtual const vector<ServiceListener<Inquiry<Bond>>*>& GetListeners() const override;

  // Send a quote back to the client
  virtual void SendQuote(const string& inquiryId, double price) override;

  // Reject an inquiry from the client
  virtual void RejectInquiry(const string& inquiryId) override;
};

/**
* Inquiry connector class specialized for bonds;
* Reads from `inquiries.txt` and sends the Inquiry object to the service
* Publishes received inquiries after quoting them
*/
class BondInquiryConnector : public Connector<Inquiry<Bond>> {
private:
  BondInquiryService* bondInquiryService_;
public:
  BondInquiryConnector(BondInquiryService* _service);
  BondInquiryConnector() = default;

  // Subscribe to a Service
  virtual void Subscribe(const char* filename, const bool& header = true) override;

  // Publish data
  virtual void Publish(Inquiry<Bond>& data) override;
};


/**
* Inquiry listener specialized for bonds
* Sends back a quote of 100 when the inquiry is in `RECEIVED` state
*/
class BondInquiryListener : public ServiceListener<Inquiry<Bond>> {
private:
  BondInquiryService* bondInquiryService_;

public:
  // ctor
  BondInquiryListener(BondInquiryService* _service);
  BondInquiryListener() = default;

  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(Inquiry<Bond>& data) override;

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(Inquiry<Bond>& data) override;

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(Inquiry<Bond>& data) override;
};

//*************************************************************************************************
// BondInquiryService implementations
//*************************************************************************************************
BondInquiryService::BondInquiryService() {
  inquiries_ = std::unordered_map<std::string, Inquiry<Bond>>();
}

void BondInquiryService::SetConnector(Connector<Inquiry<Bond>>* _connector) {
  bondInquiryConnector_ = _connector;
}

Inquiry<Bond>& BondInquiryService::GetData(std::string key) {
  return inquiries_[key];
}

void BondInquiryService::OnMessage(Inquiry<Bond>& data) {
  // add inquiry to map
  std::string id = data.GetInquiryId();
  inquiries_[id] = data;

  for (auto l : listeners_) {
    l->ProcessAdd(data);  // this is for the historical data listener
    l->ProcessUpdate(data);  // the listener will send back a quote if it is received
  }
}

void BondInquiryService::AddListener(ServiceListener<Inquiry<Bond>>* listener) {
  listeners_.push_back(listener);
}

const vector<ServiceListener<Inquiry<Bond>>*>& BondInquiryService::GetListeners() const {
  return listeners_;
}

void BondInquiryService::SendQuote(const string& inquiryId, double price) {
  // modify quote and send back to the connector
  Inquiry<Bond>& inquiry = inquiries_[inquiryId];  // get a reference so we can modify
  inquiry.SetPrice(price);
  
  std::cout << "Modified price of inquiry " << inquiry.GetInquiryId();
  std::cout << "; publishing the quoted inquiry" << std::endl;
  bondInquiryConnector_->Publish(inquiry);
}

void BondInquiryService::RejectInquiry(const string& inquiryId) {
  // modify state and send back to the connector
  Inquiry<Bond>& inquiry = inquiries_[inquiryId];  // get a reference so we can modify
  inquiry.SetState(REJECTED);

  std::cout << "Rejected inquiry " << inquiry.GetInquiryId();
  std::cout << "; publishing the rejected inquiry" << std::endl;
  bondInquiryConnector_->Publish(inquiry);
}

// ************************************************************************************************
// BondInquiryConnector implementations
// ************************************************************************************************
BondInquiryConnector::BondInquiryConnector(BondInquiryService* _service) :
  bondInquiryService_(_service) {}

void BondInquiryConnector::Subscribe(const char* filename, const bool& header) {
  std::string line;
  std::vector<std::string> row;  // to store output of string splitting
  std::string inquiry_id;
  Bond bond;  // bond object for the bond being inquired
  Side side; // buy or sell
  long qnt;
  double price;
  InquiryState state;  // received -> quoted -> done

  try {
    std::ifstream in(filename);
    if (header) std::getline(in, line);  // skip header

    while (std::getline(in, line)) {
      // preprocess line string
      boost::algorithm::trim(line);
      // get id, bond id, side, quntity, price and status
      boost::algorithm::split(row, line, boost::algorithm::is_any_of(","));

      // get inquiry information
      inquiry_id = row[0];  // THIS IS WHAT THE SERVICE IS KEYED ON!
      bond = MakeBond(row[1]);
      side = (row[2] == "SELL") ? SELL : BUY;
      qnt = std::stol(row[3]);
      price = StringToPrice(row[4]);
      if (row[5] == "RECEIVED")
        state = RECEIVED;
      else if (row[5] == "QUOTED")
        state = QUOTED;
      else if (row[5] == "DONE")
        state = DONE;
      else if (row[5] == "REJECTED")
        state = REJECTED;
      else if (row[5] == "CUSTOMER_REJECTED")
        state = CUSTOMER_REJECTED;

      // create inquiry object and send it to service
      std::cout << std::endl << PrintTimeStamp() << std::endl;
      Inquiry<Bond> inquiry_obj(inquiry_id, bond, side, qnt, price, state);
      bondInquiryService_->OnMessage(inquiry_obj);
    }    
  }
  catch (std::exception& e) {
    std::cout << "An error occurred: " << e.what() << std::endl;
  }
}

void BondInquiryConnector::Publish(Inquiry<Bond>& data) {
  if (data.GetState() == RECEIVED) {
    // transition the inquiry to the QUOTED state and send it back to the BondInquiryService
    data.SetState(QUOTED);
    std::cout << "Connector sending back quoted inquiry " << data.GetInquiryId() << std::endl;
    bondInquiryService_->OnMessage(data);  // new price was already set by the service
    // immediately send update with DONE state
    data.SetState(DONE);
    std::cout << "Connector updating inquiry " << data.GetInquiryId() << " to done." << std::endl;
    bondInquiryService_->OnMessage(data);
  }
  else {
    std::cout << "Inquiry was rejected" << std::endl;
  }
}

//*************************************************************************************************
// BondInquiryListener implementations
//*************************************************************************************************
BondInquiryListener::BondInquiryListener(BondInquiryService* _service) :
  bondInquiryService_(_service) {}

void BondInquiryListener::ProcessAdd(Inquiry<Bond>& data) {
  // not implemented
}

void BondInquiryListener::ProcessRemove(Inquiry<Bond>& data) {
  // not implemented
}

void BondInquiryListener::ProcessUpdate(Inquiry<Bond>& data) {
  // You should register a ServiceListener on the BondInquiryService
  // which sends back a quote of 100 when the inquiry is in the RECEIVED state
  if (data.GetState() == RECEIVED) {
    std::cout << "Received inquiry, sending back a quote of 100" << std::endl;
    bondInquiryService_->SendQuote(data.GetInquiryId(), 100.);
  }
}

#endif // !BONDINQUIRYSERVICE_HPP
