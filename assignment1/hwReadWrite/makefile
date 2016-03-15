obj-m += hwReadWrite.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

cc:
	make ARCH=arm CROSS_COMPILE=arm-linux- -C ~/felabs/sysdev/tinysystem/linux-2.6.34/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
