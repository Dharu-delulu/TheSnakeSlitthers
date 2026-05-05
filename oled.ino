#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// Constructor for 1.3" SH1106 I2C OLED
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//SH1106= chip of OLED, 
 //128X64= Pixels of the grid 9128 wide by 64 height)
 //NONAME= Tells library to use generic settings that work for almost OLED Modules.
 // _1_ = Using 128 bytes of RAM (Saves RAM Consumption),
 // _F_ = store 1024 byte image in RAM all at once,
 //_HW_I2C = Using dedicated Hardware I2C pins because it uses a built in ATmega328P chip specifically designed for talking to sensors and screens.
 //U8G2_R0 = Sets the Rotation (R1=90 degrees rotation if user wants to hold joystick sideways)
 // U8X8_PIN_NONE = Most I2C OLEDs Don't have a physical reset pin, this tells the Library theat The oled doesn't has a Reset Pin.

// Pins & Constants
const int BUZZER_PIN = 3;
// the value of this variable(Pin where the Buzzer is connected) is a integer type that remains constant.
const int X_PIN = A0;      // Joystick X
const int Y_PIN = A1;      // Joystick Y
const int SW_PIN = 2;       // Joystick push Button

#define SNAKE_MAX_LENGTH 25
#define BOX_SIZE 4
#define SCREEN_W 128
#define SCREEN_H 64

// Defining the Memory of the Game
struct Point { int8_t x, y; }; //a point in space is created with X and Y co-ordinates
Point snake[SNAKE_MAX_LENGTH]; //An array that creates a list of points to represent the snakes's body, The program knows how to explicitly reserve 25 points in Arduino's memory.
Point food; // Creates a single point where the food is currently located on the screen
int8_t snakeLen = 3; // Tracks how many of those 25 reserved segments are actually "Active" and should be drawn on the screen. The int8_t is used so to conserve the RAM memory.
int8_t dirX = 1, dirY = 0;  //(1 represents moving right),(0 no movement in the Left of the screen.), Game begins from the right side of the screen. 
bool gameOver = false; // the moment the snake hits a wall, the game logic shows an "OUT".

void spawnFood() {
  // screen size is 128x64 pixels, but snake moves in blocks of 4 pixels.
  food.x = (random(1, (SCREEN_W / BOX_SIZE) - 1)) * BOX_SIZE; // defining the box width,(128/4) tells the Arduino there are 32 possible coloumns for the food.
  food.y = (random(1, (SCREEN_H / BOX_SIZE) - 1)) * BOX_SIZE; //defining the box height, (64/4) tells it there are 16 possible rows.
}
// We use 1 and -1 as offsets to ensure the food doesn't spawn exactly on the edge of the screen frame, which would make it look like it's stuck in the wall.
//This part picks a "Grid Coordinate" (e.g., "Column 10, Row 5") rather than a "Pixel Coordinate."
//BOX_SIZE = if the random function picked coloumn 10 then the may=th is 10x4=40, the food is the placed 40 pixels away, avoiding collison the next time they meet.


void resetGame() {
  snakeLen = 3;//shrinks the snake back to the starting size, effectively forgetting all the food it ate in previous round.
  //resetting Game 
  dirX = 1; dirY = 0;
  gameOver = false;
  // Initialize snake in the middle, I=0 (Head, placed at x=64), I=1 (Body, placed at x=60, 64-4), I=2( Tail, paced at x=56, 65-8)
  for(int i = 0; i < snakeLen; i++)//loop begins with i(zero), moves to 1 the 1+1 so on.
    {
    snake[i] = { (int8_t)(64 - (i * BOX_SIZE)), 32 };
  }
  spawnFood();// ensures that fresh piece of food appears at  random spots.
}

void setup() {
  u8g2.begin(); //initializing OLED
  pinMode(BUZZER_PIN, OUTPUT); //Digital 3= Output
  pinMode(SW_PIN, INPUT_PULLUP); // Important for the button, the joystick gets connecte dto the arduino, it connects the pin to GND, pulling it back to 0v.
  randomSeed(analogRead(A2)); //Generated random points for the food to appear, reads static electricity coming from empty pin A2.
  resetGame();
}

void loop() {
  // Always read joystick values
  int xVal = analogRead(X_PIN); //Communication with horizontal Potentiometer
  int yVal = analogRead(Y_PIN); //Communication with Vertical Potentiometer
 //if you push the stick all the wya to one side, it will be 0, the other side will be 1023, in the middle it will be 512.
  
  if (!gameOver)// true only if the game is not over 
  {
        //joystick potentiometers are sensitive. If you move the stick slightly, the value might change from 512 to 520.
    //By using < 300 and > 700, you create a Deadzone.
    // You have to intentionally push the stick at least 40% of the way in one direction for the snake to turn.
    //0-300 = Move up/left, 301-699 = deadzone/Do nothing, 700-1023 = down/Right
    if (xVal < 300 && dirX == 0) { dirX = -1; dirY = 0; }
    else if (xVal > 700 && dirX == 0) { dirX = 1; dirY = 0; }
    else if (yVal < 300 && dirY == 0) { dirX = 0; dirY = -1; }
    else if (yVal > 700 && dirY == 0) { dirX = 0; dirY = 1; }

     // Defining the movement of snake.
    //It tells the last segment (snake[i]) to move to the position where the segment in front of you(snake[i-1]) currently is.
    for (int i = snakeLen - 1; i > 0; i--) {
      snake[i] = snake[i - 1];
    }
    //Once the body has shifted forward to fill the old spots, the head moves
    snake[0].x += dirX * BOX_SIZE;
    snake[0].y += dirY * BOX_SIZE;
    //Example: if the snake is moving right,and the head is cureently at x=64, the new position becomes 64+(1x4)=68

    //defining the Wall Collision
    if (snake[0].x < 0 || snake[0].x >= SCREEN_W || snake[0].y < 0 || snake[0].y >= SCREEN_H) {
      gameOver = true;
      tone(BUZZER_PIN, 150, 500); // Crash sound
    }

    //defining the Food Collision
    if (snake[0].x == food.x && snake[0].y == food.y) {
      if (snakeLen < SNAKE_MAX_LENGTH) snakeLen++;
      spawnFood();
      tone(BUZZER_PIN, 2000, 50); // Eat sound
    }
  } else {
    //Game Over Logic: Restart with Button Click
    if (digitalRead(SW_PIN) == LOW) {
      resetGame();
      delay(300); // Debounce to prevent multiple restarts
    }
  }

  // Drawing Logic
  u8g2.firstPage();
  do {
    if (gameOver) {
      u8g2.setFont(u8g2_font_ncenB14_tr);
      u8g2.drawStr(48, 40, "OUT");
      
      // Blinking Instruction
      if ((millis() / 500) % 2 == 0) {
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(20, 55, "> PRESS TO START <");
      }
    } else {
      u8g2.drawFrame(0, 0, SCREEN_W, SCREEN_H);
      u8g2.drawBox(food.x, food.y, BOX_SIZE, BOX_SIZE);
      for (int i = 0; i < snakeLen; i++) {
        u8g2.drawBox(snake[i].x, snake[i].y, BOX_SIZE, BOX_SIZE);
      }
    }
  } while ( u8g2.nextPage() );

  delay(150); 
}
