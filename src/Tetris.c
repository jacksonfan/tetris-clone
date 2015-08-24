/*
 * Tetris.c
 * UCR 120B Final Project
 * Created by Jackson Fan, 861115278
 * 
 * Utilized header files 5110.h, 5110.c, timeout.h for Nokia 5110
 * By Radu Motisan at http://www.pocketmagic.net/atmega8-and-nokia-5110-lcd/
 */ 

#include <avr/io.h>
#include "includes/Tetris.h"

#define GSHARP4 415.30
#define A4 440.00
#define B4 493.88
#define C5 523.25
#define D5 587.33
#define E5 659.25
#define F5 698.46
#define G5 783.99
#define GSHARP5 830.61
#define A5 880.00

double freqs[] = {E5, B4, C5, D5, C5, B4, A4, A4, C5, E5, D5, C5, B4, C5, D5, E5, 
	C5, A4, A4, A4, B4, C5, D5, F5, A5, G5, F5, E5, C5, E5, D5, C5, B4, B4, C5, D5, 
	E5, C5, A4, A4, 0, E5, C5, D5, B4, C5, A4, GSHARP4, B4, 0, E5, C5, D5, B4, C5, 
	E5, A5, GSHARP5, 0, E5, B4, C5, D5, C5, B4, A4, A4, C5, E5, D5, C5, B4, C5, D5, 
	E5, C5, A4, A4, A4, B4, C5, D5, F5, A5, G5, F5, E5, C5, E5, D5, C5, B4, B4, C5, 
	D5, E5, C5, A4, A4, 0};

double note_Held[] = {2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 3, 1, 2, 2, 2, 2, 1, 1, 
	1, 1, 3, 1, 2, 1, 1, 3, 1, 2, 1, 1, 2, 1, 1, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 
	4, 4, 2, 2, 4, 4, 4, 4, 2, 2, 4, 4, 4, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 3, 
	1, 2, 2, 2, 2, 1, 1, 1, 1, 3, 1, 2, 1, 1, 3, 1, 2, 1, 1, 2, 1, 1, 2, 2, 2, 2, 
	2, 5};

unsigned char temp = 0x00;
unsigned char count = 0x00;
unsigned char downCount = 0x00;

//Constants
const unsigned NEXT_X = 65;
const unsigned NEXT_Y[] = {11, 23, 35};
const unsigned HOLD_X = 3;
const unsigned HOLD_Y = 11;

//Inner bounds
const unsigned char topBorder = 4;
const unsigned char leftBorder = 22;
const unsigned char rightBorder = 61;
const unsigned char bottomBorder = 43;

typedef enum { L_SHAPE, SQUARE, LINE, T_SHAPE, ZIG_ZAG, ZAG_ZIG, EMPTY } BlockType;

typedef struct {
	int x;
	int y;
	int rotated;
	BlockType block;
} currentBlock;

currentBlock currBlock;
unsigned int gameScore = 0;
unsigned char loseFlag = 0;

BlockType nextBlocks[3] = {0};
BlockType holdBlock = EMPTY;

unsigned char button = 0x00;
unsigned char prevPress = 0x00;
unsigned char prevGameState = 0xFF;

/*
unsigned char* FIRSTNEXT[32] = {0};
unsigned char* SECONDNEXT[32] = {0};
unsigned char* THIRDNEXT[32] = {0};

unsigned char** allNextBlocks[3] = {0};

void fillNexts()
{
	int index = NEXT_X + NEXT_Y[0] / CHAR_HEIGHT * LCD_WIDTH;
	int index1 = NEXT_X + NEXT_Y[1] / CHAR_HEIGHT * LCD_WIDTH;
	int index2 = NEXT_X + NEXT_Y[2] / CHAR_HEIGHT * LCD_WIDTH;
	unsigned char i = 0;
	
	for (i = 0; i < 16; i++)
	{
		FIRSTNEXT[i] = playScreen + index;
		SECONDNEXT[i] = playScreen + index1;
		THIRDNEXT[i] = playScreen + index2;
	}
	for (i = 0; i < 16; i++)
	{
		FIRSTNEXT[i+16] = playScreen + index + LCD_WIDTH;
		SECONDNEXT[i+16] = playScreen + index1 + LCD_WIDTH;
		THIRDNEXT[i+16] = playScreen + index2 + LCD_WIDTH;
	}
	allNextBlocks[0] = FIRSTNEXT;
	allNextBlocks[1] = SECONDNEXT;
	allNextBlocks[2] = THIRDNEXT;
}
*/


void updateNextBlocks()
{
	unsigned char i = 0;
	for (i = 1; i < 3; i++)
	{
		nextBlocks[i-1] = nextBlocks[i];
	}
	nextBlocks[2] = EMPTY;
	for (i = 0; i < 3; i++)
	{
		if (nextBlocks[i] == EMPTY)
		{
			nextBlocks[i] = (BlockType)(rand() % 6);
		}
	}
}

void displayCurrentScreen(unsigned char *data)
{
	int pixel_cols = LCD_WIDTH * (LCD_HEIGHT / CHAR_HEIGHT); //<6> means 6 lines on LCD.
	lcd_goto_xy(0, 0);
	
	for(int i=0;i<pixel_cols;i++)
	lcd_col(*(data++));
}

void replacePixel(int x, int y, unsigned char bit) { //Modified version of printPictureOnLCD
	int index = x + y / CHAR_HEIGHT * LCD_WIDTH;
	if (bit)
		playScreen[index] = playScreen[index] | (1 << (y % CHAR_HEIGHT));
	else
		playScreen[index] = playScreen[index] & ~(1 << (y % CHAR_HEIGHT));
}

unsigned char isPixelBlack(int x, int y)
{
	int index = x + y / CHAR_HEIGHT * LCD_WIDTH;
	return ((playScreen[index] >> (y % CHAR_HEIGHT)) & 0x01);
}

void drawBlockHere(int x, int y, BlockType block)
{
	unsigned char i = 0, j = 0, bit = 0;
	switch(block) {
		case L_SHAPE:
			for (i = 0; i < 16; i++)
			{
				if (i > 11)
					bit = 0;
				else
					bit = 1;
				for (j = 0; j < 8; j++)
				{
					if ((j > 3 && i < 4) || (j < 4 && i < 12))
						bit = 1;
					else
						bit = 0;
					replacePixel(x+i, y+j, bit);
				}
			}
			break;
		case SQUARE:
			for (i = 0; i < 16; i++)
			{
				if (i < 8)
					bit = 1;
				else
					bit = 0;
				for (j = 0; j < 8; j++)
				{
					replacePixel(x+i, y+j, bit);
				}
			}
			break;
		case T_SHAPE:
			for (i = 0; i < 16; i++)
			{
				if (i > 11)
					bit = 0;
				else
					bit = 1;
				for (j = 0; j < 8; j++)
				{
					if ((i > 3 && i < 8) || (j < 4 && i < 12))
						bit = 1;
					else 
						bit = 0;
					replacePixel(x+i, y+j, bit);
				}
			}
			break;
		case LINE:
			for (i = 0; i < 16; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (j > 3)
						bit = 0;
					else
						bit = 1;
					replacePixel(x+i, y+j, bit);
				}
			}
			break;
		case ZIG_ZAG:
			for (i = 0; i < 16; i++)
			{
				if (i < 12)
					bit = 1;
				else
					bit = 0;
				for (j = 0; j < 8; j++)
				{
					if ((j > 3 && i < 8) || (j < 4 && i < 12 && i > 3))
						bit = 1;
					else
						bit = 0;
					replacePixel(x+i, y+j, bit);
				}
			}
			break;
		case ZAG_ZIG:
			for (i = 0; i < 16; i++)
			{
				if (i < 12)
					bit = 1;
				else
					bit = 0;
				for (j = 0; j < 8; j++)
				{
					if ((j < 4 && i < 8) || (j > 3 && i < 12 && i > 3))
						bit = 1;
					else
						bit = 0;
					replacePixel(x+i, y+j, bit);
				}
			}
			break;
		case EMPTY:
			bit = 0;
			for (i = 0; i < 16; i++)
			{
				for (j = 0; j < 8; j++)
				{
					replacePixel(x+i, y+j, bit);
				}
			}
			
			
	}
}

void drawBlock_NoSpace(int x, int y, BlockType block)
{
	unsigned char i = 0, j = 0, bit = 0;
	switch(block) {
		case L_SHAPE:
			for (i = 0; i < 12; i++)
			{
				bit = 1;
				for (j = 0; j < 8; j++)
				{
					if ((j > 3 && i < 4) || (j < 4))
						bit = 1;
					else
						bit = 0;
					if (bit)
						replacePixel(x+i, y+j, bit);
				}
			}
			break;
		case SQUARE:
			for (i = 0; i < 8; i++)
			{
				bit = 1;
				for (j = 0; j < 8; j++)
				{
					if (bit)
						replacePixel(x+i, y+j, bit);
				}
			}
			break;
		case T_SHAPE:
			for (i = 0; i < 12; i++)
			{
				bit = 1;
				for (j = 0; j < 8; j++)
				{
					if ((i > 3 && i < 8) || (j < 4))
						bit = 1;
					else 
						bit = 0;
					if (bit)
						replacePixel(x+i, y+j, bit);
				}
			}
			break;
		case LINE:
			for (i = 0; i < 16; i++)
			{
				for (j = 0; j < 4; j++)
				{
					bit = 1;
					replacePixel(x+i, y+j, bit);
				}
			}
			break;
		case ZIG_ZAG:
			for (i = 0; i < 12; i++)
			{
				bit = 1;
				for (j = 0; j < 8; j++)
				{
					if ((j > 3 && i < 8) || (j < 4 && i > 3))
						bit = 1;
					else
						bit = 0;
					if (bit)
						replacePixel(x+i, y+j, bit);
				}
			}
			break;
		case ZAG_ZIG:
			for (i = 0; i < 12; i++)
			{
				bit = 1;
				for (j = 0; j < 8; j++)
				{
					if ((j < 4 && i < 8) || (j > 3 && i > 3))
						bit = 1;
					else
						bit = 0;
					if (bit)
						replacePixel(x+i, y+j, bit);
				}
			}
			break;
		case EMPTY:
			bit = 0;
			for (i = 0; i < 4; i++)
			{
				for (j = 0; j < 4; j++)
				{
					replacePixel(x+i, y+j, bit);
				}
			}
	}
}

void changeHoldBox()
{
	drawBlockHere(HOLD_X, HOLD_Y, holdBlock);
}

void clearCurrBlock()
{
	unsigned char i = 0, j = 0;
	switch(currBlock.block) {
		case SQUARE:
			for (i = 0; i < 8; i++)
			{
				for (j = 0; j < 8; j++)
				{
					replacePixel(currBlock.x+i, currBlock.y+j, 0);
				}
			}
			break;
		case LINE:
			for (i = 0; i < 16; i++)
			{
				for (j = 0; j < 4; j++)
				{
					replacePixel(currBlock.x+i, currBlock.y+j, 0);
				}
			}
			break;
		case L_SHAPE:
			for (i = 0; i < 12; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (!(j > 3 && i > 3))
						replacePixel(currBlock.x+i, currBlock.y+j, 0);
				}
			}
			break;
		case T_SHAPE:
			for (i = 0; i < 12; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (!((j > 3) && (i < 4 || i > 7)))
						replacePixel(currBlock.x+i, currBlock.y+j, 0);
				}
			}
			break;
		case ZIG_ZAG:
			for (i = 0; i < 12; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (!((j < 4 && i < 4) || (j > 3 && i > 7)))
						replacePixel(currBlock.x+i, currBlock.y+j, 0);
				}
			}
			break;
		case ZAG_ZIG:
			for (i = 0; i < 12; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (!((i < 4 && j > 3) || (i > 7 && j < 4)))
						replacePixel(currBlock.x+i, currBlock.y+j, 0);
				}
			}
			break;
		case EMPTY:
			for (i = 0; i < 4; i++)
			{
				for (j = 0; j < 4; j++)
				{
					replacePixel(currBlock.x+i, currBlock.y+j, 0);
				}
			}
	}
}

void drawNextBlocks()
{
	unsigned char i = 0;
	for (i = 0; i < 3; i++)
	{
		drawBlockHere(NEXT_X, NEXT_Y[i], nextBlocks[i]);
	}
}

unsigned char isAreaClear(int x, int y)
{
	unsigned char i = 0, j = 0;
	switch(currBlock.block) {
		case LINE:
			for (i = 0; i < 16; i++)
			{
				for (j = 0; j < 4; j++)
				{
					if (isPixelBlack(x+i, y+j))
					{
						return 0;
					}
				}
			}
			break;
		case SQUARE:
			for (i = 0; i < 8; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (isPixelBlack(x+i, y+j))
					{
						return 0;
					}
				}
			}
			break;
		case L_SHAPE:
			for (i = 0; i < 12; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (!(j > 3 && i > 3))
					{
						if (isPixelBlack(x+i, y+j))
						{
							return 0;
						}
					}
				}
			}
			break;
		case T_SHAPE:
			for (i = 0; i < 12; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (!((j > 3) && (i < 4 || i > 7)))
					{
						if (isPixelBlack(x+i, y+j))
						{
							return 0;
						}
					}
				}
			}
			break;
		case ZIG_ZAG:
			for (i = 0; i < 12; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (!((j < 4 && i < 4) || (j > 3 && i > 7)))
					{
						if (isPixelBlack(x+i, y+j))
						{
							return 0;
						}
					}
				}
			}
			break;
		case ZAG_ZIG:
			for (i = 0; i < 12; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (!((i < 4 && j > 3) || (i > 7 && j < 4)))
					{
						if (isPixelBlack(x+i, y+j))
						{
							return 0;
						}
					}
				}
			}
			break;
		case EMPTY:
			for (i = 0; i < 12; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (isPixelBlack(x+i, y+j));
				}
			}
	}
	return 1;
}

void dropNewBlock()
{
	if (currBlock.block == EMPTY || (isAreaClear(currBlock.x, currBlock.y) == 0))
		currBlock.block = nextBlocks[0];
	currBlock.y = 4;
	currBlock.rotated = 0;
	if (currBlock.block == SQUARE || currBlock.block == ZIG_ZAG || currBlock.block == ZAG_ZIG)
	{
		currBlock.x = 38;
	}
	else
	{
		currBlock.x = 34;
	}
	drawBlock_NoSpace(currBlock.x, currBlock.y, currBlock.block);
}

void display_Score()
{
	unsigned char cursor = 1;
	unsigned int tempScore = gameScore;
	LCD_DisplayString(1, "Score:");
	cursor = 8;
	LCD_Cursor(cursor);
	if (tempScore > 99)
	{
		LCD_WriteData( (tempScore/100) + '0' );
		tempScore -= ((tempScore/100)*100);
		LCD_Cursor(++cursor);
	}
	if (tempScore > 9)
	{
		LCD_WriteData ( (tempScore/10) + '0' );
		tempScore = tempScore -  ((tempScore/10) * 10);
		LCD_Cursor(++cursor);
	}
	if (tempScore == 0)
	{
		LCD_WriteData ( '0' );
	}
	LCD_Cursor(0);
}

void checkForLines()
{
	unsigned char i = 0, j = 0, line = 0, eraseFrom = 0, eraseTo = 0;
	for (j = topBorder; j <= bottomBorder; j+=4)
	{
		for (i = leftBorder; i <= rightBorder; i+=4)
		{
			if (isPixelBlack(i, j))
			{
				line++;
			}
		}
		if (line == 10)
		{
			if (eraseFrom == 0)
			{
				eraseFrom = j;
			}
			gameScore = gameScore + 10;
			display_Score();
		}
		else if ((line != 10) && (eraseFrom != 0))
		{
			eraseTo = j;
		}
		line = 0;
	}
	for (j = eraseFrom; j < eraseTo; j++)
	{
		for (i = leftBorder; i <= rightBorder; i++)
		{
			replacePixel(i, j, 0);
		}
	}
	for (j = eraseTo-1; j >= topBorder+(eraseTo-eraseFrom); j--)
	{
		for (i = leftBorder; i <= rightBorder; i++)
		{
			replacePixel(i, j, isPixelBlack(i, j-(eraseTo-eraseFrom)));
		}
	}
}

void moveBlock()
{
	if (currBlock.block != EMPTY)
	{
		if (button & (0x01<<5)) //Press Left
		{
			clearCurrBlock();
			if ((currBlock.x > leftBorder) && isAreaClear(currBlock.x-4, currBlock.y))
			{
				//drawBlockHere(currBlock.x, currBlock.y, EMPTY);
				currBlock.x -= 4;
			}
		}
		else if (button & (0x01<<4))
		{
			clearCurrBlock();
			if (currBlock.block == LINE)
			{
				if ((currBlock.y < bottomBorder-4) && isAreaClear(currBlock.x, currBlock.y+4))
				{
					currBlock.y += 4;
					if (!(currBlock.y < bottomBorder-4))
					{
						drawBlock_NoSpace(currBlock.x, currBlock.y, currBlock.block);
						checkForLines();
						dropNewBlock();
						updateNextBlocks();
						drawNextBlocks();
						changeHoldBox();
					}
				}
				else if ((isAreaClear(currBlock.x, currBlock.y+4) == 0) && (currBlock.y-4 < topBorder))
				{
					loseFlag = 1;
				}
				else if (isAreaClear(currBlock.x, currBlock.y+4) == 0)
				{
					drawBlock_NoSpace(currBlock.x, currBlock.y, currBlock.block);
					checkForLines();
					dropNewBlock();
					updateNextBlocks();
					drawNextBlocks();
					changeHoldBox();
				}
			}
			else if ((currBlock.y < bottomBorder-8) && isAreaClear(currBlock.x, currBlock.y+4))
			{
				currBlock.y += 4;
				/*if (!(currBlock.y < bottomBorder-8))
				{
					drawBlock_NoSpace(currBlock.x, currBlock.y, currBlock.block);
					checkForLines();
					dropNewBlock();
					updateNextBlocks();
					drawNextBlocks();
					changeHoldBox();
				}*/
			}
			else if ((isAreaClear(currBlock.x, currBlock.y+4) == 0) && (currBlock.y-4 < topBorder))
			{
				loseFlag = 1;
			}
			else if (isAreaClear(currBlock.x, currBlock.y+4) == 0)
			{
				drawBlock_NoSpace(currBlock.x, currBlock.y, currBlock.block);
				checkForLines();
				dropNewBlock();
				updateNextBlocks();
				drawNextBlocks();
				changeHoldBox();
			}
		}
		else if (button & (0x01<<3))
		{
			clearCurrBlock();
			if (currBlock.x < rightBorder-8 && isAreaClear(currBlock.x+4, currBlock.y))
			{
				//drawBlockHere(currBlock.x, currBlock.y, EMPTY);
				currBlock.x += 4;
			}
		}
		drawBlock_NoSpace(currBlock.x, currBlock.y, currBlock.block);
		displayCurrentScreen(playScreen);
	}
}

void resetGame()
{
	unsigned char i = 0, j = 0;
	currBlock.x = 0;
	currBlock.y = 0;
	currBlock.block = EMPTY;
	currBlock.rotated = 0;
	holdBlock = EMPTY;
	nextBlocks[0] = EMPTY;
	nextBlocks[1] = EMPTY;
	nextBlocks[2] = EMPTY;
	gameScore = 0;
	drawNextBlocks();
	changeHoldBox();
	for (i = leftBorder; i <= rightBorder; i++)
	{
		for (j = topBorder; j <= bottomBorder; j++)
		{
			replacePixel(i, j, 0);
		}
	}
}

enum gameStates {
	START,
	PLAY,
	LOSE
} gameState;

void gameStateFCT()
{
	switch(gameState) { //Transitions
		case START:
			if(button && (prevPress == 0))
			{
				gameState = PLAY;
			}
			break;
		case PLAY:
			if(loseFlag)
			{
				gameState = LOSE;
			}
			break;
		case LOSE:
			if(button && (prevPress == 0))
			{
				gameState = START;
			}
			break;
		default:
			gameState = START;
	}
	
	switch(gameState) {
		case START:
			if(prevGameState != gameState)
			{
				LCD_ClearScreen();
				LCD_DisplayString(1, "Press any buttonto play");
				LCD_Cursor(0);
				printPictureOnLCD(startScreen);
			}
			break;
		case PLAY:
			if (prevGameState != gameState)
			{
				updateNextBlocks();
				dropNewBlock();
				updateNextBlocks();
				drawNextBlocks();
				changeHoldBox();
				LCD_ClearScreen();
				display_Score();
				displayCurrentScreen(playScreen);
			}
			else
			{
				if (button & (0x01<<1))
				{
					clearCurrBlock();
					unsigned char temp = currBlock.block;
					currBlock.block = holdBlock;
					holdBlock = temp;
					changeHoldBox();
					if (currBlock.block == EMPTY)
					{
						dropNewBlock();
						updateNextBlocks();
						drawNextBlocks();
						changeHoldBox();
					}
					displayCurrentScreen(playScreen);
				}
				if (downCount == 5)
				{
					downCount = 0;
					button = 0x01<<4;
					moveBlock();
				}
				else
					downCount++;
				
			}
			break;
		case LOSE:
			if(prevGameState != gameState)
			{
				loseFlag = 0;
				LCD_ClearScreen();
				LCD_DisplayString(1, "You Lose!");
				LCD_Cursor(0);
				resetGame();
				printPictureOnLCD(gameOver);
			}
	}
	prevGameState = gameState;
}

enum song_Ts{
	SILENT=15,
	PLAYM=16,
} song_T;

void play_Song()
{
	switch(song_T){
		case SILENT:
			if ((button & 0x01) && (prevPress == 0x00))
				song_T = PLAYM;
			break;
		case PLAYM:
			if ((button & 0x01) && (prevPress == 0x00))
				song_T = SILENT;
			if (count == 100)
				count = 0;
			break;
		default:
			song_T = SILENT;
	}
	switch(song_T){
		case SILENT:
			set_PWM(0);
			count = 0;
			break;
		case PLAYM:
			set_PWM(freqs[count]);
			temp++;
			break;
	}
}

int main(void)
{
	DDRA = 0x00; PORTA = 0xFF; //Initialize A0-A5 as input. Set pins to 1 to be read
	DDRB = 0xFF; PORTB = 0x00; //Initialize B0 as output and B6-7 for LCD
	DDRC = 0xFF; PORTC = 0x00; //Initialize C0-C7 as output for the LCD display
	DDRD = 0xFF; PORTD = 0x00; //Initialize D6-D7 as output for LCD display
	//Pins for D will be handled by the initializer below
	
	// define the 5 LCD Data pins: SCE, RST, DC, DATA, CLK
    Nlcd_init(&PORTD, PD4, &PORTD, PD3, &PORTD, PD2, &PORTD, PD1, &PORTD, PD0);
	LCD_init();
	TimerSet(1);
	TimerOn();
	PWM_on();
	LCD_Cursor(0);
	//unsigned int time = rand()/2;
	srand(0);
	currBlock.x = 0;
	currBlock.y = 0;
	currBlock.block = EMPTY;
	
	while (1) {
		button = ~PINA & 0x3F;
		moveBlock();
		play_Song();
		if (temp == (note_Held[count]+1))
		{
			set_PWM(0);
			temp = 0;
			count++;
		}
		gameStateFCT();
		prevPress = button;
		while(!TimerFlag);
		TimerFlag = 0;
	}		
	return 0;
}