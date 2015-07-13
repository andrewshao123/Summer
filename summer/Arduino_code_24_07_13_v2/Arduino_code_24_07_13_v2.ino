
#include <Wire.h> // Wire library that helps the Arduino with i2c 
#include <Streaming.h> // Streaming C++-style Output with Operator << from http://arduiniana.org/libraries/streaming/
#include <SPI.h>
//------------------------------------------------------------------------------
// CONFIGURATION
//------------------------------------------------------------------------------

// define FAST sensor sampling interval
unsigned long INTERVAL_FAST = 5000; // sample duration (ms)30s, david holstius used 60000
unsigned long last_time_fast_ms;     // time of last check/output

char CSV_HEADER[] = "Timestamp,Sensor,Variable,Value";

//------------------------------------------------------------------------------
// COMPONENT SETUP
//------------------------------------------------------------------------------

//-------------------------------------------------------------------------- MUX

int MUX_BASE = 0; // analog pin to interact with mux
int MUX_CONTROL[] = {2, 3, 4, 5}; // digital pins for mux pin selection
int mux(int channel) {
  // select channel on multiplexer
  for (int i = 0; i < 4; i++) { digitalWrite(MUX_CONTROL[i], bitRead(channel, i)); }
  // return base pin for analogRead/Write to use
  return(MUX_BASE);
}

void setup_mux() {
  // set digital pins controlling multiplexer to OUTPUT
  for (int i = 0; i < 4; i++) { 
    pinMode(MUX_CONTROL[i], OUTPUT); 
  }
}

//------------------------------------------------------------ SD card (logging)

#include "SD.h" //SD library to talk to the card
#define SD_PIN 10 // for the data logging shield, we use digital pin 10 for the SD cs line
#define SD_INTERVAL_SYNC 1000 // mills between calls to flush() - to write data to the card
uint32_t sd_last_time = 0; // time of last sync()
File logfile; // the logging file

// error function (ex. couldn't write to the SD card or open it)
void error_sd(char *msg)  {
  log_error("SDcard", msg);
  while(1); // sits in a while loop forever to halt setup
}

// setup
void setup_sd() {
    
  // initialize the SD card and find a FAT16/FAT32 partition 
  log_info("SDcard", "initializing..."); 

  pinMode(SD_PIN, OUTPUT); // default chip select pin is set to output

  if (!SD.begin(SD_PIN)) { // see if the card is present and can be initialized
    error_sd("card failed or not present");
    return; // don't do anything else
  }

  log_info("SDcard", "initialized"); 

  // create a new file, LOGGERnnnn.csv (nnnn is a number), make a new file every time the Arduino starts up 
  char filename[] = "0000.csv"; 
  for (uint16_t i = 0; i < 10000; i++) {

    for (int j = 0; j < 4; j++) {
      filename[4-j-1] = ((int)(i / pow(10, j)) % 10) + '0';
    }

    if(!SD.exists(filename)) { // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); // Unix style command flags - logfile.open() procedure FILE_WRITE - create the file and write data to it
      break;  // leave the loop!
    }
  }

  if (! logfile) {
    error_sd("could not create file");
  }
  
  logfile << CSV_HEADER << endl;
  log_info("SDcard", filename);

  sd_last_time = millis();
}

//------------------------------------------------------ Chronodot (timekeeping)

#include "Chronodot.h" // Chronodot RTC library
Chronodot RTC;

// setup
void setup_rtc() {
  // kick off the RTC initializing the Wire library - poking the RTC to see if its alive
  Wire.begin();  
  RTC.begin(); // Chronodot

  if (!RTC.begin()) {
    logfile.println("RTC failed");
    Serial.println("RTC failed");
  }
}

// log
void log_time() {
  DateTime now = RTC.now();
  int year  = now.year();
  int month = now.month();
  int day   = now.day();
  int hours = now.hour();
  int mins  = now.minute();
  int secs  = now.second();
  Serial  << "\"" << year;
  Serial  << "-" << ((month<10)?"0":"") << month;
  Serial  << "-" << ((day<10)?"0":"")   << day;
  Serial  << " " << ((hours<10)?"0":"") << hours;
  Serial  << ":" << ((mins<10)?"0":"")  << mins;
  Serial  << ":" << ((secs<10)?"0":"")  << secs;
  Serial  << "\",";
  logfile << "\"" << year;
  logfile << "-" << ((month<10)?"0":"") << month;
  logfile << "-" << ((day<10)?"0":"")   << day;
  logfile << " " << ((hours<10)?"0":"") << hours;
  logfile << ":" << ((mins<10)?"0":"")  << mins;
  logfile << ":" << ((secs<10)?"0":"")  << secs;
  logfile << "\",";
}

//---------------------------------------------------------- DHT22 (temperature)

#include "DHT.h"
#define DHT_PIN 7
#define DHT_TYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHT_PIN, DHT_TYPE);

// setup
void setup_dht() {
  dht.begin();
}

// log
void log_dht() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h)) {
    log_error("DHT22", "failed to read");
  } else {
    log_measurement("DHT22", "humidity", h);
    log_measurement("DHT22", "temperature", t);
  }
}

//----------------------------------------------------------- Sinyei (particles)

#define SNY_PIN 8
unsigned long sny_duration;
unsigned long sny_lowocc_us = 0; // sum of time spent LOW

unsigned long SNY_INTERVAL_ms = 30000; // sample duration (ms)30s, david holstius used 60000
unsigned long sny_last_time;           // time of last serial dump

// setup
void setup_sinyei() {
  pinMode(SNY_PIN, INPUT);
  sny_last_time = millis();
  sny_lowocc_us = 0;
}

// reading
void log_sinyei(unsigned long lowocc_us, unsigned long sampletime_ms) {
  float ratio; // fraction of time spent LOW
  float concentration = 0;

  ratio = lowocc_us / (sampletime_ms * 1000.0) * 100.0;
  concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62; // using spec sheet curve

  log_measurement("Sinyei", "sampletime_ms", sampletime_ms);
  log_measurement("Sinyei", "low_occupancy_duration_us", lowocc_us);
  log_measurement("Sinyei", "occupancy_ratio", ratio);
  log_measurement("Sinyei", "concentration", concentration);
}

//------------------------------------------------------------ Sharp (particles)

// connected to analog 1 and controlled by digital 9
#define SRP_PIN_DUST 1
#define SRP_PIN_LED 9 // led Power is any digital pin on the arduino connected to Pin 3 on the sensor (can be 2, 4,…)

int srp_delay1_us = 280; // delays are in microseconds
int srp_delay2_us = 40;
int srp_delayoff_us = 9680;

// setup
void setup_sharp() {
  pinMode(SRP_PIN_LED, OUTPUT);
}

// log
void log_sharp() {
  digitalWrite(SRP_PIN_LED, LOW); // power on the LED
  delayMicroseconds(srp_delay1_us);
  int dustVal = analogRead(SRP_PIN_DUST); // read the dust value
  delayMicroseconds(srp_delay2_us);
  digitalWrite(SRP_PIN_LED, HIGH); // turn the LED off
  delayMicroseconds(srp_delayoff_us);

  log_measurement("Sharp", "dust", dustVal);
}

//------------------------------------------------------- Arduino voltage supply

// read Arduino voltage
long read_vcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both
  long result = (high<<8) | low;
  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

// log
void log_vcc() {
  float vcc = read_vcc();
  log_measurement("Arduino", "voltage", vcc);

}

//------------------------------------------------------------------------------
// LOGGING
//------------------------------------------------------------------------------

void log_measurement(String sensor, String variable, float value) {
  log_time();
  Serial  << sensor << "," << variable << ",";
  Serial.print(value, 4);
  Serial  << endl;
  logfile << sensor << "," << variable << ",";
  logfile.print(value, 4);
  logfile << endl;
}

void log_error(String sensor, String msg) {
  log_status(sensor, "ERROR", msg);
}

void log_info(String sensor, String msg) {
  log_status(sensor, "INFO", msg);
}

void log_status(String sensor, String category, String msg) {
  log_time();
  Serial  << sensor << "," << category << "," << msg << endl;
  logfile << sensor << "," << category << "," << msg << endl;
}

//------------------------------------------------------------------------------
// SETUP
//------------------------------------------------------------------------------

void setup(void) {

  // setup serial
  Serial.begin(9600);
  Serial << CSV_HEADER << endl;
   
  // setup RTC
  setup_rtc(); // all logging depends on RTC, so start it up first

  // setup SD card
  setup_sd();

  // setup MUX
  setup_mux();
  
  // setup DHT
  setup_dht();
  
  // setup Sinyei
  setup_sinyei();

  // setup Sharp
  setup_sharp();
  
  // set fast sensor clock
  last_time_fast_ms = millis(); // get the current time

}

//------------------------------------------------------------------------------
// MAIN LOOP
//------------------------------------------------------------------------------


void loop() {

//main loop: 1)Wait until time for the next reading, 2)Ask for the current time and date from RTC, 
//3)Log the time and date to SD, 4)Read sensors, 5)Log readings to the SD, 6)Sync data to the card at right time


  sny_duration = pulseIn(SNY_PIN, LOW);
  sny_lowocc_us += sny_duration;

  //-------------------------------------- read/log Sinyei sensor (30s interval)

  if ((millis() - sny_last_time) > SNY_INTERVAL_ms) {

    // log Sinyei
    log_sinyei(sny_lowocc_us, millis() - sny_last_time);

    // reset
    sny_lowocc_us = 0;
    sny_last_time = millis(); 

  }

  //--------------------------------- read/log other sensors (fast 10s interval)

  if ((millis() - last_time_fast_ms) > INTERVAL_FAST) {

    // log Arduino voltage
    log_vcc();

    // log DHT
    log_dht();

    // log Sharp
    log_sharp();

    // log MUX
    int num_sensors = 16;
    int sensor_value;
    String sensor_name = "channel_";

    for (int channel = 0; channel < num_sensors; channel++) {
      sensor_value = analogRead(mux(channel)); 
      log_measurement("Mux", sensor_name + ((channel<10)?"0":"") + channel, sensor_value);
    }

    // reset clock to current time
    last_time_fast_ms = millis(); 

  }

  //--------------------------------------------------------------- sync SD card

  // write data to disk! Don't sync too often (2048 bytes of I/O SD card, a lot of power, takes time)
  if ((millis() - sd_last_time) > SD_INTERVAL_SYNC) { 
    logfile.flush();
    sd_last_time = millis();
  }
}


