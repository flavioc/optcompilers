#include <stdio.h>

void dosomethingelse(void)
{
   printf("ah\n");
}

void dosomething(void)
{
   dosomethingelse();
}

void
loop(void)
{
   int i;
   for(i = 0; i < 5; ++i)
      dosomething();
}
