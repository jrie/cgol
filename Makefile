#Makefile template from Lazyfoo Productions
#OBJS specifies which files to compile as part of the project
OBJS = cgol.c

#CC specifies which compiler we're using
CC = gcc

#COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
COMPILER_FLAGS = -Wall -std=c99 -O3 -g `pkg-config --cflags sdl2 SDL2_image cairo libpng`

#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = -lm `pkg-config --libs sdl2 SDL2_image cairo libpng`

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = cgol

#This is the target that compiles our executable
all : $(OBJS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
