#include <stdio.h>

int faint (int x) 
{
  int a = x + 1;
  int b = a + 2;
  int c = b - 3;
  int d = b + c;
  int e = d * x; 
  return x + a;
}

#define LOOP_COUNT 1000000
int main (int argc, char **argv)
{
  int i, j;
  int result = 0;
  printf ("Faint Program \n");
  for (i = 0; i < LOOP_COUNT; i++) {
   for (j = 0; j < 100; j++) {
      result += faint (2);
   }
  }
  return result;
}
