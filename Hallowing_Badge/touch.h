#include <Adafruit_FreeTouch.h>

#define AVERAGE_COUNT 100

//enum TouchState {
//  
//};

class Touch
{
  public:
    Touch(int pin) {
      ft = Adafruit_FreeTouch(pin, OVERSAMPLE_4, RESISTOR_50K, FREQ_MODE_NONE);
    }
    bool begin() {
      return ft.begin();
    }
    void process() {
      if (hasAverage) {
        uint16_t measure = ft.measure();
        bool isTouching = (measure >= stableAverage);
        
      } else {
        stableAverage += ft.measure();
        if (++averageCount >= AVERAGE_COUNT) {
          stableAverage /= averageCount;
          stableAverage *= 1.2;
          hasAverage = true;
        }
      }
    }
  protected:
    Adafruit_FreeTouch ft;
    uint32_t stableAverage = 0;
    uint8_t averageCount = 0;
    bool hasAverage = false;
    };
