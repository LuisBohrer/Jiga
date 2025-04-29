/*
	driver_eeprom.h
	Descrição: funções da EEPROM
	Autor: Lucas Iuri
	Data: 05/07/2024
*/


#include <stdint.h>
#include "stm32l4xx.h"
#include "stm32l431xx.h"
//-----------------------------------------------------------------------------------------

#ifndef EEPROM_H
#define EEPROM_H

#define MEMORY_ADDRESS	0xA0
#define WRITE_TIME		10

//-----------------------------------------------------------------------------------------

/*
* Função: vEEPROM_Write
* Descrição: escreve dados na memória EEPROM
* Parâmetros:
*    - I2C_HandleTypeDef *hi2c: ponteiro para o handler do canal I2C da memória.
*    - uint8_t *ui8_pDataBuffer: ponteiro de onde os dados a serem escritos
*     estão armazenados.
*    - uint32_t ui32EEPROM_memoryAddress: endereço inicial (Máx 2^18-1) da memória onde os
*     dados serão escritos.
*    - uint32_t ui32EEPROM_size: quantidade de bytes a serem escritos (máx. 256Kbytes).
* Retorno:
* 	- HAL_StatusTypeDef: status da escrita de acordo com o padrão da HAL.
*/
HAL_StatusTypeDef EEPROM_Write (I2C_HandleTypeDef *hi2c, uint8_t *dataBuffer, uint32_t memoryAddress, uint32_t dataSize);


/*
* Função: vEEPROM_Read
* Descrição: lê dados da memória EEPROM
* Parâmetros:
*    - I2C_HandleTypeDef *hi2c: ponteiro para o handler do canal I2C da memória.
*    - uint8_t *ui8EEPROM_buffer: ponteiro para o buffer onde os dados lidos serão salvos.
*    - uint32_t ui32EEPROM_memoryAddress: endereço inicial (máx 2^18 -1 ) da memória de
*     onde os dados serão lidos.
*    - uint32_t ui32EEPROM_size: quantidade de bytes a serem lidos (máximo de 256 Kbytes).
* Retorno:
* 	- HAL_StatusTypeDef: status da leitura de acordo com o padrão da HAL.
*/
HAL_StatusTypeDef EEPROM_Read (I2C_HandleTypeDef *hi2c, uint8_t *dataBuffer, uint32_t memoryAddress, uint32_t dataSize);

#endif

