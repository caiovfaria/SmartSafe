# Smart Safe ESP32

Cofre didático com ESP32, interface web armazenada na memória flash, servo e telemetria no ThingSpeak. Inicia trancado, com servo a 90°; a abertura posiciona o servo em 0°.

## Hardware

- ESP32 com Wi-Fi e cabo USB.
- Servo com sinal no GPIO 18 e alimentação adequada ao modelo. Use fonte externa dimensionada para o servo e conecte seu GND ao GND da ESP32; não alimente o servo pelo pino de 3,3 V.
- Mecanismo de trava e conexões.

## Configuração e uso

1. Instale o suporte às placas ESP32 na Arduino IDE e as bibliotecas **ESP32Servo** e **ThingSpeak**. `WiFi` e `WebServer` acompanham o suporte ESP32.
2. Abra `SmartSafe.ino` dentro da pasta `SmartSafe` e selecione a placa e a porta USB correspondentes.
3. No início do código, configure `WIFI_NOME` (SSID), `WIFI_SENHA`, `CHANNEL_ID` (substitua `0` pelo ID numérico do canal), `WRITE_API_KEY` e `SENHA_COFRE`. Os valores publicados são placeholders; mantenha suas credenciais fora do GitHub.
4. No canal ThingSpeak, habilite os campos 1 a 4 conforme a tabela abaixo.
5. Envie o programa e abra o monitor serial em **115200 baud**. Na mesma rede Wi-Fi, acesse o endereço IP mostrado. Se a conexão falhar após 20 segundos, confira as configurações e reinicie a placa.

## Rotas

| Rota | Comportamento |
| --- | --- |
| `/` | Exibe a interface com senha e botões Abrir/Trancar. |
| `/abrir?senha=...` | Compara a senha; responde `OK` e abre ou `ERRADO` e incrementa tentativas incorretas. Sem o parâmetro `senha`, retorna HTTP 400 com `ERRO`. |
| `/trancar` | Move o servo para 90°, registra o evento e responde `OK`. |
| `/status` | Retorna `ABERTO` ou `TRANCADO`, conforme o estado comandado pelo programa. |

A interface consulta `/status` ao carregar e depois dos comandos. Não há sensor de posição para confirmar fisicamente a trava.

## ThingSpeak

| Campo | Valor |
| --- | --- |
| 1 | Estado: 1 aberto, 0 trancado. |
| 2 | Contador de tentativas com senha incorreta. |
| 3 | Contador de comandos de abertura com senha correta. |
| 4 | Evento: 0 inicialização, 1 abertura, 2 travamento, 3 senha incorreta. |

O envio ocorre quando existe evento pendente, com intervalo mínimo de 15 segundos após um envio bem-sucedido. A resposta 200 confirma o envio; em caso de falha, o programa tenta novamente no loop. Eventos próximos são consolidados: apenas o último código de evento é enviado, junto dos contadores atuais. Estado e contadores reiniciam ao desligar ou reiniciar a placa.

Este é um protótipo didático: usa HTTP, envia a senha na URL e não exige autenticação para trancar ou consultar o estado. Utilize em rede local confiável, sem expor a porta do ESP32 à internet.
