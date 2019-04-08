CC = gcc
CFLAGS  = -g -Wall
RM = rm -rf

all: mirror_client reader writer

mirror_client:  mirror_client.o file_list.o utils.o
	$(CC) $(CFLAGS) -o exe/mirror_client mirror_client.o file_list.o utils.o

reader:  reader.o file_list.o utils.o
	$(CC) $(CFLAGS) -o exe/reader reader.o file_list.o utils.o

writer:  writer.o file_list.o utils.o
	$(CC) $(CFLAGS) -o exe/writer writer.o file_list.o utils.o

mirror_client.o:  mirror_client/mirror_client.c
	$(CC) $(CFLAGS) -c mirror_client/mirror_client.c

reader.o:  reader/reader.c
	$(CC) $(CFLAGS) -c reader/reader.c

writer.o:  writer/writer.c
	$(CC) $(CFLAGS) -c writer/writer.c

file_list.o:  file_list/file_list.c
	$(CC) $(CFLAGS) -c file_list/file_list.c

utils.o:  utils/utils.c
	$(CC) $(CFLAGS) -c utils/utils.c
	

clean: 
	$(RM) *.o common/* exe/*