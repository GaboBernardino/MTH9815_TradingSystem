CXX = g++
CXXFLAGS = -std=c++17 -Wall -Itradingsystem -Itradingsystem/Bond
LDFLAGS = -lrt
BOOST_INCLUDE = -I/mnt/c/Program\ Files/boost/boost_1_81_0

TARGET = TradingSystemExe
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(BOOST_INCLUDE) $(SRC) -o $(TARGET) $(LDFLAGS)

.PHONY: clean run

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
