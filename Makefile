# This is a simple Makefile for small projects.  When you 
# type make at the command prompt, it will compile the code.
# For more depth, see http://www.gnu.org/software/make/manual/make.html

CC=gcc
CFLAGS=-g -Wall -Wextra -lcurl -lssl -lcrypto

all: webspider printquery

webspider: webspider.c
	$(CC) -std=gnu89 -o webspider webspider.c htmlstreamparser.c  $(CFLAGS)


printquery: printquery.c
	$(CC) -std=gnu89 -o printquery printquery.c htmlstreamparser.c  $(CFLAGS)

