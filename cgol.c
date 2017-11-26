//------------------------------------------------------------------------------
// NOTICE
//------------------------------------------------------------------------------
//
// Working title: Conway's Game of life
// Initially written for the www.it-talents.de coding competition
//
// Version date: 25.09.2017
//
// Author: Jan R.
// Contact: <jan AT dwrox DOT net>
//
//------------------------------------------------------------------------------

// CODE START
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <limits.h>

#include <dirent.h>

#ifndef _ISWINDOWS
#include <sys/stat.h>
#endif

// SDL2
#include <SDL.h>
#include <SDL_image.h>

// Cairo
#include <cairo.h>

//libpng
#include <png.h>

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------
#define DEBUG
#undef DEBUG

// How many turns we allow for history and playboard, before we reset history and playboard turns
#define TURN_LIMIT UINT_MAX

//------------------------------------------------------------------------------
// Enums
//------------------------------------------------------------------------------

// This is used for checks of the neighbour fiels, going clockwise from TOP
enum DIRECTIONS {
  TOP = 0,
  TOP_RIGHT = 1,
  RIGHT = 2,
  BOTTOM_RIGHT = 3,
  BOTTOM = 4,
  BOTTOM_LEFT = 5,
  LEFT = 6,
  TOP_LEFT = 7
};

// The infopanel states used in the history and for drawing the current related stat cells
enum INFOPANELSTATES {
  DISABLED = -2,
  STABLE = -1,
  DEAD = 0,
  BORN = 1
};

enum COMMANDTYPES {
  CELLSXY = 0,
  COLORTRESHOLD = 1,
  MAXIMUMFITCELLS = 2,
  DRAWGRID = 3,
  SHOWANIMATIONS = 4,
  DOCREATEHISTORY = 5,
  DRAWINFOPANEL = 6,
  USERANDOM = 7,
  USECAIRO = 8
};

//------------------------------------------------------------------------------
// Structs
//------------------------------------------------------------------------------

// The structure of a cell
typedef struct cell {
  bool isLiving;        // Cell living, true, or not living, false
  bool cellChanged;     // Did the cell change its status in a turn?
  int cellX;            // The cell X location on the gameBoard
  int cellY;            // The cell Y location on the gameBoard
  int x;                // The drawing x position
  int y;                // The drawing y position
  float size;           // From 0.0 to 1.00 - the size of cell when animating
} cell;

// Options which we refer also in general and also to display messages to the user
typedef struct options {
  bool drawGrid;                // Should we draw the playboard grid?
  bool showAnimations;          // Should we show animations
  bool hasMessage;              // Do we have any message to display?
  bool doRecordHistory;         // Do we record a history of all or parts of the current game?
  bool drawInfoPanel;           // Should we render the info panel?
  int messageTicks;             // How many ticks do we show a screen message?
  char message[64];             // Which message should we display on screen?
  int maximumFitCellsForRandom; // Maximum number of cells on random placement
  unsigned int colorThreshold;  // The minimum amount of color a pixel must contain to birth a living cell
  short activePanelItem;        // Which info panel item is active?
} options;

// The gameBoard which we refer to for all actions
typedef struct playBoard {
  bool isDirty;             // Do we have any changes on the playboard or is it dirty (changed) ?
  char status[32];          // Storage for game state display
  int width;                // The width of the main window / available x space
  int height;               // The height of the main window / available y space
  int cellsX;               // The amount of cells in X direction
  int cellsY;               // The amount of cells in Y direction
  int cellWidth;            // Calculated value of spaced out X size of a cell
  int cellHeight;           // Caluclated value of spaced out Y size of a cell
  unsigned int cellCount;   // The total cell count
  unsigned int livingCells; // The total count of living cells
  unsigned int turns;       // The count of turns
  struct cell* cells;       // The data pointer for all cells

} playBoard;

//------------------------------------------------------------------------------
// Data structures for the game history recording and stats function
//------------------------------------------------------------------------------
typedef struct gameHistoryTurn {
  unsigned int turn;        // History of which turn
  unsigned int records;     // Amount of changes per turn
  unsigned int countBorn;   // How many born cells
  unsigned int countDeath;  // Died cells
  unsigned int countStable; // How many persisten cells;
  short* state;             // Stable -1 / Dead = 0 / Living = 1
  unsigned int* index;      // The index of the cell in the cell array
} gameHistoryTurn;

typedef struct gameHistoryGame {
  unsigned int turns;                // How many turns did we record
  unsigned int currentTurn;          // Which turn are we displaying?
  struct gameHistoryTurn* turnData;  // History elements, for the turns
} gameHistoryGame;

//------------------------------------------------------------------------------
// Functions / Forward declarations
//------------------------------------------------------------------------------

// Function to retrieve neighbours of a particular cell in one direction
bool get_neighbour_living(struct cell*, struct playBoard*, int);

// Function to draw the base grid of the playboard/gameBoard and the living cells
bool drawGameBoard(SDL_Window*, SDL_Surface*, cairo_surface_t*, cairo_t*, struct playBoard*, struct options*, struct gameHistoryGame*);

// Function which applies all game rules on a new turn
void applyTurn(struct playBoard*, bool);

// Function to fill a board with random cell state
void initRandomBoard(struct playBoard*, struct options*);

// Set a options message to be diplayed in the game screen (Grid on/off, Animations on/off, Pause/Play)
void set_options_message(struct options*, char[]);

// Function to draw living cells with the mouse in painting mode
void paintCell(SDL_Event*, struct playBoard*, bool);

// Function to generate a cell map from a dropped image file
bool generateCellMapFromImage(char*, struct playBoard*, struct options*);

// Function to reset the playboard state
void resetPlayboard(struct playBoard*);

// Function to print out the help and usage - display using "cgol -h"...
void printHelp();

bool writePNG(SDL_Surface*, struct playBoard*, char*, struct options*);

//------------------------------------------------------------------------------
// History Functions

// Function to clear the game history
bool clearHistory(struct gameHistoryGame*);

// Function to add the playboard state to the history
bool addHistory(struct gameHistoryGame*, struct playBoard*);

bool historyBackwards(struct gameHistoryGame*, struct playBoard*);
bool historyForwards(struct gameHistoryGame*, struct playBoard*);

// Function to display history data on the playboard
bool historyDisplayTurn(struct gameHistoryGame*, struct playBoard*);



//------------------------------------------------------------------------------
// Functions / Real function content
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Returns if a neighbour in "direction" (enum) is living
//------------------------------------------------------------------------------
bool get_neighbour_living(struct cell* gameCell, struct playBoard* gameBoard, int direction) {
  // The x and y offset used to determine the index in the cell array, defaulted in case of no change
  int limitX = gameCell->cellX;
  int limitY = gameCell->cellY;

  // Shift for the offset
  short calcX = 0;
  short calcY = 0;

  // Check for the direction according to enum
  switch (direction) {
    case TOP:
      calcY = -1;
      break;
    case TOP_RIGHT:
      calcX = 1;
      calcY = -1;
      break;
    case RIGHT:
      calcX = 1;
      break;
    case BOTTOM_RIGHT:
      calcX = 1;
      calcY = 1;
      break;
    case BOTTOM:
      calcY = 1;
      break;
    case BOTTOM_LEFT:
      calcX = -1;
      calcY = 1;
      break;
    case LEFT:
      calcX = -1;
      break;
    case TOP_LEFT:
      calcX = -1;
      calcY = -1;
      break;
    default:
      return false;
  }

  // Offset calculation in case shift is present in X
  if (calcX == -1) {
    limitX = gameCell->cellX == 0 ? gameBoard->cellsX - 1: gameCell->cellX - 1;
  } else if (calcX == 1) {
    limitX = gameCell->cellX == gameBoard->cellsX - 1 ? 0 : gameCell->cellX + 1;
  }

  // Offset calculation in case shift is present in Y
  if (calcY == -1) {
    limitY = gameCell->cellY == 0 ? gameBoard->cellsY - 1 : gameCell->cellY - 1;
  } else if (calcY == 1) {
    limitY = gameCell->cellY == gameBoard->cellsY - 1 ? 0 : gameCell->cellY + 1;
  }

  #ifdef DEBUG
  // The location of a cell in an array, used for debugging only
  int cellOffset = limitX + (limitY * gameBoard->cellsY);

  // This displays, which fields are checked for each direction enum
  printf("[%d x, %d y] = DIRECTION %d - STATUS: %d @ [%d x, %d y]\n", gameCell->cellX, gameCell->cellY, direction, gameBoard->cells[cellOffset].isLiving, gameBoard->cells[cellOffset].cellX, gameBoard->cells[cellOffset].cellY);
  #endif

  // Return the "isLiving" status of the particular cell in direction at offset in array
  return gameBoard->cells[limitX + (limitY * gameBoard->cellsY)].isLiving;
}

//------------------------------------------------------------------------------
// Draws the game board and living cells
//------------------------------------------------------------------------------
bool drawGameBoard(SDL_Window* appWindow, SDL_Surface* drawingSurface, cairo_surface_t* cairoSurface, cairo_t* drawingContext, struct playBoard* gameBoard, struct options* gameOptions, struct gameHistoryGame* gameHistory) {

  // Clear the drawing area with a white background
  SDL_FillRect(drawingSurface, NULL, SDL_MapRGB(drawingSurface->format, 255, 255, 255));

  // This might not be required, but keeps the SDL surface from changing
  SDL_LockSurface(drawingSurface);

  // Draw the game playBoard

  // Draw the grid lines
  if (gameOptions->drawGrid) {
    // Set the cairo drawing color as rgba value
    cairo_set_source_rgba(drawingContext, 0, 0, 0, 0.2);

    for (int x = gameBoard->cellWidth; x < gameBoard->width; x += gameBoard->cellWidth) {
      cairo_move_to(drawingContext, x, 0);
      cairo_line_to(drawingContext, x, gameBoard->width);
    }

    for (int y = gameBoard->cellHeight; y < gameBoard->height; y += gameBoard->cellHeight) {
      cairo_move_to(drawingContext, 0, y);
      cairo_line_to(drawingContext, gameBoard->height, y);
    }

    // Stroke the pathes created with line_to commands
    cairo_stroke(drawingContext);
  }

  // Draw the living cells
  bool isAnimating = false;   // Is any cell animating right now
  float offX = 0.0;           // Offset in X for position and size when animating
  float offY = 0.0;           // Offset in Y for postion and size when animating
  int centerX = 0;
  int centerY = 0;
  float animationStepSize = 0.05;

  if (!gameOptions->showAnimations) {
    cairo_set_source_rgba(drawingContext, 1, 0.2, 0.2, 0.75);
  }

  // Draw the living cells as rectangles
  for (int i = 0; i < gameBoard->cellCount; ++i) {
    if (gameBoard->cells[i].isLiving) {
      // The cell is living

      // If we dont show animations simply draw the rectangle
      if (!gameOptions->showAnimations) {
        cairo_rectangle(drawingContext, gameBoard->cells[i].x, gameBoard->cells[i].y, gameBoard->cellWidth, gameBoard->cellHeight);
        continue;
      }

      // If we do animate, animate here
      if (gameBoard->cells[i].size == 1) {
        cairo_set_source_rgba(drawingContext, 1, 0.2, 0.2, 0.75);
        cairo_rectangle(drawingContext, gameBoard->cells[i].x, gameBoard->cells[i].y, gameBoard->cellWidth, gameBoard->cellHeight);
        cairo_fill(drawingContext);
      } else {
        // Do the animation: Growing with increasing alpha
        gameBoard->cells[i].size += animationStepSize;

        if (gameBoard->cells[i].size >= 1) {
          gameBoard->cells[i].size = 1;
          gameBoard->cells[i].cellChanged = false;
        }

        // Set animations to be running
        isAnimating = true;

        // Animation offset in X and Y and center on cell center
        offX = round(gameBoard->cellWidth * gameBoard->cells[i].size);
        offY = round(gameBoard->cellHeight * gameBoard->cells[i].size);
        centerX = (gameBoard->cellWidth - offX) * 0.5;
        centerY = (gameBoard->cellHeight - offY) * 0.5;

        cairo_set_source_rgba(drawingContext, 1, 0.2, 0.2, (gameBoard->cells[i].size * 0.5) + 0.25);
        cairo_rectangle(drawingContext, gameBoard->cells[i].x + centerX, gameBoard->cells[i].y + centerY, offX, offY);

        // Fill the rectangles / living cells
        cairo_fill(drawingContext);
      }

    } else if (gameOptions->showAnimations && !gameBoard->cells[i].isLiving && gameBoard->cells[i].cellChanged) {
      // The cell died in an earlier or this turn, shrink animate it
      gameBoard->cells[i].size -= animationStepSize;
      isAnimating = true;

      // Animation offset in X and Y and center on cell center
      offX = round(gameBoard->cellWidth * gameBoard->cells[i].size);
      offY = round(gameBoard->cellHeight * gameBoard->cells[i].size);
      centerX = (gameBoard->cellWidth - offX) * 0.5;
      centerY = (gameBoard->cellHeight - offY) * 0.5;

      cairo_set_source_rgba(drawingContext, 1, 0.2, 0.2, gameBoard->cells[i].size - 0.25);
      cairo_rectangle(drawingContext, gameBoard->cells[i].x + centerX, gameBoard->cells[i].y + centerY, offX, offY);
      cairo_fill(drawingContext);

      if (gameBoard->cells[i].size <= 0) {
        gameBoard->cells[i].size = 0;
        gameBoard->cells[i].cellChanged = false;
      }
    }
  }

  // If we dont show animations fill the rectangles drawn earlier
  if (!gameOptions->showAnimations) {
    cairo_fill(drawingContext);
  }

  // Render turn stats and information panel
  struct cell* activeCell = NULL;
  struct gameHistoryTurn* currentTurn = &gameHistory->turnData[gameHistory->currentTurn];

  // TODO: Create the info panel display


  if (gameOptions->drawInfoPanel && gameOptions->doRecordHistory && gameHistory->turns != 0) {

    //--------------------------------------------------------------------------
    if (gameOptions->activePanelItem >= STABLE && gameOptions->activePanelItem <= BORN) {
      switch (gameOptions->activePanelItem) {
        case STABLE:
          cairo_set_source_rgba(drawingContext, 0.3, 0.3, 0.45, 1);
          break;
        case BORN:
          cairo_set_source_rgba(drawingContext, 0.3, 0.3, 0.45, 1);
          break;
        case DEAD:
          cairo_set_source_rgba(drawingContext, 0.3, 0.3, 0.45, 1);
          break;
        default:
          break;
      }



      for (unsigned int i = 0; i < currentTurn->records; ++i) {
        if (currentTurn->state[i] == gameOptions->activePanelItem) {
          activeCell = &gameBoard->cells[currentTurn->index[i]];
          cairo_rectangle(drawingContext, activeCell->x, activeCell->y, gameBoard->cellWidth, gameBoard->cellHeight);
        }
      }

      cairo_fill(drawingContext);
    }

    //--------------------------------------------------------------------------
    cairo_set_source_rgba(drawingContext, 1, 1, 1, 0.35);
    cairo_rectangle(drawingContext, 0, gameBoard->height-100, gameBoard->width, 100);
    cairo_fill(drawingContext);

    unsigned int highestNum = 0;
    unsigned int currentNum = 0;

    for (int i = 0; i < 3; ++i) {
      switch (i) {
        case 0:
          currentNum = currentTurn->countBorn;
          break;
        case 1:
          currentNum = currentTurn->countDeath;
          break;
        case 2:
          currentNum = currentTurn->countStable;
          break;
        default:
          break;
      }

      if (currentNum > highestNum) {
        highestNum = currentNum;
      }
    }

    unsigned int availableSpace = gameBoard->width - 30;
    cairo_set_source_rgba(drawingContext, 1, 0, 0, 0.8);
    cairo_rectangle(drawingContext, 15, gameBoard->height-90, availableSpace * ((double) currentTurn->countStable / highestNum), 20);
    cairo_fill(drawingContext);

    cairo_set_source_rgba(drawingContext, 0, 1, 0, 0.8);
    cairo_rectangle(drawingContext, 15, gameBoard->height-60, availableSpace * ((double) currentTurn->countBorn / highestNum), 20);
    cairo_fill(drawingContext);

    cairo_set_source_rgba(drawingContext, 0, 0, 0, 0.8);
    cairo_rectangle(drawingContext, 15, gameBoard->height-30, availableSpace * ((double) currentTurn->countDeath / highestNum), 20);
    cairo_fill(drawingContext);

    char numberToDisplay[32];


    cairo_set_source_rgba(drawingContext, 0, 0, 0, 1);

    switch (gameOptions->activePanelItem) {
      case STABLE:
        cairo_rectangle(drawingContext, 14, gameBoard->height-91, gameBoard->width - 28, 22);
        break;
      case BORN:
        cairo_rectangle(drawingContext, 14, gameBoard->height-61, gameBoard->width - 28, 22);
        break;
      case DEAD:
        cairo_rectangle(drawingContext, 14, gameBoard->height-31, gameBoard->width - 28, 22);
        break;
      default:
        break;
    }

    cairo_stroke(drawingContext);



    //--------------------------------------------------------------------------
    cairo_set_source_rgba(drawingContext, 1, 1, 1, 1);

    sprintf(numberToDisplay, "%d stable cells", currentTurn->countStable);
    cairo_move_to(drawingContext, 20, gameBoard->height-74);
    cairo_show_text(drawingContext, numberToDisplay);

    sprintf(numberToDisplay, "%d birth/s", currentTurn->countBorn);
    cairo_move_to(drawingContext, 20, gameBoard->height-44);
    cairo_show_text(drawingContext, numberToDisplay);

    sprintf(numberToDisplay, "%d death/s", currentTurn->countDeath);
    cairo_move_to(drawingContext, 20, gameBoard->height-14);
    cairo_show_text(drawingContext, numberToDisplay);
  }

  // Show a screen message in case we have one to display
  if (gameOptions->hasMessage) {
    gameOptions->messageTicks--;

    if (gameOptions->messageTicks < 10) {
      cairo_set_source_rgba(drawingContext, 0, 0, 0, gameOptions->messageTicks  * 0.1);

      if (gameOptions->messageTicks == 0) {
        gameOptions->messageTicks = 50;
        gameOptions->hasMessage = false;
      }
    } else {
      cairo_set_source_rgba(drawingContext, 0, 0, 0, 1);
    }

    cairo_move_to(drawingContext, 20, 20);
    cairo_show_text(drawingContext, gameOptions->message);
  }

  // Paint all data on the cairo drawing surface to merge it into sdl
  cairo_surface_flush(cairoSurface);

  // Unlock the SDL surface
  SDL_UnlockSurface(drawingSurface);

  // Update and render the drawingSurface contents to the application window
  SDL_UpdateWindowSurface(appWindow);

  return isAnimating;
}


//------------------------------------------------------------------------------
// Apply turn and ruleset, returns the number of changes
//------------------------------------------------------------------------------
void applyTurn(struct playBoard* gameBoard, bool isInHistory) {
  gameBoard->isDirty = false;               // Do we have any changes in this round to the playboard? Set the gameboard as staled.
  bool deathInRound[gameBoard->cellCount];  // Storage for: Which cells die in this turn?
  bool bornInRound[gameBoard->cellCount];   // Storage for: Which cells start to live in this turn?

  // Set everything to false of above containers
  memset(deathInRound, false, gameBoard->cellCount);
  memset(bornInRound, false, gameBoard->cellCount);

  for (int i = 0; i < gameBoard->cellCount; ++i) {
    int livingNeighbours = 0;   // The number of living neighbours of a cell

    // Check each direction (TOP, TOP_RIGHT, RIGHT... ) round the clock
    // for living neighbours of the current cell at "i"
    for (int j = 0; j < 8; ++j) {
      if (get_neighbour_living(&gameBoard->cells[i], gameBoard, j)) {
        ++livingNeighbours;
      }
    }

    if (gameBoard->cells[i].isLiving) {
      // The cell is living, handle rule 1 to 3...
      // Rule 1: Cell dies, if less then 2 living neighbours
      // Rule 2: Cell continues to live, if 2 or 3 living neighbours
      // Rule 3: Cell dies if more then 3 living neighbours
      if (livingNeighbours < 2 || livingNeighbours > 3) {
        deathInRound[i] = true;
      }
    } else if (livingNeighbours == 3) {
      // Rule 4: Dead cell which has 3 living neighbours is born
      bornInRound[i] = true;
    }

    #ifdef DEBUG
    // Debug output, to print out how many neighbours a particular cell has all togheter
    printf("[%d, %d] => %d\n", gameBoard->cells[i].cellX, gameBoard->cells[i].cellY, livingNeighbours);
    #endif
  }

  #ifdef DEBUG
  // Just a debug output separator
  printf("########################################\n\n");
  #endif

  // Set the status of the cells, according to values of "death" or "born" changes
  for (int i = 0; i < gameBoard->cellCount; ++i) {
    if (deathInRound[i]) {
      gameBoard->cells[i].isLiving = false;   // Cell dies
      gameBoard->cells[i].cellChanged = true; // The cell changed its status
      --gameBoard->livingCells;               // Decrease the count of living cells on the playboard
      gameBoard->isDirty = true;              // Do we have any changes in this round, yes!
    } else if (bornInRound[i]) {
      gameBoard->cells[i].isLiving = true;    // Living cell born
      gameBoard->cells[i].cellChanged = true; // The cell changed its status
      ++gameBoard->livingCells;               // Increase the count of living cells on the playboard
      gameBoard->isDirty = true;              // Do we have any changes in this round, yes!
    } else {
      gameBoard->cells[i].cellChanged = false;
    }
  }

  // Increase the turn count of playboard by one and avoid overflow
  if (!isInHistory && ++gameBoard->turns > TURN_LIMIT) {
    gameBoard->turns = 0;
  }
}

//------------------------------------------------------------------------------
// Create a random playboard state
//------------------------------------------------------------------------------
void initRandomBoard(struct playBoard* gameBoard, struct options* gameOptions) {
  srand(time(NULL) + clock());  // Init random number generator

  // Reset the playboard
  resetPlayboard(gameBoard);

  unsigned int maximumFitCellsForRandom = gameOptions->maximumFitCellsForRandom * (((rand() % 75)+25) * 0.01);

  //----------------------------------------------------------------------------
  // Set random cells active
  //----------------------------------------------------------------------------
  unsigned int i = 0;  // Cell index we want to born

  // This is the most random approach, but might end up taking unknown amount of time of execution...
  // before this was linear written, setting a random living state for each cell of maximumFitCellsForRandom
  while (maximumFitCellsForRandom > 0) {
    i = rand() % (gameBoard->cellCount - 1);

    if (!gameBoard->cells[i].isLiving) {
      // Cell is not yet living, turn its status living
      gameBoard->cells[i].isLiving = true;

      // Set the cell state for animation
      gameBoard->cells[i].size = 0;
      gameBoard->cells[i].cellChanged = true;

      --maximumFitCellsForRandom; // Reduce the count of to be born cells by one
      ++gameBoard->livingCells;   // Increase the living cell count on the playboard, as we created a living cell
    }
  }
}

//------------------------------------------------------------------------------
// Function to set a options message to be display to the user
//------------------------------------------------------------------------------
void set_options_message(struct options* gameOptions, char message[]) {
  sprintf(gameOptions->message, message);
  gameOptions->hasMessage = true;
  gameOptions->messageTicks = 50;
}

//------------------------------------------------------------------------------
// Function to paint cells with the mouse when painting mode is enabled
//------------------------------------------------------------------------------
void paintCell(SDL_Event* appEvent, struct playBoard* gameBoard, bool isMotionEvent) {
  if (!isMotionEvent) {
    // Its a mousebutton event

    // The index in the playboard cell array
    unsigned int index = (floor(appEvent->button.x / (float) gameBoard->cellWidth)) + (floor(appEvent->button.y / (float) gameBoard->cellHeight) * gameBoard->cellsY);

    // The index is valid, paint the cell living and increase living cell count by one
    if (index < gameBoard->cellCount && !gameBoard->cells[index].isLiving) {
      gameBoard->cells[index].isLiving = true;
      ++gameBoard->livingCells;
    }

    return;
  }

  // Its a motion event, try to apply smoothing
  int xDistance = appEvent->motion.xrel;  // The total x distance we traveled from last move event
  int yDistance = appEvent->motion.yrel;  // The total y distance we traveled from last move event
  unsigned int index = 0; // The array index

  // Check if our drawing would hit the boundaries in x
  if (xDistance < 0 && appEvent->motion.x + xDistance < 0) {
    xDistance = appEvent->motion.x;
  } else if (xDistance > 0 && appEvent->motion.x + xDistance > gameBoard->width) {
    xDistance = gameBoard->width - appEvent->motion.x;
  }

  // Check if our drawing would hit the boundaries in y
  if (yDistance < 0 && appEvent->motion.y + yDistance < 0) {
    yDistance = appEvent->motion.y;
  } else if (yDistance > 0 && appEvent->motion.y + yDistance > gameBoard->height) {
    yDistance = gameBoard->height - appEvent->motion.y;
  }

  // Check if xDistance or y distance is either zero so we fill one axis
  if (xDistance == 0 || yDistance == 0) {

    // How much do we increase
    int stepWidth = 0;

    // How many steps do we take?
    int steps = abs(xDistance) > abs(yDistance) ? abs(xDistance) : abs(yDistance);

    if (xDistance == 0) {
      // We only have movement in the y-axis
      stepWidth = yDistance < 0 ? -1 : 1; // Positive or negative multiplicator

      // Paint all relevant indexes
      for (int i = 0; i < steps; ++i) {
        index = (floor(appEvent->motion.x / (float) gameBoard->cellWidth)) + (floor((appEvent->motion.y + (stepWidth * i)) / (float) gameBoard->cellHeight) * gameBoard->cellsY);

        // The index is valid, paint the cell living and increase living cell count by one
        if (index < gameBoard->cellCount && !gameBoard->cells[index].isLiving) {
          gameBoard->cells[index].isLiving = true;
          ++gameBoard->livingCells;
        }
      }
    } else {
      // We only have movement in the x-axis
      stepWidth = xDistance < 0 ? -1 : 1; // Positive or negative multiplicator

      // Paint all relevant indexes
      for (int i = 0; i < steps; ++i) {
        index = (floor((appEvent->motion.x + (stepWidth * i)) / (float) gameBoard->cellWidth)) + (floor(appEvent->motion.y / (float) gameBoard->cellHeight) * gameBoard->cellsY);

        // The index is valid, paint the cell living and increase living cell count by one
        if (index < gameBoard->cellCount && !gameBoard->cells[index].isLiving) {
          gameBoard->cells[index].isLiving = true;
          ++gameBoard->livingCells;
        }
      }
    }

  } else {
    // The largest distance determines how many steps we take
    int steps = abs(xDistance) > abs(yDistance) ? abs(xDistance) : abs(yDistance);

    // How much do we increase x and y in every stepped round?
    double stepWidthX = (double) xDistance / steps; // Positive or negative additition for the x-axis
    double stepWidthY = (double) yDistance / steps; // Positive or negative additition for the y-axis

    // Store the increment of x and y
    double increaseX = 0.0;
    double increaseY = 0.0;

    for (int i = 0; i < steps; ++i) {
      // Get the index, increase x by step and y by a rounded float increament each time
      index = (floor((appEvent->motion.x + ceil(increaseX)) / (float) gameBoard->cellWidth)) + (floor((appEvent->motion.y + ceil(increaseY)) / (float) gameBoard->cellHeight) * gameBoard->cellsY);

      // The index is valid, paint the cell living and increase living cell count by one
      if (index < gameBoard->cellCount && !gameBoard->cells[index].isLiving) {
        gameBoard->cells[index].isLiving = true;
        ++gameBoard->livingCells;
      }

      // Increase x and y by it stepwidth factor
      increaseX += stepWidthX;
      increaseY += stepWidthY;
    }
  }

}


//------------------------------------------------------------------------------
// Function to generate cell content from 24 bit png images
//------------------------------------------------------------------------------
bool generateCellMapFromImage(char* droppedFilePath, struct playBoard* gameBoard, struct options* gameOptions) {
  FILE* inputFile = fopen(droppedFilePath, "rb");

  // If we couldnt open the file for reading, its not usable
  // We could perform this check with access so, but this requires gnu technology
  if (inputFile == NULL) {
    set_options_message(gameOptions, "Image file not accessiable.");
    return false;
  }

  // Close it we dont use it anymore, we have read access
  fclose(inputFile);

  // Check the filepath ending
  int pathSize = strlen(droppedFilePath);
  if (pathSize <= 3) {
    set_options_message(gameOptions, "Filename without ");
    return false;
  }

  char* fileType = strrchr(droppedFilePath, '.');

  if (fileType == NULL) {
    set_options_message(gameOptions, "Image missing \".\" in name.");
    return false;
  }

  // Lower the fileType for checking
  for (int i = strlen(fileType)-1; i > 0; --i) {
    fileType[i] = tolower(fileType[i]);
  }

  // Check if the filename is a supported png...
  if (strcmp(fileType, ".png") != 0) {
    // Its sees to be a unsupported filetype
    set_options_message(gameOptions, "Only png images are supported.");
    return false;
  }

  // Assign a surface and load the image
  SDL_Surface* imageSurface = IMG_Load(droppedFilePath);

  // Check if the file was loading properly
  if (imageSurface == NULL) {
    set_options_message(gameOptions, "Image loading error, check console.");
    printf("[ERROR] Image loading errror:\n%s\n\n", IMG_GetError());
    return false;
  }


  // Finally, work with the surface
  if (imageSurface->format->BitsPerPixel < 32 ) {
    SDL_FreeSurface(imageSurface);
    set_options_message(gameOptions, "Only 32 bit png images are supported.");
    return false;
  }

  unsigned int imgWidth = imageSurface->w;    // The image width
  unsigned int imgHeight = imageSurface->h;   // The image height

  // How many image pixels are average for one cell in x and y
  unsigned int xPixelPerCell = floor((double) imgWidth / gameBoard->cellsX);
  unsigned int yPixelPerCell = floor((double) imgHeight / gameBoard->cellsY);

  // The total amount of image pixels one conway cell covers/is averaged from
  unsigned int totalPixelPerCell = xPixelPerCell * yPixelPerCell;

  // What is the maximum limit in x and y we cover with our cells, this is used to determe the borders
  unsigned int limitX = imgWidth; // - round((double) imgWidth / gameBoard->cellsX);
  unsigned int limitY = imgHeight; // - round((double) imgHeight / gameBoard->cellsY);

  // In case we have less then one by one pixel representation, we stop here
  if (xPixelPerCell < 1 || yPixelPerCell < 1) {
    SDL_FreeSurface(imageSurface);
    set_options_message(gameOptions, "Image pixel size should be bigger then x and y count of the conway cells.");
    return false;
  }

  // Reset the game board
  resetPlayboard(gameBoard);

  unsigned int index = 0;       // Index of the conway cell in the array of cells
  unsigned int tempOffset = 0;  // The offset of a pixel in the image, used for calulation to retrive colors

  unsigned int workingX = 0;    // The maximum x offset we are scanning to/from, starting from workingX - xPixelPerCell
  unsigned int workingY = 0;    // The maximum y offset we are scanning to/from, starting from workingY - yPixelPerCell

  // The color storages in RGB and averages by amount of samples
  unsigned int rValue = 0;      // The added red color value
  unsigned int gValue = 0;      // The added green color value
  unsigned int bValue = 0;      // The added blue color value
  unsigned int rAverage = 0;    // The red average of rValue divided by samples
  unsigned int gAverage = 0;    // The green average of gValue divided by samples
  unsigned int bAverage = 0;    // The blue average of bValue divided by samples

  // Lock the sdl surface
  SDL_LockSurface(imageSurface);

  // Get the pixeldata from the image surface in a Uin32 pointer for access
  Uint32* pixelData32bit = (Uint32 *)imageSurface->pixels;

  //----------------------------------------------------------------------------
  // Analyze the image
  //----------------------------------------------------------------------------

  // Set the working Y to init with yPixelPercel as boundary
  workingY = yPixelPerCell;
  int row = 0;

  // Process loop
  while (true) {

    // Incease workingX
    workingX += xPixelPerCell;

    // Would this read of the image limit in X?
    if (workingX > limitX) {
      // Yes, we are at the right boundary of the image
      workingX = xPixelPerCell;   // Reset working x to left
      workingY += yPixelPerCell;  // Increase the row in y

      ++row;
      index = (row * gameBoard->cellsX) - row;

      if (workingY > limitY) {
        // We break the loop as reading is over the boundaries of what can/should be read
        break;
      }
    }

    // Reset the rgb values to zero to start a new count
    rValue = 0;
    gValue = 0;
    bValue = 0;

    // Retrieve the color values of the image area we are scanning in
    for (unsigned int j = workingX - xPixelPerCell; j < workingX; ++j) {
      for (unsigned int i = workingY - yPixelPerCell; i < workingY; ++i) {
        tempOffset = j + (i * imgWidth) - i;  // At which image pixel we are reading from

        // Read in the r and g and b color values from the pixel
        rValue += ((pixelData32bit[tempOffset] & imageSurface->format->Rmask) >> imageSurface->format->Rshift) << imageSurface->format->Rloss;
        gValue += ((pixelData32bit[tempOffset] & imageSurface->format->Gmask) >> imageSurface->format->Gshift) << imageSurface->format->Gloss;
        bValue += ((pixelData32bit[tempOffset] & imageSurface->format->Bmask) >> imageSurface->format->Bshift) << imageSurface->format->Bloss;
      }
    }

    // Get the average value of the rgb colorspace divided by number of samples
    rAverage = rValue / totalPixelPerCell;
    gAverage = gValue / totalPixelPerCell;
    bAverage = bValue / totalPixelPerCell;

    // Is the sum of all colors over the threshold?
    if ((rAverage + gAverage + bAverage) <= gameOptions->colorThreshold) {
      // Turn the cell on living and set its animation state
      gameBoard->cells[index].isLiving = true;
      gameBoard->cells[index].cellChanged = true;
      ++gameBoard->livingCells;
    }

    // Increase the working index
    ++index;

    // If in any case, the index would hit the playboard cell count, break
    if (index >= gameBoard->cellCount-1) {
      break;
    }

  }

  // Unlock the sdl image surface
  SDL_UnlockSurface(imageSurface);

  // Free the image surface
  SDL_FreeSurface(imageSurface);

  // Everything well, return true
  return true;
}

//------------------------------------------------------------------------------
// Function to reset the playboard to initial state
//------------------------------------------------------------------------------
void resetPlayboard(struct playBoard* gameBoard) {
  // Reset the playboard cells...
  for (unsigned int i = 0; i < gameBoard->cellCount; ++i) {
    gameBoard->cells[i].size = 0;
    gameBoard->cells[i].isLiving = false;
    gameBoard->cells[i].cellChanged = false;
  }

  // ... and turns and living cells
  gameBoard->turns = 0;
  gameBoard->livingCells = 0;
}

//------------------------------------------------------------------------------
// Function to print out the help
//------------------------------------------------------------------------------
void printHelp() {
  printf("\n\nGENERAL HELP\n\nThis is Conway's Game of life.\nYou can either paint living cells, using the mouse in paint mode\nor drag and drop a 32 bit png with alpha channel into the game window,\nthis will generate a cell map varying from the image.\n\nImages should be equal in there dimensions in order to reach the best effect.\nFor example a one by one ratio, divisble by the cells in x and y to the image pixel dimensions.\nFor generation you could use The Gimp or save out images from the game using \"s\" key.\nExample images are incuded which are all consisting of 32bit PNG images with alpha.\n");
  printf("\nSAVING IMAGES AND DRAG AND DROP\n\nIf you want to save an image, you can do so by pressing \"s\" key.\nIt is then saved to the folder \"saved_images/TIMESTAMP.png\".\nThe routine saves everything except the grid and the message displayed inside the game window.\nThose images can be reimported into the application, like other images, by drag and drop.\n\nNote that CAIRO backend png's, switch \"-cb\", cannot directly be imported as they are missing the alpha channel.\nYou have to use The Gimp or similiar to add an alpha channel, then import is working!\n");
  printf("\nINFO PANEL USAGE\n\nIf the history is enabled (\"h\" key) - and the info panel enabled,\nthree bars are shown at the bottom of the screen. When the mouse is moved on\ntop of those bars, either:\n\t- Stable cells are shown, which didnt change in this turn or survived\n\t- Born cells. cells which did become born during a turn\n\t- Dead cells, which died during this turn\n\nIf you hover over the corresponding bar, you see the exact cells highlighted.\nAnd there counts are always displayed on the bars.\n");
  printf("\nAVAILABLE COMMANDS\n");
  printf("-h\t\t\t\tThis help\n");
  printf("-c5 ... 250\t\t\tAmount of cells in x and y by one number from 5 to 250 (default: 50)\n");
  printf("-ct0.0 ... 1.0\t\t\tFloating point value 0.5 for 50 percent color threshold (default: 0.85)\n\t\t\t\t(rgb added together and averaged) for living cell image generation, below the set value\n");
  printf("-mfc0.0 ... 1.0\t\t\tMaximum fit cells for random number generator (default 0.4)\n");
  printf("\nStart options of boolean type either 0/1 or t/f AND (also in game options/keybindings):\n\n");
  printf("-gBOOL\t+[KEY]\t\t\tGrid enabled (t)rue or 1 or disabled (f)alse or 0 - (\"g\" key in game)\n");
  printf("-aBOOL\t+[KEY]\t\t\tAnimations enabled or disabled (\"a\" key in game to toggle)\n");
  printf("-htBOOL\t+[KEY]\t\t\tHistory enabled or disabled (Toggled using \"h\" key in game)\n");
  printf("-iBOOL\t+[KEY]\t\t\tDraw infopanel when history is enabled and one turn is processed (Toggled with \"i\" key)\n");
  printf("-r\t+[KEY]\t\t\tShould the game start with a random playboard, be regenerated with a\n\t\t\t\trandom playboard (\"r\" key in game)\n");
  printf("\n-cb\t\t\t\tShould cairo's png functions be used instead of libpng to save images\n\t\t\t\tCairo backend images cannot be imported directly! Please read the help for more information.\n");
  printf("\n[KEY] \"s\"\t\t\tSaves the actual scene, including the info panel if displayed (\"s\" key in game)\n");
  printf("[KEY] \"+\" (not numpad)\t\tDecrease game ticks and increase game speed (\"+\" key in game)\n");
  printf("[KEY] \",\"\t\t\tGo back in history, if enabled (\",\" key in game)\n");
  printf("[KEY] \".\"\t\t\tGo forward in history, if enabled (\".\" key in game)\n");
  printf("[KEY] \"c\"\t\t\tClear game board (\"c\" key in game)\n");
  printf("[KEY] \"p\"\t\t\tPaint mode (\"p\" key in game)\n");
  printf("[KEY] \"Space\"\t\t\tPlay/Pause the game (\"space\" key in game)\n");
}

//------------------------------------------------------------------------------
// Function to clear the game history
//------------------------------------------------------------------------------
bool clearHistory(struct gameHistoryGame* gameHistory) {
  if (gameHistory->turnData == NULL) {
    return false;
  }

  for (unsigned int i = 0; i < gameHistory->turns; ++i) {
    free(gameHistory->turnData[i].state);
    free(gameHistory->turnData[i].index);
  }

  free(gameHistory->turnData);
  gameHistory->turnData = NULL;
  gameHistory->turns = 0;
  gameHistory->currentTurn = 0;

  return true;
}

//------------------------------------------------------------------------------
// Function to add a turn to the history
//------------------------------------------------------------------------------
bool addHistory(struct gameHistoryGame* gameHistory, struct playBoard* gameBoard) {
  if (gameHistory->turns > gameBoard->turns) {
    return false;
  }

  if (gameHistory->turns == TURN_LIMIT) {
    clearHistory(gameHistory);
  }

  gameHistory->turnData = realloc(gameHistory->turnData, sizeof(struct gameHistoryTurn) * (gameHistory->turns + 1));

  struct gameHistoryTurn* currentTurn = &gameHistory->turnData[gameHistory->turns];

  if (currentTurn == NULL) {
    return false;
  }

  currentTurn->records = 0;
  currentTurn->countBorn = 0;
  currentTurn->countDeath = 0;
  currentTurn->countStable = 0;
  currentTurn->state = NULL;
  currentTurn->index = NULL;

  gameHistory->currentTurn = gameHistory->turns;
  ++gameHistory->turns;


  for (unsigned int i = 0; i < gameBoard->cellCount; ++i) {
    if (gameBoard->cells[i].cellChanged) {
      currentTurn->state = realloc(currentTurn->state, sizeof(short) * (currentTurn->records + 1));
      currentTurn->index = realloc(currentTurn->index, sizeof(unsigned int) * (currentTurn->records + 1));

      currentTurn->index[currentTurn->records] = gameBoard->cells[i].cellX + (gameBoard->cells[i].cellY * gameBoard->cellsX);

      if (gameBoard->cells[i].isLiving) {
        ++currentTurn->countBorn;
        currentTurn->state[currentTurn->records] = BORN;
      } else {
        ++currentTurn->countDeath;
        currentTurn->state[currentTurn->records] = DEAD;
      }

      ++currentTurn->records;
    } else if (gameBoard->cells[i].isLiving) {
      currentTurn->state = realloc(currentTurn->state, sizeof(short) * (currentTurn->records + 1));
      currentTurn->index = realloc(currentTurn->index, sizeof(unsigned int) * (currentTurn->records + 1));

      currentTurn->state[currentTurn->records] = STABLE;
      currentTurn->index[currentTurn->records] = gameBoard->cells[i].cellX + (gameBoard->cells[i].cellY * gameBoard->cellsX);
      ++currentTurn->countStable;

      ++currentTurn->records;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
// Function to get backwards in the history
//------------------------------------------------------------------------------
bool historyBackwards(struct gameHistoryGame* gameHistory, struct playBoard* gameBoard) {
  if (gameHistory->currentTurn == 0 || gameHistory->turns == 0) {
    return false;
  }

  --gameHistory->currentTurn;

  if (historyDisplayTurn(gameHistory, gameBoard)) {
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
// Function to get forward in the history
//------------------------------------------------------------------------------
bool historyForwards(struct gameHistoryGame* gameHistory, struct playBoard* gameBoard) {

  if (gameHistory->currentTurn == gameBoard->turns || gameHistory->turns == 0 || gameHistory->currentTurn == (gameHistory->turns - 1)) {
    return false;
  }

  ++gameHistory->currentTurn;

  if (historyDisplayTurn(gameHistory, gameBoard)) {
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
// Function to display a history state on the playboard
//------------------------------------------------------------------------------
bool historyDisplayTurn(struct gameHistoryGame* gameHistory, struct playBoard* gameBoard) {

  // Reset the playboard to zero
  for (unsigned int i = 0; i < gameBoard->cellCount; ++i) {
    gameBoard->cells[i].isLiving = false;
    gameBoard->cells[i].cellChanged = false;
    gameBoard->cells[i].size = 0;
  }

  // Get the current history turn
  struct gameHistoryTurn* currentTurn = &gameHistory->turnData[gameHistory->currentTurn];

  // The working cell index in the playboard cell array
  unsigned int index = 0;

  // Set the cells accordings there recorded states
  for (unsigned int i = 0; i < currentTurn->records; ++i) {
    index = currentTurn->index[i];

    switch (currentTurn->state[i]) {
      case STABLE:
        // The cell was stable
        gameBoard->cells[index].isLiving = true;
        gameBoard->cells[index].cellChanged = false;
        gameBoard->cells[index].size = 1;
        break;
      case DEAD:
        // The cell was dying
        gameBoard->cells[index].isLiving = false;
        gameBoard->cells[index].cellChanged = true;
        gameBoard->cells[index].size = 1;
        break;
      case BORN:
        // The cell was born
        gameBoard->cells[index].isLiving = true;
        gameBoard->cells[index].cellChanged = true;
        gameBoard->cells[index].size = 0;
        break;
      default:
        break;
    }

  }

  return true;
}

//------------------------------------------------------------------------------
// Function to write out a image using libPNG functions
//------------------------------------------------------------------------------
bool writePNG(SDL_Surface* drawingSurface, struct playBoard* gameBoard, char* filename, struct options* gameOptions) {
  // Get the surface dimensions
  unsigned int width = drawingSurface->w;
  unsigned int height = drawingSurface->h;

  // Prepare the png structures
  png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytep row;

  // Try if we can access the image for reading
  FILE *imageFile = fopen(filename, "wb");

  if (imageFile == NULL) {
    set_options_message(gameOptions, "Could not open file for writing.");
    return false;
  }

  // Initialize png write structure
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (png_ptr == NULL) {
    set_options_message(gameOptions, "Could not create png write structure.");
    fclose(imageFile);
    remove(filename);
		return false;
	}

  // Initialize png info structure
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
    set_options_message(gameOptions, "Could not create png info structure.");
    fclose(imageFile);
    remove(filename);
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	}

  // Setup Exception handling
	if (setjmp(png_jmpbuf(png_ptr))) {
    set_options_message(gameOptions, "An error in libPNG occured.");
    fclose(imageFile);
    remove(filename);
    png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	}

  // Init png
  png_init_io(png_ptr, imageFile);

	// Write header (8 bit color depth, RGB plus alpha channel)
	png_set_IHDR(png_ptr, info_ptr, width, height,
			8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);


  // Allocate memory for one row (4 bytes per pixel - RGBA)
	row = (png_bytep) calloc(sizeof(png_byte), 4 * width);

  if (row == NULL) {
    set_options_message(gameOptions, "Insufficient ram to create png row.");
    fclose(imageFile);
    remove(filename);
    png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    return false;
  }

  // Get the pixel data
  SDL_LockSurface(drawingSurface);
  Uint32* pixelData = (Uint32*) drawingSurface->pixels;
  SDL_UnlockSurface(drawingSurface);

  // Write image data
  unsigned int offset = 0;    // The offset of the pixel in the image
  unsigned int yOffset = 0;   // The yOffset adding to the pixel offset

  unsigned int rowOffset = 0;   // The offset of the pixel data in the row

	for (unsigned int y = 0; y < height; y++) {
    yOffset = (y * width);
    for (unsigned int x = 0; x < width; x++) {
      offset = x + yOffset;
      rowOffset = x * 4;
      row[rowOffset] = ((pixelData[offset] & drawingSurface->format->Rmask) >> drawingSurface->format->Rshift) << drawingSurface->format->Rloss;
      row[rowOffset + 1] = ((pixelData[offset] & drawingSurface->format->Gmask) >> drawingSurface->format->Gshift) << drawingSurface->format->Gloss;
      row[rowOffset + 2] = ((pixelData[offset] & drawingSurface->format->Bmask) >> drawingSurface->format->Bshift) << drawingSurface->format->Bloss;
      row[rowOffset + 3] = 255;
		}

		png_write_row(png_ptr, row);
	}

  // End write
	png_write_end(png_ptr, NULL);

  // Close the image and cleanup
  fclose(imageFile);
  png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
  png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

  free(row);

  return true;
}

//------------------------------------------------------------------------------
// Functions end
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// Code Main
//------------------------------------------------------------------------------
int main(int argc, char *argv[]) {

  // Check if we build on windows, otherwise assume unix
  #ifdef _ISWINDOWS
    mkdir("saved_images");
  #else
    mkdir("saved_images", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  #endif

  //----------------------------------------------------------------------------
  // Show a welcome message on start
  //----------------------------------------------------------------------------
  printf("\n################# Welcome to Conway's Game of life #################\n\nThis is a competition entry for the IT-Talents.de coding competition.\nBy Jan R. - Version date: September 25th 2017.\n\nEnter \"cgol -h\" to show the help.\n");

  //------------------------------------------------------------------------------
  // Game options and such
  //------------------------------------------------------------------------------
  int cellsX = 50;   // The amount of cells in x direction
  int cellsY = 50;   // The amount of cells in y direction

  // Init game windows width and height to be at least of 500x500 or amount
  // of cells in X and Y multiplied by 4 for pixel
  float windowWidth = 0;
  float windowHeight = 0;

  bool drawGrid = true;           // Option to control grid drawing
  bool showAnimations = true;     // Option to turn animations on or off
  bool doCreateHistory = false;   // Option to control if we record a history
  bool drawInfoPanel = false;     // Option to control the display of the info panel
  bool useCairoPNGs = false;          // Option to control saving pngs with cairo and not lippng

  // Option random, living cells all together
  bool useRandom = false;         //  Should we use a random cell living routine on game start?

  unsigned int colorThreshold = (255+255+255) * 0.85;  // How much color a image area must consist of, to birth a cell
  float colorThresholdFloat = 0.85;

  // How many cells should we at most turn on living on random, at max every forth
  int maximumFitCellsForRandom = (cellsX * cellsY) * 0.4;
  float maximumFitCellsForRandomValue = 0.4;

  // Command parser
  char commandValue[17];
  int dataPos = 0;
  int commandType = 0;
  for (int i = 0; i < argc; ++i) {
    if (argv[i][0] == '-') {
      commandType = -1;

      if (strncmp(argv[i], "-ct", 3) == 0) {
        dataPos = 3;
        commandType = COLORTRESHOLD;
      } else if (strncmp(argv[i], "-c", 2) == 0) {
        dataPos = 2;
        commandType = CELLSXY;
      } else if (strncmp(argv[i], "-mfc", 4) == 0) {
        dataPos = 4;
        commandType = MAXIMUMFITCELLS;
      } else if (strncmp(argv[i], "-h", 2) == 0 || strncmp(argv[i], "-?", 2) == 0 || strncmp(argv[i], "?", 1) == 0) {
        printHelp();
        printf("\n######### Finished program. #########\n\n");
        return EXIT_SUCCESS;
      } else if (strncmp(argv[i], "-g", 2) == 0) {
        dataPos = 2;
        commandType = DRAWGRID;
      } else if (strncmp(argv[i], "-a", 2) == 0) {
        dataPos = 2;
        commandType = SHOWANIMATIONS;
      } else if (strncmp(argv[i], "-ht", 3) == 0) {
        dataPos = 3;
        commandType = DOCREATEHISTORY;
      } else if (strncmp(argv[i], "-i", 2) == 0) {
        dataPos = 2;
        commandType = DRAWINFOPANEL;
      } else if (strncmp(argv[i], "-r", 2) == 0) {
        commandType = USERANDOM;
        dataPos = 0;
      } else if (strncmp(argv[i], "-cb", 3) == 0) {
        commandType = USECAIRO;
        dataPos = 0;
      }

      if (commandType != -1 && strlen(argv[i]) >= dataPos) {
        commandValue[0] = '\0';

        if (commandType <= MAXIMUMFITCELLS) {

          strcpy(commandValue, &argv[i][dataPos]);
          commandValue[16] = '\0';

          switch (commandType) {
            case CELLSXY:
              cellsX = atoi(commandValue);


              if (cellsX < 5) {
                cellsX = 5;
              } else if (cellsX > 250) {
                cellsX = 250;
              }

              cellsY = cellsX;

              break;
            case COLORTRESHOLD:
              colorThresholdFloat =  atof(commandValue);

              if (colorThresholdFloat < 0) {
                colorThresholdFloat = 0.85;
              } else if (colorThresholdFloat > 1) {
                colorThresholdFloat = 0.85;
              }

              break;
            case MAXIMUMFITCELLS:
              maximumFitCellsForRandomValue = atof(commandValue);

              if (maximumFitCellsForRandomValue < 0) {
                maximumFitCellsForRandomValue = 0.4;
              } else if (maximumFitCellsForRandomValue > 1) {
                maximumFitCellsForRandomValue = 0.4;
              }

              break;
            default:
              continue;
              break;
          }

        } else if (commandType == USERANDOM) {
          useRandom = true;
        } else if (commandType == USECAIRO) {
          useCairoPNGs = true;
        } else {
          strncpy(commandValue, &argv[i][dataPos], 16);
          commandValue[16] = '\0';

          // Its a boolean command
          if (argv[i][dataPos] == '1' || argv[i][dataPos] == 't') {
            switch (commandType) {
              case DRAWGRID:
                drawGrid = true;
                break;
              case SHOWANIMATIONS:
                showAnimations = true;
                break;
              case DOCREATEHISTORY:
                doCreateHistory = true;
                break;
              case DRAWINFOPANEL:
                drawInfoPanel = true;
                break;
              default:
                continue;
                break;
            }
          } else if (argv[i][dataPos] == '0' || argv[i][dataPos] == 'f') {
            switch (commandType) {
              case DRAWGRID:
                drawGrid = false;
                break;
              case SHOWANIMATIONS:
                showAnimations = false;
                break;
              case DOCREATEHISTORY:
                doCreateHistory = false;
                break;
              case DRAWINFOPANEL:
                drawInfoPanel = false;
                break;
              default:
                continue;
                break;
            }
          }

        }
      }
    }
  }

  //------------------------------------------------------------------------------
  // Game options recalculations
  //------------------------------------------------------------------------------
  // Init game windows width and height to be at least of 500x500 or amount
  // of cells in X and Y multiplied by 5 for pixel
  windowWidth = cellsX <= 50 ? cellsX * 20 : cellsX <= 100 ? cellsX * 10 : cellsX * 5;
  windowHeight = cellsY <= 50 ? cellsY * 20 : cellsY <= 100 ? cellsY * 10 : cellsY * 5;

  colorThreshold = (255+255+255) * colorThresholdFloat;

  // How many cells should we at most turn on living on random, at max every forth
  maximumFitCellsForRandom = (cellsX * cellsY) * maximumFitCellsForRandomValue;

  // Avoid to have no population at all
  if (maximumFitCellsForRandom < 10) {
    maximumFitCellsForRandom = (cellsX * cellsY) * 0.4;
  }

  //------------------------------------------------------------------------------
  // The window base title string
  //------------------------------------------------------------------------------
  char windowBaseTitle[] = "Conway's Game of Life ";

  //----------------------------------------------------------------------------
  // Init SDL video
  //----------------------------------------------------------------------------
  if (SDL_VideoInit(0) != 0) {
    printf("[ERROR] Error initializing video:\n%s\n\nExiting.\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  // Get the display mode, to check if we the windows can correctly be displayed
  SDL_DisplayMode current;

  if (SDL_GetCurrentDisplayMode(0, &current) != 0) {
    printf("[ERROR] Could not get display mode:\n%s\n\nThis is not yet critical, but the created game window might be to large for your display.\n", SDL_GetError() );
  } else {
    if (windowHeight >= current.h-250 || windowWidth >= current.w) {
      // Restore the window to display at maximum one pixel per cell (divide by 2) until we reach a possible state or exit if windowWidth is smaller then cells in X and or Y
      do {
        windowHeight *= 0.8;
        windowWidth *= 0.8;

        if (windowWidth < cellsX || windowHeight < cellsY) {
          printf("[ERROR] Display resolution does not support the amount of cells in Y or X, cells must be a least 1 pixel in size.\nExiting.\n\n");
          SDL_VideoQuit();
          SDL_Quit();
          return EXIT_FAILURE;
        }
      }
      while (windowWidth > current.w && windowHeight > current.h-250);
    }
  }

  //----------------------------------------------------------------------------
  // Init SDL image library to handle JPG AND PNG images
  //----------------------------------------------------------------------------
  if (IMG_Init(IMG_INIT_PNG) == 0) {
    printf("[ERROR] Error initializing image loading:\n%s\n\nExiting.\n", IMG_GetError());

    SDL_VideoQuit();
    SDL_Quit();
    return EXIT_FAILURE;
  }

  //----------------------------------------------------------------------------
  // Create application window
  //----------------------------------------------------------------------------
  SDL_Window *appWindow = SDL_CreateWindow(windowBaseTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, floor(windowWidth), floor(windowHeight), 0);
  if (appWindow == NULL) {
    printf("[ERROR] Error initializing main window:\n%s\n\nExiting.\n", SDL_GetError());
    IMG_Quit();
    SDL_VideoQuit();
    SDL_Quit();
    return EXIT_FAILURE;
  }

  //----------------------------------------------------------------------------
  // Retrieve the drawing surface of the application windows
  //----------------------------------------------------------------------------
  SDL_Surface *drawingSurface = SDL_GetWindowSurface(appWindow);

  if (drawingSurface == NULL) {
    printf("[ERROR] Error creating main drawing surface on main window:\n%s\n\nExiting.\n", SDL_GetError());
    IMG_Quit();
    SDL_DestroyWindow(appWindow);
    SDL_VideoQuit();
    SDL_Quit();
    return EXIT_FAILURE;
  }

  //----------------------------------------------------------------------------
  // Create the cairo contexts, a drawing surface based from the application windows drawing area
  //----------------------------------------------------------------------------
  cairo_surface_t *cairoSurface = cairo_image_surface_create_for_data(drawingSurface->pixels, CAIRO_FORMAT_RGB24, drawingSurface->w, drawingSurface->h, drawingSurface->pitch);

  if (cairoSurface == NULL) {
    printf("[ERROR] Could not create cairo image surface.\n\nExiting.\n");
    // Cleanup SDL
    SDL_FreeSurface(drawingSurface);
    SDL_DestroyWindow(appWindow);

    IMG_Quit();

    SDL_VideoQuit();
    SDL_Quit();
    return EXIT_FAILURE;
  }

  //----------------------------------------------------------------------------
  // The drawing context from the new cairo surface
  //----------------------------------------------------------------------------
  cairo_t *drawingContext = cairo_create(cairoSurface);

  if (drawingContext == NULL) {
    printf("[ERROR] Could not create cairo drawing context.\n\nExiting.\n");

    // Cleanup cairo
    cairo_surface_destroy(cairoSurface);

    // Cleanup SDL
    SDL_FreeSurface(drawingSurface);
    SDL_DestroyWindow(appWindow);

    IMG_Quit();

    SDL_VideoQuit();
    SDL_Quit();
    return EXIT_FAILURE;
  }

  // Set the default font size to use
  cairo_select_font_face(drawingContext, "sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(drawingContext, 16);

  //----------------------------------------------------------------------------
  // Set the game options
  //----------------------------------------------------------------------------
  struct options gameOptions = { drawGrid, showAnimations, doCreateHistory, drawInfoPanel, false, 50, "", maximumFitCellsForRandom, colorThreshold, DISABLED };

  //----------------------------------------------------------------------------
  // Initialze the gameBoard with default values
  //----------------------------------------------------------------------------
  struct playBoard gameBoard = { true, "[PAUSED]", windowWidth, windowHeight, cellsX, cellsY, windowWidth / cellsX, windowHeight / cellsY, cellsX * cellsY, 0, 0, NULL };

  // Get memory of the gameboard cells
  gameBoard.cells = malloc(sizeof(struct cell) * gameBoard.cellCount);

  if (gameBoard.cells == NULL) {
    printf("[ERROR] Could not reserve memory for the cells of the gameboard.\nExiting.\n");

    // Cleanup cairo
    cairo_surface_destroy(cairoSurface);
    cairo_destroy(drawingContext);

    // Cleanup SDL
    SDL_FreeSurface(drawingSurface);
    SDL_DestroyWindow(appWindow);

    IMG_Quit();

    SDL_VideoQuit();
    SDL_Quit();
    return EXIT_FAILURE;
  }

  //----------------------------------------------------------------------------
  // Initialze the game history
  //----------------------------------------------------------------------------
  struct gameHistoryGame gameHistory = {0, 0, NULL};

  //----------------------------------------------------------------------------
  // Initialze the gameboard cells
  //----------------------------------------------------------------------------
  for (int y = 0; y < gameBoard.cellsY; ++y) {
    for (int x = 0; x < gameBoard.cellsX; ++x) {

      // The offset in the array
      int offset = x + (y * gameBoard.cellsY);

      // Set default to not living
      gameBoard.cells[offset].isLiving = false;

      // The x and y position off the cell used for drawing
      gameBoard.cells[offset].x = x * gameBoard.cellWidth;
      gameBoard.cells[offset].y = y * gameBoard.cellHeight;

      // The x and y location of the grid field in the gameboard
      gameBoard.cells[offset].cellX = x;
      gameBoard.cells[offset].cellY = y;

      // Animation properties
      gameBoard.cells[offset].cellChanged = false;
      gameBoard.cells[offset].size = 0.0;
    }
  }

  //------------------------------------------------------------------------------

  // Should we init the gameboard randomly with living cells?
  if (useRandom) {
    initRandomBoard(&gameBoard, &gameOptions);
  }

  //----------------------------------------------------------------------------
  // Main loop and main loop variables
  //----------------------------------------------------------------------------
  // SDL Data and event holder
  SDL_Event appEvent;           // Handle events from SDL like user input, window closing or such
  char* droppedFilePath = NULL; // Drag on drop filename, to be freed with SDL

  // Main loop variables
  bool doRunMainLoop = true;    // Should the main loop continue to run?
  bool doRender = true;         // Should we update the screen, this is false if the game ends by itself
  bool doPause = true;          // Do "pause" or are we "running"?

  int ticks = 0;                // Game ticks
  int turnTicks = 20;           // How many ticks until a turn is triggered
  int setTurnTicks = turnTicks; // How many turnticks we revert to after minimum is reached?

  bool animationInProgress = false; // Is any animation running, which might need to finish?

  // Paint mode options and restore
  bool doPaint = false;         // Are we in painting mode?
  bool storeAnimate = false;    // When painting mode is enabled/disabled, we restore the animation on/off state to the options
  bool mousePressed = false;    // Is the left mouse button pressed?

  // History control
  bool isInHistory = false;     // Are we navigating in history?
  bool historyCreated = false;  // Has any history since enabling been created? -> make a first snapshot before we process the turn

  // Window title and message char array
  char titleString[192];        // A data holder for the window title string
  char message[64];             // Data holder for some messages to be generated dynamically

  // Saving png files
  char filename[256];           // Filename for saving a file

  // Draw the initial game board once
  animationInProgress = drawGameBoard(appWindow, drawingSurface, cairoSurface, drawingContext, &gameBoard, &gameOptions, &gameHistory);

  //----------------------------------------------------------------------------
  // Main loop
  //----------------------------------------------------------------------------
  while(doRunMainLoop) {

    // Handle SDL window events and user input and such
    while (SDL_PollEvent(&appEvent)) {
      switch(appEvent.type) {
        case SDL_WINDOWEVENT:
          switch (appEvent.window.event) {
            case SDL_WINDOWEVENT_CLOSE:
              doRunMainLoop = false;
              break;
            default:
              break;
          }
          break;
        case SDL_MOUSEMOTION:
          if (doPaint && mousePressed) {
            paintCell(&appEvent, &gameBoard, true);
          } else if (gameOptions.drawInfoPanel) {
            if (appEvent.button.y < windowHeight - 100) {
             gameOptions.activePanelItem = DISABLED;
             drawGameBoard(appWindow, drawingSurface, cairoSurface, drawingContext, &gameBoard, &gameOptions, &gameHistory);
            } else if (appEvent.button.y >= gameBoard.height - 90 && appEvent.button.y <= gameBoard.height - 70) {
              gameOptions.activePanelItem = STABLE;
            } else if (appEvent.button.y >= gameBoard.height - 60 && appEvent.button.y <= gameBoard.height - 40) {
              gameOptions.activePanelItem = BORN;
            } else if (appEvent.button.y >= gameBoard.height - 30 && appEvent.button.y <= gameBoard.height - 10) {
              gameOptions.activePanelItem = DEAD;
            }
          }
          break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
          switch(appEvent.button.button) {
            case SDL_BUTTON_LEFT:
              switch (appEvent.button.state) {
                case SDL_PRESSED:
                  mousePressed = true;

                  if (doPaint) {
                    paintCell(&appEvent, &gameBoard, false);
                    continue;
                  }
                  break;
                case SDL_RELEASED:
                default:
                  mousePressed = false;
                  break;
              }

              break;
            default:
              break;
          }

          break;
        case SDL_DROPFILE:
          droppedFilePath = appEvent.drop.file;

          if (generateCellMapFromImage(droppedFilePath, &gameBoard, &gameOptions)) {
            if (gameOptions.doRecordHistory) {
              // Clear any history if present
              clearHistory(&gameHistory);
              historyCreated = false;
              isInHistory = false;
            }

            // If we are in here, the image has been parsed successfully
            doRender = true;
            set_options_message(&gameOptions, "Playboard created from image.");
          }

          // Cleanup the filepath of the file
          SDL_free(droppedFilePath);
          break;
        case SDL_KEYDOWN:
          switch (appEvent.key.keysym.sym) {
            case SDLK_COMMA:
              // Navigate history backwards, by pressing ";" key
              if (historyCreated) {
                if (historyBackwards(&gameHistory, &gameBoard)) {
                  isInHistory = true; // Set that we are navigating in history

                  if (gameHistory.currentTurn == 0) {
                    set_options_message(&gameOptions, "[HISTORY] Reached initital state.");
                    continue;
                  }

                  sprintf(message, "[BACKWARDS] Showing historical turn: %d", gameHistory.currentTurn);
                  set_options_message(&gameOptions, message);
                }
              } else if (!gameOptions.doRecordHistory) {
                set_options_message(&gameOptions, "History is disabled. Enable using \"h\" key.");
              } else {
                set_options_message(&gameOptions, "History is not yet recorded.");
              }

              break;
            case SDLK_PERIOD:
              // Navigate history forward, by pressing "." key
              if (historyCreated) {
                if (historyForwards(&gameHistory, &gameBoard)) {
                  isInHistory = true;   // Set that we are navigating in history
                  sprintf(message, "[FORWARD] Showing historical turn: %d", gameHistory.currentTurn);
                  set_options_message(&gameOptions, message);
                } else if (gameHistory.currentTurn == (gameHistory.turns - 1)) {
                  isInHistory = false;  // Set that we are not anymore navigating in history
                  set_options_message(&gameOptions, "[HISTORY] Reached current state.");
                  continue;
                }

              } else if (!gameOptions.doRecordHistory) {
                set_options_message(&gameOptions, "History is disabled. Enable using \"h\" key.");
              } else {
                set_options_message(&gameOptions, "History is not yet recorded.");
              }

              break;
            default:
              break;
          }
          break;
        case SDL_KEYUP:
          switch (appEvent.key.keysym.sym) {
            case SDLK_s:
              // "s" key saves the current game scene to a png file labeled by the time
              // unfortuntely those cannot be reloaded in the application, but have to be added an alpha value in Gimp or similar

              set_options_message(&gameOptions, "");

              if (drawGrid) {
                // Turn the grid temporary off
                gameOptions.drawGrid = false;

                // Draw the cleared playboard
                drawGameBoard(appWindow, drawingSurface, cairoSurface, drawingContext, &gameBoard, &gameOptions, &gameHistory);

                // Turn the grid on again
                gameOptions.drawGrid = true;
              } else {
                // Draw the game board one more time
                drawGameBoard(appWindow, drawingSurface, cairoSurface, drawingContext, &gameBoard, &gameOptions, &gameHistory);
              }

              #ifdef _ISWINDOWS
                sprintf(filename, "saved_images/%I64d.png", time(NULL));
              #else
                sprintf(filename, "saved_images/%ld.png", time(NULL));
              #endif

              if (!useCairoPNGs) {
                if (writePNG(drawingSurface, &gameBoard, &filename[0], &gameOptions)) {
                  sprintf(message, "Image saved in %s", filename);
                  set_options_message(&gameOptions, message);
                } else {
                  set_options_message(&gameOptions, "Error saving the image.");
                }
              } else {
                switch (cairo_surface_write_to_png(cairoSurface, filename)) {
                  case CAIRO_STATUS_SUCCESS:
                    sprintf(message, "Image saved in %s", filename);
                    set_options_message(&gameOptions, message);
                    break;
                  case CAIRO_STATUS_NO_MEMORY:
                    set_options_message(&gameOptions, "No ram/memory to save the image.");
                    break;
                  case CAIRO_STATUS_WRITE_ERROR:
                    set_options_message(&gameOptions, "Error writing the image to the file system.");
                    break;
                  default:
                    break;
                }
              }

              break;
            case SDLK_PLUS:
              // "+" (not numpad!) to decrease turn ticks to increase game speed
              setTurnTicks -= 5;
              if (setTurnTicks < 5) {
                setTurnTicks = turnTicks; // Reset turn ticks to default 20
              }

              sprintf(message, "[SPEED] Waiting %d ticks for a turn.", setTurnTicks);
              set_options_message(&gameOptions, message);

              break;
            case SDLK_i:
              // "i" key to turn the infopanel on/off
              gameOptions.drawInfoPanel = !gameOptions.drawInfoPanel;

              sprintf(message, "Info panel %s.", gameOptions.drawInfoPanel ? "enabled" : "disabled");
              set_options_message(&gameOptions, message);
              break;
            case SDLK_c:
              // Clear the gameboard and by pressing "c" key
              historyCreated = false; // Reset history to be not yet created
              isInHistory = false;    // Reset that we are in history mode

              // Clear any history if present
              if (gameOptions.doRecordHistory) {
                clearHistory(&gameHistory);
              }

              // Reset the playboard
              resetPlayboard(&gameBoard);

              // Set to render and pause the game
              doRender = true;
              doPause = true;

              if (!doPaint) {
                sprintf(gameBoard.status, "[PAUSED]");
              } else {
                sprintf(gameBoard.status, "[PAINT MODE]");
              }

              set_options_message(&gameOptions, "Playboard cleared.");

              // Update the window title
              sprintf(titleString, "%s - TURN: %u %s", windowBaseTitle, gameBoard.turns, gameBoard.status);
              SDL_SetWindowTitle(appWindow, titleString);

              // Draw the cleared playboard
              drawGameBoard(appWindow, drawingSurface, cairoSurface, drawingContext, &gameBoard, &gameOptions, &gameHistory);

              break;
            case SDLK_p:
              // Key "p" to switch paint mode
              doPaint = !doPaint;


              if (doPaint) {
                storeAnimate = gameOptions.showAnimations;
                gameOptions.showAnimations = false;
                sprintf(gameBoard.status, "[PAINT MODE]");

                // Reset history states as we draw it invalid...
                historyCreated = false; // Reset history to be not yet created
                isInHistory = false;    // Reset that we are not anymore in history

                // Clear any history if present
                if (gameOptions.doRecordHistory) {
                  clearHistory(&gameHistory);
                }

              } else {
                gameOptions.showAnimations = storeAnimate;
                sprintf(gameBoard.status, doPause ? "[PAUSED]" : "[RUNNING]");

                // Restart processing and drawing in case the game ended
                if (gameBoard.livingCells != 0) {
                  doRender = true;  // Reset state after living cells have been painted
                }
              }

              // Update the window title
              sprintf(titleString, "%s - TURN: %u %s", windowBaseTitle, gameBoard.turns, gameBoard.status);
              SDL_SetWindowTitle(appWindow, titleString);

              if (!gameOptions.doRecordHistory) {
                set_options_message(&gameOptions, doPaint ? "Paint mode on." : "Paint mode off.");
              } else {
                set_options_message(&gameOptions, doPaint ? "Paint mode on, history cleared." : "Paint mode off.");
              }

              break;
            case SDLK_h:
              // Switch history creation on/off by pressing "h" key
              gameOptions.doRecordHistory = !gameOptions.doRecordHistory;

              if (!gameOptions.doRecordHistory) {
                if (isInHistory) {
                  gameBoard.turns -= ((gameHistory.turns - 1) - gameHistory.currentTurn);
                }

                historyCreated = false; // Reset history to be not yet created
                isInHistory = false;
                clearHistory(&gameHistory);


              }

              set_options_message(&gameOptions, gameOptions.doRecordHistory ? "History enabled." : "History disabled and cleared.");

              break;
            case SDLK_r:
              // Init a radom playboard by pressing "r" key
              doRender = true;  // Restart processing and drawing in case the game ended
              historyCreated = false; // Reset history to be not yet created
              isInHistory = false;    // Reset that we are not anymore in history

              // Clear any history if present
              if (gameOptions.doRecordHistory) {
                clearHistory(&gameHistory);
              }

              // Init a random playboard
              initRandomBoard(&gameBoard, &gameOptions);

              set_options_message(&gameOptions, "Initialized random playboard.");
              sprintf(gameBoard.status, doPause ? "[PAUSED]" : "[RUNNING]");

              break;
            case SDLK_g:
              // Turn on and off the grid drawing, by pressing "g" key
              gameOptions.drawGrid = !gameOptions.drawGrid;
              set_options_message(&gameOptions, gameOptions.drawGrid ? "Grid turned on." : "Grid turned off." );

              break;
            case SDLK_a:
              // Turn animations on or off by pressing "a" key

              // Override value of storeAnimate while in paint mode
              if (doPaint) {
                storeAnimate = !storeAnimate;
                set_options_message(&gameOptions, storeAnimate ? "Animations turned on." : "Animations turned off.");
              } else {
                // Not painting, work on normal value
                gameOptions.showAnimations = !gameOptions.showAnimations;
                set_options_message(&gameOptions, gameOptions.showAnimations ? "Animations turned on." : "Animations turned off.");
              }

              break;
            case SDLK_SPACE:
              // Pause or play by pressing "space" key

              // If we have zero cells living, we dont unpause the game.
              if (doPause && gameBoard.livingCells == 0) {
                sprintf(gameBoard.status, "[PAUSED]");
                set_options_message(&gameOptions, "No living cells on playboard.");
                continue;
              }

              // Turn off paint mode if active, restore animations and false mousePressed
              if (doPaint) {
                doPaint = false;
                mousePressed = false;
                gameOptions.showAnimations = storeAnimate;
                doPause = false;
              } else {
                // Pause state change
                doPause = !doPause;
                set_options_message(&gameOptions, doPause ? "Game paused." : "Game unpaused.");
              }

              if (doPause) {
                sprintf(gameBoard.status, "[PAUSED]");
              } else {
                sprintf(gameBoard.status, "[RUNNING]");
              }

              break;
            default:
              break;
          }

          break;
        default:
          break;
      }
    }

    if (doPaint) {
      drawGameBoard(appWindow, drawingSurface, cairoSurface, drawingContext, &gameBoard, &gameOptions, &gameHistory);
      if (gameOptions.hasMessage || !mousePressed) {
        SDL_Delay(40);
      }
      continue;
    }

    if ((!doRender || (!animationInProgress && doPause)) && !gameOptions.hasMessage && gameOptions.activePanelItem == DISABLED) {
      // Delay the application execution waiting only for user input or while in pause mode after animation finish
      SDL_Delay(125);
      continue;
    }

    if (doRender && !doPause && gameBoard.livingCells != 0) {
      // Make a turn if we have no animations played and reaching turnTicks value using ticks
      if (!animationInProgress && ++ticks >= setTurnTicks) {
        ticks = 0;

        if (gameOptions.doRecordHistory) {

          // If we create a a history add the initial turn
          if (!isInHistory && !historyCreated) {
            historyCreated = addHistory(&gameHistory, &gameBoard);
          }

          // Check if we record a history and if we are "in history" mode
          if (isInHistory) {
            if (gameHistory.currentTurn == (gameHistory.turns - 1)) {
              // Now - is reached in history, disabled playback
              isInHistory = false;
              set_options_message(&gameOptions, "[HISTORY] Review disabled, preparing for future!");
            } else {
              // Display the next step in history, avoiding recalculation
              historyForwards(&gameHistory, &gameBoard);
            }
          }
        }

        // Check that we are not regenerating the playboard from history
        if (!isInHistory) {
          // Apply a turn and rules for birth and death
          applyTurn(&gameBoard, isInHistory);

          // If we create a a history add this turn
          if (gameOptions.doRecordHistory) {
            if (!addHistory(&gameHistory, &gameBoard)) {
              set_options_message(&gameOptions, "[HISTORY] Could not add to history, recording disabled.");
              gameOptions.doRecordHistory = false;
            }
          }
        }

        // Perform a check if no changes occured anymore
        if (!gameBoard.isDirty) {
          printf("[STATUS] Reached a stale state on the gameboard after %d turns.\n", gameBoard.turns);
          sprintf(gameBoard.status, "[FINISHED: STALE STATE]");
          doRender = false;
        }

      }
    }

    // Draw the basic game board and living cells
    animationInProgress = drawGameBoard(appWindow, drawingSurface, cairoSurface, drawingContext, &gameBoard, &gameOptions, &gameHistory);

    // Perform a check if all cells died in this turn
    if (!doPause && doRender && gameBoard.livingCells == 0) {

      // Set render state to false after the last cells animated out or had been cleared
      if (!animationInProgress || !gameOptions.showAnimations) {
        // Display: All cells dead on title and console and set rendering to false
        printf("[STATUS] All cells died after %d turns.\n", gameBoard.turns);
        sprintf(gameBoard.status, "[FINISHED: CELLS DEAD]");
        doRender = false;
      }
    }

    // Update the window title
    if (isInHistory) {
      sprintf(titleString, "%s - TURN: %u %s [HISTORY TURN %u]", windowBaseTitle, gameBoard.turns, gameBoard.status, gameHistory.currentTurn);
    } else {
      sprintf(titleString, "%s - TURN: %u %s", windowBaseTitle, gameBoard.turns, gameBoard.status);
    }
    SDL_SetWindowTitle(appWindow, titleString);

    // Delay the application execution
    if (!doPaint && !mousePressed) {
      SDL_Delay(40);
    }
  }

  //----------------------------------------------------------------------------
  // Cleanup
  free(gameBoard.cells);
  clearHistory(&gameHistory);

  // Cleanup cairo
  cairo_surface_destroy(cairoSurface);
  cairo_destroy(drawingContext);

  // Cleanup SDL
  SDL_FreeSurface(drawingSurface);
  SDL_DestroyWindow(appWindow);

  IMG_Quit();

  SDL_VideoQuit();
  SDL_Quit();

  //----------------------------------------------------------------------------
  printf("\n######### Finished program. #########\n\n");

  // Return exit success
  return EXIT_SUCCESS;
}
