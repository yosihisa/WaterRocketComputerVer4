/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"
#include "fatfs.h"

/* USER CODE BEGIN Includes */
#include "I2C.h"
#define UART_DEBUG	0	//取得したデータををUARTで送信するかどうか 0:送信しない 1:送信する　（初期化中は常に送信） 0の時はSDカードに記録はしない
#define WIRELESS 1		//無線で制御するかどうか	0:制御しない 1:制御する
#define WRC_VERSION 1.0 //バージョン
#define P0 1013.25		//海面気圧

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

SD_HandleTypeDef hsd;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM1_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
uint32_t g_ADCValue;
int g_MeasurementNumber;
#define  ADC_BUFFER_LENGTH 20
uint16_t g_ADCBuffer[ADC_BUFFER_LENGTH];

uint8_t uart_rx_data;
uint8_t control_state = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart6)
{
	control_state = 1;
	if (uart_rx_data == 'R')control_state = 100;
	if (uart_rx_data == 'S')control_state = 101;
	if (uart_rx_data == 'H')control_state = 102;
	
}

/* USER CODE END 0 */

int main(void)
{

  /* USER CODE BEGIN 1 */
	char    str[1024];
	int tx_buf[I2C_BUF_SIZE] = { 0x00 }, rx_buf[I2C_BUF_SIZE] = { 0x00 };
	uint8_t pData;

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	  /* USER CODE BEGIN Init */

	    /* USER CODE END Init */

	      /* Configure the system clock */
	SystemClock_Config();

	  /* USER CODE BEGIN SysInit */

	    /* USER CODE END SysInit */

	      /* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_SDIO_SD_Init();
	MX_ADC1_Init();
	MX_USART6_UART_Init();
	MX_I2C1_Init();
	MX_USART1_UART_Init();
	MX_FATFS_Init();
	MX_TIM1_Init();

	  /* USER CODE BEGIN 2 */
	HAL_Delay(500);
	sprintf(str, "%WRC \r\n");
	HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
	sprintf(str, "\033[49m\033[36m\033[2J\033[0;0H");
	HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF);
	HAL_Delay(500);
	sprintf(str, "Water rocket computer Ver%3.2f\n\033[39m", WRC_VERSION);
	HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF); HAL_Delay(120);	
	sprintf(str, "\033[39mSystem Initializing.");
	HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF); HAL_Delay(120);	
	//ADC設定
	uint32_t adcValue = 0;
	uint32_t volt = 0;
	ADC_ChannelConfTypeDef sConfig;
	memset(g_ADCBuffer, 0, sizeof(g_ADCBuffer));
	HAL_ADC_Start_DMA(&hadc1, g_ADCBuffer, ADC_BUFFER_LENGTH);
	HAL_ADC_Stop_DMA(&hadc1);
	sprintf(str, "."); HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF);
	
	//AK8963起動
	pData = 0x00; HAL_I2C_Mem_Write(&hi2c1, MPU9250_WRITE, MPU9250_PWR_MGMT_1, 1, &pData, 1, 100);
	pData = 0x02; HAL_I2C_Mem_Write(&hi2c1, MPU9250_WRITE, MPU9250_INT_PIN_CFG, 1, &pData, 1, 100);
	
	//MPU9250設定
	pData = 0b00001000; HAL_I2C_Mem_Write(&hi2c1, MPU9250_WRITE, MPU9250_GYRO_CONFIG, 1, &pData, 1, 100);//±500dps		43   00:±250 01:±500 10:±1000 11:±2000
	pData = 0b00011000; HAL_I2C_Mem_Write(&hi2c1, MPU9250_WRITE, MPU9250_ACCEL_CONFIG, 1, &pData, 1, 100);//±16G
	
	//AK8963設定
	pData = 0b00010110; HAL_I2C_Mem_Write(&hi2c1, AK8963_WRITE, AK8963_CNTL1, 1, &pData, 1, 100);//連続測定モード 100Hz
	
	//LPS25H設定
	pData = 0b11000000; HAL_I2C_Mem_Write(&hi2c1, LPS25H_WRITE, LPS25H_CTRL_REG1, 1, &pData, 1, 100);//25Hz
	
	//ADXL375設定
	pData = 0x08; HAL_I2C_Mem_Write(&hi2c1, ADXL375_WRITE, ADXL375_POWER_CTL, 1, &pData, 1, 100);//スリープ解除
	pData = 0x0B; HAL_I2C_Mem_Write(&hi2c1, ADXL375_WRITE, ADXL375_DATA_FORMAT, 1, &pData, 1, 100);//データ右詰め
	pData = 0b00001010; HAL_I2C_Mem_Write(&hi2c1, ADXL375_WRITE, ADXL375_BW_RATE, 1, &pData, 1, 100);//100Hz
	sprintf(str, "."); HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF); HAL_Delay(120);	
	
	sprintf(str, "Done\n");
	HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF); HAL_Delay(120);	
	
	
	
	//Who am I
	HAL_I2C_Mem_Read(&hi2c1, LPS25H_READ, LPS25H_WHO_AM_I, 1, &pData, 1, 100);
	sprintf(str, "\r\nLPS25H  = 0x%02X \r\n", pData);
	HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
	
	HAL_I2C_Mem_Read(&hi2c1, MPU9250_READ, MPU9250_WHO_AM_I, 1, &pData, 1, 100);
	sprintf(str, "MPU9250 = 0x%02X \r\n", pData);
	HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
	
	HAL_I2C_Mem_Read(&hi2c1, AK8963_READ, AK8963_WIA, 1, &pData, 1, 100);
	sprintf(str, "AK8963  = 0x%02X \r\n", pData);
	HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
	
	HAL_I2C_Mem_Read(&hi2c1, ADXL375_READ, ADXL375_DEVID, 1, &pData, 1, 100);
	sprintf(str, "ADXL375 = 0x%02X \r\n", pData);
	HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
	
	sprintf(str, "System self test Start.."); 
	HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
	HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF); HAL_Delay(120);	
	
	//セルフテスト
	while (1) {
		HAL_I2C_Mem_Read(&hi2c1, LPS25H_READ, LPS25H_WHO_AM_I, 1, &pData, 1, 100);
		if (pData != 0xBD)continue;
		HAL_I2C_Mem_Read(&hi2c1, MPU9250_READ, MPU9250_WHO_AM_I, 1, &pData, 1, 100);
		if (pData != 0x71)continue;
		HAL_I2C_Mem_Read(&hi2c1, AK8963_READ, AK8963_WIA, 1, &pData, 1, 100);
		if (pData != 0x48)continue;
		HAL_I2C_Mem_Read(&hi2c1, ADXL375_READ, ADXL375_DEVID, 1, &pData, 1, 100);
		if (pData != 0xE5)continue;
		//if (g_ADCBuffer[0] == 0)continue;
		//if (g_ADCBuffer[1] == 0)continue;
		HAL_Delay(250);
		sprintf(str, "."); 
		HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
		HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF);
		HAL_Delay(250);	
		break;
	}
	sprintf(str, "Successful\n"); 
	HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
	HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF);
	
	sprintf(str, "SD card Initializing...");
	HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF); HAL_Delay(120);
	
	HAL_TIM_Base_Start(&htim1);
	HAL_Delay(1000);
	
#if ( UART_DEBUG ==0 )

	//SDカード初期化
	FRESULT res;
	FATFS fs;
	DIR dir;
	FIL fil;
	char buf[128];
	int ret;
	const int MAX_FILE = 100;
	unsigned int c = 0;
	//SDカードを初期化できるまで待つ
	do {
		res = f_mount(&fs, "", 1);
		if (res != FR_OK) {
			sprintf(str, "f_mount() : error %u\n", res);
			HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
			sprintf(str, "\033[5;0H\033[41m\033[2Kf_mount() : error %u", res); 
			HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF); HAL_Delay(120);	
		}
		
		res = f_opendir(&dir, "");
		if (res != FR_OK) {
			sprintf(str, "f_opendir() : error %u\n", res);
			HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
			sprintf(str, "\033[6;0H\033[41m\033[2Kf_opendir() : error %u", res); 
			HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF); HAL_Delay(120);	
		}
		HAL_Delay(800);
	} while (res != FR_OK);
	sprintf(str, "\033[5;0H\033[49m\033[2K"); 
	HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF); HAL_Delay(120);
	sprintf(str, "\033[6;0H\033[49m\033[2K"); 
	HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF); HAL_Delay(120);	
	sprintf(str, "\033[4;26H\033[49mSuccessful\n"); 
	HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF);
	
	
	//新しいファイルを作成
	for (c = 0; c < MAX_FILE; c++) {
		sprintf(str, "%d.csv", c);
		res = f_open(&fil, str, FA_CREATE_NEW | FA_WRITE);
		if (res == FR_OK) {
			sprintf(str, "f_open() : %d.csv\n", c);
			HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
			sprintf(str, "\033[5;0H\033[49m\033[39m\033[2KFile Name : %d.csv", c); 
			HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF); HAL_Delay(120);
			break;
		}
		if (c == MAX_FILE - 1) {
			res = f_open(&fil, "_.csv", FA_CREATE_ALWAYS | FA_WRITE);
			if (res != FR_OK) {
				sprintf(str, "f_open() : error %u\n\n", res);
				HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
				sprintf(str, "\033[5;0H\033[41m\033[2Kf_open() : error %u", res); 
				HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF); HAL_Delay(120);	
			} else {
				sprintf(str, "f_open() : _.csv \n");
				HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
				sprintf(str, "\033[5;0H\033[49m\033[39m\033[2KFile Name : _.csv"); 
				HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF); HAL_Delay(120);
			}
		}
	}
	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);	//緑点灯　SD初期化完了
	
	sprintf(str, "dt[ns],press[hPa],temp[deg celsius],ax[m/s^2],ay[m/s^2],az[m/s^2],gx[deg/s],gy[deg/s],gz[deg/s],mx,my,mz,Ax[m/s^2],Ay[m/s^2],Az[m/s^2],servo,V,I\n");
	ret = f_puts(str, &fil);
	if (ret == EOF) {
		sprintf(str, "f_puts() : error\n\n");
		HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
		
	}
	f_sync(&fil);
	
#if (WIRELESS ==0 )
	HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
	HAL_Delay(1000);
	HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
	HAL_Delay(5000);
	HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
#endif
	sprintf(str, "\033[6;0H\033[49m\033[39m\033[2K R:Start\n S:Stop \n H:Set the current altitude to 0 m"); 
	HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF); HAL_Delay(120);
#if (WIRELESS ==1 )
	HAL_UART_Receive_IT(&huart6, (uint8_t *)&uart_rx_data, 1);
	while (1) {
	
		if (control_state != 0)HAL_UART_Receive_IT(&huart6, (uint8_t *)&uart_rx_data, 1);
		if (control_state == 100)break;
	}
#endif
#endif

	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	int t = 0, n = 0;
	TIM1->CNT = 0;
	double height, height_offset =0;
	while (1) {
	/* USER CODE END WHILE */

	  /* USER CODE BEGIN 3 */
		uint8_t data[18], servo;
		uint32_t	press;
		int16_t	temp, ax, ay, az, gx, gy, gz, mx, my, mz, Ax, Ay, Az;
		double	press_d, temp_d, ax_d, ay_d, az_d, gx_d, gy_d, gz_d, mx_d, my_d, mz_d, Ax_d, Ay_d, Az_d;
	  
	  
		//気圧
		press = 0;
		HAL_I2C_Mem_Read(&hi2c1, LPS25H_READ, LPS25H_PRESS_OUT_H, 1, &data, 1, 100);
		press   = data[0] << 16;
		HAL_I2C_Mem_Read(&hi2c1, LPS25H_READ, LPS25H_PRESS_OUT_L, 1, &data, 1, 100);
		press   |= data[0] << 8;
		HAL_I2C_Mem_Read(&hi2c1, LPS25H_READ, LPS25H_PRESS_OUT_XL, 1, &data, 1, 100);
		press   |= data[0];
		press_d = (double)press / 4096.0;
	  
		//気温
		HAL_I2C_Mem_Read(&hi2c1, LPS25H_READ, LPS25H_TEMP_OUT_H, 1, &data, 1, 100);
		temp   = data[0] << 8;
		HAL_I2C_Mem_Read(&hi2c1, LPS25H_READ, LPS25H_TEMP_OUT_L, 1, &data, 1, 100);
		temp   |= data[0];
		temp_d = (double)temp / 480 + 42.5;
		  
	  
		//9軸センサ
		HAL_I2C_Mem_Read(&hi2c1, MPU9250_READ, MPU9250_ACCEL_XOUT_H, 1, &data, 14, 100);
		ax = (data[0]  << 8) | data[1];
		ay = (data[2]  << 8) | data[3];
		az = (data[4]  << 8) | data[5];
		gx = (data[8]  << 8) | data[9];
		gy = (data[10] << 8) | data[11];
		gz = (data[12] << 8) | data[13];
	  
		HAL_I2C_Mem_Read(&hi2c1, AK8963_READ, AK8963_HXL, 1, &data, 7, 100);
		my = data[0] | (data[1]  << 8);
		mx = data[2] | (data[3]  << 8);
		mz = data[4] | (data[5]  << 8);
		mx *= -1;
		my *= -1;
		mz *= -1;
	  
		ax_d = (double)ax *GRAVITY * 0.488 / 1000* (-1.0);
		ay_d = (double)ay *GRAVITY * 0.488 / 1000* (-1.0);
		az_d = (double)az *GRAVITY * 0.488 / 1000;
		gx_d = (double)gx * 0.01526* (-1.0);
		gy_d = (double)gy * 0.01526* (-1.0);
		gz_d = (double)gz * 0.01526;
	  
		//加速度センサ
		HAL_I2C_Mem_Read(&hi2c1, ADXL375_READ, ADXL375_DATAX0, 1, &data, 6, 100);
		Ax = data[0] | (data[1]  << 8);
		Ay = data[2] | (data[3]  << 8);
		Az = data[4] | (data[5]  << 8);
		Ax_d = (double)Ax *GRAVITY * 49 / 1000* (-1.0);
		Ay_d = (double)Ay *GRAVITY * 49 / 1000* (-1.0);
		Az_d = (double)Az *GRAVITY * 49 / 1000;
	  
		//サーボ
		servo = 0;
		if (HAL_GPIO_ReadPin(Servo0_GPIO_Port, Servo0_Pin) == GPIO_PIN_SET)servo |= 0x01;
		if (HAL_GPIO_ReadPin(Servo1_GPIO_Port, Servo1_Pin) == GPIO_PIN_SET)servo |= 0x02;
	  
	  
	  
#if ( UART_DEBUG ==1 )
			  //UART送信
		sprintf(str, "%f, %f  ", press_d, temp_d);
		HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
		sprintf(str, " %+3.8f , %+3.8f , %+3.8f ,, %+3.8f , %+3.8f , %+3.8f ,%+4d,%+4d,%+4d  ,", ax_d, ay_d, az_d, gx_d, gy_d, gz_d, mx, my, mz);
		HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
		sprintf(str, " %+3.8f , %+3.8f , %+3.8f,", Ax_d, Ay_d, Az_d);
		HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
		sprintf(str, "%d", servo);
		HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
		sprintf(str, "   %d  %d", g_ADCBuffer[0], g_ADCBuffer[1]);
		HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
		sprintf(str, "\r\n");
		HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
#endif

			 //地上からの制御を処理する
#if (WIRELESS ==1 )
		if (control_state != 0) {
			HAL_UART_Receive_IT(&huart6, (uint8_t *)&uart_rx_data, 1);
		 }
		if (control_state == 101) {
			control_state = 0;
			break;
		}
		if (control_state == 102) {
			control_state = 0;
			height_offset = height;
		 }
		if (n == 20){
			height = (((pow(P0 / press_d, 1 / 5.257) - 1)*(temp_d+273.15))/0.0065) - height_offset ;
		}
		if (n == 30) {
			sprintf(str, "\033[10;0H\033[49m\033[33m\033[2K %.1fm", height); 
			HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF);
		}
		if (n == 40) {
			sprintf(str, "\033[30m ,%f,%f,%f,%f,%f,%f,%d,%d,%d\n",
				Ax_d,
				Ay_d,
				Az_d,
				gx_d,
				gy_d,
				gz_d,
				servo,
				g_ADCBuffer[0],
				g_ADCBuffer[1]); 
			HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF);
		}
		
#endif
	  
#if ( UART_DEBUG ==0 )

		while (TIM1->CNT < 10000)
			;// 1/100sたつまで待つ
		t = TIM1->CNT;
		TIM1->CNT = 0;//タイマーを0にする
		sprintf(str,
			"%d,%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%d,%f,%f,%f,%d,%d,%d\n",
			t,
			press_d,
			temp_d,
			ax_d,
			ay_d,
			az_d,
			gx_d,
			gy_d,
			gz_d,
			mx,
			my,
			mz,
			Ax_d,
			Ay_d,
			Az_d,
			servo,
			g_ADCBuffer[0],
			g_ADCBuffer[1]);
	  
		n++;
		HAL_ADC_Stop_DMA(&hadc1);
		ret = f_puts(str, &fil);
		if (ret == EOF) {
			sprintf(str, "f_puts() : error \n");
			HAL_UART_Transmit(&huart1, str, strlen(str), 0x00FF);
		}
		if (n == 50) {
		  
			ret = f_sync(&fil);
			if (ret == FR_OK)HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
			n = 0;
		  
		}
		HAL_ADC_Start_DMA(&hadc1, g_ADCBuffer, ADC_BUFFER_LENGTH);
	  
#endif

	  
	}
	/* USER CODE END 3 */
	HAL_ADC_Stop_DMA(&hadc1);
#if ( UART_DEBUG ==0 )
	f_sync(&fil);
#endif
	sprintf(str,"\033[13;0H\033[49m\033[39m\033[2KRecording stop\nPlease turn off the power."); 
	HAL_UART_Transmit(&huart6, str, strlen(str), 0x00FF); HAL_Delay(120);
}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;

	    /**Configure the main internal regulator output voltage 
	    */
	__HAL_RCC_PWR_CLK_ENABLE();

	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	    /**Initializes the CPU, AHB and APB busses clocks 
	    */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = 16;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 84;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	    /**Initializes the CPU, AHB and APB busses clocks 
	    */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
	                            | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	    /**Configure the Systick interrupt time 
	    */
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);

	    /**Configure the Systick 
	    */
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	  /* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* ADC1 init function */
static void MX_ADC1_Init(void)
{

	ADC_ChannelConfTypeDef sConfig;

	    /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) 
	    */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.ScanConvMode = ENABLE;
	hadc1.Init.ContinuousConvMode = ENABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 2;
	hadc1.Init.DMAContinuousRequests = ENABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	if (HAL_ADC_Init(&hadc1) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
	    */
	sConfig.Channel = ADC_CHANNEL_10;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
	    */
	sConfig.Channel = ADC_CHANNEL_11;
	sConfig.Rank = 2;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

}

/* I2C1 init function */
static void MX_I2C1_Init(void)
{

	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 400000;
	hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

}

/* SDIO init function */
static void MX_SDIO_SD_Init(void)
{

	hsd.Instance = SDIO;
	hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
	hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
	hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
	hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
	hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
	hsd.Init.ClockDiv = 5;

}

/* TIM1 init function */
static void MX_TIM1_Init(void)
{

	TIM_ClockConfigTypeDef sClockSourceConfig;
	TIM_MasterConfigTypeDef sMasterConfig;

	htim1.Instance = TIM1;
	htim1.Init.Prescaler = 84;
	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1.Init.Period = 65535;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.RepetitionCounter = 0;
	if (HAL_TIM_Base_Init(&htim1) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

}

/* USART6 init function */
static void MX_USART6_UART_Init(void)
{

	huart6.Instance = USART6;
	huart6.Init.BaudRate = 115200;
	huart6.Init.WordLength = UART_WORDLENGTH_8B;
	huart6.Init.StopBits = UART_STOPBITS_1;
	huart6.Init.Parity = UART_PARITY_NONE;
	huart6.Init.Mode = UART_MODE_TX_RX;
	huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart6.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart6) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{
  /* DMA controller clock enable */
	__HAL_RCC_DMA2_CLK_ENABLE();

	  /* DMA interrupt init */
	  /* DMA2_Stream0_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

	GPIO_InitTypeDef GPIO_InitStruct;

	  /* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	  /*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, LED1_Pin | LED2_Pin, GPIO_PIN_RESET);

	  /*Configure GPIO pins : LED1_Pin LED2_Pin */
	GPIO_InitStruct.Pin = LED1_Pin | LED2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	  /*Configure GPIO pins : Servo0_Pin Servo1_Pin */
	GPIO_InitStruct.Pin = Servo0_Pin | Servo1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void _Error_Handler(char * file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
