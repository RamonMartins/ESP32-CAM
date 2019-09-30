/*********
  Rui Santos
   - Detalhes completo do Projeto em https://RandomNerdTutorials.com/esp32-cam-take-photo-save-microsd-card

  Ramon Martins Ferreira
  - Alteração do código para ter função de uma TimeLapse.

  IMPORTANTE!!!
   - Selecione a Placa "ESP32 Wrover Module". (Select Board "ESP32 Wrover Module")
   - Selecione "Huge APP (3MB No OTA)" em "Partion Scheme". (Select the Partion Scheme "Huge APP (3MB No OTA)")
   - O pino "GPIO 0" deve estar conectado ao GND e em seguida clicar em "Reset" para entrar em Bootloader 
   para carregar o programa. (GPIO 0 must be connected to GND to upload a sketch. After connecting GPIO 0 to GND,
   press the ESP32-CAM on-board RESET button to put your board in flashing mode).

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"
#include "FS.h"                //SD Card ESP32
#include "SD_MMC.h"            //SD Card ESP32
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "dl_lib.h"
#include "driver/rtc_io.h"     //Para fazer o led desligar
#include <EEPROM.h>            //Leitura e Escrita na memória flash

#define EEPROM_SIZE 1

//Definição de pinos para a CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

//#define intervalo 5 * 60 * 1000 //Intervalo entre as imagens "padrão: 5min"
#define intervalo 2000            //Intervalo entre as imagens "padrão: 2seg"
#define timelapse 10              //Define quantas fotos serão tiradas 0-256
unsigned long lastMillis = 0;
int pictureNumber = 0;

camera_config_t config; //Define a palavra config para iniciar as configurações da câmera

void iniciar_camera( void );
void tirar_foto( void );
void limpar_eeprom( void );

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  //Serial.setDebugOutput(true);
  //Serial.println();

  limpar_eeprom(); //Chama a função para limpar a EEPROM
  iniciar_camera(); //Chama a função para iniciar a câmera

  do {
    if ( (millis() - lastMillis) > intervalo ) {
      tirar_foto();
      lastMillis = millis(); //Atualiza o 'lastMillis' com a última hora acionada
    }
  } while (pictureNumber < timelapse);

  //Definição do pino do Led Flash
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  rtc_gpio_hold_en(GPIO_NUM_4); //Desliga o Led Flash

  delay(2000);
  Serial.println("ESP32 indo dormir...");
  delay(1000);
  esp_deep_sleep_start(); //Coloca o ESP32 para dormir
}

void loop() {
  // Nada aqui
}

void limpar_eeprom( void ) {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0 ; i <= 256 ; i++) { //Limpar a EEPROM
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}

void iniciar_camera( void ) {
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("O início da câmera falhou com erro 0x%x", err);
    return;
  }
}

void tirar_foto( void ) {
  if (!SD_MMC.begin()) {
    Serial.println("Falha na montagem do cartão SD");
    return;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("Nenhum cartão SD conectado");
    return;
  }

  camera_fb_t * fb = NULL;

  // Take Picture with Camera
  fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Falha na captura da câmera");
    return;
  }

  pictureNumber = EEPROM.read(0) + 1;

  //Path where new picture will be saved in SD Card
  String path = "/imagem" + String(pictureNumber) + ".jpg";

  fs::FS &fs = SD_MMC;
  Serial.printf("Nome do arquivo de imagem: %s\n", path.c_str());

  File file = fs.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Falha ao abrir o arquivo no modo de gravação");
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Arquivo salvo no caminho: %s\n", path.c_str());
    EEPROM.write(0, pictureNumber);
    EEPROM.commit();
  }
  file.close();
  esp_camera_fb_return(fb);
}
