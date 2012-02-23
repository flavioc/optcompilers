
#include <stdio.h>

int fun(int a)
{
   int n = (int)((float)a + 2.5);

   printf("%f\n", (float)a);
   return a * 2;
}

int
main(int argc, char **argv)
{
   return fun(argc);
}
