#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Ethernet.h>
#define DHTPIN 2     // Digital pin connected to the DHT sensor
#define ONE_WIRE_PIN 4    // Digital pin connected to the water Temp sensor
#define TdsSensorPin A1
#define VREF 5.0      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point

#define PHSensorPin A2         

 
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);


OneWire oneWire(ONE_WIRE_PIN);

DallasTemperature sensors(&oneWire);
unsigned long int avgValue;
float b;
int buf[10],temp;
int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0;

float WaterTempC=0;
// -------------------------
//         PH SENSOR
// -------------------------
int phValue;
float phVoltage;
float calibrationValue = 21.34-0.09;
unsigned long int avgPhVal;
float phAct;
int PHBuffer[10],phTemp;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  dht.begin();
  sensors.begin();
  pinMode(TdsSensorPin, INPUT);
  pinMode(phValue,INPUT); 
}

void loop() {
  // put your main code here, to run repeatedly:
// Wait a few seconds between measurements.
  delay(2000);
  sensors.requestTemperatures(); 
  WaterTempC=sensors.getTempCByIndex(0); 
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.print(F("h="));
  Serial.print(h);
  Serial.print(F(";"));
  Serial.print(F("t_c="));
  Serial.print(t);
  Serial.print(F(";"));
  Serial.print(F("wt_c="));
  Serial.print(WaterTempC);
  Serial.print(F(";"));

static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U)  //every 40 milliseconds,read the analog value from the ADC
  {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;
  }
  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U)
  {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
    float compensationCoefficient = 1.0 + 0.02 * (WaterTempC - 25.0); //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
    float compensationVolatge = averageVoltage / compensationCoefficient; //temperature compensation
    tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; //convert voltage value to tds value
    //Serial.print("voltage:");
    //Serial.print(averageVoltage,2);
    //Serial.print("V   ");
    Serial.print(F("tds="));
    Serial.print(tdsValue);
    Serial.print(F(";"));
  }

avgPhVal=analogRead(A2);  
 float volt=(float)avgPhVal*5.0/1024; 
  phAct = -5.70 * volt + calibrationValue;
 
 Serial.print(F("ph="));
 Serial.print(phAct);
 Serial.print(F(";"));
 Serial.println("");
}
int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
  {
    for (i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}
