/*****************************
 * 看门狗设备的完整char驱动
 * 1.char驱动部分支持write和unlocked_ioctl;
 * 2.支持proc文件，通过proc文件获得狗的当前工作状态；
 * 3.支持中断处理函数，一旦将狗配置为中断模式，则定时器归0时会调用中断处理函数；
 * 4.可以增加锁，确保用户态同一时间只有一个人控制狗；
 * 5.用miscdevice注册；
 * 6.在模块的入口中，获得并使能狗的时钟；
 * 7.默认采用最大的分频值，此时，WTCNT寄存器每秒钟递减3052次计数；对应的最大喂狗间隔秒数为21秒
 * Author: syy
 * Date: 2017-06-12
 ******************************/
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h> //kzalloc
#include <linux/time.h> //timeval
//char驱动需要的头文件
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
//proc文件需要的头文件
#include <linux/proc_fs.h>
//中断处理需要的头文件
#include <linux/interrupt.h>
#include <mach/irqs.h> //中断号
//寄存器访问需要的头文件
#include <linux/ioport.h>
#include <linux/io.h>
//锁需要的头文件
#include <linux/mutex.h>
//时钟需要的头文件
//由于看门狗设备的时钟默认关闭，所以必须由驱动负责找到时钟，并使能；
//如果没有使能时钟，则无法正确访问狗的寄存器；
#include <linux/clk.h>

//常量定义
#define CNT_IN_1S	3052 //1秒钟计数器递减的数量
#define DEF_TIME	10	//默认的喂狗间隔为10秒
#define MAX_TIME	21	//如果PCLK为100MHZ，则最大喂狗间隔为21秒
#define MODE_RESET	1	//狗的工作模式为reset/irq
#define MODE_IRQ	2
#define DEF_MODE	MODE_IRQ
//下面两个用于设定分频比
#define DEF_PRESC	255	//写入WTCON[15:8]
#define DEF_DIV		3	//写入WTCON[4:3]

//定义寄存器的物理基地址和偏移
#define WDT_BASE	0x10060000
#define WDT_SIZE	0x1000 //ioremap的范围
#define WTCON		0x0
#define WTDAT		0x4
#define WTCNT		0x8
#define WTCLRINT	0xC

//定义ioctl命令
#define WDT_TYPE	'W'
#define WDT_ON		_IO(WDT_TYPE, 1) //使能看门狗
#define WDT_OFF		_IO(WDT_TYPE, 2) //关闭看门狗
#define WDT_FEED	_IO(WDT_TYPE, 3) //用当前的计数值(根据喂狗间隔时间转化)来喂狗
#define WDT_CHG_MODE	_IOW(WDT_TYPE, 4, int) //改变狗的工作模式，参数支持MODE_RESET|MODE_IRQ
#define WDT_CHG_TIME	_IOW(WDT_TYPE, 5, int) //改变喂狗间隔秒数(单位为整数的秒)，不能超过最大间隔

//定义变量
static void __iomem *vir_base; //虚拟基地址
static int wdt_irq; //存储看门狗中断的中断号
static int wdt_time; //当前的喂狗间隔秒数
static int wdt_mode; //当前的模式(RESET|IRQ)
static int wdt_cnt; //根据喂狗间隔转换的计数值，用3052乘以喂狗间隔秒数
static struct clk *wdt_clk; //指向狗的时钟结构体的指针
static struct mutex wdt_clk; //用锁来确保同一时间只能一个人使用狗

//proc文件的读函数
static int 
wdt_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	//获得狗的当前状态，包括wdt_time/wdt_mode/wdt_cnt等变量；
	//也包括WTCON/WTDAT/WTCNT等寄存器的当前值
}

//中断处理函数
static irqreturn_t
wdt_service(int irq, void *dev_id)
{
	//一旦调用了中断处理函数，说明看门狗在给定时间内没有喂狗
	//应该在中断处理中通知用户没有喂狗；
	//可以用printk输出当前的时间，也可以让蜂鸣器响一声；
	//用timeval获得当前的绝对时间输出；
	//向WTCLRINT寄存器写任意值，清除中断状态
	return IRQ_HANDLED;
}

//file_operations->write
//支持用户态用echo来控制狗
//$>echo on >/dev/mywdt
//$>echo off >/dev/mywdt
//#>echo feed >/dev/mywdt 
//一旦用on使能狗，用户态需要不断用feed命令来喂狗，一旦喂狗间隔超过wdt_time秒，则会引起中断或reset
static ssize_t
wdt_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{}

//file_operations->unlocked_ioctl
//支持WDT_ON等5个命令
static long
wdt_ioctl(struct file *filp, unsigned int req, unsigned long arg)
{}

static struct file_operations wdt_fops = {
	.owner	= THIS_MODULE,
	.write	= wdt_write,
	.unlocked_ioctl = wdt_ioctl,
};

static struct miscdevice wdt_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "mywdt",
	.fops	= &wdt_fops,
};

static int __init my_init(void)
{
	int ret;
	//1.ioremap获得虚拟基地值
	//2.获得并使能看门狗的时钟
	wdt_clk = clk_get(NULL, "watchdog");
	if (IS_ERR(wdt_clk)) {
		printk("无法正确获得狗的时钟\n");
		ret = PTR_ERR(wdt_clk);
		goto err_clk;
	}
	clk_enable(wdt_clk);
	//3.初始化wdt_time/wdt_mode等变量
	//根据变量的设定，初始化WTCON寄存器
	//4.找到中断号，然后request_irq
	中断的标志flags直接写0即可
	//5.创建proc文件并关联读函数
	//6.注册miscdevice设备
	return 0;
}

static void __exit my_exit(void)
{
	//注销miscdevice
	//删除proc文件
	//free_irq
	//clk_disable(wdt_clk);
	//iounmap();
}

...


