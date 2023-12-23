/**
 * pricingservice.hpp
 * Defines the data types and Service for internal prices.
 *
 * @author Breman Thuraisingham
 */
#ifndef PRICING_SERVICE_HPP
#define PRICING_SERVICE_HPP

#include <string>
#include <iostream>
#include <tuple>
#include "soa.hpp"

/**
 * A price object consisting of mid and bid/offer spread.
 * Type T is the product type.
 */
template<typename T>
class Price
{

public:

  // ctor for a price
  Price(const T &_product, double _mid, double _bidOfferSpread);
  Price() = default;

  // Get the product
  const T& GetProduct() const;

  // Get the mid price
  double GetMid() const;

  // Get the bid/offer spread around the mid
  double GetBidOfferSpread() const;

  // Turn the attribute prices into strings for output
  void ToString() const;

private:
  T product;
  double mid;
  double bidOfferSpread;

};

/**
 * Pricing Service managing mid prices and bid/offers.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class PricingService : public Service<string,Price <T> >
{
};

/**
* Same thing for Pricing Connector
*/
template<typename T>
class PricingConnector : public Connector<Price<T>>
{
};

template<typename T>
Price<T>::Price(const T &_product, double _mid, double _bidOfferSpread) :
  product(_product)
{
  mid = _mid;
  bidOfferSpread = _bidOfferSpread;
}

template<typename T>
const T& Price<T>::GetProduct() const
{
  return product;
}

template<typename T>
double Price<T>::GetMid() const
{
  return mid;
}

template<typename T>
double Price<T>::GetBidOfferSpread() const
{
  return bidOfferSpread;
}

template<typename T>
void Price<T>::ToString() const {
  // std::string prod = product.GetProductId();
  // std::string mid_price_str = PriceToString(mid), spread_str = PriceToString(bidOfferSpread);

  std::cout << "Price<T>::ToString() HAS NOT YET BEEN IMPLEMENTED - change the return type when u do" << std::endl;

  // return make_tuple(prod, mid_price_str, spread_str);
}

#endif
