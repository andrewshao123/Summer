int measurePin = 6;
int ledPower = 12;
 
int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;
 
float voMeasured = 0;
float largeParticle = 0;
float smallParticle = 0;

float calcVoltage = 0;
float dustDensity = 0;

int pin = 8;
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 30000;//sampe 30s ;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;
 
void setup(){
  Serial.begin(9600);
  pinMode(ledPower,OUTPUT);
  pinMode(8,INPUT);
  starttime = millis();//get the current time;
}
 
void loop(){
  digitalWrite(ledPower,LOW); // power on the LED
  delayMicroseconds(samplingTime);
 
  voMeasured = analogRead(measurePin); // read the dust value
  delayMicroseconds(deltaTime);
  digitalWrite(ledPower,HIGH); // turn the LED off
  delayMicroseconds(sleepTime);
 
  // 0 - 3.3V mapped to 0 - 1023 integer values
  // recover voltage
  calcVoltage = voMeasured * (3.3 / 1024);
 
  // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
  dustDensity = 0.17 * calcVoltage - 0.1;
 
  //Serial.print("Raw Signal Value (0-1023): ");
  Serial.print(voMeasured);
 
  //Serial.print(" - Voltage: ");
  //Serial.print(calcVoltage);
 
  //Serial.print(" - Dust Density: ");
  //Serial.println(dustDensity);

  duration = pulseIn(pin, LOW);
  lowpulseoccupancy = lowpulseoccupancy+duration;

  if ((millis()-starttime) > sampletime_ms)//if the sampel time == 30s
  {
    ratio = lowpulseoccupancy/(sampletime_ms*10.0);  // Integer percentage 0=>100
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
    //Serial.print("duration = ");
    //Serial.print(duration);
    //Serial.print(" concentration = ");
    Serial.print(",");
    Serial.print(concentration);
    //Serial.println(" pcs/0.01cf");
    //Serial.println("\n");
    lowpulseoccupancy = 0;
    starttime = millis();
  }
  Serial.println("");
}
