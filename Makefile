CXX = C:/ProgramData/mingw64/mingw64/bin/g++.exe
CXXFLAGS = -g -std=c++17 -Wall -Wextra

INCLUDES = -Iinclude \
           -Iinclude/imgui \
           -Iinclude/lodepng \
           -Iinclude/tinyobjloader

LIBS = -Llib -lglfw3dll

SRC = $(wildcard src/*.cpp) \
      $(wildcard include/imgui/*.cpp) \
      $(wildcard include/lodepng/*.cpp) \
      $(wildcard include/tinyobjloader/*.cpp) \
      src/glad.c

TARGET = myprogram.exe

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

clean:
	del /Q $(TARGET)

re: clean all