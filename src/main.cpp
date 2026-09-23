#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- Configurações de MQTT ---
const char* mqtt_server = "broker.hivemq.com";
const char* topic_leds = "ifscaru/compressor/comando";
const char* topic_status = "ifscaru/compressor/status";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long tempoEstados_Leitura;
unsigned long tempoMQTT;
unsigned long marcaTempo;
unsigned long ultimaTentativa = 0;

JsonDocument doc;

bool statusEnergizado = false;
bool statusLigado = false;
bool statusAlivio = false;
bool statusSobrecarga = false;
bool desejoON = false;
bool desejoOFF = false;
//bool statusON = false;
//bool statusOFF = false;
//bool statusONSecador = false;
//bool statusOFFSecador = false;
bool valvulaAberta;
bool statusWifi = false;
bool statusMqtt = false;

// INPUTs
#define pinoEnergizado 14
#define pinoLigado 27
#define pinoAlivio 26
#define pinoSobrecarga 25
#define pinoFimdecursoFechado 33
#define pinoFimdecursoAberto 32
//#define pinoStatusON 35
//#define pinoStatusOFF 34
//#define pinoStatusONSecador 18
//#define pinoStatusOFFSecador 19 

// OUTPUTs
#define pinoDesejoON 2 
#define pinoDesejoOFF 4
//#define pinoDesejoONSecador 16  // RX2
//#define pinoDesejoOFFSecador 17  // TX2
#define pinoIN1 13
#define pinoIN2 12

enum Estados {
  DESLIGADO = 0,       // case 0
  ENERGIZADO,          // case 1
  ACIONA_ON,           // case 2
  DESACIONA_ON,        // case 3
  LIGA_SECADOR,        // case 4
  AGUARDA_ALIVIO,      // case 5
  ABRE_VALVULA,        // case 6
  EM_FUNCIONAMENTO,    // case 7
  ACIONA_OFF,          // case 8
  DESACIONA_OFF,       // case 9
  FECHA_VALVULA,       // case 10
  DESLIGA_SECADOR,     // case 11
  FALHA_DESLIGAMENTO,  // case 12 Proteção caso o compressor não desligue
  FALHA_PARTIDA        // case 13 Proteção caso o compressor não ligue
};

byte contadorTentativas = 0;
byte numeroUsuarios = 0;
byte estadoAtual = DESLIGADO;

// Função para conectar ao Wi-Fi
void setup_wifi() {
  Serial.print("Conectando ao Wi-Fi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.println(WiFi.localIP());
}

// Função chamada ao receber mensagem do Node-RED
void callback(char* topic, byte* payload, unsigned int length) {
  
  JsonDocument doc;

  DeserializationError erro = deserializeJson(doc,payload,length);

  if(erro){
    Serial.print("Falha ao ler JSON: ");
    Serial.println(erro.f_str());
    return;
  }

  if (doc["desejoON"].is<bool>()) {
    desejoON = doc["desejoON"];
  }

  if (doc["desejoOFF"].is<bool>()) {
    desejoOFF = doc["desejoOFF"];
  }
}

// Função para reconectar ao Broker MQTT
boolean reconnect() {
  Serial.print("Tentando conexão MQTT...");
  String clientId = "ESP32Client-";
  clientId += String(random(0xffff), HEX);

  if (client.connect(clientId.c_str())) {
    Serial.println("Conectado!");
    client.subscribe(topic_leds);
    return true;
  } else {
    Serial.print("Falha, rc=");
    Serial.print(client.state());
    Serial.println(" - Nova tentativa em breve.");
    return false;
  }
}

void desligaSecador() {
  //realizar função
}

void ligaSecador() {
  //realizar função
}

void fechaValvula() {  // orientação com o professor para fazer o código de fecha e abre válvula 
  if(digitalRead(pinoFimdecursoFechado) == HIGH) {
    analogWrite(pinoIN1, 150);  //problema do analogWrite com o ESP, não sei se funciona
    analogWrite(pinoIN2, 0);
  } 
  else {
    analogWrite(pinoIN1, 0);
    analogWrite(pinoIN2, 0);
    valvulaAberta = false;
  }
}

void abreValvula() {
  if(digitalRead(pinoFimdecursoAberto) == HIGH) {
    analogWrite(pinoIN1, 150);
    analogWrite(pinoIN2, 0);

    /*
    PARA LIGAR EM PEQUENOS INTERVALOS

    if(statusLigado == TRUE && millis() - tempoLigado > 3){
	    // Desliga
	    // tempoDesligado = millis();}
    if(statusLigado == FALSE && millis() - tempoDesligado > 30){
      //Liga
      // tempoLigado = millis();}
    */
   // qualquer coisa
  }
  else {
    analogWrite(pinoIN1, 0);
    analogWrite(pinoIN2, 0);
    valvulaAberta = true;
  }
}

void mudaEstado(byte novoEstado) {
  estadoAtual = novoEstado;
  marcaTempo = millis();
}

void estados() {
  switch(estadoAtual){

    case DESLIGADO:  //desliga secador e fecha válvula
    desligaSecador();
    fechaValvula();
    if (valvulaAberta == false && statusEnergizado == true){
      mudaEstado(ENERGIZADO);
    }
    break;


    case ENERGIZADO:  //energizado
    contadorTentativas = 0;
    if(desejoON == true){
      mudaEstado(ACIONA_ON);
    }
    break;


    case ACIONA_ON:  // aperta o botão ON por 2s (aciona relé liga)
    desejoON = false; //é isso mesmo? nesse caso funcionaria, mas supondo que alguém por insistencia clicasse mais de uma vez no desejoON, o resultado seria de mais usuários catalogados e esse mesmo usuário teria de clicar mais de uma vez no botão desejoOFF para que eventuamente esse valor chegue a 0 em seu funcionamento natural. 
    digitalWrite(pinoDesejoON, HIGH);
    if(millis() - marcaTempo >= 2000) {
      mudaEstado(DESACIONA_ON);
    }
    break;


    case DESACIONA_ON:    // solta o botão (desacionao o relé liga). se não ligar em 5s, liga denovo. se em 4 tentativas não der certo, ativo modo falha
    digitalWrite(pinoDesejoON, LOW);
    if(statusLigado == true){
      contadorTentativas = 0;
      mudaEstado(LIGA_SECADOR);
      numeroUsuarios = 1;
    }
    else if((millis() - marcaTempo) >= 5000){
      contadorTentativas ++;
      if(contadorTentativas <= 4){
        mudaEstado(ACIONA_ON);
      } else {
        mudaEstado(FALHA_PARTIDA);
      }
    }
    break;


    case LIGA_SECADOR:  // liga secador
    ligaSecador(); //digitalWrite(pinoOnOffSecador, HIGH);
    mudaEstado(AGUARDA_ALIVIO);
    break;


    case AGUARDA_ALIVIO:  // aguarda o compressor chegar em estado de alivio de pressão
    if(statusAlivio == true) {
      mudaEstado(ABRE_VALVULA);
    }
    break;


    case ABRE_VALVULA:  // abre a valvula de ar do compressor
    abreValvula();
    if(valvulaAberta == true){
      mudaEstado(EM_FUNCIONAMENTO);
    }
    break;


    case EM_FUNCIONAMENTO:  // colocar posteriormente: if(horarioatual >= 23h) {numero de usuários = 0} e variações usando RTC
    if(desejoON == true) {
      numeroUsuarios ++;
      //desejoON = false;
    }
    if(desejoOFF == true) {
      numeroUsuarios --;
      //desejoOFF = false;
    }
    if(numeroUsuarios == 0) {
      mudaEstado(ACIONA_OFF);
    }
    break;


    case ACIONA_OFF:  // aperta o botão OFF por 2s (aciona relé de desliga)
    desejoON = false; // VERIFICAR DEPOIS O AJUSTE PARA USAR BOTAO DE PULSO NO SCADABR
    digitalWrite(pinoDesejoOFF, HIGH);
    if(millis() - marcaTempo >= 2000) {
      mudaEstado(DESACIONA_OFF);
    }
    break;


    case DESACIONA_OFF:  // solta o botão off (desaciona o relé de desliga)
    digitalWrite(pinoDesejoOFF, LOW);
    mudaEstado(FECHA_VALVULA);
    break;


    case FECHA_VALVULA:  // fecha a válvula de ar do compressor
    fechaValvula();
    if(valvulaAberta == false){
      mudaEstado(DESLIGA_SECADOR);
    }
    break;


    case DESLIGA_SECADOR:
    desligaSecador(); //digitalWrite(pinoOnOffSecador, LOW);
    if(statusLigado == false){
      contadorTentativas = 0;
      mudaEstado(ENERGIZADO);
    }
    else if((millis() - marcaTempo) >= 300000){
      contadorTentativas ++;
      if(contadorTentativas <= 3){
        mudaEstado(ACIONA_OFF);
      } else {
        mudaEstado(FALHA_DESLIGAMENTO);
      }
    }
    break;


    case FALHA_PARTIDA:

    break;


    case FALHA_DESLIGAMENTO:

    break;
  }
}


void setup() {
  Serial.begin(115200);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  tempoEstados_Leitura = millis();
  tempoMQTT = millis();

  pinMode(pinoDesejoOFF, OUTPUT);
  pinMode(pinoDesejoON, OUTPUT);
  //pinMode(pinoDesejoOFFSecador, OUTPUT);
  //pinMode(pinoDesejoONSecador, OUTPUT);
  pinMode(pinoIN1, OUTPUT);
  pinMode(pinoIN2, OUTPUT);

  //pinMode(pinoStatusON,INPUT_PULLDOWN);
  //pinMode(pinoStatusOFF,INPUT_PULLDOWN);
  //pinMode(pinoStatusONSecador,INPUT_PULLDOWN);
  //pinMode(pinoStatusOFFSecador,INPUT_PULLDOWN);
  pinMode(pinoEnergizado, INPUT_PULLDOWN);
  pinMode(pinoLigado, INPUT_PULLDOWN);
  pinMode(pinoAlivio, INPUT_PULLDOWN);
  pinMode(pinoSobrecarga, INPUT_PULLDOWN);
  pinMode(pinoFimdecursoAberto, INPUT_PULLDOWN);
  pinMode(pinoFimdecursoFechado, INPUT_PULLDOWN);

  digitalWrite(pinoDesejoON, LOW);
  digitalWrite(pinoDesejoOFF, LOW);
  analogWrite(pinoIN1, 0);
  analogWrite(pinoIN2, 0);
  //digitalWrite(pinoDesejoONSecador, LOW);
  //digitalWrite(pinoDesejoOFFSecador, LOW);
}

void loop() {
  if (!client.connected()) {
    unsigned long agora = millis();
    statusMqtt = false;
    if (agora - ultimaTentativa > 5000) {  // verifica se já se passaram 5 segundos desde a última tentativa.
      ultimaTentativa = agora;
      if (reconnect()) {
        ultimaTentativa = 0;
      }
    }
  } else {
    client.loop();
    statusMqtt = true;
  }


// timer da execução da máquina de estados e leitura dos status.
  if (millis() - tempoEstados_Leitura > 50) {
    tempoEstados_Leitura = millis();

    statusWifi = (WiFi.status() == WL_CONNECTED); // verificar se é aqui o lugar correto
    statusEnergizado = digitalRead(pinoEnergizado);
    statusLigado = digitalRead(pinoLigado);
    statusAlivio = digitalRead(pinoAlivio);
    statusSobrecarga = digitalRead(pinoSobrecarga);
    //statusON = digitalRead(pinoStatusON);
    //statusOFF = digitalRead(pinoStatusOFF);
    //statusONSecador = digitalRead(pinoStatusONSecador);
    //statusOFFSecador = digitalRead(pinoStatusOFFSecador);

    /*if(digitalRead(pinoFimdecursoAberto) == LOW) {
      valvulaAberta = true;
    }
    if(digitalRead(pinoFimdecursoFechado) == LOW) {
      valvulaAberta = false;
    }*/ //Estaria conflitando com o estado entre aberto e fechado da válvula, então não é necessário.
    
    estados();
  }

// timer do envio MQTT. mais lento para evitar problemas.
  if (millis() - tempoMQTT > 1000) {
    tempoMQTT = millis();

    if (client.connected()) {
      //doc["ON"] = statusON;
      //doc["OFF"] = statusOFF;
      //doc["SecadorON"] = statusONSecador;
      //doc["SecadorOFF"] = statusOFFSecador;
      doc["desejoON"] = desejoON; // desejoON e desejoOFF apenas chegam e não são enviados
      doc["desejoOFF"] = desejoOFF;
      doc["Energizado"] = statusEnergizado;
      doc["Ligado"] = statusLigado;
      doc["Alivio"] = statusAlivio;
      doc["Sobrecarga"] = statusSobrecarga;
      doc["valvulaAberta"] = valvulaAberta;
      doc["Estado"] = estadoAtual;

      String payloadJSON;
      serializeJson(doc, payloadJSON);
      client.publish(topic_status, payloadJSON.c_str());
    }
  }
}
