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

/** һЩ������Դʹ��˵��
led�ƣ�
	LED_Init()
	������led�ƣ�LED0Ϊ��ơ�LED1Ϊ�̵�
	LEDx = 1 ��
		 = 0 ��
��������
	BEEP_Init()
	BEEP = 1 ��
		 = 0 ����
key������
	KEY_Init()
	����������KEY0��KEY1��KEY_UP
	ʹ��key = KEY_Scan(0)��ð���ֵ
	key = 0 û�а���������
		= KEY0_PRES KEY0������
		= KEY1_PRES KEY1������
		= WKUP_PRES KEY_UP������
����ͨ�ţ�
	�����߽�USB_232�˿�
	uart_init(Baud) ���ò�����
	����ʹ��printf������xcom����鿴�����Ҳ���Դ�xcom��������
	��\r\n�� ��ʾ����
�ڴ����
	my_mem_init(SRAMIN) ��ʼ���ڲ��ڴ��
	target=mymalloc(SRAMIN,n) Ϊtarget����n���ֽڵ��ڴ�ռ�
	mymemset(target,val,len) ��target��ǰlen���ֽڵ�ֵ����Ϊval
	myfree(SRAMIN,target) �ͷ�targetռ�õ��ڴ�
*/

//����ȫ�ֱ���
extern u8 ov_sta;	//��exit.c���涨��
extern u8 ov_frame;	//��timer.c���涨��	 
SysPara AS608Para;//ָ��ģ��AS608����
u16 ValidN;//ģ������Чָ�Ƹ���


//����Ϊ�����غ������õĽӿ�
u8 is_safe(void);			// �����¶Ⱥ�����Ũ���ж��Ƿ�ȫ

u8 face_recognition();		// ����ʶ��		δʵ�֣�
u8 face_add(void);			// ¼������		δʵ�֣�
void face_delete(void);		//ɾ������		δʵ�֣�

u8 fingerprint_recognition();	// ָ��ʶ��
u8 fingerprint_add(void);		//¼��ָ��
void fingerprint_delete(void);	//ɾ��ָ��


//����Ϊ���ӿڵ��õĹ����Ӻ���
u16 get_temperature(void); 	// ��ȡ�¶Ȳ���
u16 get_gas(void);			// ��ȡ����Ũ�Ȳ���		δʵ�֣�

void upload_pic(void);		//�ϴ�ͼƬ����������
u8 wait_ack(void);			//�ȴ����ڻش����		δʵ�֣�

u8 press_FR(void);				//ˢָ��
void ShowErrMessage(u8 ensure);	//��ʾȷ���������Ϣ

//���غ���
int main(void){	 
	// ��������
	u8 sensor=0;
	u8 res;							 
	u8 *pname;				//��·�����ļ��� 
	u8 key;					//��ֵ		   
	u32 i;						 
	u8 sd_ok=1;				//0,sd��������;1,SD������.
	u16 color;
	u8 grey;
	 
	// ������Դ��ʼ��
	delay_init();	    	 //��ʱ������ʼ��	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200
 	usmart_dev.init(72);		//��ʼ��USMART		
 	LED_Init();		  			//��ʼ����LED���ӵ�Ӳ���ӿ�
	KEY_Init();					//��ʼ������
	BEEP_Init();        		//��������ʼ��	 
	W25QXX_Init();				//��ʼ��W25Q128
 	my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ��
	exfuns_init();				//Ϊfatfs��ر��������ڴ�  
 	f_mount(fs[0],"0:",1); 		//����SD�� 
 	f_mount(fs[1],"1:",1); 		//����FLASH. 
	DS18B20_Init();				//�¶ȴ�������ʼ��
	OV7725_Init();
	OV7725_Window_Set(320,240,0);//QVGAģʽ���
	OV7725_CS=0;
	TIM6_Int_Init(10000,7199);			//10Khz����Ƶ��,1�����ж�									  
	EXTI8_Init();						//ʹ�ܶ�ʱ������
	printf("Ready!\r\n");
	
	// ���س������
	while(1){
		key = KEY_Scan(0);
		if(key == KEY0_PRES)upload_pic();
	}												    
}


u16 get_temperature(){
	/**
		����ֵΪ��ǰ������϶�*10
	*/
	u16 temperature;
	temperature = DS18B20_Get_Temp();
	printf("��ǰ�¶�Ϊ%d.%d��C\r\n",temperature/10,temperature%10);	//��ӡ�¶ȵ�����λ��С��λ
	return temperature;
}

u8 is_safe(){
	/**
	* �����غ�������
	* �����¶Ⱥ�����Ũ�Ȳ����жϻ����Ƿ�ȫ
	* ����ȫ�������ơ�����������������������0
	* ��ȫ��Ϩ���ơ��رշ�����������0
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

u8 fingerprint_recognition(){
	/**
	* �����غ�������
	* ����Ƿ���ָ�ư��²�������Ӧ
	* ����ָ�ư��»���ָ�ƴ��󣬷���0
	* ������ȷָ�ư��£�����1
	*/
	if(PS_Sta){ //���PS_Sta״̬���������ָ����
		press_FR();//ˢָ��
		delay_ms(600);
	}
}

u8 press_FR(void){
	/**
	* ��ָ��ʶ���ӳ������
	* ����ָ���Ƿ��ڿ���
	* �ڿ����򷵻�1
	* ���ڿ����򷵻�0
	*/
	SearchResult seach;
	u8 ensure;
	u8 ret = 0;
	char *str;
	ensure=PS_GetImage();
	if(ensure==0x00)//��ȡͼ��ɹ� 
	{	
		ensure=PS_GenChar(CharBuffer1);
		if(ensure==0x00) //���������ɹ�
		{		
			ensure=PS_HighSpeedSearch(CharBuffer1,0,AS608Para.PS_max,&seach);
			if(ensure==0x00)//�����ɹ�
			{				
				printf("ˢָ�Ƴɹ�");				
				str=mymalloc(SRAMIN,50);
				sprintf(str,"ȷ�д���,ID:%d  ƥ��÷�:%d",seach.pageID,seach.mathscore);
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


u8 fingerprint_add(void){
	/**
	* �����غ�������
	* ��������ָ������
	* ��ӳɹ��򷵻�1
	* ���ʧ���򷵻�0
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
				printf("�밴ָ��");
				ensure=PS_GetImage();
				if(ensure==0x00) {
					BEEP=1;
					ensure=PS_GenChar(CharBuffer1);//��������
					BEEP=0;
					if(ensure==0x00){
						printf("ָ������");
						i=0;
						processnum=1;//�����ڶ���						
					}else ShowErrMessage(ensure);				
				}else ShowErrMessage(ensure);						
				break;
			case 1:
				i++;
				printf("�밴�ٰ�һ��ָ��");
				ensure=PS_GetImage();
				if(ensure==0x00) {
					BEEP=1;
					ensure=PS_GenChar(CharBuffer2);//��������
					BEEP=0;
					if(ensure==0x00){
						printf("ָ������");
						i=0;
						processnum=2;//����������
					}else ShowErrMessage(ensure);	
				}else ShowErrMessage(ensure);		
				break;
			case 2:
				printf("�Ա�����ָ��");
				ensure=PS_Match();
				if(ensure==0x00) {
					printf("�Աȳɹ�,����ָ��һ��");
					processnum=3;//�������Ĳ�
				}
				else {
					printf("�Ա�ʧ�ܣ�������¼��ָ��");
					ShowErrMessage(ensure);
					i=0;
					processnum=0;//���ص�һ��		
				}
				delay_ms(1200);
				break;
			case 3:
				printf("����ָ��ģ��");
				ensure=PS_RegModel();
				if(ensure==0x00) {
					printf("����ָ��ģ��ɹ�");
					processnum=4;//�������岽
				}else {processnum=0;ShowErrMessage(ensure);}
				delay_ms(1200);
				break;
			case 4:	
				printf("�����봢��ID,��KEY_0������,��KEY_1����,��KEY_UP����");
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
				ensure=PS_StoreChar(CharBuffer2,ID);//����ģ��
				if(ensure==0x00) {							
					printf("¼��ָ�Ƴɹ�");
					PS_ValidTempleteNum(&ValidN);//����ָ�Ƹ���
					//printf(AS608Para.PS_max-ValidN);
					delay_ms(1500);LED0=0;
					LED1=1;
					return 1;
				}else {processnum=0;ShowErrMessage(ensure);}					
				break;				
		}
		delay_ms(400);
		if(i==5){ //����5��û�а���ָ���˳�
			break;	
		}				
	}
	return 0;
}



void fingerprint_delete(void){
	/**
	* �����غ�������
	* ɾ��ָ��IDָ�ƻ����ָ�ƿ�
	*/
	u8  ensure;
	u16 key;
	u8 ID = 1;
	printf("ɾ��ָ��");
	printf("������ɾ��ID,��KEY_0������,��KEY_1����,��KEY_UP����, ID=0��ʾ���ָ�ƿ�");
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
		ensure=PS_Empty();//���ָ�ƿ�
	else 
		ensure=PS_DeletChar(ID,1);//ɾ������ָ��
	if(ensure==0){
		printf("ɾ��ָ�Ƴɹ�");		
	}
  else
		ShowErrMessage(ensure);	
	delay_ms(1200);
	PS_ValidTempleteNum(&ValidN);//����ָ�Ƹ���
	delay_ms(50);
}


void ShowErrMessage(u8 ensure){
	/**
	* ��ָ��ʶ����غ�������
	* ��ʾȷ���������Ϣ
	*/
	printf(EnsureMessage(ensure));
}

void upload_pic(void){
	/**
	* ������ʶ��������
	* �ϴ�����ͷ�ɼ�����ͼ��������python
	*/
	u32 j;
 	u16 color;	
	u8	grey;
	if(ov_sta)//��֡�жϸ��£�
	{
		OV7725_RRST=0;				//��ʼ��λ��ָ�� 
		OV7725_RCK_L;
		OV7725_RCK_H;
		OV7725_RCK_L;
		OV7725_RRST=1;				//��λ��ָ����� 
		OV7725_RCK_H; 
		for(j=0;j<76800;j++)
		{
			OV7725_RCK_L;
			color=GPIOC->IDR&0XFF;	//������
			OV7725_RCK_H; 
			color<<=8;  
			OV7725_RCK_L;
			color|=GPIOC->IDR&0XFF;	//������
			OV7725_RCK_H; 
			printf("%d",color);
			delay_ms(50);
		}   							  
 		ov_sta=0;					//����֡�жϱ��
		ov_frame++; 
	} 
}
