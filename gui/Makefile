# Define the compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++17 -O2 -I gui/External -fno-lto
LIBS = -lglfw3
LDFLAGS = -flto

# glfw3 and OpenGL are required to compile!

# Define the source files
SOURCES = main.cpp gui.cpp DriverHelper.cpp ImGuiExtensions.cpp FunctionHelper.cpp $(wildcard External/ImGui/*.cpp) $(wildcard gui/lib/*.cpp)

# Define the object files
OBJECTS = $(patsubst %.cpp, %.o, $(SOURCES))

# Define the target
TARGET = YeetMouseGui

default: all

all: $(TARGET)

# Rule to build the target
$(TARGET): $(OBJECTS) $(LIBFIXMATH_SLIB)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) -L$(LIBFIXMATH_DIR) -L/usr/lib/x86_64-linux-gnu -lGL $(LIBS)

# Rule to build the object files without LTO
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to build the object files with LTO (only for ImGui source files)
External/ImGui/%.o: External/ImGui/%.cpp
	$(CXX) $(CXXFLAGS) -flto -c $< -o $@

# Define dependencies

main.o: main.cpp FunctionHelper.h FunctionHelper.cpp DriverHelper.h DriverHelper.cpp
gui.o: gui.cpp
DriverHelper.o: DriverHelper.cpp
ImGuiExtensions.o: ImGuiExtensions.cpp
FunctionHelper.o: FunctionHelper.cpp
External/ImGui/%.o: External/ImGui/%.cpp
gui/lib/%.o: gui/lib/%.cpp


.PHONY: clean

clean:
	rm -rf *.o
	rm -rf External/ImGui/*.o