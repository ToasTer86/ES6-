// includes 
#include <linux/kernel.h>    
#include <linux/module.h>    
#include <linux/kobject.h>   
#include <linux/device.h>
#include <linux/io.h>
#include <linux/init.h>

// macros for physical & virtual memory mapping
#define IO_BASE		0xF0000000
#define io_p2v(x) 	(IO_BASE | (((x) & 0xff000000) >> 4) | ((x) & 0x000fffff))
#define io_v2p(x) 	((((x) & 0x0ff00000) << 4) | ((x) & 0x000fffff))

//Defines for sysfs 
#define max_data 1024
#define kernel_dir	"hwReadWrite"
#define	kernel_file	"result"

#define n_registers_location 2
#define r_address_location 4

#define w_address_location 2
#define w_value_location 13

static ssize_t used_buffer_size = 0;

// this method is used when the user wants to read n registers // 
static void handle_read(const char *buffer)
{
	int registers_to_read = simple_strtol(&buffer[n_registers_location], NULL, 10);
	int i;
	int start_address = simple_strtol(&buffer[r_address_location], NULL, 16);
	printk(KERN_INFO "Reading %i memory registers, starting at address 0x%08x\n", registers_to_read, start_address);
	
	for(i = 0; i < registers_to_read; i++)
	{
		unsigned int output;
		int current_address = start_address + i;
		output = ioread32(io_p2v(current_address));
		printk(KERN_INFO "Output read at address 0x%08x: %u\n", current_address, output);
	}
}

// this method is used when the user wants to write to a register //
static void handle_write(const char* buffer)
{
	int address_to_write = simple_strtol(&buffer[r_address_location], NULL, 16);
	int value_to_write = simple_strtol(&buffer[w_value_location], NULL, 16);
	
	printk( KERN_INFO "Writing value 0x%x to memory address 0x%08x\n", value_to_write, address_to_write);
	
	iowrite32(value_to_write, io_p2v(address_to_write));
}

// this method is called when the user calls 'echo' on the kernel module //
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
		handle_read(buffer);
	}
	else if(strncmp(buffer, "w", 1) == 0)
	{
		handle_write(buffer);
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

static DEVICE_ATTR(result, S_IWUGO, NULL, sysfs_store);
static struct attribute *attrs[] = { &dev_attr_result.attr, NULL};
static struct attribute_group attr_group = {.attrs = attrs,};
static struct kobject *this_obj = NULL;
    
int __init sysfs_init(void)
{
    int result = 0;

    this_obj = kobject_create_and_add(kernel_dir, kernel_kobj);
    if (this_obj == NULL)
    {
        printk (KERN_INFO "%s kernel module could not be created \n", kernel_dir);
        return -ENOMEM;
    }

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
