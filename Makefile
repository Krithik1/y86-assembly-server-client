# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -g

# Executable names
SERVER_EXEC = server
CLIENT_EXEC = client

# Source files
SRCS = server.cpp client.cpp y86_instruction_handler.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Targets
all: $(SERVER_EXEC) $(CLIENT_EXEC)

$(SERVER_EXEC): server.o y86_instruction_handler.o
	$(CXX) -o $@ $^

$(CLIENT_EXEC): client.o y86_instruction_handler.o
	$(CXX) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(SERVER_EXEC) $(CLIENT_EXEC)

.PHONY: all clean
