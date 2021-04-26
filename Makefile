CXXFLAGS=-Wall -Wextra -std=c++17 -pedantic `pkg-config --cflags sdl2`
LIBS=`pkg-config --libs sdl2`

emoteJAM: src/main.cpp src/aids.hpp
	$(CXX) $(CXXFLAGS) -o emoteJAM src/main.cpp $(LIBS)
