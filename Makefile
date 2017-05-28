CXX = g++
CXXFLAGS += -Isrc/ --std=c++0x -g
LEX = flex
YACC = bison
VPATH = src/
TARGET = sequence_generator

.PHONY: all clean

all: $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(TARGET): main.o SVParser.tab.o SVParser.lex.o
	$(CXX) $(CXXFLAGS) $^ -o $@

SVParser.lex.cpp: SVParser.l SVParser.y
	$(LEX) -t $< > $@

SVParser.tab.cpp: SVParser.y SVParser.hpp
	$(YACC) -d -o $@ $<

clean:
	$(RM) *.o *.lex.* $(TARGET) *.tab.*