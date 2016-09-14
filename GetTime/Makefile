HYPERCLL_TEST = hypercall_test
IO_TEST = io_test
CC = gcc
RM = rm

all:
	@echo $(MAKE)...
	$(CC) $(HYPERCLL_TEST).c -o $(HYPERCLL_TEST) -lrt
	$(CC) $(IO_TEST).c -o $(IO_TEST) -lrt

clean:
	@echo $(MAKE)...
	$(RM) -rf $(HYPERCLL_TEST) $(IO_TEST)

.PHONY: all clean

