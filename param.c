/*
 * param.c
 *
 *  Created on: Feb 15, 2026
 *      Author: Max
 *
 * Flash-шаблон содержит:
 *   - метаданные (hw, sw, git)
 *   - значения параметров по умолчанию
 *   - отображаемые имена параметров и статусов (с пробелами!)
 *
 * RAM содержит:
 *   - сигнатуру begin
 *   - рабочие параметры и статусы
 */

#include "param.h"
#include "template.h"
#include <string.h>

// Сигнатуры
#define FLASH_SIGNATURE 0x44D35274U
#define RAM_SIGNATURE   0x7F4CCB30U  // Бит 0 - 0: Данные не изменились; 1:Изменения, надо записать на флеш

// ============================================================================
// FLASH-ШАБЛОН
// ============================================================================

const struct {
    uint32_t begin;
    uint16_t hw;
    uint16_t sw;
    uint32_t git;
    uint16_t N_param;
    uint16_t N_stat;
    int32_t param_val[PARAM_N];
#define X(id, val, str) str "\0"
    char param_names[sizeof(PARAM_LIST)];
    char stat_names[sizeof(STAT_LIST)];
#undef X
} param_s = {
    .begin      = FLASH_SIGNATURE,
    .hw         = 1,
    .sw         = 1,
    .git        = 0x1233333U,
    .N_param    = PARAM_N,
    .N_stat     = STAT_N,
    .param_val  = {
        #define X(id, val, str) val,
        PARAM_LIST
        #undef X
    },
#define X(id, val, str) str "\0"
    .param_names = PARAM_LIST,
    .stat_names  = STAT_LIST
#undef X
};

// ============================================================================
// RAM-СТРУКТУРА
// ============================================================================
struct param_mem param_mem;

static uint32_t eeprom_handler;
// ============================================================================
// ИНИЦИАЛИЗАЦИЯ
// ============================================================================
void params_init(void)
{
	eeprom_handler = EEPROM_Get_Handler(sizeof(param_mem));
	EEPROM_Read(eeprom_handler, 0, &param_mem, sizeof(param_mem));
	if (param_mem.begin != RAM_SIGNATURE)
	{
		param_mem.begin = RAM_SIGNATURE | 1;
		memcpy(param_mem.param, param_s.param_val, sizeof(param_mem.param));
		memset(param_mem.status, 0, sizeof(param_mem.status));
	}
}

void params_cycle(void)
{
	if (param_mem.begin == (RAM_SIGNATURE | 1))
	{
		param_mem.begin = RAM_SIGNATURE;
		EEPROM_Write_data(eeprom_handler, 0, (uint8_t*)&param_mem, sizeof(param_mem));
	}

}
