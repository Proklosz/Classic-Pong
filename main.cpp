#include <atomic>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <mutex>
#include <ncurses.h>
#include <thread>
#include <unistd.h>

using namespace std;

class MainFrame {
public:
  MainFrame();

private:
  wchar_t shade[5][3] = {
      {L' ', L' '}, {L'░', L'░'}, {L'▒', L'▒'}, {L'▓', L'▓'}, {L'█', L'█'}};
  wchar_t win[3][3] = {{'W', 'I'}, {'N', 'N'}, {'E', 'R'}};
  wchar_t loose[3][3] = {{'L', 'O'}, {'O', 'S'}, {'E', 'R'}};

  atomic<int> terminal_char_width;
  atomic<int> terminal_char_height;

  atomic<int> ballposX;
  atomic<int> ballposY;

  atomic<int> ballVecX = 0;
  atomic<int> ballVecY = 0;

  atomic<int> left_rod_pos;
  atomic<int> right_rod_pos;

  atomic<int> left_player_score;
  atomic<int> right_player_score;

  mutex LPS_mutex;
  wchar_t LPS[2] = {'0', '0'};

  mutex RPS_mutex;
  wchar_t RPS[2] = {'0', '0'};

  atomic<bool> stop;
  atomic<bool> is_game_odd;

  int getTerminalSize();
  int renderASCIIImage();
  int animate();
  int handleInput();

  void moveLeftRodDown();
  void moveLeftRodUp();
  void moveRightRodDown();
  void moveRightRodUp();

  void startGame();
};

MainFrame::MainFrame() {
  initscr();
  noecho();
  curs_set(0);
  setlocale(LC_ALL, "");

  getTerminalSize();
  stop = false;

  thread input_thread(&MainFrame::handleInput, this);
  thread anim_thread(&MainFrame::animate, this);

  anim_thread.join();
  input_thread.join();

  endwin();
}

int MainFrame::getTerminalSize() {
  getmaxyx(stdscr, terminal_char_height, terminal_char_width);
  if (terminal_char_width % 2 != 0) {
    terminal_char_width -= 1;
  }

  ballposX = static_cast<int>(terminal_char_width / 4 * 2);
  ballposY = static_cast<int>(terminal_char_height / 4 * 2);

  left_rod_pos = right_rod_pos = static_cast<int>(terminal_char_height / 4 * 2);

  left_player_score = right_player_score = 0;

  return 0;
}

int MainFrame::renderASCIIImage() {
  clear();
  for (int y = terminal_char_height; y > -1; y--) {
    for (int x = 0; x < terminal_char_width; x += 2) {
      if (x == ballposX && y == ballposY) {
        mvaddnwstr(y, x, shade[4], 2);
      } else if (x == 4 && y > left_rod_pos - 3 && y < left_rod_pos + 3) {

        mvaddnwstr(y, x, shade[4], 2);
      } else if (x == terminal_char_width - 6 && y > right_rod_pos - 3 &&
                 y < right_rod_pos + 3) {

        mvaddnwstr(y, x, shade[4], 2);
      } else if (x > terminal_char_width / 2 - 2 &&
                     x < terminal_char_width / 2 + 2 ||
                 y == 0 || y == terminal_char_height - 1) {
        mvaddnwstr(y, x, shade[2], 2);
      } else if (y == floor(terminal_char_height / 2) &&
                     x == terminal_char_width / 2 - 6 ||
                 y == floor(terminal_char_height / 2) &&
                     x == terminal_char_width / 2 - 5) {
        LPS_mutex.lock();
        mvaddnwstr(y, x, LPS, 2);
        LPS_mutex.unlock();
      } else if (y == floor(terminal_char_height / 2) &&
                     x == terminal_char_width / 2 - 8 ||
                 y == floor(terminal_char_height / 2) &&
                     x == terminal_char_width / 2 - 7) {
        if (left_player_score == 10) {

          mvaddnwstr(y, x, win[1], 2);
        }
        if (right_player_score == 10) {

          mvaddnwstr(y, x, loose[1], 2);
        }

      } else if (y == floor(terminal_char_height / 2) &&
                     x == terminal_char_width / 2 - 10 ||
                 y == floor(terminal_char_height / 2) &&
                     x == terminal_char_width / 2 - 9) {
        if (left_player_score == 10) {

          mvaddnwstr(y, x, win[0], 2);
        }
        if (right_player_score == 10) {

          mvaddnwstr(y, x, loose[0], 2);
        }

      } else if (y == floor(terminal_char_height / 2) &&
                     x == terminal_char_width / 2 + 6 ||
                 y == floor(terminal_char_height / 2) &&
                     x == terminal_char_width / 2 + 5) {
        RPS_mutex.lock();
        mvaddnwstr(y, x, RPS, 2);
        RPS_mutex.unlock();
      } else if (y == floor(terminal_char_height / 2) &&
                     x == terminal_char_width / 2 + 8 ||
                 y == floor(terminal_char_height / 2) &&
                     x == terminal_char_width / 2 + 7) {
        if (left_player_score == 10) {
          mvaddnwstr(y, x, loose[1], 2);
        }
        if (right_player_score == 10) {
          mvaddnwstr(y, x, win[1], 2);
        }

      } else if (y == floor(terminal_char_height / 2) &&
                     x == terminal_char_width / 2 + 10 ||
                 y == floor(terminal_char_height / 2) &&
                     x == terminal_char_width / 2 + 9) {
        if (left_player_score == 10) {
          mvaddnwstr(y, x, loose[2], 2);
        }
        if (right_player_score == 10) {
          mvaddnwstr(y, x, win[2], 2);
        }

      } else if (x == 0 || x > terminal_char_width - 3) {
        mvaddnwstr(y, x, shade[1], 2);
      } else {
        mvaddnwstr(y, x, shade[0], 2);
      }
    }
  }

  refresh();
  usleep(75000);

  return 0;
}

int MainFrame::animate() {
  while (1) {
    if (ballposX < 3) {
      ballposX = static_cast<int>(terminal_char_width / 4 * 2);
      ballposY = static_cast<int>(terminal_char_height / 4 * 2);

      if (is_game_odd) {
        ballVecX = 2;
        ballVecY = 0;
        is_game_odd = false;
      } else {
        ballVecX = -2;
        ballVecY = 0;
        is_game_odd = true;
      }
      right_player_score++;
    }
    if (ballposX > terminal_char_width - 5) {
      ballposX = static_cast<int>(terminal_char_width / 4 * 2);
      ballposY = static_cast<int>(terminal_char_height / 4 * 2);

      if (is_game_odd) {
        ballVecX = 2;
        ballVecY = 0;
        is_game_odd = false;
      } else {
        ballVecX = -2;
        ballVecY = 0;
        is_game_odd = true;
      }
      left_player_score++;
    }

    if (ballposY < 2 || ballposY > terminal_char_height - 3) {
      ballVecY = -ballVecY;
    }
    ////////////////////////
    if (ballposX < 6 && ballposY == left_rod_pos) {
      ballVecX = -ballVecX;
    }
    if (ballposX < 6 && ballposY > left_rod_pos - 2 &&
        ballposY < left_rod_pos) {
      ballVecX = -ballVecX;
      ballVecY -= 1;
    }
    if (ballposX < 6 && ballposY > left_rod_pos - 3 &&
        ballposY < left_rod_pos - 1) {
      ballVecX = -ballVecX;
      ballVecY -= 1;
    }
    if (ballposX < 6 && ballposY > left_rod_pos &&
        ballposY < left_rod_pos + 3) {
      ballVecX = -ballVecX;
      ballVecY += 1;
    }
    ////////////////////////////////////////
    if (ballposX > terminal_char_width - 7 && ballposY == right_rod_pos) {
      ballVecX = -ballVecX;
    }
    if (ballposX > terminal_char_width - 7 && ballposY > right_rod_pos - 2 &&
        ballposY < right_rod_pos) {
      ballVecX = -ballVecX;
      ballVecY -= 1;
    }
    if (ballposX > terminal_char_width - 7 && ballposY > right_rod_pos - 3 &&
        ballposY < right_rod_pos - 1) {
      ballVecX = -ballVecX;
      ballVecY -= 1;
    }
    if (ballposX > terminal_char_width - 7 && ballposY > right_rod_pos &&
        ballposY < right_rod_pos + 3) {
      ballVecX = -ballVecX;
      ballVecY += 1;
    }

    if (ballVecY > 1) {
      ballVecY = 1;
    }
    if (ballVecY < -1) {
      ballVecY = -1;
    }

    ballposX += ballVecX;
    ballposY += ballVecY;

    if (left_player_score < 10) {
      LPS_mutex.lock();
      LPS[1] = static_cast<char>(left_player_score + '0');
      LPS_mutex.unlock();
    } else if (left_player_score == 10) {
      LPS_mutex.lock();
      LPS[0] = 'E';
      LPS[1] = 'R';
      LPS_mutex.unlock();
      RPS_mutex.lock();
      RPS[0] = 'L';
      RPS[1] = 'O';
      RPS_mutex.unlock();

      ballVecX = 0;
      ballVecY = 0;
      ballposX = static_cast<int>(terminal_char_width / 4 * 2);
      ballposY = static_cast<int>(terminal_char_height / 4 * 2);
    }

    if (right_player_score < 10) {
      RPS_mutex.lock();
      RPS[1] = static_cast<char>(right_player_score + '0');
      RPS_mutex.unlock();
    } else if (right_player_score == 10) {
      RPS_mutex.lock();
      RPS[0] = 'W';
      RPS[1] = 'I';
      RPS_mutex.unlock();
      LPS_mutex.lock();
      LPS[0] = 'E';
      LPS[1] = 'R';
      LPS_mutex.unlock();
      ballVecX = 0;
      ballVecY = 0;
      ballposX = static_cast<int>(terminal_char_width / 4 * 2);
      ballposY = static_cast<int>(terminal_char_height / 4 * 2);
    }

    thread render_thread(&MainFrame::renderASCIIImage, this);
    render_thread.join();
  }
  return 0;
}

void MainFrame::moveLeftRodDown() {
  if (left_rod_pos < terminal_char_height - 4) {
    left_rod_pos += 2;
  }
  refresh();
}

void MainFrame::moveLeftRodUp() {
  if (left_rod_pos > 3) {
    left_rod_pos -= 2;
  }
  refresh();
}

void MainFrame::moveRightRodDown() {
  if (right_rod_pos < terminal_char_height - 4) {
    right_rod_pos += 2;
  }
  refresh();
}

void MainFrame::moveRightRodUp() {
  if (right_rod_pos > 3) {
    right_rod_pos -= 2;
  }
  refresh();
}

void MainFrame::startGame() {
  if (left_player_score == 10 || right_player_score == 10) {
    left_player_score = 0;
    right_player_score = 0;
    LPS[0] = '0';
    RPS[0] = '0';
  }
  ballVecX = -2;
}

int MainFrame::handleInput() {
  while (1) {
    int ch = getch();

    switch (ch) {
    case 'q':

      endwin();
      exit(0);
      break;
    case 'w':
      moveLeftRodUp();
      break;
    case 's':
      moveLeftRodDown();
      break;
    case 'o':
      moveRightRodUp();
      break;
    case 'l':
      moveRightRodDown();
      break;
    case ' ':
      startGame();
      break;
    }
  }

  return 0;
}

int main() {
  MainFrame game;
  return 0;
}
