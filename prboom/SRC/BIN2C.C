#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc,char **argv)
{
  char buf[256],s[100];
  int c;
  FILE *fp;
  if (argc!=2)
    {
      fprintf(stderr,"Usage: %s file\n",*argv);
      return 1;
    }
  if (!(fp = fopen(argv[1],"rb")))
    {
      fputs("Cannot open ",stderr);
      perror(argv[1]);
      return 1;
    }
  puts("static unsigned char data[] = {");
  strcpy(buf,"  ");
  while ((c=getc(fp))!=EOF)
    {
      sprintf(s,"%u,",(unsigned char) c);
      if (strlen(s)+strlen(buf) >= 80)
	puts(buf), strcpy(buf,"  ");
      strcat(buf,s);
    }
  if (*buf)
    strcat(buf,"\n");
  printf("%s};\n",buf);
  return 0;
}
