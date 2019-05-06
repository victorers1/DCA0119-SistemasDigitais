#include <iostream>
#include <signal.h>
#include <stdlib.h>

/* mraa headers */
#include "mraa/aio.hpp"
#include "mraa/common.hpp"

/* AIO ports */
#define AIO_PORT0 0
#define AIO_PORT1 1

using namespace std;

volatile sig_atomic_t flag = 1;

void sig_handler(int signum) {
  if (signum == SIGINT) {
    cout << "Exiting..." << std::endl;
    flag = 0;
  }
}

int main(void) {
  uint16_t LDR0, LDR1;
  
  signal(SIGINT, sig_handler);

  /* initialize AIO */
  mraa::Aio ldr0(AIO_PORT0);
  mraa::Aio ldr1(AIO_PORT1);

  while (flag) {
    LDR0 = ldr0.read();
    LDR1 = ldr1.read();
    printf("LDR0: %d", LDR0);
    printf("LDR1: %d", LDR1);
  }

  return EXIT_SUCCESS;
}