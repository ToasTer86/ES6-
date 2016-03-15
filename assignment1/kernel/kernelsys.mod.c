#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x45772e10, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x4dc514c7, __VMLINUX_SYMBOL_STR(kobject_put) },
	{ 0x96dc91bc, __VMLINUX_SYMBOL_STR(sysfs_create_group) },
	{ 0xe0c4c876, __VMLINUX_SYMBOL_STR(kobject_create_and_add) },
	{ 0x38f83c61, __VMLINUX_SYMBOL_STR(kernel_kobj) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x50eedeb8, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xb4390f9a, __VMLINUX_SYMBOL_STR(mcount) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "F5D8C4E6225DA7291B7E67F");
