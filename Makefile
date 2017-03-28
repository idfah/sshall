CC = /usr/bin/gcc
CFLAGS = -Wall -O0
CPPFLAGS =
LDFLAGS = -lm


APPS = sshall rshall
MODS = debug.o ioredir.o colorset.o
  
all: $(APPS)
    
mods: $(MODS)
	@ touch mods

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

%: %.c mods
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@ $(MODS) $(LDFLAGS)

rshall: sshall.c mods
	$(CC) $(CFLAGS) $(CPPFLAGS) -DRSH sshall.c -o rshall $(MODS) $(LDFLAGS)
  
clean: 
	rm -f mods $(MODS)
    
remove: clean
	rm -f $(APPS)
  
.PHONY: all clean remove
