class MenuItem {
 public:
  String label;
  static const unsigned int maxWidth = 16;

  MenuItem(String l) : label(l) {}

  void selectAction() {};

  static void printRow(int row, String s) {
    int start;
    int remainder;
    if (s.length() > maxWidth) {
      start = 0;
      remainder = 0;
    } else {
      start = (maxWidth - s.length()) / 2;
      remainder = maxWidth - start - s.length();
    }
    for (int i = 0; i < start; i++)
      s = ' ' + s;
    for (int i = 0; i < remainder; i++)
      s += ' ';
    lcd.setCursor(0, row);

    lcd.print(s);
  }

  void printLabel() {
    printRow(0, label);
    String d = info();
    if (d == NULL) {
      printRow(1, "Press [Select]");
    } else {
      printRow(1, d);
    }
  }

  void callSelect() {
    select();
    printLabel();
  }
  void callLeft() {
    left();
    printLabel();
  }
  void callRight() {
    right();
    printLabel();
  }

  // Overwrite these
  virtual void left() {}
  virtual void right() {}
  virtual void select() {}
  virtual String info() { return NULL; }
};
