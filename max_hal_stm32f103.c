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

// ===== FLASH =====

/* 1. Чтение можно по байтам
 * 2. Запись по 2 байта выровнено
 * 3. Записать можно из 0xFFFF -> 0xXXXX  и 0xXXXX -> 0x0000. Другие комбинации не пишутся
 * 4. Напомню еще раз. Вместо malloc для исключения фрагментации используется стек для сектора = 1024 байта. Не забыть отредактировать в ld
 */

static void _Flash_Unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK)
    {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }
}

static inline void _Flash_Lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

static void _Flash_ErasePage(uint32_t page_address)
{
	__disable_irq();
	_Flash_Unlock();
	FLASH->CR |= FLASH_CR_PER;
	FLASH->AR = page_address;
	FLASH->CR |= FLASH_CR_STRT;
	while (FLASH->SR & FLASH_SR_BSY);
	FLASH->CR &= ~FLASH_CR_PER;
	_Flash_Lock();
	__enable_irq();
}

// Адрес и размер должны быть выровнены по 2 байта. выход за пределы сектора не проверяется. Размер в байтах внутри переводится в полуслова.
static int _Flash_Write(uint32_t address_in, const void *data_in, uint32_t size)
{
    ASSERT_DEBUG(address_in & 1);
    ASSERT_DEBUG((uint32_t)data_in & 1);
    ASSERT_DEBUG(size & 1);
    size >>= 1;
    uint16_t *data = (uint16_t *)data_in;
    __IO uint16_t *address = (__IO uint16_t*)address_in;

    _Flash_Unlock();

    FLASH->CR |= FLASH_CR_PG;
    while(size--)
    {
    	if(*data != *address)
    	{
    		__disable_irq();
    		*address = *data;
    		while(FLASH->SR & FLASH_SR_BSY);
    		__enable_irq();
    	}
    	address++;
    	data++;
    }
    FLASH->CR &= ~FLASH_CR_PG;// Выключаем режим программирования после завершения цикла
    _Flash_Lock();// Финальная блокировка
    return 0;
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
	__attribute__((unused)) uint16_t hsize = handler;
	ASSERT_DEBUG(offset + size > hsize);
	ASSERT_DEBUG(hoffset + offset + size > PARAM_FLASH_SIZE);
	ASSERT_DEBUG(!data);
	uint8_t *ptr = (uint8_t *)PARAM_FLASH_BEGIN + hoffset + offset;
	memcpy(data, ptr, size);
}

// Всё в границах одного сектора. Пока только для Param, надо будет-расширишь
void Flash_Write_data(uint32_t handler, uint16_t offset, void* data_in, uint16_t size_b)
{

	uint16_t hoffset = handler >> 16;
	__attribute__((unused)) uint16_t hsize = handler;
	ASSERT_DEBUG(offset + size_b > hsize);
	ASSERT_DEBUG(hoffset + offset + size_b > PARAM_FLASH_SIZE);
	ASSERT_DEBUG(!data_in);
	ASSERT_DEBUG(hoffset & 3);
	ASSERT_DEBUG(hsize & 3);

	uint8_t *ptr_flash = (uint8_t*)PARAM_FLASH_BEGIN + hoffset + offset;
	// Проверяем, надо ли писать вобще
	if ( !memcmp(ptr_flash, data_in, size_b))
	{
		return;
	}
	// Не стал колдовать с выравниваниями, проверками границ data и flash. Простой но немного более накладный путь - сразу выделяю сектор и только потом смотрю, надо ли стирать. Всё равно запись гораздо дольше чем вся эта возня
	uint16_t sector[PARAM_FLASH_SIZE >> 1];
	memcpy(sector, (void *)PARAM_FLASH_BEGIN, PARAM_FLASH_SIZE);
	memcpy((uint8_t*)sector + hoffset + offset, data_in, size_b);
	// Теперь в sector то, что должно быть на флеше. Проверяем, надо ли стирать.
	for (int pos = 0; pos < (PARAM_FLASH_SIZE >> 1); pos++)
	{
		uint16_t onflash = ((uint16_t *)PARAM_FLASH_BEGIN)[pos];
		if( (onflash == 0xFFFF) || (onflash == sector[pos]) || (sector[pos] == 0) )
		{
			continue;
		}
		_Flash_ErasePage(PARAM_FLASH_BEGIN);
		break;
	}
	_Flash_Write(PARAM_FLASH_BEGIN, sector, PARAM_FLASH_SIZE);

	ASSERT_DEBUG(memcmp(sector, (void*)PARAM_FLASH_BEGIN, PARAM_FLASH_SIZE));
}

static void TestFlash()
{
	uint32_t handler0 = Flash_Get_Handler(512);
	uint32_t handler1 = Flash_Get_Handler(512);

	_Flash_ErasePage(PARAM_FLASH_BEGIN);
	uint32_t data = 0x01234567;
	Flash_Write_data(handler0, 0, &data, sizeof(data));
	data = 0xDEADBEAF;
	Flash_Write_data(handler1, 0, &data, sizeof(data));
	Flash_Write_data(handler1, 4, &data, sizeof(data));
	Flash_Write_data(handler1, 4, &data, sizeof(data));// запись того же
	data = 0x0;
	Flash_Write_data(handler0, 0, &data, sizeof(data));// запись нулей вместо 01234567
	data = 0x89ABCDEF;
	Flash_Write_data(handler0, 0, &data, sizeof(data));// перезапись 0 на 89ABCDEF
}

volatile uint32_t cnt = 0;
uint32_t MaxHal_CheckFlash(uint32_t addr, uint32_t sector_size, uint32_t sectors_qty)
{
	volatile int start = 1;
	while(start);
	TestFlash();

	ASSERT_DEBUG(sector_size & (sector_size - 1));
	ASSERT_DEBUG(addr & (sector_size - 1));

	uint8_t *buff = malloc(sector_size);
	ASSERT_DEBUG(!buff);

	for(int i = 0; i < sectors_qty; i++)
	{
		_Flash_ErasePage(addr);
		memset(buff, 0xFF, sector_size);
		int res = memcmp(buff, (void *)addr, sector_size);
		ASSERT_DEBUG(res);

		_Flash_ErasePage(addr);
		memset(buff, 0xAA, sector_size);
		_Flash_Write(addr, buff, sector_size);
		res = memcmp(buff, (void *)addr, sector_size);
		ASSERT_DEBUG(res);

		_Flash_ErasePage(addr);
		memset(buff, 0x55, sector_size);
		_Flash_Write(addr, buff, sector_size);
		res = memcmp(buff, (void *)addr, sector_size);
		ASSERT_DEBUG(res);

		_Flash_ErasePage(addr);
		memset(buff, 0x00, sector_size);
		_Flash_Write(addr, buff, sector_size);
		res = memcmp(buff, (void *)addr, sector_size);
		ASSERT_DEBUG(res);

		_Flash_ErasePage(addr);
		for (int i = 0; i < sector_size; i++) ((uint8_t *)buff)[i] = i;
		_Flash_Write(addr, buff, sector_size);
		res = memcmp(buff, (void *)addr, sector_size);
		ASSERT_DEBUG(res);

		cnt++;
		addr += sector_size;
	}
	free(buff);
	return cnt;
}

#endif //#ifdef STM32F103xB

