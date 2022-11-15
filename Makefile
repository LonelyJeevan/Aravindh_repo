obj-m = drv_net1.o
KVERSION = $(shell uname -r)

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean 
	rm -f Module.symvers
	rm -f *.ur-safe 

