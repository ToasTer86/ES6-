// includes 
#include <linux/kernel.h>    
#include <linux/module.h>    
#include <linux/kobject.h>   
#include <linux/device.h>
#include <linux/io.h>
#include <linux/init.h>

// macro's for physical & virtual memory mapping
#define IO_BASE         0xF0000000
#define io_p2v(x) (IO_BASE | (((x) & 0xff000000) >> 4) | ((x) & 0x000fffff))
#define io_v2p(x) ((((x) & 0x0ff00000) << 4) | ((x) & 0x000fffff))

//Defines for sysfs 
#define max_data 1024
#define register_address_size 10
#define dir		"buffer"
#define	file	"data" 

static ssize_t used_buffer_size = 0;
static char write_buffer[max_data +1];  // +1 for \0' terminator so that the %s modifier may be used // 

// this method is called when the user requests cat + commando //
static ssize_t sysfs_show(struct device *dev, struct device_attribute *attr,char *buffer)
{
    return sprintf(buffer, "%s", write_buffer);
}

// this method is used when the user wants to read n registers // 
static void handle_read(const char *buffer)
{
	//int registers_to_read = (int)buffer[1];
	
	char kernel_to_read[register_address_size +1];	//+1 escape char
	strcpy(kernel_to_read, &buffer[3]);
	
	int output;
	output = ioread32(io_p2v(0x40024000));
	sprintf(buffer, "%d" , output);
}


// this method is used when the user wants to write to a register //
static void handle_write(const char* buffer)
{
	
}

// this method is called when the user wants to write in the kernel "echo"  //
static ssize_t sysfs_store(struct device *dev, struct device_attribute *attr, const char *buffer, size_t count)
{
	int output;
					
    if ( count > max_data )
    {
		used_buffer_size = max_data;
		printk(KERN_INFO "Input is too large: %d is max, %d was supplied", max_data, count);
	}
	else
	{
		used_buffer_size = count;
	}

	// case statement to check if the user wants to write or read registers 

    switch(buffer[0]){
		case 'r':
			output = ioread32(io_p2v(0x40024000));			
			break;
			
		case 'w':
			handle_write(buffer);			// check implementation at top of code
			break;
			
		default:
			printk(KERN_INFO "Input is not according to the protocol: %s" , buffer);
			break;
		}
    
    
    memcpy(write_buffer, &output, used_buffer_size);
    write_buffer[used_buffer_size] = '\0'; 
    return used_buffer_size;
}

static DEVICE_ATTR(data, S_IWUGO | S_IRUGO, sysfs_show, sysfs_store);
static struct attribute *attrs[] = { &dev_attr_data.attr, NULL};
static struct attribute_group attr_group = {.attrs = attrs,};
static struct kobject *this_obj = NULL;
    
int __init sysfs_init(void)
{
    int result = 0;

    this_obj = kobject_create_and_add(dir, kernel_kobj);
    if (this_obj == NULL)
    {
        printk (KERN_INFO "%s kernel module could not be created \n", file);
        return -ENOMEM;
    }

    result = sysfs_create_group(this_obj, &attr_group);
    if (result != 0)
    {
        printk (KERN_INFO "%s could not create kernel filesystem %d\n", file, result);
        kobject_put(this_obj);
        return -ENOMEM;
    }

    printk(KERN_INFO "/sys/kernel/%s/%s created\n", dir, file);
    return result;
}

void __exit sysfs_exit(void)
{
    kobject_put(this_obj);
    printk (KERN_INFO "/sys/kernel/%s/%s removed\n", dir, file);
}

module_init(sysfs_init);
module_exit(sysfs_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Johri&Mark with help from Freddy");
MODULE_DESCRIPTION("hwReadWrite");
