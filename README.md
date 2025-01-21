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

</details>

#

<details>

<summary>

## String

</summary>

</details>

#

<details>

<summary>

## Utils

</summary>

</details>

</details>

#
