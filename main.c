#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define NUM_DISKS 3
#define ROD_WIDTH 10
#define DISK_HEIGHT 30
#define MAX_MOVES 100  // Maximum moves for the undo stack

// Disk structure to hold position and size
typedef struct {
    int width;
    int height;
    int x;
    int y;
    SDL_Color color;
    bool isDragging;  // To track if the disk is being dragged
} Disk;

// Rod positions
int rod_positions[3] = {200, 400, 600};

// Disk array to hold the disks for each rod
Disk rods[3][NUM_DISKS];
int num_disks_on_rod[3];  // Disk count on each rod

// Dragging variables
Disk* draggedDisk = NULL;  // Currently dragged disk
int draggedFromRod = -1;  // Which rod the disk was picked from

bool gameWon = false;  // Flag to track if the game is won
int totalMoves = 0;  // Counter for total moves

// Structure to represent a move
typedef struct {
    int fromRod;
    int toRod;
    Disk disk;
} Move;

// Stack for undoing moves
Move undoStack[MAX_MOVES];
int top = -1;  // Stack top index

// Function to initialize the disks
void initDisks() {
    num_disks_on_rod[0] = NUM_DISKS;  // Set the count for the first rod
    num_disks_on_rod[1] = 0;  // No disks on the second rod
    num_disks_on_rod[2] = 0;  // No disks on the third rod

    for (int i = 0; i < NUM_DISKS; i++) {
        rods[0][i].width = 150 - i * 50;
        rods[0][i].height = DISK_HEIGHT;
        rods[0][i].x = rod_positions[0] - rods[0][i].width / 2;
        rods[0][i].y = SCREEN_HEIGHT - (i + 1) * DISK_HEIGHT;
        rods[0][i].isDragging = false;

        // Assign different colors for the disks
        rods[0][i].color = (SDL_Color) {255 - i * 50, i * 80, 255 - i * 60, 255};
    }
}

// Function to render the rods and disks
void renderGame(SDL_Renderer* renderer) {
    // Set background color to #0b1cf4 (blue)
    SDL_SetRenderDrawColor(renderer, 11, 28, 244, 255);
    SDL_RenderClear(renderer);

    // Draw rods
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < 3; i++) {
        SDL_Rect rod = {rod_positions[i] - ROD_WIDTH / 2, 100, ROD_WIDTH, SCREEN_HEIGHT - 100};
        SDL_RenderFillRect(renderer, &rod);
    }

    // Draw disks as rectangles
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < num_disks_on_rod[i]; j++) {
            Disk* disk = &rods[i][j];
            SDL_SetRenderDrawColor(renderer, disk->color.r, disk->color.g, disk->color.b, 255);
            SDL_Rect rect = {disk->x, disk->y, disk->width, disk->height};
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    // Update screen
    SDL_RenderPresent(renderer);
}

// Function to check if a disk can be dropped onto a rod
bool canMove(int rod_from, int rod_to) {
    if (num_disks_on_rod[rod_from] == 0) return false;  // No disks to move
    if (num_disks_on_rod[rod_to] == 0) return true;  // If the destination rod is empty
    Disk* top_from = &rods[rod_from][num_disks_on_rod[rod_from] - 1];
    Disk* top_to = &rods[rod_to][num_disks_on_rod[rod_to] - 1];
    return top_from->width < top_to->width;  // Only move if the disk is smaller
}

// Function to move a disk
void moveDisk(int rod_from, int rod_to) {
    if (!canMove(rod_from, rod_to)) return;

    // Get the top disk from the source rod
    Disk disk = rods[rod_from][num_disks_on_rod[rod_from] - 1];
    num_disks_on_rod[rod_from]--;

    // Place the disk on the destination rod
    disk.x = rod_positions[rod_to] - disk.width / 2;
    disk.y = SCREEN_HEIGHT - (num_disks_on_rod[rod_to] + 1) * DISK_HEIGHT;
    rods[rod_to][num_disks_on_rod[rod_to]] = disk;
    num_disks_on_rod[rod_to]++;

    // Record the move for undoing
    if (top < MAX_MOVES - 1) {
        top++;
        undoStack[top].fromRod = rod_from;
        undoStack[top].toRod = rod_to;
        undoStack[top].disk = disk;  // Store the disk's current state
    }

    // Log the move and increment the total move count
    totalMoves++;
    printf("Disk %d moved from rod %d to rod %d.\n", NUM_DISKS - num_disks_on_rod[rod_from], rod_from + 1, rod_to + 1);
}

// Function to undo the last move
void undoLastMove() {
    if (top < 0) return;  // No moves to undo

    Move lastMove = undoStack[top];  // Get the last move
    top--;  // Remove the last move from the stack

    // Move the disk back to the original rod
    num_disks_on_rod[lastMove.toRod]--;
    num_disks_on_rod[lastMove.fromRod]++;

    // Update the disk's position back to the original rod
    lastMove.disk.x = rod_positions[lastMove.fromRod] - lastMove.disk.width / 2;
    lastMove.disk.y = SCREEN_HEIGHT - (num_disks_on_rod[lastMove.fromRod]) * DISK_HEIGHT;
    rods[lastMove.fromRod][num_disks_on_rod[lastMove.fromRod] - 1] = lastMove.disk;  // Restore the disk on the original rod

    // Decrement total moves since we're undoing
    totalMoves--;
}

// Function to check if the game has been won (all disks on the third rod)
void checkWinCondition() {
    if (num_disks_on_rod[2] == NUM_DISKS) {
        gameWon = true;  // Set the flag that the game is won
        printf("You Win! Total moves: %d\n", totalMoves);  // Print the winning message with total moves
    }
}

// Function to find which rod the mouse is hovering over
int getRodAtPosition(int mouseX) {
    for (int i = 0; i < 3; i++) {
        if (mouseX >= rod_positions[i] - 50 && mouseX <= rod_positions[i] + 50) {
            return i;
        }
    }
    return -1;
}

// Function to restart the game
void restartGame() {
    initDisks();
    gameWon = false;  // Reset the win condition
    top = -1;  // Reset the undo stack
    totalMoves = 0;  // Reset total moves
}

int main(int argc, char* args[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create window
    SDL_Window* window = SDL_CreateWindow("Tower of Hanoi Drag and Drop", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize disks
    initDisks();

    bool quit = false;
    SDL_Event e;

    // Main loop
    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;  // Quit the game
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    int rod = getRodAtPosition(mouseX);
                    if (rod != -1 && num_disks_on_rod[rod] > 0) {
                        draggedDisk = &rods[rod][num_disks_on_rod[rod] - 1];  // Start dragging the top disk
                        draggedFromRod = rod;
                        draggedDisk->isDragging = true;
                    }
                }
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                if (draggedDisk != NULL) {
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    int rod = getRodAtPosition(mouseX);
                    if (rod != -1 && rod != draggedFromRod) {
                        moveDisk(draggedFromRod, rod);  // Move the disk
                        checkWinCondition();  // Check win condition
                    }
                    draggedDisk->isDragging = false;  // Stop dragging
                    draggedDisk = NULL;  // Clear the dragged disk
                }
            } else if (e.type == SDL_MOUSEMOTION) {
                if (draggedDisk != NULL) {
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    draggedDisk->x = mouseX - draggedDisk->width / 2;  // Update dragged disk position
                    draggedDisk->y = mouseY - DISK_HEIGHT / 2;
                }
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.mod & KMOD_CTRL && e.key.keysym.sym == SDLK_z) {
                    undoLastMove();  // Undo last move on Ctrl + Z
                }
                if (e.key.keysym.sym == SDLK_r) {  // Check if "R" key is pressed
                    restartGame();  // Restart the game
                }
            }
        }

        // Render the game
        renderGame(renderer);
    }

    // Clean up
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
