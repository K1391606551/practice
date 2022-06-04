#pragma once

#include<stdio.h>
#include<string.h>

#define ROW 20
#define COL 20

#define PLAYER1 1
#define PLAYER2 2


#define NEXT        0
#define PLAYER1_WIN 1
#define PLAYER2_WIN 2
#define DRAW        3

enum pat
{
  LEFT,
  RIGHT,
  UP,
  DOWN,
  LEFT_UP,
  LEFT_DOWN,
  RIGHT_UP,
  RIGHT_DOWN
};

void Menu();
void Game();
