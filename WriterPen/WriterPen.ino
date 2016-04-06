/* 
*/

#define SERVO_LEFT    9
#define SERVO_RIGHT   10
#define SERVO_RAISE   8

#include <Servo.h>

Servo servo_left;
Servo servo_right;

Servo servo_raise;
int last_x, last_y;

int serial_putc( char c, FILE * )  {
  Serial.write( c );
  return c;
}

void printf_begin(void) {
  fdevopen( &serial_putc, 0 );
}

void setup()
{
  // initialize serial:
  Serial.begin(9600);
  printf_begin();

  servo_left.attach(SERVO_LEFT);
  servo_right.attach(SERVO_RIGHT);

  up(true);
  move_to(0, 105);
  up(false);
}

void loop() 
{ 
  while (Serial.available() > 0) {
    int x = Serial.parseInt();
    int y = Serial.parseInt();
    
    swift_move_to(x, y);    
  } 
}

void up(bool move_up) {
  servo_raise.attach(SERVO_RAISE);
  if (move_up) {
    servo_raise.write(75);
    delay(800);
  } else {
    servo_raise.write(90);
    delay(200);
  }
  servo_raise.detach();
}

void move_to(int x, int y) {
  double l, r;
  int rounded_l, rounded_r;
  if (servo_coordinates(x, y, &l, &r)) {
    l = 180.0 - l * 90.0;
    r = r * 90.0;
    rounded_l = round(l);
    rounded_r = round(r);
    printf("X = %d, Y = %d, l = %d, r = %d\n", x, y, (int)l, (int)r);
    servo_left.write(rounded_l);
    servo_right.write(rounded_r);
    last_x = x;
    last_y = y;
  } else {
    printf("X = %d, Y = %d is invalid\n", x, y);
  }
}

void swift_move_to(int x, int y) {
  int delta_x = x - last_x;
  int delta_y = y - last_y;
  int distance = round(sqrt(delta_x * delta_x + delta_y * delta_y));
  double step_x = (double)delta_x / (double)distance;
  double step_y = (double)delta_y / (double)distance;
  double current_x = last_x;
  double current_y = last_y;
  for (int i = 0; i < distance; i++) {
    move_to(round(current_x), round(current_y));
    delay(5);
    current_x += step_x;
    current_y += step_y;
  }
  move_to(x, y);
}

// Adapted from https://gist.github.com/m-ou-se/b9d215db145cfe018021

#define tau  (6.28318530717959)

// Gives the length of one side of a triangle given the other two and the angle between those.
// Just needed to calculate CX bellow so I pre-ran and got the value bellow.
/*
double cosine_rule(double side_1, double side_2, double opposite_angle) {
  return sqrt(side_1*side_1 + side_2*side_2 - 2*side_1*side_2*cos(opposite_angle));
}
*/

// Note: Gives only one intersection: The one left of A->B.
bool circles_intersection(
    const double a_x, const double a_y, const double a_r,
    const double b_x, const double b_y, const double b_r,
    double *i_x, double *i_y
) {
    const double d_x = b_x - a_x, d_y = b_y - a_y;
    const double d = sqrt(d_x*d_x + d_y*d_y);
    const double x = (d - b_r*b_r/d + a_r*a_r/d) / 2;
    const double s = a_r*a_r - x*x;
    if (s < 0) {
        return false;
    }
    const double y = sqrt(s);
    *i_x = (x*d_x - y*d_y) / d + a_x;
    *i_y = (x*d_y + y*d_x) / d + a_y;
    return true;
}

#define AB  44
#define AC  50
#define BD  50
#define CE  50
#define DE  50
#define EX  11
#define XEC (3.0 / 8.0 * tau)

#define A_x  (-AB/2)
#define A_y  0
#define B_x  (+AB/2)
#define B_y  0

#define CX   (58.299378)    // = cosine_rule(CE, EX, XEC);

bool servo_coordinates(
    double const X_x, double const X_y,
    double *left_servo, double *right_servo
) {
    // A: Left servo axis
    // B: Right servo axis
    // C: Left joint
    // D: Right joint
    // E: Pen joint
    // X: Pen
    // O: Origin
    //  ________________
    // |     X          |
    // |      )E        |
    // |     /   \      |
    // |   /       \    |  ________________   _______
    // |  C         D   | |      1  1      | |       |
    // |   \       /    | |      |  |      | | y     |
    // |    \     /     | |      |  |      | | |     |
    // |     A O B      | | 0----A  B----0 | | O---x |
    // |________________| |________________| |_______|
    //  Point names        Servo values       Axis

    if (X_y < 60) return false;

    double C_x, C_y;
    if (!circles_intersection(A_x, A_y, AC, X_x, X_y, CX, &C_x, &C_y)) return false;

    double E_x, E_y;
    circles_intersection(X_x, X_y, EX, C_x, C_y, CE, &E_x, &E_y);

    double D_x, D_y;
    if (!circles_intersection(E_x, E_y, DE, B_x, B_y, BD, &D_x, &D_y)) return false;

     *left_servo = atan2(C_y - A_y, A_x - C_x) / (tau / 4);
    *right_servo = atan2(D_y - B_y, D_x - B_x) / (tau / 4);
    return true;
}

