#if 0
int g;
int g_incr (int c)
{
   g += c;
}

int loop(int a, int b, int c)
{
   int i;
   int ret = 0;
   for(i = a; i < b; i++) {
      g_incr(c);
   }
   return ret + g;
}


int loop2(int a, int b, int c)
{
   int i;
   int v;
   int ret = 0;
   for(i = a; i < b; i++) {

       int j = 0;
       ret = 3 * i;
       j = ret - 1;
       v = g_incr(j);
   }

   printf("%i", v);
   return ret + g;
}

int loop3(int a, int b, int c)
{
   int i;
   int v;
   int ret = 0;

   for(i = a; i < b; i++) {
       if( i * c > 0 )
       {
           ret = i * ret;
       }
       else
       {
           ret = i - c;
       }

       print("%i",ret);

   }


   return ret + g;
}

#endif

int loop4(int a, int b, int c)
{
    int inv = a +b;

   for(; a < b; a++)
   {
       inv = c + b;
       //printf("%i, %i",a,inv);
   }
   printf("%i",c);
   return inv;
}
