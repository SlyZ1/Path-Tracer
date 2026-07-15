CUDA_PATH ?= $(CUDA_PATH)

MSVC_BIN = C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64

CXX  = "$(MSVC_BIN)/cl.exe"
LINK = "$(MSVC_BIN)/link.exe"
LIBTOOL  = "$(MSVC_BIN)/lib.exe"
NVCC = "$(CUDA_PATH)/bin/nvcc.exe"

CXXFLAGS     = /nologo /Zi /std:c++17 /EHsc /W4 /MD
CXXFLAGS_SRC = $(CXXFLAGS) /showIncludes

NVCCFLAGS = -std=c++17 -ccbin "$(MSVC_BIN)/cl.exe" -g -Xcompiler /MD -arch=sm_89

INCLUDES = -Iinclude \
           -Iinclude/imgui \
           -Iinclude/lodepng \
           -Iinclude/tinyobjloader \
           -Iinclude/nativefiledialog \
           -Iinclude/tensorrt \
           -Iinclude/tinyexr \
           -Iinclude/nlohmannjson \
           -I"$(CUDA_PATH)/include"

LIBS = /LIBPATH:lib glfw3dll.lib nfd.lib ole32.lib uuid.lib nvinfer.lib dwmapi.lib \
       /LIBPATH:"$(CUDA_PATH)/lib/x64" cudart.lib cuda.lib

rwildcard = $(foreach d, $(wildcard $(1:=/*)), $(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC     = $(call rwildcard, src, *.cpp) $(call rwildcard, src, *.c)
CU_SRC  = $(call rwildcard, src, *.cu)

OBJ    = $(patsubst src/%.cpp, build/src/%.obj, \
         $(patsubst src/%.c,   build/src/%.obj, $(SRC)))
CU_OBJ = $(patsubst src/%.cu, build/src/%.obj, $(CU_SRC))

DEP    = $(OBJ:.obj=.d)

IMGUI_SRC   = $(wildcard include/imgui/*.cpp)
LODEPNG_SRC = $(wildcard include/lodepng/*.cpp)
TINYOBJ_SRC = $(wildcard include/tinyobjloader/*.cpp)
TINYEXR_SRC = $(wildcard include/tinyexr/*.c)

IMGUI_OBJ   = $(patsubst include/imgui/%.cpp,         	build/imgui/%.obj,   $(IMGUI_SRC))
LODEPNG_OBJ = $(patsubst include/lodepng/%.cpp,       	build/lodepng/%.obj, $(LODEPNG_SRC))
TINYOBJ_OBJ = $(patsubst include/tinyobjloader/%.cpp, 	build/tinyobj/%.obj, $(TINYOBJ_SRC))
TINYEXR_OBJ = $(patsubst include/tinyexr/%.c, 		  	build/tinyexr/%.obj, $(TINYEXR_SRC))

THIRD_PARTY_LIB = build/thirdparty.lib
TARGET          = myprogram.exe

all: $(TARGET)

$(TARGET): $(OBJ) $(CU_OBJ) $(THIRD_PARTY_LIB)
	@cmd /c "echo LIB=%LIB%"
	$(LINK) /nologo $(OBJ) $(CU_OBJ) $(THIRD_PARTY_LIB) /OUT:$@ $(LIBS)

build/src/%.obj: src/%.cpp
	if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	$(CXX) $(CXXFLAGS_SRC) $(INCLUDES) /c $< /Fo:$@ > "$(subst /,\,$@).tmp"
	powershell -NoProfile -ExecutionPolicy Bypass -File tools\gen_deps.ps1 -TmpFile "$(subst /,\,$@).tmp" -ObjFile "$(subst /,\,$@)" -SrcFile "$(subst /,\,$<)" -DepFile "$(subst /,\,$(@:.obj=.d))"

build/src/%.obj: src/%.c
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	$(CXX) $(CXXFLAGS_SRC) $(INCLUDES) /c $< /Fo:$@ > "$(subst /,\,$@).tmp"
	powershell -NoProfile -ExecutionPolicy Bypass -File tools\gen_deps.ps1 -TmpFile "$(subst /,\,$@).tmp" -ObjFile "$(subst /,\,$@)" -SrcFile "$(subst /,\,$<)" -DepFile "$(subst /,\,$(@:.obj=.d))"

build/src/%.obj: src/%.cu
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	$(NVCC) $(NVCCFLAGS) $(INCLUDES) -c $< -o $@

$(THIRD_PARTY_LIB): $(IMGUI_OBJ) $(LODEPNG_OBJ) $(TINYOBJ_OBJ) $(TINYEXR_OBJ)
	$(LIBTOOL) /nologo /OUT:$@ $^

build/imgui/%.obj: include/imgui/%.cpp | build/imgui
	$(CXX) $(CXXFLAGS) $(INCLUDES) /c $< /Fo:$@

build/lodepng/%.obj: include/lodepng/%.cpp | build/lodepng
	$(CXX) $(CXXFLAGS) $(INCLUDES) /c $< /Fo:$@

build/tinyobj/%.obj: include/tinyobjloader/%.cpp | build/tinyobj
	$(CXX) $(CXXFLAGS) $(INCLUDES) /c $< /Fo:$@
	
build/tinyexr/%.obj: include/tinyexr/%.c | build/tinyexr
	$(CXX) $(CXXFLAGS) $(INCLUDES) /c $< /Fo:$@

build/imgui build/lodepng build/tinyobj build/tinyexr:
	if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"

-include $(DEP)

clean:
	rmdir /S /Q build
	del /Q $(TARGET)

re: clean all

run: all
	$(TARGET) $(ARGS)