//-----------------------------------------------------------------
// ��������:
//     WinnerI�����峣�ù��ܶ���
// ��    ��: ���ǵ���
// ��ʼ����: 2018-08-04
// �������: 2018-08-04
// �޸�����: 
// ��ǰ�汾: V1.0
// ��ʷ�汾:
//  - V1.0: (2018-08-04)�ṩһЩ�̼����ܣ����ڹ���STM32.
// ���Թ���: ����STM32F429+CycloneIV����ϵͳ��ƿ����塢LZE_ST_LINK2
// ˵    ��: 
//    
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// ͷ�ļ�����
//-----------------------------------------------------------------
#include "stm32f429_winner.h"
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// void SystemClock_Config(u32 plln,u32 pllm,u32 pllp,u32 pllq)
//-----------------------------------------------------------------
// 
// ��������: ϵͳʱ�ӳ�ʼ��
// ��ڲ���: u32 plln: ��PLL��Ƶϵ��(PLL��Ƶ),ȡֵ��Χ:64~432.
//					 u32 pllm: ��PLL����ƵPLL��Ƶϵ��(PLL֮ǰ�ķ�Ƶ),ȡֵ��Χ:2~63.
//					 u32 pllp: ϵͳʱ�ӵ���PLL��Ƶϵ��(PLL֮��ķ�Ƶ),ȡֵ��Χ:2,4,6,8.(������4��ֵ!)
//					 u32 pllq: USB/SDIO/������������ȵ���PLL��Ƶϵ��(PLL֮��ķ�Ƶ),ȡֵ��Χ:2~15.
// �� �� ֵ: 0,�ɹ�;1,ʧ��
// ע������: �ⲿ����Ϊ25M��ʱ��,�Ƽ�ֵ:plln=360,pllm=25,pllp=2,pllq=8.
//					 Fvco:VCOƵ��												Fvco=Fs*(plln/pllm)													Fvco=25*(360/25)=360Mhz
//					 SYSCLK:ϵͳʱ��Ƶ��								SYSCLK=Fvco/pllp=Fs*(plln/(pllm*pllp))			SYSCLK=360/2=180Mhz
//					 Fusb:USB,SDIO,RNG�ȵ�ʱ��Ƶ��			Fusb=Fvco/pllq=Fs*(plln/(pllm*pllq))				Fusb=360/8=45Mhz
//-----------------------------------------------------------------
void SystemClock_Config_HSE(u32 plln,u32 pllm,u32 pllp,u32 pllq)
{
    HAL_StatusTypeDef ret = HAL_OK;
    RCC_OscInitTypeDef RCC_OscInitStructure; 
    RCC_ClkInitTypeDef RCC_ClkInitStructure;
    
    __HAL_RCC_PWR_CLK_ENABLE(); //ʹ��PWRʱ��
    
    // ������������������õ�ѹ�������ѹ�����Ա�������δ�����Ƶ�ʹ���
    // ʱʹ�����빦��ʵ��ƽ�⣬�˹���ֻ��STM32F42xx��STM32F43xx�����У�
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);//���õ�ѹ�������ѹ����1
    
    RCC_OscInitStructure.OscillatorType=RCC_OSCILLATORTYPE_HSE; // ʱ��ԴΪHSE
    RCC_OscInitStructure.HSEState=RCC_HSE_ON;                   // ��HSE
    RCC_OscInitStructure.PLL.PLLState=RCC_PLL_ON;								// ��PLL
    RCC_OscInitStructure.PLL.PLLSource=RCC_PLLSOURCE_HSE;				// PLLʱ��Դѡ��HSE
    RCC_OscInitStructure.PLL.PLLM=pllm; 												// ��PLL����ƵPLL��Ƶϵ��(PLL֮ǰ�ķ�Ƶ),ȡֵ��Χ:2~63.
    RCC_OscInitStructure.PLL.PLLN=plln; 												// ��PLL��Ƶϵ��(PLL��Ƶ),ȡֵ��Χ:64~432.  
    RCC_OscInitStructure.PLL.PLLP=pllp; 												// ϵͳʱ�ӵ���PLL��Ƶϵ��(PLL֮��ķ�Ƶ),ȡֵ��Χ:2,4,6,8.(������4��ֵ!)
    RCC_OscInitStructure.PLL.PLLQ=pllq; 												// USB/SDIO/������������ȵ���PLL��Ƶϵ��(PLL֮��ķ�Ƶ),ȡֵ��Χ:2~15.
    ret=HAL_RCC_OscConfig(&RCC_OscInitStructure);								// ��ʼ��
	
    if(ret!=HAL_OK) while(1);
    
    ret=HAL_PWREx_EnableOverDrive(); // ����Over-Driver����
    if(ret!=HAL_OK) while(1);
    
    // ѡ��PLLCLK��Ϊϵͳʱ��Դ��������HCLK,PCLK1��PCLK2
    RCC_ClkInitStructure.ClockType=(RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStructure.SYSCLKSource=RCC_SYSCLKSOURCE_PLLCLK;		 // ����ϵͳʱ��ʱ��ԴΪPLL
    RCC_ClkInitStructure.AHBCLKDivider=RCC_SYSCLK_DIV1;						 // AHB ��Ƶϵ��Ϊ1��AHB �������ʱ��Ƶ��Ϊ180MHz
    RCC_ClkInitStructure.APB1CLKDivider=RCC_HCLK_DIV4; 						 // APB1��Ƶϵ��Ϊ4��APB1�������ʱ��Ƶ��Ϊ45MHz
    RCC_ClkInitStructure.APB2CLKDivider=RCC_HCLK_DIV2; 						 // APB2��Ƶϵ��Ϊ2��APB2�������ʱ��Ƶ��Ϊ90MHz
    ret=HAL_RCC_ClockConfig(&RCC_ClkInitStructure,FLASH_LATENCY_5);// ͬʱ����FLASH��ʱ����Ϊ5WS��Ҳ����6��CPU���ڡ�
		
    if(ret!=HAL_OK) 
			while(1);
}

//-----------------------------------------------------------------
// End Of File
//----------------------------------------------------------------- 
