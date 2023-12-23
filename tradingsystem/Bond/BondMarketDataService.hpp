/**
* BondMarketDataService.hpp
*
* Derives from MarketDataService to handle bonds
*
* @author: Gabo Bernardino
*/

#ifndef BONDMARKETDATASERVICE_HPP
#define BONDMARKETDATASERVICE_HPP

#include "boost/algorithm/string.hpp"
#include "../marketdataservice.hpp"
#include "../utils.hpp"


class BondMarketDataService : public MarketDataService<Bond> {
private:
  std::vector<ServiceListener<OrderBook<Bond>>*> listeners_;
  std::unordered_map<std::string, OrderBook<Bond>> books_;  // keyed on product id

public:
  // ctor
  BondMarketDataService();

  // Get data on our service given a key
  virtual OrderBook<Bond>& GetData(std::string key) override;

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(OrderBook<Bond>& data) override;

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<OrderBook<Bond>>* listener) override;

  // Get all listeners on the Service.
  virtual const vector<ServiceListener<OrderBook<Bond>>*>& GetListeners() const override;

  // Get the best bid/offer order
  virtual const BidOffer& GetBestBidOffer(const string& productId) override;

  // Aggregate the order book
  virtual const OrderBook<Bond>& AggregateDepth(const string& productId) override;
};

/**
* Market data connector class specialized for bonds;
* Subscribe-only connector
*/
class BondMarketDataConnector : public Connector<OrderBook<Bond>> {
private:
  BondMarketDataService* marketDataService_;

public:
  BondMarketDataConnector(BondMarketDataService* _service);
  BondMarketDataConnector() = default;

  // Subscribe to a Service
  virtual void Subscribe(const char* filename, const bool& header = true) override;

  // Publish data - this one is subscribe only tho
  virtual void Publish(OrderBook<Bond>& data) override;
};


// ************************************************************************************************
// BondMarketDataService implementations
// ************************************************************************************************
BondMarketDataService::BondMarketDataService() {
  books_ = std::unordered_map<std::string, OrderBook<Bond>>();
}

OrderBook<Bond>& BondMarketDataService::GetData(std::string key) {
  return books_[key];
}

void BondMarketDataService::OnMessage(OrderBook<Bond>& data) {
  // add data to stored books
  std::string id = data.GetProduct().GetProductId();
  books_[id] = data;

  // communicate book to listeners
  cout << "Communicating order book to algo execution listeners..." << endl;
  for (auto l : listeners_) {
    l->ProcessAdd(data);
  }
}

void BondMarketDataService::AddListener(ServiceListener<OrderBook<Bond>>* listener) {
  listeners_.push_back(listener);
}

const vector<ServiceListener<OrderBook<Bond>>*>& BondMarketDataService::GetListeners() const {
  return listeners_;
}

// Get the best bid/offer order
const BidOffer& BondMarketDataService::GetBestBidOffer(const string& productId) {
  return books_[productId].GetBestBidOffer();
}

// Aggregate the order book
const OrderBook<Bond>& BondMarketDataService::AggregateDepth(const string& productId) {
  
  // aggregate different orders with same price
  OrderBook<Bond> book = books_[productId];
  std::vector<Order> bid_stack = book.GetBidStack(), offer_stack = book.GetOfferStack();

  // ue a map to merge orders with same price
  std::unordered_map<double, long> bid_map, offer_map;  // price -> quantity
  int n_bids = bid_stack.size(), n_offers = offer_stack.size();
  double price;
  Order curr_order;

  // BIDS
  for (int i = 0; i < n_bids; ++i) {
    curr_order = bid_stack[i];
    price = curr_order.GetPrice();

    if (bid_map.find(price) != bid_map.end())
      bid_map[price] += curr_order.GetQuantity();

    else bid_map[price] = curr_order.GetQuantity();;
  }
  // OFFERS
  for (int i = 0; i < n_offers; ++i) {
    curr_order = offer_stack[i];
    price = curr_order.GetPrice();

    if (offer_map.find(price) != offer_map.end())
      offer_map[price] += curr_order.GetQuantity();

    else offer_map[price] = curr_order.GetQuantity();;
  }

  // create the merged stacks
  vector<Order> new_bids, new_offers;
  for (auto const& [price, quantity] : bid_map) {
    new_bids.push_back(Order(price, quantity, BID));
  }
  for (auto const& [price, quantity] : offer_map) {
    new_offers.push_back(Order(price, quantity, OFFER));
  }

  Bond bond = MakeBond(productId);
  OrderBook<Bond> new_book(bond, new_bids, new_offers);
  books_[productId] = new_book;
  return books_[productId];
}


// ************************************************************************************************
// BondMarketDataConnector implementations
// ************************************************************************************************
BondMarketDataConnector::BondMarketDataConnector(BondMarketDataService* _service) :
  marketDataService_(_service) {}

void BondMarketDataConnector::Subscribe(const char* filename, const bool& header) {
  std::string line;
  std::vector<std::string> row;
  Bond bond;
  double order_price;
  long order_size;
  PricingSide side;
  std::vector<Order> bid_stack, offer_stack;

  long orders_per_bond = 10L;  // HARDCODED, instructions say 5 bids, 5 offers
  long counter = 0L;

  try {
    std::ifstream in(filename);
    if (header) std::getline(in, line);  // skip header

    while (std::getline(in, line)) {
      counter++;  // keep count of orders

      // get bond id, price, size and side
      boost::algorithm::split(row, line, boost::algorithm::is_any_of(","));

      // some items need preprocessing
      // compute price
      order_price = StringToPrice(row[1]);
      // trade size:
      order_size = std::stol(row[2]);
      // trade side:
      boost::algorithm::trim(row[3]);
      side = (row[3] == "BID") ? BID : OFFER;

      // create order object and add it to correct stack
      Order order(order_price, order_size, side);
      switch (side){
      case BID:
        bid_stack.push_back(order);
      case OFFER:
        offer_stack.push_back(order);
      default:
        break;
      }

      // we create a full order book after going through 10 lines of `mkt_data.txt`
      if (counter % orders_per_bond == 0) {
        // create a bond object from the id:
        bond = MakeBond(row[0]);
        std::cout << std::endl << PrintTimeStamp() << std::endl << "Bond: " << bond << std::endl;

        OrderBook<Bond> book_obj(bond, bid_stack, offer_stack);
        // communicate book to service
        marketDataService_->OnMessage(book_obj);

        // start over
        counter = 0L;
        bid_stack = std::vector<Order>();
        offer_stack = std::vector<Order>();
      }
    }
  }
  catch (std::exception& e) {
    std::cout << "An error occurred: " << e.what() << std::endl;
  }
}

void BondMarketDataConnector::Publish(OrderBook<Bond>& data) {
  // subscribe only
}


#endif // !BONDMARKETDATASERVICE_HPP
