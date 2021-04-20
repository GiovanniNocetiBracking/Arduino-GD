/********************************************************************************************************** 
  Author : Andy @ MYBOTIC www.mybotic.com.my ==>  Modified by Jhimmy Astoraque https://www.youtube.com/c/jadsatv
  Date:5/7/2016 -- 24/04/2019
  Project: How to detect the concentration of gas by using MQ2 sensor
**********************************************************************************************************/
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

SoftwareSerial nodemcu(5, 6);
/************************Hardware Related Macros************************************/
const int calibrationLed = 13;    //when the calibration start , LED pin 13 will light up , off when finish calibrating
const int MQ_PIN = A0;            //define which analog input channel you are going to use
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
  pinMode(calibrationLed, OUTPUT);
  digitalWrite(calibrationLed, HIGH);
  Serial.print("Calibrating...");

  Ro = MQCalibration(MQ_PIN); //Calibrating the sensor. Please make sure the sensor is in clean air
  digitalWrite(calibrationLed, LOW);

  Serial.println("done!");
  Serial.print("Ro= ");
  Serial.print(Ro);
  Serial.println("kohm\n");
  delay(2000);
}

void loop()
{
  long iPPM_LPG = 0;
  long iPPM_CO = 0;
  long iPPM_Smoke = 0;

  iPPM_LPG = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_LPG);
  iPPM_CO = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_CO);
  iPPM_Smoke = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_SMOKE);

  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject &data = jsonBuffer.createObject();

  data["LPG"] = iPPM_LPG;
  data["CO"] = iPPM_CO;
  data["SMOKE"] = iPPM_Smoke;

  data.printTo(nodemcu);
  jsonBuffer.clear();

  Serial.println("**************************** CONCETRATION OF GASES ****************************");

  Serial.print("LPG: ");
  Serial.print(iPPM_LPG);
  Serial.println(" ppm");

  Serial.print("CO: ");
  Serial.print(iPPM_CO);
  Serial.println(" ppm");

  Serial.print("Smoke: ");
  Serial.print(iPPM_Smoke);
  Serial.println(" ppm");
  Serial.println("*******************************************************************************\n");
  Serial.println();

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
