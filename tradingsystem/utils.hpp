/**
* helper functions for the trading system
* 
* @author: Gabo Bernardino
*/

#ifndef UTILS_HPP
#define UTILS_HPP

#include <unordered_map>
#include <functional>
#include <chrono>
#include "boost/algorithm/string.hpp"
#include "products.hpp"
#include "marketdataservice.hpp"

// ************************************************************************************************
// Function to create a bond object object based on its CUSIP
// ************************************************************************************************
Bond MakeBond(const std::string& cusip) {
  
  Bond bond;

  if (cusip == "91282CJL6") bond = Bond(cusip, CUSIP, "US2Y", 0.04875, boost::gregorian::from_string("2025/11/30"));
  if (cusip == "91282CJK8") bond = Bond(cusip, CUSIP, "US3Y", 0.04625, boost::gregorian::from_string("2026/11/15"));
  if (cusip == "91282CJN2") bond = Bond(cusip, CUSIP, "US5Y", 0.04375, boost::gregorian::from_string("2028/11/30"));
  if (cusip == "91282CJM4") bond = Bond(cusip, CUSIP, "US7Y", 0.04375, boost::gregorian::from_string("2030/11/30"));
  if (cusip == "91282CJJ1") bond = Bond(cusip, CUSIP, "US10Y", 0.045, boost::gregorian::from_string("2033/11/15"));
  if (cusip == "912810TW8") bond = Bond(cusip, CUSIP, "US20Y", 0.0475, boost::gregorian::from_string("2043/11/15"));
  if (cusip == "912810TV0") bond = Bond(cusip, CUSIP, "US30Y", 0.0475, boost::gregorian::from_string("2053/11/15"));

  return bond;
}


std::unordered_map<string, double> PV_Map() {
  
  std::unordered_map<string, double> pv_map;

  pv_map["91282CJL6"] = 0.01;
  pv_map["91282CJK8"] = 0.02;
  pv_map["91282CJN2"] = 0.03;
  pv_map["91282CJM4"] = 0.04;
  pv_map["91282CJJ1"] = 0.05;
  pv_map["912810TW8"] = 0.06;
  pv_map["912810TV0"] = 0.07;

  return pv_map;
}

std::unordered_map<std::string, std::vector<std::string>> BucketMap() {
  
  // front end: 2Y and 3Y
  std::vector<std::string> frontEnd{ "91282CJL6", "91282CJK8" };
  // belly: 5Y, 7Y and 10Y
  std::vector<std::string> belly{ "91282CJN2", "91282CJM4", "91282CJJ1"};
  // long end: 20Y and 30Y
  std::vector<std::string> longEnd{ "912810TW8", "912810TV0" };

  std::unordered_map<std::string, std::vector<std::string>> map;

  map["FrontEnd"] = frontEnd;
  map["Belly"] = belly;
  map["LongEnd"] = longEnd;

  return map;
}

// ************************************************************************************************
// Functions to convert price to and from fractional (256th)
// ************************************************************************************************
double StringToPrice(const std::string& s_price) {

  std::vector<std::string> tokens;
  boost::algorithm::split(tokens, s_price, boost::algorithm::is_any_of("-"));
  // this splits the integer and the 'xyz' part;
  std::string integer_str = tokens[0];
  std::string thirtysec = tokens[1].substr(0, 2);
  std::string eighth = tokens[1].substr(2, 3);

  if (eighth == "+") eighth = "4";

  double integer_d = std::stod(integer_str);
  double decimal32 = std::stod(thirtysec) / 32.; 
  double decimal8 = std::stod(eighth) / 256.;

  return integer_d + decimal32 + decimal8;
}

std::string PriceToString(const double& d_price) {

  int int_price = std::floor(d_price);
  double remainder = d_price - int_price;

  std::string output = std::to_string(int_price) + "-";

  double thirtysec_delta = remainder / (1. / 32.);
  int thirtysec = std::floor(thirtysec_delta);
  if (thirtysec < 10) output += "0";
  output += std::to_string(thirtysec);

  int eighth = static_cast<int>((thirtysec_delta - thirtysec) / (1. / 8.));
  if (eighth == 4) output += "+";
  else output += std::to_string(eighth);

  return output;
}

// ************************************************************************************************
// Function to find the best bid and best offer in an order book
// ************************************************************************************************
Order find_best_order(const vector<Order>& order_stack, const PricingSide& side) {
  
  // comparator function depending on side:
  function<bool(double, double)> comparator;
  if (side == BID) {
    comparator = [](double a, double b) {return (a > b); };
  }
  else {
    comparator = [](double a, double b) {return (a < b); };
  }

  Order best_order = order_stack[0];  // current best order
  int n_orders = order_stack.size();

  for (int i = 1; i < n_orders; ++i) {
    if (comparator(order_stack[i].GetPrice(), best_order.GetPrice()))
      // if price is better, update the current best order
      best_order = order_stack[i];
  }

  return best_order;
}

//*************************************************************************************************
// Function to print the current timestamp with millisecond precision
//*************************************************************************************************
std::string PrintTimeStamp() {
  auto current_time = std::chrono::system_clock::now();
  auto millisec = std::chrono::duration_cast<std::chrono::milliseconds>
    (current_time.time_since_epoch()).count() % 1000;
  // extract time in human-readable format
  auto timeT = std::chrono::system_clock::to_time_t(current_time);
  auto local_time = *std::localtime(&timeT);
  // create the string
  std::ostringstream oss;
  oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S.")
    << std::setw(3) << std::setfill('0') << millisec;
  return oss.str();
}

#endif // !UTILS_HPP
