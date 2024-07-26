#pragma once

class BTN
{
  public:
    void Init(int Pin);
    bool CheckButtonPress();
    bool IsDown();

    bool CheckButtonPressed(bool& longPress, unsigned long holdTimeMS = 2000UL);
    
  private:
    int m_iPin;
    int m_iPrevReading;
    int m_iPrevState;
    unsigned long m_iTransitionTimeMS;
};

extern BTN btn;
