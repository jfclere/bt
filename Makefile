sendmess: sendmess.c
	cc -c sendmess.c
	cc -o sendmess sendmess.o -l paho-mqtt3as
recvmess: recvmess.o inserttemp.o
	cc -o recvmess recvmess.o inserttemp.o -lpq -lz -lpaho-mqtt3a
recvmess.o: recvmess.c
	cc -c recvmess.c
inserttemp.o: inserttemp.c
	cc -c inserttemp.c
