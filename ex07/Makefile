obj-m += main.o

K-VERSION = $(shell uname -r)

all:
	make -C /lib/modules/$(K-VERSION)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(K-VERSION)/build M=$(PWD) clean


re : clean all