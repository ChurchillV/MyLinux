CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2
TARGET = mycat
INSTALL_DIR = ~/.local/bin

all: $(TARGET)

$(TARGET) : mycat.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) mycat.cpp

clean:
	rm -f $(TARGET)

install: $(TARGET)
	mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)/
	chmod +x $(INSTALL_DIR)/$(TARGET)

uninstall:
	rm -f $(INSTALL_DIR)/$(TARGET)

.PHONY: all clean install uninstall