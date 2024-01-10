#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

#undef unix
struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = __stringify(KBUILD_MODNAME),
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
};

static const struct modversion_info ____versions[]
__attribute_used__
__attribute__((section("__versions"))) = {
	{ 0x3edc613c, "struct_module" },
	{ 0xc192d491, "unregister_chrdev" },
	{ 0xf20dabd8, "free_irq" },
	{ 0xbce89f77, "__wake_up" },
	{ 0x9941ccb8, "free_pages" },
	{ 0x407a28d8, "interruptible_sleep_on_timeout" },
	{ 0x1244ed45, "pci_set_master" },
	{ 0x9874c61e, "pci_bus_write_config_word" },
	{ 0x26e96637, "request_irq" },
	{ 0x4784e424, "__get_free_pages" },
	{ 0xa64656e2, "pci_bus_read_config_word" },
	{ 0xb09d8eda, "pci_find_device" },
	{ 0xfaa7a5e9, "register_chrdev" },
	{ 0x2f287f0d, "copy_to_user" },
	{ 0xd6c963c, "copy_from_user" },
	{ 0x1b7d4074, "printk" },
};

static const char __module_depends[]
__attribute_used__
__attribute__((section(".modinfo"))) =
"depends=";

