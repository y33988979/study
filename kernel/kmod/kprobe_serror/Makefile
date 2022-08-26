BASEINCLUDE 	?= /usr/src/kernels/$(shell  uname -r)/
proctest-objs 	:= kprobe_serror.o
obj-m			:= kprobe_serror.o

all:
	$(MAKE) -C $(BASEINCLUDE) M=$(PWD) modules;

clean:
	$(MAKE) -C $(BASEINCLUDE) M=$(PWD) clean;
	rm -rf *.ko
