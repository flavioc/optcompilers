#include <stdio.h>

#define SIZE_ARRAY 200

int faint (int x) 
{
   int i;
   int arr[SIZE_ARRAY];
  int a = x + 1;

   for(i = 0; i < SIZE_ARRAY; ++i) {
     int b = a + 2;
     int c = b - 3;
     int d = b + c;
     int e = d * x; 

     arr[i] = e;
   }
  return x + a;
}

#define LOOP_COUNT 1000000

int main (int argc, char **argv)
{
  int i, j;
  int result = 0;
  printf ("Multiple Program \n");
  for (i = 0; i < LOOP_COUNT; i++) {
   result += faint (2);
  }
  return result;
}
