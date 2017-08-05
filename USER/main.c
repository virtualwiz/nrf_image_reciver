#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "lcd.h"
#include "led.h"
#include "spi.h"
#include "24l01.h"   

u8 Do_Calculate_Rate = 0;

void LCD_ShowMenuItem(u8 line,u8 *p){
	LCD_ShowString(5,line,239,24,24,p,0);
}

void LCD_ClearMenuArea(){
	LCD_Fill(0,20,239,308,WHITE);
}

void LCD_DrawIndex(){
	LCD_Clear(WHITE);
	POINT_COLOR = BLACK;
	//void LCD_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p)
	LCD_ShowString(5,0,240,12,12,"COPYRIGHT (C) 2016 OWLET INDUSTRIES",1);
	LCD_ShowString(5,307,240,12,12,"NRF REMOTE TUNER V2.5",1);
}

void TIM3_Int_Init(u16 arr,u16 psc)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能

	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	 计数到5000为500ms
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值  10Khz的计数频率  
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(  //使能或者失能指定的TIM中断
		TIM3, //TIM2
		TIM_IT_Update ,
		ENABLE  //使能
		);
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器

	TIM_Cmd(TIM3, ENABLE);  //使能TIMx外设
							 
}

void TIM3_IRQHandler(void)   //TIM3中断
{
	Do_Calculate_Rate = 1;
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
		{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  //清除TIMx的中断待处理位:TIM 中断源 
		}
}

void Func_Main_Menu()
{
	LCD_ClearMenuArea();
	LCD_ShowMenuItem(LINE3,"[MAIN MENU]");
	LCD_ShowMenuItem(LINE5,"1.NRF Listener");
	LCD_ShowMenuItem(LINE6,"2.Data Logger");
	LCD_ShowMenuItem(LINE7,"3.Image Viewer");
	LCD_ShowMenuItem(LINE8,"4.Terminal");
}
u8 tmp_buf[32];
void Func_Image_Viewer()
{
	u8 k;
	u16 color;
	NRF24L01_RX_Mode();
	LCD_Scan_Dir(U2D_L2R);		//从上到下,从左到右 
	LCD_SetCursor(0x00,0x0000);	//设置光标位置 
	LCD_WriteRAM_Prepare();     //开始写入GRAM
	while(1)
	{
		if(NRF24L01_RxPacket(tmp_buf)==0)
		{
			LED0 = 0;
			if(tmp_buf[0]=='D' && tmp_buf[1]=='E' && tmp_buf[2]=='A' && tmp_buf[3]=='D')
			{
				LCD_Scan_Dir(U2D_L2R);		//从上到下,从左到右 
				LCD_SetCursor(0x00,0x0000);	//设置光标位置 
				LCD_WriteRAM_Prepare();     //开始写入GRAM
			}
			else
			{
				for(k=0;k<16;k++)
				{
					color = 0;
					color = tmp_buf[2*k]<<8 | tmp_buf[2*k+1];
					LCD_WriteRAM(~color);
					//((LCD_TypeDef *) LCD_BASE)->LCD_RAM=color;
				}
			}
			LED0 = 1;
		}
	}
}

void Func_Data_Counter()
{
	u32 Packages_Count = 0;
	u16 Packages_Count_1s = 0;
	u8 tmp_buf[33];
	NRF24L01_RX_Mode();
	
	LCD_ClearMenuArea();
	LCD_ShowString(5,40,240,24,24,"Incoming message:",1);
	LCD_ShowString(5,120,240,24,24,"Rx Packages:",1);	
	LCD_ShowString(5,170,240,24,24,"Data Flow:",1);
	LCD_ShowString(5,220,240,24,24,"Speed:",1);		
	while(1){
		if(NRF24L01_RxPacket(tmp_buf)==0)//一旦接收到信息,则显示出来.
		{
			tmp_buf[32]=0;//加入字符串结束符
			Packages_Count++;
			Packages_Count_1s++;
		}
		
		if(Do_Calculate_Rate)
		{
			LCD_ShowString(0,64,239,32,16,tmp_buf,0);
			LCD_ShowxNum(10,144,Packages_Count,18,24,0);
			LCD_ShowxNum(10,194,Packages_Count*32,18,24,0);
			LCD_ShowxNum(10,244,Packages_Count_1s * 32*3,11,24,0);
			LCD_ShowString(160,244,100,24,24,"Byte/s",1);
			Packages_Count_1s = 0;
			Do_Calculate_Rate = 0;
		}
		
	}
}

int main(void){

	delay_init();	    	 //延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(9600);	 //串口初始化为9600
	TIM3_Int_Init(3333,7199);
	LCD_Init();
	LED_Init();
	LCD_DrawIndex();
	NRF24L01_Init();
	
	while(NRF24L01_Check())	//检查NRF24L01是否在位.	
	{
		LCD_ShowString(60,130,200,16,16,"NRF24L01 Error",0);
		delay_ms(200);
		LCD_Fill(60,130,239,130+16,WHITE);
 		delay_ms(200);
	}
	
	//Func_Main_Menu();
	Func_Data_Counter();
	Func_Image_Viewer();
	while(1);
}

