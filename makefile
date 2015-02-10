CC=gcc -w
CFLAGS=-I include/
OBJS=rajaramr_proj1.o client.o server.o IPListOperations.o commandOperations.o
SRC=src/
EXE=app

$(EXE):	rajaramr_proj1.o client.o server.o IPListOperations.o commandOperations.o
	$(CC) -o $(EXE) $(CFLAGS) $(OBJS)
	rm -rf $(OBJS)

rajaramr_proj1.o: 
	$(CC) -c $(SRC)rajaramr_proj1.c $(CFLAGS) 
	
client.o:
	$(CC) -c $(CFLAGS) $(SRC)client.c

server.o:
	$(CC) -c $(CFLAGS) $(SRC)server.c

IPListOperations.o:
	$(CC) -c $(CFLAGS) $(SRC)IPListOperations.c

commandOperations.o:
	$(CC) -c $(CFLAGS) $(SRC)commandOperations.c

clean:
	rm -rf $(EXE)
	rm -rf $(OBJS)
