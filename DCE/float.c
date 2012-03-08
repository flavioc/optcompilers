#include <stdio.h>

float faint (float x) 
{
  float a = x + 1.0;
  float b = a + 2.0;
  float c = b - 3.0;
  float d = b + c;
  float e = d * x; 
  return x + a;
}

#define LOOP_COUNT 1000000

int main (int argc, char **argv)
{
  int i, j;
  int result = 0;
  printf ("Float Program \n");
  for (i = 0; i < LOOP_COUNT; i++) {
   for (j = 0; j < 100; j++) {
      result += faint (2);
   }
  }
  return result;
}
