int gasSensor = 1; // select analog pin for gasSensor
int val = 0; // variable to store the value coming from the sensor
float Ro = 20000.0;
float NOVrl = 0.0;
float NORs = 0.0;
float NOratio = 0.0;
float NOppm = 0.0;
int delay1 = 100; // delay for half a second

void setup() {
  Serial.begin(9600);

}

void loop() {
  val = analogRead(gasSensor); // read the value from the pot
  NOVrl = val * ( 5.00 / 1024.0  );      // V    //SEND <gas>ppm VALUES OUT (ppm)
  NORs = 20000 * ( 5.00 - NOVrl) / NOVrl ;   // Ohm
  NOratio =  NORs/Ro;
  NOppm = get_ppm(NOratio);
  delay(delay1);
  Serial.print("val: ");
  Serial.println(val);
  Serial.println(NOppm);

}
float get_ppm (float ratio){
  float ppm = 0.0;
  ppm = 37143 * pow (ratio, -3.178);
  return ppm;
}
