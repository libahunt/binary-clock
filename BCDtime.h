class BCDTime {
  
  public:
  
    void setTime(int newH, int newM, int newS) {
      h = newH;
      m = newM;
      s = newS;
    }
    
    int getBCDcomponent(int part) {
      switch (part) {
        case 0:
          return (int)h/10;
        case 1:
          return (int)h%10;
        case 2:
          return (int)m/10;
        case 3:
          return (int)m%10;
        case 4:
          return (int)s/10;
        case 5:
          return (int)s%10;
      }
    }

    void setBCD(int h10, int h1, int m10, int m1, int s10, int s1) {
      h = h10*10 + h1;
      m = m10*10 + m1;
      s = s10*10 + s1;
    }

    void tick() {
      s++;
      if (s>59) {
        m++;
        s=0;
        if (m>59) {
          h++;
          m=0;
          if(h>23) {
            h=0;
          }
        }
      }
    }

    void incrementMin() {
      m++;
      if (m==60) {
        m=0;
      }
    }

    void incrementHour() {
      h++;
      if (h==24) {
        h=0;
      }
    }
    void setSeconds(int n) {
      s = n;
    }

    int h;
    int m;
    int s;
};
