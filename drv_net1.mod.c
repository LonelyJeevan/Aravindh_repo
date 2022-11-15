#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x27126e5a, "module_layout" },
	{ 0x3339318a, "unregister_netdev" },
	{ 0xda576b4c, "register_netdev" },
	{ 0x88e616da, "free_netdev" },
	{ 0xd73c1f93, "alloc_netdev_mqs" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0x16a0370b, "consume_skb" },
	{ 0xbcc67f2d, "netif_rx" },
	{ 0xeaaf3246, "eth_type_trans" },
	{ 0x92ea52a9, "skb_put" },
	{ 0x7e454aa3, "__netdev_alloc_skb" },
	{ 0x15ba50a6, "jiffies" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0x76b2db57, "ether_setup" },
	{ 0x70dae7ef, "skb_push" },
	{ 0xc5850110, "printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "4BE42DB538A368171B73BA9");
