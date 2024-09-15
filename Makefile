sendmess: sendmess.c
	cc -c sendmess.c
	cc -o sendmess sendmess.o -l paho-mqtt3a
recvmess: recvmess.c
	cc -c recvmess.c
	cc -o recvmess recvmess.o -l paho-mqtt3a
