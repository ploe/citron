main:
	g++ -g $(pkg-config sdl2 --cflags --libs) $(pkg-config glew --cflags --libs) -framework OpenGL main.cpp -o main
