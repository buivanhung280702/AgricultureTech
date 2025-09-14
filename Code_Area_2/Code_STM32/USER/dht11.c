#include "dht11.h"
//#include "Delay.h"

void ddelay_ms(u16 nms)
{
        volatile uint32_t nCount;
        RCC_ClocksTypeDef RCC_Clocks;
        RCC_GetClocksFreq (&RCC_Clocks);

        nCount=(RCC_Clocks.HCLK_Frequency/10000)*nms;
        for (; nCount!=0; nCount--);
}

void ddelay_us(u32 nus)
{
        volatile uint32_t nCount;
        RCC_ClocksTypeDef RCC_Clocks;
        RCC_GetClocksFreq (&RCC_Clocks);

        nCount=(RCC_Clocks.HCLK_Frequency/10000000)*nus;
        for (; nCount!=0; nCount--);
}

u8 DHT11_Init(void){
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;			    
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//GPIO_Mode_Out_PP; 	 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//GPIO_Speed_50MHz;	 
  GPIO_Init(GPIOA, &GPIO_InitStructure);			    
	
	DHT11_DQ_OUT=1;
	DHT11_Rst();
	
	return DHT11_Check();
}

void DHT11_Rst(void){
	
	DHT11_IO_OUT();
	
	DHT11_DQ_OUT=0;
	ddelay_ms(20);
	DHT11_DQ_OUT=1;
	ddelay_us(30);
	
}

u8 DHT11_Check(void){
	u8 time=0;
	DHT11_IO_IN();
	while(DHT11_DQ_IN&&time<100){time++;ddelay_us(1);}//cho xuong
	if(time>=100) return 1;//loi
	else time=0;
	while(!DHT11_DQ_IN&&time<100){time++;ddelay_us(1);}// cho len
	if(time>=100) return 1;//loi
	else return 0;
	
	//return 0;
}

u8 DHT11_Read_Bit(void){
	
	u8 time=0;
	
	while(DHT11_DQ_IN&&time<100){time++;ddelay_us(1);}//cho xuong

	time=0;
	while(!DHT11_DQ_IN&&time<100){time++;ddelay_us(1);}// cho len
	ddelay_us(40);
	if(DHT11_DQ_IN) return 1;
	else return 0;
	
	//return 0;
}

u8 DHT11_Read_Byte(void){
	u8 i=0;
	u8 data=0;//00000001
	
	for(i=0;i<8;i++){
		
		data<<=1;
		data|=DHT11_Read_Bit();
		
	}
	return data;
}

u8 DHT11_Read_Data(u8 *temp,u8 *humi){
	u8 i=0;
	u8 buf[5];
	
	DHT11_Rst();
	
	if(DHT11_Check()==0)
	{
		for(i=0;i<5;i++){
			
			buf[i]=DHT11_Read_Byte();
			
		}
		
		if(buf[0]+buf[1]+buf[2]+buf[3]==buf[4])
		{
			*humi=buf[0]; 
			*temp=buf[2];
		}
	}else return 1;//loi
	return 0;//ok
}

