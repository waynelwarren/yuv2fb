all: img

img: img.c
	gcc -g -o img img.c
