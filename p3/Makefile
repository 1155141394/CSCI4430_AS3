## CSCI 4430 Advanced Makefile

# How to use this Makefile...
###################
###################
##               ##
##  $ make help  ##
##               ##
###################
###################

CXX = g++
# TODO For C++ only.
CXXFLAGS = -g -std=c++11 -pedantic

all: wSender-base
all: wReceiver-base
all: wSender-opt
all: wReceiver-opt

# TODO Modify source file name for your project.
# For C only.
RECEIVER-BASE = ./WTP-base/wReceiver.cpp
SENDER-BASE = ./WTP-base/wSender.cpp
RECEIVER-OPT = ./WTP-opt/wReceiver.cpp
SENDER-OPT = ./WTP-opt/wSender.cpp
# For C++ only.
# SOURCES = iPerfer.cpp
wSender-base: $(SENDER-BASE)
	$(CXX) $(CXXFLAGS) $(SENDER) -o ./WTP-base/wSender

wReceiver-base: $(RECEIVER-BASE)
	$(CXX) $(CXXFLAGS) $(RECEIVER) -o ./WTP-base/wReceiver

wSender-opt: $(RECEIVER-OPT)
	$(CXX) $(CXXFLAGS) $(SENDER) -o ./WTP-opt/wSender

wSender-opt: $(SENDER-OPT)
	$(CXX) $(CXXFLAGS) $(SENDER) -o ./WTP-opt/wSender

clean:
	rm -rf wSender *.dSYM
	rm -rf wReceiver *.dSYM

.PHONY: clean