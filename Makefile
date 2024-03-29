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

all: wSender
all: wReceiver

# TODO Modify source file name for your project.
# For C only.
RECEIVER = wReceiver.cpp
SENDER = wSender.cpp
# For C++ only.
# SOURCES = iPerfer.cpp
wSender: $(SENDER) $(RECEIVER)
	$(CXX) $(CXXFLAGS) $(SENDER) -o wSender

wReceiver: $(RECEIVER)
	$(CXX) $(CXXFLAGS) $(RECEIVER) -o wReceiver

clean:
	rm -rf wSender *.dSYM
	rm -rf wReceiver *.dSYM

.PHONY: clean