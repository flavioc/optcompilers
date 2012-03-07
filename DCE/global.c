
static int global = 1;

int
main(int argc, char **argv)
{
   int a = argc;

   global++;

   return a + 2;
}
