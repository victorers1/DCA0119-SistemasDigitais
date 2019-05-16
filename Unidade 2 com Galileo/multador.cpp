#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctime>

/* mraa headers */
#include "mraa/aio.hpp"
#include "mraa/common.hpp"
#include "mraa/gpio.hpp"

#define DELAY_MS 250

/* AIO ports */
#define AIO_PORT0 0
#define AIO_PORT1 1

/* gpio declaration */
#define GPIO_OUT_LDR0 7
#define GPIO_OUT_LDR1 8

#define GPIO_IN_ONOFF 6

using namespace std;

volatile sig_atomic_t flag = 1;

void inicial();
void desligado();
void ligado();
void botaoOnOff(void* args);
void sig_handler(int signum);

uint16_t LDR0, LDR1;
bool timer = false; //Indica se temporizador está ativo
bool S_ligado = false; // Chave geral do sistema
clock_t tempo;

mraa::Aio ldr0(AIO_PORT0);
mraa::Aio ldr1(AIO_PORT1);

/* initialize GPIO - pino 6 no galileo*/
mraa::Gpio gpio_24(GPIO_OUT_LDR0);
/* initialize GPIO - pino 7 no galileo*/
mraa::Gpio gpio_27(GPIO_OUT_LDR1);

/* initialize GPIO */
mraa::Gpio gpio(GPIO_IN_ONOFF);

static pthread_mutex_t time; // indica se contador já passou
pthread_t thread_contador;

int main(void){
  signal(SIGINT, sig_handler);

  inicial();

  while(1){
    desligado();
    ligado();
  }

  return EXIT_SUCCESS;
}

void* contaTempo(void *id){
  pthread_mutex_lock(&time);
  while(1){
    tempo = clock_t();
    usleep(DELAY_MS);
    tempo = clock_t() - temp;


  }

  pthread_mutex_unlock(&time);

  pthread_exit(NULL);
}

/**
 * Todas as inicializações
 */
void inicial(){
  /* initialize AIO */
  /* set GPIO to digital output */
  gpio_24.dir(mraa::DIR_OUT_LOW);
  //gpio_24.write(0);
  /* set gpio to digital output */
  gpio_27.dir(mraa::DIR_OUT_LOW);
  //gpio_27.write(0);

  // /* initialize GPIO */
  // mraa::Gpio gpio(GPIO_IN_ONOFF);

  /* set GPIO to digital input */
  gpio.dir(mraa::DIR_IN);
  /* configure ISR for GPIO */
  gpio.isr(mraa::EDGE_BOTH, &botaoOnOff,NULL);

}

/**
 * Comandos executados enquanto o sistema está desligado
 */
void desligado(){
  cout<<"Desligado"<<endl;
  while (!S_ligado){
    gpio_24.write(0);
  }
}

/**
 * Comandos executados enquanto o sistema está ligado
 */
void ligado() {
  cout<<"Ligado"<<endl;
  gpio_24.write(1);
  while (flag && S_ligado){
      LDR0 = ldr0.read();
      LDR1 = ldr1.read();

      if (LDR0 < 400 && !timer){
        gpio_27.write(1);
      }
      if (LDR1 < 400 && timer){
        gpio_27.write(0);
      }
    }
}

void botaoOnOff(void* args){
  //cout<<"teste"<<endl;
  while(gpio.read()==0){}
  S_ligado = !S_ligado;
  //gpio_24.write(S_ligado);
  int cont = 0;
  while(gpio.read()==1){
    cont++;
    if(cont==10){
      break;
    }
  }
  cout<<"Botao acionado\n";
}

void sig_handler(int signum){
  if (signum == SIGINT){
    cout << "Exiting..." << std::endl;
    flag = 0;
    gpio_24.write(0);
    gpio_27.write(0);
    //finalizar();
    exit(0);
  }
}
