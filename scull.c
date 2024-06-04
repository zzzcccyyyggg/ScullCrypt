#include <linux/module.h>
// #include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/crypto.h>
#include <linux/err.h>
#include <asm/uaccess.h>
#include <crypto/hash.h>

#include "scull.h"		/* local definitions */


static int scull_major = SCULL_MAJOR;
static int scull_minor = SCULL_MINOR;
static int scull_num_devs = SCULL_NUM_DEVS;

static int scull_quantum = SCULL_QUANTUM;
static int scull_qset    = SCULL_QSET;

struct scull_dev *scull_devices;
module_param(scull_major, int, 0664);
MODULE_PARM_DESC(scull_major, "Major number for the scull driver");

module_param(scull_minor, int, 0664);
MODULE_PARM_DESC(scull_minor, "Minor number for the scull driver");

module_param(scull_quantum, int, 0664);
MODULE_PARM_DESC(scull_quantum, "Quantum size for the scull driver");

module_param(scull_qset, int, 0664);
MODULE_PARM_DESC(scull_qset, "Quantum set size for the scull driver");

MODULE_LICENSE("Dual BSD/GPL");

struct file_operations scull_fops = { 
    .owner = THIS_MODULE,
    .llseek = scull_llseek,
    .read   = scull_read,
    .write  = scull_write,
    .open   = scull_open,
    .release= scull_release,
};

//全局变量
char lkey[16];
char rkey[16];
char key[32];
char Sbox[8][16] = {
    {
        0x03, 0x08, 0x0F, 0x01, 0x0A, 0x06, 0x05, 0x0B,
        0x0E, 0x0D, 0x04, 0x02, 0x07, 0x00, 0x09, 0x07
    },
    {
        0x0F, 0x0C, 0x02, 0x07, 0x09, 0x00, 0x05,0x0A,
        0x01, 0x0B, 0x0E, 0x08, 0x06, 0x0D, 0x0C, 0x0D
    },
    {
        0x08, 0x06, 0x07, 0x09, 0x03, 0x0C, 0x0A,
        0x0F, 0x0D, 0x01, 0x0E, 0x04, 0x00,0x0B, 0x03, 0x07
    },
    {
        0x00, 0x0F, 0x0B, 0x08, 0x0C, 0x09, 0x06, 0x03,
        0x0D, 0x01, 0x02, 0x04, 0x0A, 0x07, 0x04, 0x01
    },
    {
        0x01, 0x0F, 0x08, 0x03, 0x0C, 0x00, 0x0B, 0x06,
        0x02, 0x05, 0x04, 0x0A, 0x09, 0x0E, 0x05, 0x0A
    },
    {
        0x0F, 0x05, 0x02, 0x0B, 0x04, 0x0A, 0x09, 0x0C,
        0x00, 0x03, 0x0E, 0x08, 0x0D, 0x06, 0x02, 0x00
    },
    {
        0x07, 0x02, 0x0C, 0x05, 0x08, 0x04, 0x06, 0x0B,
        0x0E, 0x09, 0x01, 0x0F, 0x0D, 0x03, 0x05, 0x05
    },
    {
        0x01, 0x0D, 0x0F, 0x00, 0x0E, 0x08, 0x02, 0x0B,
        0x07, 0x04, 0x0C, 0x00, 0x09, 0x03, 0x0E, 0x06
    }
};

//全局变量

int scull_trim_mem(struct scull_dev *dev){
    struct scull_qset *next,*dptr;
    int qset = dev->qset;
    int i;
    for(dptr = dev->data;dptr;dptr = next){
        if(dptr->data){
            for(i = 0;i < qset;i++){
                kfree(dptr->data[i]);
            }
            kfree(dptr->data);
            dptr->data = NULL;
        }
        next = dptr->next;
        kfree(dptr);
    }
    dev->size = 0;
    dev->quantum = scull_quantum;
    dev->qset = scull_qset;
    dev->data = NULL;
    return 0;
}

/*将 c_dev 结构到这个 scull_devices */
static void scull_setup_cdev(struct scull_dev *dev,int index){

    int err,dev_t = MKDEV(scull_major,scull_minor + index);
    cdev_init(&dev->cdev,&scull_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops   = &scull_fops;
    err = cdev_add(&dev->cdev,dev_t,1);
    if(err){
        printk("error %d adding scull%d",err,index);
    }

}


/**
 * @brief 清理和卸载 scull 驱动程序
 */
void scull_cleanup_module(void)
{
    int i;
    dev_t devno = MKDEV(scull_major, scull_minor); // 构造设备号

    /* 删除字符设备条目 */
    if (scull_devices) {
        for (i = 0; i < scull_num_devs; i++) {
            scull_trim_mem(scull_devices + i); // 修剪每个设备的内存
            cdev_del(&scull_devices[i].cdev); // 删除字符设备条目
        }
        kfree(scull_devices); // 释放设备数组的内存
    }

    /* 如果注册失败,cleanup_module 就不会被调用 */
    unregister_chrdev_region(devno, scull_num_devs); // 注销字符设备驱动程序

    /* 调用其他模块的清理函数 */
    //scull_p_cleanup();
    //scull_access_cleanup();
}

int scull_openkey_init(struct file *filp){
    char *buf;
    char *hash;
    struct file *file;
    struct crypto_shash *tfm;
    struct shash_desc *shash;
    loff_t pos;
    int buflen;
    int result;
    /* Allocate buffer for reading file content */
    printk(KERN_INFO "scull_init: start running key init\n");
    buf = kmalloc(PAGE_SIZE,GFP_KERNEL);
    hash = kmalloc(32,GFP_KERNEL);
    if(!buf || !hash){
        printk(KERN_INFO "scull_init: buf and hash malloc failed\n");
        return 0;
    }
    memset(buf,0,PAGE_SIZE);
    memset(hash,0,32);
    // dentry_path_raw(filp->f_path.dentry,buf,buflen);
    buf[0] = 's';
    buf[1] = 'c';
    buf[2] = 'u';
    buf[3] = 'l';
    buf[4] = 'l';
    tfm = crypto_alloc_shash("sha256", 0, 0);
    if (IS_ERR(tfm)) {
        result = PTR_ERR(tfm);
        printk(KERN_INFO "failed 2");
        goto fail_buf;
    }

    shash = kmalloc(sizeof(struct shash_desc{}) + crypto_shash_descsize(tfm), GFP_KERNEL);
    if (!shash) {
        result = -ENOMEM;
        printk(KERN_INFO "failed 3");
        goto fail_tfm;
    }

    shash->tfm = tfm;
    printk(KERN_INFO "scull_init: scull_init_key finished\n");
    /* Compute hash of the key file content */
    crypto_shash_init(shash);
    crypto_shash_update(shash, buf, strlen(buf));
    crypto_shash_final(shash, hash);
    // for (int i=0;i<16;i++){
    //     printk(KERN_INFO "scull_init: hash[%d] is %d\n",i, hash[i]);
    // }
    /* Copy the first 16 bytes of the hash into key */
    memcpy(rkey, hash, 16);

    // Continue with your existing setup code...
    // for (int i=0;i<16;i++){
    //     printk(KERN_INFO "scull_init: rkey[%d] is %d\n",i, rkey[i]);
    // }

    kfree(shash);
    crypto_free_shash(tfm);
    kfree(buf);
    kfree(hash);
    return 0;

fail_tfm:
    crypto_free_shash(tfm);
fail_buf:
    kfree(buf);
fail:
    scull_cleanup_module();
    return result;
}

int scull_open(struct inode* inode,struct file *filp){
	struct scull_dev *dev; /* device information */
	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	filp->private_data = dev; /* 保存设备状态 供其他方法使用 */
	/* 如果是只写打开模式，则将设备长度修剪为 0  */
    // 只写模式、读写模式、只读模式都不同 只写模式一般会清空空间
	if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
        //这里采取了简单的信号量并发机制
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;
		scull_trim_mem(dev); /* ignore errors */
		up(&dev->sem);
	}
	return 0;          /* success */
}


// 内核会帮助自动清除 filp->private_data
int scull_release(struct inode * inode,struct file*filp){
    return 0;
}

/**
 * @brief 遍历量子集链表,返回指定位置的量子集指针
 *
 * @param dev 设备结构体指针
 * @param n 要访问的量子集索引
 *
 * @return 指向第 n 个量子集的指针,如果分配失败则返回 NULL
 */
struct scull_qset *scull_follow(struct scull_dev *dev, int n)
{
    struct scull_qset *qs = dev->data; // 获取第一个量子集指针
    // 能够在这里去实现内存的分配是因为 scull_read 做出了相关的限制，使其并不会越界访问
    /* 如果第一个量子集未分配,则显式分配 */
    if (!qs) {
        qs = dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
        if (qs == NULL)
            return NULL; // 分配失败,返回 NULL
        memset(qs, 0, sizeof(struct scull_qset)); // 初始化量子集
    }

    /* 然后遍历量子集链表 */
    while (n--) {
        if (!qs->next) { // 如果下一个量子集未分配
            qs->next = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
            if (qs->next == NULL)
                return NULL; // 分配失败,返回 NULL
            memset(qs->next, 0, sizeof(struct scull_qset)); // 初始化量子集
        }
        qs = qs->next; // 移动到下一个量子集
    }
    return qs; // 返回第 n 个量子集指针
}
//这里并未实现解密逻辑 且只能读取前4000个字节数据
ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    struct scull_dev *dev = filp->private_data; /* 获取设备结构体指针 */
    struct scull_qset *dptr;    /* 指向第一个量子集的指针 */
    int quantum = dev->quantum, qset = dev->qset; /* 获取量子大小和量子集数量 */
    int itemsize = quantum * qset; /* 计算每个量子集的总大小 */
    int item, s_pos, q_pos, rest;
    ssize_t retval = 0;

    if (down_interruptible(&dev->sem)) /* 获取信号量锁 */
        return -ERESTARTSYS;

    if (*f_pos >= dev->size) /* 如果文件位置超出设备大小,直接返回 */
        goto out;

    if (*f_pos + count > dev->size) /* 如果要读取的数据超出设备大小,则调整读取大小 */
        count = dev->size - *f_pos;

    item = (long)*f_pos / itemsize;
    rest = (long)*f_pos % itemsize;
    s_pos = rest / quantum; q_pos = rest % quantum;

    //printk(KERN_INFO "scull_read: 在量子集 %d, 量子 %d, 偏移 %d 读取\n", item, s_pos, q_pos);

    dptr = scull_follow(dev, item);

    if (dptr == NULL || !dptr->data || !dptr->data[s_pos])
        goto out; /* 如果量子集为空,则不读取数据 */
    //printk(KERN_INFO "scull_read: 开始读取, 请求读取 %zu 字节\n", count);
    if(count) {
        int temp_count = 0;
        if (count > quantum - q_pos){
            temp_count  = quantum - q_pos;
            count = 0;
        }else{
            temp_count = count;
            count = 0;
        }

        if (copy_to_user(buf, dptr->data[s_pos] + q_pos, temp_count)) {
            retval = -EFAULT;
            goto out;
        }

        //printk(KERN_INFO "scull_read: 从量子集 %d, 量子 %d, 读取 %zu 字节\n", item, s_pos, temp_count);

        *f_pos += temp_count; /* 更新文件位置 */
        retval += temp_count; /* 设置返回值为实际读取的字节数 */
    }

out:
    up(&dev->sem); /* 释放信号量锁 */
    //将文件位置还原至文件开头

    //printk(KERN_INFO "scull_read: 完成读取, 总共读取 %zd 字节\n", retval);
    return retval;
}

void myswap(unsigned char *a, unsigned char *b) {
    unsigned char temp = *a;
    *a = *b;
    *b = temp;
}

void write_encrypt(char *data){
    unsigned char T[ARRAY_LENGTH];
    unsigned char S[ARRAY_LENGTH];
    int key_lenth = 32;
    for (int i = 0; i < ARRAY_LENGTH; i++) {
        S[i] = i;
        T[i] = key[i % key_lenth];
    }

    int j = 0;
    for (int i = 0; i < ARRAY_LENGTH; i++) {
        j = (j + S[i] + T[i]) % ARRAY_LENGTH;
        myswap(&S[i], &S[j]);
    }
    //这里为改进部分
    for (int i = 0;i < ARRAY_LENGTH;i++){
        S[i] = (Sbox[i%8][(S[i]>>4)]^Sbox[i%8][(S[i]&(0xf))]);
    }
    //以上为改进的rc4 ksa
    int i = 0; 
    j = 0;
    for (int n = 0; n < strlen(data); n++) {
        i = (i + 1) % ARRAY_LENGTH;
        j = (j + S[i]) % ARRAY_LENGTH;
        myswap(&S[i], &S[j]);
        int rand = S[(S[i] + S[j]) % ARRAY_LENGTH];
        data[n] = data[n] ^ rand;
    }
}
/**
 * @brief 向 scull 设备写入数据
 *
 * @param filp 文件结构体指针
 * @param buf 用户空间缓冲区
 * @param count 要写入的字节数
 * @param f_pos 文件位置指针
 *
 * @return 实际写入的字节数,或者出错时返回错误码
 */
ssize_t scull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    struct scull_dev *dev = filp->private_data; // 获取设备结构体指针
    struct scull_qset *dptr; // 用于指向目标量子集
    int quantum = dev->quantum, qset = dev->qset; // 获取量子大小和量子集数量
    int itemsize = quantum * qset; // 计算每个量子集的总大小
    //printk(KERN_INFO "scull_write: 量子大小 quantum=%d, 量子集数量 qset=%d, 量子集总大小 itemsize=%d\n", quantum, qset, itemsize);
    int item, s_pos, q_pos, rest;
    ssize_t retval = 0; /* 用于 "goto out" 语句的值 */
    size_t temp_count = count;
    printk(KERN_INFO "scull_write: 开始写入，请求写入 %zu 字节\n", count);
    if (down_interruptible(&dev->sem)) // 获取信号量锁
        return -ERESTARTSYS;
    //一点改动 可以写多个量子集
    scull_openkey_init(filp);
    memcpy(key,lkey,16);
    memcpy(&(key[16]),rkey,16);
    char *kbuf = kmalloc(count,GFP_KERNEL);
    copy_from_user(kbuf,buf,count);
    //printk("you will write %s",kbuf);
    write_encrypt(kbuf);
    while(count>0){
        /* 计算要写入的数据在哪个量子集中,以及在量子集中的偏移位置 */
        item = (long)*f_pos / itemsize;
        rest = (long)*f_pos % itemsize;
        s_pos = rest / quantum; q_pos = rest % quantum;
        
        //printk(KERN_INFO "scull_write: 写入位置 item=%d, s_pos=%d, q_pos=%d\n", item, s_pos, q_pos);

        temp_count = count;
        /* 遍历量子集链表,找到要写入的量子集 */
        dptr = scull_follow(dev, item);
        if (dptr == NULL){
            printk(KERN_INFO "scull_write: 未找到量子集\n");
            goto out;
        } // 如果量子集不存在,则跳转到 out 标签

        if (!dptr->data) { // 如果量子集中没有数据指针数组
            dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL); // 分配数据指针数组
            if (!dptr->data){
                printk(KERN_INFO "scull_write: kmalloc 分配内存失败\n");
                goto out;
            }
            memset(dptr->data, 0, qset * sizeof(char *)); // 初始化数据指针数组
        }
        
        if (!dptr->data[s_pos]) { // 如果目标量子未分配
            dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL); // 分配目标量子
            if (!dptr->data[s_pos]){
                printk(KERN_INFO "scull_write: kmalloc 分配目标量子失败\n");
                goto out;
            }
        }

        if (count > (quantum - q_pos)){
            temp_count = quantum - q_pos;
            count = count - temp_count;
        }else{
            count = 0;
        }

        memcpy(dptr->data[s_pos] + q_pos, kbuf, temp_count);
        printk(KERN_INFO "scull_write: 成功写入 %zd 字节\n", temp_count);
        *f_pos += temp_count; // 更新文件位置指针
        retval += temp_count; // 设置返回值为实际写入的字节数

        /* 更新设备大小 */
        if (dev->size < *f_pos)
            dev->size = *f_pos;

    }

out:
    up(&dev->sem); // 释放信号量锁
    kfree(kbuf);
    *f_pos = 0;
    printk(KERN_INFO "scull_write: 写入完成，总共写入 %zd 字节\n", retval);
    return retval;
}

/**
 * @brief 更新文件位置指针
 *
 * @param filp 文件结构体指针
 * @param off 偏移量
 * @param whence 偏移起始位置标志
 *
 * @return 新的文件位置,或者出错时返回错误码
 */
static loff_t scull_llseek(struct file *filp, loff_t off, int whence)
{
    struct scull_dev *dev = filp->private_data; // 获取设备结构体指针
    struct scull_qset *qset; // 用于遍历量子集链表
    loff_t newpos; // 存储新的文件位置

    if (down_interruptible(&dev->sem)) // 获取信号量锁
        return -ERESTARTSYS;

    switch (whence) { // 根据 whence 参数计算新的文件位置
    case 0: /* SEEK_SET */
        newpos = off; // 新位置为偏移量 off
        break;
    case 1: /* SEEK_CUR */
        newpos = filp->f_pos + off; // 新位置为当前位置加上偏移量 off
        if(newpos > dev->size){
            newpos = dev->size;
        }
        break;
    case 2: /* SEEK_END */
        newpos = dev->size - off;
        break;
    default:
        up(&dev->sem); // 释放信号量锁
        return -EINVAL; // 无效的 whence 参数
    }

    if (newpos < 0 || newpos > dev->size) { // 检查新位置是否超出设备大小限制
        up(&dev->sem); // 释放信号量锁
        return -EINVAL; // 返回 EINVAL 错误码
    }

    filp->f_pos = newpos; // 更新文件位置指针
    up(&dev->sem); // 释放信号量锁
    return newpos; // 返回新的文件位置
}


int scull_key_init(void){
    char *buf;
    char *hash;
    struct file *file;
    struct crypto_shash *tfm;
    struct shash_desc *shash;
    loff_t pos;
    int result;
    /* Allocate buffer for reading file content */
    printk(KERN_INFO "scull_init: start running key init\n");
    buf = kmalloc(PAGE_SIZE,GFP_KERNEL);
    hash = kmalloc(32,GFP_KERNEL);
    if(!buf || !hash){
        printk(KERN_INFO "scull_init: buf and hash malloc failed\n");
        return 0;
    }
    memset(buf,0,PAGE_SIZE);
    memset(hash,0,32);
    file = filp_open("./.key", O_RDONLY, 0);
    /*
    IS_ERR宏：用于检查一个函数返回的指针是否表示一个错误。
    在Linux内核中，错误是通过指针来表示的，而不是通常在用户空间程序中返回的负数错误码。
    如果一个函数返回的指针是一个错误指针（而不是一个有效的内存地址），IS_ERR宏就会评估为true。

    PTR_ERR宏：当IS_ERR宏确认一个指针为错误指针时，PTR_ERR宏用于从错误指针中提取错误码。
    这个错误码是一个负值，表示特定的错误类型。
    */
    if (IS_ERR(file)) {
        result = PTR_ERR(file);
        printk(KERN_INFO "failed 1");
        goto fail_buf;
    }

    kernel_read(file, buf, PAGE_SIZE, &pos);
    filp_close(file, NULL);
    //printk(KERN_INFO "./.key is %s",buf);
    /* Initialize hash transformation */
    tfm = crypto_alloc_shash("sha256", 0, 0);
    if (IS_ERR(tfm)) {
        result = PTR_ERR(tfm);
        printk(KERN_INFO "failed 2");
        goto fail_buf;
    }

    shash = kmalloc(sizeof(struct shash_desc{}) + crypto_shash_descsize(tfm), GFP_KERNEL);
    if (!shash) {
        result = -ENOMEM;
        printk(KERN_INFO "failed 3");
        goto fail_tfm;
    }

    shash->tfm = tfm;
    /* Compute hash of the key file content */
    crypto_shash_init(shash);
    //printk(KERN_INFO "the key is %s,length is %d",buf,strlen(buf));
    crypto_shash_update(shash, buf, strlen(buf));
    crypto_shash_final(shash, hash);
    // for (int i=0;i<16;i++){
    //     printk(KERN_INFO "scull_init: hash[%d] is %d\n",i, hash[i]);
    // }
    /* Copy the first 16 bytes of the hash into key */
    memcpy(lkey, hash, 16);

    // Continue with your existing setup code...
    // for (int i=0;i<16;i++){
    //     printk(KERN_INFO "scull_init: lkey[%d] is %d\n",i, lkey[i]);
    // }

    kfree(shash);
    crypto_free_shash(tfm);
    kfree(buf);
    kfree(hash);
    return 0;

fail_tfm:
    crypto_free_shash(tfm);
fail_buf:
    kfree(buf);
fail:
    scull_cleanup_module();
    return result;
}
int scull_init_module(void)   /*获取主设备号，或者创建设备编号*/
{
    int result ,i;
    dev_t dev = 0;
    scull_key_init();
    if(scull_major){
        dev = MKDEV(scull_major,scull_minor);     /*将两个设备号转换为dev_t类型*/
        result = register_chrdev_region(dev,scull_num_devs,"scull");/*申请设备编号*/
    }else{
        result = alloc_chrdev_region(&dev,scull_minor,scull_num_devs,"scull");/*分配主设备号*/
        scull_major = MAJOR(dev);
    }

    if(result < 0){
        printk("scull : cant get major %d\n",scull_major);
        return result;
    }else{
        printk("register a dev_t %d %d\n",scull_major,scull_minor);
    }
    
    /*分配设备的结构体*/
    scull_devices = kmalloc(scull_num_devs * sizeof(struct scull_dev),GFP_KERNEL);

    if(!scull_devices){
        result = -1;
        goto fail;
    }
    memset(scull_devices,0,scull_num_devs * sizeof(struct scull_dev));

    for(i = 0;i < scull_num_devs;i++){
        scull_devices[i].quantum = scull_quantum;
        scull_devices[i].qset = scull_qset;
        sema_init(&scull_devices[i].sem,1);
        /*sema_init  是内核用来新代替dev_INIT 的函数，初始化互斥量*/
        scull_setup_cdev(&scull_devices[i],i);
        /*注册每一个设备到总控结构体*/
    }

    return 0;

fail:
    scull_cleanup_module();
    return result;

}

module_init(scull_init_module);
module_exit(scull_cleanup_module);


