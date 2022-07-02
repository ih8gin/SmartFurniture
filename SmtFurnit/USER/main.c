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
	LEDx = 0 ��
		 = 1 ��
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
#define usart2_baund  57600//����2�����ʣ�����ָ��ģ�鲨���ʸ���
extern u8 ov_sta;	//��exit.c���涨��
extern u8 ov_frame;	//��timer.c���涨��	 
SysPara AS608Para;//ָ��ģ��AS608����
u16 ValidN;//ģ������Чָ�Ƹ���


//����Ϊ�����غ������õĽӿ�
u8 is_safe(void);			// �����¶Ⱥ�����Ũ���ж��Ƿ�ȫ

u8 face_recognition();		// ����ʶ��		δʵ�֣�
u8 face_add(void);			// ¼������
void face_delete(void);		// ɾ������

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
	u8 status;
	u8 key;
	u8 ensure;
	char *str;	
	 
	// ������Դ��ʼ��
	delay_init();	    	 //��ʱ������ʼ��	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200
	usart2_init(usart2_baund);//��ʼ������2,������ָ��ģ��ͨѶ
	PS_StaGPIO_Init();	//��ʼ��FR��״̬����		
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
	printf("Ready!\n");
	
	printf("��AS608ģ������....\r\n");	
	while(PS_HandShake(&AS608Addr))//��AS608ģ������
	{
		delay_ms(400);
		printf("δ��⵽ģ��!!!\r\n");
		delay_ms(800);
		printf("��������ģ��...\r\n");		  
	}
	printf("ͨѶ�ɹ�!!!\r\n");
	str=mymalloc(SRAMIN,30);
	sprintf(str,"������:%d   ��ַ:%x",usart2_baund,AS608Addr);
	printf((u8*)str);
	ensure=PS_ValidTempleteNum(&ValidN);//����ָ�Ƹ���
	if(ensure!=0x00)
		ShowErrMessage(ensure);//��ʾȷ���������Ϣ	
	ensure=PS_ReadSysPara(&AS608Para);  //������ 
	if(ensure==0x00)
	{
		mymemset(str,0,50);
		sprintf(str,"������:%d     �Աȵȼ�: %d",AS608Para.PS_max-ValidN,AS608Para.PS_level);
		printf((u8*)str);
	}
	else
		ShowErrMessage(ensure);	
	myfree(SRAMIN,str);
	
	// ����������
	while(1){
		status = is_safe();		// �жϻ����Ƿ�ȫ
		if(status){
			status = 0;
			LED1=1;
			BEEP=0;
			LED0=0;				// ��ȫ������Ʊ�ʾ�����ر�
			status = fingerprint_recognition();		// ���ָ��
			key = KEY_Scan(0);
			if(key == WKUP_PRES){					// ����KEY_UP��������
				if(face_recognition() == 49){
					printf("������֤ͨ��!\r\n");
					status = 1;
				}else{
					printf("������֤ʧ��!\r\n");
				}
			}
			while(status){		// ��ָ�ƻ�����ʶ��ͨ�������̵Ʊ�ʾ�����򿪣�������޸�ָ�ƿ��Ȩ��
				LED0=1;
				LED1=0;
				key = KEY_Scan(0);
				if(key == KEY0_PRES){			// ��KEY_0¼ָ��
					fingerprint_add();
				}else if(key == KEY1_PRES){		// ��KEY_1ɾָ��
					fingerprint_delete();
				}else if(key == WKUP_PRES){		// ��KEY_UP�ͷ�Ȩ�ޣ��ر�����
					break;
				}delay_ms(50);
			}
			delay_ms(50);
		}else{
			LED1=0;
			BEEP=1;
		}delay_ms(50);
	}												    
}



//�ӿ�ʵ��

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
		return 0;
	}else{
		return 1;
	}
}

u8 face_recognition(){
	/**
	* �����غ�������
	* ������ͷ�ɼ�����ͼ������ͨ�������ϴ�����λ��
	* �ȴ�����λ��������ɺ󷵻ص��ź�֡
	* ��ʶ�𵽿�������������1
	* ��δʶ����Ч����������0
	*/
	u8 ret;
	upload_pic();
	printf("upload finished\r\n");
	ret = wait_ack();
	return ret;
}

u8 face_add(void){
	/**
	* �����غ�������
	* �����������
	* ����ӳɹ�������1
	* �����ʧ�ܣ�����0
	*/
	printf("����python��λ�����������Ӳ������в�֧����stm32ִ�д˲���");
	return 0;
}

void face_delete(void){
	/**
	* �����غ�������
	* ɾ��ָ�������������������������
	*/
	printf("����python��λ�����������ɾ���������в�֧����stm32ִ�д˲���");
}

u8 fingerprint_recognition(){
	/**
	* �����غ�������
	* ����Ƿ���ָ�ư��²�������Ӧ
	* ����ָ�ư��»���ָ�ƴ��󣬷���0
	* ������ȷָ�ư��£�����1
	*/
	u8 ret = 0;
	if(PS_Sta){ //���PS_Sta״̬���������ָ����
		ret = press_FR();//ˢָ��
		delay_ms(600);
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
				printf("�밴ָ��\r\n");
				ensure=PS_GetImage();
				if(ensure==0x00) {
					ensure=PS_GenChar(CharBuffer1);//��������
					if(ensure==0x00){
						printf("ָ������\r\n");
						i=0;
						processnum=1;//�����ڶ���						
					}else ShowErrMessage(ensure);				
				}else ShowErrMessage(ensure);						
				break;
			case 1:
				i++;
				printf("�밴�ٰ�һ��ָ��\r\n");
				ensure=PS_GetImage();
				if(ensure==0x00) {
					ensure=PS_GenChar(CharBuffer2);//��������
					if(ensure==0x00){
						printf("ָ������\r\n");
						i=0;
						processnum=2;//����������
					}else ShowErrMessage(ensure);	
				}else ShowErrMessage(ensure);		
				break;
			case 2:
				printf("�Ա�����ָ��\r\n");
				ensure=PS_Match();
				if(ensure==0x00) {
					printf("�Աȳɹ�,����ָ��һ��\r\n");
					processnum=3;//�������Ĳ�
				}
				else {
					printf("�Ա�ʧ�ܣ�������¼��ָ��\r\n");
					ShowErrMessage(ensure);
					i=0;
					processnum=0;//���ص�һ��		
				}
				delay_ms(1200);
				break;
			case 3:
				printf("����ָ��ģ��\r\n");
				ensure=PS_RegModel();
				if(ensure==0x00) {
					printf("����ָ��ģ��ɹ�\r\n");
					processnum=4;//�������岽
				}else {processnum=0;ShowErrMessage(ensure);}
				delay_ms(1200);
				break;
			case 4:	
				printf("�����봢��ID,��KEY_0������,��KEY_1����,��KEY_UP����\r\n");
				printf("ID = %d ? \r\n",ID);	
				while(1){
					key=KEY_Scan(0);
					if(key==KEY0_PRES){
						ID+=1;
						printf("ID = %d ? \r\n",ID);
					}
					if(key==KEY1_PRES){
						ID=1;
						printf("ID = %d ? \r\n",ID);
					}
					if(key==WKUP_PRES){
						break;
					}
				}
				ensure=PS_StoreChar(CharBuffer2,ID);//����ģ��
				if(ensure==0x00) {							
					printf("¼��ָ�Ƴɹ�\r\n");
					//PS_ValidTempleteNum(&ValidN);//����ָ�Ƹ���
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
	printf("ɾ��ָ��\r\n");
	printf("������ɾ��ID,��KEY_0������,��KEY_1����,��KEY_UP����, ID=0��ʾ���ָ�ƿ�\r\n");
	printf("ID = %d ? \r\n",ID);
	while(1){
		key=KEY_Scan(0);
		if(key==KEY0_PRES){
			ID+=1;
			printf("ID = %d ? \r\n",ID);
		}
		if(key==KEY1_PRES){
			ID=0;
			printf("ID = %d ? \r\n",ID);
		}
		if(key==WKUP_PRES){
			break;
		}
		delay_ms(100);
	}
	if(ID == 0)
		ensure=PS_Empty();//���ָ�ƿ�
	else 
		ensure=PS_DeletChar(ID,1);//ɾ������ָ��
	if(ensure==0){
		printf("ɾ��ָ�Ƴɹ�\r\n");		
	}
  else
		ShowErrMessage(ensure);	
	
	BEEP=1;
	delay_ms(100);
	BEEP=0;
	//PS_ValidTempleteNum(&ValidN);//����ָ�Ƹ���
	delay_ms(50);
}

u16 get_temperature(){
	/**
	* ����ȫ�жϺ�������
	* ���������ػ����¶�
	* ����ֵΪ��ǰ������϶�*10
	*/
	u16 temperature;
	temperature = DS18B20_Get_Temp();
	//printf("��ǰ�¶�Ϊ%d.%d��C\r\n",temperature/10,temperature%10);	//��ӡ�¶ȵ�����λ��С��λ
	return temperature;
}

u16 get_gas(){
	/**
	* ����ȫ�жϺ�������
	* ����������һ����̼����Ũ��
	*/
	printf("������ģ����δ��ɣ�");
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
			printf("%d\n",color);
		}   							  
 		ov_sta=0;					//����֡�жϱ��
	} 
}

u8 wait_ack(void){
	/**
	* ������ʶ����غ�������
	* ����������λ�����ź�֡
	* �յ���������һ�����ź���ȷ��
	* �����ؽ��յ����ź�
	*/
	u8 i;
	u8 recv;
	printf("waiting for ack\r\n");
	while(1){
		if((USART_RX_STA&0x3fff) >= 3){
			printf("recieve:%d\r\n", USART_RX_BUF[0]);
			if(recv == USART_RX_BUF[0]) i++;
			else{
				recv = USART_RX_BUF[0];
				i=0;
			}
			USART_RX_STA=0;
			if(i > 2) return recv;
		}
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
	BEEP=1;
	delay_ms(100);
	BEEP=0;
	return ret;
}

void ShowErrMessage(u8 ensure){
	/**
	* ��ָ��ʶ����غ�������
	* ��ʾȷ���������Ϣ
	*/
	printf(EnsureMessage(ensure));
}