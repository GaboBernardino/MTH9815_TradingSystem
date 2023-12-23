/**
 * historicaldataservice.hpp
 * historicaldataservice.hpp
 *
 * @author Breman Thuraisingham
 * Defines the data types and Service for historical data.
 *
 * @author Breman Thuraisingham
 * @modified by Gabo Bernardino
 */
#ifndef HISTORICAL_DATA_SERVICE_HPP
#define HISTORICAL_DATA_SERVICE_HPP

#include <unordered_map>
#include <string>
#include "soa.hpp"

/**
 * Service for processing and persisting historical data to a persistent store.
 * Keyed on some persistent key.
 * We will write all the functionality in this class then derive
 * Type T is the data type to persist.
 */
template<typename T>
class HistoricalDataService : Service<std::string,T>
{
protected:
  std::vector<ServiceListener<T>*> listeners_;
  std::unordered_map<std::string, T> historicalData_;
  Connector<T>* historicalDataConnector_;  // special connector for type T

public:
  //ctor
  HistoricalDataService();
  void SetConnector(Connector<T>* _connector);

  // Get data on our service given a key
  virtual T& GetData(std::string key) override;

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(T& data) override;

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<T>* listener) override;

  // Get all listeners on the Service.
  virtual const vector<ServiceListener<T>*>& GetListeners() const override;
  
  // Persist data to a store
  void PersistData(string persistKey, T& data);
};

/**
* Historical data listener
* to be specialized and added to a service to receive data
*/
template <typename T>
class HistoricalDataListener : public ServiceListener<T> {
private:
  HistoricalDataService<T>* historicalDataService_;

public:
  // ctor
  HistoricalDataListener(HistoricalDataService<T>* _service);
  HistoricalDataListener() = default;

  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(T& data) override;

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(T& data) override;

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(T& data) override;
};


//*************************************************************************************************
// HistoricalDataService implementations
//*************************************************************************************************
template <typename T>
HistoricalDataService<T>::HistoricalDataService()
{
  historicalData_ = std::unordered_map<std::string, T>();
}

template <typename T>
void HistoricalDataService<T>::SetConnector(Connector<T>* _connector) {
  historicalDataConnector_ = _connector;
}

template <typename T>
T& HistoricalDataService<T>::GetData(std::string key) {
  return historicalData_[key];
}

template <typename T>
void HistoricalDataService<T>::OnMessage(T& data) {
  // not implemented for this service
}

template <typename T>
void HistoricalDataService<T>::AddListener(ServiceListener<T>* listener) {
  listeners_.push_back(listener);
}

template <typename T>
const vector<ServiceListener<T>*>& HistoricalDataService<T>::GetListeners() const {
  return listeners_;
}

template <typename T>
void HistoricalDataService<T>::PersistData(std::string persistKey, T& data) {
  // add new data to map
  historicalData_[persistKey] = data;
  // then publish via connector to persist in appropriate file
  historicalDataConnector_->Publish(data);
}

//*************************************************************************************************
// HistoricalDataListener implementations
//*************************************************************************************************
template <typename T>
HistoricalDataListener<T>::HistoricalDataListener(HistoricalDataService<T>* _service) :
  historicalDataService_(_service) {}

template <typename T>
void HistoricalDataListener<T>::ProcessAdd(T& data) {
  std::string persist_key = data.GetProduct().GetProductId();  // key is always product id
  historicalDataService_->PersistData(persist_key, data);
}

template <typename T>
void HistoricalDataListener<T>::ProcessRemove(T& data) {
  // not implemented
}

template <typename T>
void HistoricalDataListener<T>::ProcessUpdate(T& data) {
  // not implemented
}

#endif
