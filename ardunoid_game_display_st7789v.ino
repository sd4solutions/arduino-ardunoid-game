/*******************************************************************************
  Arduino ardunoid game v1
  https://github.com/sd4solutions/arduino-ardunoid-game

  Work in progress, need to solve some glitches and need to add some some features(shots, bonus etc...)
  
  This code is provided by SD4Solutions di Domenico Spina
  Email me: info@sd4solutions.com
  https://github.com/sd4solutions
  Please leave my logo and my copyright then use as you want
  
  Thanks to:
  https://github.com/adafruit/Adafruit-GFX-Library
  https://github.com/adafruit/Adafruit-ST7735-Library/

   
  DISPLAY ST7789V 240X320

  Display PIN:
    VCC:  5V
    GND   GND
    DIN:  SPI DATA IN - arduino d11 (MOSI)
    CLK:  SPI CLOCK IN  - arduino d13 (SCK)
    CS :  ACTIVE LOW - arduino pin (9)
    DC :  DATA/COMMAND (HIGH DATA, LOW COMMAND) - arduino pin (8)
    RST:  RESET LOW ACTIVE - arduino pin (rst)
    BL :  5V
        
 ******************************************************************************/
#include <SPI.h>
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <Adafruit_GFX.h>    // Core graphics library
#include <EEPROM.h>
#include "sprites.h"

//definizione PIN DISPLAY
#define display_CS        9
#define display_RST      -1 // -1 for arduino pin reset, or any other pin value to use as display reset
#define display_DC        8
Adafruit_ST7789 display = Adafruit_ST7789(display_CS, display_DC, display_RST);

//buzzer pin arduino
#define buzzer 10

//movement pin arduino
#define up 4
#define down 5
#define right 6
#define left 3

//init values
bool is_up = false, is_down = false, is_left = false, is_right = false;
short x_direction = 0; //-1 0 1
short y_direction = 0; //-1 0 1

//game area
int offsetX = 0;
int offsetY = 80; //offset for game area
int max_width = 240;
int max_height = 240;

//player size
int playerSize = 1;
uint16_t playerWidth = 24;
uint16_t playerHeight = 12;

//ball size
uint16_t ballWidth = 6;
uint16_t ballHeight = 6;
uint16_t ball_x_sign = 1;
uint16_t ball_y_sign = -1;
uint16_t ball_x = 0;
uint16_t ball_y = 0;
uint16_t ballSpeed = 4;

//brick size
uint16_t brickWidth = 17;
uint16_t brickHeight = 12;
uint16_t brick_colors [7] = {ST77XX_RED,ST77XX_GREEN,ST77XX_BLUE,ST77XX_CYAN,ST77XX_MAGENTA,ST77XX_YELLOW,ST77XX_ORANGE};
uint16_t brick_scores [9] = {10,30,10,4,1,26,5,6,13};
uint16_t nbricks = 0;
uint16_t nhit = 0;

//store for brick xy 
struct gameItem {
  int X;
  int Y;
  int W;
  int H;
  int score;
  int hit;
};

//max brick size
gameItem bricks[72];
gameItem player;
gameItem ball;

//some vars
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
unsigned long speed = 50; //game speed
unsigned long delayTime = speed;
short current_stage = 0;

unsigned long max_score = 0;
unsigned long your_score = 0;
bool over_max_score = false;
int current_x = 0;
int current_y = 0;
int current_player_width = 0;
short player_steps = 1;

bool stop_game = false;
bool game_over = false;
bool in_game_over = false;
bool stages_cleared = false;


void setup(void) {

  Serial.begin(9600);

  //only first time to write some max score
  //writeEepromScore(225);
  
  pinMode(up, INPUT);pinMode(down, INPUT);pinMode(left, INPUT);pinMode(right, INPUT); pinMode(buzzer, OUTPUT);
  
  randomSeed(analogRead(0));
  
  display.init(240, 320);

  display.fillScreen(ST77XX_BLACK);
  display.drawBitmap(0, 0, sd4solutions, 240, 320, ST77XX_WHITE);
  delay(3000);

  initGame();
}

void writeEepromScore(long score)
{ 
  int address = 0;
  EEPROM.write(address, (score >> 24) & 0xFF);
  EEPROM.write(address + 1, (score >> 16) & 0xFF);
  EEPROM.write(address + 2, (score >> 8) & 0xFF);
  EEPROM.write(address + 3, score & 0xFF);
}

long getEepromScore(){

  int address = 0;
  return ((long)EEPROM.read(address) << 24) +
   ((long)EEPROM.read(address + 1) << 16) +
   ((long)EEPROM.read(address + 2) << 8) +
   (long)EEPROM.read(address + 3);
}

void initGame(){
  
  display.fillScreen(ST77XX_BLACK);

  //we read maxscore from eeprom
  long score = getEepromScore();
  if(score>0)
    max_score = score;    
  
  stop_game = game_over = stages_cleared= false;
  
  //draw stage nuum
  display.setTextSize(3);
  display.setTextColor(ST77XX_MAGENTA);
  char sbuff[10];
  sprintf(sbuff, "Level %d",(current_stage+1));
  drawCenterString(sbuff,160);
  tone(buzzer, 750);
  delay(100);
  tone(buzzer, 1500);
  delay(100);
  tone(buzzer, 2250);
  delay(1000);
  noTone(buzzer);
  delay(2000);
  
  display.fillScreen(ST77XX_BLACK);
  nhit = 0;
  delayTime = speed;
  drawGameArea();
  drawPlayer();
  drawBricks();
  drawBall();
}

void drawGameArea(){
  
  display.drawRect(0, 80, 240, 240, ST77XX_MAGENTA);
  display.setTextColor(ST77XX_MAGENTA);
  display.setTextSize(4);
  display.setCursor(24,0);
  display.print("ARDUNOID");

  display.setTextColor(ST77XX_RED);
  display.setTextSize(2);
  display.setCursor(0, 35);
  display.print("Max Score");
  display.setCursor(0, 55);
  display.setTextColor(ST77XX_RED);
  display.setTextSize(3);
  display.print(max_score);


  display.setTextColor(ST77XX_WHITE);
  display.setTextSize(2);
  drawRightString("Your Score", 240,35);
  display.setTextColor(ST77XX_WHITE);
  display.setTextSize(3);
  drawRightString(String(your_score), 240,55);
}

void drawPlayer(){
  
  player.X = offsetX+(max_width-2)/2-playerWidth/2;
  player.Y = offsetY+(max_height-2)-playerHeight;
  player.W = playerWidth;
  player.H = playerHeight;

  display.fillRoundRect( player.X, player.Y, player.W,player.H, 5, ST77XX_WHITE);
}

//here we move player
void movePlayer(){

   if(stop_game)
      return;
       
   int old_x = player.X;
   int old_y = player.Y;
   int old_w = player.W;
   int old_h = player.H;
       
   if(is_left || is_right)
   {
      display.fillRect( old_x, old_y, old_w,player.H, ST77XX_BLACK);
      player.X+= x_direction*player_steps;
      if((player.X+old_w)>(max_width-2))
        player.X = (max_width-2)-old_w;
      if(player.X<2)
        player.X = 2;        
      display.fillRoundRect(player.X, player.Y, old_w, old_h, 5, ST77XX_WHITE);
   }
   else
   {
      //fast redraw of player
      display.fillRoundRect(player.X, player.Y, old_w, old_h, 5, ST77XX_WHITE);
   }
  
}

void drawBricks(){

    int prev_x = offsetX+2;
    int prev_y = offsetY+2;
    uint16_t color;
    uint16_t score;
    int i=0;
    int j=0;
    int x=0;
    int y=0;
    
    nbricks = 0;
    
    //empty all bricks
    for(i=0;i<72;i++)
    {
      bricks[i].X = bricks[i].Y = -1;
      bricks[i].hit = false;
      bricks[i].score = 0;
    }
    
    for(i=0;i<stages[current_stage].n_bricks;i++)
    {
        j=0;
        color = random(0,7);
        score = random(0,9);
        
        do {
          x =random(prev_x,(max_width-brickWidth));
        } while ((x % (brickWidth+2) != 0));
      
        do {
          y =random(prev_y,(max_height-stages[current_stage].max_y-brickHeight));
        } while (y % (brickHeight+2) != 0);

        //check overlaps
        while(1)
        {
          if( bricks[j].X == x && bricks[j].Y == y)
          {
            do {
              x =random(prev_x,(max_width-brickWidth));
            } while ((x % (brickWidth+2) != 0));
          
            do {
             y =random(prev_y,(max_height-stages[current_stage].max_y-brickHeight));
            } while (y % (brickHeight+2) != 0);
          }
          else
            break;
        }
                
        bricks[i].X = x+2;
        bricks[i].Y = y+2;
        bricks[i].W = brickWidth;
        bricks[i].H = brickHeight;
        bricks[i].score = brick_scores[score];
        bricks[i].hit = false;
        display.fillRect(bricks[i].X,bricks[i].Y,brickWidth, brickHeight, brick_colors[color]);       
        nbricks++;
    }
    
}

void drawBall(){

  //center ball on player
  ball.X = player.X+player.W/2;
  ball.Y = player.Y-player.H-1;
  ball.W = ballWidth;
  ball.H = ballHeight;  
  ball_y_sign = -1;
  ball_x_sign = random(0,3);
  ball_x_sign = ball_x_sign==2?-1:ball_x_sign;
  display.fillCircle(ball.X, ball.Y, ball.W/2, ST77XX_WHITE);
}

void moveBall(){

  if(stop_game)
    return;

      
  if(millis()-previousMillis >delayTime)
  {
     //tone(buzzer, 80,25);
     previousMillis = millis();

     int old_x = ball.X;
     int old_y = ball.Y;
     int old_w = ball.W;
     int old_h = ball.H;

     display.fillCircle(old_x, old_y, old_w, ST77XX_BLACK);

     ball.X+= ballSpeed*ball_x_sign;
     ball.Y+= ballSpeed*ball_y_sign;
    
     noTone(buzzer);

     if((ball.X+ball.W)>(offsetX+max_width-2))
     {
        ball.X = offsetX+max_width-ball.W-2;
        ball_x_sign = -1;
        tone(buzzer, 80,10);
     }

     if((ball.X-ball.W)<=(offsetX+2))
     {
        ball.X = (offsetX+2)+ballWidth;
        ball_x_sign = 1;
        tone(buzzer, 80,10);
     }

     if((ball.Y+ball.H)>(offsetY+max_height-2))
     {
        ball.Y = offsetY+max_height-ball.H-2;
        ball_y_sign = -1;
        tone(buzzer, 80,10);

        //game over
        stop_game = true;
        game_over = true;
     }

     if((ball.Y-ball.H)<=(offsetY+2))
     {
        ball.Y = (offsetY+2)+ballHeight;
        ball_y_sign = 1;
        tone(buzzer, 80,10);
     }
  
     display.fillCircle(ball.X, ball.Y, ball.W/2, ST77XX_WHITE);
     
  }
  
}

void updateScore(){

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(String(your_score), 240,55 , &x1, &y1, &w, &h);
  display.fillRect(240-w,55, w , h, ST77XX_BLACK);
  display.setTextColor(ST77XX_WHITE);
  display.setTextSize(3);    
  
  drawRightString(String(your_score), 240,55);

  //check if stage clear
  int count_cleared = 0;
  for(int i=0;i<=nbricks;i++)
  {
     if(bricks[i].hit)
        count_cleared++;
  }

  //stage clear
  if(count_cleared>=stages[current_stage].n_bricks && count_cleared>0)
  {
      //success clear all levels
      if(current_stage>6)
      { 
          stages_cleared = true;
          game_over=true;
          return;
      }
      current_stage++;
      initGame();
      return;
  }
  
  
  //speedup ball after nhit
  if(nhit%stages[current_stage].hit_speed!= 0)
  {
    delayTime-=1;
    if(delayTime<10)
        delayTime = 10; //minimum value of speed
  }
   
}

void drawRightString(const String &text, int x, int y)
{
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
    display.setCursor(x - w, y);
    display.print(text);
}

void drawCenterString(const String &text, int y)
{
    int x = 0;
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
    display.setCursor(x + w/2, y);
    display.print(text);
}


//check collition with player and bricks
void checkCollitions(){

    //we have to check edge for ball collitions
    int ball_edge_t = (ball.Y-ball.H/2);
    int ball_edge_m = ball.Y;
    int ball_edge_b = (ball.Y+ball.H/2);

    //edges for bricks
    int ball_edge_c = ball.X;
    int ball_edge_l = ball.X-ball.W/2;
    int ball_edge_r = ball.X+ball.W/2;

    int player_c = player.X+player.W/2;
    
    //ball with player
    bool case1 = (ball_edge_c<=(player.X+player.W) && ball_edge_c>=(player.X));
    bool case2 = (ball_edge_l<=(player.X+player.W) && ball_edge_l>=(player.X));
    bool case3 = (ball_edge_r<=(player.X+player.W) && ball_edge_r>=(player.X));
    if(
        (case1 || case2 || case3)&&
        (ball_edge_b-ball.H)>(player.Y-player.H)
     )
     {
        //random change x direction
        ball_x_sign = random(0,3);
        ball_x_sign = ball_x_sign==2?-1:ball_x_sign;
        
        //change x direction with logic
        /*if(ball_edge_c<player_c)  ball_x_sign = -1;
        if(ball_edge_c==player_c) ball_x_sign =  0;
        if(ball_edge_c>player_c)  ball_x_sign =  1;*/
                
        ball_y_sign = -1;
        //clear ball to avoid glitch from interval
        display.fillCircle(ball.X, ball.Y, ball.W/2, ST77XX_BLACK);
        ball.Y = (player.Y-player.H) - ball.H/2;
        ball.X+=random(0,7)*ball_x_sign;
        if((ball.X+ball.W/2) >max_width -2)
        {
          ball_x_sign = -1;
          ball.X+=random(0,7)*ball_x_sign;
        }
        if((ball.X-ball.W/2) <2)
        {
          ball_x_sign = 1;
          ball.X+=random(0,7)*ball_x_sign;
        }
          
        tone(buzzer, 3000,10);
     }

     //ball with brick
     for(int i=nbricks;i>=0;i--)
     {
         if(
              (
                  ((ball_edge_l<=(bricks[i].X+bricks[i].W) && ball_edge_l>(bricks[i].X)))||
                  ((ball_edge_r<=(bricks[i].X+bricks[i].W) && ball_edge_r>(bricks[i].X)))
              ) &&
              (
                  ((ball_edge_t<=(bricks[i].Y+bricks[i].H) && ball_edge_t>(bricks[i].Y)))||
                  ((ball_edge_b<=(bricks[i].Y+bricks[i].H) && ball_edge_b>(bricks[i].Y)))
              ) &&              
              !bricks[i].hit
         )
         {
            display.fillRect(bricks[i].X,bricks[i].Y,bricks[i].W, bricks[i].H, ST77XX_BLACK);
            
            //change the y direction if over brick!
            if(ball_edge_m<=bricks[i].Y)
            {
              ball_y_sign = -1;
            }
            else
            {
              ball_y_sign = 1;
            }

            if(ball_edge_c<=bricks[i].X)
            {
              ball_x_sign = 1;
            }
            else
            {
              ball_x_sign = -1;
            }
                        
            bricks[i].hit = true;
            nhit++;
            your_score+=bricks[i].score;
            updateScore();
            noTone(buzzer);
            tone(buzzer, 2000,10);
            delay(20);
         }
     }
}

void readButtons(){
    is_up = digitalRead(up);
    is_down = digitalRead(down);
    is_left = digitalRead(left);
    is_right = digitalRead(right);

    if(in_game_over && game_over && (is_up || is_down || is_left || is_right))
    {
      in_game_over  = false;
      game_over = false;
      stop_game = false; 
      initGame();
      return;
    }
    
    x_direction = y_direction = 0;
    
    if(is_up && !y_direction){
      y_direction = -1;
    }
    if(is_down && !y_direction){
      y_direction = 1;
    }
    if(is_right && !x_direction){
      x_direction = 1;
    }
    if(is_left && !x_direction){
      x_direction = -1;
    }
}

void displayGameOver()
{
  if(in_game_over || !game_over)
      return;

  over_max_score = your_score>max_score;

  if(over_max_score)
  {
    writeEepromScore(your_score);
  }
  
  if(over_max_score || stages_cleared)
  {
    display.fillRect(0, 80, 240, 240, ST77XX_GREEN); 
  }
  else
  {
    display.fillRect(0, 80, 240, 240, ST77XX_RED);
  }
  
  display.setTextColor(ST77XX_BLACK);
  display.setTextSize(4);
  display.setCursor(18,170);
  if(over_max_score)
  {
    display.print("-YOU WIN-");
  }
  else
  {
    display.print("GAME OVER");
  }

  tone(buzzer, 2250);
  delay(100);
  tone(buzzer, 1500);
  delay(100);
  tone(buzzer, 750);
  delay(1500);
  noTone(buzzer);

  your_score = 0;
  in_game_over = true;  
  current_stage = 0;
  display.setCursor(6,220);
  display.setTextSize(2);
  display.print("press a key to play");
  
}

void loop() {
  readButtons();
  movePlayer();
  moveBall();
  checkCollitions();
  displayGameOver();
}
