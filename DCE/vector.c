
#include <stdio.h>

int main(int argc, char **argv)
{
   int a[2] = {argc, 2};
   int b;

   a[1] = a[1] - 1;

   if(a[1]) {
      printf("YES!\n");
      b = b + 1;
   } else {
      printf("NO!\n");
   }

   return 0;
}
