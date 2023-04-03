INCLDIR		= ../include

SRCS		= PacketQueue.C

testqueue:	PacketQueue.C
		$(CXX) -g -o $@ -I${INCLDIR} ${SRCS} ../libcomserv/Logger.o ../libcsutil/logging.o

clean:		
		-rm -f testqueue *.o
