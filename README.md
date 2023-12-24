# MTH9815_TradingSystem

To run from root directory, either:
* use make:
** modify the include path for boost in the Makefile and run `make`, then `./TradingSystemExe`;
* use g++ directly:
** `g++ -std=c++17 -Wall -Itradingsystem -Itradingsystem/Bond -I['path_to_boost'] main.cpp -o TradingSystemExe -lrt`

Please refer to `Final Project.docx` for a description of what each service does.
The source file `main.cpp` simulates four processes in a trading system:

## Pricing and GUI
A `BondPricingService` will read price data from prices.txt and communicate it to a `BondGUIService` and a `BondAlgoStreamingService`.
The GUI will print out some of the prices to gui.txt (with a throttle interval of 300ms).
The algo streaming stream will send a price stream to a `BondStreamingService`, which will output it to streaming.txt via a specialized historical data service.

## Trade and Risk
A `TradeBookingService` will read trade data from trades.txt and communicate it to an `PositionService`.
The Positions will then be communicated to a `RiskService`, which updates the pv01 based on positions in individual bonds as well as in 3 bucketed sectors (front end, belly, long end).
An `AlgoExecutionService` will also get data from the `MarketDataService` and send more execution orders to an `ExecutionService` which will execute them and update the positions in the `PositionService`.
Three historical data services will produce outputs in positions.txt, execution.txt and risk.txt

## Market Data
A `MarketDataService` will read data from marketdata.txt and sommunicate it to the `AlgoExecutionService` to start it.

## Inquiry Service
An `InquiryService` will read data from `inquiries.txt`, handle the inquiries (that is, receive them and provide a quote).
It will then communicate them to a specialized historical data service which outputs them to allinquiries.txt