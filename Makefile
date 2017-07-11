MAKE = make
CXX = g++
CXXFLAGS += -Isrc/ --std=c++11 -g
LEX = flex
YACC = bison
BUILDDIR ?= build
VPATH = src:$(BUILDDIR)
OBJS = main.o SVParser.tab.o Assertion.o SVParser.lex.o Pattern.o State.o FiniteStateMachine.o InputSequenceGenerator.o
BINARY = sequence_generator
DEBUG_BIN = bugcry
CASE ?= tb1
CASEDIR = test_cases/$(CASE)
SERVER = cadb036@140.110.214.97
REMOTEDIR = cadb2

.PHONY: all clean test simulation output deploy findbug binary


all: $(BINARY)

test:
	$(MAKE) -C $(CASEDIR) --makefile=../../Makefile simulation

simulation: simv
	make -C ../../ output CASE=$(CASE)
	./simv

simv: fsm.v test.v
	vcs -sverilog fsm.v test.v

output:
	bash -c "time ./$(BINARY) -i $(CASEDIR)/fsm.v -o $(CASEDIR)/input_sequence > /dev/null"

deploy: $(BINARY)
	ssh $(SERVER) "rm -rf ~/$(REMOTEDIR); mkdir -p ~/$(REMOTEDIR)"
	scp -r test_cases/ $(BINARY) Makefile $(SERVER):~/$(REMOTEDIR)

binary: $(BINARY)

findbug:
	$(eval CXXFLAGS += -DDEBUG)
	make binary BINARY=$(DEBUG_BIN) CXXFLAGS="$(CXXFLAGS)" BUILDDIR=debug
	./$(DEBUG_BIN) -i $(CASEDIR)/fsm.v -o $(CASEDIR)/input_sequence

$(BUILDDIR)/%.o: src/%.cpp $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(BINARY): $(foreach obj, $(OBJS), $(BUILDDIR)/$(obj))
	$(CXX) $(CXXFLAGS) $^ -static -lboost_system -lboost_filesystem -o $@

$(BUILDDIR)/SVParser.lex.cpp: SVParser.l SVParser.y
	$(LEX) -t $< > $@

$(BUILDDIR)/SVParser.tab.cpp: SVParser.y
	$(YACC) -d -o $@ $<

$(BUILDDIR):
	mkdir $(BUILDDIR)

clean:
	$(RM) *.o *.lex.* $(BINARY) $(DEBUG_BIN) *.tab.* build/ debug/ -rf
