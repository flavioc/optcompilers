

int main(int argc, char **argv)
{
   int i;
   for(i = 0; i < 5; ++i) {
      int b = 2;
      int c = b + 4;

      b = b + c;
   }
   return 0;
}
