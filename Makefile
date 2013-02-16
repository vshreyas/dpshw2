CC = gcc

TARGET = sample_client sample_server

CFLAGS += -g -I/opt/local/include
LDFLAGS += -g -lprotobuf-c -L/opt/local/lib

all:	$(TARGET)

$(TARGET): 	lsp.o lspmessage.pb-c.o

%.o:	%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o 
	rm -f $(TARGET)

