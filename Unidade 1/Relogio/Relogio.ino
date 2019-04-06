/*
   PORTB0 = pino 8; Botão Modo
   PORTB1 = pino 9; Botão Set
   PORTB2 = pino 10; Botão Reset
   PORTC0 = A0; Potenciometro
   PORTD6 = pino 6; Buzzer
*/

#include <avr/io.h>
#include <math.h>
#include <avr/interrupt.h>
#include <inttypes.h>

volatile uint16_t tempo = 0; // Contador de segundos decorridos desde o início do programa
volatile uint8_t sR = 0; // Valor de segundos mostrado no relógio
volatile uint8_t mR = 0; // Valor de minutos mostrado no relógio
volatile uint8_t hR = 0; // Valor de horas mostrado no relógio
volatile uint8_t mRC = 0; // Valor de minutos mostrado no relógio em configuração
volatile uint8_t hRC = 0; // Valor de horas mostrado no relógio me configuração

volatile uint8_t sC = 0; // Valor de segundos mostrado no cronometro
volatile uint8_t mC = 0; // Valor de minutos mostrado no cronometro
volatile uint8_t hC = 0; // Valor de horas mostrado no cronometro

volatile uint8_t mA = 59; // Valor de minutos mostrado no alarme
volatile uint8_t hA = 23; // Valor de horas mostrado no alarme
volatile uint8_t mAC = 0; // Valor de minutos mostrado no alarme em configuração
volatile uint8_t hAC = 0; // Valor de horas mostrado no alarme em configuração

volatile uint8_t x = 0;

enum modos {RELOGIO, CRONOMETRO, ALARME}; // Modos do relógio Casio
enum modoConfig {PADRAO, HORA, MINUTO}; // Indica qual variável estou configurando

modos modoAtual = RELOGIO;
modoConfig configRelogio = PADRAO;
modoConfig configAlarme = PADRAO;
bool cronoAtivo = false, alarmeAtivo = false;
bool botaoModoApertado = false;
bool bSetApertado = false, bResetApertado = false;

ISR(TIMER0_OVF_vect) {
  x++;
  if (x < 10) { //61 overflows = 1 segundo decorrido

  } else { // ELSE executado a cada 1 segundo
    sR++;
    if (cronoAtivo) sC++;
    x = 0;

    if (sC >= 60) {   // Incremento das variáveis
      mC++; sC = 0;
    } if (mC >= 60) {
      hC++; mC = 0;
    } if (hC >= 24) {
      hC = 0;
    }

    if (sR >= 60) {
      mR++; mRC++; sR = 0;
    } if (mR >= 60) {
      hR++; hRC++; mR = 0;
    } if (hR >= 24) {
      hR = 0; hRC = 0;
    }                 // Incremento das variáveis

    if (modoAtual == RELOGIO) { // Relógio
      if (configRelogio == PADRAO) { // modo padrão, mostra tudo
        Serial.print(hR);
        Serial.print(":");
        Serial.print(mR);
        Serial.print(":");
        Serial.println(sR);
      } else {
        if (configRelogio == HORA) { // configurando hora
          if (sR % 2 == 0)  Serial.print(hRC); // Mostra hora
          else Serial.print("  "); //Esconde hora

        } else Serial.print(hRC);
        Serial.print(":");
        if (configRelogio == MINUTO) { // condigurando minuto
          if (sR % 2 == 0) Serial.print(mRC); //Mostra minuto
          else Serial.print("  "); //Esconde minuto

        } else Serial.print(mRC);

        Serial.print(":");
        Serial.println(sR);
      }

    } else if (modoAtual == CRONOMETRO) { // Cronometro
      Serial.print(hC);
      Serial.print(":");
      Serial.print(mC);
      Serial.print(":");
      Serial.println(sC);

    } else if (modoAtual == ALARME) { // Alarme
      if (alarmeAtivo) {
        Serial.print("ON ");
      } else {
        Serial.print("OFF ");
      }
      if (configAlarme == PADRAO) { // modo padrão, mostra tudo
        Serial.print(hA);
        Serial.print(":");
        Serial.println(mA);
      } else {
        if (configAlarme == HORA) { // configurando hora
          if (sR % 2 == 0)  Serial.print(hA); // Mostra hora
          else Serial.print("  "); //Esconde hora

        } else Serial.print(hA);
        Serial.print(":");
        if (configAlarme == MINUTO) { // condigurando minuto
          if (sR % 2 == 0) Serial.println(mA); //Mostra minuto
          else Serial.println("  "); //Esconde minuto

        } else Serial.println(mA);
      }
    }

    if (mR == mA && hR == hA && alarmeAtivo) {
      PORTD |= 0b00001000; //digitalWrite(3, HIGH);
    } else {
      PORTD &= 0b11110111; //digitalWrite(3, LOW);
    }

    /*
      if(AC0A){
      PORTB |= 0b00001000;
      } else {
      PORTB &= 0b11110111;
      }
    */
  }
}


int main(void) {
  Serial.begin(9600);

  // valores iniciais dos sinais PWM
  // configuracao do PWM
  //FAST PWM 10 bits (Modo 7) sem inversão
  TCCR1A = _BV(COM1A1) | _BV(WGM10) | _BV(WGM11);
  TCCR1B = _BV(CS11) | _BV(WGM12);

  // PB1/OC1A como saída
  DDRB = 0b00100010;
  //Valor do PWM OCR1A = 0 - 1023

  ADMUX   |= 0b01000000;
  ADCSRA  |= 0b10000111;

  DDRD  |= 0b00001000; // Pino PD3 como saída
  PORTD &= 0b00001000;

  DDRB  &= 0b11111000;  //Pino PB0:2 como entrada
  PORTB |= 0b00000111; // Entradas pull up

  //CONTAR TEMPO COM O CONTADOR ZERO
  TCCR0A |= 0b00000000;
  TCCR0B |= _BV(CS02) | _BV(CS00);
  TIMSK0 |= _BV(TOIE0);
  sei(); // habilita interrupções


  while (1) {
    if (!(PINB & 0x01)) { // PORTB0 pressionado, Modo
      if (!botaoModoApertado) {
        botaoModoApertado = true;
        mudarModo(); // relogio -> cronometro -> alarme
        imprimeModo();
      }
    }
    if (PINB & 0x01) { // Se PINB0 estiver solto
      botaoModoApertado = false;
    }

    if (modoAtual == RELOGIO) {
      if (!(PINB & 0b00000010)) { // PORTB1, Set muda estado salvando
        if (!bSetApertado) {
          bSetApertado = true;
          imprimeModo();

          if (configRelogio == MINUTO) {
            mR = mRC;
            mudarModoConfig();
          } else if (configRelogio == HORA) {
            hR = hRC;
            mudarModoConfig();
          }
        }
      }
      if (PINB & 0b00000010) bSetApertado = false;

      if (!(PINB & 0b00000100)) { // PORTB2, Reset muda estado sem salvar
        if (!bResetApertado) {
          bResetApertado = true;
          hRC = hR; mRC = mR;
          mudarModoConfig(); imprimeModo();
        }
      }
      if (PINB & 0b00000100) bResetApertado = false;

      if (configRelogio != PADRAO) { // Só mostrando
        ADCSRA |= 0b01000000;
        while (!(ADCSRA & 0b00010000)); //Ainda não estamos usando ADC
        if (configRelogio == MINUTO) { //Ajustando minutos
          mRC = (ADC * 59) / 1023;
        } else if (configRelogio == HORA) { //Ajustando horas
          hRC = (ADC * 23) / 1023;
        }
      }

    } else if (modoAtual == CRONOMETRO) {
      if (!(PINB & 0b00000010)) { //PORTB1, Set crono
        if (!bSetApertado) {
          bSetApertado = true;
          cronoAtivo = !cronoAtivo;
          Serial.print("Estado cronoAtivo: ");
          Serial.println(cronoAtivo);
        }
      }
      if (PINB & 0b00000010) bSetApertado = false;  // Se PINB1 estiver solto

      if (!(PINB & 0b00000100)) { //PORTB2, Reset crono
        if (!bResetApertado) {
          bResetApertado = true;
          if (!cronoAtivo) {
            Serial.println("Resetando cronometro");
            sC = mC = hC = 0;
          }
        }
      }
      if (PINB & 0b00000100) bResetApertado = false; // Se PINB2 estiver solto

    }  else if (modoAtual == ALARME) {
      if (!(PINB & 0b00000100)) { // PORTB2, Reset muda o modo do alarme
        if (!bResetApertado) {
          bResetApertado = true;
          mudarModoConfig();
          imprimeModo();
        }
      }
      if (PINB & 0b00000100) bResetApertado = false;

      if (!(PINB & 0b00000010)) { // PORTB1, Set muda o modo do alarme
        if (!bSetApertado) {
          bSetApertado = true;
          alarmeAtivo = !alarmeAtivo;
        }
      }
      if (PINB & 0b00000010) bSetApertado = false;

      if (configAlarme != PADRAO) { // Só mostrando
        ADCSRA |= 0b01000000;
        while (!(ADCSRA & 0b00010000)); //Ainda não estamos usando ADC
        if (configAlarme == MINUTO) { //Ajustando minutos
          mA = (ADC * 59) / 1023;
        } else if (configAlarme == HORA) { //Ajustando horas
          hA = (ADC * 23) / 1023;
        }
      }
    }
  }
}//FIM DO MAIN





void mudarModo() {
  //modoAtual é GLOBAL
  if (modoAtual == RELOGIO) modoAtual = CRONOMETRO;
  else if (modoAtual == CRONOMETRO) modoAtual = ALARME;
  else modoAtual = RELOGIO;
}
void mudarModoConfig() {
  if (modoAtual == RELOGIO) {
    if (configRelogio == PADRAO) configRelogio = MINUTO;
    else if (configRelogio == MINUTO) configRelogio = HORA;
    else configRelogio = PADRAO;
  }
  else if (modoAtual == ALARME) {
    if (configAlarme == PADRAO) configAlarme = MINUTO;
    else if (configAlarme == MINUTO) configAlarme = HORA;
    else configAlarme = PADRAO;
  }
}

void imprimeModo() {
  //Todas variáveis em uso são GLOBAIS
  if (modoAtual == RELOGIO) {
    if (configRelogio == PADRAO)  Serial.println("RELOGIO PADRAO");
    else if (configRelogio == HORA) Serial.println("CONFIG RELOGIO HORA");
    else if (configRelogio == MINUTO) Serial.println("CONFIG RELOGIO MINUTO");

  }
  else if (modoAtual == CRONOMETRO) Serial.println("CRONOMETRO");
  else if (modoAtual == ALARME) {
    if (configAlarme == PADRAO) Serial.println("ALARME PADRAO");
    else if (configAlarme == HORA) Serial.println("CONFIG ALARME HORA");
    else if (configAlarme == MINUTO) Serial.println("CONFIG ALARME MINUTO");

  }
}
