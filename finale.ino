volatile uint16_t datiCatturati[32];                    // Memorizza i periodi di tempo ricevuti
volatile uint8_t primoSegnaleArrivato = 0;              // Flag del primo trigger
volatile uint8_t contatoreRicezione = 0;                // Contatore di ricezione
volatile uint8_t ricezioneCompletata = 0;               // Flag di ricezione completa

void setup() {
  Serial.begin(9600);                                   // Inizializza la comunicazione seriale per il debug

  DDRB &= ~(1 << DDB0);                                 // Imposta il pin digitale 8 come input
  PORTB |= (1 << PORTB0);

  cli();                                                // Disabilita tutti gli interrupt finché non sono configurate le impostazioni del Timer 1

  // Configura il Timer 1 per la cattura dell'input
  TCCR1A = 0x00;                                        // Imposta TCCR1A a 0
  TCCR1B = (1 << ICES1) | (1 << CS11) | (1 << CS10);    // Abilita il trigger su fronte di salita, prescaler a 64
  TCCR1C = 0x00;                                        // Imposta TCCR1C a 0

  // Abilita l'interrupt di cattura dell'input
  TIMSK1 |= (1 << ICIE1);

  sei();                                                // Abilita tutti gli interrupt globali
}

void loop() {
  getCommand();
}

ISR(TIMER1_CAPT_vect) {                                 // Il Timer 1 è stato configurato per lavorare in modalità di cattura dell'input
  if (primoSegnaleArrivato) {                           // La cattura inizia dopo il primo fronte di discesa rilevato dal pin ICP1
    datiCatturati[contatoreRicezione] = ICR1;           // Leggi il valore del registro di cattura dell'INPUT
    if (datiCatturati[contatoreRicezione] > 625) {      // Se il valore è maggiore di 600 (~2.4ms), allora
      contatoreRicezione = 0;                           // Resetta "contatoreRicezione"
      ricezioneCompletata = 0;                          // Resetta "ricezioneCompletata"
    } else {
      contatoreRicezione++;
      if (contatoreRicezione == 32) {                   // Se tutti i bit sono stati rilevati,
        ricezioneCompletata = 1;                        // allora imposta il flag "ricezioneCompletata" a "1"
      }
    }
  } else {
    primoSegnaleArrivato = 1;                           // Primo fronte di discesa rilevato! Inizia la cattura dal secondo fronte di discesa.
  }
  TCNT1 = 0;                                            // Resetta il contatore del Timer 1 dopo ogni cattura
}

/*
   Il periodo di tempo t è calcolato come:
   ( datiCatturati[<INDICE>] * 16us ) / 1000 -> restituirà il risultato in millisecondi
   Es:
      t = (325 * 256) / 1000
      t = 1,3 ms
*/
uint32_t getCommand() {
  if (ricezioneCompletata) {                                              // Se la ricezione è completa, inizia il processo di decodifica
    uint32_t streamRicevuto = 0;                                          // Per memorizzare il valore decodificato
    for (int i = 0; i < 32; i++) {                                        // Decodifica tutti i 32 bit ricevuti come periodi di tempo
      if (datiCatturati[i] < 300 && datiCatturati[i] > 250 && i != 31) {  // Se il periodo di tempo t* è compreso tra 1,0 ms e 1,2 ms
        streamRicevuto = (streamRicevuto << 1);                           // Esegui solo lo shift a sinistra del valore corrente
      } else if (datiCatturati[i] < 600 && datiCatturati[i] > 525) {      // Se il periodo di tempo t* è compreso tra 2,1 ms e 2,4 ms
        streamRicevuto |= 0x0001;                                         // Incrementa il valore di 1 tramite l'operazione OR logico
        if (i != 31) {                                                    // Esegui solo lo shift del bit a meno che non sia l'ultimo bit della sequenza catturata
          streamRicevuto = (streamRicevuto << 1);                         // Esegui solo lo shift a sinistra del valore corrente
        }
        ricezioneCompletata = 0;                                          // Imposta la ricezione completa a 0 per la prossima cattura dei dati
      }
    }
    Serial.print("Codice ricevuto: ");
    Serial.print("0x00");
    Serial.println(streamRicevuto, HEX);                                  // Stampa il valore nel monitor seriale per il debug
    return streamRicevuto;                                                // Restituisci la sequenza di dati ricevuti
  }
  return 0;                                                               // Il valore di default restituito è 0
}
