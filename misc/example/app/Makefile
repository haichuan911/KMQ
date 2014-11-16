CC = g++
CFLAGS = -g -Wall
LIBS = -L/home/w/lib64 -L/usr/local/lib -L/usr/local/lib64 -lkmq -lrt -lpthread -luuid
INC = -I/home/w/include

all:
	$(CC) $(CFLAGS) $(INC) $(LIBS) appcli_example.cc -o appcli
	$(CC) $(CFLAGS) $(INC) $(LIBS) appsvr_example.cc -o appsvr
clean:
	rm *.o
	rm appsvr && rm appcli