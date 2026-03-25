/**
 * @file stm32f103.c
 * @brief Реализация пользовательского HAL для STM32F103.
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
#ifdef STM32F103xB

#include "max_hal.h"
#include <string.h>
#include <stdlib.h>
#include "template_config.h"

// ===== EEPROM =====

static void Flash_Unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK)
    {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }
}

static void Flash_Lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}


static int Flash_ErasePage(uint32_t page_address)
{
	__disable_irq();
	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xCDEF89AB;
	while (FLASH->SR & FLASH_SR_BSY);
	FLASH->CR |= FLASH_CR_PER;
	FLASH->AR = page_address;
	FLASH->CR |= FLASH_CR_STRT;
	while (FLASH->SR & FLASH_SR_BSY);
	FLASH->CR |= FLASH_CR_LOCK;
	__enable_irq();




	return 1;
	while (FLASH->SR & FLASH_SR_BSY);
    Flash_Unlock();// 2. Разблокировка
    FLASH->SR |= (FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR);// 3. Очистка флагов статуса (EOP и ошибки) перед началом
    FLASH->CR |= FLASH_CR_PER;// 4. Установка бита PER (Page Erase)
    FLASH->AR = page_address;// 5. Запись адреса в AR
    Flash_ErasePage_mem();
    FLASH->CR &= ~FLASH_CR_PER;// 8. Сброс бита PER
    if (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR))// 9. Проверка на ошибки после операции
    {

        FLASH->SR |= (FLASH_SR_PGERR | FLASH_SR_WRPRTERR);// Очищаем флаги ошибок перед выходом
        Flash_Lock(); // ВАЖНО: Блокируем при ошибке записи/защиты
        return -4;
    }
    FLASH->SR |= FLASH_SR_EOP;// Очистка флага EOP (успех)
    Flash_Lock();// 10. Блокировка Flash
    return 0;
}

__attribute__((section(".RamFunc"))) void Flash_write_mem(__IO uint32_t* ptr, uint32_t word)
{
	*ptr = word;
	while (FLASH->SR & FLASH_SR_BSY);
}

static int Flash_Write(uint32_t address, const uint32_t *data, uint32_t size_words)
{
    if (address % 4 != 0) return -1;
    Flash_Unlock();
    while (FLASH->SR & FLASH_SR_BSY);
    FLASH->SR |= (FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR);// Очистка всех флагов один раз перед циклом
    FLASH->CR |= FLASH_CR_PG;// ОПТИМИЗАЦИЯ: Включаем режим программирования ОДИН РАЗ

    for (uint32_t i = 0; i < size_words; i++)    // Прямая запись слова
    {
    	__IO uint32_t* curr_adr = (__IO uint32_t*)(address + (i * 4));
    	if (data[i] == *curr_adr) continue;
    	Flash_write_mem(curr_adr, data[i]);
        while (FLASH->SR & FLASH_SR_BSY);
        if (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR))// Проверка ошибок «на лету»
        {
            FLASH->SR |= (FLASH_SR_PGERR | FLASH_SR_WRPRTERR);// ВАЖНО: Сбрасываем флаги ошибок перед выходом, иначе следующая операция может глючить
            FLASH->CR &= ~FLASH_CR_PG;
            Flash_Lock();
            return -4;
        }
        FLASH->SR |= FLASH_SR_EOP;// Сбрасываем EOP для следующей итерации
    }
    FLASH->CR &= ~FLASH_CR_PG;// Выключаем режим программирования после завершения цикла
    Flash_Lock();// Финальная блокировка
    return 0;
}

static bool Flash_Is_need_erase(uint32_t *flash, uint32_t *data, uint32_t size_word)
{
	while(size_word--)
	{
		if ((flash[size_word] != data[size_word]) && ((flash[size_word] ^ data[size_word]) & data[size_word])) return true;
	}
	return false;
}

uint32_t Flash_Get_Handler(uint16_t size)
{
	static uint16_t offset = 0;
	size = (size + 3) & (~3U);
	ASSERT_DEBUG(offset + size > PARAM_FLASH_SIZE);
	uint32_t res = (offset << 16) | size;
	offset += size;
	return res;
}

void Flash_Read(uint32_t handler, uint16_t offset, void *data, uint16_t size)
{
	uint16_t hoffset = handler >> 16;
	uint16_t hsize = handler;
	ASSERT_DEBUG(offset + size > hsize);
	ASSERT_DEBUG(hoffset + offset + size > PARAM_FLASH_SIZE);
	ASSERT_DEBUG(!data);
	uint8_t *ptr = (uint8_t *)PARAM_FLASH_BEGIN + hoffset + offset;
	memcpy(data, ptr, size);
}

void Flash_Write_data(uint32_t handler, uint16_t offset, void* data_in, uint16_t size_byte)
{
	uint16_t hoffset = handler >> 16;
	uint16_t hsize = handler;
	ASSERT_DEBUG(offset + size_byte > hsize);
	ASSERT_DEBUG(hoffset + offset + size_byte > PARAM_FLASH_SIZE);
	ASSERT_DEBUG(!data_in);
	ASSERT_DEBUG(hoffset & 3);
	ASSERT_DEBUG(hsize & 3);
	hoffset >>= 2;

	uint32_t *h_ptr = malloc(hsize);
	ASSERT_DEBUG(!h_ptr);
	uint32_t *ptr_flash = (uint32_t*)PARAM_FLASH_BEGIN + hoffset;

	memcpy(h_ptr, ptr_flash, hsize);
	memcpy((uint8_t*)h_ptr + offset, data_in, size_byte);

	bool is_need_erase = Flash_Is_need_erase(ptr_flash, h_ptr, hsize >> 2);
	__disable_irq();
	if (is_need_erase)
	{
		uint32_t *sector = malloc(PARAM_FLASH_SIZE);
		ASSERT_DEBUG(!sector);
		memcpy(sector, (void *)PARAM_FLASH_BEGIN, PARAM_FLASH_SIZE);
		memcpy(sector + hoffset, h_ptr, hsize);
		Flash_ErasePage(PARAM_FLASH_BEGIN);
		Flash_Write(PARAM_FLASH_BEGIN, sector, PARAM_FLASH_SIZE >> 2);
		free(sector);
	}
	else
	{
		Flash_Write((uint32_t)ptr_flash, h_ptr, hsize >> 2);
	}
	__enable_irq();
	free(h_ptr);
}

void MaxHal_CheckFlash(uint32_t addr, uint32_t sector_size, uint32_t sectors_qty)
{
	volatile int start = 1;
	while(start);

	ASSERT_DEBUG(!(sector_size & (sector_size - 1)));
	ASSERT_DEBUG(addr & (sector_size - 1));

	void *buff = malloc(sector_size);
	ASSERT_DEBUG(!buff);

	for(int i = 0; i < sectors_qty; i++)
	{
		addr += i * sector_size;
		Flash_ErasePage(addr);

		memset(buff, 0xAA, sector_size);
		Flash_Write(addr, buff, sector_size / 2);



	}



}

#endif //#ifdef STM32F103xB

