MAKE = make
CXX = g++
CXXFLAGS += -Isrc/ --std=c++0x -g
LEX = flex
YACC = bison
VPATH = src/
TARGET = sequence_generator
CASE ?= tb1
CASEDIR = test_cases/$(CASE)

.PHONY: all clean test
all: $(TARGET)

test: all
	$(MAKE) --makefile=../../Makefile -C $(CASEDIR) simulator

simulator: simv input_sequence
	./simv

simv: fsm.v test.v
	vcs -sverilog fsm.v test.v

%.o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(TARGET): main.o SVParser.tab.o Assertion.o SVParser.lex.o Pattern.o State.o FiniteStateMachine.o InputSequenceGenerator.o
	$(CXX) $(CXXFLAGS) $^ -static -o $@

SVParser.lex.cpp: SVParser.l SVParser.y
	$(LEX) -t $< > $@

SVParser.tab.cpp: SVParser.y SVParser.hpp
	$(YACC) -d -o $@ $<

clean:
	$(RM) *.o *.lex.* $(TARGET) *.tab.*
