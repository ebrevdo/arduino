struct Buttons
{
  /**
   * buttonNone = 1023;  // 11 1111 1111
   * buttonSelect = 720; // 10 1101 0000
   * buttonLeft = 479;   // 01 1101 1111
   * buttonDown = 306;   // 01 0011 0010
   * buttonUp = 132;     // 00 1000 0100
   * buttonRight = 0;    // 00 0000 0000
   */

  enum bType { NONE, SELECT, LEFT, DOWN, UP, RIGHT };
  static const char* bTypeStrings[6];
  enum bType type(int b) {
    switch (b >> 4) {
    case 0b111111:
      return NONE;
    case 0b101101:
      return SELECT;
    case 0b011101:
      return LEFT;
    case 0b010011:
      return DOWN;
    case 0b001000:
      return UP;
    case 0b000000:
      return RIGHT;
    default:
      return NONE;
    }
  }
  const char* toString(enum bType t) {
    return bTypeStrings[t];
  }
};
const char* Buttons::bTypeStrings[6] = { "None", "Select", "Left", "Down", "Up", "Right" };
