CC = gcc
CFLAGS  = -g -Wall
RM = rm -rf

default: mirror_client

mirror_client:  mirror_client.o reader.o writer.o file_list.o utils.o
	$(CC) $(CFLAGS) -o mirror_client mirror_client.o reader.o writer.o file_list.o utils.o

mirror_client.o:  mirror_client.c
	$(CC) $(CFLAGS) -c mirror_client.c

reader.o:  reader/reader.c
	$(CC) $(CFLAGS) -c reader/reader.c

writer.o:  writer/writer.c
	$(CC) $(CFLAGS) -c writer/writer.c

file_list.o:  file_list/file_list.c
	$(CC) $(CFLAGS) -c file_list/file_list.c

utils.o:  utils/utils.c
	$(CC) $(CFLAGS) -c utils/utils.c



clean: 
	$(RM) mirror_client *.o common/* mirror