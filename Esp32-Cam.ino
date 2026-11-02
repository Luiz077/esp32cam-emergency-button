#include <WiFi.h>
  #include <WiFiClientSecure.h>
  #include <UniversalTelegramBot.h>
  #include <ArduinoJson.h>
  #include "esp_camera.h"
  #include <SD_MMC.h>
  #include <FS.h>

  #define PWDN_GPIO_NUM 32
  #define RESET_GPIO_NUM -1
  #define XCLK_GPIO_NUM 0
  #define SIOD_GPIO_NUM 26
  #define SIOC_GPIO_NUM 27

  #define Y9_GPIO_NUM 35
  #define Y8_GPIO_NUM 34
  #define Y7_GPIO_NUM 39
  #define Y6_GPIO_NUM 36
  #define Y5_GPIO_NUM 21
  #define Y4_GPIO_NUM 19
  #define Y3_GPIO_NUM 18
  #define Y2_GPIO_NUM 5
  #define VSYNC_GPIO_NUM 25
  #define HREF_GPIO_NUM 23
  #define PCLK_GPIO_NUM 22

  //wifi
  const char *ssid = "Seu_wifi";
  const char *password = "Sua_senha";

  //bot
  #define BOTtoken "Token_do_seu_BOT"
  #define CHAT_ID "Seu_chat_ID"

  WiFiClientSecure client;
  UniversalTelegramBot bot(BOTtoken, client);

  //botao
  const int botao = 15;
  bool estadoAnterior = HIGH;

  //camera
  const int MAX_FOTOS = 60;
  int indiceFoto = 0;
  unsigned long primeiraFoto = 0;
  bool memoriaCheia = false;
  unsigned long ultimoFrame = 0;
  const unsigned long intervalo = 1000;
  void capturarfoto(char *nomeArquivo);
  bool enviandoFotos = false;


  //Funções
  File fotoAtual;

  bool isMoreDataAvailable()
  {
      return fotoAtual.available();
  }

  byte getNextByte()
  {
      return fotoAtual.read();
  }

  void enviarfoto(unsigned long numero)
  {
      char nomeArquivo[30];
      gerarNomeArquivo(nomeArquivo, numero);

      fotoAtual = SD_MMC.open(nomeArquivo);

      String enviado = bot.sendPhotoByBinary(
      CHAT_ID,
      "image/jpeg",
      fotoAtual.size(),
      isMoreDataAvailable,
      getNextByte,
      nullptr,
      nullptr
  );

      fotoAtual.close();
  }

  void enviartodasfotos() {

    unsigned long ultimaFoto = indiceFoto - 1;


    if (memoriaCheia) {

      for (unsigned long i = primeiraFoto; i <= ultimaFoto; i++) {
        enviarfoto(i);
      }

    } else {

      for (unsigned long i = 0; i <= ultimaFoto; i++) {
        enviarfoto(i);
      }
    }
  }


  void verificarbotao() {
    bool estadoAtual = digitalRead(botao);
    if (estadoAnterior == HIGH && estadoAtual == LOW) {

      bot.sendMessage(CHAT_ID, "Emergência acionada!", "");
      bot.sendMessage(CHAT_ID, "Localização: -7.162287, -34.817208", "");

      enviartodasfotos();
    }
    estadoAnterior = estadoAtual;
  }

  void gerarNomeArquivo(char *nome, unsigned long numero) {

    snprintf(nome, 30, "/foto%06lu.jpg", numero);
  }


  void salvarnovafoto() {

    char nomeArquivo[30];

    if (memoriaCheia) {

      gerarNomeArquivo(nomeArquivo, primeiraFoto);

      SD_MMC.remove(nomeArquivo);

      primeiraFoto++;
    }

    gerarNomeArquivo(nomeArquivo, indiceFoto);

    capturarfoto(nomeArquivo);

    indiceFoto++;

    if (indiceFoto >= MAX_FOTOS) {

      memoriaCheia = true;
    }
  }



  void capturarfoto(char *nomeArquivo) {
    camera_fb_t *fb = esp_camera_fb_get();


    File arquivo = SD_MMC.open(nomeArquivo, FILE_WRITE);


    arquivo.write(fb->buf, fb->len);

    arquivo.close();

    delay(50);

    esp_camera_fb_return(fb);
  }




  void tempocaptura() {

    if (millis() - ultimoFrame >= intervalo) {

      ultimoFrame = millis();

      salvarnovafoto();
    }
  }


  void setup() {
    //codigo do wifi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int tentativas = 0;

    while (WiFi.status() != WL_CONNECTED && tentativas < 40) {
      delay(500);
      Serial.print(".");
      tentativas++;
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Falhou. Código: ");
      Serial.println(WiFi.status());
    }

    //codigo do bot
    client.setInsecure();

    bot.sendMessage(CHAT_ID, "Sistema de emergencia iniciado!", "");
    Serial.println("Mensagem enviada");

    //codigo do botão
    pinMode(botao, INPUT_PULLUP);

    //iniciar cartao sd
    if (!SD_MMC.begin()) {
      Serial.println("Erro ao iniciar SD");
    } else {
      Serial.println("SD iniciado");
    }

    camera_config_t config;
    memset(&config, 0, sizeof(config));

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
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    config.xclk_freq_hz = 20000000;

    config.pixel_format = PIXFORMAT_JPEG;

    config.frame_size = FRAMESIZE_VGA;

    config.jpeg_quality = 12;

    config.fb_count = 1;

    if (psramFound()) {
      config.frame_size = FRAMESIZE_VGA;
      config.jpeg_quality = 12;
      config.fb_count = 2;
    } else {
      config.frame_size = FRAMESIZE_QVGA;
      config.jpeg_quality = 15;
      config.fb_count = 1;
    }
     esp_camera_init(&config);
  }

  void loop() {

    verificarbotao();
    tempocaptura();
  }
