#include <iostream>
#include <vector>
#include <conio.h>
#include <windows.h>
#include <thread>
#include <array>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace std;
int scorelec = 0;
int levellec = 0;

// �e�g���~�m�̒�`
const array<string, 7> tetromino = {
    "..X...X...X...X.",
    "..X..XX...X.....",
    ".....XX..XX.....",
    "..X..XX..X......",
    ".X...XX...X.....",
    ".X...X...XX.....",
    "..X...X..XX....."
};

// �t�B�[���h�̃T�C�Y
const int fieldWidth = 12;
const int fieldHeight = 18;

class Field {
public:
    Field(int width, int height)
        : width(width), height(height), field(width* height, ' ') {
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                field[y * width + x] = (x == 0 || x == width - 1 || y == height - 1) ? '#' : ' ';
            }
        }
    }

    void Draw(HANDLE hConsole, int cursorX, int cursorY) const {
        COORD coord;
        coord.X = cursorX;
        coord.Y = cursorY;
        SetConsoleCursorPosition(hConsole, coord);
        for (int y = 0; y < height; y++) {

            for (int x = 0; x < width; x++) {
                cout << field[y * width + x];
            }
            cout << endl;
        }
    }

    char& At(int x, int y) {
        return field[y * width + x];
    }

    const char& At(int x, int y) const {
        return field[y * width + x];
    }

    void ClearLine(int y) {
        for (int py = y; py > 0; py--) {
            for (int px = 1; px < width - 1; px++) {
                At(px, py) = At(px, py - 1);
            }
        }
        for (int px = 1; px < width - 1; px++) {
            At(px, 0) = ' ';
        }
    }

    int ClearLines() {
        int linesCleared = 0;
        for (int y = 0; y < height - 1; y++) {
            if (all_of(field.begin() + y * width + 1, field.begin() + (y + 1) * width - 1, [](char c) { return c != ' '; })) {
                ClearLine(y);
                linesCleared++;
            }
        }
        if (linesCleared > 0) {
            thread([]() { mciSendString(L"play line_clear.wav", NULL, 0, NULL); }).detach(); // �s�N���A��
        }
        return linesCleared;
    }

private:
    int width;
    int height;
    vector<char> field;
};

class Tetromino {
public:
    Tetromino(int type, int rotation, int posX, int posY)
        : type(type), rotation(rotation), posX(posX), posY(posY) {}

    int Rotate(int px, int py, int rotation) const {
        switch (rotation % 4) {
        case 0: return py * 4 + px;         // 0�x
        case 1: return 12 + py - (px * 4);  // 90�x
        case 2: return 15 - (py * 4) - px;  // 180�x
        case 3: return 3 - py + (px * 4);   // 270�x
        }
        return 0;
    }

    bool DoesFit(const Field& field, int newX, int newY, int newRotation) const {
        for (int px = 0; px < 4; px++) {
            for (int py = 0; py < 4; py++) {
                int pi = Rotate(px, py, newRotation);
                int fi = (newY + py) * fieldWidth + (newX + px);
                if (newX + px >= 0 && newX + px < fieldWidth && newY + py >= 0 && newY + py < fieldHeight) {
                    if (tetromino[type][pi] == 'X' && field.At(newX + px, newY + py) != ' ')
                        return false;
                }
                else {
                    return false;
                }
            }
        }
        return true;
    }

    bool DoesFit(const Field& field) const {
        return DoesFit(field, posX, posY, rotation);
    }

    void PlaceOnField(Field& field) const {
        for (int px = 0; px < 4; px++) {
            for (int py = 0; py < 4; py++) {
                if (tetromino[type][Rotate(px, py, rotation)] == 'X') {
                    field.At(posX + px, posY + py) = 'X';
                }
            }
        }
    }

    void Draw(Field& field, HANDLE hConsole, int cursorX, int cursorY) const {
        for (int px = 0; px < 4; px++) {
            for (int py = 0; py < 4; py++) {
                if (tetromino[type][Rotate(px, py, rotation)] == 'X') {
                    COORD coord;
                    coord.X = cursorX + posX + px;
                    coord.Y = cursorY + posY + py;
                    SetConsoleCursorPosition(hConsole, coord);
                    cout << 'O';
                }
            }
        }
    }

    void Move(int dx, int dy) {
        posX += dx;
        posY += dy;
    }

    void RotateClockwise(const Field& field) {
        int newRotation = (rotation + 1) % 4;
        if (DoesFit(field, posX, posY, newRotation)) {
            rotation = newRotation;
        }
    }

    int GetX() const { return posX; }
    int GetY() const { return posY; }
    int GetRotation() const { return rotation; }
    int GetType() const { return type; }

private:
    int type;
    int rotation;
    int posX;
    int posY;
};

void PlayBGM() {
    mciSendString(L"open \"bgm.wav\" type mpegvideo alias bgm", NULL, 0, NULL);
    mciSendString(L"play bgm repeat", NULL, 0, NULL);
}

void StopBGM() {
    mciSendString(L"stop bgm", NULL, 0, NULL);
    mciSendString(L"close bgm", NULL, 0, NULL);
}

void ClearConsole() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count, cellCount;
    COORD homeCoords = { 0, 0 };

    if (hConsole == INVALID_HANDLE_VALUE) return;

    // ���݂̃o�b�t�@�̃T�C�Y���擾
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
    cellCount = csbi.dwSize.X * csbi.dwSize.Y;

    // ��ʂ��󔒂Ŗ��߂�
    if (!FillConsoleOutputCharacter(hConsole, (TCHAR)' ', cellCount, homeCoords, &count)) return;

    // ���݂̑������g���ĉ�ʂ𖄂߂�
    if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count)) return;

    // �J�[�\��������Ɉړ�
    SetConsoleCursorPosition(hConsole, homeCoords);
}

void DrawNextTetromino(HANDLE hConsole, int nextPiece, int cursorX, int cursorY) {
    COORD coord;
    coord.X = cursorX;
    coord.Y = cursorY;

    SetConsoleCursorPosition(hConsole, coord);
    cout << "Next:" << endl;

    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            coord.X = cursorX + px;
            coord.Y = cursorY + 1 + py;
            SetConsoleCursorPosition(hConsole, coord);
            if (tetromino[nextPiece][py * 4 + px] == 'X') {
                cout << 'O';
            }
            else {
                cout << ' ';
            }
        }
        cout << endl;
    }
}

bool PlayGame(HANDLE hConsole) {
    Field field(fieldWidth, fieldHeight);
    int currentPiece = rand() % 7;
    int nextPiece = rand() % 7; // ���̃e�g���~�m���Ǘ�
    Tetromino tetromino(currentPiece, 0, fieldWidth / 2 - 2, 0); // �����ʒu���C��
    int score = 0;
    int speed = 20;
    int speedCount = 0;
    int level = 0;
    bool gameOver = false;

    int cursorX = 0, cursorY = 0; // �R���\�[���J�[�\���̏����ʒu

    PlayBGM(); // �Q�[���J�n�Ɠ�����BGM���Đ�

    while (!gameOver) {
        this_thread::sleep_for(chrono::milliseconds(50));
        speedCount++;
        bool forceDown = (speedCount == speed);

        if (_kbhit()) {
            switch (_getch()) {
            case 'a':
                if (tetromino.DoesFit(field, tetromino.GetX() - 1, tetromino.GetY(), tetromino.GetRotation())) {
                    tetromino.Move(-1, 0);
                }
                break;
            case 'd':
                if (tetromino.DoesFit(field, tetromino.GetX() + 1, tetromino.GetY(), tetromino.GetRotation())) {
                    tetromino.Move(1, 0);
                }
                break;
            case 's':
                if (tetromino.DoesFit(field, tetromino.GetX(), tetromino.GetY() + 1, tetromino.GetRotation())) {
                    tetromino.Move(0, 1);
                }
                break;
            case 'w':
                tetromino.RotateClockwise(field);
                break;
            }
        }

        if (forceDown) {
            if (tetromino.DoesFit(field, tetromino.GetX(), tetromino.GetY() + 1, tetromino.GetRotation())) {
                tetromino.Move(0, 1);
            }
            else {
                tetromino.PlaceOnField(field);
                int lines = field.ClearLines();
                if (lines > 0) score += (1 << lines) * 100;
                currentPiece = nextPiece;
                nextPiece = rand() % 7; // ���̃e�g���~�m���X�V
                tetromino = Tetromino(currentPiece, 0, fieldWidth / 2 - 2, 0);
                gameOver = !tetromino.DoesFit(field);
                if (gameOver) {
                    mciSendString(L"play game_over.wav", NULL, 0, NULL); // �Q�[���I�[�o�[�̉�
                }

                // �X�R�A�ɉ����đ��x���グ��
                if (score / 1000 > level) {
                    level++;
                    speed = max(10, speed - 1); // �ŏ����x��10�ɐݒ�
                }
            }
            speedCount = 0;
        }

        // ��ʂ̈ꕔ�̂ݍX�V����
        field.Draw(hConsole, cursorX, cursorY);
        tetromino.Draw(field, hConsole, cursorX, cursorY);

        // ���̃e�g���~�m���E���ɕ\��
        DrawNextTetromino(hConsole, nextPiece, cursorX + fieldWidth + 2, cursorY);

        COORD coord;
        coord.X = cursorX;
        coord.Y = cursorY + fieldHeight;
        SetConsoleCursorPosition(hConsole, coord);
        cout << "Score: " << score << endl;
        cout << "Level: " << level << endl;
    }

    this_thread::sleep_for(chrono::seconds(2));

    StopBGM(); // �Q�[���I�[�o�[�Ɠ�����BGM���~
    scorelec = score;
    levellec = level;

    COORD coord;
    coord.X = cursorX;
    coord.Y = cursorY + fieldHeight + 1;
    SetConsoleCursorPosition(hConsole, coord);
    cout << "Game Over!" << endl;
    return false;
}

int main() {
    srand(static_cast<unsigned int>(time(0)));
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false; // �J�[�\�����\���ɂ���
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    while (true) {
        // �Q�[���J�n�O�Ɋm�F����
        COORD coord;
        coord.X = 0;
        coord.Y = 0;
        SetConsoleCursorPosition(hConsole, coord);
        cout << "Are you ready?(y/n): ";
        char ready;
        cin >> ready;
        if (ready != 'y' && ready != 'Y') {
            break;
        }

        ClearConsole(); // ��ʂ��N���A

        bool playAgain = PlayGame(hConsole);

        // �Q�[���I�[�o�[��ɍēx�m�F����O�ɉ�ʂ��N���A
        ClearConsole();

        coord.X = 0;
        coord.Y = 0;
        SetConsoleCursorPosition(hConsole, coord);
        cout << "score:" << scorelec << endl;
        cout << "level:" << levellec << endl;
        cout << "Do you want to play again? (y/n): ";
        char choice;
        cin >> choice;
        if (choice != 'y' && choice != 'Y') {
            break;
        }

        ClearConsole(); // ��ʂ��N���A
    }

    cout << "Thank you for playing!" << endl;
    return 0;
}
