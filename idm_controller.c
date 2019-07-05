// code makes you happy

#include <MeMCore.h>
#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>

/* light sensor */
MeLightSensor lightsensor_6(6);
float get_light_value() { return lightsensor_6.read(); }

/* line follower sensor */
MeLineFollower linefollower_2(2);

// 0: black|black, 1: black|white, 2: white|black, 3: white|white
int get_line_follower_val() { return int(linefollower_2.readSensors()); }
bool is_left_black() { return get_line_follower_val() <= 1; }
bool is_right_black() { return get_line_follower_val() % 2 == 0; }
bool is_left_white() { return !is_left_black(); }
bool is_right_white() { return !is_right_black(); }

/* motors */
MeDCMotor motor_9(9);
MeDCMotor motor_10(10);

// speeds between -100 and 100
void set_speed(float left_speed, float right_speed) {
    int left = left_speed / 100.0 * 255;
    int right = right_speed / 100.0 * 255;
    motor_9.run((9) == M1 ? -(left) : (left));
    motor_10.run((10) == M1 ? -(right) : (right));
}

// direction: 1 (forward), 2 (backward), 3 (left), 4 (right)
// speed: between -100 and 100
void move(int direction, float speed) {
    switch(direction) {
        case 1: set_speed(speed, speed); break;
        case 2: set_speed(-speed, -speed); break;
        case 3: set_speed(-speed, speed); break;
        case 4: set_speed(speed, -speed); break;
        default: break;
    }
}

void stop() { set_speed(0, 0); }

/* distance sensor */
MeUltrasonicSensor ultrasonic_3(3);
float get_headway() { return ultrasonic_3.distanceCm(); }

/* sound module */
MeBuzzer buzzer;
// this function pauses the executions for time_ms ms
void play_sound(int frequency, int time_ms) { buzzer.tone(frequency, time_ms); }

/* leds */
MeRGBLed rgbled_7(7, 2);

// can take hexadecimal color codes as parameters, eg set_leds_color(#ff235a, #ff00ff);
void set_leds_color(int left_r, int left_g, int left_b, int right_r, int right_g, int right_b) { 
    rgbled_7.setColor(2, left_r, left_g, left_b);
    rgbled_7.setColor(1, right_r, right_g, right_b);
    rgbled_7.show(); 
}

void set_leds_color(int r, int g, int b) { set_leds_color(r, g, b, r, g, b); }
void clear_leds() { set_leds_color(#000, #000); }

// elapsed time since board began to run current program
unsigned long elapsed_millis() { return millis(); } // 50 days overflow
unsigned long elapsed_micros() { return micros(); } // 70 minutes overflow

void _delay(float seconds) {
  long endTime = millis() + seconds * 1000;
  while(millis() < endTime) _loop();
}

void sleep(float ms) {
    _delay(ms / 1000.0f);
}

void _loop() {
}

////////////////////////////////////////////////////////////////////////////////

// between -100% and 100%
float speed_left = 0.0f; 
float speed_right = 0.0f; 

float max_accel = 50.0f; // in %/s/s (not used for braking which is instant)

unsigned long time = 0;

// called when the board is set up
void setup() {
    clear_leds();
    stop();

    // init bots
    play_sound(800, 200);
    set_leds_color(#faf);

    do {} while(get_light_value() > 300);
    play_sound(500, 400);

    sleep(700);
    for(int i = 0; i < 3; ++i) {
        set_leds_color(#fff);
        sleep(100);
        clear_leds();
    }
    sleep(1300);

    do {} while(get_light_value() < 300);
    play_sound(800, 200);

    set_led_colors(#0f0);

    time = elapsed_micros();
}

// called at every step
void loop() {
    float dt = float(elapsed_micros() - time) / 1e6; // s
    time = elapsed_micros();

    // compute new speeds

    bool left_ok = is_left_black();
    bool right_ok = is_right_black();

    int dir = -1;

    if(left_ok && right_ok) { dir = 0; set_led_colors(#0f0); }
    else if(left_ok && !right_ok) { dir = 1; set_led_colors(#0f0, #f00); }
    else if(!left_ok && right_ok) { dir = 2; set_led_colors(#f00, #0f0); }
    else { dir = 3; set_led_colors(#f00); }

    switch(dir) {
        case 0: // all good, go forward (and lean left because ring)

            break;
        case 1: // correct by going left

            break;
        case 2: // correct by going right

            break;
        case 3: // correct by going backwards

            break;
        default:
            break;
    }

    // adapt speeds given headway

    if(dir != 3) {
        float headway = get_headway();

        
    }

    // update speeds

    set_speed(speed_left, speed_right);

    _loop();
}