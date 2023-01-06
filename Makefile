CXX := g++
CXXFLAGS := -std=c++17 -Iinclude
LDFLAGS := -lSDL2 -lSDL2_image -lSDL2_mixer -lstdc++fs
OBJS = $(addprefix build/, main.o graphics.o character.o)
EXECNAME = OpenManifold
ICON = 

ifeq ($(OS),Windows_NT)
	LDFLAGS := -lmingw32 -lSDL2main $(LDFLAGS)
    EXECNAME := $(addsuffix .exe,$(EXECNAME))
	ICON := res/icon.res
endif

all: build $(OBJS)
	$(CXX) -o bin/$(EXECNAME) $(OBJS) $(ICON) $(LDFLAGS)
	
$(OBJS): build/%.o: src/%.cpp
	$(CXX) -c $< $(CXXFLAGS) $(LDFLAGS) -o $@
	
bgtest:
	$(CXX) src/tests/bg_test.cpp -o bin/background_test.exe $(CXXFLAGS) $(LDFLAGS)

chartest:
	$(CXX) src/tests/character_test.cpp -o bin/character_test.exe $(CXXFLAGS) $(LDFLAGS)

icon: 
	rm -rf res/icon.res
	-windres res/icon.rc -O coff -o res/icon.res

pkg: clean icon all
	@echo Creating temporary directory...
	mkdir release-tmp
	
	@echo Copying files...
	cp bin/$(EXECNAME) ./release-tmp/$(EXECNAME)
	cp README.md ./release-tmp/README.md
	cp README.md ./release-tmp/README.txt
	cp LICENSE ./release-tmp/LICENSE.txt
	cp -r ./res/assets ./release-tmp/assets
	
ifeq ($(OS),Windows_NT)
	@echo Copying SDL2 DLLs...
	cp -r ./res/dll/. ./release-tmp/
endif
	
	@if [ -d "./release" ]; then echo Copying release-version contents...; cp -r ./release/. ./release-tmp/assets; fi
	
	@echo Zipping via 7zip...
	7z a Open-Manifold.zip ./release-tmp/*
	
	@echo Deleting temporary directory...
	rm -rf release-tmp

install:
	cp -r ./res/assets ./bin/assets
	cp -r ./release/. ./bin/assets
ifeq ($(OS),Windows_NT)
	cp -r ./res/dll/. ./abin/
endif

build:
	if [ ! -d "./build" ]; then mkdir -p build; fi
	if [ ! -d "./bin" ]; then mkdir -p bin; fi

clean:
	rm -f build/*.o

help:
	@echo -----------------------------------------------------------------
	@echo Targets:
	@echo [none]   - Builds the game executable.
	@echo bgtest   - Builds a background test program.
	@echo chartest - Builds a character file test program.
	@echo install  - Copies game assets into bin folder.
	@echo build    - Creates build and bin folders.
	@echo pkg      - Cleans, builds the game, and makes a ZIP. Requires Bash and 7zip!
	@echo icon     - Creates Windows resource file.
	@echo clean    - Deletes all object files.
	@echo help     - Prints this text.
	@echo -----------------------------------------------------------------