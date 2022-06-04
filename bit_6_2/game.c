#include"game.h"
int x = 0, y = 0;
void Menu()
{
  printf("#######################\n");
  printf("##1.play     0.exit  ##\n");
  printf("#######################\n");
  printf("Please Select\n");
}


void ShowBoard(int board[][COL], int row, int col)
{

  printf("\e[1;1H\e[2J");
  printf(" ");
  int i = 1;
  for(; i <= col; i++)
  {
    printf("%3d", i);
  }
  printf("\n");
  for(i = 0; i < row; i++)
  {
    printf("%2d",i + 1);
    int j = 0;
    for(; j < col; j++)
    {
      if(board[i][j] == 0)
      {
        printf(" . ");
      }
      else if(board[i][j] == PLAYER1)
      {
        printf(" ● ");
      }
      else 
      {
        printf(" ○ ");
      }
    }
    printf("\n");
  }

}

void PlayerMove(int board[][COL], int row, int col, int who)
{
  while(1)
  {
    printf("Player[%d] Please Enter pos#\n", who);
    scanf("%d%d", &x, &y);
    if(x < 1 || x > row || y < 1 || y > col)
    {
      printf("Pos is Not Right!\n");
      continue;
    }
    if(board[x - 1][y - 1] != 0)
    {
      printf("Pos is occupied!\n");
      continue;
    }
    else
    {
      board[x -1][y - 1] = who;
    }
    break;
  }
}
int ChessCount(int board[][COL], int row, int col, enum pat a)
{
  int _x = x , _y = y;
  int count = 0;
  while(1)
  {
    switch(a)
    {
      case LEFT:
        --_y;
        break;
      case RIGHT:
        ++_y;
        break;
      case UP:
        --_x;
        break;
      case DOWN:
        ++_x;
        break;
      case LEFT_UP:
        --_x, --_y;
       break;
      case LEFT_DOWN:
        --_y, ++_x;
        break;
      case RIGHT_UP:
        ++_y, --_x;
        break;
      case RIGHT_DOWN:
        ++_y, ++_x;
       break;
      default:
       //Do nothing
       break;
    }
    if((_x >= 1 || _x <= row) && (_y >= 1 || _y <= col))
    {
      if(board[_x - 1][_y - 1] == board[x - 1][y - 1])
      {
        count++;
      }
      else 
      {
        break;
      }
    }
    else 
    {
      return count;
    }
  }
  return count;
}
int IsOver(int board[][COL], int row, int col)
{
  int count1 = ChessCount(board, row, col, LEFT) + ChessCount(board, row, col, RIGHT) + 1;
  int count2 = ChessCount(board, row, col, UP) + ChessCount(board, row, col, DOWN) + 1;
  int count3 = ChessCount(board, row, col,LEFT_UP) + ChessCount(board, row, col, RIGHT_DOWN) + 1;
  int count4 = ChessCount(board, row, col,LEFT_DOWN) + ChessCount(board, row, col,RIGHT_UP) + 1;

  if(count1 >= 5 || count2 >= 5 || count3 >= 5 || count4 >= 5)
  {
    if(board[x - 1][y - 1] == PLAYER1)
    {
    
      return PLAYER1_WIN; 
    }
    else 
    {
      return PLAYER2_WIN;
    }
  }
  
 int i = 0;
 for(; i < row; i++)
 {
   int j = 0;
   for(;j < col; j++)
   {
     if(board[i][j] == 0)
     {
       return NEXT;
     }
   }
 }
return DRAW;

}


void Game()
{
  int board[ROW][COL];
  memset(board, 0, sizeof(board));
  int result = NEXT;

  do
  {
    ShowBoard(board, ROW, COL);
    PlayerMove(board, ROW, COL, PLAYER1);
    result = IsOver(board, ROW, COL);
    if(NEXT != result)
    {
      break;
    }
    ShowBoard(board, ROW, COL);
    PlayerMove(board, ROW, COL, PLAYER2);
    if(NEXT != result)
    {
      break;
    }
  }while(1);

    ShowBoard(board, ROW, COL);
//输出游戏结果
  switch(result)
  {
    case PLAYER1_WIN:
      printf("PLAYER1 is winer\n");
      break;
    case PLAYER2_WIN:
      printf("PLAYER2 is winer\n");
      break;
    case DRAW:
      printf("This is a draw\n");
    default:
      break;
  }
}
