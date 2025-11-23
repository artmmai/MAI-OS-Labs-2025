all: libmy1.so libmy2.so static dynamic

libmy1.so: lib1.c
	gcc -shared -fPIC lib1.c -o libmy1.so

libmy2.so: lib2.c
	gcc -shared -fPIC lib2.c -o libmy2.so

static: static_part.c libmy1.so
	gcc static_part.c -L. -lmy1 -o static

dynamic: dynamic_part.c
	gcc dynamic_part.c -ldl -o dynamic

clean:
	rm -f *.so static dynamic
