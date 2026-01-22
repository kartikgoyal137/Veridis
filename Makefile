CXX = g++
CXXFLAGS = -std=c++17

SRC = \
	main.cpp \
	src/core/monitor.cpp \
	src/system/cgroup.cpp \
	src/system/helper.cpp \
	src/system/rapl.cpp

TARGET = veridis

LIBS = -lbpf -lelf -lz

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

