# Robô controlado por Wi-Fi com ESP32

Projeto acadêmico de um robô com duas rodas controlado pelo navegador do celular. O ESP32 cria a rede Wi-Fi `ROBO_SUMO`, hospeda a página de controle e envia os sinais de direção para a ponte H TB6612FNG.

## Estado atual

- Rede Wi-Fi e página de controle funcionando.
- Comandos de frente, ré, esquerda, direita e parada recebidos pelo ESP32.
- Parada automática após 700 ms sem novo comando.
- Acionamento dos motores pela ponte H em fase de testes.

## Componentes

- ESP32 NodeMCU ESP-32S.
- Ponte H TB6612FNG.
- Dois motores DC com rodas.
- Suporte para baterias 18650.
- Protoboard.
- Fios jumper.

## Ligações entre ESP32 e TB6612FNG

| TB6612FNG | ESP32 | Função |
|---|---|---|
| PWMA | GPIO 32 | Velocidade do motor A |
| AIN2 | GPIO 33 | Direção do motor A |
| AIN1 | GPIO 25 | Direção do motor A |
| STBY | GPIO 26 | Habilita a ponte H |
| BIN1 | GPIO 27 | Direção do motor B |
| BIN2 | GPIO 14 | Direção do motor B |
| PWMB | GPIO 13 | Velocidade do motor B |
| VCC | 3V3 | Alimentação da lógica |
| GND | GND | Terra comum |
| VM | Positivo da bateria | Alimentação dos motores |

O motor esquerdo usa `AO1` e `AO2`. O motor direito usa `BO1` e `BO2`. O negativo da bateria, o GND da ponte e o GND do ESP32 precisam compartilhar a mesma referência.

