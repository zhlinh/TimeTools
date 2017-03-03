CPU_STRESS=cpu_stress
CC = gcc
RM = rm

all:
	@echo $(MAKE)...
	$(CC) $(CPU_STRESS).c -o $(CPU_STRESS) -lrt

clean:
	@echo $(MAKE)...
	$(RM) -rf $(CPU_STRESS)

.PHONY: all clean

