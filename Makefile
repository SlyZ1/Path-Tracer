CXX = C:/ProgramData/mingw64/mingw64/bin/g++.exe
AR  = C:/ProgramData/mingw64/mingw64/bin/ar.exe

CXXFLAGS     = -g -std=c++17 -Wall -Wextra
CXXFLAGS_SRC = $(CXXFLAGS) -MMD -MP

INCLUDES = -Iinclude \
           -Iinclude/imgui \
           -Iinclude/lodepng \
           -Iinclude/tinyobjloader \
           -Iinclude/nativefiledialog

LIBS = -Llib -lglfw3dll -lnfd -lole32 -luuid

SRC = $(wildcard src/*.cpp) src/glad.c
OBJ = $(patsubst src/%.cpp, build/src/%.o, $(filter %.cpp, $(SRC))) \
      $(patsubst src/%.c,   build/src/%.o, $(filter %.c,   $(SRC)))

-include $(OBJ:.o=.d)

IMGUI_SRC   = $(wildcard include/imgui/*.cpp)
LODEPNG_SRC = $(wildcard include/lodepng/*.cpp)
TINYOBJ_SRC = $(wildcard include/tinyobjloader/*.cpp)

IMGUI_OBJ   = $(patsubst include/imgui/%.cpp,          		build/imgui/%.o,   $(IMGUI_SRC))
LODEPNG_OBJ = $(patsubst include/lodepng/%.cpp,        		build/lodepng/%.o, $(LODEPNG_SRC))
TINYOBJ_OBJ = $(patsubst include/tinyobjloader/%.cpp,  		build/tinyobj/%.o, $(TINYOBJ_SRC))

THIRD_PARTY_LIB = build/libthirdparty.a
TARGET          = myprogram.exe


all: $(TARGET)

$(TARGET): $(OBJ) $(THIRD_PARTY_LIB)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)


build/src/%.o: src/%.cpp | build/src
	$(CXX) $(CXXFLAGS_SRC) $(INCLUDES) -MF build/src/$*.d -c $< -o $@

build/src/%.o: src/%.c | build/src
	$(CXX) $(CXXFLAGS_SRC) $(INCLUDES) -MF build/src/$*.d -c $< -o $@


$(THIRD_PARTY_LIB): $(IMGUI_OBJ) $(LODEPNG_OBJ) $(TINYOBJ_OBJ)
	$(AR) rcs $@ $^

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

run: all
	$(TARGET)