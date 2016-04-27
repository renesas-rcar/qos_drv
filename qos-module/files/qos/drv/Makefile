qos-y := qos_drv.o qos_core.o
obj-m := qos.o

ccflags-y += -I$(KERNELSRC)/include

all:
	$(CP) ./qos.h $(KERNELSRC)/include
	make -C $(KERNELDIR) M=$(shell pwd) modules

clean:
	make -C $(KERNELDIR) M=$(shell pwd) clean



