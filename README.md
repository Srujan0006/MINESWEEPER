# MINESWEEPER-RAYLIB
A classic Minesweeper game built in C using the raylib graphics library.It features a graphical user interface, multiple difficulty levels, and responsive UI scaling.
## Features
* Difficulty Levels: Choose from Beginner, Intermediate, and Expert presets.
* Dynamic Board Sizing: The window resizes to fit the selected difficulty, and the UI scales dynamically with window resizing.
* Standard Minesweeper Gameplay:
* Left-click to reveal cells.
* Right-click to flag/unflag cells.
* Chord (right-click on a revealed numbered cell with the correct number of flags around it) to reveal surrounding unflagged cells.
* First Click Safety: The first click will always be on a safe cell (not a mine), and a 3x3 area around it is guaranteed to be mine-free.
* Game Over/Win States: Clear visual indications for winning and losing the game, with a "Play Again" option.
* Mine Counter: Displays the number of flags currently placed versus the total number of mines.
## How to Play
Run the executable: Execute the compiled program.
#### Main Menu:
Select a difficulty level (Beginner, Intermediate, Expert) by clicking on the corresponding button.
The window will resize to fit the selected board size.
#### Gameplay:
* Left-click on a cell to reveal it.
* If you click on a mine, the game is over.
* If you click on an empty cell, it and neighboring empty cells will automatically reveal.
* If you click on a numbered cell, it will reveal the number of adjacent mines.
* Right-click on an unrevealed cell to place a flag (marking it as a suspected mine) or remove a flag.
* When a numbered cell is revealed, and you have placed the correct number of flags around it, right-click on that revealed numbered cell to perform a chord. This will reveal all unflagged adjacent cells. Be careful, if you've incorrectly flagged, this can lead to revealing a mine!
* Win Condition: Reveal all non-mine cells to win the game.
* Game Over/Win Screen: After the game ends (win or lose), a message will appear, and you can click "Play Again" to return to the main menu.
## Building and Running
#### Prerequisites
* Raylib Library: You need to have the Raylib library installed and configured for your development environment. You can find instructions on the [Raylib](https://www.raylib.com/) website.
* C Compiler: A C compiler (e.g., GCC) is required.
