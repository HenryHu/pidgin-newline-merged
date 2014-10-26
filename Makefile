SRCS=newline_merged.c
OBJS=newline_merged.o
TARGET=libnewlinemerged.so

$(TARGET): $(SRCS)
	cc $(SRCS) -o $@ -shared `pkg-config --cflags --libs purple` -fPIC

install:
	cp $(TARGET) $(HOME)/.purple/plugins/

clean:
	rm -f $(OBJS) $(TARGET)
