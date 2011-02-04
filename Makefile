CC := gcc
LD := ld
SRC := tftp.c
OBJ := $(SRC:%.c=%.o)
OS=$(shell uname)
TARGET := tftp

.PHONY: depend clean

DEFS=-Wall

# Automatically detect SunOS or Linux:
ifeq ($(OS),SunOS)
DEFS=-Wall -DSUNOS_5
CLIBS=-lsocket -lnsl -lresolv
endif

default: $(TARGET)

# Insert your dependencies here
$(OBJ): $(SRC)
	$(CC) $(DEFS) -c -o $@ $<

$(TARGET): $(OBJ)
	$(CC) $(DEFS) $(CLIBS) -o $@ $<

depend:
	makedepend -Y./ $(SRC) &> /dev/null

clean:
	rm -f $(OBJ) $(TARGET) *~

# DO NOT DELETE

tftp.o: tftp.h
