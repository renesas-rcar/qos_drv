qos-y := qos_drv.o qos_core.o
obj-m := qos.o

ccflags-y += -I$(KERNELSRC)/include
QOS_MODULE =

all:
	make -C $(KERNELSRC) M=$(shell pwd) modules

clean:
	make -C $(KERNELSRC) M=$(shell pwd) clean

install:
	$(CP) ./qos.h $(KERNELSRC)/include/$(QOS_MODULE)
	$(CP) ./qos_public_common.h $(KERNELSRC)/include/$(QOS_MODULE)
	$(CP) ./qos_public_common.h $(INCSHARED)/$(QOS_MODULE)

