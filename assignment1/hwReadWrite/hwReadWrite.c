/**
 * Includes from the linux/kernel libraries 
 * */
#include <linux/kernel.h>    
#include <linux/module.h>    
#include <linux/kobject.h>   
#include <linux/device.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/unistd.h>

/**
 * Physical to virtual and virtual to physical address mapping macros
 * */
#define IO_BASE		0xF0000000
#define io_p2v(x) 	(IO_BASE | (((x) & 0xff000000) >> 4) | ((x) & 0x000fffff))
#define io_v2p(x) 	((((x) & 0x0ff00000) << 4) | ((x) & 0x000fffff))

/**
 * Defines for our kernel attributes
 * */
#define max_data 1024
#define kernel_dir	"hwReadWrite"
#define	kernel_file	"result"

#define msg_param_offset 2

static ssize_t used_buffer_size = 0;

volatile int errno = 0;

/**
 * Handles the read function
 * buffer: the incoming message to be handled
 * */
static void handle_read(const char *buffer)
{	
	int i;	
	char *endPtr;
	int registers_to_read = simple_strtol(buffer, &endPtr, 10);
	endPtr++; //Set endPtr ahead one position of the space in the message	

	int start_address = simple_strtol(endPtr, NULL, 16);
	printk(KERN_INFO "Reading %i memory registers, starting at address 0x%08x\n", registers_to_read, start_address);
	
	for(i = 0; i < registers_to_read; i++)
	{
		unsigned int output;
		int current_address = start_address + i;
		output = ioread32(io_p2v(current_address));
		printk(KERN_INFO "Output read at address 0x%08x: %u\n", current_address, output);
	}
}

/**
 * Handles the write function
 * buffer: the incoming message to be handled
 * */
static void handle_write(const char* buffer)
{
	char *endPtr;
	int address_to_write = simple_strtol(buffer, &endPtr, 16);
	endPtr++; //Set endPtr ahead one position of the space in the message	
	int value_to_write = simple_strtol(endPtr, NULL, 16);
	
	printk( KERN_INFO "Writing value 0x%x to memory address 0x%08x\n", value_to_write, address_to_write);
	
	iowrite32(value_to_write, io_p2v(address_to_write));
}

/**
 * This method is called when the user calls echo on our kernel module
 * *dev and *attr: not yet required for our functionality
 * buffer: the message that is being echoed to our kernel
 * size: the size of the message.
 * return value: if return != count, then sysfs will call echo with the remainder of the message. (should not be >1024 bytes)
 * */
static ssize_t sysfs_store(struct device *dev, struct device_attribute *attr, const char *buffer, size_t count)
{
    if ( count > max_data )
    {
		used_buffer_size = max_data;
		printk(KERN_INFO "Input is too large: %d is max, %d was supplied", max_data, count);
	}
	else
	{
		used_buffer_size = count;
	}
	
	if(strncmp(buffer, "r", 1) == 0)
	{
		handle_read(&buffer[msg_param_offset]);
	}
	else if(strncmp(buffer, "w", 1) == 0)
	{
		handle_write(&buffer[msg_param_offset]);
	}
	else
	{
		printk(KERN_INFO "Input is not according to the protocol. Input: %s\n" , buffer);
		printk(KERN_INFO "If you wish to read:\n");
		printk(KERN_INFO "\"r <amount of registers to read> <physical address of register to start at>\"\n");
		printk(KERN_INFO "Example: echo \"r 8 0x40024000\"\n\n");
		printk(KERN_INFO "If you wish to write:\n");
		printk(KERN_INFO "\"w <physical address of register to write to> <value to write>\"\n");
		printk(KERN_INFO "Example: echo \"w 0x40024000 0x222\"\n");
	}
	
    return used_buffer_size;
}

/**
 * result =  our file where we store user input  /sys/kernel/hwReadWrite/result
 * S_IWUGO =  our kernel only requires write permissions for that reason S_IWUGO
 * NULL =  we dont have to read from our kernel file
 * sysfs_store =  The method that should be called when we echo to the kernel
 **/
static DEVICE_ATTR(result, S_IWUGO, NULL, sysfs_store);
static struct attribute *attrs[] = { &dev_attr_result.attr, NULL};
static struct attribute_group attr_group = {.attrs = attrs,};
static struct kobject *this_obj = NULL;
    
int __init sysfs_init(void)
{
    int result = 0;
    
	/**
	 * Here we write our kernel into the kobject pointer
	 * the 2nd parameter here implies that our kernel should be stored in /sys/kernel/
	 **/
    this_obj = kobject_create_and_add(kernel_dir, kernel_kobj);
    
    if (this_obj == NULL)
    {
        printk (KERN_INFO "%s kernel module could not be created \n", kernel_dir);
        return -ENOMEM;
    }
	
	/**
	 * inject our result file into the kernel folder that was created earlier
	 **/
    result = sysfs_create_group(this_obj, &attr_group);
    if (result != 0)
    {
        printk (KERN_INFO "%s could not create kernel filesystem %d\n", kernel_file, result);
        kobject_put(this_obj);
        return -ENOMEM;
    }

    printk(KERN_INFO "/sys/kernel/%s/%s created\n", kernel_dir, kernel_file);
    return result;
}

void __exit sysfs_exit(void)
{
    kobject_put(this_obj);
    printk (KERN_INFO "/sys/kernel/%s/%s removed\n", kernel_dir, kernel_file);
}

module_init(sysfs_init);
module_exit(sysfs_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Johri&Mark with help from Freddy");
MODULE_DESCRIPTION("hwReadWrite");
