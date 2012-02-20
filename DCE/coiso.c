
int bah(void)
{
   int x = 2;

   {
      int x = 5;

      int a = x + 5;
   }

   return x;
}
