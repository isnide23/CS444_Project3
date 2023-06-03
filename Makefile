pcseml: pcseml.c eventbuf/eventbuf.c
	gcc -Wall -Wextra -o $@ $^

pcseml.zip:
	rm -f $@
	zip $@ Makefile pcseml.c