#include <stdio.h>


int a(int a) {
  a = 1;
  
  a * a;

  return a;
}

int main (int argc, char *argv[]) { 
  (void)argc;
  (void)argv;

  printf("main() hit!\n");

  return 0;
}
