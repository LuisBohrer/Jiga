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
| **APP_InitUarts** | `void` | `void` | Faz a inicialização das uarts de display, debug e modbus, bem como de seus ring buffers. |
| **APP_InitTimers** | `void` | `void` | Faz a inicialização do timer 6 (1 ms). |
| **APP_StartAdcReadDma** | `void` | <ul><li>`uint16_t* readsBuffer:` buffer onde as leituras são armazenadas <li>`reading_t rypeOfRead:` escolhe se a leitura é de tensão ou corrente | Inicia a leitura por DMA e seta a variável global que indica o tipo de leitura sendo feito. |
| **APP_UpdateReads** | `void` | `void` | Verifica se há novas leituras e, caso sim, as envia para o display. Também faz a requisição de uma nova leitura do outro tipo. |

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
| `NEXTION_Begin` | `void` | <ul><li>`UART_HandleTypeDef *displayUartAddress:` endereço da uart do display | Define o endereço da uart do display para as outras funções. |
| `NEXTION_SendCharMessage` | `void` | <ul><li>`const char* const message:` vetor de char a ser enviado para o display | Adiciona os bytes finais à mensagem e a envia. |
| `NEXTION_SendStringMessage` | `void` | <ul><li>`string *message:` string a ser enviada para o display | Adiciona os bytes finais à mensagem e a envia. |
| `NEXTION_SetComponentText` | `void` | <ul><li>`const string *component:` nome do componente que será alterado <li>`const string *newText:` texto que será escrito no componente | Faz a mensagem para alterar o texto de um componente e a envia. |
| `NEXTION_SetComponentIntValue` | `void` | <ul><li>`const string *component:` nome do componente que será alterado <li>`int32_t newValue:` valor que será escrito no componente | Faz a mensagem para alterar um valor inteiro de um componente e a envia. |
| `NEXTION_SetComponentFloatValue` | `void` | <ul><li>`const string *component:` nome do componente que será alterado <li>`float newValue:` valor que será escrito no componente <li>`uint32_t decimalSpaces:` número de casas decimais desejadas | Faz a mensagem para alterar um valor float de um componente e a envia. |
| `NEXTION_SetGlobalVariableValue` | `void` | <ul><li>`const string *variable:` nome da variável que será alterada <li>`int32_t value:` valor que será escrito na variável | Faz a mensagem para alterar o valor de uma variável global e a envia. |
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
| `RB_Init` | `void` | <ul><li>`ringBuffer_t *ringBuffer:` endereço do buffer circular | Inicializa os componentes do struct. |
| `RB_PutByte` | `void` | <ul><li>`ringBuffer_t *ringBuffer:` endereço do buffer circular <li>`uint8_t byte:` byte para a adicionar | Adicona um byte no final da fila, se tiver espaço. |
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
| `STRING_Init` | `void` | <ul><li>`string *self:` endereço da string | Inicializa os componentes da string. |
| `STRING_GetBuffer` | `uint8_t*:` endereço do buffer da string | <ul><li>`string *self:` endereço da string | Retorna o buffer da string passada. |
| `STRING_GetLength` | `uint16_t:` tamnanho da mensagem armazenada na string | <ul><li>`string *self:` endereço da string | Retorna o tamanho da mensagem armazenada na string. |
| `STRING_AddChar` | `void` | <ul><li>`string *self:` endereço da string <li>`char character:` caractere para adicionar na string | Adiciona um char ao final da string. |
| `STRING_AddInt` | `void` | <ul><li>`string *self:` endereço da string <li>`uint32_t number:` valor inteiro para adicionar na string | Adiciona um int ao final da string. |
| `STRING_AddFloat` | `void` | <ul><li>`string *self:` endereço da string <li>`float number:` valor decimal para adicionar na string <li>`uint32_t decimalSpaces:` quantidade de casas decimais desejadas <li>`char separator:` separador das casas decimais | Adiciona um float ao final da string. |
| `STRING_AddCharString` | `void` | <ul><li>`string *self:` endereço da string <li>`const char* const inputCharString:` buffer de char terminado em '\0' | Adiciona um buffer de char terminado em '\0' ao final da string. |
| `STRING_AddString` | `void` | <ul><li>`string *self:` endereço da string <li>`const string *inputString:` string para adicionar | Adiciona uma outra string ao final da string. |
| `STRING_CopyString` | `void` | <ul><li>`const string *copyFrom:` string de origem, não é alterada <li>`string *copyTo:` string de destino | Copia a mensagem de uma string em outra. |
| `STRING_Clear` | `void` | <ul><li>`string *self:` endereço da string | Reinicializa os componentes da string. |
| `STRING_IsDigit` | `uint8_t:` 0 caso não for um número, 1 caso for | <ul><li>`char inputchar:` char para verificação | Verifica se o char passado é um número ou não. |
| `STRING_IsPrintable` | `uint8_t:` 0 caso não for imprimível, 1 caso for | <ul><li>`char inputchar:` char para verificação | Verifica se o char passado é imprimível ou não. |
| `STRING_CharStringToString` | `void` | <ul><li>`const char* const inputCharString:` buffer de char de origem  <li>`string *outputString:` string de destino | Converte um buffer de char em uma string. |
| `STRING_StringToCharString` | `void` | <ul><li>`const string *inputString:` string de origem <li>`char *outputCharString:` buffer de char de destino | Converte uma string em um buffer de char. |
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
| `UTILS_CpuSleep` | `void` | `void` | Coloca o microcontrolador no modo sleep. |
| `UTILS_Map` | `float` | <ul><li>`float value:` valor para converter <li>`float fromMin:` limite inferior do valor original <li>`float fromMax:` limite superior do valor original <li>`float toMin:` limite inferior da conversão desejada <li>`float toMax:` limite superior da conversão desejada | Faz a conversão de um valor para outra base. |

</details>

</details>

#
