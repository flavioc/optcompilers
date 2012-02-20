
int donothing(void)
{
   return 2;
}

int stupid(int x)
{
   int a = x + 2;

   if(a > 5) {
      int c = 5;
      c += 5;
   }

   if(x < 10) {
      int d = donothing();
   }

   return x;
}

int main(int argc, char **argv)
{
   int i;
   for(i = 0; i < 100; ++i)
      stupid(i);

   return 0;
}
