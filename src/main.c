#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

// -------------------------
// Pinos da ponte TB6612FNG
// -------------------------

// Motor A - roda esquerda
#define PWMA GPIO_NUM_32
#define AIN2 GPIO_NUM_33
#define AIN1 GPIO_NUM_25

// Motor B - roda direita
#define BIN1 GPIO_NUM_27
#define BIN2 GPIO_NUM_14
#define PWMB GPIO_NUM_13

// Habilita a ponte H
#define STBY GPIO_NUM_26

// -------------------------
// Configuracao do Wi-Fi
// -------------------------

#define NOME_WIFI  "ROBO_SUMO"
#define SENHA_WIFI "prototipo32"

// Se o celular parar de enviar comandos, o robo para sozinho.
#define TEMPO_LIMITE_MS 700

// Prototipos das funcoes.
void app_main(void);
void setup(void);
void loop(void);
void setup_gpio(void);
void setup_wifi(void);
void setup_servidor(void);
void frente(void);
void re(void);
void esquerda(void);
void direita(void);
void parar(void);

// Ultimo instante em que o celular enviou um comando.
volatile TickType_t ultimo_comando = 0;
volatile bool robo_em_movimento = false;

// Pagina que aparece no celular.
static const char PAGINA[] =
"<!DOCTYPE html>"
"<html lang='pt-BR'>"
"<head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Robo Sumo</title>"
"<style>"
"body{font-family:Arial;text-align:center;background:#202124;color:white}"
"button{width:120px;height:80px;margin:8px;font-size:22px;border:0;border-radius:12px}"
".andar{background:#35a853;color:white}.parar{background:#e53935;color:white}"
"</style>"
"</head>"
"<body>"
"<h1>Robo Sumo</h1>"
"<p>Segure um botao para movimentar.</p>"
"<div><button class='andar' data-cmd='f'>Frente</button></div>"
"<div>"
"<button class='andar' data-cmd='e'>Esquerda</button>"
"<button class='parar' id='parar'>Parar</button>"
"<button class='andar' data-cmd='d'>Direita</button>"
"</div>"
"<div><button class='andar' data-cmd='r'>Re</button></div>"
"<script>"
"let repeticao=null;"
"function enviar(c){fetch('/'+c,{cache:'no-store'}).catch(()=>{});}"
"function iniciar(c){finalizar();enviar(c);repeticao=setInterval(()=>enviar(c),200);}"
"function finalizar(){if(repeticao){clearInterval(repeticao);repeticao=null;}enviar('p');}"
"document.querySelectorAll('[data-cmd]').forEach(b=>{"
"b.onpointerdown=e=>{e.preventDefault();iniciar(b.dataset.cmd);};"
"b.onpointerup=finalizar;b.onpointercancel=finalizar;b.onpointerleave=finalizar;"
"});"
"document.getElementById('parar').onclick=finalizar;"
"window.onblur=finalizar;"
"</script>"
"</body>"
"</html>";

void setup_gpio(void)
{
    gpio_set_direction(AIN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(AIN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(PWMA, GPIO_MODE_OUTPUT);

    gpio_set_direction(BIN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(BIN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(PWMB, GPIO_MODE_OUTPUT);

    gpio_set_direction(STBY, GPIO_MODE_OUTPUT);
}

void parar(void)
{
    gpio_set_level(PWMA, 0);
    gpio_set_level(PWMB, 0);

    gpio_set_level(AIN1, 0);
    gpio_set_level(AIN2, 0);
    gpio_set_level(BIN1, 0);
    gpio_set_level(BIN2, 0);

    gpio_set_level(STBY, 0);

    robo_em_movimento = false;
    printf("PARAR\n");
}

void frente(void)
{
    gpio_set_level(STBY, 1);
    gpio_set_level(PWMA, 1);
    gpio_set_level(PWMB, 1);

    gpio_set_level(AIN1, 1);
    gpio_set_level(AIN2, 0);

    // O motor B fica montado do lado oposto do motor A.
    gpio_set_level(BIN1, 0);
    gpio_set_level(BIN2, 1);

    robo_em_movimento = true;
    ultimo_comando = xTaskGetTickCount();
    printf("FRENTE\n");
}

void re(void)
{
    gpio_set_level(STBY, 1);
    gpio_set_level(PWMA, 1);
    gpio_set_level(PWMB, 1);

    gpio_set_level(AIN1, 0);
    gpio_set_level(AIN2, 1);

    gpio_set_level(BIN1, 1);
    gpio_set_level(BIN2, 0);

    robo_em_movimento = true;
    ultimo_comando = xTaskGetTickCount();
    printf("RE\n");
}

void esquerda(void)
{
    gpio_set_level(STBY, 1);
    gpio_set_level(PWMA, 1);
    gpio_set_level(PWMB, 1);

    gpio_set_level(AIN1, 0);
    gpio_set_level(AIN2, 1);
    gpio_set_level(BIN1, 0);
    gpio_set_level(BIN2, 1);

    robo_em_movimento = true;
    ultimo_comando = xTaskGetTickCount();
    printf("ESQUERDA\n");
}

void direita(void)
{
    gpio_set_level(STBY, 1);
    gpio_set_level(PWMA, 1);
    gpio_set_level(PWMB, 1);

    gpio_set_level(AIN1, 1);
    gpio_set_level(AIN2, 0);
    gpio_set_level(BIN1, 1);
    gpio_set_level(BIN2, 0);

    robo_em_movimento = true;
    ultimo_comando = xTaskGetTickCount();
    printf("DIREITA\n");
}

static esp_err_t pagina_handler(httpd_req_t *requisicao)
{
    httpd_resp_set_type(requisicao, "text/html");
    return httpd_resp_send(requisicao, PAGINA, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t comando_handler(httpd_req_t *requisicao)
{
    if (strcmp(requisicao->uri, "/f") == 0)
        frente();
    else if (strcmp(requisicao->uri, "/r") == 0)
        re();
    else if (strcmp(requisicao->uri, "/e") == 0)
        esquerda();
    else if (strcmp(requisicao->uri, "/d") == 0)
        direita();
    else
        parar();

    return httpd_resp_sendstr(requisicao, "OK");
}

void setup_wifi(void)
{
    esp_err_t resultado = nvs_flash_init();

    if (resultado == ESP_ERR_NVS_NO_FREE_PAGES ||
        resultado == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t configuracao_inicial = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&configuracao_inicial);

    wifi_config_t configuracao_wifi = {
        .ap = {
            .ssid = NOME_WIFI,
            .password = SENHA_WIFI,
            .channel = 1,
            .max_connection = 1,
            .authmode = WIFI_AUTH_WPA2_PSK
        }
    };

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &configuracao_wifi);
    esp_wifi_start();

    printf("Wi-Fi iniciado\n");
    printf("Rede: %s\n", NOME_WIFI);
    printf("Senha: %s\n", SENHA_WIFI);
    printf("Endereco: http://192.168.4.1\n");
}

void setup_servidor(void)
{
    httpd_config_t configuracao = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t servidor = NULL;

    httpd_start(&servidor, &configuracao);

    httpd_uri_t pagina = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = pagina_handler
    };
    httpd_register_uri_handler(servidor, &pagina);

    const char *comandos[] = {"/f", "/r", "/e", "/d", "/p"};

    for (int i = 0; i < 5; i++)
    {
        httpd_uri_t comando = {
            .uri = comandos[i],
            .method = HTTP_GET,
            .handler = comando_handler
        };
        httpd_register_uri_handler(servidor, &comando);
    }
}

void setup(void)
{
    setup_gpio();
    parar();
    setup_wifi();
    setup_servidor();
    printf("setup() concluido\n");
}

void loop(void)
{
    TickType_t agora = xTaskGetTickCount();

    if (robo_em_movimento == true &&
        (agora - ultimo_comando) > pdMS_TO_TICKS(TEMPO_LIMITE_MS))
    {
        parar();
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
}

void app_main(void)
{
    setup();

    while (1)
    {
        loop();
    }
}
