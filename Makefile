CFLAGS=-g -Wall
LDFLAGS=-lusb

all: usb_push cscope_create

usb_push: usb_push.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

usb_push.o: usb_push.c
	$(CC) $(CFLAGS) -c $^

cscope_create:
	cscope -R -q -b 

.PHONY: clean

clean:
	rm -v *.o usb_push *.out
