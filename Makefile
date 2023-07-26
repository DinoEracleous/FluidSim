CC = g++

LIBRARIES = -lmingw32 -lglfw3 -lopengl32 -lgdi32
CFLAGS = -mwindows

All: project

debug: fluidSim.cpp glad.c stb_image.cpp
	$(CC) -o fluidSim -g $^ $(LIBRARIES) $(CFLAGS)

project: fluidSim.cpp glad.c stb_image.cpp
	$(CC) -o fluidSim $^ $(LIBRARIES) $(CFLAGS)
