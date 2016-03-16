#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/io.h>
 
//#define io_p2v(x) (0xf0000000 | (((x) & 0xfff00000) >> 4) | ((x) & 0x0000ffff))
//#define io_v2p(x) (             (((x) & 0x0fff0000) << 4) | ((x) & 0x0000ffff))
 
#define IO_BASE         0xF0000000
#define io_p2v(x) (IO_BASE | (((x) & 0xff000000) >> 4) | ((x) & 0x000fffff))
#define io_v2p(x) ((((x) & 0x0ff00000) << 4) | ((x) & 0x000fffff))
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Merint van Senus");
 
static int __init hello_5_init(void)
{   
//iowrite32(0x40024000, 28);
    int a = 0x40024000;
 
    unsigned long long test = ioread32((int*)io_p2v(a));
    printk("Real addres value: ");
    printk("%llu\n", test);
 
     
 
    return 0;
}
 
static void __exit hello_5_exit(void)
{
  printk(KERN_INFO "Exiting kernel...ls\n");
}
 
module_init(hello_5_init);
module_exit(hello_5_exit);