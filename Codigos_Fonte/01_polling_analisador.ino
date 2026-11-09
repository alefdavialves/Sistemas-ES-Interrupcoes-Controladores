// POLLING - Arduino Uno (ATmega328P) no Wokwi
// D2: botao (INPUT_PULLUP; pressionado = LOW)
// D6: marcador de conclusao do atendimento (analisador logico D2)
// D7: marcador de deteccao do evento (analisador logico D1)
// D13: LED integrado, aceso apos o evento
// Execute uma unica pressao por simulacao. Janela maxima: 10 segundos.

const byte PIN_BOTAO = 2;
const byte PIN_LED_EVENTO = LED_BUILTIN;
const byte PIN_CONCLUSAO = 6;
const byte PIN_DETECCAO = 7;
const unsigned long JANELA_MS = 10000UL;

unsigned long consultasSemEvento = 0;
unsigned long inicioJanelaMs = 0;
unsigned long inicioJanelaUs = 0;
bool finalizado = false;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BOTAO, INPUT_PULLUP);
  pinMode(PIN_LED_EVENTO, OUTPUT);
  pinMode(PIN_CONCLUSAO, OUTPUT);
  pinMode(PIN_DETECCAO, OUTPUT);

  digitalWrite(PIN_LED_EVENTO, LOW);
  digitalWrite(PIN_CONCLUSAO, LOW);
  digitalWrite(PIN_DETECCAO, LOW);

  Serial.println(F("[POLLING] Pronto. Pressione o botao D2 uma vez."));
  Serial.println(F("[POLLING] Janela de observacao: 10 segundos."));
  inicioJanelaMs = millis();
  inicioJanelaUs = micros();
}

void loop() {
  if (finalizado) return;

  // Cada execucao deste digitalRead() e uma consulta ao estado do botao.
  if (digitalRead(PIN_BOTAO) == LOW) {
    // Marco D7: o polling identificou o evento.
    digitalWrite(PIN_DETECCAO, HIGH);

    // Atendimento definido para o experimento: registrar o evento e acender D13.
    digitalWrite(PIN_LED_EVENTO, HIGH);

    // Marco D6: o atendimento definido acima foi concluido.
    digitalWrite(PIN_CONCLUSAO, HIGH);

    finalizado = true;
    Serial.println(F("[RESULTADO] POLLING COM EVENTO"));
    Serial.print(F("Consultas sem evento: "));
    Serial.println(consultasSemEvento);
    Serial.print(F("Consultas totais (incluindo a que detectou): "));
    Serial.println(consultasSemEvento + 1);
    Serial.print(F("Tempo desde o inicio da janela (us; NAO e latencia): "));
    Serial.println(micros() - inicioJanelaUs);
    Serial.println(F("Latencia e tempo de resposta: medir no VCD entre D2, D7 e D6."));
    return;
  }

  consultasSemEvento++;

  // Teste A: se ninguem pressionar o botao, termina a janela e apresenta a contagem.
  if (millis() - inicioJanelaMs >= JANELA_MS) {
    finalizado = true;
    Serial.println(F("[RESULTADO] POLLING SEM EVENTO - janela de 10 s"));
    Serial.print(F("Consultas sem evento em 10 s: "));
    Serial.println(consultasSemEvento);
  }
}
