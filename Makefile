MAKE = make
CXX = g++
SRC ?= src
CXXFLAGS += -I$(SRC)/ --std=c++11 -O3
LEX = flex
YACC = bison
VPATH = $(SRC)
BINARY = cadb036
CASE ?= tb1
BUILDDIR ?= build
ARGS ?= ""
CASEDIR = test_cases/$(CASE)
SERVER = cadb036@140.110.214.97
AUTOTEST = plugin/autotest
REMOTEDIR = lichen
OBJS = main.o SVParser.tab.o Assertion.o SVParser.lex.o Pattern.o State.o FiniteStateMachine.o InputSequenceGenerator.o

.PHONY: all clean test simulation output deploy gentest info cleanlog upload

all: $(BUILDDIR) $(BUILDDIR)/$(BINARY)

simulation: $(CASEDIR)/simv output
	cd $(CASEDIR)
	tcsh -c "cd $(CASEDIR); ./simv"

gentest: output
	./$(AUTOTEST) $(CASE)

cleanlog:
	@for src in $$(ls $(SRC)/*.cpp); do sed -irn -e '/log4cxx/d' -e '/Logger/d' -e '/LOG4CXX/d' $$src; done
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

stage:
	cp -r src/ $@

deploy: stage cleanlog 
	make upload BUILDDIR=release SRC=stage

upload: $(BUILDDIR) $(BUILDDIR)/$(BINARY)
	ssh $(SERVER) "rm -rf ~/$(REMOTEDIR); mkdir -p ~/$(REMOTEDIR)"
	$(RM) -r test_cases/
	git checkout -f test_cases/
	scp -r test_cases/ $(BUILDDIR) Makefile $(SERVER):~/$(REMOTEDIR)
	ssh $(SERVER) "cp ~/$(REMOTEDIR)/$(BUILDDIR)/$(BINARY) ~/$(REMOTEDIR)"
	ssh $(SERVER)

$(BUILDDIR)/%.o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

release/$(BINARY): $(foreach obj, $(OBJS), $(BUILDDIR)/$(obj))
	$(CXX) $(CXXFLAGS) $^ -static -o $@

build/$(BINARY): $(foreach obj, $(OBJS), $(BUILDDIR)/$(obj))
	$(CXX) $(CXXFLAGS) $^ -llog4cxx -o $@

$(SRC)/SVParser.lex.cpp: SVParser.l SVParser.y
	$(LEX) -t $< > $@

$(SRC)/SVParser.tab.cpp: SVParser.y
	$(YACC) -d -o $@ $<

$(BUILDDIR):
	mkdir -p $@
clean:
	$(RM) -rf $(BUILDDIR) src/*.lex.* src/*.tab.* release/ stage/
