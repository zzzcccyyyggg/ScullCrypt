#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
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
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x6bd0e573, "down_interruptible" },
	{ 0xcf2a6966, "up" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0x37a0cba, "kfree" },
	{ 0x92650f08, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x92997ed8, "_printk" },
	{ 0x4b51c401, "kmalloc_caches" },
	{ 0x323c05f9, "kmalloc_trace" },
	{ 0xe0d432ad, "crypto_alloc_shash" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0xde1b8fce, "crypto_destroy_tfm" },
	{ 0xa916b694, "strnlen" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0x107fd4ca, "crypto_shash_update" },
	{ 0xb85c0970, "crypto_shash_final" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x754d539c, "strlen" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0xa19b956, "__stack_chk_fail" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0xb21da122, "filp_open" },
	{ 0xff8c7332, "kernel_read" },
	{ 0xe9340b0f, "filp_close" },
	{ 0x3fd78f3b, "register_chrdev_region" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x2528433a, "cdev_init" },
	{ 0x65ffc5b5, "cdev_add" },
	{ 0xeac1dec1, "param_ops_int" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xace58596, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "89B561518A3C359CF591B87");
