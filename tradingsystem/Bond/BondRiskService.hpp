/**
* BondRiskService.hpp
*
* Derives from RiskService to handle bonds
*
* @author: Gabo Bernardino
*/

#ifndef BONDRISKSERVICE_HPP
#define BONDRISKSERVICE_HPP

#include <unordered_map>
#include "../riskservice.hpp"
#include "../products.hpp"
#include "../utils.hpp"


/**
* Risk service class specialized for bonds;
* stores a vector of listeners and a map of strings -> risk info
* and a map of strings (sector names) -> sector risk info
* 
* Gets data from listener on BondPositionService and communicates it
* to Historical Data Listeners
*/
class BondRiskService : public RiskService<Bond> {
private:
  std::vector<ServiceListener<PV01<Bond>>*> listeners_;
  std::unordered_map<std::string, PV01<Bond>> pv_;  // keyed on product id
  std::unordered_map<std::string, PV01<BucketedSector<Bond>>> pv_buckets_;  // keyed on sector name
public:
  // ctor
  BondRiskService();

  // Get data on our service given a key
  virtual PV01<Bond>& GetData(std::string key) override;

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(PV01<Bond>& data) override;

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<PV01<Bond>>* listener) override;

  // Get all listeners on the Service.
  virtual const vector<ServiceListener<PV01<Bond>>*>& GetListeners() const override;

  // Add a position that the service will risk
  virtual void AddPosition(Position<Bond>& position) override;

  // Get the bucketed risk for the bucket sector
  virtual const PV01< BucketedSector<Bond> >& GetBucketedRisk(const BucketedSector<Bond>& sector) const override;
  virtual const PV01< BucketedSector<Bond> >& GetBucketedRisk(std::string& sectorName) const override;

  // Update the bucketed sector risk
  virtual void UpdateBucketedRisk(std::string& sector) override;
};

/**
* Risk listener specialized for bonds
* Sends positon information from Risk service to Historical Data
*/
class BondRiskListener : public ServiceListener<Position<Bond>> {
private:
  BondRiskService* bondRiskService_;

public:
  // ctor
  BondRiskListener(BondRiskService* _service);
  BondRiskListener() = default;

  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(Position<Bond>& data) override;

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(Position<Bond>& data) override;

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(Position<Bond>& data) override;
};

// ************************************************************************************************
// BondRiskService implementations
// ************************************************************************************************
BondRiskService::BondRiskService() {
  // initialize the PV01 map of individual bonds
  std::unordered_map <std::string, double> pv_base_map = PV_Map();  // map with the hardcoded PV01 values for each ticker
  pv_ = std::unordered_map<std::string, PV01<Bond>>();  // actual member
  PV01<Bond> pv_obj;

  for (auto [id, pv_value] : pv_base_map) {
    pv_obj = PV01<Bond>(MakeBond(id), pv_value, 0);
    pv_[id] = pv_obj;
  }

  // now initialize PV01 map for bucketed bonds
  std::unordered_map<std::string, std::vector<std::string>> pv_buckets_base = BucketMap();  // sector name -> cusips
  pv_buckets_ = std::unordered_map<std::string, PV01<BucketedSector<Bond>>>();  // actual member
  BucketedSector<Bond> bucket_obj;
  PV01<BucketedSector<Bond>> pv_of_bucket;

  for (auto [sector, products] : pv_buckets_base) {
    std::vector<Bond> bonds;
    for (auto ticker : products) {
      bonds.push_back(MakeBond(ticker));  // create the actual bond, not just the ticker
    }
    bucket_obj = BucketedSector<Bond>(bonds, sector);
    // initialize with everything 0, then we will call `UpdateBucketedRisk`
    pv_of_bucket = PV01<BucketedSector<Bond>>(bucket_obj, 0., 0);
    pv_buckets_[sector] = pv_of_bucket;
  }
}

PV01<Bond>& BondRiskService::GetData(std::string key) {
  return pv_[key];
}

void BondRiskService::OnMessage(PV01<Bond>& data) {
  // not implemented
}

void BondRiskService::AddListener(ServiceListener<PV01<Bond>>* listener) {
  listeners_.push_back(listener);
}

const vector<ServiceListener<PV01<Bond>>*>& BondRiskService::GetListeners() const {
  return listeners_;
}

void BondRiskService::AddPosition(Position<Bond>& position) {

  // get (current) PV object to update the exposure and send to listeners
  std::string id = position.GetProduct().GetProductId();
  PV01<Bond>& pv_obj = pv_[id];
  // modify quantity in PV object to communicate to listeners
  long long quantity = pv_obj.GetQuantity() + position.GetAggregatePosition();
  pv_obj.SetQuantity(quantity);

  std::cout << "New position: size is " << pv_[id].GetQuantity() << ", PV01 = " << pv_obj.GetPV01() << std::endl;

  std::cout << "Communicating risk of new position to listeners..." << endl;
  for (auto l : listeners_) {
    l->ProcessAdd(pv_obj);  // this is for the historical data listener
  }
}

const PV01< BucketedSector<Bond> >& BondRiskService::GetBucketedRisk(const BucketedSector<Bond>& sector) const {
  std::string name = sector.GetName();
  return pv_buckets_.at(name);
}

const PV01< BucketedSector<Bond> >& BondRiskService::GetBucketedRisk(std::string& sectorName) const {
  return pv_buckets_.at(sectorName);
}

void BondRiskService::UpdateBucketedRisk(std::string& sector) {
  
  // get bucketed sector object
  BucketedSector<Bond> bucket = pv_buckets_[sector].GetProduct();
  
  // loop thru bonds in bucketed sector and compute weighted PV
  vector<Bond> products = bucket.GetProducts();
  long long qnt = 0LL;  // needed to divide and get weighted avg
  double cumulative_pv01 = 0.;
  std::string id;
  PV01<Bond> position;

  for (Bond bond : products) {
    id = bond.GetProductId();
    position = pv_[id];
    qnt += position.GetQuantity();
    cumulative_pv01 += position.GetPV01() * position.GetQuantity();
  }
  // compute weighted pv01
  double pv01 = (qnt != 0) ? cumulative_pv01 / qnt : 0.;

  PV01<BucketedSector<Bond>> pv01bucket(bucket, pv01, qnt);
  pv_buckets_[sector] = pv01bucket;
}

// ************************************************************************************************
// BondRiskListener implementations
// ************************************************************************************************
BondRiskListener::BondRiskListener(BondRiskService* _service) :
  bondRiskService_(_service) {}

void BondRiskListener::ProcessAdd(Position<Bond>& data) {
  // not implemented
}

void BondRiskListener::ProcessRemove(Position<Bond>& data) {
  // not implemented
}

void BondRiskListener::ProcessUpdate(Position<Bond>& data) {
  bondRiskService_->AddPosition(data);
}


#endif // !BONDRISKSERVICE_HPP
