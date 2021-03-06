#include "app.h"
#include "sys_config.h"
#include "sc93f833X_C.h"
#include "data_type.h"
#include "SensorMethod.h"

#include "bsp_i2c_s.h"
#include "bsp_uart.h"
#include "bsp_pwm.h"
#include "bsp_timer.h"
#include "timeout.h"
#include "commu.h"

#include <string.h>

static xdata App_Para_t App_Para;
static xdata IIC_PARA_t I2c_Para[IIC_NUM];
extern  Bsp_Tx_t Tx_Buf;
unsigned char key_static;
unsigned char status;
unsigned char key_press_flag;
unsigned int key_press_cnt;

static xdata unsigned char HEAT_COOL_ARR[3]={0,0,0};
unsigned char HEAT_COOL_START;



#define APP_SET_BACKLIGHT(X)		( App_Para.Tp_LedSta &= (unsigned char)(X))
#define APP_CLR_BACKLIGHT(X)		( App_Para.Tp_LedSta |= (unsigned char)(~X))
#define APP_SET_SENDDATA(X)		(SentData |= (X))
#define APP_CLR_SNEDDATA(X)		(SentData &= ~(X))
#define APP_CLR_SHUTDOWN_CNT()	(App_Para.SystemAutoCnt = 0)
/*****************************************************************************
 * Function	  : App_LedInit
 * Description   : 配置LED脚为输出模式
 * Input           : void  
 * Output		  : None
 * Return		  : 
 * Others		  : 
 * Record
 * 1.Date		  : 20180404
 *   Author       : marco
 *   Modification: Created function

*****************************************************************************/
static void App_LedInit(void){
	P0CON |= (1<<1);
	P2CON |= (1<<2) | (1<<3) | (1<<4) |(1<<5) |(1<<7) |(1<<6);
	P5CON |= (1<<1) | (1<<0);
	P2PH &= ~((1<<7)|(1<<6));
	LED0_PIN = LED1_PIN = LED2_PIN = LED3_PIN = LED4_PIN = LED5_PIN = RED_LED = OFF;         
	
	BACK_LIGHT = 1;				
}

/*****************************************************************************
 * Function	  : App_HeatCoolHandle
 * Description   : 制冷制热模块处理
 * Input           : void  
 * Output		  : None
 * Return		  : 
 * Others		  : 
 * Record
 * 1.Date		  : 20180414
 *   Author       : marco
 *   Modification: Created function

*****************************************************************************/
void App_HeatCoolHandle(void){
	/**
		@brief:处理背光灯显示和控制制冷或制热状态
	*/
	if(App_Para.HC_Sta == HEAT_MODE){
        #if 1
		LED5_PIN = ON;
        RED_LED = ON;
        APP_SET_BACKLIGHT(TP_KEY0_LED);
		Bsp_PwmOff();
		IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);
        #endif
	}else if(App_Para.HC_Sta == COOL_MODE){
	    #if 1
     	LED5_PIN = ON;
        APP_SET_BACKLIGHT(TP_KEY0_LED);
		Bsp_PwmOff();		
		Bsp_PwnFullOn();        
		IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);
        #endif
	}else{
	    #if 1
		Bsp_PwmOff();
		if(App_Para.FootLedSta){
			Bsp_PwmON();
		}
        LED5_PIN = OFF;
        RED_LED = OFF;
		APP_CLR_BACKLIGHT(TP_KEY0_LED);
		IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);
        #endif
	}     
}


void App_LockKeyHandle(void){
	if(App_Para.LockFlag){		
		App_Para.Tp_LedStaBak = App_Para.Tp_LedSta;
		
		if(App_Para.FootLedSta || App_Para.LockFlag){
			APP_SET_BACKLIGHT(TP_KEY5_LED);      
			LED0_PIN = ON;
		}else{
			APP_CLR_BACKLIGHT(TP_KEY5_LED);  
			LED0_PIN = OFF;
		}
		
		if(App_Para.HC_Sta == STOP_MODE){
			APP_CLR_BACKLIGHT(TP_KEY0_LED);	  
			LED5_PIN = OFF;
		}else if((App_Para.HC_Sta == HEAT_MODE)||(App_Para.HC_Sta == COOL_MODE)){
			APP_SET_BACKLIGHT(TP_KEY0_LED);   
            LED5_PIN = ON;
		}
       
        if(App_Para.MASSAGESta == MASSAGE_ON)
        {
           LED1_PIN = ON;
		   APP_SET_BACKLIGHT(TP_KEY4_LED);
        }
        else
        {
           LED1_PIN = OFF;
           APP_CLR_BACKLIGHT(TP_KEY4_LED);
        }
        #if 1
        if(App_Para.WarmSta == WARM_ON)
        {
           LED4_PIN = ON;
           APP_SET_BACKLIGHT(TP_KEY1_LED);
        }
        else
        {
           LED4_PIN = OFF;
           APP_CLR_BACKLIGHT(TP_KEY1_LED);
        }
        #endif 
		BACK_LIGHT = 0;
		//LED0_PIN = ON;
        //APP_SET_BACKLIGHT(TP_KEY5_LED);
		App_Para.Tp_LedSta|= 0X40;
		IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);
	}else{
		if(App_Para.ShutDownFlag){
			App_Para.ShutDownFlag = False;
			App_Para.SystemAutoClsFlag = True;
			App_Para.SystemAutoCnt = 0;
			App_Para.Tp_LedSta = 0;
			App_Para.FootLedSta = FOOT_LED_OFF;
			APP_CLR_BACKLIGHT(TP_BACKLIGHT_LED);
			IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);		
			LED0_PIN = LED1_PIN = LED2_PIN = LED3_PIN = LED4_PIN = LED5_PIN= OFF;
			BACK_LIGHT = 1;
			return ;
		}
		App_Para.Tp_LedSta = App_Para.Tp_LedStaBak;

		if(App_Para.HC_Sta == STOP_MODE){
			APP_CLR_BACKLIGHT(TP_KEY0_LED);	
			LED5_PIN = OFF;
			
		}else if((App_Para.HC_Sta == HEAT_MODE)||(App_Para.HC_Sta == COOL_MODE)){
			APP_SET_BACKLIGHT(TP_KEY0_LED);
            LED5_PIN = ON;
		}
        if(App_Para.FootLedSta || App_Para.LockFlag){
			APP_SET_BACKLIGHT(TP_KEY5_LED);       
			LED0_PIN = ON;
		}else{
			APP_CLR_BACKLIGHT(TP_KEY5_LED);  
			LED0_PIN = OFF;
		}
        
        if(App_Para.MASSAGESta == MASSAGE_ON)
        {
           LED1_PIN = ON;
		   APP_SET_BACKLIGHT(TP_KEY4_LED);
        }
        else
        {
           LED1_PIN = OFF;
           APP_CLR_BACKLIGHT(TP_KEY4_LED);
        }
        #if 1
        if(App_Para.WarmSta == WARM_ON)
        {
           LED4_PIN = ON;
           APP_SET_BACKLIGHT(TP_KEY1_LED);
        }
        else
        {
           LED4_PIN = OFF;
           APP_CLR_BACKLIGHT(TP_KEY1_LED);
        }
		#endif
		IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedStaBak);
        #if 0
		LED0_PIN =App_Para.Tp_LedSta & ~TP_KEY0_LED;
		LED1_PIN =App_Para.Tp_LedSta & ~TP_KEY1_LED;
		LED2_PIN =App_Para.Tp_LedSta & ~TP_KEY2_LED;
		LED3_PIN =App_Para.Tp_LedSta & ~TP_KEY3_LED;
		LED4_PIN =App_Para.Tp_LedSta & ~TP_KEY4_LED;
		LED5_PIN =App_Para.Tp_LedSta & ~TP_KEY5_LED;
        #endif
		BACK_LIGHT = 1;		
        //LED0_PIN = OFF;
        //APP_CLR_BACKLIGHT(TP_KEY5_LED);  //Tp_LedSta |= ~0xdf     0010 0000
	}	
	App_Para.UpDataFlag= True;
	APP_CLR_SHUTDOWN_CNT();
}
#if 1
void App_OpenKeyHandle(void){
	if(!App_Para.LockFlag){
		App_Para.UpDataFlag= True;
		App_Para.Pushrod_Status = PUSHROD_A_OPEN;
		APP_CLR_SHUTDOWN_CNT();
	}
}

void App_CLoseKeyHandle(void){
	if(!App_Para.LockFlag){		
		App_Para.UpDataFlag= True;
		App_Para.Pushrod_Status = PUSHROD_A_CLOSE;
		APP_CLR_SHUTDOWN_CNT();
	}
}
#endif
void App_LightKeyHandle(void){
	if(!App_Para.LockFlag){
		App_Para.UpDataFlag= True;
		
		if(App_Para.FootLedSta == FOOT_LED_OFF){
			APP_SET_BACKLIGHT(TP_KEY5_LED);
            LED0_PIN = ON;
			App_Para.FootLedSta = FOOT_LED_ON;
			if(App_Para.HC_Sta == STOP_MODE){
				Bsp_PwmON();
			}
		}else{
			APP_CLR_BACKLIGHT(TP_KEY5_LED);
			LED0_PIN = OFF;
			App_Para.FootLedSta = FOOT_LED_OFF;
			if(App_Para.HC_Sta == STOP_MODE){
				Bsp_PwmOff();
			}
		}
		IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);
		APP_CLR_SHUTDOWN_CNT();
	}
}

void App_MassKeyHandle(void){
	if(!App_Para.LockFlag){
		App_Para.UpDataFlag= True;
		
		if(App_Para.MASSAGESta != MASSAGE_ON){
			APP_SET_BACKLIGHT(TP_KEY4_LED);
			App_Para.MASSAGESta = MASSAGE_ON;
            App_Para.MassAutoClsFlag = True;
            App_Para.MassAutoCnt = 0;
			LED1_PIN = ON;
		}else{
			APP_CLR_BACKLIGHT(TP_KEY4_LED);
			LED1_PIN = OFF;
			App_Para.MASSAGESta = MASSAGE_OFF;
            App_Para.MassAutoClsFlag = False;
		}
		IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);
		APP_CLR_SHUTDOWN_CNT();
	}
}
#if 1
void App_WarmKeyHandle(void){
	if(!App_Para.LockFlag){
		App_Para.UpDataFlag= True;
		
		if(App_Para.WarmSta == WARM_OFF){
			APP_SET_BACKLIGHT(TP_KEY1_LED);
			App_Para.WarmSta = WARM_ON;
            App_Para.WarmAutoClsFlag = True;
            App_Para.WarmAutoCnt = 0;
			LED4_PIN = ON;
		}else{
			APP_CLR_BACKLIGHT(TP_KEY1_LED);
			LED4_PIN = OFF;
			App_Para.WarmSta = WARM_OFF;
            App_Para.WarmAutoClsFlag = False;
		}
		IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);
		APP_CLR_SHUTDOWN_CNT();
	}
}
#endif



/*****************************************************************************
 * Function	  : App_Key0Handle
 * Description   : TK0
 * Input           : void  
 * Output		  : None
 * Return		  : 
 * Others		  : 
 * Record
 * 1.Date		  : 20180416
 *   Author       : marco
 *   Modification: Created function

*****************************************************************************/

void App_Key0Handle(void){
	App_LockKeyHandle();   
}

/*****************************************************************************
 * Function	  : App_Key1Handle
 * Description   : TK 1
 * Input           : void  
 * Output		  : None
 * Return		  : 
 * Others		  : 
 * Record
 * 1.Date		  : 20180416
 *   Author       : marco
 *   Modification: Created function

*****************************************************************************/
#if 0
void App_Key1Handle(void){
   
	if(!App_Para.LockFlag){		
		App_Para.UpDataFlag= True;
		if(App_Para.HC_Sta != COOL_MODE){
			App_Para.HC_Sta = COOL_MODE;
			App_Para.CoolAutoClsFlag = True;
			App_Para.CoolAutoCnt = 0;
		}else{
			App_Para.HC_Sta = STOP_MODE;
			App_Para.CoolAutoClsFlag = False;
		}
		App_HeatCoolHandle();			
		APP_CLR_SHUTDOWN_CNT();
	}
   
    if(!App_Para.LockFlag){		
		App_Para.UpDataFlag= True;
		App_Para.Pushrod_Status = PUSHROD_B_CLOSE;
		APP_CLR_SHUTDOWN_CNT();
	}
   
}
#endif 


/*****************************************************************************
 * Function	  : App_Key2Handle
 * Description   : TK2
 * Input           : void  
 * Output		  : None
 * Return		  : 
 * Others		  : 
 * Record
 * 1.Date		  : 20180416
 *   Author       : marco
 *   Modification: Created function

*****************************************************************************/
void App_Key2Handle(void){
	App_CLoseKeyHandle();
}

/*****************************************************************************
 * Function	  : App_Key3Handle
 * Description   : TK3
 * Input           : void  
 * Output		  : None
 * Return		  : 
 * Others		  : 
 * Record
 * 1.Date		  : 20180416
 *   Author       : marco
 *   Modification: Created function

*****************************************************************************/
void App_Key3Handle(void){
	App_OpenKeyHandle();
}

/*****************************************************************************
 * Function	  : App_Key4Handle
 * Description   : TK4
 * Input           : void  
 * Output		  : None
 * Return		  : 
 * Others		  : 
 * Record
 * 1.Date		  : 20180416
 *   Author       : marco
 *   Modification: Created function

*****************************************************************************/

void App_Key4Handle(void){
#if 0

	if(!App_Para.LockFlag){		
		App_Para.UpDataFlag= True;
		if(App_Para.HC_Sta != HEAT_MODE){
			App_Para.HC_Sta = HEAT_MODE;
			App_Para.HeatAutoClsFlag = True;
			App_Para.HeatAutoCnt = 0;
		}else{
			App_Para.HC_Sta = STOP_MODE;
			App_Para.HeatAutoClsFlag = False;
		}
		App_HeatCoolHandle();
	}

if(!App_Para.LockFlag){
		App_Para.UpDataFlag= True;
		App_Para.Pushrod_Status = PUSHROD_B_OPEN;
		APP_CLR_SHUTDOWN_CNT();
	}
#endif
#if 1
if(!App_Para.LockFlag){ 	
	App_Para.UpDataFlag= True;
	if(!HEAT_COOL_START)
	{
		HEAT_COOL_START = 1;
		switch(++HEAT_COOL_ARR[1])
		{
		case 1:
				if(App_Para.HC_Sta != COOL_MODE){
					App_Para.HC_Sta = COOL_MODE;
					App_Para.CoolAutoClsFlag = True;
					App_Para.CoolAutoCnt = 0;
				}
				break;
		case 2:
				if(App_Para.HC_Sta != HEAT_MODE){
					App_Para.HC_Sta = HEAT_MODE;
					App_Para.HeatAutoClsFlag = True;
					App_Para.HeatAutoCnt = 0;
				}								   
			   break;
		case 3:
				App_Para.HC_Sta = STOP_MODE;
				App_Para.HeatAutoClsFlag = False;
				App_Para.CoolAutoClsFlag = False;
				HEAT_COOL_ARR[1] = 0;
				break;
		default:
				App_Para.HC_Sta = STOP_MODE;
				App_Para.HeatAutoClsFlag = False;
				App_Para.CoolAutoClsFlag = False;
				HEAT_COOL_ARR[1] = 0;
				break;
		}
	}	
	App_HeatCoolHandle();
	APP_CLR_SHUTDOWN_CNT();
}
#endif				 

}




/*****************************************************************************
 * Function	  : App_Key5Handle
 * Description   : TK5
 * Input           : void  
 * Output		  : None
 * Return		  : 
 * Others		  : 
 * Record
 * 1.Date		  : 20180416
 *   Author       : marco
 *   Modification: Created function

*****************************************************************************/
void App_Key5Handle(void){
	App_MassKeyHandle();
}


static int System_Send_Message(unsigned int cont){    
	COMMU_BUF_t TX_Buf;    
	TX_Buf.Data_Format.Start = START_FIELD;    
	TX_Buf.Data_Format.MessageID = NUM1;    
	TX_Buf.Data_Format.Cont1 = Get_High(cont);    
	TX_Buf.Data_Format.Cont2 = Get_Low(cont);    
	TX_Buf.Data_Format.Checksum = (unsigned char)(TX_Buf.Data_Format.Start + TX_Buf.Data_Format.MessageID+          
		TX_Buf.Data_Format.Cont1 +  TX_Buf.Data_Format.Cont2);    
	TX_Buf.Data_Format.Stop1 = Get_High(STOP_MESSAGE);    
	TX_Buf.Data_Format.Stop2 = Get_Low(STOP_MESSAGE);    
	Bsp_UartSendBuf(BSP_UART1_ID, TX_Buf.Buf, BUF_SIZE);
	return 0; 
} 

/*****************************************************************************
 * Function	  : Key_Scan
 * Description   : 触摸按键扫描
 * Input           : void  
 * Output		  : None
 * Return		  : 
 * Others		  : 
 * Record
 * 1.Date		  : 20180403
 *   Author       : marco
 *   Modification: Created function

*****************************************************************************/
void Key_Scan(void){      				
	if(SOCAPI_TouchKeyStatus&0x80)	    		//重要步骤2:  触摸键扫描一轮标志，是否调用TouchKeyScan()一定要根据此标志位置起后
	 {	   																	
		SOCAPI_TouchKeyStatus &= 0x7f;	//重要步骤3: 清除标志位， 需要外部清除。													    
		App_Para.Chg_GetKey = TouchKeyScan();
		switch(App_Para.Chg_GetKey){	
			case CHG_KEY0_VALUE:{
                if(App_Para.Chg_CurKey != CHG_KEY0_VALUE){
					App_Para.Chg_CurKey = CHG_KEY0_VALUE;
                    key_static = 1;
                    key_press_flag = 1;
				}
				break;
			}
			
			case CHG_KEY1_VALUE:{
				if(App_Para.Chg_CurKey != CHG_KEY1_VALUE){					
					App_Para.Chg_CurKey = CHG_KEY1_VALUE;
					App_Key5Handle();
				}
				break;
			}
			
			case CHG_KEY2_VALUE:{
				if(App_Para.Chg_CurKey != CHG_KEY2_VALUE){
					App_Para.Chg_CurKey = CHG_KEY2_VALUE;
					App_Key2Handle(); //推杆B打开
					
				}
				break;
			}
			
			case CHG_KEY3_VALUE:{
				if(App_Para.Chg_CurKey != CHG_KEY3_VALUE){
					App_Para.Chg_CurKey = CHG_KEY3_VALUE;
					App_Key3Handle();//推杆A打开
				}
				break;
			}
			
			case CHG_KEY4_VALUE:{
				if(App_Para.Chg_CurKey != CHG_KEY4_VALUE){
					App_Para.Chg_CurKey = CHG_KEY4_VALUE;
					App_WarmKeyHandle();
				}
				break;
			}
			
			case CHG_KEY5_VALUE:{
				if(App_Para.Chg_CurKey != CHG_KEY5_VALUE){
					App_Para.Chg_CurKey = CHG_KEY5_VALUE;
				    App_Key4Handle();
			       }
                break;
             }
			default:{
				if(App_Para.Chg_CurKey){
					App_Para.Chg_CurKey = 0;
                    key_static = 2;
                    key_press_flag = 0;
                    key_press_cnt  = 0;
                    HEAT_COOL_START = 0;
					//LED1_PIN = OFF; 
					LED2_PIN = OFF;
                    LED3_PIN = OFF; 
					//LED4_PIN = OFF;
					//APP_CLR_BACKLIGHT(TP_KEY1_LED);
					APP_CLR_BACKLIGHT(TP_KEY2_LED);
                    APP_CLR_BACKLIGHT(TP_KEY3_LED);
					//APP_CLR_BACKLIGHT(TP_KEY4_LED);
					IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);
					App_Para.LockDetFlag[0] = False;
					if(App_Para.Pushrod_Status != PUSHROD_STOP){
						App_Para.UpDataFlag = True;
						App_Para.Pushrod_Status = PUSHROD_STOP;
					}
				}
				break;
			}
		}
		TouchKeyRestart();					//启动下一轮转换																														 			
	}		  	   
}

void Heat_AutoClose(void)
{
   if((App_Para.HeatAutoCnt  >= HEAT_AUTO_CLOSE) && App_Para.HC_Sta == HEAT_MODE){
		App_Para.HC_Sta = STOP_MODE;
		App_Para.UpDataFlag = True;
		LED5_PIN = OFF;
		RED_LED = OFF;
        key_static = 2;
        key_press_flag = 0;
        key_press_cnt  = 0;
        HEAT_COOL_START = 0;
		HEAT_COOL_ARR[1] = 0;
		if(App_Para.FootLedSta){
			Bsp_PwmON();
		}else{
			Bsp_PwmOff();
		}
		APP_CLR_BACKLIGHT(TP_KEY0_LED);

		IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);
	}
}

void Cool_AutoClose(void)
{
     if((App_Para.CoolAutoCnt >= COOL_AUTO_CLOSE) && App_Para.HC_Sta == COOL_MODE){
		App_Para.HC_Sta = STOP_MODE;
		App_Para.UpDataFlag = True;
        LED5_PIN = OFF;
		RED_LED = OFF;
        key_static = 2;
        key_press_flag = 0;
        key_press_cnt  = 0;
        HEAT_COOL_START = 0;
		HEAT_COOL_ARR[1] = 0;
		if(App_Para.FootLedSta){
			Bsp_PwmON();
		}else{
			Bsp_PwmOff();
		}
        APP_CLR_BACKLIGHT(TP_KEY0_LED);
    
		IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);
	}
}

void Warm_AutoClose(void)
{
     #if 1
    	if((App_Para.WarmAutoCnt >= WARM_AUTO_CLOSE) &&  App_Para.WarmSta == WARM_ON){
		App_Para.WarmSta = WARM_OFF;
		App_Para.UpDataFlag = True;
		LED4_PIN = OFF;
		RED_LED = OFF;
        APP_CLR_BACKLIGHT(TP_KEY1_LED);
		IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);
	}
    #endif
}

void Mass_AutoClose(void)
{
    #if 1
	if((App_Para.MassAutoCnt >= MASS_AUTO_CLOSE) && App_Para.MASSAGESta == MASSAGE_ON){
		App_Para.MASSAGESta = MASSAGE_OFF;
		App_Para.UpDataFlag = True;
		
		LED1_PIN = OFF;
		APP_CLR_BACKLIGHT(TP_KEY4_LED);
		IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);
	}
    #endif
}
#if 1
void System_AutoClose(void)
{
	if((App_Para.SystemAutoCnt >= SYSTEM_AUTO_CLOSE) && !App_Para.ShutDownFlag){
        key_static = 2;
        key_press_flag = 0;
        key_press_cnt  = 0;
        HEAT_COOL_START = 0;
        HEAT_COOL_ARR[1] = 0;
		App_Para.ShutDownFlag = True;
		App_Para.LockFlag = True;
		App_Para.UpDataFlag = True;
		App_Para.HC_Sta = STOP_MODE;
		App_Para.FootLedSta = FOOT_LED_OFF;
		App_Para.Pushrod_Status = PUSHROD_STOP;
		App_Para.Chg_CurKey = 0;
		LED0_PIN = ON;
		LED1_PIN = LED2_PIN = LED3_PIN = LED4_PIN = LED5_PIN = OFF;
		BACK_LIGHT = 0;	
		Bsp_PwmOff();
		RED_LED = OFF;
		App_Para.Tp_LedSta = 0;
		APP_SET_BACKLIGHT(TP_KEY5_LED);
		APP_CLR_BACKLIGHT(TP_KEY1_LED);
		APP_CLR_BACKLIGHT(TP_KEY2_LED);
		APP_CLR_BACKLIGHT(TP_KEY3_LED);
		APP_CLR_BACKLIGHT(TP_KEY4_LED);
		APP_CLR_BACKLIGHT(TP_KEY0_LED);
		App_Para.Tp_LedSta |= 0x40;
		App_Para.Tp_LedStaBak = App_Para.Tp_LedSta;	
		IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);
		}
}
#endif
/*****************************************************************************
 * Function	  : App_Init
 * Description   : 初始化系统
 * Input           : void  
 * Output		  : None
 * Return		  : 
 * Others		  : 
 * Record
 * 1.Date		  : 20180403
 *   Author       : marco
 *   Modification: Created function

*****************************************************************************/
void Delay_Time(unsigned int Time)
{
	static unsigned int i,j;
	for(i = Time; i > 0; i --)
	{
		for(j = 200; j > 0 ; j --);
	}
}
void App_Init(void){
	int i;	
	App_LedInit();
	Bsp_PWMInit();
	Bsp_PwmOff();
	Bsp_UartInit(BSP_UART1_ID);
	Bsp_Timer0Init();
	TimeOutDet_Init();	
	for(i = 0;i < IIC_NUM;i ++){
		I2c_Para[i].ID = i;
		if(i == 0){
			I2c_Para[i].read_addr = I2C_READ_ADDR;
			I2c_Para[i].write_addr = I2C_WRITE_ADDR;
		}
		IIC_Init(&I2c_Para[i]);
	}
	Delay_Time(5000);
	P1PH |= 0X80;		// i2c int 
	APP_CLR_BACKLIGHT(TP_BACKLIGHT_LED);
	IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);	
	TouchKeyInit();	
	App_Para.UpDataFlag = True;
	App_Para.SystemAutoClsFlag = True;
	EA = 1;		
}

/*****************************************************************************
 * Function	  : App_Handle
 * Description   : 系统事件处理
 * Input           : void  
 * Output		  : None
 * Return		  : 
 * Others		  : 
 * Record
 * 1.Date		  : 20180403
 *   Author       : marco
 *   Modification: Created function

*****************************************************************************/
void App_Handle(void){
	#ifdef SYS_ENABLE_WDT
		CLEAR_WDT();
	#endif
	/**
		@brief: Processing CHG button
	*/
	Key_Scan();	  

	/**
		@brief:Processing TP button
	*/
	if(!Get_I2C_INT()){
		IIC_Read_Data(&I2c_Para[0], I2c_Para[0].read_addr, &App_Para.TP_GetKey);
	
		switch(App_Para.TP_GetKey){
			case TP_KEY0_VALUE:{
				if(App_Para.Tp_CurKey != TP_KEY3_VALUE){
					App_Para.Tp_CurKey = TP_KEY3_VALUE;
                    App_Key3Handle(); //推杆B打开
				}
				break;
			}

			case TP_KEY1_VALUE:{
				if(App_Para.Tp_CurKey != TP_KEY1_VALUE){
					App_Para.Tp_CurKey = TP_KEY1_VALUE;
					App_WarmKeyHandle();
				}
				break;
			}

			case TP_KEY2_VALUE:{
				if(App_Para.Tp_CurKey != TP_KEY2_VALUE){
					App_Para.Tp_CurKey = TP_KEY2_VALUE;
                    App_Key4Handle();              
				}
				break;
			}

			case TP_KEY3_VALUE:{
				if(App_Para.Tp_CurKey != TP_KEY0_VALUE){
					App_Para.Tp_CurKey = TP_KEY0_VALUE;
					key_static = 1;
                    key_press_flag = 1;
				}
				break;
			}
			
			case TP_KEY4_VALUE:{
				if(App_Para.Tp_CurKey != TP_KEY4_VALUE){
					App_Para.Tp_CurKey = TP_KEY4_VALUE;
					App_Key5Handle();
				}
				break;
			}

			case TP_KEY5_VALUE:{
				if(App_Para.Tp_CurKey != TP_KEY5_VALUE){
					App_Para.Tp_CurKey = TP_KEY5_VALUE;
					App_Key2Handle(); //推杆B打开
				}
				break;
			}
			default:{
				if(App_Para.Tp_CurKey){
					App_Para.Tp_CurKey = 0;
                    key_static = 2;
                    key_press_flag = 0;
                    key_press_cnt  = 0;
                    HEAT_COOL_START = 0;
                    //LED1_PIN = OFF; 
                    LED2_PIN = OFF;
                    LED3_PIN = OFF; 
                    //LED4_PIN = OFF;
                    //APP_CLR_BACKLIGHT(TP_KEY1_LED);
                    APP_CLR_BACKLIGHT(TP_KEY2_LED);
                    APP_CLR_BACKLIGHT(TP_KEY3_LED);
                    //APP_CLR_BACKLIGHT(TP_KEY4_LED);
					IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);
					App_Para.LockDetFlag[1] = False;					
					if(App_Para.Pushrod_Status != PUSHROD_STOP){
						App_Para.UpDataFlag = True;
						App_Para.Pushrod_Status = PUSHROD_STOP;
					}
				}
				break;
			}
		}
	}

	/**
		@brief:TP Handle button length press.
	*/
    switch (status)
        {
            case START:
                      if(key_static == 1)
                      {
                         status = SHORT;
                      }
                      break;
            case SHORT:
                       if(key_static == 2)
                       {
                            App_LightKeyHandle();
                            status = START;
                            key_static = 0;
                            key_press_cnt = 0;
                       }
                       if(key_press_cnt > 1500)
                       {
                            status = LONG;
                            key_press_cnt = 0;
                    		if(!App_Para.LockFlag){
                    			App_Para.LockFlag = True;
                    		}else{
                    			App_Para.LockFlag = False;
                    		}					
                    		App_Key0Handle();
                       }
                       
                       break;
            case LONG:
                       if(key_static == 2)
                       {
                          status = START; 
                          key_static = 0;
                       }
                       break;
            default:
                    break;        
        }

	/**
		@brief: refresh TP led display
	*/
	if(!App_Para.TpLedDisCntFlag){
		IIC_Write_Data(&I2c_Para[0], I2c_Para[0].write_addr, App_Para.Tp_LedSta);
		App_Para.TpLedDisCntFlag = False;
		App_Para.TpLedDisCnt = 0;
	}
	
	/**
		@brief:Send control command
	*/
	if(App_Para.UpDataFlag){
		unsigned int SentData = 0;
		App_Para.UpDataFlag = False;
		/**
			@brief:LED moulde
		*/
		#if 1
		if(App_Para.FootLedSta == FOOT_LED_ON){
			APP_CLR_SNEDDATA(CONT_LED_OFF);
			APP_SET_SENDDATA(CONT_LED_ON);
		}else if(App_Para.FootLedSta == FOOT_LED_OFF){
			APP_CLR_SNEDDATA(CONT_LED_ON);
			APP_SET_SENDDATA(CONT_LED_OFF);
		}
        #endif

		/**
			@brief:pushrod moudle
		*/
		
		if(App_Para.Pushrod_Status == PUSHROD_STOP ||App_Para.Pushrod_Status == PUSHROD_A_STOP){
			APP_CLR_SNEDDATA(CONT_PUSHROD_A_OPEN);
			APP_CLR_SNEDDATA(CONT_PUSHROD_A_CLOSE);
			APP_SET_SENDDATA(CONT_PUSHROD_STOP);
		}else if(App_Para.Pushrod_Status == PUSHROD_A_OPEN){
		    APP_CLR_SNEDDATA(CONT_PUSHROD_A_STOP);
			APP_CLR_SNEDDATA(CONT_PUSHROD_A_CLOSE);
			APP_SET_SENDDATA(CONT_PUSHROD_A_OPEN);
		}else if(App_Para.Pushrod_Status == PUSHROD_A_CLOSE){
			APP_CLR_SNEDDATA(CONT_PUSHROD_A_OPEN);
			APP_CLR_SNEDDATA(CONT_PUSHROD_A_OPEN);
			APP_SET_SENDDATA(CONT_PUSHROD_A_CLOSE);
		}
		#if 0
		if(App_Para.Pushrod_Status == PUSHROD_STOP ||App_Para.Pushrod_Status == PUSHROD_B_STOP){
			APP_CLR_SNEDDATA(CONT_PUSHROD_B_OPEN);
			APP_CLR_SNEDDATA(CONT_PUSHROD_B_CLOSE);
			APP_SET_SENDDATA(CONT_PUSHROD_STOP);
		}else if(App_Para.Pushrod_Status == PUSHROD_B_OPEN){
			APP_CLR_SNEDDATA(CONT_PUSHROD_B_STOP);
			APP_CLR_SNEDDATA(CONT_PUSHROD_A_CLOSE);
			APP_SET_SENDDATA(CONT_PUSHROD_B_OPEN);
		}else if(App_Para.Pushrod_Status == PUSHROD_B_CLOSE){
			APP_CLR_SNEDDATA(CONT_PUSHROD_B_OPEN);
			APP_CLR_SNEDDATA(CONT_PUSHROD_B_OPEN);
			APP_SET_SENDDATA(CONT_PUSHROD_B_CLOSE);
		}
        #endif
		/**
			@brief:heat & cool moudle
		*/
		if(App_Para.HC_Sta == CH_A_COOL){		
			APP_CLR_SNEDDATA(CONT_CH_A_HEAT);
			APP_CLR_SNEDDATA(CONT_CH_A_OFF);
			APP_SET_SENDDATA(CONT_CH_A_COOL);
		}else if(App_Para.HC_Sta == CH_A_HEAT){
			APP_CLR_SNEDDATA(CONT_CH_A_OFF);
			APP_CLR_SNEDDATA(CONT_CH_A_COOL);
			APP_SET_SENDDATA(CONT_CH_A_HEAT);
		}else{
			APP_CLR_SNEDDATA(CONT_CH_A_COOL);
			APP_CLR_SNEDDATA(CONT_CH_A_HEAT);
			APP_SET_SENDDATA(CONT_CH_A_OFF);
          }

      
        /**
			@brief:mass moudle
		*/
        if(App_Para.MASSAGESta == MASSAGE_ON)
        {
            APP_CLR_SNEDDATA(CONT_MASS_OFF);
            APP_SET_SENDDATA(CONT_MASS_ON);
        }
        else if(App_Para.MASSAGESta == MASSAGE_OFF)
        {
            APP_CLR_SNEDDATA(CONT_MASS_ON);
            APP_SET_SENDDATA(CONT_MASS_OFF);
        }
		 #if 1
        /**
			@brief: warm moudle
		*/
        if(App_Para.WarmSta== WARM_ON)
        {
            APP_CLR_SNEDDATA(CONT_WARM_CLOTH_OFF);
            APP_SET_SENDDATA(CONT_WARM_CLOTH_ON);
        }
        else if(App_Para.WarmSta == WARM_OFF)
        {
            APP_CLR_SNEDDATA(CONT_WARM_CLOTH_ON);
            APP_SET_SENDDATA(CONT_WARM_CLOTH_OFF);
        }
        #endif
        
 		App_Para.SentData = SentData;
		System_Send_Message(SentData);
	}

	/**
		@brief: heartbeat package
	*/
	if(App_Para.OnLineFlag){
		App_Para.OnLineFlag = False;
		App_Para.OnLineCnt = 0;
		System_Send_Message(App_Para.SentData);
	}

	/**
		@breif: auto close
	*/  
	Cool_AutoClose();
    Mass_AutoClose();
    Warm_AutoClose();
    Heat_AutoClose();
    System_AutoClose();
}

/*****************************************************************************
 * Function	  : I2c_Irq
 * Description   : 系统硬件I2C中断处理函数
 * Input		  : None
 * Output		  : None
 * Return		  : 
 * Others		  : 
 * Record
 * 1.Date		  : 20180404
 *   Author       : marco
 *   Modification: Created function

*****************************************************************************/
#if SYS_ENABLE_I2C
void I2c_Irq() interrupt 7{
	/*<主机发送数据>*/
	if( (SSCON0&0x07) == 0x02){
		unsigned char get_data = SSDAT;
		if(get_data != I2C_ADDR){
			App_LedHandle(get_data);
		}
	}
	/*<主机接收数据>*/
	else if( (SSCON0&0x07) == 0x03 ){
		if(I2C_IntGet() == 0){
			I2C_IntSet(1);
		}
	}
	/*<必须手动清掉此位>*/	
	SSCON0 &= ~0X40;
}
#endif

/*****************************************************************************
 * Function	  : Bsp_Uart1IqrHandle
 * Description   : UART1中断处理函数
 * Input		  : None
 * Output		  : None
 * Return		  : 
 * Others		  : 
 * Record
 * 1.Date		  : 20180409
 *   Author       : marco
 *   Modification: Created function

*****************************************************************************/
#if SYS_ENABLE_UART1
void Bsp_Uart1IqrHandle() interrupt 7{
	unsigned char crc = 0;
	/*<发送数据>*/
	if(SSCON0&0x02){
		SSCON0&=0xFD;
		if(Tx_Buf.send_cnt == Tx_Buf.len - 1){
			Tx_Buf.flag = False;
			P20 = 1;
			return ;
		}
		SSDAT = Tx_Buf.buf[++Tx_Buf.send_cnt];	
	}
	/*<接收数据>*/
	#if 0
	if((SSCON0&0x01)){
		SSCON0&=0xFE;
		App_Para.Buf.rev_buf.buf[App_Para.Buf.rev_buf.len++] = SSDAT;
		
		if(App_Para.Buf.rev_buf.len>= APP_REV_SIZE ){
			/*<不是发送到本机的数据，不理会>*/
			if(App_Para.Buf.rev_buf.buf[1] != App_Para.addr){				
				memset((void*)&App_Para.Buf,0,sizeof(Buf_t));
				return ;
			}
			/*<结尾符错误>*/
			if(App_Para.Buf.rev_buf.buf[APP_REV_SIZE-2] != 0x0d || App_Para.Buf.rev_buf.buf[APP_REV_SIZE-1] != 0x0a){
				memset((void*)&App_Para.Buf,0,sizeof(Buf_t));
				return ;
			}
			/*<校验错误>*/
			crc = (unsigned char)(App_Para.Buf.rev_buf.buf[0] + App_Para.Buf.rev_buf.buf[1] + App_Para.Buf.rev_buf.buf[2] + App_Para.Buf.rev_buf.buf[3]);
			if(crc != App_Para.Buf.rev_buf.buf[4]){
				memset((void*)&App_Para.Buf,0,sizeof(Buf_t));
				return ;
			}
			
			switch(App_Para.Buf.rev_buf.buf[0]){
				case HOST_QUERY:{
					memset((void*)&App_Para.Buf.send_buf,0,sizeof(Buf_t));
					App_Para.Buf.send_buf.buf[0] = SLAVE_SEND_KEY_VALUE;
					App_Para.Buf.send_buf.buf[1] = App_Para.addr;
					App_Para.Buf.send_buf.buf[2] = App_Para.key_value/256;
					App_Para.Buf.send_buf.buf[3] = App_Para.key_value%256;
					App_Para.Buf.send_buf.buf[4] = (unsigned char)(App_Para.Buf.send_buf.buf[0]+ App_Para.Buf.send_buf.buf[1] + App_Para.Buf.send_buf.buf[2] + App_Para.Buf.send_buf.buf[3]);
					App_Para.Buf.send_buf.buf[5] = 0x0d;
					App_Para.Buf.send_buf.buf[6] = 0x0a;
					Bsp_UartSendBuf(BSP_UART1_ID, &App_Para.Buf.send_buf.buf[0], APP_REV_SIZE);
					break;
				}

				case HOST_LIGHT_ON:{
					// add light handle
					break;
				}

				default:
					break;
			}
		}
	}
	#endif
}
#endif

void Bsp_Timer0IqrHandle() interrupt 1{	
	static unsigned int base_time = 0;
	TL0 = (65536 - 1000)%256;   
	TH0 = (65536 - 1000)/256;
	TimeOutDet_DecHandle();
    if(key_press_flag)
    {
       key_press_cnt++;
    }
	if(base_time ++ > 1000){
		base_time = 0;
		if(App_Para.HeatAutoClsFlag){
			if(App_Para.HeatAutoCnt ++ > HEAT_AUTO_CLOSE){
				App_Para.HeatAutoClsFlag = False;
                App_Para.HC_Sta = STOP_MODE;
			}
		}	

		if(App_Para.CoolAutoClsFlag){
			if(App_Para.CoolAutoCnt ++ > COOL_AUTO_CLOSE){
				App_Para.CoolAutoClsFlag = False;
			}
		}	
        #if 1
		if(App_Para.SystemAutoClsFlag){
			if(App_Para.SystemAutoCnt ++ > SYSTEM_AUTO_CLOSE){
				App_Para.SystemAutoClsFlag = False;
			}
		}
        #endif
        if(App_Para.MassAutoClsFlag)
        {
           if(App_Para.MassAutoCnt ++ > MASS_AUTO_CLOSE){
                App_Para.MassAutoClsFlag = False;    
            }
        }
        #if 1
        if(App_Para.WarmAutoClsFlag)
        {
            if(App_Para.WarmAutoCnt++ > WARM_AUTO_CLOSE)
            {
                App_Para.WarmAutoClsFlag = False;                
            }
        }
        #endif
	}
	#if 1
	if(!App_Para.OnLineFlag){
		if(App_Para.OnLineCnt ++ >= ONLINE_TIME){
			App_Para.OnLineFlag = True;
		}
	}	

	if(!App_Para.TpLedDisCntFlag){
		if(App_Para.TpLedDisCnt ++ > LED_DIS_TIME){
			App_Para.TpLedDisCntFlag = True;
		}
	}	
	#endif
}


