#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

/* mraa headers */
#include "mraa/aio.hpp"
#include "mraa/common.hpp"
#include "mraa/gpio.hpp"

#define DELAY_US  100000// intervalo de delay em microsegundos

/* AIO ports */
#define AIO_PORT0 0
#define AIO_PORT1 1

/* gpio declaration */
#define GPIO_OUT_LDR0 7
#define GPIO_OUT_LDR1 8

#define GPIO_IN_ONOFF 5

using namespace std;

volatile sig_atomic_t flag = 1;

void inicial();
void desligado();
void ligado();
void botaoOnOff(void* args);
void sig_handler(int signum);

uint16_t LDR0, LDR1;
bool timer = false; //Indica se temporizador está ativo
bool S_ligado = true; // Chave geral do sistema
float tempo_passagem = 0; // tempo que o objeto leva para passsar em ambos LDRs
long long tempo_ms = 0; //tempo decorrido desde a inicialização do sistema
float vel = 0;

mraa::Aio ldr0(AIO_PORT0);
mraa::Aio ldr1(AIO_PORT1);

/* initialize GPIO - pino 6 no galileo*/
mraa::Gpio gpio_24(GPIO_OUT_LDR0);
/* initialize GPIO - pino 7 no galileo*/
mraa::Gpio gpio_27(GPIO_OUT_LDR1);

/* initialize GPIO */
mraa::Gpio gpio(GPIO_IN_ONOFF);

//pthread_mutex_t time; // indica se contador terminou contagem
pthread_t tid;
pthread_t tmain;

void* threadPrincipal(void *id){
  while(1){
    desligado();
    ligado();
  }

  pthread_exit(NULL);
}

void* contaTempo(void *id){
  while(1){
    usleep(DELAY_US);
    tempo_ms+=DELAY_US/1000;
    cout<<"Tempo: "<<tempo_ms/1000<<" s"<<endl;
  }

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
  mraa::Gpio gpio(GPIO_IN_ONOFF);

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
 * Comandos executados enquanto o sistema está ligado.
 * Faz repetidas leituras dos LDRs.
 */
void ligado() {
  cout<<"Ligado"<<endl;
  gpio_24.write(1);
  while (flag && S_ligado){
      LDR0 = ldr0.read();
      LDR1 = ldr1.read();

      if (LDR0 < 400 && !timer){
        gpio_27.write(1);
        timer=true;
        tempo_passagem = tempo_ms;
        cout<<"tempo_inicial: "<<tempo_passagem<<endl;
      }
      if (LDR1 < 400 && timer){
        gpio_27.write(0);
        timer=false;
        cout<<"tempo_final: "<<tempo_ms<<endl;
        tempo_passagem = (tempo_ms - tempo_passagem)/1000; // tempo_passagem em segundos
        cout<<"tempo_passagem: "<<tempo_passagem<<endl;
        vel = 1.0/tempo_passagem*3.6;
        cout<<"Velocidade = " << vel << " km/h\n";

        if(vel>80){
          cout<<"Infração detectada! Velocidade "<<vel<<" km/h é acima da permitida (80 km/h)\n";
        }
          
        
      }
    }
}

void botaoOnOff(void* args){
  // while(gpio.read()==0){}
  S_ligado = !S_ligado;
  
  // int cont = 0;
  // while(gpio.read()==1){
  //   cont++;
  //   if(cont==10){
  //     break;
  //   }
  // }
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

int main(void){
  signal(SIGINT, sig_handler);

  inicial();
  pthread_create(&tid, NULL, contaTempo, NULL);
  pthread_create(&tmain, NULL, threadPrincipal, NULL);
  
  pthread_join(tmain, NULL);
  pthread_join(tid, NULL);

  return EXIT_SUCCESS;
}


