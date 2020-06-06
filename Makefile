CC=clang
CFLAGS=-Wall
CXX=clang++
CXXFLAGS=-Wall -std=c++17 -fno-exceptions -Iinclude
LD=ld
LDFLAGS=
MKDIR=mkdir

HVNFLAGS=-lSystem
HVN_MNGENFLAGS=-lSystem -lcmark
HVN_CFGENFLAGS=-lSystem -lc++
HVN_MKGENFLAGS=-lSystem -lc++
HVN_CREATFLAGS=-lSystem -lc++

BINARIES=build/bin
LIBRARIES=build/lib
OBJECTS=build/objects

.PHONY: first all clean
first: all
$(OBJECTS)/hvn/main.o: src/hvn/main.c $(OBJECTS)/hvn
	$(CC) $(CFLAGS) -c -o $@ $<
$(OBJECTS)/hvn:
	$(MKDIR) $@
$(BINARIES)/hvn: $(OBJECTS)/hvn/main.o
	$(LD) $(LDFLAGS) $(HVNFLAGS) -o $@ $^
$(OBJECTS)/hvn-cfgen/main.o: src/hvn-cfgen/main.cc $(OBJECTS)/hvn-cfgen
	$(CXX) $(CXXFLAGS) -c -o $@ $<
$(OBJECTS)/hvn-cfgen:
	$(MKDIR) $@
$(BINARIES)/hvn-cfgen: $(OBJECTS)/hvn-cfgen/main.o
	$(LD) $(LDFLAGS) $(HVN_CFGENFLAGS) -o $@ $^
$(OBJECTS)/hvn-mkgen/main.o: src/hvn-mkgen/main.cc $(OBJECTS)/hvn-mkgen
	$(CXX) $(CXXFLAGS) -c -o $@ $<
$(OBJECTS)/hvn-mkgen:
	$(MKDIR) $@
$(BINARIES)/hvn-mkgen: $(OBJECTS)/hvn-mkgen/main.o
	$(LD) $(LDFLAGS) $(HVN_MKGENFLAGS) -o $@ $^
$(OBJECTS)/hvn-mngen/main.o: src/hvn-mngen/main.c $(OBJECTS)/hvn-mngen
	$(CC) $(CFLAGS) -c -o $@ $<
$(OBJECTS)/hvn-mngen:
	$(MKDIR) $@
$(BINARIES)/hvn-mngen: $(OBJECTS)/hvn-mngen/main.o
	$(LD) $(LDFLAGS) $(HVN_MNGENFLAGS) -o $@ $^
$(OBJECTS)/hvn-creat/main.o: src/hvn-creat/main.cc $(OBJECTS)/hvn-creat
	$(CXX) $(CXXFLAGS) -c -o $@ $<
$(OBJECTS)/hvn-creat:
	$(MKDIR) $@
$(BINARIES)/hvn-creat: $(OBJECTS)/hvn-creat/main.o
	$(LD) $(LDFLAGS) $(HVN_CREATFLAGS) -o $@ $^
all: $(BINARIES)/hvn-mngen $(BINARIES)/hvn-creat $(BINARIES)/hvn-mkgen $(BINARIES)/hvn-cfgen $(BINARIES)/hvn
clean:
	rm -rf $(BINARIES)/* $(LIBRARIES)/* $(OBJECTS)/*
