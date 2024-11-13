#include <M5Core2.h>
#include <Adafruit_VCNL4040.h> // Sensor libraries
#include "Adafruit_SHT4x.h"    // Sensor libraries

// Initialize library objects (sensors and Time protocols)
Adafruit_VCNL4040 vcnl4040 = Adafruit_VCNL4040();
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

// LCD variables
int sWidth;  // 320
int sHeight; // 240

// Time variables
unsigned long lastTime = 0;
unsigned long timerDelayMs;
unsigned long mazeStartTime;
unsigned long mazeEndTime;

// state things
enum ScreenState
{
    START,
    INSTRUCTIONS,
    MAZE,
    END
};
static ScreenState screenState;

// button things
const int levelButtonY = 0;
const int textLevelButtonY = 15;
const int levelButtonHeight = 45;
const int easyButtonX = 10;
const int medButtonX = 75;
const int hardButtonX = 165;
const int extremeButtonX = 230;
const int buttonShiftX = 5;
const uint32_t buttonSelectedColor = TFT_PINK;
const uint32_t buttonUnselectedColor = TFT_LIGHTGREY;

Button easyButton(0, levelButtonY, medButtonX - easyButtonX, levelButtonHeight, "easy");
Button medButton(medButtonX - buttonShiftX, levelButtonY, hardButtonX - medButtonX - buttonShiftX, levelButtonHeight, "med");
Button hardButton(hardButtonX - buttonShiftX, levelButtonY, extremeButtonX - hardButtonX - buttonShiftX, levelButtonHeight, "hard");
Button extremeButton(extremeButtonX - buttonShiftX, levelButtonY, 320 - extremeButtonX, levelButtonHeight, "extreme");
Button startButton(100, 190, 130, 50, "start");
Button bottomRightButton(235, 210, 80, 30, "bottom-right");
Button bottomLeftButton(0, 210, 160, 30, "bottom-left");

// maze things
// floor types
enum FloorType
{
    FLOWER,
    ICE,
    WALKABLE,
    BLOOMED,
    STARTTILE
};

// floor tile struct
struct FloorTile
{
    int x;
    int y;
    // false means there is a wall, true means you can move to this position (no wall!)
    bool left;
    bool right;
    bool above;
    bool below;
    FloorType floor;

    FloorTile(int xCoor, int yCoor)
    {
        x = xCoor;
        y = yCoor;
        left = false;
        right = false;
        above = false;
        below = false;
        floor = WALKABLE;
    }
};

// maze levels
enum MazeLevel
{
    EASY,
    MEDIUM,
    HARD,
    EXTREME
};
static MazeLevel mazeMap = EASY; // default to the easy map
static MazeLevel mazeSpeed = EASY; // default easy speed

// maze variables
// maze size
const int width = 8;
const int height = 6;

const int halfWall = 5;
const int floorLength = 30;
const int floorTileLength = 40;

const uint32_t floorColor = TFT_GREENYELLOW;
const uint32_t wallColor = TFT_DARKGREEN;

// maze array sample positions FloorType[height][width]
/**
 * 00 01 02 03
 * 10 11 12 13
 * 20 21 22 23
 * 30 31 32 33
 *
 */
FloorTile mazeFloorPlan[height][width] = {
    {FloorTile(0, 0), FloorTile(0, 1), FloorTile(0, 2), FloorTile(0, 3), FloorTile(0, 4), FloorTile(0, 5), FloorTile(0, 6), FloorTile(0, 7)},
    {FloorTile(1, 0), FloorTile(1, 1), FloorTile(1, 2), FloorTile(1, 3), FloorTile(1, 4), FloorTile(1, 5), FloorTile(1, 6), FloorTile(1, 7)},
    {FloorTile(2, 0), FloorTile(2, 1), FloorTile(2, 2), FloorTile(2, 3), FloorTile(2, 4), FloorTile(2, 5), FloorTile(2, 6), FloorTile(2, 7)},
    {FloorTile(3, 0), FloorTile(3, 1), FloorTile(3, 2), FloorTile(3, 3), FloorTile(3, 4), FloorTile(3, 5), FloorTile(3, 6), FloorTile(3, 7)},
    {FloorTile(4, 0), FloorTile(4, 1), FloorTile(4, 2), FloorTile(4, 3), FloorTile(4, 4), FloorTile(4, 5), FloorTile(4, 6), FloorTile(4, 7)},
    {FloorTile(5, 0), FloorTile(5, 1), FloorTile(5, 2), FloorTile(5, 3), FloorTile(5, 4), FloorTile(5, 5), FloorTile(5, 6), FloorTile(5, 7)},
};

int startX;
int startY;

int endX;
int endY;

int currentX;
int currentY;

struct Hat
{
    int x;
    int y;
};

static Hat hat;

// maze objective variables
float iceMeltTemp;
const int bloomBrightness = 4000;
int numFlowersToBloom;
int numFlowersBloomed;

////////////////////////////////////////////////////////////////////
// Method header declarations
////////////////////////////////////////////////////////////////////
void onTap(Event &e);
void onDoubleTap(Event &e);
void initMazeVariables();
void makeMazeMap();
void drawMaze();
void drawStartScreen();
void drawLevelButtons();
void drawEndScreen();
void drawFlower(int xCenter, int yCenter, uint32_t petalColor, uint32_t centerColor);
void drawFlowerBud(int xCenter, int yCenter, uint32_t color);
void drawIceBlock(int xCenter, int yCenter);
void drawHowToPlayScreen();
void drawHat(int xCenter, int yCenter);
int convertCoor(int coor);
void drawTileCover();
void drawEndTile();
void drawStartTile();
void drawSensorScreen();

void setup()
{
    // Initialize the device
    M5.begin();
    M5.IMU.Init();
    M5.Buttons.addHandler(onTap, E_TOUCH);
    bottomRightButton.addHandler(onDoubleTap, E_DBLTAP);
    M5.Spk.begin();

    // Initialize VCNL4040
    if (!vcnl4040.begin())
    {
        Serial.println("Couldn't find VCNL4040 chip");
        while (1)
            delay(1);
    }
    Serial.println("Found VCNL4040 chip");

    // Initialize SHT40
    if (!sht4.begin())
    {
        Serial.println("Couldn't find SHT4x");
        while (1)
            delay(1);
    }
    Serial.println("Found SHT4x sensor");

    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);

    // Set up some variables for use in drawing
    sWidth = M5.Lcd.width();
    sHeight = M5.Lcd.height();

    screenState = START;

    drawStartScreen();

    // TODO Taz whiteboard
    //M5.Lcd.clear(TFT_GREENYELLOW);
    // drawBloomedPopup();
    //drawSensorScreen();
    // drawMaze();
    // drawHowToPlayScreen();
    // drawStartScreen();
    // drawEndScreen();
    // drawFlowerBud((sWidth/2)+40,(sHeight/2)-20, TFT_WHITE);
    // drawIceBlock(sWidth/2, (sHeight/2)-20);
    // drawHat(sWidth/2, (sHeight/2)-20);
    // drawFlower((sWidth/2)+40,(sHeight/2)-60, TFT_PINK, TFT_YELLOW);
}

void loop()
{
    M5.update();

    if (screenState == MAZE)
    {
        if (((millis() - lastTime) > timerDelayMs))
        {
            //~ ~ ~ ~ ~ ~ ~ ~ ~ ~ check for ice tile ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
            if (mazeFloorPlan[currentY][currentX].floor == ICE)
            {
                sensors_event_t rHum, temp;
                sht4.getEvent(&rHum, &temp);

                if (iceMeltTemp == 0)
                {
                    // takes the current temp, adds 2 degrees C for the melting temperature
                    iceMeltTemp = temp.temperature + 2.0;
                }
                else if (temp.temperature >= iceMeltTemp)
                {
                    M5.Spk.DingDong();
                    drawTileCover();
                    drawHat(convertCoor(currentX), convertCoor(currentY));
                    // melt the ice!
                    mazeFloorPlan[currentY][currentX].floor = WALKABLE;
                    // reset the iceMeltTemp to frozen for the next ice tile
                    iceMeltTemp = 0;
                }
            }
            else
                //~ ~ ~ ~ ~ ~ ~ ~ ~ ~ check for flower tile ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
                if (mazeFloorPlan[currentY][currentX].floor == FLOWER)
                {
                    uint16_t whiteLight = vcnl4040.getWhiteLight();

                    if (whiteLight >= bloomBrightness)
                    {
                        mazeFloorPlan[currentY][currentX].floor = BLOOMED;
                        numFlowersBloomed++;
                        M5.Spk.DingDong();

                        if (numFlowersBloomed == numFlowersToBloom)
                        {
                            drawEndTile();
                        }
                    }
                }
                else
                    //~ ~ ~ ~ ~ ~ ~ ~ ~ ~ check for tilting movement ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
                    if (mazeFloorPlan[currentY][currentX].floor == WALKABLE ||
                        mazeFloorPlan[currentY][currentX].floor == BLOOMED ||
                        mazeFloorPlan[currentY][currentX].floor == STARTTILE)
                    {
                        float accX; // postive val: tilt to the left    negative val: tilt to the right
                        float accY; // positive val: tilt down          negative val: tilt up
                        float accZ; // don't need this data
                        M5.IMU.getAccelData(&accX, &accY, &accZ);
                        accX *= 9.8;
                        accY *= 9.8;

                        // figure out which way the device is tilting the most
                        if (abs(accX) > 1 || abs(accY) > 1)
                        { // only if it's tilted at least a little
                            if (abs(accX) > abs(accY))
                            {
                                // tilt left/right
                                if (accX > 0)
                                {
                                    // tilt left
                                    if (mazeFloorPlan[currentY][currentX].left)
                                    {
                                        // draw tile over current hat position
                                        drawTileCover();

                                        // move the hat to the left
                                        hat.x = hat.x - 1;
                                        // update the current tile
                                        currentX -= 1;

                                        // draw the hat in its new position
                                        drawHat(convertCoor(hat.x), convertCoor(hat.y));
                                    }
                                }
                                else
                                {
                                    // tilt right
                                    if (mazeFloorPlan[currentY][currentX].right)
                                    {
                                        // draw tile over current hat position
                                        drawTileCover();

                                        // move the hat to the right
                                        hat.x = hat.x + 1;
                                        // update the current tile
                                        currentX += 1;

                                        // draw the hat in its new position
                                        drawHat(convertCoor(hat.x), convertCoor(hat.y));
                                    }
                                }
                            }
                            else
                            {
                                // tilt up/down
                                if (accY > 0)
                                {
                                    // tilt down
                                    if (mazeFloorPlan[currentY][currentX].below)
                                    {
                                        // draw tile over current hat position
                                        drawTileCover();

                                        // move the hat down
                                        hat.y = hat.y + 1;
                                        // update the current tile
                                        currentY += 1;

                                        // draw the hat in its new position
                                        drawHat(convertCoor(hat.x), convertCoor(hat.y));
                                    }
                                }
                                else
                                {
                                    // tilt up
                                    if (mazeFloorPlan[currentY][currentX].above)
                                    {
                                        // draw tile over current hat position
                                        drawTileCover();

                                        // move the hat up
                                        hat.y = hat.y - 1;
                                        // update the current tile
                                        currentY -= 1;

                                        // draw the hat in its new position
                                        drawHat(convertCoor(hat.x), convertCoor(hat.y));
                                    }
                                }
                            }
                        }
                        // Delay: Update the last time to NOW
                        lastTime = millis();
                    }
        }

        if (currentX == endX && currentY == endY && numFlowersBloomed >= numFlowersToBloom)
        {
            mazeEndTime = millis();
            screenState = END;
            drawEndScreen();
        }
    }
}

void initMazeVariables()
{
    // set up the maze walls
    makeMazeMap();

    // Set up the starting and ending tiles of the maze
    startX = 3;
    startY = 0;
    mazeFloorPlan[startY][startX].floor = STARTTILE;

    endX = 4;
    endY = 5;

    // Set up the hat at the starting point
    hat.x = startX;
    hat.y = startY;

    // set the current x and y values at the starting point
    currentX = startX;
    currentY = currentY;

    // set the maze speed
    switch (mazeSpeed)
    {
    case EASY:
        timerDelayMs = 300;
        break;
    case MEDIUM:
        timerDelayMs = 200;
        break;
    case HARD:
        timerDelayMs = 100;
        break;
    case EXTREME:
        timerDelayMs = 50;
        break;
    default:
        timerDelayMs = 300;
        break;
    }

    // set the number of flowers to bloom 
    switch (mazeMap)
    {
    case EASY:
        numFlowersToBloom = 5;
        break;
    case MEDIUM:
        numFlowersToBloom = 6; 
        break;
    case HARD:
        numFlowersToBloom = 5; // TODO update
        break;
    case EXTREME:
        numFlowersToBloom = 5;
        break;
    default:
        numFlowersToBloom = 5;
        break;
    }

    // set maze objective variables to default
    iceMeltTemp = 0;
    numFlowersBloomed = 0;
    mazeStartTime = millis(); // set the timer to now!
    mazeEndTime = 0;
}

void makeMazeMap()
{
    if (mazeMap == EASY) {
        // row 1
        mazeFloorPlan[0][0].right = true;

        mazeFloorPlan[0][1].left = true;
        mazeFloorPlan[0][1].right = true;
        mazeFloorPlan[0][1].below = true;

        mazeFloorPlan[0][2].left = true;
        mazeFloorPlan[0][2].below = true;

        mazeFloorPlan[0][3].below = true;

        mazeFloorPlan[0][4].right = true;
        mazeFloorPlan[0][4].below = true;

        mazeFloorPlan[0][5].left = true;
        mazeFloorPlan[0][5].right = true;

        mazeFloorPlan[0][6].left = true;
        mazeFloorPlan[0][6].below = true;

        mazeFloorPlan[0][7].below = true;

        // row 2
        mazeFloorPlan[1][0].right = true;
        mazeFloorPlan[1][0].below = true;

        mazeFloorPlan[1][1].above = true;
        mazeFloorPlan[1][1].left = true;

        mazeFloorPlan[1][2].above = true;
        mazeFloorPlan[1][2].below = true;

        mazeFloorPlan[1][3].above = true;
        mazeFloorPlan[1][3].below = true;

        mazeFloorPlan[1][4].above = true;
        mazeFloorPlan[1][4].right = true;

        mazeFloorPlan[1][5].left = true;
        mazeFloorPlan[1][5].below = true;

        mazeFloorPlan[1][6].above = true;
        mazeFloorPlan[1][6].below = true;

        mazeFloorPlan[1][7].above = true;
        mazeFloorPlan[1][7].below = true;

        // row 3
        mazeFloorPlan[2][0].above = true;
        mazeFloorPlan[2][0].right = true;

        mazeFloorPlan[2][1].left = true;
        mazeFloorPlan[2][1].below = true;

        mazeFloorPlan[2][2].above = true;
        mazeFloorPlan[2][2].below = true;

        mazeFloorPlan[2][3].above = true;
        mazeFloorPlan[2][3].right = true;

        mazeFloorPlan[2][4].left = true;
        mazeFloorPlan[2][4].right = true;

        mazeFloorPlan[2][5].left = true;
        mazeFloorPlan[2][5].above = true;

        mazeFloorPlan[2][6].above = true;
        mazeFloorPlan[2][6].right = true;
        mazeFloorPlan[2][6].below = true;

        mazeFloorPlan[2][7].left = true;
        mazeFloorPlan[2][7].above = true;

        // row 4
        mazeFloorPlan[3][0].below = true;

        mazeFloorPlan[3][1].above = true;
        mazeFloorPlan[3][1].below = true;

        mazeFloorPlan[3][2].above = true;
        mazeFloorPlan[3][2].right = true;

        mazeFloorPlan[3][3].left = true;
        mazeFloorPlan[3][3].right = true;

        mazeFloorPlan[3][4].left = true;
        mazeFloorPlan[3][4].right = true;

        mazeFloorPlan[3][5].left = true;
        mazeFloorPlan[3][5].right = true;
        mazeFloorPlan[3][5].below = true;

        mazeFloorPlan[3][6].left = true;
        mazeFloorPlan[3][6].right = true;
        mazeFloorPlan[3][6].above = true;

        mazeFloorPlan[3][7].left = true;

        // row 5
        mazeFloorPlan[4][0].above = true;
        mazeFloorPlan[4][0].below = true;

        mazeFloorPlan[4][1].above = true;
        mazeFloorPlan[4][1].below = true;
        mazeFloorPlan[4][1].right = true;

        mazeFloorPlan[4][2].left = true;
        mazeFloorPlan[4][2].below = true;

        mazeFloorPlan[4][3].right = true;

        mazeFloorPlan[4][4].left = true;
        mazeFloorPlan[4][4].below = true;

        mazeFloorPlan[4][5].above = true;
        mazeFloorPlan[4][5].below = true;

        mazeFloorPlan[4][6].below = true;
        mazeFloorPlan[4][6].right = true;

        mazeFloorPlan[4][7].below = true;
        mazeFloorPlan[4][7].left = true;

        // row 6
        mazeFloorPlan[5][0].above = true;
        mazeFloorPlan[5][0].right = true;

        mazeFloorPlan[5][1].left = true;
        mazeFloorPlan[5][1].above = true;

        mazeFloorPlan[5][2].right = true;
        mazeFloorPlan[5][2].above = true;

        mazeFloorPlan[5][3].left = true;
        mazeFloorPlan[5][3].right = true;

        mazeFloorPlan[5][4].left = true;
        mazeFloorPlan[5][4].above = true;

        mazeFloorPlan[5][5].above = true;
        mazeFloorPlan[5][5].right = true;

        mazeFloorPlan[5][6].left = true;
        mazeFloorPlan[5][6].above = true;

        mazeFloorPlan[5][7].above = true;

        // FLOWERS
        mazeFloorPlan[0][0].floor = FLOWER;
        mazeFloorPlan[0][7].floor = FLOWER;
        mazeFloorPlan[4][0].floor = FLOWER;
        mazeFloorPlan[4][3].floor = FLOWER;
        mazeFloorPlan[5][7].floor = FLOWER;

        // ICE
        mazeFloorPlan[0][4].floor = ICE;
        mazeFloorPlan[5][5].floor = ICE;
        mazeFloorPlan[3][1].floor = ICE;

    } else if (mazeMap == MEDIUM) {
        // row 1
        mazeFloorPlan[0][0].right = true;
        mazeFloorPlan[0][0].below = true;

        mazeFloorPlan[0][1].below = true;
        mazeFloorPlan[0][1].left = true;
        mazeFloorPlan[0][1].right = true;

        mazeFloorPlan[0][2].left = true;

        mazeFloorPlan[0][3].below = true;

        mazeFloorPlan[0][4].right = true;

        mazeFloorPlan[0][5].right = true;
        mazeFloorPlan[0][5].left = true;

        mazeFloorPlan[0][6].right = true;
        mazeFloorPlan[0][6].left = true;
        mazeFloorPlan[0][6].below = true;

        mazeFloorPlan[0][7].below = true;
        mazeFloorPlan[0][7].left = true;

        // row 2
        mazeFloorPlan[1][0].above = true;
        mazeFloorPlan[1][0].below = true;

        mazeFloorPlan[1][1].above = true;
        mazeFloorPlan[1][1].below = true;

        mazeFloorPlan[1][2].below = true;
        mazeFloorPlan[1][2].right = true;

        mazeFloorPlan[1][3].above = true;
        mazeFloorPlan[1][3].left = true;

        mazeFloorPlan[1][4].right = true;
        mazeFloorPlan[1][4].below = true;

        mazeFloorPlan[1][5].below = true;
        mazeFloorPlan[1][5].left = true;

        mazeFloorPlan[1][6].above = true;

        mazeFloorPlan[1][7].above = true;
        mazeFloorPlan[1][7].below = true;

        // row 3
        mazeFloorPlan[2][0].below = true;
        mazeFloorPlan[2][0].above = true;

        mazeFloorPlan[2][1].above = true;
        mazeFloorPlan[2][1].below = true;

        mazeFloorPlan[2][2].above = true;
        mazeFloorPlan[2][2].right = true;

        mazeFloorPlan[2][3].right = true;
        mazeFloorPlan[2][3].left = true;

        mazeFloorPlan[2][4].left = true;
        mazeFloorPlan[2][4].above = true;

        mazeFloorPlan[2][5].above = true;
        mazeFloorPlan[2][5].right = true;

        mazeFloorPlan[2][6].right = true;
        mazeFloorPlan[2][6].left = true;

        mazeFloorPlan[2][7].left = true;
        mazeFloorPlan[2][7].above = true;
        mazeFloorPlan[2][7].below = true;

        // row 4
        mazeFloorPlan[3][0].above = true;
        mazeFloorPlan[3][0].below = true;

        mazeFloorPlan[3][1].above = true;

        mazeFloorPlan[3][2].right = true;
        mazeFloorPlan[3][2].below = true;

        mazeFloorPlan[3][3].right = true;
        mazeFloorPlan[3][3].left = true;

        mazeFloorPlan[3][4].left = true;
        mazeFloorPlan[3][4].right = true;

        mazeFloorPlan[3][5].below = true;
        mazeFloorPlan[3][5].left = true;
        mazeFloorPlan[3][5].right = true;

        mazeFloorPlan[3][6].left = true;
        mazeFloorPlan[3][6].below = true;

        mazeFloorPlan[3][7].below = true;
        mazeFloorPlan[3][7].above = true;

        // row 5
        mazeFloorPlan[4][0].above = true;
        mazeFloorPlan[4][0].below = true;

        mazeFloorPlan[4][1].right = true;
        mazeFloorPlan[4][1].below = true;

        mazeFloorPlan[4][2].left = true;
        mazeFloorPlan[4][2].above = true;

        mazeFloorPlan[4][3].right = true;
        mazeFloorPlan[4][3].below = true;

        mazeFloorPlan[4][4].below = true;
        mazeFloorPlan[4][4].left = true;

        mazeFloorPlan[4][5].above = true;
        mazeFloorPlan[4][5].below = true;

        mazeFloorPlan[4][6].above = true;

        mazeFloorPlan[4][7].above = true;
        mazeFloorPlan[4][7].below = true;

        // row 6
        mazeFloorPlan[5][0].above = true;
        mazeFloorPlan[5][0].right = true;

        mazeFloorPlan[5][1].right = true;
        mazeFloorPlan[5][1].left = true;
        mazeFloorPlan[5][1].above = true;

        mazeFloorPlan[5][2].left = true;
        mazeFloorPlan[5][2].right = true;

        mazeFloorPlan[5][3].left = true;
        mazeFloorPlan[5][3].above = true;

        mazeFloorPlan[5][4].above = true;

        mazeFloorPlan[5][5].above = true;
        mazeFloorPlan[5][5].right = true;

        mazeFloorPlan[5][6].left = true;
        mazeFloorPlan[5][6].right = true;

        mazeFloorPlan[5][7].left = true;
        mazeFloorPlan[5][7].above = true;

        // FLOWERS
        mazeFloorPlan[0][2].floor = FLOWER;
        mazeFloorPlan[0][4].floor = FLOWER;
        mazeFloorPlan[1][6].floor = FLOWER;
        mazeFloorPlan[3][1].floor = FLOWER;
        mazeFloorPlan[4][2].floor = FLOWER;
        mazeFloorPlan[4][6].floor = FLOWER;

        // ICE
        mazeFloorPlan[2][2].floor = ICE;
        mazeFloorPlan[4][7].floor = ICE;
        mazeFloorPlan[4][0].floor = ICE;
        mazeFloorPlan[1][1].floor = ICE;

    } else if (mazeMap == HARD) {
        // row 1
        mazeFloorPlan[0][0].right = true;
        mazeFloorPlan[0][0].below = true;

        mazeFloorPlan[0][1].below = true;
        mazeFloorPlan[0][1].left = true;

        mazeFloorPlan[0][2].right = true;

        mazeFloorPlan[0][3].below = true;
        mazeFloorPlan[0][3].left = true;

        mazeFloorPlan[0][4].right = true;
        mazeFloorPlan[0][4].below = true;

        mazeFloorPlan[0][5].right = true;
        mazeFloorPlan[0][5].left = true;

        mazeFloorPlan[0][6].right = true;
        mazeFloorPlan[0][6].left = true;

        mazeFloorPlan[0][7].below = true;
        mazeFloorPlan[0][7].left = true;

        // row 2
        mazeFloorPlan[1][0].above = true;
        mazeFloorPlan[1][0].below = true;

        mazeFloorPlan[1][1].above = true;
        mazeFloorPlan[1][1].below = true;

        mazeFloorPlan[1][2].below = true;
        mazeFloorPlan[1][2].right = true;

        mazeFloorPlan[1][3].above = true;
        mazeFloorPlan[1][3].left = true;

        mazeFloorPlan[1][4].right = true;
        mazeFloorPlan[1][4].above = true;

        mazeFloorPlan[1][5].below = true;
        mazeFloorPlan[1][5].left = true;

        mazeFloorPlan[1][6].right = true;

        mazeFloorPlan[1][7].above = true;
        mazeFloorPlan[1][7].left = true;

        // row 3
        mazeFloorPlan[2][0].below = true;
        mazeFloorPlan[2][0].above = true;

        mazeFloorPlan[2][1].above = true;
        mazeFloorPlan[2][1].right = true;

        mazeFloorPlan[2][2].above = true;
        mazeFloorPlan[2][2].left = true;

        mazeFloorPlan[2][3].right = true;
        mazeFloorPlan[2][3].below = true;

        mazeFloorPlan[2][4].left = true;
        mazeFloorPlan[2][4].right = true;

        mazeFloorPlan[2][5].above = true;
        mazeFloorPlan[2][5].left = true;
        mazeFloorPlan[2][5].below = true;

        mazeFloorPlan[2][6].right = true;
        mazeFloorPlan[2][6].below = true;

        mazeFloorPlan[2][7].left = true;
        mazeFloorPlan[2][7].below = true;

        // row 4
        mazeFloorPlan[3][0].above = true;
        mazeFloorPlan[3][0].below = true;

        mazeFloorPlan[3][1].right = true;

        mazeFloorPlan[3][2].right = true;
        mazeFloorPlan[3][2].left = true;

        mazeFloorPlan[3][3].above = true;
        mazeFloorPlan[3][3].left = true;

        mazeFloorPlan[3][4].below = true;
        mazeFloorPlan[3][4].right = true;

        mazeFloorPlan[3][5].above = true;
        mazeFloorPlan[3][5].left = true;

        mazeFloorPlan[3][6].above = true;
        mazeFloorPlan[3][6].below = true;

        mazeFloorPlan[3][7].below = true;
        mazeFloorPlan[3][7].above = true;

        // row 5
        mazeFloorPlan[4][0].above = true;
        mazeFloorPlan[4][0].below = true;
        mazeFloorPlan[4][0].right = true;

        mazeFloorPlan[4][1].left = true;
        mazeFloorPlan[4][1].below = true;

        mazeFloorPlan[4][2].right = true;
        mazeFloorPlan[4][2].below = true;

        mazeFloorPlan[4][3].left = true;
        mazeFloorPlan[4][3].below = true;

        mazeFloorPlan[4][4].above = true;
        mazeFloorPlan[4][4].right = true;

        mazeFloorPlan[4][5].left = true;
        mazeFloorPlan[4][5].right = true;

        mazeFloorPlan[4][6].left = true;
        mazeFloorPlan[4][6].above = true;

        mazeFloorPlan[4][7].above = true;
        mazeFloorPlan[4][7].below = true;

        // row 6
        mazeFloorPlan[5][0].above = true;

        mazeFloorPlan[5][1].right = true;
        mazeFloorPlan[5][1].above = true;

        mazeFloorPlan[5][2].left = true;
        mazeFloorPlan[5][2].above = true;

        mazeFloorPlan[5][3].right = true;
        mazeFloorPlan[5][3].above = true;

        mazeFloorPlan[5][4].left = true;
        mazeFloorPlan[5][4].right = true;

        mazeFloorPlan[5][5].left = true;
        mazeFloorPlan[5][5].right = true;

        mazeFloorPlan[5][6].left = true;
        mazeFloorPlan[5][6].right = true;

        mazeFloorPlan[5][7].left = true;
        mazeFloorPlan[5][7].above = true;

        // FLOWERS
        mazeFloorPlan[0][2].floor = FLOWER;
        mazeFloorPlan[1][6].floor = FLOWER;
        mazeFloorPlan[3][1].floor = FLOWER;
        mazeFloorPlan[4][6].floor = FLOWER;
        mazeFloorPlan[5][0].floor = FLOWER;
        mazeFloorPlan[5][7].floor = FLOWER;

        // ICE
        mazeFloorPlan[0][1].floor = ICE;
        mazeFloorPlan[1][5].floor = ICE;
        mazeFloorPlan[3][3].floor = ICE;
        mazeFloorPlan[5][5].floor = ICE;
        mazeFloorPlan[4][0].floor = ICE;

    } else if (mazeMap == EXTREME) {
         // row 1
        mazeFloorPlan[0][0].below = true;

        mazeFloorPlan[0][1].below = true;
        mazeFloorPlan[0][1].right = true;

        mazeFloorPlan[0][2].right = true;
        mazeFloorPlan[0][2].left = true;
        mazeFloorPlan[0][2].below = true;

        mazeFloorPlan[0][3].left = true;

        mazeFloorPlan[0][4].right = true;
        mazeFloorPlan[0][4].below = true;

        mazeFloorPlan[0][5].below = true;
        mazeFloorPlan[0][5].left = true;

        mazeFloorPlan[0][6].right = true;
        mazeFloorPlan[0][6].below = true;

        mazeFloorPlan[0][7].below = true;
        mazeFloorPlan[0][7].left = true;

        // row 2
        mazeFloorPlan[1][0].above = true;
        mazeFloorPlan[1][0].right = true;

        mazeFloorPlan[1][1].above = true;
        mazeFloorPlan[1][1].left = true;

        mazeFloorPlan[1][2].above = true;
        mazeFloorPlan[1][2].right = true;

        mazeFloorPlan[1][3].below = true;
        mazeFloorPlan[1][3].left = true;

        mazeFloorPlan[1][4].below = true;
        mazeFloorPlan[1][4].above = true;

        mazeFloorPlan[1][5].above = true;
        mazeFloorPlan[1][5].right = true;

        mazeFloorPlan[1][6].above = true;
        mazeFloorPlan[1][6].left = true;
        mazeFloorPlan[1][6].below = true;

        mazeFloorPlan[1][7].above = true;

        // row 3
        mazeFloorPlan[2][0].below = true;
        mazeFloorPlan[2][0].right = true;

        mazeFloorPlan[2][1].below = true;
        mazeFloorPlan[2][1].left = true;

        mazeFloorPlan[2][2].below = true;

        mazeFloorPlan[2][3].above = true;
        mazeFloorPlan[2][3].below = true;

        mazeFloorPlan[2][4].above = true;
        mazeFloorPlan[2][4].right = true;

        mazeFloorPlan[2][5].left = true;

        mazeFloorPlan[2][6].right = true;
        mazeFloorPlan[2][6].above = true;

        mazeFloorPlan[2][7].left = true;
        mazeFloorPlan[2][7].below = true;

        // row 4
        mazeFloorPlan[3][0].above = true;
        mazeFloorPlan[3][0].below = true;

        mazeFloorPlan[3][1].above = true;
        mazeFloorPlan[3][1].below = true;

        mazeFloorPlan[3][2].above = true;
        mazeFloorPlan[3][2].below = true;

        mazeFloorPlan[3][3].above = true;
        mazeFloorPlan[3][3].right = true;

        mazeFloorPlan[3][4].left = true;
        mazeFloorPlan[3][4].right = true;

        mazeFloorPlan[3][5].below = true;
        mazeFloorPlan[3][5].left = true;

        mazeFloorPlan[3][6].right = true;
        mazeFloorPlan[3][6].below = true;

        mazeFloorPlan[3][7].left = true;
        mazeFloorPlan[3][7].above = true;

        // row 5
        mazeFloorPlan[4][0].above = true;
        mazeFloorPlan[4][0].below = true;

        mazeFloorPlan[4][1].above = true;
        mazeFloorPlan[4][1].right = true;

        mazeFloorPlan[4][2].above = true;
        mazeFloorPlan[4][2].left = true;

        mazeFloorPlan[4][3].below = true;

        mazeFloorPlan[4][4].below = true;
        mazeFloorPlan[4][4].right = true;

        mazeFloorPlan[4][5].left = true;
        mazeFloorPlan[4][5].above = true;

        mazeFloorPlan[4][6].right = true;
        mazeFloorPlan[4][6].above = true;

        mazeFloorPlan[4][7].left = true;
        mazeFloorPlan[4][7].below = true;

        // row 6
        mazeFloorPlan[5][0].above = true;
        mazeFloorPlan[5][0].right = true;

        mazeFloorPlan[5][1].right = true;
        mazeFloorPlan[5][1].left = true;

        mazeFloorPlan[5][2].left = true;
        mazeFloorPlan[5][2].right = true;

        mazeFloorPlan[5][3].right = true;
        mazeFloorPlan[5][3].above = true;
        mazeFloorPlan[5][3].left = true;

        mazeFloorPlan[5][4].left = true;
        mazeFloorPlan[5][4].right = true;
        mazeFloorPlan[5][4].above = true;

        mazeFloorPlan[5][5].left = true;
        mazeFloorPlan[5][5].right = true;

        mazeFloorPlan[5][6].left = true;
        mazeFloorPlan[5][6].right = true;

        mazeFloorPlan[5][7].left = true;
        mazeFloorPlan[5][7].above = true;

        // FLOWERS
        mazeFloorPlan[0][0].floor = FLOWER;
        mazeFloorPlan[1][7].floor = FLOWER;
        mazeFloorPlan[2][2].floor = FLOWER;
        mazeFloorPlan[2][5].floor = FLOWER;
        mazeFloorPlan[4][3].floor = FLOWER;
        mazeFloorPlan[5][7].floor = FLOWER;
        mazeFloorPlan[5][0].floor = FLOWER;

        // ICE
        mazeFloorPlan[0][5].floor = ICE;
        mazeFloorPlan[1][1].floor = ICE;
        mazeFloorPlan[1][6].floor = ICE;
        mazeFloorPlan[2][1].floor = ICE;
        mazeFloorPlan[5][3].floor = ICE;
        mazeFloorPlan[3][3].floor = ICE;
        
    }

}

void drawMaze()
{
    M5.Lcd.clear(floorColor);

    for (int row = 0; row < height; row++)
    {
        for (int col = 0; col < width; col++)
        {

            // draw walls, if there are any
            if (!mazeFloorPlan[row][col].left)
            {
                M5.Lcd.fillRect(col * floorTileLength, row * floorTileLength, halfWall, floorTileLength, wallColor);
            }
            if (!mazeFloorPlan[row][col].above)
            {
                M5.Lcd.fillRect(col * floorTileLength, row * floorTileLength, floorTileLength, halfWall, wallColor);
            }
            if (!mazeFloorPlan[row][col].right)
            {
                M5.Lcd.fillRect((col * floorTileLength) + halfWall + floorLength, row * floorTileLength, halfWall, floorTileLength, wallColor);
            }
            if (!mazeFloorPlan[row][col].below)
            {
                M5.Lcd.fillRect(col * floorTileLength, (row * floorTileLength) + halfWall + floorLength, floorTileLength, halfWall, wallColor);
            }

            // draw start tile, if applicable
            if (mazeFloorPlan[row][col].floor == STARTTILE)
            {
                drawStartTile();
            }

            // draw flower bud tiles, if applicable
            if (mazeFloorPlan[row][col].floor == FLOWER)
            {
                drawFlowerBud(convertCoor(col), convertCoor(row), TFT_WHITE);
            }

            // draw ice tiles, if applicable
            if (mazeFloorPlan[row][col].floor == ICE)
            {
                drawIceBlock(convertCoor(col), convertCoor(row));
            }
        }
    }
}

void drawStartScreen()
{
    M5.Lcd.clear(TFT_BLACK);

    M5.Lcd.setCursor(sWidth / 5, sHeight / 3);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.println("Maze Time!");

    drawFlower(sWidth/2, sHeight/2, TFT_MAGENTA, TFT_YELLOW);	
    drawFlower((sWidth/2)-25, sHeight/2, TFT_WHITE, TFT_YELLOW);	
    drawFlower((sWidth/2)+25, sHeight/2, TFT_WHITE, TFT_YELLOW);

    M5.Lcd.setTextColor(TFT_PINK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(20 + 10 + 100, 40 + 10 + 70 + 70 + 20);
    M5.Lcd.print("start!");

    M5.Lcd.setCursor(240, 225);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(TFT_MAGENTA);
    M5.Lcd.print("how to play");

    drawLevelButtons();
}

void drawLevelButtons()
{
    M5.Lcd.setTextSize(2);

    M5.Lcd.setTextColor(mazeSpeed == EASY ? buttonSelectedColor : buttonUnselectedColor);
    M5.Lcd.setCursor(easyButtonX, textLevelButtonY);
    M5.Lcd.print("Easy");

    M5.Lcd.setTextColor(mazeSpeed == MEDIUM ? buttonSelectedColor : buttonUnselectedColor);
    M5.Lcd.setCursor(medButtonX, textLevelButtonY);
    M5.Lcd.print("Medium");

    M5.Lcd.setTextColor(mazeSpeed == HARD ? buttonSelectedColor : buttonUnselectedColor);
    M5.Lcd.setCursor(hardButtonX, textLevelButtonY);
    M5.Lcd.print("Hard");

    M5.Lcd.setTextColor(mazeSpeed == EXTREME ? buttonSelectedColor : buttonUnselectedColor);
    M5.Lcd.setCursor(extremeButtonX, textLevelButtonY);
    M5.Lcd.print("Extreme");
}

void drawHowToPlayScreen()
{
    M5.Lcd.clear(TFT_BLACK);

    M5.Lcd.setCursor(20, 20);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.print("how to play:");

    drawFlowerBud(20, 60, WHITE);
    M5.Lcd.setCursor(40, 60);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print("navigate through the");
    M5.Lcd.setCursor(40, 80);
    M5.Lcd.print("maze!");

    drawFlower(20, 120, PINK, YELLOW);
    M5.Lcd.setCursor(40, 120);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print("shine a light on each ");
    M5.Lcd.setCursor(40, 140);
    M5.Lcd.print("flower bud to make");
    M5.Lcd.setCursor(40, 160);
    M5.Lcd.print("them bloom!");

    drawIceBlock(20, 200);
    M5.Lcd.setCursor(40, 200);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print("melt ice blocks to keep ");
    M5.Lcd.setCursor(40, 220);
    M5.Lcd.print("moving!");

    M5.Lcd.setCursor(230, 225);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(TFT_MAGENTA);
    M5.Lcd.print("back to start");
}

void drawEndScreen()
{
    M5.Lcd.clear(TFT_BLACK);
    	
    drawFlower(20, 20, TFT_WHITE, TFT_YELLOW);	
    drawFlower(50, 20, TFT_PINK, TFT_YELLOW);	
    drawFlower(20, 50, TFT_MAGENTA, TFT_YELLOW);	
    drawFlower(sWidth - 20, 20, TFT_WHITE, TFT_YELLOW);	
    drawFlower(sWidth - 50, 20, TFT_PINK, TFT_YELLOW);	
    drawFlower(sWidth - 20, 50, TFT_MAGENTA, TFT_YELLOW);

    M5.Lcd.setCursor(sWidth / 5, sHeight / 3 - 20);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.println("You did it!");

    M5.Lcd.setCursor(sWidth / 5, sHeight / 2 - 20);
    M5.Lcd.setTextSize(2);
    unsigned long totalTime = (mazeEndTime - mazeStartTime) / 1000;
    int minutes = totalTime / 60;
    int seconds = totalTime % 60;
    String timeTaken = "Time Taken: " + (String)minutes + ":";
    if (seconds < 10)
        timeTaken += "0";
    timeTaken += (String)seconds;
    M5.Lcd.println(timeTaken);

    M5.Lcd.setCursor(sWidth / 5 + 15, (sHeight / 2) + 10);
    String mapLevel = "Level: ";
    if (mazeMap == EASY) mapLevel += "Easy";
    else if (mazeMap == MEDIUM) mapLevel += "Medium";
    else if (mazeMap == HARD) mapLevel += "Hard";
    else if (mazeMap == EXTREME) mapLevel += "Extreme";
    M5.Lcd.println(mapLevel);

    M5.Lcd.setCursor(sWidth / 5 + 15, (sHeight / 2) + 40);
    String speedLevel = "Speed: ";
    if (mazeSpeed == EASY) speedLevel += "Easy";
    else if (mazeSpeed == MEDIUM) speedLevel += "Medium";
    else if (mazeSpeed == HARD) speedLevel += "Hard";
    else if (mazeSpeed == EXTREME) speedLevel += "Extreme";
    M5.Lcd.println(speedLevel);

    M5.Lcd.setCursor(130, 220);
    M5.Lcd.setTextColor(TFT_MAGENTA);
    M5.Lcd.print("Exit");
}
void drawFlower(int xCenter, int yCenter, uint32_t petalColor, uint32_t centerColor)
{
    M5.Lcd.fillCircle(xCenter, yCenter - 5, 3, petalColor);
    M5.Lcd.fillCircle(xCenter + 3, yCenter + 5, 3, petalColor);
    M5.Lcd.fillCircle(xCenter + 5, yCenter - 2, 3, petalColor);
    M5.Lcd.fillCircle(xCenter - 5, yCenter - 2, 3, petalColor);
    M5.Lcd.fillCircle(xCenter - 3, yCenter + 5, 3, petalColor);
    M5.Lcd.fillCircle(xCenter, yCenter, 2, centerColor);
}

void drawFlowerBud(int xCenter, int yCenter, uint32_t color)
{
    M5.Lcd.fillCircle(xCenter, yCenter, 8, TFT_DARKGREEN);
    M5.Lcd.fillEllipse(xCenter, yCenter - 3, 2, 4, color);
    M5.Lcd.fillEllipse(xCenter, yCenter + 3, 2, 4, color);
    M5.Lcd.fillEllipse(xCenter + 3, yCenter, 4, 2, color);
    M5.Lcd.fillEllipse(xCenter - 3, yCenter, 4, 2, color);
}

void drawIceBlock(int xCenter, int yCenter)
{
    int width = 20;
    int height = 20;
    int topLeftCornerX = xCenter - (width / 2);
    int topRightCornerX = xCenter + (width / 2);
    int topLeftCornerY = yCenter - (height / 2);
    int topRightCornerY = yCenter + (height / 2);
    M5.Lcd.fillRoundRect(topLeftCornerX, topLeftCornerY, width, height, 2, TFT_CYAN);
    M5.Lcd.fillCircle(topRightCornerX - 5, topLeftCornerY + 5, 2, TFT_WHITE);
    M5.Lcd.fillEllipse(topRightCornerX - 5, topLeftCornerY + 12, 2, 4, TFT_WHITE);
}

void drawHat(int xCenter, int yCenter)
{
    M5.Lcd.fillCircle(xCenter, yCenter, 10, TFT_MAROON);
    M5.Lcd.fillCircle(xCenter, yCenter, 6, TFT_ORANGE);
    M5.Lcd.fillCircle(xCenter, yCenter, 5, TFT_MAROON);
}

int convertCoor(int coor)
{
    return (coor * 40) + 20;
}

void drawTileCover()
{
    int topLeftCornerX = (currentX * floorTileLength + (floorTileLength / 2)) - (floorLength / 2);
    int topLeftCornerY = (currentY * floorTileLength + (floorTileLength / 2)) - (floorLength / 2);
    M5.Lcd.fillRect(topLeftCornerX, topLeftCornerY, floorLength, floorLength, floorColor);

    if (mazeFloorPlan[currentY][currentX].floor == BLOOMED)
    {
        drawFlower(convertCoor(currentX), convertCoor(currentY), TFT_MAGENTA, TFT_YELLOW);
    }
    else if (mazeFloorPlan[currentY][currentX].floor == STARTTILE)
    {
        drawStartTile();
    }
}

void drawEndTile()
{
    int topLeftCornerX = (endX * floorTileLength + (floorTileLength / 2)) - (floorLength / 2);
    int topLeftCornerY = (endY * floorTileLength + (floorTileLength / 2)) - (floorLength / 2);
    M5.Lcd.fillRect(topLeftCornerX, topLeftCornerY, floorLength, floorLength, M5.Lcd.alphaBlend(128, TFT_PURPLE, TFT_WHITE));
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.drawString("End", (endX * floorTileLength) + halfWall + 7, (endY * floorTileLength) + halfWall + 11, 1);
}

void drawStartTile()
{
    int topLeftCornerX = (startX * floorTileLength + (floorTileLength / 2)) - (floorLength / 2);
    int topLeftCornerY = (startY * floorTileLength + (floorTileLength / 2)) - (floorLength / 2);
    M5.Lcd.fillRect(topLeftCornerX, topLeftCornerY, floorLength, floorLength, M5.Lcd.alphaBlend(128, TFT_PURPLE, TFT_WHITE));
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("Start", (startX * floorTileLength) + halfWall, (startY * floorTileLength) + halfWall + 11, 1);
}

void onTap(Event &e)
{
    Button &b = *e.button;

    if (screenState == START)
    {
        if (b.instanceIndex() == 4)
        {
            mazeSpeed = EASY;
            mazeMap = EASY;
        }
        if (b.instanceIndex() == 5)
        {
            mazeSpeed = MEDIUM;
            mazeMap = MEDIUM;
        }
        if (b.instanceIndex() == 6)
        {
            mazeSpeed = HARD;
            mazeMap = HARD;
        }
        if (b.instanceIndex() == 7)
        {
            mazeSpeed = EXTREME;
            mazeMap = EXTREME;
        }

        drawLevelButtons();

        if (b.instanceIndex() == 8) {
            // start button
            initMazeVariables();
            drawMaze();
            drawHat(convertCoor(hat.x), convertCoor(hat.y));
            screenState = MAZE;
            Serial.println(timerDelayMs);
        }
        if (b.instanceIndex() == 9) {
            // bottom right button
            drawHowToPlayScreen();
            screenState = INSTRUCTIONS;
        }
    }

    if (screenState == END) {
        if (b.instanceIndex() == 8) {
            // bottom right button
            M5.shutdown();
        }
    }
}

void onDoubleTap(Event& e) {
    Button &b = *e.button;

    if (screenState == INSTRUCTIONS) {  
        // go back to start screen
        drawStartScreen();
        screenState = START;
        
    }
}

void drawSensorScreen(){
     M5.Lcd.clear(TFT_BLACK);

    M5.Lcd.setCursor(sWidth / 5, sHeight / 3);
    M5.Lcd.setTextColor(TFT_CYAN);
    M5.Lcd.setTextSize(3);
    M5.Lcd.println("uh-oh...");

    M5.Lcd.setCursor((sWidth / 5)- 20, sHeight / 2);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print("seems like ur missing");
    M5.Lcd.setCursor((sWidth / 5)- 20,(sHeight / 2)+20);
    M5.Lcd.print("a sensor or two :(");

    drawIceBlock(sWidth-43, sHeight-25);
    M5.Lcd.fillEllipse(sWidth-20, sHeight-23, 3, 2, TFT_CYAN);
    M5.Lcd.fillEllipse(sWidth-15, sHeight-15, 6, 2, TFT_CYAN);
}