
PROGRAM		=	echo_server

CC			=	gcc
CCFLAGS		=	-g -Wall -Wextra -pedantic -Wno-unused-parameter

LINKER		=	$(CC)

OBJS		=	echo_server.o

.SUFFIXES: .o .c .s

.c.o:
	@$(CC) $(CCFLAGS) -c $<

$(PROGRAM):	$(OBJS)
	@echo "Loading $@ ... "
	@$(LINKER) -o $@ $^
	@echo "done"

.PHONY: all
all:	$(PROGRAM)

.PHONY: clean
clean:
	@rm -f $(PROGRAM)
	@rm -f $(OBJS)

