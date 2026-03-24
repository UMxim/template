/*
 * max_hal.h
 *
 *  Created on: Feb 22, 2026
 *      Author: Max
 */

#ifndef MAX_HAL_H_
#define MAX_HAL_H_

/**
 * @file stm32l011_my_hal.h
 * @brief Заголовочный файл пользовательского HAL для STM32L011.
 *
 * Содержит объявления функций для работы с периферией (I2C, EEPROM, ADC, SysTick, IWDG),
 * которые не представлены в стандартной LL библиотеке или требуют упрощённого интерфейса.
 *
 * Логика работы:
 * - I2C: Функции i2c_write/read упрощают обмен данными с внешними устройствами
 *   по протоколу I2C, включая указание адреса регистра.
 * - EEPROM: Функции для чтения/записи данных в энергонезависимую память.
 * - ADC: Функция для чтения значения с указанного канала АЦП.
 * - SysTick: Инициализация и глобальная переменная для отсчёта времени в мс.
 * - IWDG: Управление сторожевым таймером.
 */

#ifndef STM32L011_MY_HAL_H_
#define STM32L011_MY_HAL_H_

#include <stddef.h>
#include <stdint.h>
#include "defines.h"
#include "pt.h"

#ifdef STM32L011xx

#include "stm32l0xx.h"

#define ADC_V_REF_mV			1224	// Значение Vref у этого МК

void OB_disable_boot0_pin();

#define Storage_Get_Handler 	EEPROM_Get_Handler
#define Storage_Read			EEPROM_Read
#define Storage_Write_data 		EEPROM_Write_data
// ===== EEPROM =====

uint32_t EEPROM_Get_Handler(uint16_t size);	// Резервирование памяти и возврат хэндлера

void 	 EEPROM_Read(uint32_t handler, uint16_t offset, void *data, uint16_t size);
uint8_t  EEPROM_Read_byte(uint32_t handler, uint16_t offset);
uint16_t EEPROM_Read_halfword(uint32_t handler, uint16_t offset);
uint32_t EEPROM_Read_word(uint32_t handler, uint16_t offset);

void EEPROM_Write_data(uint32_t handler, uint16_t offset, void* data_in, uint16_t size_byte);
void EEPROM_WriteByte(uint32_t handler, uint16_t offset, uint8_t data);
void EEPROM_WriteHalfWord(uint32_t handler, uint16_t offset, uint16_t data);
void EEPROM_WriteWord(uint32_t handler, uint16_t offset, uint32_t data);




#elif defined(STM32F103xB)

#include "stm32f1xx.h"

void CheckMaxFlash();

uint32_t 	Flash_Get_Handler(uint16_t size);
void 		Flash_Read(uint32_t handler, uint16_t offset, void *data, uint16_t size);
void 		Flash_Write_data(uint32_t handler, uint16_t offset, void* data_in, uint16_t size_byte);

#define Storage_Get_Handler 	Flash_Get_Handler
#define Storage_Read			Flash_Read
#define Storage_Write_data 		Flash_Write_data

#include "stm32f1xx.h"

#endif // stm32XXX

// ===== I2C =====

/**
 * @brief Запись данных в устройство по шине I2C.
 *
 * Отправляет сначала адрес регистра (или команду), затем сами данные.
 *
 * @param[in] I2Cx Указатель на I2C периферию (например, I2C1).
 * @param[in] addr 7-битный адрес устройства, сдвинутый влево (LSB = 0).
 * @param[in] reg Указатель на буфер с адресом регистра для записи.
 * @param[in] reg_size Размер адреса регистра в байтах (обычно 1).
 * @param[in] buff Указатель на буфер с данными для записи.
 * @param[in] size Размер данных для записи в байтах.
 * @return Результат операции (обычно количество переданных байт или код ошибки).
 */
pt_t maxhal_i2c_write(I2C_TypeDef *i2c, uint8_t addr, uint8_t *reg, uint16_t reg_size, uint8_t *buff, uint16_t size);

/**
 * @brief Чтение данных из устройства по шине I2C.
 *
 * Сначала записывает адрес регистра, затем читает данные.
 *
 * @param[in] I2Cx Указатель на I2C периферию (например, I2C1).
 * @param[in] addr 7-битный адрес устройства, сдвинутый влево (LSB = 0).
 * @param[in] reg Указатель на буфер с адресом регистра для чтения.
 * @param[in] reg_size Размер адреса регистра в байтах (обычно 1).
 * @param[out] buff Указатель на буфер для получения данных.
 * @param[in] size Размер данных для чтения в байтах.
 * @return Результат операции (обычно количество полученных байт или код ошибки).
 */
int i2c_read(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t *reg, uint16_t reg_size, uint8_t *buff, uint16_t size);


// ===== ADC =====

/**
 * @brief Чтение значения с указанного канала АЦП.
 *
 * @param[in] channel Канал АЦП (например, LL_ADC_CHANNEL_VREFINT).
 * @return Значение АЦП (обычно 12-битное).
 */
uint16_t Read_ADC_Channel(uint32_t channel);

/// @brief Опорное напряжение АЦП в мВ.

// ===== IWDG =====

/**
 * @brief Запуск сторожевого таймера (IWDG) с максимальным таймаутом.
 */
void IWDG_Start_MaxTimeout(void);

/**
 * @brief Обновление (перезапуск) сторожевого таймера (IWDG).
 */
void IWDG_Refresh(void);

#endif /* STM32L011_MY_HAL_H_ */


#endif /* MAX_HAL_H_ */
