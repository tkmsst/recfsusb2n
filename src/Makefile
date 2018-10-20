CC	= gcc
CFLAGS	= -O2 -Wall
OBJS	= main.o utils.o message.o osdepend.o usbdevfile.o usbops.o tsthread.o em287x.o em287x_usb.o tc90522.o mxl136.o tda20142.o tssplitter_lite.o
LIBS	= -lpthread
TARGET	= recfsusb2n
CONF = $(TARGET).conf
PREFIX = /usr/local

ifdef B25
LIBS	+= -laribb25
CFLAGS_utils	+= -DSTD_B25
endif
ifeq ($(shell getconf LONG_BIT),32)
CFLAGS	+= -D_FILE_OFFSET_BITS=64
endif

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

uninstall:
	rm -vf $(PREFIX)/bin/$(TARGET)
	rm -vf $(PREFIX)/bin/$(CONF)

install: uninstall
	install -m 755 $(TARGET) $(CONF) $(PREFIX)/bin

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

depend:
	$(CC) -MM $(OBJS:.o=.c) > Makefile.dep

-include Makefile.dep

utils.o :
	$(CC) $(CFLAGS) $(CFLAGS_utils) -c -o $@ $(@:.o=.c)
