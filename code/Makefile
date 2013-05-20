OBJS = sim
SRCS = simulation.c evolution.c scenario.c
TESTS = testevolution #testscenario

FANNLIBDIR+=fann-libs/lib/
SFMTDIR+=SFMT-libs/
#LIBS+=fann
STATICLIBS=$(FANNLIBDIR)/libfann.a

INCLUDES+=-I fann-libs/include/ 
INCLUDES+=-I $(SFMTDIR)
CFLAGS+=-Wall
CFLAGS+=-Werror
#CFLAGS+=-g 
CFLAGS+=-O2
#CFLAGS+=-pedantic 
#CFLAGS+=-dynamiclib

DEFINES+=-D MEXP=19937

SFMT_SRC+=$(SFMTDIR)/SFMT.c 

all: $(OBJS)

test: $(TESTS)

clean:
	rm -f $(OBJS) $(TESTS)

$(OBJS): $(SRCS)
	gcc $(CFLAGS) $(DEFINES) $(INCLUDES) -o $(OBJS) $(SRCS) $(SFMT_SRC) $(STATICLIBS)
	touch test*.c

###
# define tests here, list them at beginning

testevolution: testevolution.c 
	gcc -D DBG $(CFLAGS) $(DEFINES) $(INCLUDES) -o $@ $? $(SFMT_SRC) $(STATICLIBS)

testscenario: testscenario.c 
	gcc -D DBG $(CFLAGS) $(DEFINES) $(INCLUDES) -o $@ $? $(SFMT_SRC) $(STATICLIBS)

linux: $(SRCS)
	gcc -Wall -Werror -O2 -D MEXP=19937 -I fann-libs_linux/include/ -I SFMT-libs/ -o sim simulation.c evolution.c scenario.c SFMT-libs/SFMT.c -L fann-libs_linux/lib/ -lfann -lm
	echo "dont forget to export LD_LIBRARY_PATH=fann-libs_linux/lib/"

