# Define the compiler and flags
CXX = g++
CXXFLAGS = -Wall -fsanitize=undefined -std=c++17 -O3 -I gui/External
LIBS = -lglfw3

# glfw3 and OpenGL are required to compile!

# Define the source files
SOURCES = main.cpp gui.cpp DriverHelper.cpp ImGuiExtensions.cpp FunctionHelper.cpp $(wildcard External/ImGui/*.cpp) $(wildcard gui/lib/*.cpp)

# Define the object files
OBJECTS = $(patsubst %.cpp, %.o, $(SOURCES))

# Define the target
TARGET = YeetMouseGui

default: all

all: $(TARGET)

# Define the libfixmath library
#LIBFIXMATH_DIR = ../libfixmath
#LIBFIXMATH_SLIB = $(LIBFIXMATH_DIR)/libfixmath.a
#LIBFIXMATH_SOURCES = $(wildcard $(LIBFIXMATH_DIR)/*.cpp)

# Rule to build the target
$(TARGET): $(OBJECTS) $(LIBFIXMATH_SLIB)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) -L$(LIBFIXMATH_DIR) -L/usr/lib/x86_64-linux-gnu -lGL $(LIBS)

# Rule to build the libfixmath library
#$(LIBFIXMATH_SLIB): $(LIBFIXMATH_SOURCES)
#	$(MAKE) -C $(LIBFIXMATH_DIR)

# Rule to build the object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $(dir $<)$(notdir $@)

# Add this rule to compile the ImGui source files
External/ImGui/%.o: External/ImGui/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -rf *.o
	rm -rf External/ImGui/*.o