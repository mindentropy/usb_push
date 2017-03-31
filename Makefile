CFLAGS=-g -Wall
LDFLAGS=-lusb-1.0

all: cscope_create usb_push

usb_push: usb_push.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

usb_push.o: usb_push.c
	$(CC) $(CFLAGS) -c $^

cscope_create:
	cscope -R -q -b 

.PHONY: clean

clean:
	rm -fv *.o usb_push *.out
