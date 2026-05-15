CXX = g++
CXXFLAGS = -std=c++20 -O3 -march=native -ffast-math -flto=auto -pthread
TARGET = tuner

SRCS = main.cpp tuner.cpp threadpool.cpp \
       engines/fourku.cpp engines/fourkdotcpp.cpp \
       engines/toy.cpp engines/toy_tapered.cpp

HDRS = $(wildcard *.h engines/*.h)

$(TARGET): $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: clean
