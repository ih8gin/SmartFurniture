#include "ds18b20.h"
#include "delay.h"	


// 复位DS18B20
/*
1、主机输出低电平，保持低电平时间至少480us（复位脉冲）
2、主机释放总线，高电平，延时15-60us
           ___
__________|

*/
void DS18B20_Rst(void)	   
{                 
	DS18B20_IO_OUT(); 	// 设置IO口为输出模式
	
    DS18B20_DQ_OUT=0; 	// 拉低总线
	
    delay_us(750);    	// 拉低750us（复位脉冲至少480us ）
	
    DS18B20_DQ_OUT=1; 	// 释放总线
	
	delay_us(15);     	// 延时15us
}





// 检查传感器是否存在，等待应答
// 返回1:未检测到DS18B20的存在
// 返回0:存在
/*
DS18B20传感器拉低总线，60-240us（低电平应答脉冲）

      |______

*/
u8 DS18B20_Check(void) 	   
{   
	u8 retry=0;
	DS18B20_IO_IN();	// 设置IO口为输入模式 
    while (DS18B20_DQ_IN&&retry<200)// 当总线为高电平，且未超时，则等待200us
	{
		retry++;
		delay_us(1);
	};	
	
	if(retry>=200)return 1;// 当超时，则返回1（传感器不存在）
	else retry=0;// 将retry重新设置为0，用以计时
  

  
		while (!DS18B20_DQ_IN&&retry<240)// 当总线为低电平，且低电平宽度小于240us，则等待；当总线拉高或超时，跳出循环
	{
		retry++;
		delay_us(1);
	};
	
	if(retry>=240)return 1;// 当拉低的时间超过240us，则返回1（传感器不存在）	    
	return 0;// 只要满足低电平宽度小于240us，就返回0（传感器存在）
}






// 从DS18B20读取一位
// 返回值：1/0
/*
所有读时序至少需要60us，且在两次写时序之间至少需要1us恢复时间
每个读时序都由主机发起，至少拉低总线1us
主机在读时序期间必须释放总线，并且在时序其实后的15us之内采样总线状态

1、主机输出低电平延时2us
2、主机转入输入模式延时12us
3、读取总线当前电平
4、延时50us
*/
u8 DS18B20_Read_Bit(void) 	 
{
  u8 data;				//用于读取并返回的变量
	
	DS18B20_IO_OUT();	// 设置IO口为输出模式
	
  DS18B20_DQ_OUT=0; // 主机输出低电平2us
	delay_us(2);			
	
  DS18B20_DQ_OUT=1; // 释放总线
	
	DS18B20_IO_IN();	// 设置IO口为输入模式
	
	delay_us(12);			// 延时12us
	
	
	if(DS18B20_DQ_IN)data=1;//读取总线数据，若为高电平，data置为1
    else data=0;					//若为低电平，data置为0
	
    delay_us(50);         //延时50us  
	
    return data;					//返回data
}





// 从DS18B20读取一个字节(将串行的比特变成字节)
// 返回值：读取到的字节
u8 DS18B20_Read_Byte(void)     
{        
    u8 i,j,dat;// 计数变量i，读取到的位j，数据dat
    dat=0;		 // dat初始化为0
	for (i=1;i<=8;i++)//将j赋值给dat每一位 
	{
        j=DS18B20_Read_Bit();
        dat=(j<<7)|(dat>>1);// j左移是因为j只有一位有效，左移至最高位赋值给dat最高位，然后dat右移，j重新赋值，直至最开始j的值右移至dat最低位
    }						    
    return dat;							//返回读取到的字节
}






// 写一个字节到DS18B20
/*
所有写时序至少需要60us，且在两次写时序之间至少需要1us恢复时间
两种写时序均始于主机拉低总线

1、写1时序：主机输出低电平，延时2us，释放总线，延时60us
   ___________________
__|

2、写0时序：主机输出低电平，延时60us，释放总线，延时2us
                    __
___________________|

*/

// dat：要写入的字节
void DS18B20_Write_Byte(u8 dat)     
 {             
    u8 j;// 计数变量
    u8 testb;// 每次输出数据的的最低位
	 
	DS18B20_IO_OUT();	// 设置IO口为输出
	 
    for (j=1;j<=8;j++)// 每次写一位 
	{
        testb=dat&0x01;// 读取数据最低位
        dat=dat>>1;// 向右移位，次低位移位到最低位
		
		
        if (testb)// 数据最低位为1 
        {
            DS18B20_DQ_OUT=0;	// 主机输出低电平
            delay_us(2);      // 延时2us                      
            DS18B20_DQ_OUT=1; // 释放总线
            delay_us(60);     // 延时60us        
        }
				
				
        else// 数据最低位为0 
        {
            DS18B20_DQ_OUT=0;	// 主机输出低电平
            delay_us(60);     // 延时60us        
            DS18B20_DQ_OUT=1; // 释放总线
            delay_us(2);      // 延时2us                    
        }
    }
}
 





//开始温度转换
void DS18B20_Start(void) 
{   						               
    DS18B20_Rst();	// 复位   
	DS18B20_Check();	// 检测传感器是否存在，等待应带
    DS18B20_Write_Byte(0xcc);	// 跳过rom（因为只有一个传感器，不需要校验）
    DS18B20_Write_Byte(0x44);	// 发送开始转换命令
} 





//初始化DS18B20的IO口 DQ 同时检测DS的存在
u8 DS18B20_Init(void)
{
 	GPIO_InitTypeDef  GPIO_InitStructure;
 	
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);	 //使能IO口G口时钟 
	
 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;				//IO口G.11 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 	//设置推挽输出	  
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //设置传输速度50MHz
 	GPIO_Init(GPIOG, &GPIO_InitStructure);

 	GPIO_SetBits(GPIOG,GPIO_Pin_11);    //输出1

	DS18B20_Rst();											//复位

	return DS18B20_Check();							//返回（检查传感器是否存在）值：1不存在；0存在
}  





//从ds18b20得到温度值
//精度：0.1C
//返回值：温度值 （-550~1250） 
/*
转换后得到12位数据，16位中前5位为符号位
LSB:2^3, 2^2, 2^1, 2^0, 2^-1, 2^-2, 2^-3, 2^-4
MSB: S ,  S ,  S ,  S ,  S ,  2^6,  2^5,  2^4
*/
short DS18B20_Get_Temp(void)
{
    u8 temp;
    u8 TL,TH;
	short tem;
    DS18B20_Start ();  			  // 调用函数，开始转换
    DS18B20_Rst();						// 复位
    DS18B20_Check();	 				// 检查传感器是否存在，等待应答
    DS18B20_Write_Byte(0xcc);	// 跳过rom（因为只有一个传感器，不需要校验）
    DS18B20_Write_Byte(0xbe);	// 发送开始转换命令	    
    TL=DS18B20_Read_Byte(); 	// 读取低字节并赋值给TL   
    TH=DS18B20_Read_Byte(); 	// 读取高字节并赋值给TH  



	
	
//计算	
    if(TH>7)//符号位为1（2^0+2^1+2^2=7）
    {
        TH=~TH;
        TL=~TL; 
        temp=0;					//温度为负  
    }else temp=1;				//温度为正	  	  
    tem=TH; 					//先将高八位赋值到tem
    tem<<=8;    			//将TH值从tem低八位移到高八位
    tem+=TL;					//两个八位合成为一个十六位
    tem=(float)tem*0.625;		//二进制数最低位变化1，映射温度变化0.0625（125°为07d0h）   
	if(temp)return tem; 		//若温度值为正，返回温度值
	else return -tem;    		//若温度值为负，取反后返回温度值
}



 
