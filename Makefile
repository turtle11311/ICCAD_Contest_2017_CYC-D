MAKE = make
CXX = g++
CXXFLAGS += -Isrc/ --std=c++11 -O3
LEX = flex
YACC = bison
VPATH = src/
BINARY = cadb036
CASE ?= tb1
CASEDIR = test_cases/$(CASE)
SERVER = cadb036@140.110.214.97
AUTOTEST = plugin/autotest
REMOTEDIR = lichen

.PHONY: all clean test simulation output deploy gentest info

all: $(BINARY)

simulation: $(CASEDIR)/simv output
	cd $(CASEDIR)
	tcsh -c "cd $(CASEDIR); ./simv"

gentest: output
	./$(AUTOTEST) $(CASE)

test:
	make simulation CASE=tb1 | grep 'assertion rule' | sort -u | wc -l; wc -l test_cases/tb1/input_sequence
	make simulation CASE=tb2 | grep 'assertion rule' | sort -u | wc -l; wc -l test_cases/tb2/input_sequence
	make simulation CASE=tb3 | grep 'assertion rule' | sort -u | wc -l; wc -l test_cases/tb3/input_sequence

$(CASEDIR)/simv: gentest $(CASEDIR)/fsm.v $(CASEDIR)/test.v
	$(RM) -rf csrc/ simv.daidir simv ucli.key
	tcsh -c "vcs -sverilog $(CASEDIR)/*.v -Mdir=$(CASEDIR)/csrc -o $@"

output:
	bash -c "time ./$(BINARY) -i $(CASEDIR)/fsm.v -o $(CASEDIR)/input_sequence > /dev/null"

info:
	./$(BINARY) -i $(CASEDIR)/fsm.v -o $(CASEDIR)/input_sequence

deploy: $(BINARY)
	ssh $(SERVER) "rm -rf ~/$(REMOTEDIR); mkdir -p ~/$(REMOTEDIR)"
	scp -r test_cases/ $(BINARY) Makefile $(SERVER):~/$(REMOTEDIR)
	ssh $(SERVER)

%.o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(BINARY): main.o SVParser.tab.o Assertion.o SVParser.lex.o Pattern.o State.o FiniteStateMachine.o InputSequenceGenerator.o
	$(CXX) $(CXXFLAGS) $^ -llog4cxx -o $@

SVParser.lex.cpp: SVParser.l SVParser.y
	$(LEX) -t $< > $@

SVParser.tab.cpp: SVParser.y
	$(YACC) -d -o $@ $<

clean:
	$(RM) *.o *.lex.* $(BINARY) *.tab.* assertion_* *.coverage *.act
