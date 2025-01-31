# Giga

Repositório destinado ao versionamento do firmware da placa e display da Giga.

O objetivo da placa é fazer a coleta de 10 leituras de tensão e corrente. Ela faz isso a partir de 10 canais de ADC e um interruptor que seleciona se as leituras serão de tensão
ou corrente.

Ela também pode comunicar com a Coletora e com Modbus (ainda não implementados).

#

<details>

<summary>

## Firmware

</summary>

As implementações estão concentradas no arquivo app.c. Os outros arquivos servem apenas como bibliotecas.

Em resumo, o programa faz a leitura dos canais ADC por DMA, envia os resultados para o display, então troca o tipo de leitura (tensão ou corrente) e repete o processo a cada 1 ms.
Além disso, ele também recebe as mensagens de erro do display e as classifica (atualmente apenas em "buffer overflow" ou "invalid variable"); ainda não foi feito nenhum
tratamento para esses erros.

#

<details>

<summary>

### App

</summary>

Faz a inicialização e tratamento do programa. Idealmente, não é incluido em outros módulos, já que faz a junção de todos eles.

Também define as callbacks das interrupções de ADC, timer e uarts.

### Enums

| Enum | Componentes | Descrição |
| --- | --- | --- |
| `reading_t` | <ul><li>`READ_VOLTAGE` <li>`READ_CURRENT` | Tipos de leitura do ADC. |

### Funções

| Função | Retorno | Parâmetros | Descrição |
| --- | --- | --- | --- |
| **APP_InitUarts** | <div align="center">`void`</div> | <div align="center">`void`</div> | Faz a inicialização das uarts de display, debug e modbus, bem como de seus ring buffers. |
| **APP_InitTimers** | <div align="center">`void`</div> | <div align="center">`void`</div> | Faz a inicialização do timer 6 (1 ms). |
| **APP_StartAdcReadDma** | <div align="center">`void`</div> | <ul><li>`uint16_t* readsBuffer:` buffer onde as leituras são armazenadas <li>`reading_t rypeOfRead:` escolhe se a leitura é de tensão ou corrente | Inicia a leitura por DMA e seta a variável global que indica o tipo de leitura sendo feito. |
| **APP_UpdateReads** | <div align="center">`void`</div> | <div align="center">`void`</div> | Verifica se há novas leituras e, caso sim, as envia para o display. Também faz a requisição de uma nova leitura do outro tipo. |

### Callbacks das Interrupções

| Função | Origem |Descrição |
| --- | --- | --- |
| **HAL_ADC_ConvCpltCallback** | ADC | Seta a flag que indica que há uma nova leitura para ser enviada. |
| **HAL_UART_RxCpltCallback** | UART | Guarda o byte recebido e reseta a interrupção. |
| **HAL_TIM_PeriodElapsedCallback** | Timer | Aumenta a contagem dos contadores de tempo. |

</details>

#

<details>

<summary>

## Comm

</summary>

Cuida da comunicação de debug (coletora) da placa. Ainda precisa definir uma lista de códigos comuns entre as placas.

### Enums

| Nome | Constantes | Descrição |
| --- | --- | --- |
| `debugRequest_t` | <ul><li>`INCOMPLETE_REQUEST` <li>`INVALID_REQUEST` <li>`SEND_VOLTAGE_READS` <li>`SEND_CURRENT_READS` <li>`SEND_ALL_READS` <li>`SET_MODBUS_CONFIG` <li>`CHANGE_SCALE` <li>`LOGS` | Lista de requisições que podem ser feitas à placa. Provisória, passível de mudança. |
| `debugAckSignals_t` | <ul><li>`NACK` = 1 <li>`ACK_VOLTAGE_READS` <li>`ACK_CURRENT_READS` <li>`ACK_ALL_READS` <li>`ACK_MODBUS_CONFIG` <li>`ACK_CHANGE_SCALE` <li>`ACK_LOGS` = 22 | Lista de sinais de resposta às respectivas requisições. Provisória, passível de mudança. |

### Funções

| Função | Retorno | Parâmetros | Descrição |
| --- | --- | --- | --- |
| `COMM_Begin` | <div align="center">`void`</div> | <ul><li>`UART_HandleTypeDef *huart:` endereço da uart de debug | Seta a uart passada para o envio das mensagens. |
| `COMM_SendStartPacket` | <div align="center">`void`</div> | <div align="center">`void`</div> | Envia os bytes que sinalizam o inínio da mensagem. Atualmente `##` (`0x23` `0x23`). |
| `COMM_SendEndPacket` | <div align="center">`void`</div> | <div align="center">`void`</div> | Envia os bytes que sinalizam o fim da mensagem. Atualmente `@` (`0x40`). |
| `COMM_SendAck` | <div align="center">`void`</div> | <ul><li>`debugAckSignals_t ack:` sinal de reconhecimento a enviar | Envia o byte de resposta da requisição. |
| `COMM_SendValues8Bits` | <div align="center">`void`</div> | <ul><li>`uint8_t *values:` buffer com os valores <li>`uint16_t length:` quantidade de valores | Envia valores de 8 bits pela uart de debug. |
| `COMM_SendValues16Bits` | <div align="center">`void`</div> | <ul><li>`uint16_t *values:` buffer com os valores <li>`uint16_t length:` quantidade de valores | Envia valores de 16 bits pela uart de debug. |
| `COMM_SendValues32Bits` | <div align="center">`void`</div> | <ul><li>`uint32_t *values:` buffer com os valores <li>`uint16_t length:` quantidade de valores | Envia valores de 32 bits pela uart de debug. |
| `COMM_SendString` | <div align="center">`void`</div> | <ul><li>`string *message:` endereço da string | Envia uma string pela uart de debug. |
| `COMM_SendChar` | <div align="center">`void`</div> | <ul><li>`uint8_t *buffer:` buffer com caracteres <li>`uint16_t length:` quantidade de caracteres | Envia caracteres pela usart de debug. |
| `COMM_TreatResponse` | `debugRequest_t:` qual requisição feita | <ul><li>`string *message:` mensagem de debug | Faz a interpretação de uma mensagem chegada pela uart de debug. |

</details>

#

<details>
  
<summary>
  
## Modbus

</summary>

Faz a comunicação por modbus. Trata do envio e verificação de mensagens.

### Enums

Referência: [embarcados.com.br/protocolo-modbus/](https://embarcados.com.br/protocolo-modbus/)
| Nome | Constantes | Descrição |
|---|---|---|
| `modbusStates_t` | <ul><li>`MODBUS_STARTING` = 0 <li>`MODBUS_IDLE` <li>`MODBUS_SENDING` <li>`MODBUS_RECEIVING` | Estados do modbus |
| `modbusOpcodes_t` | <ul><li>`READ_COILS` = 0x01  <li>`READ_DISCRETE_INPUTS`  <li>`READ_HOLDING_REGISTERS`  <li>`READ_INPUT_REGISTERS`  <li>`WRITE_SINGLE_COIL`  <li>`WRITE_SINGLE_HOLDING_REGISTER`  <li>`READ_EXCEPTION_STATUS`  <li>`DIAGNOSTICS`  <li>`GET_COMM_EVENT_COUNTER` = 0x0B  <li>`GET_COMM_EVENT_LOG`  <li>`WRITE_MULTIPLE_COILS` = 0x0F  <li>`WRITE_MULTIPLE_HOLDING_REGISTERS`  <li>`REPORT_SLAVE_ID`  <li>`READ_FILE_RECORD` = 0x14  <li>`WRITE_FILE_RECORD`  <li>`MASK_WRITE_REGISTER`  <li>`READ_WRITE_MULTIPLE_REGISTER`  <li>`READ_FIFO_QUEUE`  <li>`ENCAPSULATED_INTERFACE_TRANSPORT` = 0x2B | Opcodes disponíveis no protocolo modbus |
| `modbusError_t` | <ul><li>`MODBUS_NO_ERROR` = 0 <li>`MODBUS_INVALID_OPCODE` <li>`MODBUS_RESPONSE_ERROR` <li>`MODBUS_INVALID_REGISTER_ADDRESS` <li>`MODBUS_TIMEOUT` <li>`MODBUS_INVCORRECT_ID` <li>`MODBUS_INCORRECT_OPCODE` <li>`MODBUS_INCORRECT_FIRST_REGISTER` <li>`MODBUS_INCORRECT_QTT_REGISTERS` <li>`MODBUS_INCORRECT_CRC` | Erros de comunicação.<br>`RESPONSE_ERROR` se trata de uma resposta de erro vinda do equipamento secundário, ela indica que a requisição feita pelo modbus é válida, mas não é aplicável para aquele equipamento.<br>`INCORRECT` indica que uma parte da resposta não estava de acordo com o esperado pela coletora.<br>`INVALID` indica que o tratamento de tal requisição ou resposta ainda não foi implementado ou não existe.

### Structs

| Nome | Componentes | Descrição |
|---|---|---|
| `modbusHandler_t` | <ul><li>`GPIO_TypeDef *sendReceivePort:` porta do pino que controla se o modbus envia ou recebe mensagens <li>`uint16_t sendReceivePin:` número do pino que controla se o modbus envia ou recebe mensagens <li>`UART_HandleTypeDef *modbusUart:` endereço da uart em que o modbus está conectado <li>`uint8_t deviceAddress:` endereço do dispositivo <li>`modbusStates_t modbusState:` estado do modbus <li>`uint8_t payloadBuffer[MODBUS_BUFFER_SIZE]:` buffer que armazena os bytes enviados pelo modbus <li>`uint8_t payloadIndex:` índice do buffer do modbus <li>`uint8_t requestId:` endereço de destino da última mensagem enviada <li>`modbusOpcodes_t opcode:` opcode da última mensagem enviada <li>`uint16_t firstRegister:` primeiro endereço da última mensagem enviada <li>`uint16_t qttRegisters:` quantidade de registros da última mensagem enviada <li>`uint32_t calculatedCRC:` CRC calculado a partir do `payloadBuffer` | Handler do modbus. Controla as mensagens enviadas e faz o tratamento das mensagens recebidas. |

### Funções

| Função | Retorno | Parâmetros | Descrição |
|---|---|---|---|
| `MODBUS_Begin` | <div align="center">`void`</div> | <ul><li>`modbusHandler_t *modbusHandler:` endereço do handler do modbus <li>`GPIO_TypeDef *sendReceivePort:` porta do pino que controla se o modbus envia ou recebe mensagens <li>`uint16_t sendReceivePin:` número do pino que controla se o modbus envia ou recebe mensagens <li>`UART_HandleTypeDef *huart:` endereço da uart em que o modbus está conectado <li>`uint8_t deviceAddress:` endereço do dispositivo | Inicializa os componentes do handler do modbus. |
| `MODBUS_SetSendReceive` | <div align="center">`void`</div> | <ul><li>`modbusHandler_t *modbusHandler:` endereço do handler do modbus <li>`sendOrReceive_t sendOrReceive:` estado para setar o modbus | Configura o pino de envio e recebimento do modbus para enviar ou receber dados. |
| `MODBUS_GetSendReceive` | `sendOrReceive_t:` `MODBUS_SET_RECEIVE` ou `MODBUS_SET_SEND` | <ul><li>`modbusHandler_t *modbusHandler:` endereço do handler do modbus |  informa se o modbus está enviando ou recebendo mensagens. |
| `MODBUS_VerifyMessage` | `modbusError_t:` tipo de erro encontrado | <ul><li>`uint8_t expectedSecondaryAddress:` endereço de origem experado <li>`uint8_t expectedOpcode:` opcode esperado <li>`uint16_t expectedFirstAdress:` primeiro endereço esperado <li>`uint16_t expectedNumberOfData:` quantidade de dados esperados <li>`uint8_t *messageBuffer:` buffer com a mensagem <li>`uint32_t messageLength:` tamanho da mensagem | Informa se a mensagem passada é válida e, se não for, acusa o motivo do erro. |
| `MODBUS_VerifyWithHandler` | `modbusError_t:` tipo de erro encontrado | <ul><li>`modbusHandler_t *modbusHandler:` endereço do handler do modbus <li>`uint8_t *messageBuffer:` buffer com a mensagem <li>`uint32_t messageLength:` tamanho da mensagem | Informa se a mensagem passada é válida de acordo com a última mensagem enviada e, se não for, acusa o motivo do erro. |
| `MODBUS_VerifyCrc` | `modbusError_t:` tipo de erro encontrado | <ul><li>`uint8_t *message:` buffer com a mensagem <li>`uint32_t length:` tamanho da mensagem | Informa se o crc da mensagem passada é válido. |
| `MODBUS_ReadCoils` | <div align="center">`void`</div> | <ul><li>`modbusHandler_t *modbusHandler:` endereço do handler do modbus <li>`uint8_t secondaryAddress:` endereço do dispositivo de destino <li>`uint16_t firstCoilAddress:` endereço da primeira bobina desejada <li>`uint16_t numberOfCoils:` número de bobinas para ler | Faz uma requisição de leitura de bobinas. |
| `MODBUS_ReadInputRegisters` | <div align="center">`void`</div> | <ul><li>`modbusHandler_t *modbusHandler:` endereço do handler do modbus <li>`uint8_t secondaryAddress:` endereço do dispositivo de destino <li>`uint16_t firstRegisterAddress:` endereço do primeiro input register desejado <li>`uint16_t numberOfRegisters:` número de input registers para ler | Faz uma requisição de leitura de input registers. |
| `MODBUS_ReadSingleHoldingRegister` | <div align="center">`void`</div> | <ul><li>`modbusHandler_t *modbusHandler:` endereço do handler do modbus <li>`uint8_t secondaryAddress:` endereço do dispositivo de destino <li>`uint16_t firstRegisterAddress:` endereço do primeiro registrador desejado | Faz uma requisição de leitura de um registrador. |
| `MODBUS_ReadMultipleHoldingRegisters` | <div align="center">`void`</div> | <ul><li>`modbusHandler_t *modbusHandler:` endereço do handler do modbus <li>`uint8_t secondaryAddress:` endereço do dispositivo de destino <li>`uint16_t firstRegisterAddress:` endereço do primeiro registrador desejado <li>`uint16_t numberOfRegisters:` número de registradores para ler <li>`registerBytes_t sizeOfRegisterBytes:` tamanho em bytes do registrador | Faz uma requisição de leitura de multiplos registradores. |
| `MODBUS_WriteSingleCoil` | <div align="center">`void`</div> | <ul><li>`modbusHandler_t *modbusHandler:` endereço do handler do modbus <li>`uint8_t secondaryAddress:` endereço do dispositivo de destino <li>`uint16_t coilAddress:` endereço da bobina desejada <li>`uint8_t valueToWrite:` valor para escrever na bobina | Faz uma requisição de escrita de uma bobina. |
| `MODBUS_WriteMultipleCoils` | <div align="center">`void`</div> | <ul><li>`modbusHandler_t *modbusHandler:` endereço do handler do modbus <li>`uint8_t secondaryAddress:` endereço do dispositivo de destino <li>`uint16_t firstCoilAddress:` endereço da primeira bobina desejada <li>`uint16_t numberOfCoils:` número de bobinas para escrever <li>`uint8_t *valuesToWrite:` endereço do buffer com os valores para ser escritos | Faz uma requisição de escrita de multiplas bobinas. |
| `MODBUS_WriteSingleHoldingRegister` | <div align="center">`void`</div> | <ul><li>`modbusHandler_t *modbusHandler:` endereço do handler do modbus <li>`uint8_t secondaryAddress:` endereço do dispositivo de destino <li>`uint16_t firstRegisterAddress:` endereço do primeiro registrador desejado <li>`uint32_t valueToWrite:` valor para escrever no registrador <li>`registerBytes_t sizeOfRegisterBytes:` tamanho do registrador | Faz uma requisição de escrita de um registrador. |
| `MODBUS_WriteMultipleHoldingRegisters` | <div align="center">`void`</div> | <ul><li>`modbusHandler_t *modbusHandler:` endereço do handler do modbus <li>`uint8_t secondaryAddress:` endereço do dispositivo de destino <li>`uint16_t firstRegisterAddress:` endereço do primeiro registrador desejado <li>`uint16_t numberOfRegisters:` número de registradores para ler <li>`registerBytes_t sizeOfRegisterBytes:` tamanho em bytes dos registradores <li>`uint8_t *valuesToWrite:` buffer com os valores que serão escritos nos registradores | Faz uma requisição de escrita de multiplos registradores. |

</details>

#

<details>

<summary>

## Nextion

</summary>

Os arquivos nextionComponents guardam os nomes dos componentes do display qu serão alterados pelo programa, eles servem para fazer um interfaceamento melhor no código.

As funções dessa seção visam facilitar a montagem das mensagens de envio ao display, adicionando os sufixos necessários dependendo do tipo de mensagem que se deseja enviar.

**Obs:** para usar a biblioteca, é necessário primeiro usar a função `NEXTION_Begin` para definir em qual uart o display está conectado.

### Enums

| Enum | Componentes | Descrição |
| --- | --- | --- |
| `displayResponses_t` | <ul><li>`NO_MESSAGE` <li>`INCOMPLETE_MESSAGE` <li>`ERROR_INVALID_VARIABLE` <li>`ERROR_BUFFER_OVERFLOW` <li>`VALID_MESSAGE` | Classificações das mensagens do display. |

### Funções

| Função | Retorno | Parâmetros | Descrição |
| --- | --- | --- | --- |
| `NEXTION_Begin` | <div align="center">`void`</div> | <ul><li>`UART_HandleTypeDef *displayUartAddress:` endereço da uart do display | Define o endereço da uart do display para as outras funções. |
| `NEXTION_SendCharMessage` | <div align="center">`void`</div> | <ul><li>`const char* const message:` vetor de char a ser enviado para o display | Adiciona os bytes finais à mensagem e a envia. |
| `NEXTION_SendStringMessage` | <div align="center">`void`</div> | <ul><li>`string *message:` string a ser enviada para o display | Adiciona os bytes finais à mensagem e a envia. |
| `NEXTION_SetComponentText` | <div align="center">`void`</div> | <ul><li>`const string *component:` nome do componente que será alterado <li>`const string *newText:` texto que será escrito no componente | Faz a mensagem para alterar o texto de um componente e a envia. |
| `NEXTION_SetComponentIntValue` | <div align="center">`void`</div> | <ul><li>`const string *component:` nome do componente que será alterado <li>`int32_t newValue:` valor que será escrito no componente | Faz a mensagem para alterar um valor inteiro de um componente e a envia. |
| `NEXTION_SetComponentFloatValue` | <div align="center">`void`</div> | <ul><li>`const string *component:` nome do componente que será alterado <li>`float newValue:` valor que será escrito no componente <li>`uint32_t decimalSpaces:` número de casas decimais desejadas | Faz a mensagem para alterar um valor float de um componente e a envia. |
| `NEXTION_SetGlobalVariableValue` | <div align="center">`void`</div> | <ul><li>`const string *variable:` nome da variável que será alterada <li>`int32_t value:` valor que será escrito na variável | Faz a mensagem para alterar o valor de uma variável global e a envia. |
| `NEXTION_TreatMessage` | `displayResponses_t:` classificação da última mensagem do display | <ul><li>`ringBuffer_t *buffer:` buffer contendo bytes vindo do display <li>`string *message:` mensagem analisada | Faz a interpretação das mensagens do display e as classifica. |

</details>

#

<details>

<summary>

## Ring Buffer

</summary>

Faz o tratamento dos buffers circulares, que funcionam como filas.

### Structs

| Struct | Componentes | Descrição |
| --- | --- | --- |
| ringBuffer_t | <ul><li>`uint8_t buffer[RING_BUFFER_DEFAULT_SIZE]:` buffer onde são guardados os bytes, o tamanho default é 1000 <li>`uint16_t first:` indice do primeiro da fila <li>`last:` indice do último da fila <li>`numberOfBytes:` quantidade de bytes na fila | Buffer circular. |

### Funções

| Função | Retorno | Parâmetros | Descrição |
| --- | --- | --- | --- |
| `RB_Init` | <div align="center">`void`</div> | <ul><li>`ringBuffer_t *ringBuffer:` endereço do buffer circular | Inicializa os componentes do struct. |
| `RB_PutByte` | <div align="center">`void`</div> | <ul><li>`ringBuffer_t *ringBuffer:` endereço do buffer circular <li>`uint8_t byte:` byte para a adicionar | Adicona um byte no final da fila, se tiver espaço. |
| `RB_GetByte` | `uint8_t:` byte no início da fila | <ul><li>`ringBuffer_t *ringBuffer:` endereço do buffer circular | Retorna o byte no início da fila, caso a fila estiver vazia, retorna 0. |
| `RB_IsEmpty` | `uint8_t:` 0 se a fila não estiver vazia, 1 se estiver | <ul><li>`ringBuffer_t *ringBuffer:` endereço do buffer circular | Verifica se a fila está vazia e retorna verdadeiro caso estiver. |
| `RB_IsFull` | `uint8_t:` 0 se a fila não estiver cheia, 1 se estiver | <ul><li>`ringBuffer_t *ringBuffer:` endereço do buffer circular | Verifica se a fila está cheia e retorna verdadeiro caso estiver. |
| `RB_GetNumberOfBytes` | `uint16_t:` quantidade de bytes na fila | <ul><li>`ringBuffer_t *ringBuffer`:` endereço do buffer circular | Retorna o número de bytes dentro da fila. |

</details>

#

<details>

<summary>

## String

</summary>

Trata o tipo "string" para facilitar a construção e envio de mensagens por uart.

### Structs

| Struct | Componentes | Descrição |
| --- | --- | --- |
| `string` | <ul><li>`uint8_t buffer[BUFFER_SIZE]:` buffer onde a string é armazenada; o tamanho default é 100 <li>`uint16_t length:` tamanho da string armazenada | Armazenamento de uma mensagem e seu tamanho. |

### Funções

| Função | Retorno | Parâmetros | Descrição |
| --- | --- | --- | --- |
| `STRING_Init` | <div align="center">`void`</div> | <ul><li>`string *self:` endereço da string | Inicializa os componentes da string. |
| `STRING_GetBuffer` | `uint8_t*:` endereço do buffer da string | <ul><li>`string *self:` endereço da string | Retorna o buffer da string passada. |
| `STRING_GetLength` | `uint16_t:` tamnanho da mensagem armazenada na string | <ul><li>`string *self:` endereço da string | Retorna o tamanho da mensagem armazenada na string. |
| `STRING_AddChar` | <div align="center">`void`</div> | <ul><li>`string *self:` endereço da string <li>`char character:` caractere para adicionar na string | Adiciona um char ao final da string. |
| `STRING_AddInt` | <div align="center">`void`</div> | <ul><li>`string *self:` endereço da string <li>`uint32_t number:` valor inteiro para adicionar na string | Adiciona um int ao final da string. |
| `STRING_AddFloat` | <div align="center">`void`</div> | <ul><li>`string *self:` endereço da string <li>`float number:` valor decimal para adicionar na string <li>`uint32_t decimalSpaces:` quantidade de casas decimais desejadas <li>`char separator:` separador das casas decimais | Adiciona um float ao final da string. |
| `STRING_AddCharString` | <div align="center">`void`</div> | <ul><li>`string *self:` endereço da string <li>`const char* const inputCharString:` buffer de char terminado em '\0' | Adiciona um buffer de char terminado em '\0' ao final da string. |
| `STRING_AddString` | <div align="center">`void`</div> | <ul><li>`string *self:` endereço da string <li>`const string *inputString:` string para adicionar | Adiciona uma outra string ao final da string. |
| `STRING_CopyString` | <div align="center">`void`</div> | <ul><li>`const string *copyFrom:` string de origem, não é alterada <li>`string *copyTo:` string de destino | Copia a mensagem de uma string em outra. |
| `STRING_Clear` | <div align="center">`void`</div> | <ul><li>`string *self:` endereço da string | Reinicializa os componentes da string. |
| `STRING_IsDigit` | `uint8_t:` 0 caso não for um número, 1 caso for | <ul><li>`char inputchar:` char para verificação | Verifica se o char passado é um número ou não. |
| `STRING_IsPrintable` | `uint8_t:` 0 caso não for imprimível, 1 caso for | <ul><li>`char inputchar:` char para verificação | Verifica se o char passado é imprimível ou não. |
| `STRING_CharStringToString` | <div align="center">`void`</div> | <ul><li>`const char* const inputCharString:` buffer de char de origem  <li>`string *outputString:` string de destino | Converte um buffer de char em uma string. |
| `STRING_StringToCharString` | <div align="center">`void`</div> | <ul><li>`const string *inputString:` string de origem <li>`char *outputCharString:` buffer de char de destino | Converte uma string em um buffer de char. |
| `STRING_StringToInt` | `int32_t:` valor armazenado na string | <ul><li>`const string *inputString:` string com o int armazenado | Retorna um valor inteiro armazenado dentro da string. |
| `STRING_StringToFloat` | `float:` valor armazenado na string | <ul><li>`const string *inputString:` string com o float armazenado <li>`char separator:` separador das casas decimais | Retorna um valor float armazenado dentro da string. |
| `STRING_CompareStrings` | `uint8_t:` 0 se as seções foram diferentes, 1 se forem iguais | <ul><li>`const string *string1:` primeira string <li>`const string *string2:` segunda string `uint16_t length:` tamanho da seção para comparar | Compara as seções de duas strings a partir do seu início. |
| `STRING_CompareStringsRev` | `uint8_t:` 0 se as seções foram diferentes, 1 se forem iguais | <ul><li>`const string *string1:` primeira string <li>`const string *string2:` segunda string `uint16_t length:` tamanho da seção para comparar | Compara as seções de duas strings a partir do seu fim. |
| `STRING_GetChar` | `uint8_t:` char armazenado na posição desejada | <ul><li>`const string *inputString:` string de origem <li>`uint16_t index:` indice do char desejado | Retorna um char armazenado dentro da string. |


</details>

#

<details>

<summary>

## Utils

</summary>

Funções utilitárias.

### Funções

| Função | Retorno | Parâmetros | Descrição |
| --- | --- | --- | --- |
| `UTILS_CpuSleep` | <div align="center">`void`</div> | <div align="center">`void`</div> | Coloca o microcontrolador no modo sleep. |
| `UTILS_Map` | `float:` valor convertido | <ul><li>`float value:` valor para converter <li>`float fromMin:` limite inferior do valor original <li>`float fromMax:` limite superior do valor original <li>`float toMin:` limite inferior da conversão desejada <li>`float toMax:` limite superior da conversão desejada | Faz a conversão de um valor para outra base. |

</details>

</details>

#
