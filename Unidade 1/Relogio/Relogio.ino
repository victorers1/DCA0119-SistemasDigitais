/*
   PORTB0 = pino 8; Botão Modo
   PORTB1 = pino 9; Botão Set
   PORTB2 = pino 10; Botão Reset
   PORTC0 = A0; Potenciometro
   PORTD6 = pino 9; Buzzer
*/

#include <avr/io.h>
#include <math.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#define F_CPU 16000000UL
#define BAUD   9600    //taxa de 9600 bps
#define MYUBRR  F_CPU/16/BAUD-1

#define tam_vetor  2 //número de digitos individuais para a conversão por ident_num()   
#define conv_ascii  48  //48 se ident_num() deve retornar um número no formato ASCII (0 para formato normal)

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
volatile uint8_t hA = 3; // Valor de horas mostrado no alarme
volatile uint8_t mAC = 0; // Valor de minutos mostrado no alarme em configuração
volatile uint8_t hAC = 0; // Valor de horas mostrado no alarme em configuração

volatile uint8_t x = 0;

char numero[5];
char *n = numero;
char dp[2] = ":\0";
char *doisPontos = dp;
enum modos {RELOGIO, CRONOMETRO, ALARME}; // Modos do relógio Casio
enum modoConfig {PADRAO, HORA, MINUTO}; // Indica qual variável estou configurando

modos modoAtual = RELOGIO;
modoConfig configRelogio = PADRAO;
modoConfig configAlarme = PADRAO;
bool cronoAtivo = false, alarmeAtivo = false;
bool botaoModoApertado = false;
bool bSetApertado = false, bResetApertado = false;

ISR(USART_RX_vect) {
  while (!(UCSR0A & (1 << RXC0))); //espera o dado ser recebido
  if (UDR0 == 'm') {
    mudarModo();
  } else if (UDR0 == 'r') {
    if (modoAtual == RELOGIO) {
      hRC = hR; mRC = mR;
      mudarModoConfig();
    } else if (modoAtual == ALARME) {
      mudarModoConfig();
    } else if (modoAtual == CRONOMETRO) {
      if (!cronoAtivo) {
        escreve_USART("Resetando cronometro\n\0");
        sC = mC = hC = 0;
      }
    }
  } else if (UDR0 == 's') {
    if (modoAtual == RELOGIO) {
      if (configRelogio == HORA) {
        hR = hRC;
      } else { //Configurando minutos
        mR = mRC;
      }
      mudarModoConfig();
    } else if (modoAtual == ALARME) {
      alarmeAtivo = !alarmeAtivo;
    } else if (modoAtual == CRONOMETRO) {
      cronoAtivo = !cronoAtivo;
      escreve_USART("Estado cronoAtivo: \0");
      ident_num(cronoAtivo, n);
      escreve_USART(n);
      escreve_USART("\n\0");
    }
  }
}
int main(void) {
  USART_Inic(MYUBRR);

  // valores iniciais dos sinais PWM
  // configuracao do PWM
  //FAST PWM 10 bits (Modo 7) sem inversão
  TCCR1A = _BV(COM1A1) | _BV(COM1A0) | _BV(WGM10) | _BV(WGM11);
  TCCR1B = _BV(CS11) | _BV(WGM12);

  ADMUX   |= 0b01000000;
  ADCSRA  |= 0b10000111;

  DDRD  |= 0b00001000; // Pino PD3 como saída
  PORTD &= 0b00001000;

  DDRB |= 0b00000010; // PB1/OC1A como saída
  DDRB &= 0b11110010;  //Pino PB0, PB2 PB3 e como entrada
  PORTB |= 0b00001101; // Entradas pull up

  //CONTAR TEMPO COM O CONTADOR ZERO
  TCCR0A |= 0b00000000;
  TCCR0B |= _BV(CS02) | _BV(CS00);
  TIMSK0 |= _BV(TOIE0); //Habilita interrupção por Overflow
  sei(); // habilita interrupções


  while (1) {
    if (!(PINB & 0x01)) { // PORTB0 pressionado, Modo
      if (!botaoModoApertado) {
        botaoModoApertado = true;
        mudarModo(); // relogio -> cronometro -> alarme
      }
    }
    if (PINB & 0x01) { // Se PINB0 estiver solto
      botaoModoApertado = false;
    }

    if (modoAtual == RELOGIO) {
      if (!(PINB & 0b00001000)) { // PORTB3, Set muda estado salvando
        if (!bSetApertado) {
          bSetApertado = true;

          if (configRelogio == MINUTO) {
            mR = mRC;
          } else if (configRelogio == HORA) {
            hR = hRC;
          }
          mudarModoConfig();
        }
      }
      if (PINB & 0b00001000) bSetApertado = false;

      if (!(PINB & 0b00000100)) { // PORTB2, Reset muda estado sem salvar
        if (!bResetApertado) {
          bResetApertado = true;
          hRC = hR; mRC = mR;
          mudarModoConfig();
        }
      }
      if (PINB & 0b00000100) bResetApertado = false;

      if (configRelogio != PADRAO) { // Só mostrando
        ADCSRA |= 0b01000000;
        while (!(ADCSRA & 0b00010000));

        if (configRelogio == MINUTO) { //Ajustando minutos
          mRC = (ADC * 59) / 1023;
        } else if (configRelogio == HORA) { //Ajustando horas
          hRC = (ADC * 23) / 1023;
        }
      }

    } else if (modoAtual == CRONOMETRO) {
      if (!(PINB & 0b00001000)) { //PORTB1, Set crono
        if (!bSetApertado) {
          bSetApertado = true;
          cronoAtivo = !cronoAtivo;
          escreve_USART("Estado cronoAtivo: \0");
          ident_num(cronoAtivo, n);
          escreve_USART(n);
          escreve_USART("\n\0");
        }
      }
      if (PINB & 0b00001000) bSetApertado = false;  // Se PINB1 estiver solto

      if (!(PINB & 0b00000100)) { //PORTB2, Reset crono
        if (!bResetApertado) {
          bResetApertado = true;
          if (!cronoAtivo) {
            escreve_USART("Resetando cronometro\n\0");
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
        }
      }
      if (PINB & 0b00000100) bResetApertado = false;

      if (!(PINB & 0b00001000)) { // PORTB1, Set muda o modo do alarme
        if (!bSetApertado) {
          bSetApertado = true;
          alarmeAtivo = !alarmeAtivo;
        }
      }
      if (PINB & 0b00001000) bSetApertado = false;

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

  configRelogio = PADRAO;
  configAlarme = PADRAO;
  imprimeModo();
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
    if (configRelogio == PADRAO)  escreve_USART("RELOGIO PADRAO\n\0");
    else if (configRelogio == HORA) escreve_USART("CONFIG RELOGIO HORA\n\0");
    else if (configRelogio == MINUTO) escreve_USART("CONFIG RELOGIO MINUTO\n\0");

  }
  else if (modoAtual == CRONOMETRO) escreve_USART("CRONOMETRO\n\0");
  else if (modoAtual == ALARME) {
    if (configAlarme == PADRAO) escreve_USART("ALARME PADRAO\n\0");
    else if (configAlarme == HORA) escreve_USART("CONFIG ALARME HORA\n\0");
    else if (configAlarme == MINUTO) escreve_USART("CONFIG ALARME MINUTO\n\0");

  }
}
void ident_num(unsigned int valor, unsigned char *disp) {
  unsigned char n;

  for (n = 0; n < tam_vetor; n++)
    disp[n] = 0 + conv_ascii; //limpa vetor para armazenagem dos digitos

  *disp = valor / 10 + conv_ascii;
  disp++;
  *disp = valor % 10 + conv_ascii;
  /*do {
     disp = (valor % 10) + conv_ascii; //pega o resto da divisao por 10
    valor /= 10;   //pega o inteiro da divisão por 10
    disp++;

    } while (valor != 0);*/
}
void USART_Transmite(unsigned char dado) {
  while (!( UCSR0A & (1 << UDRE0)) ); //espera o dado ser enviado
  UDR0 = dado;          //envia o dado
}
void escreve_USART(char *c) {   //escreve String (RAM)
  for (; *c != 0; c++) USART_Transmite(*c);
}
unsigned char USART_Recebe() {
  while (!(UCSR0A & (1 << RXC0))); //espera o dado ser recebido
  return UDR0;        //retorna o dado recebido
}
void USART_Inic(unsigned int ubrr0) {
  UBRR0H = (unsigned char)(ubrr0 >> 8); //Ajusta a taxa de transmissão
  UBRR0L = (unsigned char)ubrr0;

  UCSR0A = 0;//desabilitar velocidade dupla (no Arduino é habilitado por padrão)
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); //Habilita a transmissão e a recepção
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00) | (1 << RXC0); /*modo assíncrono, 8 bits de dados, 1 bit de parada, sem paridade*/
}

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
        ident_num(hR, n);
        escreve_USART(n);
        escreve_USART(":\0");
        ident_num(mR, n);
        escreve_USART(n);
        escreve_USART(":\0");
        ident_num(sR, n);
        escreve_USART(n);
        escreve_USART("\n\0");
      } else {
        if (configRelogio == HORA) { // configurando hora
          if (sR % 2 == 0) {
            ident_num(hRC, n);
            escreve_USART(n); // Mostra hora
          } else escreve_USART("  \0"); //Esconde hora
        } else {
          ident_num(hRC, n);
          escreve_USART(n);
        }
        escreve_USART(":\0");
        if (configRelogio == MINUTO) { // condigurando minuto
          if (sR % 2 == 0) {
            ident_num(mRC, n);
            escreve_USART(n); //Mostra minuto
          }
          else escreve_USART("  \0"); //Esconde minuto

        } else {
          ident_num(mRC, n);
          escreve_USART(n);
        }

        escreve_USART(":\0");
        ident_num(sR, n);
        escreve_USART(n);
        escreve_USART("\n\0");
      }

    } else if (modoAtual == CRONOMETRO) { // Cronometro
      ident_num(hC, n);
      escreve_USART(n);
      escreve_USART(":\0");
      ident_num(mC, n);
      escreve_USART(n);
      escreve_USART(":\0");
      ident_num(sC, n);
      escreve_USART(n);
      escreve_USART("\n\0");

    } else if (modoAtual == ALARME) { // Alarme
      if (alarmeAtivo) {
        escreve_USART("ON \0");
      } else {
        escreve_USART("OFF \0");
      }
      if (configAlarme == PADRAO) { // modo padrão, mostra tudo
        ident_num(hA, n);
        escreve_USART(n);
        escreve_USART(":\0");
        ident_num(mA, n);
        escreve_USART(n);
        escreve_USART("\n\0");
      } else {
        if (configAlarme == HORA) { // configurando hora
          if (sR % 2 == 0)  {
            ident_num(hA, n);
            escreve_USART(n);// Mostra hora
          }
          else {
            escreve_USART("  \0"); //Esconde hora
          }

        } else {
          ident_num(hA, n);
          escreve_USART(n);
        }
        escreve_USART(":\0");
        if (configAlarme == MINUTO) { // condigurando minuto
          if (sR % 2 == 0) {
            ident_num(mA, n);
            escreve_USART(n);//Mostra minuto
            escreve_USART("\n\0");
          }
          else {
            escreve_USART("  \n\0"); //Esconde minuto
          }

        } else {
          ident_num(mA, n);
          escreve_USART(n);
          escreve_USART("\n\0");
        }
      }
    }

    if (mR == mA && hR == hA && alarmeAtivo) {
      ADCSRA |= 0b01000000;
      while (!(ADCSRA & 0b00010000));
      OCR1A =  ADC;
    } else {
      OCR1A = 1023;
    }
  }
}
