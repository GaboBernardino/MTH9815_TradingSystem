# MTH9815_TradingSystem

To run from root directory, either:
* use the makefile:
`make`, then `./TradingSystemExe`;
* use g++ directly:
`g++ -std=c++17 -Wall -Itradingsystem -Itradingsystem/Bond -I/mnt/c/Program\ Files/boost/boost_1_81_0 main.cpp -o TradingSystemExe -lrt`