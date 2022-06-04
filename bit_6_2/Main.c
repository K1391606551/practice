#include"game.h"
#include"PracBar.h"
 int main()
{
  int select = 0;
  do 
  {
    Menu();
    scanf("%d",&select);
    switch(select)
    {
      case 1:
        process_bar();
        Game();
        break;
      case 0:
        printf("Bye\n");
        break;
      default:
        printf("Enter Error, Try Again!\n");
        break;

    }
  }while(select);
  return 0;
}
