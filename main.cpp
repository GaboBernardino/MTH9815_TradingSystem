// Gabo Bernardino - main file for MTH9815 final project

#include <iostream>
#include <iomanip>
#include "tradingsystem/Bond/BondPricingService.hpp"
#include "tradingsystem/Bond/BondTradeBookingService.hpp"
#include "tradingsystem/Bond/BondPositionService.hpp"
#include "tradingsystem/Bond/BondRiskService.hpp"
#include "tradingsystem/Bond/BondMarketDataService.hpp"
#include "tradingsystem/Bond/BondAlgoExecutionService.hpp"
#include "tradingsystem/Bond/BondAlgoStreamingService.hpp"
#include "tradingsystem/Bond/BondGUIService.hpp"
#include "tradingsystem/Bond/BondExecutionService.hpp"
#include "tradingsystem/Bond/BondStreamingService.hpp"
#include "tradingsystem/Bond/BondInquiryService.hpp"
#include "tradingsystem/Bond/BondHistoricalDataConnectors.hpp"

int main() {

  std::cout << std::fixed << std::setprecision(8);

  std::cout << PrintTimeStamp() << " Program starting" << std::endl;

  std::cout << PrintTimeStamp() << " Creating services" << std::endl;

  BondPricingService price_service;
  BondAlgoStreamingService algo_stream_service;
  BondStreamingService stream_service;
  BondGUIService gui_service(300);
  HistoricalDataService<PriceStream<Bond>> stream_historical_service;

  BondTradeBookingService trade_service;
  BondPositionService pos_service;
  BondRiskService risk_service;
  BondExecutionService execution_service;
  HistoricalDataService<ExecutionOrder<Bond>> execution_history_service;
  HistoricalDataService<PV01<Bond>> risk_history_service;
  HistoricalDataService<Position<Bond>> position_history_service;

  BondMarketDataService mkt_service;
  BondAlgoExecutionService algo_service;

  BondInquiryService inquiry_service;
  HistoricalDataService<Inquiry<Bond>> inquiry_historical_service;

  std::cout << PrintTimeStamp() << " Services created" << std::endl;

  std::cout << PrintTimeStamp() << " Linking services" << std::endl;

  BondGUIListener gui_listener(&gui_service);
  price_service.AddListener(&gui_listener);

  BondStreamingListener stream_listener(&stream_service);
  algo_stream_service.AddListener(&stream_listener);
  BondAlgoStreamingListener algo_stream_listener(&algo_stream_service);
  price_service.AddListener(&algo_stream_listener);

  HistoricalDataListener<PriceStream<Bond>> stream_hist_listener(&stream_historical_service);
  stream_service.AddListener(&stream_hist_listener);

  BondRiskListener risk_listener(&risk_service);
  pos_service.AddListener(&risk_listener);
  BondPositionListener pos_listener(&pos_service);
  trade_service.AddListener(&pos_listener);
  BondTradeBookingListener trade_listener(&trade_service);
  execution_service.AddListener(&trade_listener);
  BondExecutionListener execution_listener(&execution_service);
  algo_service.AddListener(&execution_listener);
  BondAlgoExecutionListener algo_listener(&algo_service);
  mkt_service.AddListener(&algo_listener);

  HistoricalDataListener<ExecutionOrder<Bond>> exec_hist_listener(&execution_history_service);
  execution_service.AddListener(&exec_hist_listener);
  HistoricalDataListener<PV01<Bond>> risk_hist_listener(&risk_history_service);
  risk_service.AddListener(&risk_hist_listener);
  HistoricalDataListener<Position<Bond>> position_hist_listener(&position_history_service);
  pos_service.AddListener(&position_hist_listener);

  BondInquiryListener inquiry_listener(&inquiry_service);
  inquiry_service.AddListener(&inquiry_listener);
  HistoricalDataListener<Inquiry<Bond>> inquiry_hist_listener(&inquiry_historical_service);
  inquiry_service.AddListener(&inquiry_hist_listener);

  std::cout << PrintTimeStamp() << " Services linked" << std::endl;

  std::cout << "*************** Pricing and GUI Services ***************" << std::endl << std::endl;
  
  std::cout << PrintTimeStamp() << " Creating connector for price data" << std::endl;
  // connector for historical stream data
  BondHistoricalStreamingConnector stream_history_conn;
  stream_historical_service.SetConnector(&stream_history_conn);
  
  BondGUIConnector gui_connector(&price_service, "Data/gui.txt");
  gui_service.SetConnector(&gui_connector);

  BondPricingConnector price_connector(&price_service);
  price_connector.Subscribe("Data/prices.txt", false);
  std::cout << PrintTimeStamp() << " Created connector for price data" << std::endl;

  std::cout << "\n*************** Trade and Risk Services ***************" << endl << std::endl;
  
  std::cout << PrintTimeStamp() << " Creating connector for trade data" << std::endl;
  // connector for historical execution, position and risk data
  BondHistoricalExecutionConnector exec_history_conn;
  execution_history_service.SetConnector(&exec_history_conn);
  BondHistoricalPositionConnector pos_history_conn;
  position_history_service.SetConnector(&pos_history_conn);
  BondHistoricalRiskConnector risk_history_conn(&risk_service);
  risk_history_service.SetConnector(&risk_history_conn);

  BondTradeBookingConnector trade_connector(&trade_service);
  trade_connector.Subscribe("Data/trades.txt", false);
  std::cout << PrintTimeStamp() << " Created connector for trade data" << std::endl;

  std::cout << "\n*************** Market Data and Algo Services ***************" << endl<< std::endl;
  
  std::cout << PrintTimeStamp() << " Creating connector for market data" << std::endl;

  BondMarketDataConnector mkt_connector(&mkt_service);
  mkt_connector.Subscribe("Data/toy_mktdata.txt", false);
  std::cout << PrintTimeStamp() << " Created connector for market data" << std::endl;

  std::cout << "\n*************** Inquiry Service ***************" << endl << std::endl;
  
  std::cout << PrintTimeStamp() << " Creating connector for inquiries" << std::endl;
  // connector to publish historical inquiry data
  BondHistoricalInquiryConnector inquiry_history_conn;
  inquiry_historical_service.SetConnector(&inquiry_history_conn);

  BondInquiryConnector inquiry_connector(&inquiry_service);
  inquiry_service.SetConnector(&inquiry_connector);
  inquiry_connector.Subscribe("Data/inquiries.txt", false);
  std::cout << PrintTimeStamp() << " Created connector for market data" << std::endl;

  return 0;
}
