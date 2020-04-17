objs += main.o
objs += decoders.o
#objs += ext_scoring_func-noback.o
objs += ext_scoring_func.o

CFLAGS = -g -O0

edit:$(objs)
	gcc prob2.c -o edit $(objs)

main.o:decoders.h
decoders.o:config.h ext_scoring_func.h
ext_scoring_func.o:config.h

.PHONY:clean
clean:
	-rm edit $(objs)

