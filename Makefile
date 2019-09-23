main:
#	g++ -g `pkg-config sdl2 glew --cflags --libs` -framework OpenGL main.cpp -o main
	clang++ main.cpp -g -o main `pkg-config ILU sdl2 SDL2_image glew --cflags --libs`
