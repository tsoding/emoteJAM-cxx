CXXFLAGS=-Wall -Wextra -Wno-missing-field-initializers -std=c++17 -pedantic -fno-exceptions `pkg-config --cflags sdl2`
LIBS=`pkg-config --libs sdl2`

emoteJAM: src/main.cpp src/aids.hpp
	$(CXX) $(CXXFLAGS) -o emoteJAM src/main.cpp $(LIBS)
