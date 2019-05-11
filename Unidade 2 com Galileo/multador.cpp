#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <thread>
#include <ctime>

/* mraa headers */
#include "mraa/aio.hpp"
#include "mraa/common.hpp"
#include "mraa/gpio.hpp"

/* AIO ports */
#define AIO_PORT0 0
#define AIO_PORT1 1

/* gpio declaration */
#define GPIO_OUT_LDR0 24
#define GPIO_OUT_LDR1 27

define GPIO_IN_ONOFF 6

using namespace std;

volatile sig_atomic_t flag = 1;

void sig_handler(int signum){
  if (signum == SIGINT){
    cout << "Exiting..." << std::endl;
    flag = 0;
  }
}

int main(void){
  uint16_t LDR0, LDR1;
  boolean timer = false; //Indica se temporizador está ativo
  boolean S_ligado = false; // Chave geral do sistema
  clock_t t;

  signal(SIGINT, sig_handler);

  inicial();

  while(1){
    desligado();
    ligado();
  }

  return EXIT_SUCCESS;
}

/**
 * Todas as inicializações
 */
void inicial(){
  /* initialize AIO */
  mraa::Aio ldr0(AIO_PORT0);
  mraa::Aio ldr1(AIO_PORT1);

  /* initialize GPIO - pino 6 no galileo*/
  mraa::Gpio gpio_24(GPIO_OUT_LDR0);
  /* initialize GPIO - pino 7 no galileo*/
  mraa::Gpio gpio_27(GPIO_OUT_LDR1);

  /* set GPIO to digital output */
  gpio_1.dir(mraa::DIR_OUT);
  /* set gpio to digital output */
  gpio_2.dir(mraa::DIR_OUT);


  /* initialize GPIO */
  mraa::Gpio gpio(GPIO_IN_ONOFF);

  /* set GPIO to digital input */
  gpio.dir(mraa::DIR_IN);
  /* configure ISR for GPIO */
  gpio.isr(mraa::EDGE_BOTH, &botaoOnOff, NULL);
}

/**
 * Comandos executados enquanto o sistema está desligado
 */
void desligado(){
while (!S_ligado){
  }
}

/**
 * Comandos executados enquanto o sistema está ligado
 */
void ligado(){
while (flag && S_ligado){
    LDR0 = ldr0.read();
    LDR1 = ldr1.read();

    if (LDR0 < 400){
      timer = true;
      t = clock();
    }
    if (LDR1 < 400 && timer){
      timer = false;
      t = t - clock();
      cout<<"Tempo: "<<t<<endl;
    }
    cout << "LDR0: " << LDR0 << " - LDR1: " << LDR1 << endl;
  }
}

void botaoOnOff(){
  S_ligado = !S_ligado;
}
