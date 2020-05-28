CFLAGS = -g3 -Wall -Wextra -Wconversion -Wcast-qual -Wcast-align -g
CFLAGS += -Winline -Wfloat-equal -Wnested-externs
CFLAGS += -pedantic -std=gnu99 -Werror -D_GNU_SOURCE
CC = gcc
SHELL_SOURCE_CODE = sh.c
JOBS_SOURCE_CODE = jobs.c
EXECS = 33sh 33noprompt
PROMPT = -DPROMPT
.PHONY: all clean



all: $(EXECS)
	/course/cs0330/bin/cs0330_cleanup_shell

33sh:$(SHELL_SOURCE_CODE) $(JOBS_SOURCE_CODE)
	$(CC) $(CFLAGS) $(PROMPT) $(SHELL_SOURCE_CODE) $(JOBS_SOURCE_CODE) -o $@

33noprompt:$(SHELL_SOURCE_CODE) $(JOBS_SOURCE_CODE)
	$(CC) $(CFLAGS) $(SHELL_SOURCE_CODE) $(JOBS_SOURCE_CODE) -o $@

clean:
	rm -f 33sh
	rm -f 33noprompt

