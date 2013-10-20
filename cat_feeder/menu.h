typedef String (*label_t)();

struct Menu
{
  int cItem;
  MenuItem **mItems;
  int n;
  Menu(MenuItem** items, int num) : cItem(0), mItems(items), n(num) { }

  MenuItem* getCurrent() { return mItems[cItem]; }
  void showCurrent() { getCurrent()->printLabel(); }
  void callRight() { getCurrent()->callRight(); }
  void callLeft() { getCurrent()->callLeft(); }
  void callSelect() { getCurrent()->callSelect(); }

  void set(int c) {
    cItem = c;
    showCurrent();
  }
  void inc() {
    cItem += 1;
    if (cItem >= n)
      cItem = 0;
    showCurrent();
  }
  void dec() {
    cItem -= 1;
    if (cItem < 0)
      cItem = n-1;
    showCurrent();
  }
};
