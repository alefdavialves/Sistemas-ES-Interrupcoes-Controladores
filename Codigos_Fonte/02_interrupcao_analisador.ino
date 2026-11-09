// INTERRUPCAO EXTERNA - Arduino Uno (ATmega328P) no Wokwi
// D2: botao / INT0 (INPUT_PULLUP; pressionado = LOW)
// D4: chave que habilita/desabilita INT0 (LOW = desabilitada; HIGH = habilitada)
// D6: marcador de conclusao do atendimento (analisador logico D2)
// D7: marcador de deteccao no inicio da ISR aceita (analisador logico D1)
// D8: LED que indica atividade do loop principal
// D13: LED integrado, aceso apos o processamento do evento
// Execute uma unica pressao por simulacao. Janela maxima: 10 segundos.

const byte PIN_BOTAO = 2;
const byte PIN_IRQ_ENABLE = 4;
const byte PIN_LED_EVENTO = LED_BUILTIN;
const byte PIN_LED_LOOP = 8;
const byte PIN_CONCLUSAO = 6;
const byte PIN_DETECCAO = 7;
const unsigned long JANELA_MS = 10000UL;
const unsigned long DEBOUNCE_US = 50000UL;

volatile bool eventoPendente = false;
volatile unsigned long contIsr = 0;
volatile unsigned long tEvento = 0;
volatile unsigned long duracaoIsr = 0;

unsigned long inicioJanelaMs = 0;
unsigned long iteracoesLoop = 0;
unsigned long eventosTratados = 0;
unsigned long ultimaLeituraChave = 0;
unsigned long ultimoBatimento = 0;
bool irqHabilitada = false;
bool primeiraLeitura = true;
bool estadoLedLoop = false;
bool finalizado = false;

void isrBotao() {
  unsigned long t0 = micros();
  contIsr++; // Conta todas as entradas na ISR, inclusive possiveis rebotes.

  if (t0 - tEvento > DEBOUNCE_US) {
    // D7 marca o primeiro ponto de deteccao aceito dentro da ISR.
    digitalWrite(PIN_DETECCAO, HIGH);
    tEvento = t0;
    eventoPendente = true;
    // Mede apenas a execucao da ISR aceita. E uma medida aproximada.
    duracaoIsr = micros() - t0;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BOTAO, INPUT_PULLUP);
  pinMode(PIN_IRQ_ENABLE, INPUT_PULLUP);
  pinMode(PIN_LED_EVENTO, OUTPUT);
  pinMode(PIN_LED_LOOP, OUTPUT);
  pinMode(PIN_CONCLUSAO, OUTPUT);
  pinMode(PIN_DETECCAO, OUTPUT);

  digitalWrite(PIN_LED_EVENTO, LOW);
  digitalWrite(PIN_LED_LOOP, LOW);
  digitalWrite(PIN_CONCLUSAO, LOW);
  digitalWrite(PIN_DETECCAO, LOW);

  attachInterrupt(digitalPinToInterrupt(PIN_BOTAO), isrBotao, FALLING);

  Serial.println(F("[INTERRUPCAO] Pronto. Aguarde a mensagem de IRQ habilitada/desabilitada."));
  Serial.println(F("Aguarde 1 segundo apos a configuracao e pressione D2 uma vez."));
  inicioJanelaMs = millis();
}

void loop() {
  if (finalizado) return;

  unsigned long agoraMs = millis();

  // A chave D4 e consultada a cada 50 ms. O botao D2 NAO e consultado no loop.
  if (agoraMs - ultimaLeituraChave >= 50UL) {
    ultimaLeituraChave = agoraMs;
    bool desejada = (digitalRead(PIN_IRQ_ENABLE) == HIGH);

    if (primeiraLeitura || desejada != irqHabilitada) {
      primeiraLeitura = false;
      irqHabilitada = desejada;
      if (irqHabilitada) {
        EIMSK |= (1 << INT0);
      } else {
        EIMSK &= ~(1 << INT0);
      }
      Serial.print(F("[CONTROL] INT0 "));
      Serial.println(irqHabilitada ? F("HABILITADA") : F("DESABILITADA"));
    }
  }

  // Contador de iteracoes do loop: indicador de atividade, nao de trabalho util real.
  iteracoesLoop++;
  if (agoraMs - ultimoBatimento >= 250UL) {
    ultimoBatimento = agoraMs;
    estadoLedLoop = !estadoLedLoop;
    digitalWrite(PIN_LED_LOOP, estadoLedLoop ? HIGH : LOW);
  }

  if (eventoPendente) {
    noInterrupts();
    eventoPendente = false;
    unsigned long t = tEvento;
    unsigned long dur = duracaoIsr;
    unsigned long n = contIsr;
    interrupts();

    unsigned long atrasoIsrLoop = micros() - t;
    eventosTratados++;
    digitalWrite(PIN_LED_EVENTO, HIGH);

    // D6 marca o fim do atendimento definido para o experimento.
    digitalWrite(PIN_CONCLUSAO, HIGH);
    finalizado = true;

    Serial.println(F("[RESULTADO] INTERRUPCAO COM EVENTO"));
    Serial.print(F("Iteracoes do loop principal: "));
    Serial.println(iteracoesLoop);
    Serial.println(F("Consultas ao botao D2 no loop: 0"));
    Serial.print(F("Atraso entre registro na ISR e processamento no loop (us): "));
    Serial.println(atrasoIsrLoop);
    Serial.print(F("Duracao aproximada da ISR aceita (us): "));
    Serial.println(dur);
    Serial.print(F("Entradas na ISR (inclui possiveis rebotes): "));
    Serial.println(n);
    Serial.print(F("Eventos processados pelo loop: "));
    Serial.println(eventosTratados);
    Serial.println(F("Latencia e tempo de resposta: medir no VCD entre D2, D7 e D6."));
    return;
  }

  // Teste D: com INT0 desabilitada, o loop nao processa evento; ao fim,
  // registra o estado do bit de habilitacao e da flag de interrupcao pendente.
  if (agoraMs - inicioJanelaMs >= JANELA_MS) {
    noInterrupts();
    unsigned int habilitada = (EIMSK & (1 << INT0)) ? 1 : 0;
    unsigned int pendente = (EIFR & (1 << INTF0)) ? 1 : 0;
    unsigned long n = contIsr;
    interrupts();

    finalizado = true;
    Serial.println(F("[RESULTADO] JANELA ENCERRADA SEM ATENDIMENTO"));
    Serial.print(F("INT0 habilitada (EIMSK): "));
    Serial.println(habilitada);
    Serial.print(F("Flag INTF0 pendente (EIFR): "));
    Serial.println(pendente);
    Serial.print(F("Entradas na ISR: "));
    Serial.println(n);
    Serial.println(F("Se houve borda em D2 e INTF0=1, a requisicao ficou pendente."));
  }
}
