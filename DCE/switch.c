
#include <stdio.h>

void
dofun(int n)
{
   switch(n) {
      case 1: {
                 int a = n + 1;

                 printf("%d\n", a);
              }
         break;
      case 2:
         break;
      case 3: {
                 printf("%d\n", n + 1);
              }
              break;
   }
}

int
main(int argc, char **argv)
{
   dofun(argc);
   return 0;
}
