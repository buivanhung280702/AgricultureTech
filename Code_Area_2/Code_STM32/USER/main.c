#include "stm32f10x.h"                  // Device header
#include "FreeRTOS.h"                   // ARM.FreeRTOS::RTOS:Core
#include "task.h"                       // ARM.FreeRTOS::RTOS:Core
#include "queue.h"                      // ARM.FreeRTOS::RTOS:Core
#include "semphr.h"                     // ARM.FreeRTOS::RTOS:Core
#include "event_groups.h"               // ARM.FreeRTOS::RTOS:Event Groups
#include "FreeRTOSConfig.h"             // ARM.FreeRTOS::RTOS:Config
#include "dht11.h"
#include "uart.h" 
#include "i2c.h"
#include "i2c_lcd.h"
#include "stdio.h"
#include "Delay.h"
#include <math.h>
__IO uint32_t ADC_ARRAY[2];
long map(long x, long in_min, long in_max, long out_min, long out_max);

typedef struct
{
	uint8_t Sensor_Temperature;
	uint8_t Sensor_Humidity;
	uint8_t Sensor_Light;
	uint8_t Sensor_SoilMoisture;
}sensor_DataStruct_t;

xQueueHandle xQueue;

void SYS_Init(void);
void ADC_Inits(void);
void Relay_Inits(void);

void TaskReadSensor(void *p); 
void TaskSendSensor(void *p); 
void TaskGetControl(void *p);
void TaskProcess(void *p);
int main (void){
	xQueue = xQueueCreate(1, sizeof(sensor_DataStruct_t));
	SYS_Init();
	I2C_LCD_Init();
	I2C_LCD_BackLight(1);
	xTaskCreate(TaskReadSensor, (const char*) "Read", 512, NULL, 1, NULL);
	xTaskCreate(TaskSendSensor, (const char*) "Send", 512, NULL, 1, NULL);
	xTaskCreate(TaskGetControl, (const char*) "Send", 512, NULL, 1, NULL);
	vTaskStartScheduler();
	return 0; 
}

char buffer[20];
char buffer1[20];
uint8_t temp  = 0, humi = 0;

void TaskReadSensor(void *p)
{
	sensor_DataStruct_t sendDataSensor = {0,0,0,0};

	while(1)
	{
		unsigned char i = 0;
		unsigned int SumLight = 0, SumSoilMoisture  = 0;
		DHT11_Read_Data(&temp,&humi);
		sendDataSensor.Sensor_Temperature 	= temp;
		sendDataSensor.Sensor_Humidity			= humi;
		for(i=0; i<50; i++)
		{
			SumLight 				+= ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_2)/10;
			SumSoilMoisture += ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1)/10;
		}
		SumLight 				/= 50;
		SumSoilMoisture /= 50;
		if(SumSoilMoisture < 290)
		{
			SumSoilMoisture = 290;
		}
		else if(SumSoilMoisture > 390)
		{
			SumSoilMoisture = 390;
		}
		
		if(SumLight < 70)
		{
			SumLight = 70;
		}
		else if(SumLight > 350)
		{
			SumLight = 350;
		}
		
//		I2C_LCD_Clear();
//		sprintf(buffer, "%d %d", sendDataSensor.Sensor_Temperature, sendDataSensor.Sensor_Humidity);
//		I2C_LCD_Puts(buffer);
//		
			
		sendDataSensor.Sensor_SoilMoisture 	= (uint8_t)map(SumSoilMoisture,290,390,100, 0);
		sendDataSensor.Sensor_Light 				= (uint8_t)map(SumLight, 70, 350, 100, 0);
		unsigned char lastLight = 0;
		if(xQueue != 0)
		{	
			if(sendDataSensor.Sensor_Light != lastLight){
				if (xQueueOverwrite(xQueue, (void *)&sendDataSensor)){
					lastLight = sendDataSensor.Sensor_Light;
				} 
			}
		}
		vTaskDelay(10/portTICK_PERIOD_MS);   
	}
}

void TaskSendSensor(void *p)
{
	char ARR_Number[50];
	
	while(1)
	{
		sensor_DataStruct_t getDataSensor;
		if(xQueueReceive(xQueue, (void*)&getDataSensor, (portTickType)0))
		{
			//UART2_SendByte((uint8_t*)(&getDataSensor), sizeof(getDataSensor));
			I2C_LCD_Clear();
			I2C_LCD_Puts("Node1");
			I2C_LCD_NewLine();
			sprintf(ARR_Number,"%d %d %d %d!",getDataSensor.Sensor_Temperature, getDataSensor.Sensor_Humidity, getDataSensor.Sensor_SoilMoisture, getDataSensor.Sensor_Light);
			
			I2C_LCD_Puts(ARR_Number);
			
			
			UART1_SendString(ARR_Number);//gui du lieu cho esp
		}
		vTaskDelay(5000/portTICK_PERIOD_MS);   
	}
}

void TaskGetControl(void *p)
{
	while(1)
	{
		if(Flag3_Receive == 1)
		{
			Flag3_Receive = 0;
			
			//UART3_SendChar(Array1_Receive[0]);
//			UART1_SendString(" ");
//			UART1_SendChar(Array2_Receive[2]);
//			UART1_SendString("\n");
			
			if(Array3_Receive[0] == '0')
			{
				//UART1_SendString("Relay1: OFF ");
				GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
			}
			else if(Array3_Receive[0] == '1')
			{
				//UART1_SendString("Relay1: ON ");

				GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
			}
			
		}
	vTaskDelay(100/portTICK_PERIOD_MS);
	}
}

void TaskProcess(void *p)
{
	
}

void SYS_Init(void)
{
	SystemInit();
	SystemCoreClockUpdate();
	Fn_DELAY_Init(72);
	INIT_UART_1(9600);
	INIT_UART_3(115200);
	DHT11_Init();
	ADC_Inits();
	Relay_Inits();
}
/**	
 * |----------------------------------------------------------------------
 * |  *  02, 2014
 *	- Added support for measure Vbat pin with ADC
 *	
 * Pinout
 *	CHANNEL			ADC1	ADC2	ADC3
 *	0							PA0		PA0		PA0
 *	1							PA1		PA1		PA1
 *	2							PA2		PA2		PA2
 *	3							PA3		PA3		PA3
 *	4							PA4		PA4		PF6
 *	5							PA5		PA5		PF7
 *	6							PA6		PA6		PF8
 *	7							PA7		PA7		PF9
 *	8							PB0		PB0		PF10
 *	9							PB1		PB1		PF3
 *	10						PC0		PC0		PC0
 *	11						PC1		PC1		PC1
 *	12						PC2		PC2		PC2
 *	13						PC3		PC3		PC3
 *	14						PC4		PC4		PF4
 *	15						PC5		PC5		PF5
 *
 * |----------------------------------------------------------------------
 */

void ADC_Inits(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	RCC_ADCCLKConfig (RCC_PCLK2_Div6);

	RCC_APB2PeriphClockCmd ( RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1 ,	ENABLE ) ;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_NbrOfChannel = 2;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_InjectedSequencerLengthConfig(ADC1, 2);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_7Cycles5);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_7Cycles5);

	ADC_ExternalTrigInjectedConvConfig( ADC1, ADC_ExternalTrigInjecConv_None );

	ADC_Cmd ( ADC1 , ENABLE ) ;

	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));

	ADC_AutoInjectedConvCmd( ADC1, ENABLE );
	ADC_SoftwareStartInjectedConvCmd ( ADC1 , ENABLE ) ;
}

void Relay_Inits(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOC,	ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
//	GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_SET);
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
