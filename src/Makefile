CC = gcc
CFLAGS = -O0 -g -I.
LDFLAGS = -ludev

SRC = pn532_com.c pn532_hf15.c pn532_test.c crc16.c
OBJ = $(SRC:.c=.o)
TARGET = pn532_test

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
