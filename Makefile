CXX = g++
CXXFLAGS = -std=c++17

SRC = \
	main.cpp \
	src/core/monitor.cpp \
	src/system/cgroup.cpp \
	src/system/helper.cpp \
	src/system/rapl.cpp \
	src/system/carbon.cpp

BPF_SRC = bpf/veridis.bpf.c
BPF_OBJ = target/veridis.bpf.o

TARGET = veridis

LIBS = -lbpf -lelf -lz -lcurl

all: $(BPF_OBJ) $(TARGET)

$(BPF_OBJ): $(BPF_SRC)
	mkdir -p target
	clang -g -O2 -target bpf -I./bpf -c $(BPF_SRC) -o $(BPF_OBJ)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)
	rm -rf target/

