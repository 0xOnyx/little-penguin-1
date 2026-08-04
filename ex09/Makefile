obj-m += main.o

# fs/mount.h n'est pas un header public : on ajoute le repertoire fs/
# des sources kernel au chemin d'inclusion.
ccflags-y += -I$(srctree)/fs

KVERSION := $(shell uname -r)
KDIR     := /lib/modules/$(KVERSION)/build

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

re: clean all

.PHONY: all clean re
