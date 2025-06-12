ZIG = zig c++
TARGET = modelinfoembedder
CXXFLAGS = -I ./include

linux:
	$(ZIG) -O2 -target x86_64-linux-gnu $(CXXFLAGS) -o $(TARGET) ./src/*

windows:
	$(ZIG) -O2 -target x86_64-windows $(CXXFLAGS) -o $(TARGET).exe ./src/*

macos:
	$(ZIG) -O2 -target aarch64-macos $(CXXFLAGS) -o $(TARGET) ./src/*
