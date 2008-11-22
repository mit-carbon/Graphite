TARGET = rltotiff
OBJS   = rltotiff.o tiff_rgba_io.o

include ../../splash_support/Makefile.config

LDFLAGS := $(LDFLAGS) -ltiff

clean:
	rm -rf $(OBJS) $(TARGET)
