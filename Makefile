MAKE = make
CXX = g++
CXXFLAGS += -Isrc/ --std=c++0x -g
LEX = flex
YACC = bison
VPATH = src/
BINARY = sequence_generator
CASE ?= tb1
CASEDIR = test_cases/$(CASE)

.PHONY: all clean test simulation output

all: $(BINARY)

test: $(BINARY) simulation

simulation: $(CASEDIR)/simv output
	./simv

simv: fsm.v test.v
	vcs -sverilog fsm.v test.v

output: $(CASEDIR)/input_sequence

$(CASEDIR)/input_sequence: $(BINARY)
	./$(BINARY) -i $(CASEDIR)/fsm.v -o $(CASEDIR)/input_sequence

%.o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(BINARY): main.o SVParser.tab.o Assertion.o SVParser.lex.o Pattern.o State.o FiniteStateMachine.o InputSequenceGenerator.o
	$(CXX) $(CXXFLAGS) $^ -static -o $@

SVParser.lex.cpp: SVParser.l SVParser.y
	$(LEX) -t $< > $@

SVParser.tab.cpp: SVParser.y SVParser.hpp
	$(YACC) -d -o $@ $<

clean:
	$(RM) *.o *.lex.* $(BINARY) *.tab.*
