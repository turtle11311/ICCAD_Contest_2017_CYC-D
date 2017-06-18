CXX = g++
CXXFLAGS += -Isrc/ --std=c++0x -g
LEX = flex
YACC = bison
VPATH = src/
TARGET = sequence_generator

.PHONY: all clean

all: $(TARGET)

%.o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(TARGET): main.o SVParser.tab.o Assertion.o SVParser.lex.o Pattern.o State.o FiniteStateMachine.o
	$(CXX) $(CXXFLAGS) $^ -static -o $@

SVParser.lex.cpp: SVParser.l SVParser.y
	$(LEX) -t $< > $@

SVParser.tab.cpp: SVParser.y SVParser.hpp
	$(YACC) -d -o $@ $<

clean:
	$(RM) *.o *.lex.* $(TARGET) *.tab.*
