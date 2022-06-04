#include "sys.h"
#include "delay.h"
#include "usart.h" 
#include "led.h" 	
#include "key.h"     
#include "usmart.h" 
#include "malloc.h"
#include "sdio_sdcard.h"  
#include "w25qxx.h"    
#include "ff.h"  
#include "exfuns.h"   
#include "string.h"		
#include "math.h"	 
#include "ov7725.h"
#include "beep.h" 
#include "timer.h" 
#include "bmp.h"
#include "usart2.h"
#include "as608.h"
#include "ds18b20.h" 
#include "exti.h"

/** 一些板载资源使用说明
led灯：
	LED_Init()
	有两个led灯，LED0为红灯、LED1为绿灯
	LEDx = 1 亮
		 = 0 灭
蜂鸣器：
	BEEP_Init()
	BEEP = 1 响
		 = 0 不响
key按键：
	KEY_Init()
	有三个按键KEY0、KEY1、KEY_UP
	使用key = KEY_Scan(0)获得按键值
	key = 0 没有按键被按下
		= KEY0_PRES KEY0被按下
		= KEY1_PRES KEY1被按下
		= WKUP_PRES KEY_UP被按下
串口通信：
	数据线接USB_232端口
	uart_init(Baud) 设置波特率
	可以使用printf函数在xcom界面查看输出，也可以从xcom界面输入
	‘\r\n’ 表示换行
内存管理：
	my_mem_init(SRAMIN) 初始化内部内存池
	target=mymalloc(SRAMIN,n) 为target申请n个字节的内存空间
	mymemset(target,val,len) 将target的前len个字节的值设置为val
	myfree(SRAMIN,target) 释放target占用的内存
*/

//定义全局变量
extern u8 ov_sta;	//在exit.c里面定义
extern u8 ov_frame;	//在timer.c里面定义	 
SysPara AS608Para;//指纹模块AS608参数
u16 ValidN;//模块内有效指纹个数


//以下为供主控函数调用的接口
u8 is_safe(void);			// 根据温度和气体浓度判断是否安全

u8 face_recognition();		// 人脸识别		未实现！
u8 face_add(void);			// 录入人脸
void face_delete(void);		// 删除人脸

u8 fingerprint_recognition();	// 指纹识别
u8 fingerprint_add(void);		//录入指纹
void fingerprint_delete(void);	//删除指纹


//以下为供接口调用的功能子函数
u16 get_temperature(void); 	// 获取温度参数
u16 get_gas(void);			// 获取气体浓度参数		未实现！

void upload_pic(void);		//上传图片数据至串口
u8 wait_ack(void);			//等待串口回传结果		未实现！

u8 press_FR(void);				//刷指纹
void ShowErrMessage(u8 ensure);	//显示确认码错误信息

//主控函数
int main(void){	 
	// 变量定义
	u8 sensor=0;
	u8 res;							 
	u8 *pname;				//带路径的文件名 
	u8 key;					//键值		   
	u32 i;						 
	u8 sd_ok=1;				//0,sd卡不正常;1,SD卡正常.
	u16 color;
	u8 grey;
	 
	// 板载资源初始化
	delay_init();	    	 //延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口初始化为115200
 	usmart_dev.init(72);		//初始化USMART		
 	LED_Init();		  			//初始化与LED连接的硬件接口
	KEY_Init();					//初始化按键
	BEEP_Init();        		//蜂鸣器初始化	 
	W25QXX_Init();				//初始化W25Q128
 	my_mem_init(SRAMIN);		//初始化内部内存池
	exfuns_init();				//为fatfs相关变量申请内存  
 	f_mount(fs[0],"0:",1); 		//挂载SD卡 
 	f_mount(fs[1],"1:",1); 		//挂载FLASH. 
	DS18B20_Init();				//温度传感器初始化
	OV7725_Init();
	OV7725_Window_Set(320,240,0);//QVGA模式输出
	OV7725_CS=0;
	TIM6_Int_Init(10000,7199);			//10Khz计数频率,1秒钟中断									  
	EXTI8_Init();						//使能定时器捕获
	printf("Ready!\r\n");
	
	// 主控程序入口
	while(1){
		key = KEY_Scan(0);
		if(key == KEY0_PRES)upload_pic();
	}												    
}



//接口实现

u8 is_safe(){
	/**
	* 供主控函数调用
	* 根据温度和气体浓度参数判断环境是否安全
	* 不安全则点亮红灯、开启蜂鸣器报警，并返回0
	* 安全则熄灭红灯、关闭蜂鸣器，返回0
	*/
	u16 temperature;
	u16 gas;
	temperature=get_temperature();
	if(temperature > 300){
		BEEP=1;
		LED0=0;
		return 0;
	}else{
		BEEP=0;
		LED0=1;
		return 1;
	}
}

u8 face_recognition(){
	/**
	* 供主控函数调用
	* 将摄像头采集到的图像数据通过串口上传至上位机
	* 等待在上位机处理完成后返回的信号帧
	* 若识别到库中人脸，返回1
	* 若未识别到有效人脸，返回0
	*/
	u8 ret;
	upload_pic();
	return wait_ack();
}

u8 face_add(void){
	/**
	* 供主控函数调用
	* 添加人脸特征
	* 若添加成功，返回1
	* 若添加失败，返回0
	*/
	printf("请在python上位机完成人脸添加操作，尚不支持用stm32执行此操作");
	return 0;
}

void face_delete(void){
	/**
	* 供主控函数调用
	* 删除指定人脸特征或清空人脸特征库
	*/
	printf("请在python上位机上完成人脸删除操作，尚不支持用stm32执行此操作");
}

u8 fingerprint_recognition(){
	/**
	* 供主控函数调用
	* 检查是否有指纹按下并做出回应
	* 若无指纹按下或按下指纹错误，返回0
	* 若有正确指纹按下，返回1
	*/
	if(PS_Sta){ //检测PS_Sta状态，如果有手指按下
		press_FR();//刷指纹
		delay_ms(600);
	}
}

u8 fingerprint_add(void){
	/**
	* 供主控函数调用
	* 向库中添加指纹特征
	* 添加成功则返回1
	* 添加失败则返回0
	*/
	u8 i,ensure ,processnum=0;
	u16 ID = 1;
	u8 key;
	LED0=1;
	LED1=0;
	while(1){
		switch (processnum){
			case 0:
				i++;
				printf("请按指纹");
				ensure=PS_GetImage();
				if(ensure==0x00) {
					BEEP=1;
					ensure=PS_GenChar(CharBuffer1);//生成特征
					BEEP=0;
					if(ensure==0x00){
						printf("指纹正常");
						i=0;
						processnum=1;//跳到第二步						
					}else ShowErrMessage(ensure);				
				}else ShowErrMessage(ensure);						
				break;
			case 1:
				i++;
				printf("请按再按一次指纹");
				ensure=PS_GetImage();
				if(ensure==0x00) {
					BEEP=1;
					ensure=PS_GenChar(CharBuffer2);//生成特征
					BEEP=0;
					if(ensure==0x00){
						printf("指纹正常");
						i=0;
						processnum=2;//跳到第三步
					}else ShowErrMessage(ensure);	
				}else ShowErrMessage(ensure);		
				break;
			case 2:
				printf("对比两次指纹");
				ensure=PS_Match();
				if(ensure==0x00) {
					printf("对比成功,两次指纹一样");
					processnum=3;//跳到第四步
				}
				else {
					printf("对比失败，请重新录入指纹");
					ShowErrMessage(ensure);
					i=0;
					processnum=0;//跳回第一步		
				}
				delay_ms(1200);
				break;
			case 3:
				printf("生成指纹模板");
				ensure=PS_RegModel();
				if(ensure==0x00) {
					printf("生成指纹模板成功");
					processnum=4;//跳到第五步
				}else {processnum=0;ShowErrMessage(ensure);}
				delay_ms(1200);
				break;
			case 4:	
				printf("请输入储存ID,按KEY_0计数加,按KEY_1清零,按KEY_UP保存");
				while(1){
					key=KEY_Scan(0);
					printf("ID = %d ? \r\n",ID);
					if(key==KEY0_PRES){
						ID+=1;
					}
					if(key==KEY1_PRES){
						ID=1;
					}
					if(key==WKUP_PRES){
						break;
					}
				}
				ensure=PS_StoreChar(CharBuffer2,ID);//储存模板
				if(ensure==0x00) {							
					printf("录入指纹成功");
					PS_ValidTempleteNum(&ValidN);//读库指纹个数
					//printf(AS608Para.PS_max-ValidN);
					delay_ms(1500);LED0=0;
					LED1=1;
					return 1;
				}else {processnum=0;ShowErrMessage(ensure);}					
				break;				
		}
		delay_ms(400);
		if(i==5){ //超过5次没有按手指则退出
			break;	
		}				
	}
	return 0;
}

void fingerprint_delete(void){
	/**
	* 供主控函数调用
	* 删除指定ID指纹或清空指纹库
	*/
	u8  ensure;
	u16 key;
	u8 ID = 1;
	printf("删除指纹");
	printf("请输入删除ID,按KEY_0计数加,按KEY_1清零,按KEY_UP保存, ID=0表示清空指纹库");
	while(1){
		key=KEY_Scan(0);
		printf("ID = %d ? \r\n",ID);
		if(key==KEY0_PRES){
			ID+=1;
		}
		if(key==KEY1_PRES){
			ID=0;
		}
		if(key==WKUP_PRES){
			break;
		}
	}
	if(ID == 0)
		ensure=PS_Empty();//清空指纹库
	else 
		ensure=PS_DeletChar(ID,1);//删除单个指纹
	if(ensure==0){
		printf("删除指纹成功");		
	}
  else
		ShowErrMessage(ensure);	
	delay_ms(1200);
	PS_ValidTempleteNum(&ValidN);//读库指纹个数
	delay_ms(50);
}

u16 get_temperature(){
	/**
	* 供安全判断函数调用
	* 测量并返回环境温度
	* 返回值为当前检测摄氏度*10
	*/
	u16 temperature;
	temperature = DS18B20_Get_Temp();
	printf("当前温度为%d.%d°C\r\n",temperature/10,temperature%10);	//打印温度的整数位和小数位
	return temperature;
}

u16 get_gas(){
	/**
	* 供安全判断函数调用
	* 测量并返回一氧化碳气体浓度
	*/
	printf("气体检测模块尚未完成！");
}

void upload_pic(void){
	/**
	* 供人脸识别函数调用
	* 上传摄像头采集到的图像数据至python
	*/
	u32 j;
 	u16 color;	
	u8	grey;
	if(ov_sta)//有帧中断更新？
	{
		OV7725_RRST=0;				//开始复位读指针 
		OV7725_RCK_L;
		OV7725_RCK_H;
		OV7725_RCK_L;
		OV7725_RRST=1;				//复位读指针结束 
		OV7725_RCK_H; 
		for(j=0;j<76800;j++)
		{
			OV7725_RCK_L;
			color=GPIOC->IDR&0XFF;	//读数据
			OV7725_RCK_H; 
			color<<=8;  
			OV7725_RCK_L;
			color|=GPIOC->IDR&0XFF;	//读数据
			OV7725_RCK_H; 
			printf("%d",color);
			delay_ms(50);
		}   							  
 		ov_sta=0;					//清零帧中断标记
		ov_frame++; 
	} 
}

u8 wait_ack(void){
	/**
	* 供人脸识别相关函数调用
	* 接受来自上位机的信号帧
	* 收到连续三个一样的信号则确认
	* 并返回接收到的信号
	*/
	u16 len;
	u16 t;
	u8 cnt;
	u8 recv;
	while(1){
		if(USART_RX_STA&0x8000){					   
			len=USART_RX_STA&0x3fff;//得到此次接收到的数据长度
			for(t=0;t<len;t++){
				if(recv != USART_RX_BUF[t]){
					recv = USART_RX_BUF[t];
					cnt = 0;
				}else{
					cnt++;
				}
				if(cnt>3)break;
			}
			USART_RX_STA=0;
		}
	}
	return recv;
}

u8 press_FR(void){
	/**
	* 供指纹识别子程序调用
	* 搜索指纹是否在库中
	* 在库中则返回1
	* 不在库中则返回0
	*/
	SearchResult seach;
	u8 ensure;
	u8 ret = 0;
	char *str;
	ensure=PS_GetImage();
	if(ensure==0x00)//获取图像成功 
	{	
		ensure=PS_GenChar(CharBuffer1);
		if(ensure==0x00) //生成特征成功
		{		
			ensure=PS_HighSpeedSearch(CharBuffer1,0,AS608Para.PS_max,&seach);
			if(ensure==0x00)//搜索成功
			{				
				printf("刷指纹成功");				
				str=mymalloc(SRAMIN,50);
				sprintf(str,"确有此人,ID:%d  匹配得分:%d",seach.pageID,seach.mathscore);
				printf((u8*)str);
				myfree(SRAMIN,str);
				ret = 1;
			}
			else 
				ShowErrMessage(ensure);					
	  }
		else
			ShowErrMessage(ensure);
	}	
	return ret;
}

void ShowErrMessage(u8 ensure){
	/**
	* 供指纹识别相关函数调用
	* 显示确认码错误信息
	*/
	printf(EnsureMessage(ensure));
}
