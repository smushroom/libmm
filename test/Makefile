ASM:=nasm
CC:=gcc 
AR:=ar
LD:=ld
MAKE:=make

CFLAGS=-g -fno-builtin -Wall -ggdb  -O0

INCLUDE=-I ../src
LDFLAGS= -L ../src -lmm -lm -lpthread

TEST_PAGE:=test_page
TEST_SLAB:=test_slab
TEST_RTPLUS:=rtest
TEST_WTPLUS:=wtest
TEST_THREAD:=test_thread
TEST_SERVER:=test_server
TEST_MALLOC:=test_malloc

all:$(TEST_PAGE) $(TEST_SLAB) $(TEST_RTPLUS) $(TEST_WTPLUS) $(TEST_THREAD) $(TEST_SERVER) $(TEST_MALLOC)

%.o:%.c
	$(CC) -c $(CFLAGS) $< $(INCLUDE)

$(TEST_PAGE):test_page.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_SLAB):test_slab.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_RTPLUS):rtest.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_WTPLUS):wtest.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_THREAD):test_thread.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_SERVER):test_server.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_MALLOC):test_malloc.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TEST_SLAB) $(TEST_PAGE) $(TEST_RTPLUS) $(TEST_WTPLUS) $(TEST_THREAD) $(TEST_SERVER) $(TEST_MALLOC)
	rm -f *.o 
	rm -f *.idx

