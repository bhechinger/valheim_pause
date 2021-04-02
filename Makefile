all:
	gcc -o valheim_pause valheim_pause.c `pkg-config --cflags --libs jack`
