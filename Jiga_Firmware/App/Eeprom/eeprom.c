/*
	driver_eeprom.c
	Descrição: funções da EEPROM
	Autor: Lucas Iuri
	Data: 05/07/2024
*/
//-----------------------------------------------------------------------------------------------
#include <Eeprom/eeprom.h>
//-----------------------------------------------------------------------------------------------

/*
* Função: EEPROM_Write
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
HAL_StatusTypeDef EEPROM_Write (I2C_HandleTypeDef *hi2c, uint8_t *dataBuffer, uint32_t memoryAddress, uint32_t dataSize){
	HAL_StatusTypeDef status;
	uint16_t i2c_mem_address;
	uint8_t device_Address;

	if (memoryAddress >= 1 << 16){
		device_Address = MEMORY_ADDRESS | (memoryAddress & 0x30000) >> 15;
	}
	else device_Address = MEMORY_ADDRESS;

	i2c_mem_address = (uint16_t)memoryAddress;

	status = HAL_I2C_Mem_Write(hi2c, device_Address, i2c_mem_address, I2C_MEMADD_SIZE_16BIT, dataBuffer, dataSize, 100);
	HAL_Delay(10);

	return status;

}


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
HAL_StatusTypeDef EEPROM_Read (I2C_HandleTypeDef *hi2c, uint8_t *dataBuffer, uint32_t memoryAddress, uint32_t dataSize){
	HAL_StatusTypeDef status;
	uint16_t i2c_mem_address;
	uint8_t device_Address;

	if (memoryAddress >= 1 << 16){
		device_Address = MEMORY_ADDRESS | (memoryAddress & 0x30000) >> 15;
	}
	else device_Address = MEMORY_ADDRESS;
	i2c_mem_address = (uint16_t)memoryAddress;

	status = HAL_I2C_Mem_Read(hi2c, device_Address, i2c_mem_address, I2C_MEMADD_SIZE_16BIT, dataBuffer, dataSize, 100);

	return status;
}

