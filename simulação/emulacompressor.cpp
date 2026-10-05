const int ledEnergizado = 5;
const int ledLigado = 4;
const int ledAlivio = 14;
const int ledSobrecarga = 12;
const int botaoSobrecarga = 13;
const int ligar = 15;
const int desligar = 16;

unsigned long tempoAlivio;
unsigned long tempoDesligar;

#define intervaloAlivio 10000

enum Estados {
  ENERGIZADO = 0,
  LIGADO,
  DESLIGANDO,
  SOBRECARGA
};

Estados estadoAtual = ENERGIZADO;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(ledEnergizado, OUTPUT);
  pinMode(ledLigado, OUTPUT);
  pinMode(ledAlivio, OUTPUT);
  pinMode(ledSobrecarga, OUTPUT);
  pinMode(botaoSobrecarga, INPUT_PULLUP);
  pinMode(ligar,INPUT);
  pinMode(desligar,INPUT);

  digitalWrite(ledLigado,LOW);
  digitalWrite(ledAlivio,LOW);
  digitalWrite(ledSobrecarga,LOW);
}

void loop() {
  switch(estadoAtual){
    case ENERGIZADO:
      Serial.println("ENERGIZADO");
      digitalWrite(ledEnergizado,HIGH);
      if(digitalRead(ligar)){
        tempoAlivio = millis();
        estadoAtual = LIGADO;}
    break;

    case LIGADO:
      Serial.println("LIGADO");
      digitalWrite(ledLigado,HIGH);
      if(millis()-tempoAlivio > intervaloAlivio){
        tempoAlivio = millis();
        digitalWrite(ledAlivio, !digitalRead(ledAlivio));}
      if(digitalRead(desligar)){
        estadoAtual = DESLIGANDO;
        tempoDesligar = millis();}
    break;

    case DESLIGANDO:
      Serial.println("DESLIGANDO");
      if(millis() - tempoDesligar > 10000){
        digitalWrite(ledLigado,LOW);
        digitalWrite(ledAlivio,LOW);
        estadoAtual = ENERGIZADO;}
    break;

    case SOBRECARGA:

    break;

    default:
      Serial.println("Default");
  }

  delay(200);
}