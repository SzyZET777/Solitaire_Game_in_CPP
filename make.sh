clear
if ! clang++ -v main.cpp \
  -I/opt/homebrew/opt/ncurses/include \
  -L/opt/homebrew/opt/ncurses/lib \
  -lncursesw \
  -o main; then
  exit -1
fi
strip main
if ! ./main; then
  exit -1
fi