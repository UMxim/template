/**
 * @file stm32l011_my_hal.c
 * @brief Реализация пользовательского HAL для STM32L011.
 *
 * Содержит реализации функций для работы с I2C, EEPROM, ADC, SysTick и IWDG.
 *
 * Логика работы:
 * - I2C: Функции реализуют стандартные сценарии записи (Write) и записи-чтения (Write-Read)
 *   по протоколу I2C с проверкой состояния шины и таймаутом.
 * - EEPROM: Функции разблокируют/блокируют EEPROM, проверяют границы,
 *   и выполняют побайтовую/многобайтовую запись/чтение.
 * - ADC: Настройка и запуск одиночного преобразования на указанный канал.
 * - SysTick: Использует TIM2 для генерации прерываний каждую мс и инкремента глобальной переменной.
 * - IWDG: Настройка сторожевого таймера на максимальный возможный таймаут.
 */
#ifdef STM32L011xx

#include "max_hal.h"
#include <string.h>
// ===== I2C =====

/**
 * @brief Макрос для проверки состояния I2C и обработки ошибок/таймаута.
 *
 * Проверяет флаги ошибок (NACK, ARLO, BERR) и таймаут.
 * Если флаг установлен или таймаут истёк, возвращает соответствующий код ошибки.
 * Используется внутри функций i2c_write и i2c_read.
 *
 * @param I2Cn Указатель на периферию I2C (например, I2C1).
 */
#define _CHECK_I2C_STATE(I2Cn) do { \
		  	  	  	  	  	  	if (--timeout == 0) return -5; \
		        				if (LL_I2C_IsActiveFlag_NACK(I2Cn)) { LL_I2C_ClearFlag_NACK(I2Cn); return -2; } \
		        				if (LL_I2C_IsActiveFlag_ARLO(I2Cn)) { LL_I2C_ClearFlag_ARLO(I2Cn); return -3; } \
		        				if (LL_I2C_IsActiveFlag_BERR(I2Cn)) { LL_I2C_ClearFlag_BERR(I2Cn); return -4; } \
		        				} while(0)

/**
 * @brief Реализация записи данных в устройство по шине I2C.
 *
 * Выполняет сценарий: START -> (addr + WRITE) -> [reg_bytes] -> [data_bytes] -> STOP.
 *
 * @param[in] I2Cx Указатель на I2C периферию.
 * @param[in] addr 7-битный адрес устройства, сдвинутый влево.
 * @param[in] reg Указатель на буфер с адресом регистра.
 * @param[in] reg_size Размер адреса регистра.
 * @param[in] buff Указатель на буфер с данными.
 * @param[in] size Размер данных.
 * @return 1 при успехе, отрицательное значение при ошибке (-2: NACK, -3: ARLO, -4: BERR, -5: Timeout).
 */
pt_t maxhal_i2c_write(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t *reg, uint16_t reg_size, uint8_t *buff, uint16_t size)
{

 /*   // Проверки на валидность аргументов
	ASSERT_DBG(buff || reg, ERROR_WRONG_PARAM); // Нечего делать
	ASSERT_DBG(reg_size || size, ERROR_WRONG_PARAM); // Нечего делать

	// Расчет общего количества байт для передачи
	uint32_t total_size = reg_size + size;

	// Настройка CR2:
	// - Очистка старого адреса и количества байт
	// - Установка адреса (SADD), количества байт (NBYTES)
	// - Режим AUTOEND и генерация START
	uint32_t cr2 = I2Cx->CR2;
	cr2 &= ~((uint32_t)(I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND | I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_STOP));

	// Адрес в STM32 смещается на 1 бит влево (SADD[7:1])
	cr2 |= (uint32_t)((uint32_t)addr & I2C_CR2_SADD);
	cr2 |= (uint32_t)((total_size << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES);
	cr2 |= I2C_CR2_AUTOEND;
	cr2 |= I2C_CR2_START;

	I2Cx->CR2 = cr2;

	    // Передать байты регистра (адресную часть внутри устройства)
	    for (uint16_t i = 0; i < reg_size; i++)
	    {
	        while (!(i2c->ISR & I2C_ISR_TXIS))
	        {
	            // Здесь должна быть проверка ошибок (NACK, BERR и т.д.) через ваш макрос
	            // _CHECK_I2C_STATE_CMSIS(i2c);
	        }
	        i2c->TXDR = reg[i];
	    }

	    // Передать байты данных
	    for (uint16_t i = 0; i < size; i++)
	    {
	        while (!(i2c->ISR & I2C_ISR_TXIS))
	        {
	            // _CHECK_I2C_STATE_CMSIS(i2c);
	        }
	        i2c->TXDR = buff[i];
	    }

	    // Дождаться флага STOP (так как включен AUTOEND, стоп сгенерируется сам)
	    while (!(i2c->ISR & I2C_ISR_STOPF))
	    {
	        // _CHECK_I2C_STATE_CMSIS(i2c);
	    }

	    // Сбросить флаг STOP
	    i2c->ICR = I2C_ICR_STOPCF;

	    return 1; // Успех
	}













    // Инициировать передачу: START + адрес + WRITE
    LL_I2C_HandleTransfer(I2Cx, addr, LL_I2C_ADDRSLAVE_7BIT, reg_size + size, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

    // Передать байты регистра
    for (uint16_t i = 0; i < reg_size; i++)
    {
    	uint32_t timeout = 10000;
    	while (!LL_I2C_IsActiveFlag_TXIS(I2Cx)) // Ждать, пока буфер передатчика готов
    		_CHECK_I2C_STATE(I2Cx);
    	LL_I2C_TransmitData8(I2Cx, reg[i]);
    }

    // Передать байты данных
    for (uint16_t i = 0; i < size; i++)
    {
    	uint32_t timeout = 10000;
        while (!LL_I2C_IsActiveFlag_TXIS(I2Cx)) // Ждать, пока буфер передатчика готов
        	_CHECK_I2C_STATE(I2Cx);
        LL_I2C_TransmitData8(I2Cx, buff[i]);
    }

    // Дождаться флага STOP (передача завершена)
    uint32_t timeout = 10000;
    while (!LL_I2C_IsActiveFlag_STOP(I2Cx))
    	_CHECK_I2C_STATE(I2Cx);
    // Сбросить флаг STOP
    LL_I2C_ClearFlag_STOP(I2Cx);

    return 1; // успех*/
}

/**
 * @brief Реализация чтения данных из устройства по шине I2C.
 *
 * Выполняет сценарий: START -> (addr + WRITE) -> [reg_bytes] -> RESTART -> (addr + READ) -> [data_bytes] -> STOP.
 * Если reg_size = 0, пропускается этап записи регистра.
 *
 * @param[in] I2Cx Указатель на I2C периферию.
 * @param[in] addr 7-битный адрес устройства, сдвинутый влево.
 * @param[in] reg Указатель на буфер с адресом регистра.
 * @param[in] reg_size Размер адреса регистра.
 * @param[out] buff Указатель на буфер для данных.
 * @param[in] size Размер данных для чтения.
 * @return 1 при успехе, отрицательное значение при ошибке (-1: неверные аргументы, -2: NACK, -3: ARLO, -4: BERR, -5: Timeout).
 */
int i2c_read(I2C_TypeDef *I2Cx, uint8_t addr, uint8_t *reg, uint16_t reg_size, uint8_t *buff, uint16_t size)
{
  /*  // Проверки на валидность аргументов
    if ((!reg && reg_size > 0) || (!buff && size > 0)) {
        return -1; // Неверные аргументы
    }
    if (reg_size == 0 && size == 0) {
        return 0; // Нечего делать — успех
    }

    uint32_t timeout;

    // Этап 1: Запись адреса регистра (если требуется)
    if (reg_size > 0)
    {
        // Начать передачу адреса регистра
        LL_I2C_HandleTransfer(I2Cx, addr, LL_I2C_ADDRSLAVE_7BIT, reg_size, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);

        // Передать байты регистра
        for (uint16_t i = 0; i < reg_size; i++)
        {
            timeout = 10000;
            while (!LL_I2C_IsActiveFlag_TXIS(I2Cx)) // Ждать, пока буфер передатчика готов
                _CHECK_I2C_STATE(I2Cx);
            LL_I2C_TransmitData8(I2Cx, reg[i]);
        }

        // Дождаться завершения передачи регистра (флаг TC)
        timeout = 10000;
        while (!LL_I2C_IsActiveFlag_TC(I2Cx))
            _CHECK_I2C_STATE(I2Cx);
    }

    // Этап 2: Чтение данных
    if (size > 0)
    {
        // Начать чтение данных (с RESTART, если писали регистр, или с START, если нет)
        LL_I2C_HandleTransfer(I2Cx, addr, LL_I2C_ADDRSLAVE_7BIT, size, LL_I2C_MODE_AUTOEND, reg_size > 0 ? LL_I2C_GENERATE_RESTART_7BIT_READ : LL_I2C_GENERATE_START_READ);

        // Прочитать байты данных
        for (uint16_t i = 0; i < size; i++)
        {
            timeout = 10000;
            while (!LL_I2C_IsActiveFlag_RXNE(I2Cx)) // Ждать, пока буфер приёмника готов
                _CHECK_I2C_STATE(I2Cx);
            buff[i] = LL_I2C_ReceiveData8(I2Cx);
        }

        // Дождаться флага STOP (чтение завершено)
        timeout = 10000;
        while (!LL_I2C_IsActiveFlag_STOP(I2Cx))
            _CHECK_I2C_STATE(I2Cx);
        // Сбросить флаг STOP
        LL_I2C_ClearFlag_STOP(I2Cx);
    }

    return 1; // успех*/
}

// ===== EEPROM =====

#define EEPROM_PEKEY1           (0x89ABCDEFU)
#define EEPROM_PEKEY2           (0x02030405U)

uint32_t EEPROM_Get_Handler(uint16_t size)
{
	static uint16_t offset = 0;
	size = (size + 3) & (~3U);
	ASSERT_DEBUG(offset + size > EEPROM_SIZE);
	uint32_t res = (offset << 16) | size;
	offset += size;
	return res;
}

static void EEPROM_Unlock(void)
{
    if (FLASH->PECR & FLASH_PECR_PELOCK)
    {
        FLASH->PEKEYR = EEPROM_PEKEY1;
        FLASH->PEKEYR = EEPROM_PEKEY2;
    }
}

static void EEPROM_Lock(void)
{
    FLASH->PECR |= FLASH_PECR_PELOCK;
}

void EEPROM_Read(uint32_t handler, uint16_t offset, void *data, uint16_t size)
{
	uint16_t hoffset = handler >> 16;
	uint16_t hsize = handler;
	ASSERT_DEBUG(offset + size > hsize);
	ASSERT_DEBUG(hoffset + offset + size > EEPROM_SIZE);
	ASSERT_DEBUG(!data);
	uint8_t *ptr = (uint8_t *)(EEPROM_BASE_ADDR + hoffset + offset);
	memcpy(data, ptr, size);
}

uint8_t EEPROM_Read_byte(uint32_t handler, uint16_t offset)
{
	uint16_t hoffset = handler >> 16;
	uint16_t hsize = handler;
	ASSERT_DEBUG(offset + sizeof(uint8_t) > hsize);
	ASSERT_DEBUG(hoffset + offset + sizeof(uint8_t) > EEPROM_SIZE);
	uint8_t res = *(uint8_t *)(EEPROM_BASE_ADDR + hoffset + offset);
	return res;
}

uint16_t EEPROM_Read_halfword(uint32_t handler, uint16_t offset)
{
	uint16_t hoffset = handler >> 16;
	uint16_t hsize = handler;
	ASSERT_DEBUG(offset + sizeof(uint16_t) > hsize);
	ASSERT_DEBUG(hoffset + offset + sizeof(uint16_t) > EEPROM_SIZE);
	ASSERT_DEBUG((hoffset + offset) & 1);
	uint16_t res = *(uint16_t *)(EEPROM_BASE_ADDR + hoffset + offset);
	return res;
}

uint32_t EEPROM_Read_word(uint32_t handler, uint16_t offset)
{
	uint16_t hoffset = handler >> 16;
	uint16_t hsize = handler;
	ASSERT_DEBUG(offset + sizeof(uint32_t) > hsize);
	ASSERT_DEBUG(hoffset + offset + sizeof(uint32_t) > EEPROM_SIZE);
	ASSERT_DEBUG((hoffset + offset) & 3);
	uint32_t res = *(uint32_t *)(EEPROM_BASE_ADDR + hoffset + offset);
	return res;
}

void EEPROM_Write_data(uint32_t handler, uint16_t offset, void* data_in, uint16_t size_byte)
{
	uint8_t *data = (uint8_t *)data_in;
	uint16_t hoffset = handler >> 16;
	uint16_t hsize = handler;
	ASSERT_DEBUG(offset + size_byte > hsize);
	ASSERT_DEBUG(hoffset + offset + size_byte > EEPROM_SIZE);
	offset += hoffset;
	EEPROM_Unlock();
	while(size_byte)
	{
		FLASH->SR = FLASH_SR_EOP | FLASH_SR_WRPERR | FLASH_SR_PGAERR;		// Сброс флагов ошибок перед записью
		if ((offset & 3) || (size_byte < sizeof(uint32_t)))
		{
			if (*(volatile uint8_t *)(EEPROM_BASE_ADDR + offset) != *data)
			{
				*(volatile uint8_t *)(EEPROM_BASE_ADDR + offset) = *data;		// Запись байта
			}
			data++;
			offset++;
			size_byte--;
		}
		else
		{
			uint32_t word;
			memcpy(&word, data, sizeof(word));
			if (*(volatile uint32_t *)(EEPROM_BASE_ADDR + offset) != word)
			{
				*(volatile uint32_t *)(EEPROM_BASE_ADDR + offset) = word;		// Запись слова
			}
			offset+= 4;
			size_byte-= 4;
			data += 4;
		}
		while (FLASH->SR & FLASH_SR_BSY);									// Ожидание завершения записи
	}

	EEPROM_Lock();
}

void EEPROM_WriteByte(uint32_t handler, uint16_t offset, uint8_t data)
{
	uint16_t hoffset = handler >> 16;
	uint16_t hsize = handler;
	ASSERT_DEBUG(offset + sizeof(uint8_t) > hsize);
	ASSERT_DEBUG(hoffset + offset + sizeof(uint8_t) > EEPROM_SIZE);
    EEPROM_Unlock();
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_WRPERR | FLASH_SR_PGAERR;			// Сброс флагов ошибок
    *(volatile uint8_t *)(EEPROM_BASE_ADDR + hoffset + offset) = data;				// Запись байта
    while (FLASH->SR & FLASH_SR_BSY);										// Ожидание завершения записи
    EEPROM_Lock();
}

void EEPROM_WriteHalfWord(uint32_t handler, uint16_t offset, uint16_t data)
{
	uint16_t hoffset = handler >> 16;
	uint16_t hsize = handler;
	ASSERT_DEBUG(offset + sizeof(uint16_t) > hsize);
	ASSERT_DEBUG(hoffset + offset + sizeof(uint16_t) > EEPROM_SIZE);
	ASSERT_DEBUG((hoffset + offset) & 1);
    EEPROM_Unlock();
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_WRPERR | FLASH_SR_PGAERR;			// Сброс флагов ошибок
    *(volatile uint16_t *)(EEPROM_BASE_ADDR + hoffset + offset) = data;				// Запись байта
    while (FLASH->SR & FLASH_SR_BSY);										// Ожидание завершения записи
    EEPROM_Lock();
}

void EEPROM_WriteWord(uint32_t handler, uint16_t offset, uint32_t data)
{
	uint16_t hoffset = handler >> 16;
	uint16_t hsize = handler;
	ASSERT_DEBUG(offset + sizeof(uint32_t) > hsize);
	ASSERT_DEBUG(hoffset + offset + sizeof(uint32_t) > EEPROM_SIZE);
	ASSERT_DEBUG((hoffset + offset) & 3);
    EEPROM_Unlock();
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_WRPERR | FLASH_SR_PGAERR;			// Сброс флагов ошибок
    *(volatile uint32_t *)(EEPROM_BASE_ADDR + hoffset + offset) = data;				// Запись байта
    while (FLASH->SR & FLASH_SR_BSY);										// Ожидание завершения записи
    EEPROM_Lock();
}

void OB_disable_boot0_pin()
{
	#define FLASH_OPTR_nBOOT_SEL 	(1 << 29)
	#define FLASH_OPTR_nBOOT0   	(1 << 30)

	uint32_t reg = FLASH->OPTR;
	if (BIT_GET_MASK(reg, FLASH_OPTR_nBOOT_SEL | FLASH_OPTR_nBOOT0) == (FLASH_OPTR_nBOOT_SEL | FLASH_OPTR_nBOOT0) ) return;
	// 4. Разблокировка PECR
	FLASH->PEKEYR = 0x89ABCDEF;
	FLASH->PEKEYR = 0x02030405;
	// 5. Разблокировка Option Bytes
	FLASH->OPTKEYR = 0xFBEAD9C8;
	FLASH->OPTKEYR = 0x24252627;
	// 6. Ожидание готовности
	while (FLASH->SR & FLASH_SR_BSY);
	// 8. Подготовка и запись нового значения OPTR по адресу 0x1FF80000
	reg |= FLASH_OPTR_nBOOT_SEL | FLASH_OPTR_nBOOT0;
	uint32_t new_val = (reg & 0xFFFF) | (~(reg & 0xFFFF) << 16);
	*(__IO uint32_t *)0x1FF80000 = new_val;
	reg >>= 16;
	new_val = (reg & 0xFFFF) | (~(reg & 0xFFFF) << 16);
	*(__IO uint32_t *)0x1FF80004 = new_val;
	// ожидание завершения программирования
	while (FLASH->SR & FLASH_SR_BSY) ;
	// 12. Применение настроек — ВЫЗОВЕТ НЕМЕДЛЕННЫЙ СБРОС!
	FLASH->PECR |= FLASH_PECR_OBL_LAUNCH;
	while(1);
}

// ===== ADC =====

/**
 * @brief Чтение значения с указанного канала АЦП.
 * @param[in] channel Канал АЦП (например, LL_ADC_CHANNEL_VREFINT).
 * @return Значение АЦП (12-битное).
 */
uint16_t Read_ADC_Channel(uint32_t channel) //  LL_ADC_CHANNEL_X или LL_ADC_CHANNEL_VREFINT или LL_ADC_CHANNEL_TEMPSENSOR
{
 /*   // Установить канал для одиночного преобразования
    LL_ADC_REG_SetSequencerChannels(ADC1, channel);

    // Запустить преобразование
    LL_ADC_REG_StartConversion(ADC1);

    // Дождаться завершения
    while (!LL_ADC_IsActiveFlag_EOC(ADC1));

    // Сбросить флаг завершения
    LL_ADC_ClearFlag_EOC(ADC1);

    // Прочитать результат
    return LL_ADC_REG_ReadConversionData12(ADC1);*/
}

/**
 * @brief Запуск сторожевого таймера (IWDG) с максимальным таймаутом.
 *
 * Устанавливает максимальный предделитель (256) и максимальное значение
 * перезагрузки (4095), что даёт таймаут ~32.7 секунды (при LSI ~37KHz).
 */
void IWDG_Start_MaxTimeout(void)
{
    // 1. Разрешить доступ к регистрам IWDG
    IWDG->KR = 0x5555;

    // 2. Установить максимальный предделитель (PR = 0b111 -> 256)
    IWDG->PR = IWDG_PR_PR_2 | IWDG_PR_PR_1 | IWDG_PR_PR_0;

    // 3. Установить максимальное значение перезагрузки (RLR = 0xFFF -> 4095)
    IWDG->RLR = 0xFFF;

    // 4. Перезагрузить счётчик IWDG
    IWDG->KR = 0xAAAA;

    // 5. Запустить IWDG (отключить нельзя после этого!)
    IWDG->KR = 0xCCCC;
}

/**
 * @brief Обновление (перезапуск) сторожевого таймера (IWDG).
 * "Поглаживание собаки" - предотвращает сброс МК.
 */
void IWDG_Refresh(void)
{
    IWDG->KR = 0xAAAA; // Команда перезагрузки счётчика
}

#endif //#ifdef STM32L011xx

