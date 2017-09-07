MAKE = make
CXX = g++
CXXFLAGS += -Isrc/ --std=c++11 -O3
LEX = flex
YACC = bison
VPATH = src
BINARY = cadb036
CASE ?= tb1
BUILDDIR ?= build
ARGS ?= ""
CASEDIR = test_cases/$(CASE)
SERVER = cadb036@140.110.214.97
AUTOTEST = plugin/autotest
REMOTEDIR = lichen
OBJS = main.o SVParser.tab.o Assertion.o SVParser.lex.o Pattern.o State.o FiniteStateMachine.o InputSequenceGenerator.o

.PHONY: all clean test simulation output deploy gentest info

all: $(BUILDDIR) $(BINARY)

simulation: $(CASEDIR)/simv output
	cd $(CASEDIR)
	tcsh -c "cd $(CASEDIR); ./simv"

gentest: output
	./$(AUTOTEST) $(CASE)

test:
	make simulation CASE=tb1 ARGS=$(ARGS) | grep 'assertion rule' | sort -u | wc -l; wc -l test_cases/tb1/input_sequence
	make simulation CASE=tb2 ARGS=$(ARGS) | grep 'assertion rule' | sort -u | wc -l; wc -l test_cases/tb2/input_sequence
	make simulation CASE=tb3 ARGS=$(ARGS) | grep 'assertion rule' | sort -u | wc -l; wc -l test_cases/tb3/input_sequence

$(CASEDIR)/simv: gentest $(CASEDIR)/fsm.v $(CASEDIR)/test.v
	$(RM) -rf csrc/ simv.daidir simv ucli.key
	tcsh -c "vcs -sverilog $(CASEDIR)/*.v -Mdir=$(CASEDIR)/csrc -o $@"

output:
	bash -c "time ./$(BINARY) -i $(CASEDIR)/fsm.v -o $(CASEDIR)/input_sequence $(ARGS) > /dev/null"

info:
	./$(BINARY) -i $(CASEDIR)/fsm.v -o $(CASEDIR)/input_sequence $(ARGS)

deploy: $(BINARY)
	ssh $(SERVER) "rm -rf ~/$(REMOTEDIR); mkdir -p ~/$(REMOTEDIR)"
	scp -r test_cases/ $(BINARY) Makefile $(SERVER):~/$(REMOTEDIR)
	ssh $(SERVER)

$(BUILDDIR)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(BINARY): $(foreach obj, $(OBJS), $(BUILDDIR)/$(obj))
	$(CXX) $(CXXFLAGS) $^ -llog4cxx -o $@

src/SVParser.lex.cpp: SVParser.l SVParser.y
	$(LEX) -t $< > $@

src/SVParser.tab.cpp: SVParser.y
	$(YACC) -d -o $@ $<

$(BUILDDIR):
	mkdir $@
clean:
	$(RM) -rf $(BINARY) $(BUILDDIR) src/*.lex.* src/*.tab.*
