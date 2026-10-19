CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -g

TARGET = S_Expr_2
OBJS = main.o sexpression.o token.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

main.o: main.cpp sexpression.h token.h
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

sexpression.o: sexpression.cpp sexpression.h token.h
	$(CXX) $(CXXFLAGS) -c sexpression.cpp -o sexpression.o

token.o: token.cpp token.h
	$(CXX) $(CXXFLAGS) -c token.cpp -o token.o

clean:
	$(RM) $(OBJS) $(TARGET)