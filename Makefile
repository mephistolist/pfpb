CC = cc
CFLAGS = -Wall -Wextra -Werror -pedantic -O2 -pipe -march=native --std=c17 -march=native -fPIC -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Wformat-security -flto -fvisibility=hidden -fno-common -Wshadow -fstrict-aliasing -Wuninitialized
CFLAGS_RETRIEVE = $(CFLAGS) `pkg-config --cflags libcurl`
LDFLAGS_RETRIEVE = `pkg-config --libs libcurl`
LDFLAGS = -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now,-lssl,-lcrypto,-lz -fPIE -pie -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now $(LDFLAGS_RETRIEVE)

# Source files and target
SRCS = retrieve.c copy.c parser.c dupe_parse.c table_loader.c pfcount.c main.c
OBJS = $(SRCS:.c=.o)
TARGET = pfpb

# Default rule
all: $(TARGET)

# Rule to build the final binary
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# Rule to compile retrieve.c with libcurl flags
retrieve.o: retrieve.c
	$(CC) $(CFLAGS_RETRIEVE) -c $< -o $@

# General rule to compile all other .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Install rule
install: $(TARGET)
	mkdir -p /var/pfpb/gzips
	mkdir -p /var/pfpb/tables
	cp -v config.txt /var/pfpb
	install -m 755 $(TARGET) /usr/sbin/$(TARGET)
	pfpb update
	Install is complete.

# Clean rule to remove generated files
clean:
	rm -f $(OBJS) $(TARGET)
