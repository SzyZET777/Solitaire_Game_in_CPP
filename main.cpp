#define _XOPEN_SOURCE_EXTENDED 1 
// ^^^ Needed to include ncurses with unicode support
#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <random>
#include <vector>


using std::vector;
using std::srand;
using std::shuffle;
using std::begin;
using std::end;


// Enums and global vars - zmienne globalne

enum Colors {NONE, FRONT, BACK, RED, BLACK, TITLE,
             CURSOR_GREEN, CURSOR_WHITE, CURSOR_RED,
             CURSOR_BLACK, CURSOR_HOLDING, BACKGROUND};
enum Background {B_GREEN, B_WHITE, B_RED, B_BLACK};
enum Suits {KIER, KARO, PIK, TREFL};
enum Return_Card_To {NOWHERE, REVEALED, STACK, FIN_STACK};

std::fstream file("ranking.txt", std::ios::in | std::ios::app);
std::random_device rd;
std::default_random_engine rng(rd());

const int starting_undos = 3;
bool colors = true;

int background_color[52][28];
int undos;
bool undo_now = false;
bool undo_in_series = false;
bool important_move = false;
bool menu_running = true;
bool win = false;
int moves_after_win = 0;
int diff_after_win = 1;
int difficulty = 1;
bool saved_to_ranking = true;


// Functions - funkcje globalne

void color_on(int c) {
  if (colors) {
    attron(COLOR_PAIR(c));
  } else if (c == FRONT || c == RED || c == BLACK || c == CURSOR_WHITE) {
    attron(A_REVERSE);
  }
}

void color_off(int c) {
  if (colors) {
    attroff(COLOR_PAIR(c));
  } else if (c == FRONT || c == RED || c == BLACK || c == CURSOR_WHITE) {
    attroff(A_REVERSE);
  }
}

void init_ncurses() {
  // Rozpoczyna ncurses
  setlocale(LC_ALL, "");  // Enable unicode
  initscr();              // Init screem
  cbreak();               // No input buffering
  noecho();               // Don't display what is writen
  curs_set(false);        // Don't show the cursor
  keypad(stdscr, true);   // enable arrow keys

  if (has_colors()) {
    start_color();
    colors = true;
    init_pair(FRONT, COLOR_BLUE, COLOR_WHITE);
    init_pair(BACK, COLOR_BLACK, COLOR_RED);
    init_pair(RED, COLOR_RED, COLOR_WHITE);
    init_pair(BLACK, COLOR_BLACK, COLOR_WHITE);
    init_pair(TITLE, COLOR_WHITE, COLOR_WHITE);
    init_pair(CURSOR_GREEN, COLOR_BLACK, COLOR_GREEN);
    init_pair(CURSOR_WHITE, COLOR_BLACK, COLOR_WHITE);
    init_pair(CURSOR_RED, COLOR_WHITE, COLOR_RED);
    init_pair(CURSOR_BLACK, COLOR_WHITE, COLOR_BLACK);
    init_pair(CURSOR_HOLDING, COLOR_WHITE, COLOR_BLUE);
    init_pair(BACKGROUND, COLOR_BLACK, COLOR_GREEN);
  } else {
    colors = false;
  }
}

void end_ncurses() {
  getch();
  endwin();
}

bool custom_comp(std::string a, std::string b) {
  // Porwnuje 2 pozycje w rankingu
  // W przypadku remisu w ilosci ruchow bierze pod uwage ilosc cofniec
  int sum_a = 0, sum_b = 0, num ;
  num = 1;
  for (int i = a.size()-1; i >= 0; i--) {
    if (a[i] >= '0' && a[i] <= '9') {
      sum_a += (a[i] - '0') * num;
      num *= 10;
    }
  }
  num = 1;
  for (int i = b.size()-1; i >= 0; i--) {
    if (b[i] >= '0' && b[i] <= '9') {
      sum_b += (b[i] - '0') * num;
      num *= 10;
    }
  }
  return sum_a < sum_b;
}




// Classes - klasy

// Klasa Card - karta, przedstawiajaca jedna karte
// Zarzadza pozycja i rysowanie karty
class Card {
  public:

  static constexpr int height = 4, width = 5;
  int pos_x, pos_y;
  int val, suit;
  bool front;

  Card(int val_in=1, int suit_in=KARO, int pos_x_in=0, 
       int pos_y_in=0, int front_in=true) {
    val = val_in;
    suit = suit_in;
    pos_x = pos_x_in;
    pos_y = pos_y_in;
    front = front_in;
  }

  void background_color_on() {
    if (front) color_on((FRONT));
    else color_on((BACK));
  }

  void background_color_off() {
    if (front) color_off((FRONT));
    else color_off((BACK));
  }

  void print_value() {
    if (val == 1) printw("A");
    else if (val == 11) printw("J");
    else if (val == 12) printw("Q");
    else if (val == 13) printw("K");
    else printw("%d",val);
  }

  void draw(int x_off, int y_off) { 
    pos_x += x_off;
    pos_y += y_off;

    background_color_on();
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

          if (front) background_color[pos_x+j][pos_y+i] = B_WHITE;
          else background_color[pos_x+j][pos_y+i] = B_RED;

          if ((i == height-1 || i == 0) && (j == width-1 || j == 0)) {
            move(pos_y+i, pos_x+j);
            printw("+");
          } else if (i == height-1 || i == 0) {
            move(pos_y+i, pos_x+j);
            printw("-");
          } else if (j == width-1 || j == 0) {
            move(pos_y+i, pos_x+j);
            printw("|");
          } else {
            move(pos_y+i, pos_x+j);
            printw(" ");
          }
       }
    }
    background_color_off();

    if (front) {
      if (suit == KIER || suit == KARO) color_on((RED));
      else color_on((BLACK));

      move(pos_y, pos_x);
      print_value();

      if (val != 10) move(pos_y+height-1, pos_x+width-1);
      else move(pos_y+height-1, pos_x+width-2);
      print_value();

      if (suit == TREFL) {
        mvaddwstr(pos_y+1, pos_x, L"♣");
        mvaddwstr(pos_y+height-2, pos_x+width-1, L"♣");
      } else if (suit == PIK) {
        mvaddwstr(pos_y+1, pos_x, L"♠");
        mvaddwstr(pos_y+height-2, pos_x+width-1, L"♠");
      } else if (suit == KARO) {
        mvaddwstr(pos_y+1, pos_x, L"♦");
        mvaddwstr(pos_y+height-2, pos_x+width-1, L"♦");
      } else {
        mvaddwstr(pos_y+1, pos_x, L"♥");
        mvaddwstr(pos_y+height-2, pos_x+width-1, L"♥");
      }

      if (suit == KIER || suit == KARO) color_off((RED));
      else color_off((BLACK));
    }

    pos_x -= x_off;
    pos_y -= y_off;
  }
};



// Klasa Deck - talia
// Pozwala na dobieranie kart, przetasowanie kart i rysowanie stosu kart zakrytych oraz odkrytch
class Deck {
  public:

  static constexpr int pos_x = 2, pos_y = 1;

  vector<Card> cards;
  vector<int> order; // biggest number <- most recetnly used
  vector<Card> revealed_cards;
  vector<bool> removed;

  void init() {
    int num = 0;
    for (int suit = KIER; suit <= TREFL; suit++) {
      for (int val = 1; val <= 13; val++) {
        cards.push_back(Card(val, suit, pos_x, pos_y, false));
        order.push_back(num);
        removed.push_back(false);
        num++;
      }
    }
  }

  bool collides_with_deck(int x, int y) {
    if (x < pos_x || x > pos_x + 4) return false;
    if (y < pos_y || y > pos_y + 3) return false;
    return true;
  }

  bool collides_with_revealed(int x, int y) {
    if (x < pos_x+7 || x > pos_x + 12 + 5) return false;
    if (y < pos_y || y > pos_y + 3) return false;
    return true;
  }

  void reveal_next_card(int num_of_cards = difficulty) {
    int revealed = 0;
    for (int i = 0; i < cards.size(); i++) {
      for (int j = 0; j < cards.size(); j++) {
        if (order[j] == i && !cards[j].front && !removed[j]) {
          cards[j].front = true;
          cards[j].pos_x += 6;
          revealed_cards.push_back(cards[j]);
          revealed++;
          if (revealed == num_of_cards) return;
        }
      }
    }
    if (revealed > 0) return;

    for (int i = 0; i < cards.size(); i++) {
      cards[i].front = false;
      cards[i].pos_x = pos_x;
    }
    shuffle_cards();
    revealed_cards.clear();
  }

  void return_card(Card card) {
      card.pos_x = pos_x + 7;
      card.pos_y = pos_y;
      revealed_cards.push_back(card);
  }

  void shuffle_cards() {
    shuffle(begin(order), end(order), rng);
  }

  void draw() {
    int offset = 0;
    bool not_revealed_found = false;

    for (int i = 0; i < cards.size(); i++) {
      for (int j = 0; j < cards.size(); j++) {
        if (order[j] == i && !cards[j].front && !removed[j]) {
          cards[j].draw(0 ,0);
          not_revealed_found = true;
        }
      }
    }

    if (!not_revealed_found) {
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 5; j++) {
          background_color[pos_x+j][pos_y+i] = B_BLACK;
          if ((i == 4-1 || i == 0) && (j == 5-1 || j == 0)) {
            move(pos_y+i, pos_x+j);
            printw("+");
          } else if (i == 4-1 || i == 0) {
            move(pos_y+i, pos_x+j);
            printw("-");
          } else if (j == 5-1 || j == 0) {
            move(pos_y+i, pos_x+j);
            printw("|");
          } else {
            move(pos_y+i, pos_x+j);
            printw(" ");
          }
        }
      }
    }

    int revealed_cards_drawn = 0;
    int start = revealed_cards.size()-3;
    if (start < 0) start = 0;

    for (int i = start; i < revealed_cards.size(); i++) {
      revealed_cards[i].draw(offset, 0);
      offset += 3;
      if (++revealed_cards_drawn >= 3) break;
    }
  }

};



// Przedstawia jeden z 7 glownych stosow na ktorych rozgrywa sie gra
class Stack {
  public:
  
  int pos_x = 0, pos_y = 0;

  vector<Card> cards;

  Stack(int pos_x_in=0, int pos_y_in=0) {
    pos_x = pos_x_in;
    pos_y = pos_y_in;
  }

  void draw() {
    int cards_drawn = 0;
    for (auto card : cards) {
      card.draw(pos_x, pos_y + cards_drawn);
      cards_drawn++;
    }
  }

  int collides_with_stack(int x, int y) {
    if (x < pos_x || x > pos_x+4) return 0;
    if (y-pos_y >= 0) return y-pos_y+1;
    else return 0;
  }
};



// Jeden z 4 stosow koncowych
class Final_Stack {
  public:
  
  int pos_x = 0, pos_y = 0;

  vector<Card> cards;

  Final_Stack(int pos_x_in=0, int pos_y_in=0) {
    pos_x = pos_x_in;
    pos_y = pos_y_in;
  }

  void draw() {
    if (cards.size() == 0) {
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 5; j++) {
          background_color[pos_x+j][pos_y+i] = B_BLACK;
          if ((i == 4-1 || i == 0) && (j == 5-1 || j == 0)) {
            move(pos_y+i, pos_x+j);
            printw("+");
          } else if (i == 4-1 || i == 0) {
            move(pos_y+i, pos_x+j);
            printw("-");
          } else if (j == 5-1 || j == 0) {
            move(pos_y+i, pos_x+j);
            printw("|");
          } else {
            move(pos_y+i, pos_x+j);
            printw(" ");
          }
        }
      }
    }

    for (auto card : cards) {
      card.draw(pos_x, pos_y);
    }
  }

  int collides_with_final_stack(int x, int y) {
    if (x < pos_x || x > pos_x + 4) return false;
    if (y < pos_y || y > pos_y + 3) return false;
    return true;
  }
};



// Wskaznik, ktorym mozna poruszac uzywajac strzalek
class Cursor {
  public:

  int pos_x = 11, pos_y = 13;
  int return_card_to = NOWHERE;
  int speed = 1;
  Stack stack;

  void update_pos(int input) {
    if (input == KEY_LEFT) {
      pos_x -= speed;
    } else if (input == KEY_RIGHT) {
      pos_x += speed;
    } else if (input == KEY_UP) {
      pos_y -= speed;
    } else if (input == KEY_DOWN) {
      pos_y += speed;
    }

    if (stack.cards.size() == 0) {
      if (pos_x < 0) pos_x = 0;
      else if (pos_x > 51) pos_x = 51;
      if (pos_y < 0) pos_y = 0;
      else if (pos_y > 27) pos_y = 27;
    } else {
      if (pos_x < 2) pos_x = 2;
      else if (pos_x > 49) pos_x = 49;
      if (pos_y < 1) pos_y = 1;
      else if (pos_y > 26 - stack.cards.size()) pos_y = 26 - stack.cards.size();
    }

    stack.pos_x = pos_x;
    stack.pos_y = pos_y;
  }

  void draw() {
    if (stack.cards.size() > 0) {
      stack.draw();
      color_on((CURSOR_HOLDING));
      mvaddwstr(pos_y, pos_x, L"⬉");
      color_off((CURSOR_HOLDING));
    } else {
      if (background_color[pos_x][pos_y] == B_GREEN) color_on((CURSOR_GREEN));
      else if (background_color[pos_x][pos_y] == B_WHITE) color_on((CURSOR_WHITE));
      else if (background_color[pos_x][pos_y] == B_RED) color_on((CURSOR_RED));
      else if (background_color[pos_x][pos_y] == B_BLACK) color_on((CURSOR_BLACK));

      mvaddwstr(pos_y, pos_x, L"⬉");

      if (background_color[pos_x][pos_y] == B_GREEN) color_off((CURSOR_GREEN));
      else if (background_color[pos_x][pos_y] == B_WHITE) color_off((CURSOR_WHITE));
      else if (background_color[pos_x][pos_y] == B_BLACK) color_off((CURSOR_BLACK));

    }
  }
};


// Wspolna funkcja miedzy obiektem Game i Menu
class UpdateAndDrawShared {
  public:

  void background_fill() {
    for (int i = 0; i < 28; i++) {
      for (int j = 0; j < 52; j++) {
        color_on((BACKGROUND));
        mvaddwstr(i, j, L" ");
        background_color[j][i] = B_GREEN;
        color_off((BACKGROUND));
      }
    }
  }
};



// Gra, rysuje i zarzadza cala gra od rozpoczecia do wygranej
class Game : public UpdateAndDrawShared {
  private:

  // All game state should be in those vars/objs

  Cursor cursor;
  Deck deck;
  Stack stacks[7];
  Final_Stack fin_stacks[4];

  int moves = 0;

  int card_in_stack_num = 0;
  int last_used_stack = 0;
  int last_used_fin_stack = 0;
  
  public:

  void init() {
    deck.init();
    deck.shuffle_cards();
    for (int i = 0; i < 7; i++) {
      Stack& stack = stacks[i];
      stack.pos_y = 6;
      stack.pos_x = 8 + 6 * i;
      for (int j = 0; j < i+1; j++) {
        deck.reveal_next_card(1);
        Card c = deck.revealed_cards.back();
        deck.removed[13 * c.suit + c.val - 1] = true;
        stack.cards.push_back(deck.revealed_cards.back());
        stack.cards.back().pos_x = 0;
        stack.cards.back().pos_y = 0;
        if (j != i) stack.cards.back().front = false;
        deck.revealed_cards.pop_back();
      }
    }
    for (int i = 0; i < 4; i++) {
      Final_Stack& fin_stack = fin_stacks[i];
      fin_stack.pos_y = 1;
      fin_stack.pos_x = 26 + 6 * i;
    }
  }

  void show_hidden_card() {
    if (last_used_stack == 0) return; // "0" means the card already got hidden
    if (stacks[last_used_stack-1].cards.empty()) return;
    stacks[last_used_stack-1].cards.back().front = true;
    last_used_stack = 0;
  }

  void drop_cards(bool update_moves = true) {
    if (update_moves) {
      important_move = true;
      moves++;
    }
    cursor.stack.cards.clear();
    cursor.return_card_to = NOWHERE;
    show_hidden_card();
  }

  // Game loop - glowna pętla gry
  void update() {
    int input = getch();
    cursor.update_pos(input);

    if (input == ' ') {

      for (int i = 0; i < 7; i++) {
        Stack& stack = stacks[i];
        card_in_stack_num = stack.collides_with_stack(cursor.pos_x, cursor.pos_y);
        
        if (card_in_stack_num == 0) continue; 
        // "0" means "pointing at a space outside of stacks" 
        
        if (cursor.stack.cards.size() > 0) {
          if (stack.cards.size() > 0) {
            if (cursor.stack.cards[0].val != (stack.cards.back().val - 1)) break;
            
            int color_1 = (cursor.stack.cards[0].suit == KIER || 
                            cursor.stack.cards[0].suit == KARO);
            int color_2 = (stack.cards.back().suit == KIER || 
                            stack.cards.back().suit == KARO);

            if (color_1 == color_2) break;

          } else {
            if (cursor.stack.cards[0].val != 13) break;
          }

          for (auto card : cursor.stack.cards) {
            stack.cards.push_back(card);
            stack.cards.back().pos_x = 0;
            stack.cards.back().pos_y = 0;
          }
          drop_cards();

        } else {
          if (stack.cards.size() == 0) break;

          last_used_stack = i + 1;

          int start = card_in_stack_num - 1;
          int end = stack.cards.size();

          if (start >= end - 1 && start-2 <= end) start = end - 1;
          if (start < 0 || start >= end) break;
          if (stack.cards[start].front == false) break;

          for (int i = start; i < end; i++) {
            cursor.stack.cards.push_back(stack.cards[i]);
            cursor.stack.cards.back().pos_x = -2;
            cursor.stack.cards.back().pos_y = -1;
            cursor.return_card_to = STACK;
          }
          for (int i = start; i < end; i++) {
            stack.cards.pop_back();
          }
        }
        return;
      }

      int full_stacks = 0;
      for (int i = 0; i < 4; i++) {
        Final_Stack& fin_stack = fin_stacks[i];

        if (fin_stack.cards.size() == 13) full_stacks++;
        if (full_stacks == 4) {
          win = true;
          moves_after_win = moves;
          diff_after_win = win;
        }

        if (!fin_stack.collides_with_final_stack(cursor.pos_x, cursor.pos_y)) continue;

        if (cursor.stack.cards.size() == 1) {
          if (fin_stack.cards.size() > 0) {
            if (cursor.stack.cards.back().val != (fin_stack.cards.back().val + 1)) break;
            if (cursor.stack.cards.back().suit != fin_stack.cards.back().suit) break;
          } else {
            if (cursor.stack.cards.back().val != 1) break;
          }
          fin_stack.cards.push_back(cursor.stack.cards.back());
          fin_stack.cards.back().pos_x = 0;
          fin_stack.cards.back().pos_y = 0;
          drop_cards();

        } else {
          if (fin_stack.cards.size() == 0) break;

          last_used_fin_stack = i;
          cursor.stack.cards.push_back(fin_stack.cards.back());
          cursor.stack.cards.back().pos_x = -2;
          cursor.stack.cards.back().pos_y = -1;
          cursor.return_card_to = FIN_STACK;
          fin_stack.cards.pop_back();
        }
        return;
      }


      if (deck.collides_with_deck(cursor.pos_x, cursor.pos_y) && 
          cursor.stack.cards.size() == 0) {
        important_move = true;
        moves++;
        deck.reveal_next_card();
      } else if (deck.collides_with_revealed(cursor.pos_x, cursor.pos_y) && 
                 cursor.stack.cards.size() == 0 && deck.revealed_cards.size() > 0) {
        cursor.stack.cards.push_back(deck.revealed_cards.back());
        cursor.stack.cards.back().pos_x = -2;
        cursor.stack.cards.back().pos_y = -1;
        cursor.return_card_to = REVEALED;
        Card c = cursor.stack.cards.back();
        deck.removed[13 * c.suit + c.val - 1] = true;
        deck.revealed_cards.pop_back();
      } else {
        if (cursor.stack.cards.size() > 0) {
          if (cursor.return_card_to == REVEALED) {
            deck.return_card(cursor.stack.cards.back());
            Card c = cursor.stack.cards.back();
            deck.removed[13 * c.suit + c.val - 1] = false;
            drop_cards(false);
          } else if (cursor.return_card_to == STACK) {
            for (auto card : cursor.stack.cards) {
              stacks[last_used_stack-1].cards.push_back(card);
              stacks[last_used_stack-1].cards.back().pos_x = 0;
              stacks[last_used_stack-1].cards.back().pos_y = 0;
            }
            drop_cards(false);
          } else if (cursor.return_card_to == FIN_STACK) {
            stacks[last_used_stack-1].cards.push_back(cursor.stack.cards.back());
            stacks[last_used_stack-1].cards.back().pos_x = 0;
            stacks[last_used_stack-1].cards.back().pos_y = 0;
          }
        }
      }
    } else if (input == 'z') {
      undo_now = true;
    }
  }

  void draw() {
    background_fill();
    color_on((BACKGROUND));
    move(25, 2);
    printw("Ruchy: %d", moves);
    move(26, 2);
    printw("Cofniecia: %d", undos);
    color_off((BACKGROUND));
    deck.draw();
    for (auto stack : stacks) stack.draw();
    for (auto fin_stack : fin_stacks) fin_stack.draw();
    cursor.draw();
    refresh();
  }
};



// Menu do wybierania ustawien, rozpoczynania gry i ogladania rekordow
class Menu : public UpdateAndDrawShared {
  public:
  
  void update() {
    int input = getch();
    if (input == '1') {
      difficulty = 1;
      menu_running = false;
    } else if (input == '2') {
      difficulty = 3;
      menu_running = false;
    } else if (input == '8') {
      colors = true;
    } else if (input == '9') {
      color_off(BACKGROUND);
      colors = false;
    } else if (input == '0') {
      file.close();
      file.open("ranking.txt", std::ios::out | std::ios::trunc);
      file.close();
    }
  }

  void draw() {
    background_fill();

    color_on((BACKGROUND));
    move(1, 0);
    printw(" Pasjans:");

    move(3, 0);
    printw(" Poprzednia gra:");
    move(4, 0);
    if (win == false) {
      printw("   Nie rozpoczeto jescze zadnej gry");
    } else {
      printw("   Wygrana na poziomie trudnosci: ");
      if (diff_after_win == 3) printw("trudnym");
      else printw("latwym");
      move(5, 0);
      printw("   Wygrana w %d ruchach uzywajac %d cofniec", moves_after_win, 3-undos);
    }

    if (!saved_to_ranking) {
      file.close();
      file.open("ranking.txt", std::ios::app);
      file << "   Wygrana w " << moves_after_win << 
      " ruchach uzywajac " << 3-undos <<  " cofniec" << std::endl;
      file.close();
      saved_to_ranking = true;
    }

    move(7, 0);
    printw(" Nowa gra:");
    move(8, 0);
    printw("   Rozpocznij gre (trudnosc - latwy)  - kliknij 1");
    move(9, 0);
    printw("   Rozpocznij gre (trudnosc - trudny) - kliknij 2");

    move(12, 0);
    printw(" Sterowanie:");
    move(13, 0);
    printw("   Poruszaj sie uzywajac strzalek na klawiaturze");
    move(14, 0);
    printw("   Podnos i upuszczaj karty klikajac spacje");
    move(15, 0);
    printw("   Cofnij ruch klikacjac 'z' na klawiaturze");

    move(16, 0);
    printw(" Ustawienia:");
    move(17, 0);
    printw("   Kliknij 8, aby wlaczyc kolory");
    move(18, 0);
    printw("   Kliknij 9, aby wylaczyc kolory");
    move(19, 0);
    printw("   Kliknij 0, aby zrestowac ranking");

    move(21, 0);
    printw(" Ranking:");

    std::string line;
    file.close();
    file.open("ranking.txt", std::ios::in);

    vector<std::string> vec;

    while (std::getline(file, line)) {
      vec.push_back(line);
    }

    std::sort(vec.begin(), vec.end(), custom_comp);

    for (int i = 0; i < std::min((int)vec.size(), 4); i++) {
      move(22+i, 0);
      printw("%s", vec[i].c_str());
    }

    file.close();
  }
};



void handle_undos(Game game[starting_undos+2]) {
  if (important_move) {
    for (int i = starting_undos+1; i >= 1; i--) {
      game[i] = game[i-1];
    }
    important_move = false;
    undo_in_series = false;
  }
  
  if (undo_now && undos > 0) {
    int x = (undo_in_series) ? 1 : 2;
    for (int i = 0; i+x < starting_undos+2; i++) {
      game[i] = game[i+x];
    }

    undos--;
    undo_now = false;
    undo_in_series = true;

    game[0].draw();
  }
}


// Main
int main() {
    if (!file) {
      std::cerr << "Failed to open file." << std::endl;
      return 1;
    }

    init_ncurses();

    while (true) {
      Menu menu;
      menu.draw();

      menu_running = true;

      while (menu_running) {
        menu.update();
        menu.draw();
      }

      Game game[starting_undos+2];
      win = false;

      game[0].init();
      game[0].draw();

      important_move = false;
      undos = starting_undos;

      for (int i = 1; i < starting_undos+2; i++) {
        game[i] = game[0];
      }
      bool running = true;

      while (running) {
        game[0].update();
        handle_undos(game);
        game[0].draw();
        if (win) {
          saved_to_ranking = false;
          break;
        }
      }
    }

    end_ncurses();
    return 0;
}

