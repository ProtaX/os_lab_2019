all: main.c revert_string.c
# -I. - headers inlucde dir is ./
	gcc main.c revert_string.c -I. -o revert_string
static: revert_string.c
	gcc revert_string.c -c -o revert_string.o
# c - create archive
# r - include files
# s - write index of an object file (?)
	ar rcs ./librevert_string.a ./revert_string.o
	rm revert_string.o
link-static: main.c static
	gcc main.c -I. -L./ -lrevert_string -o revert_string_static
dynamic: revert_string.c
# PIC - position independent code
	gcc -fPIC -c revert_string.c -o revert_string.o
	gcc -shared revert_string.o -o librevert_string.so
	rm revert_string.o
link-dynamic: main.c dynamic
	gcc main.c -L./ -lrevert_string -o revert_string_dynamic
	export LD_LIBRARY_PATH=$(pwd)
clean:
	rm *.o *.a *.so revert_string_dynamic revert_string_static -f