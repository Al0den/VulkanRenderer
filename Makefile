
CXX = g++
CXXFLAGS = -std=c++20 
LDFLAGS = $(pkg-config --libs vulkan) -lglfw -lvulkan.1.3.290

GLSLC = glslc

TARGET=VulkanRenderer

SRCS_CPP = $(wildcard src/*.cpp) 
SRCS_VERT = $(wildcard shaders/*.vert)
SRCS_FRAG = $(wildcard shaders/*.frag)

HEADERS_CPP = $(wildcard include/*.hpp)

OBJS_CPP = $(SRCS_CPP:.cpp=.o)
OBJS_VERT = $(SRCS_VERT:.vert=.vert.spv)
OBJS_FRAG = $(SRCS_FRAG:.frag=.frag.spv)

OBJS = $(OBJS_CPP) $(OBJS_VERT) $(OBJS_FRAG)

all: $(TARGET)

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(OBJS) $(HEADERS_CPP)
	$(CXX) -o $@ $(OBJS_CPP) $(LDFLAGS)

%.hpp: %.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.frag.spv: %.frag
	$(GLSLC) $< -o $@

%.vert.spv: %.vert
	$(GLSLC) $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean all run
