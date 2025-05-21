#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"

#define MAX_SOUNDS 3
typedef enum {
    SOUND_CLICK = 0,
    SOUND_EXPLOSION,
    SOUND_FLAG
} sound_asset;
Sound sounds[MAX_SOUNDS];

#define MAX_MUSIC 1
typedef enum {
    MUSIC_BACKGROUND = 0
} music_asset;
Music music[MAX_MUSIC];

#define MAX_TEXTURES 1
typedef enum {
    TEXTURE_FLAG_IMG = 0
} texture_asset;
Texture2D textures[MAX_TEXTURES];

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600

#define MAX_BOARD_WIDTH 30
#define MAX_BOARD_HEIGHT 16

#define BASE_CELL_SIZE 32
#define CELL_PADDING_PERCENT 0.03f
#define STATUS_BAR_HEIGHT_PERCENT 0.08f

typedef enum {
    MENU,
    PLAYING,
    LOST,
    WON,
} GameState;

typedef struct {
    const char* name;
    int width;
    int height;
    int mines;
} DifficultyLevel;

typedef struct {
    bool isRevealed;
    bool isFlagged;
    bool hasMine;
    int neighborMines;
} Cell;

Cell gameBoard[MAX_BOARD_HEIGHT][MAX_BOARD_WIDTH];
int currentWidth = 9;
int currentHeight = 9;
int mineCount = 10;
int flagCount = 0;
bool gameIsOver = false;
bool playerWon = false;
GameState currentState = MENU;
bool isFirstClick = true;

bool isSoundEnabled = true;
bool isMusicEnabled = true;

DifficultyLevel gameLevels[] = {
    {"Beginner", 9, 9, 10},
    {"Intermediate", 16, 16, 40},
    {"Expert", 30, 16, 99},
};

float cellSize;
float cellPadding;
float statusHeight;

void InitGameAudio(void);
void ShutdownGameAudio(void);
void LoadGameTextures(void);
void UnloadGameTextures(void);
void ResetGame(void);
bool IsValidCell(int x, int y);
void CheckForWin(void);
void RevealCell(int x, int y);
void PlaceMines(int safeX, int safeY);
void DrawMainMenu(void);
void DrawGameBoard(void);
void HandlePlayerInput(void);
bool DrawStyledButton(float x, float y, float width, float height, const char* text, int fontSize, Color baseColor, Color hoverColor, Color textColor);
int CountSurroundingFlags(int x, int y);
void UpdateUIScaling(void);
void GamePlaySound(int sound);
void ToggleSoundEnabled(void);
void ToggleMusicEnabled(void);

bool IsMusicReady(Music music) {
    return music.frameCount > 0;
}

bool IsSoundReady(Sound sound) {
    return sound.frameCount > 0;
}

bool IsTextureReady(Texture2D texture) {
    return texture.id > 0;
}

bool IsMusicEnabled(void) {
    return isMusicEnabled;
}

void InitGameAudio(void) {
    InitAudioDevice();
    sounds[SOUND_CLICK] = LoadSound("assets\\click.wav");
    sounds[SOUND_EXPLOSION] = LoadSound("assets\\explosion.wav");
    sounds[SOUND_FLAG] = LoadSound("assets\\pickupCoin.wav");

    music[MUSIC_BACKGROUND] = LoadMusicStream("assets\\assets_8 - bit - game - 158815.mp3");

    if (IsMusicReady(music[MUSIC_BACKGROUND])) {
        SetMusicVolume(music[MUSIC_BACKGROUND], 0.5f);
        if (isMusicEnabled) {
            PlayMusicStream(music[MUSIC_BACKGROUND]);
        }
    }
    else {
        TraceLog(LOG_WARNING, "MUSIC: Failed to load background music.");
    }
}

void ShutdownGameAudio(void) {
    for (int i = 0; i < MAX_SOUNDS; i++) {
        if (IsSoundReady(sounds[i])) UnloadSound(sounds[i]);
    }
    if (IsMusicReady(music[MUSIC_BACKGROUND])) {
        StopMusicStream(music[MUSIC_BACKGROUND]);
        UnloadMusicStream(music[MUSIC_BACKGROUND]);
    }
    CloseAudioDevice();
}

void LoadGameTextures(void) {
    Image flagImg = LoadImage("assets\\flag.png");
    if (flagImg.data != NULL) {
        ImageResize(&flagImg, (int)(BASE_CELL_SIZE * 0.7f), (int)(BASE_CELL_SIZE * 0.7f));
        textures[TEXTURE_FLAG_IMG] = LoadTextureFromImage(flagImg);
        UnloadImage(flagImg);
    }
    else {
        TraceLog(LOG_WARNING, "TEXTURE: Failed to load flag.png");
    }
}

void UnloadGameTextures(void) {
    if (IsTextureReady(textures[TEXTURE_FLAG_IMG])) {
        UnloadTexture(textures[TEXTURE_FLAG_IMG]);
    }
}

void GamePlaySound(int sound) {
    if (isSoundEnabled && sound < MAX_SOUNDS && IsSoundReady(sounds[sound])) {
        PlaySound(sounds[sound]);
    }
}

void ToggleSoundEnabled(void) {
    isSoundEnabled = !isSoundEnabled;
    if (isSoundEnabled) GamePlaySound(SOUND_CLICK);
}

void ToggleMusicEnabled(void) {
    isMusicEnabled = !isMusicEnabled;
    if (IsMusicReady(music[MUSIC_BACKGROUND])) {
        if (isMusicEnabled) {
            PlayMusicStream(music[MUSIC_BACKGROUND]);
        }
        else {
            StopMusicStream(music[MUSIC_BACKGROUND]);
        }
    }
    if (isSoundEnabled) GamePlaySound(SOUND_CLICK);
}

void UpdateUIScaling(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    float availableBoardHeight = screenHeight * (1.0f - STATUS_BAR_HEIGHT_PERCENT);

    float widthBasedSize = (float)screenWidth / (currentWidth + (currentWidth + 1) * CELL_PADDING_PERCENT);
    float heightBasedSize = availableBoardHeight / (currentHeight + (currentHeight + 1) * CELL_PADDING_PERCENT);

    cellSize = fminf(widthBasedSize, heightBasedSize);

    cellSize = fminf(fmaxf(cellSize, 16.0f), 100.0f);

    cellPadding = cellSize * CELL_PADDING_PERCENT;
    statusHeight = screenHeight * STATUS_BAR_HEIGHT_PERCENT;
    statusHeight = fmaxf(statusHeight, 30.0f);
}

bool IsValidCell(int x, int y) {
    return x >= 0 && x < currentWidth && y >= 0 && y < currentHeight;
}

void CheckForWin(void) {
    if (gameIsOver) return;

    for (int y = 0; y < currentHeight; y++) {
        for (int x = 0; x < currentWidth; x++) {
            if (!gameBoard[y][x].hasMine && !gameBoard[y][x].isRevealed) {
                return;
            }
        }
    }
    playerWon = true;
    currentState = WON;
    gameIsOver = true;
    GamePlaySound(SOUND_FLAG);
}

void RevealCell(int x, int y) {
    if (!IsValidCell(x, y)) return;

    Cell* cell = &gameBoard[y][x];
    if (cell->isRevealed || cell->isFlagged) return;

    cell->isRevealed = true;

    if (cell->hasMine) {
        gameIsOver = true;
        currentState = LOST;
        GamePlaySound(SOUND_EXPLOSION);
        for (int r = 0; r < currentHeight; r++) {
            for (int c = 0; c < currentWidth; c++) {
                if (gameBoard[r][c].hasMine) gameBoard[r][c].isRevealed = true;
            }
        }
        return;
    }
    else {
        if (!gameIsOver) GamePlaySound(SOUND_CLICK);
    }

    if (cell->neighborMines == 0) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                RevealCell(x + dx, y + dy);
            }
        }
    }
}

void PlaceMines(int safeX, int safeY) {
    for (int y = 0; y < currentHeight; y++) {
        for (int x = 0; x < currentWidth; x++) {
            gameBoard[y][x].hasMine = false;
            gameBoard[y][x].neighborMines = 0;
        }
    }

    bool safeZone[MAX_BOARD_HEIGHT][MAX_BOARD_WIDTH] = { false };

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int nx = safeX + dx;
            int ny = safeY + dy;
            if (IsValidCell(nx, ny)) {
                safeZone[ny][nx] = true;
            }
        }
    }

    int placedMines = 0;
    while (placedMines < mineCount) {
        int x = GetRandomValue(0, currentWidth - 1);
        int y = GetRandomValue(0, currentHeight - 1);

        if (safeZone[y][x] || gameBoard[y][x].hasMine) continue;

        gameBoard[y][x].hasMine = true;
        placedMines++;
    }

    for (int y = 0; y < currentHeight; y++) {
        for (int x = 0; x < currentWidth; x++) {
            if (gameBoard[y][x].hasMine) continue;

            int surroundingMineCount = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (IsValidCell(nx, ny) && gameBoard[ny][nx].hasMine) {
                        surroundingMineCount++;
                    }
                }
            }
            gameBoard[y][x].neighborMines = surroundingMineCount;
        }
    }
}

void DrawMainMenu(void) {
    ClearBackground(RAYWHITE);

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    const char* title = "MINESWEEPER";
    int titleSize = screenW / 12;
    titleSize = fmaxf(titleSize, 40);
    titleSize = fminf(titleSize, 100);
    DrawText(title, screenW / 2 - MeasureText(title, titleSize) / 2,
        screenH * 0.15f, titleSize, DARKBLUE);

    float btnWidth = screenW * 0.35f;
    btnWidth = fmaxf(btnWidth, 200);
    btnWidth = fminf(btnWidth, 400);
    float btnHeight = screenH * 0.08f;
    btnHeight = fmaxf(btnHeight, 40);
    float btnSpacing = screenH * 0.04f;
    int btnTextSize = (int)(btnHeight * 0.4f);
    btnTextSize = fmaxf(btnTextSize, 16);

    for (int i = 0; i < 3; i++) {
        float btnY = screenH * 0.4f + i * (btnHeight + btnSpacing);
        if (DrawStyledButton(screenW / 2 - btnWidth / 2, btnY, btnWidth, btnHeight, gameLevels[i].name, btnTextSize, LIGHTGRAY, GRAY, BLACK)) {
            GamePlaySound(SOUND_CLICK);
            currentWidth = gameLevels[i].width;
            currentHeight = gameLevels[i].height;
            mineCount = gameLevels[i].mines;

            float idealWidth = currentWidth * BASE_CELL_SIZE + (currentWidth + 1) * (BASE_CELL_SIZE * CELL_PADDING_PERCENT);
            float idealHeight = currentHeight * BASE_CELL_SIZE + (currentHeight + 1) * (BASE_CELL_SIZE * CELL_PADDING_PERCENT) + (DEFAULT_HEIGHT * STATUS_BAR_HEIGHT_PERCENT);

            idealWidth = fmaxf(idealWidth, DEFAULT_WIDTH * 0.5f);
            idealHeight = fmaxf(idealHeight, DEFAULT_HEIGHT * 0.5f);

            SetWindowSize((int)idealWidth, (int)idealHeight);
            UpdateUIScaling();

            int monitorWidth = GetMonitorWidth(GetCurrentMonitor());
            int monitorHeight = GetMonitorHeight(GetCurrentMonitor());
            SetWindowPosition(monitorWidth / 2 - GetScreenWidth() / 2, monitorHeight / 2 - GetScreenHeight() / 2);

            ResetGame();
            currentState = PLAYING;
        }
    }

    float toggleBtnWidth = screenW * 0.3f;
    toggleBtnWidth = fmaxf(toggleBtnWidth, 180);
    toggleBtnWidth = fminf(toggleBtnWidth, 300);
    float toggleBtnHeight = screenH * 0.06f;
    toggleBtnHeight = fmaxf(toggleBtnHeight, 35);
    int toggleBtnTextSize = (int)(toggleBtnHeight * 0.45f);
    toggleBtnTextSize = fmaxf(toggleBtnTextSize, 14);

    char soundButtonText[32];
    snprintf(soundButtonText, sizeof(soundButtonText), "Sound: %s", isSoundEnabled ? "ON" : "OFF");

    if (DrawStyledButton(screenW / 2 - toggleBtnWidth / 2, screenH * 0.80f, toggleBtnWidth, toggleBtnHeight, soundButtonText, toggleBtnTextSize, SKYBLUE, BLUE, DARKBLUE)) {
        ToggleSoundEnabled();
    }

    char musicButtonText[32];
    snprintf(musicButtonText, sizeof(musicButtonText), "Music: %s", isMusicEnabled ? "ON" : "OFF");

    if (DrawStyledButton(screenW / 2 - toggleBtnWidth / 2, screenH * 0.80f + toggleBtnHeight + btnSpacing * 0.5f, toggleBtnWidth, toggleBtnHeight, musicButtonText, toggleBtnTextSize, SKYBLUE, BLUE, DARKBLUE)) {
        ToggleMusicEnabled();
    }
}

void DrawGameBoard(void) {
    ClearBackground(RAYWHITE);

    float totalBoardDrawingWidth = currentWidth * cellSize + (currentWidth + 1) * cellPadding;
    float totalBoardDrawingHeight = currentHeight * cellSize + (currentHeight + 1) * cellPadding;

    float offsetX = (GetScreenWidth() - totalBoardDrawingWidth) / 2;
    float offsetY = (GetScreenHeight() - statusHeight - totalBoardDrawingHeight) / 2;

    offsetX = fmaxf(offsetX, 0);
    offsetY = fmaxf(offsetY, 0);

    for (int y = 0; y < currentHeight; y++) {
        for (int x = 0; x < currentWidth; x++) {
            Cell cell = gameBoard[y][x];
            float cellX = offsetX + x * (cellSize + cellPadding) + cellPadding;
            float cellY = offsetY + y * (cellSize + cellPadding) + cellPadding;
            Rectangle cellRect = { cellX, cellY, cellSize, cellSize };

            DrawRectangleLinesEx(cellRect, 1, DARKGRAY);

            if (cell.isRevealed) {
                DrawRectangleRec(cellRect, LIGHTGRAY);
                if (cell.hasMine) {
                    DrawCircle(cellX + cellSize / 2, cellY + cellSize / 2, cellSize * 0.3f, RED);
                }
                else if (cell.neighborMines > 0) {
                    char numText[2];
                    snprintf(numText, sizeof(numText), "%d", cell.neighborMines);

                    Color numColor;
                    switch (cell.neighborMines) {
                    case 1: numColor = BLUE; break;
                    case 2: numColor = GREEN; break;
                    case 3: numColor = RED; break;
                    case 4: numColor = DARKBLUE; break;
                    case 5: numColor = MAROON; break;
                    case 6: numColor = DARKBROWN; break;
                    case 7: numColor = BLACK; break;
                    case 8: numColor = DARKGRAY; break;
                    default: numColor = BLACK; break;
                    }
                    int numSize = (int)(cellSize * 0.6f);
                    numSize = fmaxf(numSize, 10);
                    DrawText(numText,
                        cellX + (cellSize - MeasureText(numText, numSize)) / 2,
                        cellY + (cellSize - numSize) / 2,
                        numSize, numColor);
                }
            }
            else {
                DrawRectangleRec(cellRect, GRAY);
                if (cell.isFlagged) {
                    if (IsTextureReady(textures[TEXTURE_FLAG_IMG])) {
                        DrawTexturePro(textures[TEXTURE_FLAG_IMG],
                            (Rectangle) {
                            0, 0, (float)textures[TEXTURE_FLAG_IMG].width, (float)textures[TEXTURE_FLAG_IMG].height
                        },
                            (Rectangle) {
                            cellX + cellSize * 0.15f, cellY + cellSize * 0.15f, cellSize * 0.7f, cellSize * 0.7f
                        },
                            (Vector2) {
                            0, 0
                        }, 0.0f, WHITE);
                    }
                    else {
                        int flagSize = (int)(cellSize * 0.6f);
                        flagSize = fmaxf(flagSize, 10);
                        DrawText("F", cellX + (cellSize - MeasureText("F", flagSize)) / 2, cellY + (cellSize - flagSize) / 2, flagSize, MAROON);
                    }
                }
            }
        }
    }

    DrawRectangle(0, GetScreenHeight() - statusHeight, GetScreenWidth(), statusHeight, DARKGRAY);

    char statusText[50];
    snprintf(statusText, sizeof(statusText), "Flags: %d/%d", flagCount, mineCount);

    int statusTextSize = (int)(statusHeight * 0.5f);
    statusTextSize = fmaxf(statusTextSize, 16);
    DrawText(statusText, 10, GetScreenHeight() - statusHeight + (statusHeight - statusTextSize) / 2, statusTextSize, RAYWHITE);

    if (currentState == LOST || currentState == WON) {
        const char* message = (currentState == LOST) ? "Game Over!" : "You Win!";
        Color messageColor = (currentState == LOST) ? RED : GREEN;

        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight() - statusHeight, Fade(BLACK, 0.5f));

        int messageSize = GetScreenHeight() / 12;
        messageSize = fmaxf(messageSize, 30);
        DrawText(message, GetScreenWidth() / 2 - MeasureText(message, messageSize) / 2,
            GetScreenHeight() * 0.3f, messageSize, messageColor);

        float btnWidth = GetScreenWidth() * 0.3f;
        btnWidth = fmaxf(btnWidth, 180);
        float btnHeight = GetScreenHeight() * 0.07f;
        btnHeight = fmaxf(btnHeight, 40);
        int btnTextSize = (int)(btnHeight * 0.4f);
        btnTextSize = fmaxf(btnTextSize, 16);

        if (DrawStyledButton(GetScreenWidth() / 2 - btnWidth / 2, GetScreenHeight() * 0.5f, btnWidth, btnHeight, "Play Again", btnTextSize, LIGHTGRAY, GRAY, BLACK)) {
            GamePlaySound(SOUND_CLICK);
            currentState = MENU;
            SetWindowSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
            UpdateUIScaling();
            int monitorWidth = GetMonitorWidth(GetCurrentMonitor());
            int monitorHeight = GetMonitorHeight(GetCurrentMonitor());
            SetWindowPosition(monitorWidth / 2 - GetScreenWidth() / 2, monitorHeight / 2 - GetScreenHeight() / 2);
        }
    }
}

void ResetGame(void) {
    for (int y = 0; y < MAX_BOARD_HEIGHT; y++) {
        for (int x = 0; x < MAX_BOARD_WIDTH; x++) {
            gameBoard[y][x].isRevealed = false;
            gameBoard[y][x].isFlagged = false;
            gameBoard[y][x].hasMine = false;
            gameBoard[y][x].neighborMines = 0;
        }
    }
    flagCount = 0;
    gameIsOver = false;
    playerWon = false;
    isFirstClick = true;
}

int CountSurroundingFlags(int x, int y) {
    int count = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx;
            int ny = y + dy;
            if (IsValidCell(nx, ny) && gameBoard[ny][nx].isFlagged) {
                count++;
            }
        }
    }
    return count;
}

void HandlePlayerInput(void) {
    if (currentState != PLAYING || gameIsOver) return;

    Vector2 mousePos = GetMousePosition();

    float totalBoardDrawingWidth = currentWidth * cellSize + (currentWidth + 1) * cellPadding;
    float totalBoardDrawingHeight = currentHeight * cellSize + (currentHeight + 1) * cellPadding;
    float offsetX = (GetScreenWidth() - totalBoardDrawingWidth) / 2;
    float offsetY = (GetScreenHeight() - statusHeight - totalBoardDrawingHeight) / 2;
    offsetX = fmaxf(offsetX, 0);
    offsetY = fmaxf(offsetY, 0);

    float relativeMouseX = mousePos.x - offsetX;
    float relativeMouseY = mousePos.y - offsetY;

    int cellX = (int)(relativeMouseX / (cellSize + cellPadding));
    int cellY = (int)(relativeMouseY / (cellSize + cellPadding));

    float clickXInCellUnits = relativeMouseX / (cellSize + cellPadding);
    float clickYInCellUnits = relativeMouseY / (cellSize + cellPadding);

    bool clickedOnPadding = (clickXInCellUnits - cellX > cellSize / (cellSize + cellPadding)) ||
        (clickYInCellUnits - cellY > cellSize / (cellSize + cellPadding));

    if (clickedOnPadding && (relativeMouseX < cellPadding || relativeMouseY < cellPadding)) return;
    if (mousePos.y > GetScreenHeight() - statusHeight) return;
    if (!IsValidCell(cellX, cellY)) return;

    Cell* clickedCell = &gameBoard[cellY][cellX];

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (!clickedCell->isRevealed && !clickedCell->isFlagged) {
            if (isFirstClick) {
                PlaceMines(cellX, cellY);
                isFirstClick = false;
            }
            RevealCell(cellX, cellY);
            if (!gameIsOver) CheckForWin();
        }
        else if (clickedCell->isRevealed && clickedCell->neighborMines > 0) {
            int flagsAround = CountSurroundingFlags(cellX, cellY);
            if (flagsAround == clickedCell->neighborMines) {
                GamePlaySound(SOUND_CLICK);
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = cellX + dx;
                        int ny = cellY + dy;
                        if (IsValidCell(nx, ny) && !gameBoard[ny][nx].isFlagged && !gameBoard[ny][nx].isRevealed) {
                            RevealCell(nx, ny);
                            if (gameIsOver && currentState == LOST) return;
                        }
                    }
                }
                if (!gameIsOver) CheckForWin();
            }
        }
    }

    else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        if (!clickedCell->isRevealed) {
            clickedCell->isFlagged = !clickedCell->isFlagged;
            flagCount += clickedCell->isFlagged ? 1 : -1;
            GamePlaySound(SOUND_FLAG);
        }
    }
}

bool DrawStyledButton(float x, float y, float width, float height, const char* text, int fontSize, Color baseColor, Color hoverColor, Color textColor) {
    Rectangle btnRect = { x, y, width, height };
    Vector2 mousePos = GetMousePosition();
    bool isHovered = CheckCollisionPointRec(mousePos, btnRect);

    DrawRectangleRec(btnRect, isHovered ? hoverColor : baseColor);
    DrawRectangleLinesEx(btnRect, 2, DARKGRAY);

    int textWidth = MeasureText(text, fontSize);
    DrawText(text, x + (width - textWidth) / 2, y + (height - fontSize) / 2, fontSize, textColor);

    return isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "Minesweeper - Raylib Edition");
    SetTargetFPS(60);

    SetRandomSeed(time(NULL));

    InitGameAudio();
    LoadGameTextures();

    int monitorWidth = GetMonitorWidth(GetCurrentMonitor());
    int monitorHeight = GetMonitorHeight(GetCurrentMonitor());
    SetWindowPosition(monitorWidth / 2 - DEFAULT_WIDTH / 2, monitorHeight / 2 - DEFAULT_HEIGHT / 2);

    UpdateUIScaling();

    while (!WindowShouldClose()) {
        if (IsMusicEnabled() && IsMusicReady(music[MUSIC_BACKGROUND])) {
            UpdateMusicStream(music[MUSIC_BACKGROUND]);
        }

        if (IsWindowResized()) {
            UpdateUIScaling();
        }

        switch (currentState) {
        case MENU:
            break;
        case PLAYING:
            HandlePlayerInput();
            break;
        case LOST:
        case WON:
            break;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (currentState) {
        case MENU:
            DrawMainMenu();
            break;
        case PLAYING:
        case LOST:
        case WON:
            DrawGameBoard();
            break;
        }

        EndDrawing();
    }

    UnloadGameTextures();
    ShutdownGameAudio();
    CloseWindow();

    return 0;
}