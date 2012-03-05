#include <stdio.h>

void dosomethingelse(void)
{
   printf("ah\n");
}

void dosomethingfancy(int i, int j, int p)
{
    printf("ah\n %i %i %i",i,j,p);
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

void superfancy(int i, int l, int r)
{
    int j;

    if(i > 0)
        i=l * 4;
    else
        i=r * 7;

    j = i + 5;

    print("%i",j);
}
