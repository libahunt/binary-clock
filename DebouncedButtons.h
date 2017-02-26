class DebouncedButton {
  public:
    DebouncedButton(int pinNumber, unsigned long debounceDelay, bool pullup) {
      pin = pinNumber;
      dbDelay = debounceDelay;
      lastDbState = pullup;
      value = lastDbState;
      lastDbTime = millis();
      pullup = pullup;
      lastSeen = 0;
    };
    boolean dbReadPushed() {
      boolean dbState = digitalRead(pin);
      if (dbState != lastDbState) {// if reading changes
        lastDbTime = millis();     //      we reset timer
      }
      lastDbState = dbState;       //save current reading for next loop
      if ((millis() - lastDbTime) > dbDelay) {  //if reading has been constant for long enough
        value = dbState;                        //    we assign reading as new value
      }
      if (pullup) {//with pullup pressed state is LOW state
        return !value;
      }
      else {//with pulldown pressed state is HIGH state
        return value;
      }
    };

    boolean dbReadPushStarted() {
      bool buttonStatus = this -> dbReadPushed();
      bool pushStarted = false;
      if (buttonStatus == 1 && lastSeen == 0) {
        pushStarted = true; //a debounced push has just started
      }
      lastSeen = buttonStatus;
      return pushStarted;
    }

  private:
    int pin;
    unsigned long dbDelay;
    boolean lastDbState;
    unsigned long lastDbTime;
    boolean value;
    bool pullup;
    bool lastSeen;
};

