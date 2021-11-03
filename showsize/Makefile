ALL	= showsize showsize_packed

all:	$(ALL)

showsize:	showsize.c
	cc -g -I ../lib660 -I ../q660util -I../include -o showsize showsize.c

showsize_packed:	showsize.c
	cc -g -DUSE_GCC_PACKING -I ../lib660 -I ../q660util -I../include -o showsize_packed showsize.c

clean:
	-rm $(ALL)
