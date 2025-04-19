#include <LedControl.h>

LedControl lc = LedControl(12, 11, 10, 4); // DIN, CLK, CS, devices

const int VRx = A0;
const int VRy = A1;
const int SW = 2;

unsigned long lastMove = 0;
unsigned long fallSpeed = 300;
unsigned long lastInput = 0;
bool buttonHeld = false;

uint16_t board[16]; // 16x16 board, each row is 16 bits wide

const byte pieces[7][4][2] = {
  // I
  { {0,3}, {1,3}, {2,3}, {3,3} },
  // O
  { {0,3}, {0,4}, {1,3}, {1,4} },
  // T
  { {0,3}, {1,2}, {1,3}, {1,4} },
  // L
  { {0,3}, {1,3}, {2,3}, {2,4} },
  // J
  { {0,4}, {1,4}, {2,4}, {2,3} },
  // S
  { {1,2}, {1,3}, {0,3}, {0,4} },
  // Z
  { {0,2}, {0,3}, {1,3}, {1,4} }
};

byte currentPiece[4][2];
int type = 0;

void setup() {
  for (int i = 0; i < 4; i++) {
    lc.shutdown(i, false);
    lc.setIntensity(i, 8);
    lc.clearDisplay(i);
  }

  pinMode(SW, INPUT_PULLUP);
  randomSeed(analogRead(A5));
  spawnPiece();
}

void loop() {
  unsigned long now = millis();
  if (now - lastMove > fallSpeed) {
    lastMove = now;
    if (!movePiece(1)) {
      lockPiece();
      clearLines();
      spawnPiece();
    }
  }

  handleInput();
  drawBoard();
  delay(30);
}

void drawBoard() {
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      bool filled = (board[y] & (1 << x)) != 0;
      setPixel16x16(x, y, filled);
    }
  }

  for (int i = 0; i < 4; i++) {
    int row = currentPiece[i][0];
    int col = currentPiece[i][1];
    if (row >= 0 && row < 16 && col >= 0 && col < 16)
      setPixel16x16(col, row, true);
  }
}

void setPixel16x16(int x, int y, bool state) {
  int module = -1;
  int row = y % 8;
  int col = x % 8;

  if (x < 8 && y >= 8) module = 0;       // M0 (bottom-left)
  else if (x >= 8 && y >= 8) module = 1; // M1 (bottom-right)
  else if (x < 8 && y < 8) module = 2;   // M2 (top-left)
  else if (x >= 8 && y < 8) module = 3;  // M3 (top-right)

  if (module == 1 || module == 3) {
    row = 7 - row;
    col = 7 - col;
  }

  lc.setLed(module, row, col, state);
}

bool movePiece(int dy) {
  for (int i = 0; i < 4; i++) {
    int ny = currentPiece[i][0] + dy;
    int nx = currentPiece[i][1];
    if (ny >= 16 || (board[ny] & (1 << nx))) return false;
  }
  for (int i = 0; i < 4; i++) currentPiece[i][0] += dy;
  return true;
}

void moveHorizontal(int dx) {
  for (int i = 0; i < 4; i++) {
    int nx = currentPiece[i][1] + dx;
    int ny = currentPiece[i][0];
    if (nx < 0 || nx >= 16 || (board[ny] & (1 << nx))) return;
  }
  for (int i = 0; i < 4; i++) currentPiece[i][1] += dx;
}

void lockPiece() {
  for (int i = 0; i < 4; i++) {
    int y = currentPiece[i][0];
    int x = currentPiece[i][1];
    if (y >= 0 && y < 16 && x >= 0 && x < 16)
      board[y] |= (1 << x);
  }
}

void clearLines() {
  for (int y = 15; y >= 0; y--) {
    if (board[y] == 0xFFFF) {
      for (int j = y; j > 0; j--) board[j] = board[j - 1];
      board[0] = 0;
      y++;
    }
  }
}

void spawnPiece() {
  type = random(0, 7);
  for (int i = 0; i < 4; i++) {
    currentPiece[i][0] = pieces[type][i][0];
    currentPiece[i][1] = pieces[type][i][1] + 6; // center spawn
  }
}

void handleInput() {
  int xVal = analogRead(VRx);
  static unsigned long lastJoy = 0;

  if (millis() - lastJoy > 150) {
    if (xVal < 300) {
      moveHorizontal(-1);
      lastJoy = millis();
    } else if (xVal > 700) {
      moveHorizontal(1);
      lastJoy = millis();
    }
  }

  bool pressed = digitalRead(SW) == LOW;
  if (pressed && !buttonHeld) {
    buttonHeld = true;
    unsigned long t = millis();
    delay(25); // debounce

    // Check if it's a tap or hold
    while (digitalRead(SW) == LOW) {
      if (millis() - t > 400) {
        // Held: hard drop
        while (movePiece(1));
        return;
      }
    }

    rotatePiece(); // Tap: rotate
  }

  if (!pressed) buttonHeld = false;
}

void rotatePiece() {
  int cx = currentPiece[1][1];
  int cy = currentPiece[1][0];
  byte rotated[4][2];

  for (int i = 0; i < 4; i++) {
    int x = currentPiece[i][1] - cx;
    int y = currentPiece[i][0] - cy;
    int rx = -y + cx;
    int ry = x + cy;

    if (rx < 0 || rx >= 16 || ry < 0 || ry >= 16 || (board[ry] & (1 << rx)))
      return;

    rotated[i][0] = ry;
    rotated[i][1] = rx;
  }

  for (int i = 0; i < 4; i++) {
    currentPiece[i][0] = rotated[i][0];
    currentPiece[i][1] = rotated[i][1];
  }
}
