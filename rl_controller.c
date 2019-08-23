/*
Arduino C - Code makes you happy
AV Controller
*/

#include <MeMCore.h>
#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>


/****************************************
**              ROBOT API              **
*****************************************/

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
void play_sound(int frequency, int time_ms) {
    buzzer.tone(frequency, time_ms);
}

/* leds */
MeRGBLed rgbled_7(7, 2);

// can take hexadecimal color codes as parameters
// eg set_leds_color(#ff235a, #ff00ff);
void set_leds_color(int left_r, int left_g, int left_b,
                    int right_r, int right_g, int right_b) 
{ 
    rgbled_7.setColor(2, left_r, left_g, left_b);
    rgbled_7.setColor(1, right_r, right_g, right_b);
    rgbled_7.show(); 
}

void set_leds_color(int r, int g, int b) { set_leds_color(r, g, b, r, g, b); }
void clear_leds() { rgbled_7.setColor(0, 0, 0, 0); rgbled_7.show(); }

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


/****************************************
**            RL CONTROLLER            **
*****************************************/

// hard-coded controller reproducing RL-trained controllers

// time since robot started moving, in seconds
float time_since_start = 0;
// time before RL controller should be activated (before that, robot is human)
float time_before_activation = 30;
// speed multiplier once RL controller is activated
float low_speed_factor = 0.8;
// time in seconds after when RL controller should speed up
float time_before_speedup = 90;
// speed multiplier once speed up is activated
float speedup_factor = 1;

// speed of left and right wheels, between -100% and 100%
float speed_left = 0.0f;
float speed_right = 0.0f;

// time in microseconds since the Arduino board booted up
unsigned long time = 0;

// called when the board is set up
void setup() {
    // init robot
    clear_leds();
    stop();
    play_sound(800, 200);

    // blink until lights are turned off
    do {
        set_leds_color(#8f00ff);
        sleep(1000);
        clear_leds();
        sleep(1000);
    } while(get_light_value() > 300);

    // lights are turned off
    play_sound(500, 400);
    sleep(1000);
    for(int i = 0; i < 5; ++i) {
        set_leds_color(#ffffff);
        sleep(100);
        clear_leds();
        sleep(100);
    }
    sleep(1000);

    // block until lights are turned back on
    // this is done to ensure all robots are synchronized
    do {} while(get_light_value() < 300);

    // lights are on, simulation starts
    play_sound(800, 200);
    set_leds_color(#00ff00);

    time = elapsed_micros();
}


// current direction
// -1: undefined, 0: forward, 1: left, 2: right, 3: backwards
int dir = -1;

// dynamic direction correction
// red_right and red_left are incremented when the robot steps out to the right
// or to the left of the ring track, respectively
// the turning speed is then adapted so that the robot stays within the track
int red_right = 0;
int red_left = 0;

// whether robot is stopped or moving
bool stopped = true;

// headway = distance to vehicle in front, in cm
// the speed of the robot decreases proportionally when the headway is between
// headway_min and headway_max
// the robot does emergency braking if the headway is less than headway_break
float headway_min = 8;
float headway_max = 25;
float headway_break = 12;

// speed multiplier, used when the robots starts moving
float start_speed_mult = 1;
// acceleration of the robot after it starts moving
float start_accel = 0.7;
// time the vehicle should wait before accelerating after it stopped (in ms)
float start_reaction_time = 800;

// speed multiplier for RL controller
float speed_mult_rl = 1.0;

// called at every step
void loop() {
    // elapsed time since the last step, in seconds
    float dt = float(elapsed_micros() - time) / 1e6;
    time = elapsed_micros();
    time_since_start += dt;

    // get speed multiplier from RL controller
    if(time_since_start <= time_before_activation) {
        speed_mult_rl = 1.0;
        set_leds_color(#00ff00);
    }
    else if(time_since_start < time_before_speedup) { 
        speed_mult_rl = low_speed_factor; 
        set_leds_color(#ff0000);
    }
    else { 
        speed_mult_rl = speedup_factor; 
        set_leds_color(#00ffff); 
    }

    // compute new speeds for left and right wheels

    float headway = get_headway();
    float speed_mult = 1.0;

    // in case robot needs to brake
    if(headway <= headway_max) {
        if(headway >= headway_min) {
            // reduce speed depending linearly on headway
            speed_mult = (headway - headway_min) / (headway_max - headway_min);
        }
        if(headway <= headway_break) {
            // emergency breaking
            stopped = true;
            set_leds_color(#ffaa00);
            speed_left = 0;
            speed_right = 0;
        }

    }

    // in case robot doesn't need to do emergency breaking
    if(headway > headway_break) {
        if(stopped) {
            // robot was stopped, start it again after some reaction time
            sleep(start_reaction_time);
            stopped = false;
            start_speed_mult = 0;
        }

        // if vehicle wasn't at full speed, make it accelerate progressively
        if(start_speed_mult < 1) start_speed_mult += start_accel * dt;
        if(start_speed_mult > 1) start_speed_mult = 1;

        // get direction of vehicle depending on where it is on track
        bool left_ok = is_left_black();
        bool right_ok = is_right_black();
        
        if(left_ok && right_ok)
            dir = 0;
        else if(left_ok && !right_ok) { 
            if(dir != 1) red_right++;
            dir = 1; 
        }
        else if(!left_ok && right_ok) {
            if(dir != 2) red_left++;
            dir = 2;
        }
        else 
            dir = 3;

        // compute speed of both wheels depending on direction
        switch(dir) {
            case 0: // all good, go forward (and lean left to follow ring)
                // also apply dynamic correction to follow ring track
                speed_left = 85 - red_right + red_left; 
                speed_right = 100;
                break;
            case 1: // correct trajectory by going left
                speed_left = 60;
                speed_right = 100;
                break;
            case 2: // correct trajectory by going right
                speed_left = 100;
                speed_right = 60;
                break;
            case 3: // lost the track; correct trajectory by going backwards
                speed_left = -50;
                speed_right = -50;
                break;
            default:
                break;
        }

        // clip speed values
        if(speed_left > 100) speed_left = 100;
        if(speed_right > 100) speed_right = 100;
        if(speed_left < -100) speed_left = -100;
        if(speed_right < -100) speed_right = -100;

        // apply speed multipliers, respectively for braking, acceleration
        // after having stopped, and RL controller
        speed_left *= speed_mult * start_speed_mult * speed_mult_rl;
        speed_right *= speed_mult * start_speed_mult * speed_mult_rl;
    }

    if(speed_left < 20 && speed_right < 20)
        set_leds_color(#ffaa00);

    // apply desired speed to motors
    set_speed(speed_left, speed_right);

    _loop();
}