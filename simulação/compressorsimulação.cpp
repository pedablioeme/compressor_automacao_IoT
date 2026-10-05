#include <Arduino.h>

#define PIN_ENERGIZADO 25
#define PIN_LIGADO     26
#define PIN_ALIVIO     27
#define PIN_SOBRECARGA 14

#define PIN_DESEJO_LIGA    32
#define PIN_DESEJO_DESLIGA 33
#define PIN_BTN_SOBRECARGA 13

#define T_ENERGIZAR  5000
#define T_ALIVIO     10000

bool energizado = false;
bool ligado     = false;
bool alivio     = false;
bool sobrecarga = false;

unsigned long tempo_atual;
unsigned long tLigadoMudou = 0;  // instante em que "ligado" mudou de valor
bool desligaArmado = false;      // true enquanto o botão de desligar está em HIGH

// ---------- Declarações ----------
void atualizaSaidas();

void setup() {
  pinMode(PIN_ENERGIZADO, OUTPUT);
  pinMode(PIN_LIGADO, OUTPUT);
  pinMode(PIN_ALIVIO, OUTPUT);
  pinMode(PIN_SOBRECARGA, OUTPUT);

  pinMode(PIN_DESEJO_LIGA, INPUT_PULLDOWN);
  pinMode(PIN_DESEJO_DESLIGA, INPUT_PULLDOWN);
  pinMode(PIN_BTN_SOBRECARGA, INPUT_PULLDOWN);

  atualizaSaidas();
}

void loop() {
  tempo_atual = millis();

  // 1) Energizado: true 5 s após a inicialização
  if (!energizado && tempo_atual >= T_ENERGIZAR) {
    energizado = true;
  }

  // 2) Sobrecarga: segue o pino de simulação
  sobrecarga = digitalRead(PIN_BTN_SOBRECARGA) == HIGH;

  // 3) Ligar: só se energizado e sem sobrecarga
  if (!ligado && energizado && !sobrecarga && digitalRead(PIN_DESEJO_LIGA) == HIGH) {
    ligado = true;
    tLigadoMudou = tempo_atual;
  }

  // 4) Desligar: HIGH e depois LOW no pino de desligamento
  if (digitalRead(PIN_DESEJO_DESLIGA) == HIGH) {
    desligaArmado = true;
  } else if (desligaArmado) {   // acabou de cair para LOW
    desligaArmado = false;
    if (ligado) {
      ligado = false;
      tLigadoMudou = tempo_atual;
    }
  }

  /*
  // 5) Sobrecarga derruba o compressor
  if (sobrecarga && ligado) {
    ligado = false;
    tLigadoMudou = tempo_atual;
  }
  */
 
  // 6) Alívio: true após 10 s com ligado == true, false após 10 s com ligado == false
  if (ligado && !alivio && (tempo_atual - tLigadoMudou >= T_ALIVIO)) {
    alivio = true;
  }
  if (!ligado && alivio && (tempo_atual - tLigadoMudou >= T_ALIVIO)) {
    alivio = false;
  }

  atualizaSaidas();
}

void atualizaSaidas() {
  digitalWrite(PIN_ENERGIZADO, energizado ? HIGH : LOW);
  digitalWrite(PIN_LIGADO,     ligado     ? HIGH : LOW);
  digitalWrite(PIN_ALIVIO,     alivio     ? HIGH : LOW);
  digitalWrite(PIN_SOBRECARGA, sobrecarga ? HIGH : LOW);
}