
#include <stdio.h>

int fun(int arg)
{
   switch(3) {
      case 3: printf("%d\n", 2);
              break;
      case 2: printf("%d\n", 2);
              break;
   }

   switch(arg) {
      default:
         printf("%d\n", arg);
         return 2;
         break;
   }

   printf("not reachable\n");
   return 2;
}
