#include <windows.h>
#include <windowsx.h> // Include windowsx.h for GET_X_LPARAM and GET_Y_LPARAM
#include <vector>
#include <ctime>

// Define grid size and mine count
const int GRID_SIZE = 10;
int NUM_MINES = 10; // You can adjust the number of mines as needed
const int CELL_SIZE = 30; // Define cell size
int windowWidth = CELL_SIZE * GRID_SIZE + 30;
int windowHeight = CELL_SIZE * GRID_SIZE + 160;
bool GameOver = false;
int remainingMines = NUM_MINES;
int flaggedMines = 0;

std::vector<bool> isMine(GRID_SIZE* GRID_SIZE, false);
std::vector<bool> isFlagged(GRID_SIZE* GRID_SIZE, false);
std::vector<bool> isChecked(GRID_SIZE* GRID_SIZE, false);
std::vector<bool> isRightClicked(GRID_SIZE* GRID_SIZE, false); // Track right-clicks

HWND hLabel = NULL; // Global variable to store the label control handle
HWND hMineLabel = NULL;
HWND hRestartButton = NULL;
HWND hExitButton = NULL;
HWND hUtilityWnd = NULL;
HWND hDialogButton = NULL;
HWND hComboBox = NULL;

// Define constants for difficulty levels
enum Difficulty {
    Noob = 1,
    Easy = 5,
    Normal = 10,
    Hard = 15,
    Pain = 20
};

Difficulty currentDifficulty = Normal; // Default difficulty

HHOOK g_hHook = NULL;
HWND g_hMainWindow = NULL;
bool g_bUtilityWindowVisible = false;

void ToggleUtilityWindowVisibility() {
    g_bUtilityWindowVisible = !g_bUtilityWindowVisible;
    ShowWindow(hUtilityWnd, g_bUtilityWindowVisible ? SW_SHOW : SW_HIDE);
}

// Function to randomly place mines on the grid
void PlaceMines() {
    srand(static_cast<unsigned>(time(nullptr)));

    int minesPlaced = 0;
    while (minesPlaced < NUM_MINES) {
        int randomPosition = rand() % (GRID_SIZE * GRID_SIZE);
        if (!isMine[randomPosition]) {
            isMine[randomPosition] = true;
            minesPlaced++;
        }
    }
}

// Function to draw the grid cells
void DrawGridCells(HDC hdc) {
    for (int row = 0; row < GRID_SIZE; ++row) {
        for (int col = 0; col < GRID_SIZE; ++col) {
            // Calculate cell position
            int x = col * CELL_SIZE;
            int y = row * CELL_SIZE;

            int dr[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
            int dc[] = { -1, 0, 1, -1, 1, -1, 0, 1 };

            int mineCount = 0;

            for (int i = 0; i < 8; ++i) {
                int newRow = row + dr[i];
                int newCol = col + dc[i];

                // Check if the neighboring cell is within bounds
                if (newRow >= 0 && newRow < GRID_SIZE && newCol >= 0 && newCol < GRID_SIZE) {
                    int cellId = newRow * GRID_SIZE + newCol;
                    if (isMine[cellId]) {
                        mineCount++;
                    }
                }
            }
            // Create a white brush for non-flagged cells
            HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));

            // Check if the cell is flagged
            int cellId = row * GRID_SIZE + col;
            if (isFlagged[cellId]) {
                // Change the brush to red for flagged cells
                DeleteObject(hBrush); // Delete the previous white brush
                hBrush = CreateSolidBrush(RGB(255, 0, 0));
            }
            else if (isChecked[cellId])
            {
                DeleteObject(hBrush); // Delete the previous white brush
                hBrush = CreateSolidBrush(RGB(192, 192, 192));
            }

            RECT cellRect = { x, y, x + CELL_SIZE, y + CELL_SIZE }; // Create a RECT structure

            // Fill the cell with the selected brush color
            FillRect(hdc, &cellRect, hBrush);

            if (mineCount > 0 && isChecked[cellId]) {
                // Display mine count in the cell for cells with nearby mines
                SetBkMode(hdc, TRANSPARENT);
                wchar_t mineCountStr[2];
                swprintf_s(mineCountStr, L"%d", mineCount);
                DrawText(hdc, mineCountStr, -1, &cellRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            // Draw an outline around the cell
            FrameRect(hdc, &cellRect, CreateSolidBrush(RGB(0, 0, 0)));

            // Delete the brush
            DeleteObject(hBrush);

            // You can add logic here to draw mines and safe spaces based on the game state
        }
    }
}

bool CheckWinCondition() {
    for (int row = 0; row < GRID_SIZE; ++row) {
        for (int col = 0; col < GRID_SIZE; ++col) {
            int cellId = row * GRID_SIZE + col;
            // If any empty cell is not revealed or any mine is not flagged, the game is not won
            if ((!isMine[cellId] && isFlagged[cellId]) || (isMine[cellId] && !isFlagged[cellId])) {
                return false;
            }
        }
    }
    return true;
}

void YouWin() {
    SetWindowText(hLabel, L"YOU WIN");
    GameOver = true;
}

// Function to reveal empty cells and count mines in the vicinity
void RevealEmptyCells(int row, int col, HDC hdc) {
    // Array of neighboring offsets (8 positions)
    int dr[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    int dc[] = { -1, 0, 1, -1, 1, -1, 0, 1 };

    int mineCount = 0;

    for (int i = 0; i < 8; ++i) {
        int newRow = row + dr[i];
        int newCol = col + dc[i];

        // Check if the neighboring cell is within bounds
        if (newRow >= 0 && newRow < GRID_SIZE && newCol >= 0 && newCol < GRID_SIZE) {
            int cellId = newRow * GRID_SIZE + newCol;
            if (isMine[cellId]) {
                mineCount++;
            }
        }
    }

    // Calculate cell position
    int x = col * CELL_SIZE;
    int y = row * CELL_SIZE;

    // Create a white brush for non-flagged cells
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));

    RECT cellRect = { x, y, x + CELL_SIZE, y + CELL_SIZE };

    // Gray out revealed cells
    hBrush = CreateSolidBrush(RGB(192, 192, 192));

    

    // Fill the cell with the selected brush color
    FillRect(hdc, &cellRect, hBrush);

    if (mineCount > 0) {
        // Display mine count in the cell for cells with nearby mines
        SetBkMode(hdc, TRANSPARENT);
        wchar_t mineCountStr[2];
        swprintf_s(mineCountStr, L"%d", mineCount);
        DrawText(hdc, mineCountStr, -1, &cellRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // Draw an outline around the cell
    FrameRect(hdc, &cellRect, CreateSolidBrush(RGB(0, 0, 0)));

    // Delete the brush
    DeleteObject(hBrush);

    // Recursively reveal neighboring empty cells if mine count is 0
    if (mineCount == 0) {
        for (int i = 0; i < 8; ++i) {
            int newRow = row + dr[i];
            int newCol = col + dc[i];

            // Check if the neighboring cell is within bounds
            if (newRow >= 0 && newRow < GRID_SIZE && newCol >= 0 && newCol < GRID_SIZE) {
                int cellId = newRow * GRID_SIZE + newCol;
                if (!isFlagged[cellId] && !isChecked[cellId] && !isMine[cellId]) {
                    isChecked[cellId] = true;
                    RevealEmptyCells(newRow, newCol, hdc);
                }
            }
        }
    }
}




// Function to show "YOU DIED" on the label and lock gameplay
void YouDied() {
    SetWindowText(hLabel, L"YOU DIED");
    GameOver = true;
}

void RestartGame(Difficulty newDifficulty) {
    // Reset game variables and redraw the grid
    GameOver = false;
    isMine.assign(GRID_SIZE * GRID_SIZE, false);
    isFlagged.assign(GRID_SIZE * GRID_SIZE, false);
    isChecked.assign(GRID_SIZE * GRID_SIZE, false);

    switch (newDifficulty) {
    case 1:
        NUM_MINES = 1;
        break;
    case 2:
        NUM_MINES = 5;
        break;
    case 3:
        NUM_MINES = 10;
        break;
    case 4:
        NUM_MINES = 15;
        break;
    case 5:
        NUM_MINES = 20;
        break;
    }

    PlaceMines();
    InvalidateRect(g_hMainWindow, NULL, FALSE);
    SetWindowText(hLabel, L""); // Clear the label
    remainingMines = NUM_MINES;
    flaggedMines = 0;
    wchar_t mineCountStr[50];
    swprintf_s(mineCountStr, L"Mines Remaining: %d", NUM_MINES - flaggedMines);
    SetWindowText(hMineLabel, mineCountStr);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        // Create the label control
        // Create the "Restart" button
        hRestartButton = CreateWindow(L"BUTTON", L"Restart", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, windowHeight - 130, 80, 30, hWnd, (HMENU)1001, NULL, NULL);

        // Create the "Exit" button
        hExitButton = CreateWindow(L"BUTTON", L"Exit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            100, windowHeight - 130, 80, 30, hWnd, (HMENU)1002, NULL, NULL);
        hLabel = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
            0, windowHeight - 160, windowWidth, 30, hWnd, NULL, NULL, NULL);
        hMineLabel = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
            0, windowHeight - 70, windowWidth, 30, hWnd, NULL, NULL, NULL);
        SetWindowText(hLabel, L""); // Initially empty text
        wchar_t mineCountStr[50];
        swprintf_s(mineCountStr, L"Mines Remaining: %d", NUM_MINES - flaggedMines);
        SetWindowText(hMineLabel, mineCountStr);
        PlaceMines(); // Randomly place mines
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1001: // Restart button
            // Reset game variables and redraw the grid
            GameOver = false;
            isMine.assign(GRID_SIZE * GRID_SIZE, false);
            isFlagged.assign(GRID_SIZE * GRID_SIZE, false);
            isChecked.assign(GRID_SIZE * GRID_SIZE, false);
            PlaceMines();
            InvalidateRect(hWnd, NULL, FALSE);
            SetWindowText(hLabel, L""); // Clear the label
            remainingMines = NUM_MINES;
            flaggedMines = 0;
            wchar_t mineCountStr[50];
            swprintf_s(mineCountStr, L"Mines Remaining: %d", NUM_MINES - flaggedMines);
            SetWindowText(hMineLabel, mineCountStr);
            break;
        case 1002: // Exit button
            // Close the application
            PostQuitMessage(0);
            break;
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        DrawGridCells(hdc); // Draw the grid cells
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_LBUTTONDOWN:
        // Handle left mouse button down
        if (!GameOver) {
            int xPos = LOWORD(lParam) / CELL_SIZE;
            int yPos = HIWORD(lParam) / CELL_SIZE;
            int cellId = yPos * GRID_SIZE + xPos;

            if (xPos >= GRID_SIZE || yPos >= GRID_SIZE)
                break;

            if (!isFlagged[cellId] && !isChecked[cellId]) {
                if (!isMine[cellId]) {
                    // Reveal the clicked empty cell and its neighbors
                    isChecked[cellId] = true;
                    HDC hdc = GetDC(hWnd);
                    RevealEmptyCells(yPos, xPos, hdc);
                    ReleaseDC(hWnd, hdc);
                }
                else {
                    YouDied();
                }
            }
            if (CheckWinCondition()) {
                YouWin(); // Check for win condition after each move
            }
        }
        break;

    case WM_RBUTTONDOWN:
        // Handle right mouse button down
        if (!GameOver) {
            int xPos = LOWORD(lParam) / CELL_SIZE;
            int yPos = HIWORD(lParam) / CELL_SIZE;
            int cellId = yPos * GRID_SIZE + xPos;

            if (!isFlagged[cellId] && !isChecked[cellId]) {
                // Mark the cell as a mine
                isFlagged[cellId] = true;
                flaggedMines++;
                wchar_t mineCountStr[50];
                swprintf_s(mineCountStr, L"Mines Remaining: %d", NUM_MINES - flaggedMines);
                SetWindowText(hMineLabel, mineCountStr);
                // Redraw the cell to indicate flagging
                InvalidateRect(hWnd, NULL, FALSE);
            }
            else
            {
                isFlagged[cellId] = false;
                flaggedMines--;
                wchar_t mineCountStr[50];
                swprintf_s(mineCountStr, L"Mines Remaining: %d", NUM_MINES - flaggedMines);
                SetWindowText(hMineLabel, mineCountStr);
                InvalidateRect(hWnd, NULL, FALSE);
            }

        }
        if (CheckWinCondition()) {
            YouWin(); // Check for win condition after each move
        }
        break;
    case WM_DESTROY:
        // Post a quit message to exit the application
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* pKeyInfo = (KBDLLHOOKSTRUCT*)lParam;
        if (pKeyInfo->vkCode == 'H') { // Check for the "H" key
            ToggleUtilityWindowVisibility(); // Toggle the Utility Window's visibility
        }
    }
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

// Utility Window procedure
LRESULT CALLBACK UtilityWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        // Create the "Show Dialog" button
        hDialogButton = CreateWindow(L"BUTTON", L"Show Dialog", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 10, 100, 30, hWnd, (HMENU)2001, NULL, NULL);

        // Create the combo box
        hComboBox = CreateWindow(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            120, 10, 120, 200, hWnd, (HMENU)2002, NULL, NULL);

        // Add difficulty levels to the combo box
        SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)L"Noob");
        SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)L"Easy");
        SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)L"Normal");
        SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)L"Hard");
        SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)L"Pain");

        // Set the default selection
        SendMessage(hComboBox, CB_SETCURSEL, currentDifficulty - 1, 0);
        g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, NULL, 0);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 2001: // Show Dialog button
            MessageBox(hWnd, L"ЛКМ - открыть клетку, ПКМ - поставить флаг, H - открыть меню", L"Game Instructions", MB_OK);
            break;
        case 2002: // Combo box selection
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                // Get the selected difficulty level
                int selected = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
                if (selected != CB_ERR) {
                    // Update the current difficulty
                    currentDifficulty = static_cast<Difficulty>(selected + 1);
                    RestartGame(currentDifficulty); // Restart with the new difficulty
                }
            }
            break;
        }
        break;

    case WM_CLOSE:
        // Hide the Utility Window instead of closing it
        ShowWindow(hUtilityWnd, SW_HIDE);
        break;

    case WM_DESTROY:
        // Post a quit message for the Utility Window
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Define the window class for the main window
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"GridWindowClass", NULL };

    // Register the window class for the main window
    RegisterClassEx(&wcex);

    // Create the application window with a fixed size
    HWND hWnd = CreateWindow(L"GridWindowClass", L"Grid of Buttons", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        return -1;
    }

    // Show and update the window
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Set the global main window handle
    g_hMainWindow = hWnd;

    // Register the window class for the Utility Window
    WNDCLASSEX wcexUtility = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, UtilityWndProc, 0, 0, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"UtilityWindowClass", NULL };
    RegisterClassEx(&wcexUtility);

    // Create the Utility Window
    hUtilityWnd = CreateWindow(L"UtilityWindowClass", L"Utility Window", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 100, NULL, NULL, hInstance, NULL);

    if (!hUtilityWnd) {
        return -1;
    }

    // Show and update the Utility Window
    ShowWindow(hUtilityWnd, SW_HIDE); // Initially hide the Utility Window
    UpdateWindow(hUtilityWnd);

    // ...

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}



