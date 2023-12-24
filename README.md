# MTH9815_TradingSystem

To run from root directory, either:
* use make:
** modify the include path for boost in the Makefile and run `make`, then `./TradingSystemExe`;
* use g++ directly:
** `g++ -std=c++17 -Wall -Itradingsystem -Itradingsystem/Bond -I['path_to_boost'] main.cpp -o TradingSystemExe -lrt`

Please refer to `Final Project.docx` for a description of what each service does.
The source file `main.cpp` simulates four separated processes in a trading system:

## Pricing and GUI
A `BondPricingService` will read price data from prices.txt and communicate it to a `BondGUIService` and a `BondAlgoStreamingService`.
The GUI will print out some of the prices to gui.txt (with a throttle interval of 300ms).
The Algo stream will send a price stream to a `BondStreamingService`, which will output it to streaming.txt via a specialized historical data service.

## Trade and Risk
