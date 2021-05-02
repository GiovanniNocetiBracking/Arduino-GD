#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>    // include Adafruit graphics library
#include <Adafruit_ST7735.h> // include Adafruit ST7735 TFT library

#define TFT_RST 9 // TFT RST pin is connected to arduino pin 8
#define TFT_CS 10 // TFT CS  pin is connected to arduino pin 9
#define TFT_DC 8  // TFT DC  pin is connected to arduino pin 10
// initialize ST7735 TFT library
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
//declare the pins for serial comunication with nodeMCU
SoftwareSerial nodemcu(5, 6);
/************************Hardware Related Macros************************************/
const int MQ_PIN = A0; //define which analog input channel you are going to use
const int buzzer = 7;
const int ledVerde = 4;
const int ledAmarillo = 3;
const int ledRojo = 2;
int RL_VALUE = 1;                 //define the load resistance on the board, in kilo ohms
float RO_CLEAN_AIR_FACTOR = 9.86; //RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,
                                  //which is derived from the chart in datasheet

/***********************Software Related Macros************************************/
int CALIBARAION_SAMPLE_TIMES = 50;     //define how many samples you are going to take in the calibration phase
int CALIBRATION_SAMPLE_INTERVAL = 500; //define the time interal(in milisecond) between each samples in the
                                       //cablibration phase
int READ_SAMPLE_INTERVAL = 50;         //define how many samples you are going to take in normal operation
int READ_SAMPLE_TIMES = 5;             //define the time interal(in milisecond) between each samples in
                                       //normal operation

/**********************Application Related Macros**********************************/
#define GAS_LPG 0
#define GAS_CO 1
#define GAS_SMOKE 2

/*****************************Globals***********************************************/
float LPGCurve[3] = {2.3, 0.21, -0.47};   //two points are taken from the curve.
                                          //with these two points, a line is formed which is "approximately equivalent"
                                          //to the original curve.
                                          //data format:{ x, y, slope}; point1: (lg200, 0.21), point2: (lg10000, -0.59)
float COCurve[3] = {2.3, 0.72, -0.34};    //two points are taken from the curve.
                                          //with these two points, a line is formed which is "approximately equivalent"
                                          //to the original curve.
                                          //data format:{ x, y, slope}; point1: (lg200, 0.72), point2: (lg10000,  0.15)
float SmokeCurve[3] = {2.3, 0.53, -0.44}; //two points are taken from the curve.
                                          //with these two points, a line is formed which is "approximately equivalent"
                                          //to the original curve.
                                          //data format:{ x, y, slope}; point1: (lg200, 0.53), point2: (lg10000,  -0.22)
float Ro = 10;                            //Ro is initialized to 10 kilo ohms

void setup()
{
  Serial.begin(9600);
  nodemcu.begin(9600);

  pinMode(ledVerde, OUTPUT);    //definir pin como salida
  pinMode(ledAmarillo, OUTPUT); //definir pin como salida
  pinMode(ledRojo, OUTPUT);     //definir pin como salida

  Serial.print("Calibrating...");
  Ro = MQCalibration(MQ_PIN); //Calibrating the sensor. Please make sure the sensor is in clean air
  Serial.println("done!");
  Serial.print("Ro= ");
  Serial.print(Ro);
  Serial.println("kohm\n");

  tft.initR(INITR_BLACKTAB);                    // initialize a ST7735S chip, black tab
  tft.fillScreen(ST7735_BLACK);                 // fill screen with black color
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK); // set text color to white and black background
  tft.setTextSize(2);                           // text size = 1
  tft.setCursor(5, 5);                          // move cursor to position (4, 16) pixel
  tft.print("GAS DETECT ");
  tft.drawFastHLine(0, 25, tft.width(), ST7735_BLUE); // draw horizontal blue line at position (0, 50)
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);       // set text color to green and black background
  tft.setCursor(5, 30);                               // move cursor to position (25, 61) pixel
  tft.print("GLP");
  tft.drawFastHLine(0, 70, tft.width(), ST7735_BLUE); // draw horizontal blue line at position (0, 102)
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);       // set text color to yellow and black background
  tft.setCursor(5, 75);                               // move cursor to position (34, 113) pixel
  tft.print("CO");
  tft.drawFastHLine(0, 115, tft.width(), ST7735_BLUE); // draw horizontal blue line at position (0, 102)
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);        // set text color to yellow and black background
  tft.setCursor(5, 120);                               // move cursor to position (34, 113) pixel
  tft.print("HUMO");

  delay(2000);
}
char _buffer[3];

void loop()
{
  long iPPM_LPG = 0;
  long iPPM_CO = 0;
  long iPPM_Smoke = 0;

  iPPM_LPG = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_LPG);
  iPPM_CO = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_CO);
  iPPM_Smoke = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_SMOKE);

  if (iPPM_LPG <= 500)
  {
    digitalWrite(ledVerde, HIGH);
    digitalWrite(ledAmarillo, LOW);
    digitalWrite(ledRojo, LOW);
  }
  if (iPPM_LPG > 500 && iPPM_LPG <= 1000)
  {
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledAmarillo, HIGH);
  }
  if (iPPM_LPG > 1000)
  {
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledAmarillo, LOW);
    digitalWrite(ledRojo, HIGH);
    //generar tono de 523Hz durante 500ms, y detenerlo durante 500ms.
    tone(buzzer, 523, 300);
    delay(500);
  }

  sprintf(_buffer, "%05u PPM%", iPPM_LPG);
  tft.setTextColor(ST7735_CYAN, ST7735_BLACK); // set text color to cyan and black background
  tft.setCursor(10, 50);
  tft.print(_buffer);
  sprintf(_buffer, "%05u PPM%", iPPM_CO);
  tft.setTextColor(ST7735_CYAN, ST7735_BLACK); // set text color to cyan and black background
  tft.setCursor(10, 95);
  tft.print(_buffer);
  sprintf(_buffer, "%05u PPM%", iPPM_Smoke);
  tft.setTextColor(ST7735_CYAN, ST7735_BLACK); // set text color to cyan and black background
  tft.setCursor(10, 140);
  tft.print(_buffer);

  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject &data = jsonBuffer.createObject();

  data["LPG"] = iPPM_LPG;
  data["CO"] = iPPM_CO;
  data["SMOKE"] = iPPM_Smoke;

  data.printTo(nodemcu);
  jsonBuffer.clear();

  delay(500);
}

/****************** MQResistanceCalculation ****************************************
Input:   raw_adc - raw value read from adc, which represents the voltage
Output:  the calculated sensor resistance
Remarks: The sensor and the load resistor forms a voltage divider. Given the voltage
         across the load resistor and its resistance, the resistance of the sensor
         could be derived.
************************************************************************************/
float MQResistanceCalculation(int raw_adc)
{
  return (((float)RL_VALUE * (1023 - raw_adc) / raw_adc));
}

/***************************** MQCalibration ****************************************
Input:   mq_pin - analog channel
Output:  Ro of the sensor
Remarks: This function assumes that the sensor is in clean air. It use  
         MQResistanceCalculation to calculates the sensor resistance in clean air 
         and then divides it with RO_CLEAN_AIR_FACTOR. RO_CLEAN_AIR_FACTOR is about 
         10, which differs slightly between different sensors.
************************************************************************************/
float MQCalibration(int mq_pin)
{
  int i;
  float val = 0;

  for (i = 0; i < CALIBARAION_SAMPLE_TIMES; i++)
  { //take multiple samples
    val += MQResistanceCalculation(analogRead(mq_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val = val / CALIBARAION_SAMPLE_TIMES; //calculate the average value
  val = val / RO_CLEAN_AIR_FACTOR;      //divided by RO_CLEAN_AIR_FACTOR yields the Ro
  return val;                           //according to the chart in the datasheet
}

/*****************************  MQRead *********************************************
Input:   mq_pin - analog channel
Output:  Rs of the sensor
Remarks: This function use MQResistanceCalculation to caculate the sensor resistenc (Rs).
         The Rs changes as the sensor is in the different consentration of the target
         gas. The sample times and the time interval between samples could be configured
         by changing the definition of the macros.
************************************************************************************/
float MQRead(int mq_pin)
{
  int i;
  float rs = 0;

  for (i = 0; i < READ_SAMPLE_TIMES; i++)
  {
    rs += MQResistanceCalculation(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
  }

  rs = rs / READ_SAMPLE_TIMES;

  return rs;
}

/*****************************  MQGetGasPercentage **********************************
Input:   rs_ro_ratio - Rs divided by Ro
         gas_id      - target gas type
Output:  ppm of the target gas
Remarks: This function passes different curves to the MQGetPercentage function which 
         calculates the ppm (parts per million) of the target gas.
************************************************************************************/
long MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
  if (gas_id == GAS_LPG)
  {
    return MQGetPercentage(rs_ro_ratio, LPGCurve);
  }
  else if (gas_id == GAS_CO)
  {
    return MQGetPercentage(rs_ro_ratio, COCurve);
  }
  else if (gas_id == GAS_SMOKE)
  {
    return MQGetPercentage(rs_ro_ratio, SmokeCurve);
  }

  return 0;
}

/*****************************  MQGetPercentage **********************************
Input:   rs_ro_ratio - Rs divided by Ro
         pcurve      - pointer to the curve of the target gas
Output:  ppm of the target gas
Remarks: By using the slope and a point of the line. The x(logarithmic value of ppm) 
         of the line could be derived if y(rs_ro_ratio) is provided. As it is a 
         logarithmic coordinate, power of 10 is used to convert the result to non-logarithmic 
         value.
************************************************************************************/
long MQGetPercentage(float rs_ro_ratio, float *pcurve)
{
  return (pow(10, (((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0])));
}
