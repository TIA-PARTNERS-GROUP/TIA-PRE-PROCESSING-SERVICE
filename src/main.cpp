#include <stdio.h>


void a(int a) {
  printf"%d", a);
}

int main (int argc, char *argv[]) { 
  (void)argc;
  (void)argv;

  printf("main() hit!\n");

  return 0;
}
