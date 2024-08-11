/*
gauge moves ~56 mm
1% = 0.54 mm

Barrel:
1 mm = 16 in^3 = 0.071 gallons
14.08 mm is 1 gallon
float moves 673 mm
1 mm in gauge = 12.46 mm in barrel
1 mm in barrel = 0.08 mm in gauge

best flow is 1 gallon / min

TODO periodic pump turn on to prime pump
*/

#include <Wire.h>
#include <LinkedList.h>
#include "Adafruit_VL6180X.h"

#define MINUTES = 60
#define HOURS = 3600
#define DEFAULT_MIN 16 // distance from sensor to gauge when full
#define DEFAULT_MAX 74 // distance from sensor to gauge when empty
#define PUMP_RELAY 2
#define TICK_TIME 5000000 // seconds in micro seconds
#define NUM_RANGES 24 // number of range data points to collect each TIME_INTERVAL
#define TIME_INTERVAL 24 // in seconds

unsigned long present_time, past_time, time_delta;
unsigned int counter;
unsigned int seconds;
int wait_time;
int time_counter;
float sum_ranges;
float avg_range;
float avg_range_delta;
float percent_full;
bool pump_on;

LinkedList<unsigned int> ranges;

// infrared distance sensor
Adafruit_VL6180X vl = Adafruit_VL6180X();

void setup() {
    Serial.begin(115200);
    Serial.println("VL6180 Start. . .");
    // wait for serial port to open on native usb devices
    while (!Serial) {
        delay(1);
    }

    if (! vl.begin()) {
        Serial.println("Failed to find sensor");
        while (1);
    }
    Serial.println("Sensor found!");

    counter = 0;
    wait_time = 0;
    time_counter = 0;
    sum_ranges = 0;
    // difference between current range and the previous one
    avg_range_delta = 0;
    pump_on = false;

    seconds = 0;
    past_time = micros();
    time_delta = 0;
}

void loop() {
    // float lux = vl.readLux(VL6180X_ALS_GAIN_5);
    // Serial.print("Lux: "); Serial.println(lux);
    
    uint8_t range = 
    .readRange();
    uint8_t status = vl.readRangeStatus();
    present_time = micros();

    // Serial.print("Range: "); Serial.println(range);

    // clock tick tock every second
    time_delta = (present_time - past_time);
    if (time_delta > TICK_TIME) {
        past_time = present_time - (time_delta - TICK_TIME) - 1275;
        seconds++;
        // Serial.println(seconds);

        // average distance inputs and calculate percent full
        if (status == VL6180X_ERROR_NONE) {
            avg_range = float(sum_ranges/counter);
            percent_full = (1.0 - (avg_range-DEFAULT_MIN) / (DEFAULT_MAX-DEFAULT_MIN)) * 100;
            seconds = 0;

            
            if (ranges.size() == NUM_RANGES) {
                avg_range_delta = avg_range - ranges.get(ranges.size() - 1);
                ranges.pop();
            }
            // add average range to the beginning of LinkedList
            ranges.unshift(avg_range);

            Serial.print("Average Range: "); Serial.println(avg_range);
            Serial.print("Range Delta: "); Serial.println(avg_range_delta);
            Serial.print("Percent Full: "); Serial.print(percent_full); Serial.println("%");
            Serial.print("Num Samples: "); Serial.println(counter);
            Serial.print("Wait Time: "); Serial.println(wait_time);
            Serial.print("Pump On: "); Serial.println(pump_on);
        }

        counter = 0;
        sum_ranges = 0;
        if (wait_time > 0) {
            wait_time--;
        }
        if (pump_on) {
            time_counter++;
        }
        
    } else {
        sum_ranges += range;
        counter++;
    }

    // turn pump on and off
    if(pump_on) {
        if (percent_full > 98) {
            digitalWrite(PUMP_RELAY, LOW);
            pump_on = false;

            // if the water level isn't changing much or if the time is up
        } else if (avg_range_delta < 2 || time_counter >= TIME_INTERVAL){
            digitalWrite(PUMP_RELAY, LOW);
            pump_on = false;
            wait_time = 5 * MINUTES; // wait 5 minutes
        }
    } else { //pump off
        if (percent_full < 95 && wait_time <= 0) {
            digitalWrite(PUMP_RELAY, HIGH);
            pump_on = true;
            time_counter = 0;
        }
    }

    // Some error occurred, print it out!
    if (status != 0) {
        if  ((status >= VL6180X_ERROR_SYSERR_1) && (status <= VL6180X_ERROR_SYSERR_5)) {
            Serial.println("System error");
        }
        else if (status == VL6180X_ERROR_ECEFAIL) {
            Serial.println("ECE failure");
        }
        else if (status == VL6180X_ERROR_NOCONVERGE) {
            Serial.println("No convergence");
        }
        else if (status == VL6180X_ERROR_RANGEIGNORE) {
            Serial.println("Ignoring range");
        }
        else if (status == VL6180X_ERROR_SNR) {
            Serial.println("Signal/Noise error");
        }
        else if (status == VL6180X_ERROR_RAWUFLOW) {
            Serial.println("Raw reading underflow");
        }
        else if (status == VL6180X_ERROR_RAWOFLOW) {
            Serial.println("Raw reading overflow");
        }
        else if (status == VL6180X_ERROR_RANGEUFLOW) {
            Serial.println("Range reading underflow");
        }
        else if (status == VL6180X_ERROR_RANGEOFLOW) {
            Serial.println("Range reading overflow");
        } else {
            Serial.println(status);
        }
    }
}
