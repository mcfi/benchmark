CXX := g++
CXXFLAGS := -g -O3

TARGET := pingpong
SRC := pingpong.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

