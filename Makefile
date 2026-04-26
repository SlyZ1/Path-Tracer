CXX = C:/ProgramData/mingw64/mingw64/bin/g++.exe
AR  = C:/ProgramData/mingw64/mingw64/bin/ar.exe
CXXFLAGS = -g -std=c++17 -Wall -Wextra

INCLUDES = -Iinclude \
           -Iinclude/imgui \
           -Iinclude/lodepng \
           -Iinclude/tinyobjloader

LIBS = -Llib -lglfw3dll

SRC     = $(wildcard src/*.cpp) src/glad.c
OBJ     = $(patsubst src/%.cpp, build/src/%.o, $(filter %.cpp, $(SRC))) \
          $(patsubst src/%.c,   build/src/%.o, $(filter %.c,   $(SRC)))

IMGUI_SRC = $(wildcard include/imgui/*.cpp)
LODEPNG_SRC = $(wildcard include/lodepng/*.cpp)
TINYOBJ_SRC = $(wildcard include/tinyobjloader/*.cpp)
THIRD_PARTY_LIB = build/libthirdparty.a

TARGET = myprogram.exe

all: $(TARGET)

$(TARGET): $(OBJ) $(THIRD_PARTY_LIB)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(THIRD_PARTY_LIB): $(patsubst include/imgui/%.cpp,       build/imgui/%.o,   $(IMGUI_SRC)) \
                    $(patsubst include/lodepng/%.cpp,     build/lodepng/%.o, $(LODEPNG_SRC)) \
                    $(patsubst include/tinyobjloader/%.cpp, build/tinyobj/%.o, $(TINYOBJ_SRC))
	$(AR) rcs $@ $^

build/src/%.o: src/%.cpp | build/src
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

build/src/%.o: src/%.c | build/src
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# --- Compilation des libs tierces ---
build/imgui/%.o: include/imgui/%.cpp | build/imgui
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

build/lodepng/%.o: include/lodepng/%.cpp | build/lodepng
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

build/tinyobj/%.o: include/tinyobjloader/%.cpp | build/tinyobj
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

build/src build/imgui build/lodepng build/tinyobj:
	if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"

clean:
	rmdir /S /Q build
	del /Q $(TARGET)

re: clean all