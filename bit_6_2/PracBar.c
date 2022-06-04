#include"PracBar.h"

void process_bar()
 {
  char bar[NUM];
  memset(bar, '\0', sizeof(bar));
  int i = 0;
  const char *lable = "|/-\\";
  printf("Loading...\n");
  while(i <= 100)
  {
    printf("[%-100s][%d%%][%c]\r",bar,i,lable[i % 4]);
    fflush(stdout);
    bar[i++] = '#';
    usleep(30000);
  }
  printf("\n");
}
