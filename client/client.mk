
PROGRAM		=	echo_client

CC			=	gcc
LINKER		=	$(CC)

CCFLAGS		=	-c -g -Wall -Wextra	-pedantic -Wno-unused-parameter

OBJS		=	echo_client.o

.SUFFIXES:	.exe .o .c .pc

.c.o:
	@$(CC) $(CCFLAGS) $<

$(PROGRAM):	$(OBJS)
			@echo "Loading $@ ... "
			@$(LINKER) -o $@ $^
			@echo "done"

.PHONY: all
all:		$(PROGRAM)

.PHONY: clean
clean:
	@rm -f $(PROGRAM)
			@rm -f $(OBJS)

