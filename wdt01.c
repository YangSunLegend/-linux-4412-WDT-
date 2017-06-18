//用户态的测试例子
//对狗的ioctl命令进行测试

支持的ioctl命令包括:

#define MAX_TIME	21
#define MODE_RESET	1	//狗的工作模式为reset/irq
#define MODE_IRQ	2

#define WDT_TYPE	'W'
#define WDT_ON		_IO(WDT_TYPE, 1) //使能看门狗
#define WDT_OFF		_IO(WDT_TYPE, 2) //关闭看门狗
#define WDT_FEED	_IO(WDT_TYPE, 3) //用当前的计数值(根据喂狗间隔时间转化)来喂狗
#define WDT_CHG_MODE	_IOW(WDT_TYPE, 4, int) //改变狗的工作模式，参数支持MODE_RESET|MODE_IRQ
#define WDT_CHG_TIME	_IOW(WDT_TYPE, 5, int) //改变喂狗间隔秒数(单位为整数的秒)，不能超过最大间隔



