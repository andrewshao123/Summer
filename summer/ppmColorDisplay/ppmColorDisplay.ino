/*
troubleshoot procedures:
If the serial port does not read anything, possible problems causing the problem:
1. the reading is out of the range
2. the connection is loose -> only 0
*/
int ledcolor = 0;
int a = 1000; //this sets how long the stays one color for
int red = 9; //this sets the red led pin
int green = 8; //this sets the green led pin
int blue = 10; //this sets the blue led pin
int delay1 = 100; // delay for half a second
int R=0;//value for red
int G=0;//value for green
int B=0;//value for blue
int increment = 17;//increment/decrement of each number
int colorIndex = 0;
int x;//for serial reading
String str;
int gasSensor = 0; // select analog pin for gasSensor
int val = 0; // variable to store the value coming from the sensor
int counterSetup = 0;
int average = 0;
//USB port
int resistanceRange[] = {100,550}; //range for gas sensor value***change this value to change the upper/lower bound resistance range
//Battery
//int resistanceRange[] = {450,600}; //range for gas sensor value***change this value to change the upper/lower bound resistance range
float Ro = 10000.0;
float NOVrl = 0.0;
float NORs = 0.0;
float NOratio = 0.0;
float NOppm = 0.0;

int colorRange = 1100;//***change this value to change the color of the highest resistance
void setup() { //this sets the output pins
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  Serial.begin(9600);
}

void loop(){
  val = analogRead(gasSensor); // read the value from the pot
  NOVrl = val * ( 5.00 / 1024.0  );      // V    //SEND <gas>ppm VALUES OUT (ppm)
  NORs = 20000 * ( 5.00 - NOVrl) / NOVrl ;   // Ohm
  NOratio =  NORs/Ro;
  NOppm = get_ppm(NOratio);
  delay(delay1);
  Serial.print("val: ");
  Serial.print(val);
  Serial.print(" ppm: ");
  Serial.println( NOppm );

//  if (val != 0) {
//    if (NOppm != 0 && NOppm >= resistanceRange[0] && NOppm <= resistanceRange[1]){//filter out the edge values such as if the values are out of the range or if the value is 0    
//      colorIndex = custom_map(NOppm);//convert the color value to the one that is mapped to the led
//      Serial.print("colorindex: ");
//      Serial.println(colorIndex);
//    }
//    else if(NOppm > resistanceRange[1]){
//      colorIndex = 255;
//    }
//    Display(colorIndex);
//  }
  if (val != 0) {
    if (val >= resistanceRange[0] && val <= resistanceRange[1]){//filter out the edge values such as if the values are out of the range or if the value is 0    
      colorIndex = custom_map(val);//convert the color value to the one that is mapped to the led
      Serial.print("colorindex: ");
      Serial.println(colorIndex);
    }
    else if(val > resistanceRange[1]){
      //colorIndex = 255;
      colorIndex = 1400;
    }
    else if(val < resistanceRange[0]){
      colorIndex = 255;
      //colorIndex = 1400;
    }
    Display(colorIndex);
  }
}

void Display(int colorIndex){
  analogWrite(red, R);
  analogWrite(green, G);
  analogWrite(blue, B);
  Serial.print("R: ");
  Serial.print(R);
  Serial.print(" G: ");
  Serial.print(G);
  Serial.print(" B: ");
  Serial.println(B);
  if (colorIndex <= 255){//red
    R = colorIndex; 
    G = 0;
    B = 0;
  }
  else if(colorIndex >255 && colorIndex <= 510){//red-<yellow
    R = 255;
    G = colorIndex - 255;
    B = 0; 
  }
  else if(colorIndex >510 && colorIndex <= 765){//yellow->green
    R = 255 - (colorIndex-510);
    G = 255;
    B = 0;
  }
  else if(colorIndex >765 && colorIndex <=1020){//green->cyan
    R = 0;
    G = 255;
    B = colorIndex - 765; 
  }
  else if(colorIndex >1020 && colorIndex <=1275){//cyan->blue
    R = 0;
    G = 255 - (colorIndex-1020); 
    B = 255;
  }
  else if(colorIndex >1275 && colorIndex <=1530){//blue->purple
    R = colorIndex - 1275;
    G = 0;
    B = 255;
  }
  else if(colorIndex >1530 && colorIndex <=1785){//purple->red
    R = 255;
    G = 0;
    B = 255 - (colorIndex-1530); 
  }  
}

void readSignalFromComp() {//read from serial port
  if(Serial.available() > 0){
    str = Serial.readStringUntil('\n');
    x = Serial.parseInt();
    Serial.println(str);
  }
}

//990-1008 map to 220~1785
int custom_map(int num){//map the values
  int adjustedLight;
  float range = float(colorRange)/(float(resistanceRange[1]-resistanceRange[0]));
  //adjustedLight = 1400 - range*(num-resistanceRange[0]);
  adjustedLight = 300 + range*(num-resistanceRange[0]);
  Serial.println(adjustedLight);
  return (int(adjustedLight));
}

int DynamicCheck(int val){
  int sum = 0;
  for (int i = 0; i<50;i++){
    sum = sum + val;
  }
  average = sum/50;
  //if (average)
}
float get_ppm (float ratio){
  float ppm = 0.0;
  ppm = 37143 * pow (ratio, -3.178);
  return ppm;
}
