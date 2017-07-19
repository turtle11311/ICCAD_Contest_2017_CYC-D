MAKE = make
CXX = g++
CXXFLAGS += -Isrc/ --std=c++0x -g
LEX = flex
YACC = bison
VPATH = src/
BINARY = sequence_generator
CASE ?= tb1
CASEDIR = test_cases/$(CASE)
SERVER = cadb036@140.110.214.97
REMOTEDIR = lichen

.PHONY: all clean test simulation output deploy info


all: $(BINARY)

test:
	$(MAKE) -C $(CASEDIR) --makefile=../../Makefile simulation

simulation: simv
	make -C ../../ output CASE=$(CASE)
	./simv

simv: fsm.v test.v
	vcs -sverilog fsm.v test.v

separable:
	bash -c "time ./$(BINARY) -i $(CASEDIR)/fsm.v -o $(CASEDIR)/input_sequence -s true > /dev/null"

output:
	bash -c "time ./$(BINARY) -i $(CASEDIR)/fsm.v -o $(CASEDIR)/input_sequence -s false > /dev/null"

info: $(BINARY)
	./$(BINARY) -i $(CASEDIR)/fsm.v -o $(CASEDIR)/input_sequence -s false

deploy: $(BINARY)
	ssh $(SERVER) "rm -rf ~/$(REMOTEDIR); mkdir -p ~/$(REMOTEDIR)"
	scp -r test_cases/ $(BINARY) Makefile $(SERVER):~/$(REMOTEDIR)
	ssh $(SERVER)

%.o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(BINARY): main.o SVParser.tab.o Assertion.o SVParser.lex.o Pattern.o State.o FiniteStateMachine.o InputSequenceGenerator.o
	$(CXX) $(CXXFLAGS) $^ -static -o $@

SVParser.lex.cpp: SVParser.l SVParser.y
	$(LEX) -t $< > $@

SVParser.tab.cpp: SVParser.y
	$(YACC) -d -o $@ $<

clean:
	$(RM) *.o *.lex.* $(BINARY) *.tab.* assertion_* *.coverage *.act
