# Define the compiler
CXX = clang++

# Define the source files
SOURCES = ./gui.cpp

# Define the object files
OBJECTS = $(SOURCES:.cpp=.o)

# Define the flags
CXXFLAGS = -O3

# SDL flags
SDL_FLAGS = `sdl2-config --cflags`
SDL_LIBS = `sdl2-config --libs`

# Combine all flags and libraries
FLAGS = $(CXXFLAGS) $(SDL_FLAGS)
LIBS = $(SDL_LIBS)

# Define the target executable
TARGET = Change

# Default rule
all: $(TARGET)

# Rule to build the target
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LIBS) -o $(TARGET)

# Pattern rule for building object files
%.o: %.cpp
	$(CXX) $(FLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: all clean run
