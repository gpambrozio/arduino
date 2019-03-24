#ifndef Mode_h
#define Mode_h

class Mode
{
  public:
    Mode() {};
    virtual void init();
    virtual void setup();
    virtual void tearDown();
    virtual void draw(bool hasClick);
    virtual bool manualAdvance() { return false; }
  protected:
};

#endif
