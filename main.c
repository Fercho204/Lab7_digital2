/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Cuerpo del programa principal
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * Este software está licenciado bajo términos que se encuentran en el archivo LICENSE
  * en el directorio raíz de este componente de software.
  * Si no se incluye un archivo LICENSE, se proporciona TAL CUAL.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "stdio.h"
/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
FATFS fs;           // Sistema de archivos
FIL fil;           // Objeto de archivo
FRESULT fres;      // Resultado de operaciones FATFS
DIR dir;           // Objeto de directorio
FILINFO fno;       // Información de archivo
char buffer[100];  // Buffer para lectura
#define MAX_FILES 10 // Máximo número de archivos a mostrar
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;  // Declaración del manejador UART

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_SPI1_Init(void);
void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void transmit_uart(char* string);         // Función para transmitir por UART
void display_menu(void);                  // Función para mostrar el menú
void read_and_display_file(char* filename); // Función para leer y mostrar archivo
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void transmit_uart(char* string) {
    uint8_t len = strlen(string);
    HAL_UART_Transmit(&huart2, (uint8_t*) string, len, 200);
    HAL_Delay(10); // Pequeño retraso para que la terminal procese la línea
}

void display_menu(void) {
    char file_list[MAX_FILES][13]; // Lista de nombres de archivo (formato 8.3)
    uint8_t file_count = 0;        // Contador de archivos
    char menu_str[100];           // Buffer para mensajes del menú

    // Abrir directorio raíz
    fres = f_opendir(&dir, "/");
    if (fres != FR_OK) {
        transmit_uart("¡Error al abrir el directorio!\n");
        return;
    }

    // Mostrar encabezado del menú
    transmit_uart("\n=== Galeria de Arte ASCII ===\n");
    transmit_uart("Archivos disponibles:\n");

    // Leer contenido del directorio
    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0] && file_count < MAX_FILES) {
        if (!(fno.fattrib & AM_DIR)) { // Verificar que sea archivo, no directorio
            strcpy(file_list[file_count], fno.fname);
            sprintf(menu_str, "%d. %13s\n", file_count + 1, fno.fname); // Limitar a 13 caracteres
            transmit_uart(menu_str);
            file_count++;
        }
    }
    f_closedir(&dir);

    // Mostrar mensaje de selección
    transmit_uart("\nIngrese numero para seleccionar archivo (1-");
    sprintf(menu_str, "%d): ", file_count);
    transmit_uart(menu_str);

    // Obtener entrada del usuario
    char input[10];
    HAL_UART_Receive(&huart2, (uint8_t*)input, 2, HAL_MAX_DELAY);
    input[1] = '\0'; // Terminar cadena
    int choice = atoi(input);

    // Validar y mostrar archivo seleccionado
    if (choice > 0 && choice <= file_count) {
        read_and_display_file(file_list[choice-1]);
    } else {
        transmit_uart("¡Seleccion invalida!\n");
    }
}

void read_and_display_file(char* filename) {
    fres = f_open(&fil, filename, FA_READ);
    if (fres == FR_OK) {
        transmit_uart("\nMostrando Arte ASCII:\n");
        while (f_gets(buffer, sizeof(buffer), &fil)) {
            // Eliminar cualquier '\r' (retorno de carro) que pueda estar presente
            char *pos;
            if ((pos = strchr(buffer, '\r')) != NULL) {
                *pos = '\0'; // Reemplazar '\r' con terminador nulo
            }
            // Transmitir la línea
            transmit_uart(buffer);
            // Asegurarse de que haya un salto de línea
            if (buffer[strlen(buffer)-1] != '\n') {
                transmit_uart("\n");
            }
        }
        f_close(&fil);
        transmit_uart("\nFin del arte\n");
    } else {
        transmit_uart("¡Error al abrir el archivo!\n");
    }
}
/* USER CODE END 0 */

/**
  * @brief  Punto de entrada de la aplicación
  * @retval int
  */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART2_UART_Init();
    MX_FATFS_Init();

    HAL_Delay(500); // Esperar inicialización de la tarjeta SD

    // Montar sistema de archivos
    fres = f_mount(&fs, "/", 0);
    if (fres == FR_OK) {
        transmit_uart("¡Tarjeta Micro SD montada exitosamente!\n");
    } else {
        transmit_uart("¡Error al montar la tarjeta Micro SD!\n");
        return 0;
    }

    // Bucle principal con el menú
    while (1) {
        display_menu();
        HAL_Delay(1000); // Pequeña pausa antes de mostrar menú otra vez
    }

    // Desmontar sistema de archivos (no se alcanza por el bucle infinito)
    f_mount(NULL, "", 1);
    return 0;
}

/**
  * @brief Configuración del reloj del sistema
  * @retval None
  */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 64;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief Inicialización de SPI1
  * @retval None
  */
void MX_SPI1_Init(void) {
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief Inicialización de USART2
  * @retval None
  */
void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief Inicialización de GPIO
  * @retval None
  */
void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(SD_SS_GPIO_Port, SD_SS_Pin, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = SD_SS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(SD_SS_GPIO_Port, &GPIO_InitStruct);
}

/**
  * @brief Función ejecutada en caso de error
  * @retval None
  */
void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief Reporta el nombre del archivo fuente y número de línea donde ocurrió un error
  * @param  file: puntero al nombre del archivo fuente
  * @param  line: número de línea del error
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line) {
    /* El usuario puede agregar su propia implementación */
}
#endif /* USE_FULL_ASSERT */
