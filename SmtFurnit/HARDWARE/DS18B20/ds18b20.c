#include "ds18b20.h"
#include "delay.h"	


// ��λDS18B20
/*
1����������͵�ƽ�����ֵ͵�ƽʱ������480us����λ���壩
2�������ͷ����ߣ��ߵ�ƽ����ʱ15-60us
           ___
__________|

*/
void DS18B20_Rst(void)	   
{                 
	DS18B20_IO_OUT(); 	// ����IO��Ϊ���ģʽ
	
    DS18B20_DQ_OUT=0; 	// ��������
	
    delay_us(750);    	// ����750us����λ��������480us ��
	
    DS18B20_DQ_OUT=1; 	// �ͷ�����
	
	delay_us(15);     	// ��ʱ15us
}





// ��鴫�����Ƿ���ڣ��ȴ�Ӧ��
// ����1:δ��⵽DS18B20�Ĵ���
// ����0:����
/*
DS18B20�������������ߣ�60-240us���͵�ƽӦ�����壩

      |______

*/
u8 DS18B20_Check(void) 	   
{   
	u8 retry=0;
	DS18B20_IO_IN();	// ����IO��Ϊ����ģʽ 
    while (DS18B20_DQ_IN&&retry<200)// ������Ϊ�ߵ�ƽ����δ��ʱ����ȴ�200us
	{
		retry++;
		delay_us(1);
	};	
	
	if(retry>=200)return 1;// ����ʱ���򷵻�1�������������ڣ�
	else retry=0;// ��retry��������Ϊ0�����Լ�ʱ
  

  
		while (!DS18B20_DQ_IN&&retry<240)// ������Ϊ�͵�ƽ���ҵ͵�ƽ���С��240us����ȴ������������߻�ʱ������ѭ��
	{
		retry++;
		delay_us(1);
	};
	
	if(retry>=240)return 1;// �����͵�ʱ�䳬��240us���򷵻�1�������������ڣ�	    
	return 0;// ֻҪ����͵�ƽ���С��240us���ͷ���0�����������ڣ�
}






// ��DS18B20��ȡһλ
// ����ֵ��1/0
/*
���ж�ʱ��������Ҫ60us����������дʱ��֮��������Ҫ1us�ָ�ʱ��
ÿ����ʱ������������������������1us
�����ڶ�ʱ���ڼ�����ͷ����ߣ�������ʱ����ʵ���15us֮�ڲ�������״̬

1����������͵�ƽ��ʱ2us
2������ת������ģʽ��ʱ12us
3����ȡ���ߵ�ǰ��ƽ
4����ʱ50us
*/
u8 DS18B20_Read_Bit(void) 	 
{
  u8 data;				//���ڶ�ȡ�����صı���
	
	DS18B20_IO_OUT();	// ����IO��Ϊ���ģʽ
	
  DS18B20_DQ_OUT=0; // ��������͵�ƽ2us
	delay_us(2);			
	
  DS18B20_DQ_OUT=1; // �ͷ�����
	
	DS18B20_IO_IN();	// ����IO��Ϊ����ģʽ
	
	delay_us(12);			// ��ʱ12us
	
	
	if(DS18B20_DQ_IN)data=1;//��ȡ�������ݣ���Ϊ�ߵ�ƽ��data��Ϊ1
    else data=0;					//��Ϊ�͵�ƽ��data��Ϊ0
	
    delay_us(50);         //��ʱ50us  
	
    return data;					//����data
}





// ��DS18B20��ȡһ���ֽ�(�����еı��ر���ֽ�)
// ����ֵ����ȡ�����ֽ�
u8 DS18B20_Read_Byte(void)     
{        
    u8 i,j,dat;// ��������i����ȡ����λj������dat
    dat=0;		 // dat��ʼ��Ϊ0
	for (i=1;i<=8;i++)//��j��ֵ��datÿһλ 
	{
        j=DS18B20_Read_Bit();
        dat=(j<<7)|(dat>>1);// j��������Ϊjֻ��һλ��Ч�����������λ��ֵ��dat���λ��Ȼ��dat���ƣ�j���¸�ֵ��ֱ���ʼj��ֵ������dat���λ
    }						    
    return dat;							//���ض�ȡ�����ֽ�
}






// дһ���ֽڵ�DS18B20
/*
����дʱ��������Ҫ60us����������дʱ��֮��������Ҫ1us�ָ�ʱ��
����дʱ���ʼ��������������

1��д1ʱ����������͵�ƽ����ʱ2us���ͷ����ߣ���ʱ60us
   ___________________
__|

2��д0ʱ����������͵�ƽ����ʱ60us���ͷ����ߣ���ʱ2us
                    __
___________________|

*/

// dat��Ҫд����ֽ�
void DS18B20_Write_Byte(u8 dat)     
 {             
    u8 j;// ��������
    u8 testb;// ÿ��������ݵĵ����λ
	 
	DS18B20_IO_OUT();	// ����IO��Ϊ���
	 
    for (j=1;j<=8;j++)// ÿ��дһλ 
	{
        testb=dat&0x01;// ��ȡ�������λ
        dat=dat>>1;// ������λ���ε�λ��λ�����λ
		
		
        if (testb)// �������λΪ1 
        {
            DS18B20_DQ_OUT=0;	// ��������͵�ƽ
            delay_us(2);      // ��ʱ2us                      
            DS18B20_DQ_OUT=1; // �ͷ�����
            delay_us(60);     // ��ʱ60us        
        }
				
				
        else// �������λΪ0 
        {
            DS18B20_DQ_OUT=0;	// ��������͵�ƽ
            delay_us(60);     // ��ʱ60us        
            DS18B20_DQ_OUT=1; // �ͷ�����
            delay_us(2);      // ��ʱ2us                    
        }
    }
}
 





//��ʼ�¶�ת��
void DS18B20_Start(void) 
{   						               
    DS18B20_Rst();	// ��λ   
	DS18B20_Check();	// ��⴫�����Ƿ���ڣ��ȴ�Ӧ��
    DS18B20_Write_Byte(0xcc);	// ����rom����Ϊֻ��һ��������������ҪУ�飩
    DS18B20_Write_Byte(0x44);	// ���Ϳ�ʼת������
} 





//��ʼ��DS18B20��IO�� DQ ͬʱ���DS�Ĵ���
u8 DS18B20_Init(void)
{
 	GPIO_InitTypeDef  GPIO_InitStructure;
 	
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);	 //ʹ��IO��G��ʱ�� 
	
 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;				//IO��G.11 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 	//�����������	  
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //���ô����ٶ�50MHz
 	GPIO_Init(GPIOG, &GPIO_InitStructure);

 	GPIO_SetBits(GPIOG,GPIO_Pin_11);    //���1

	DS18B20_Rst();											//��λ

	return DS18B20_Check();							//���أ���鴫�����Ƿ���ڣ�ֵ��1�����ڣ�0����
}  





//��ds18b20�õ��¶�ֵ
//���ȣ�0.1C
//����ֵ���¶�ֵ ��-550~1250�� 
/*
ת����õ�12λ���ݣ�16λ��ǰ5λΪ����λ
LSB:2^3, 2^2, 2^1, 2^0, 2^-1, 2^-2, 2^-3, 2^-4
MSB: S ,  S ,  S ,  S ,  S ,  2^6,  2^5,  2^4
*/
short DS18B20_Get_Temp(void)
{
    u8 temp;
    u8 TL,TH;
	short tem;
    DS18B20_Start ();  			  // ���ú�������ʼת��
    DS18B20_Rst();						// ��λ
    DS18B20_Check();	 				// ��鴫�����Ƿ���ڣ��ȴ�Ӧ��
    DS18B20_Write_Byte(0xcc);	// ����rom����Ϊֻ��һ��������������ҪУ�飩
    DS18B20_Write_Byte(0xbe);	// ���Ϳ�ʼת������	    
    TL=DS18B20_Read_Byte(); 	// ��ȡ���ֽڲ���ֵ��TL   
    TH=DS18B20_Read_Byte(); 	// ��ȡ���ֽڲ���ֵ��TH  



	
	
//����	
    if(TH>7)//����λΪ1��2^0+2^1+2^2=7��
    {
        TH=~TH;
        TL=~TL; 
        temp=0;					//�¶�Ϊ��  
    }else temp=1;				//�¶�Ϊ��	  	  
    tem=TH; 					//�Ƚ��߰�λ��ֵ��tem
    tem<<=8;    			//��THֵ��tem�Ͱ�λ�Ƶ��߰�λ
    tem+=TL;					//������λ�ϳ�Ϊһ��ʮ��λ
    tem=(float)tem*0.625;		//�����������λ�仯1��ӳ���¶ȱ仯0.0625��125��Ϊ07d0h��   
	if(temp)return tem; 		//���¶�ֵΪ���������¶�ֵ
	else return -tem;    		//���¶�ֵΪ����ȡ���󷵻��¶�ֵ
}



 
