#include "SdsDustSensor.h"
#include "ArduinoLowPower.h"
#include "arduino_secrets.h"
#include <Wire.h>
#include "Adafruit_HTU21DF.h"
#include <MKRWAN.h>
#include <RTCZero.h>
// 48 messages par jour pendant 120 secondes  1 message toutes les 30 mins apres 120 secondes
// In this sketch, the internal RTC will wake up the processor every 2 seconds.
//Please note that, if the processor is sleeping, a new sketch can't be uploaded. To overcome this, manually reset the board (usually with a single or double tap to the RESET button)

// Please enter your sensitive data in the Secret tab or arduino_secrets.h
String appEui = SECRET_APP_EUI;
String appKey = SECRET_APP_KEY;

#define ACTIVEPM 9
#define ACTIVETH 10
#define RESET 8
#define PULSE 7


void dopulse(void);
void dopulse(void)
{
    int index = 0;
    for(index = 0; index <154; index ++)
    {
      digitalWrite(ACTIVEPM, HIGH);
      digitalWrite(ACTIVETH, HIGH);
      digitalWrite(PULSE, HIGH);
      delay(1000);
      digitalWrite(PULSE, LOW);
      digitalWrite(ACTIVEPM, LOW);
      digitalWrite(ACTIVETH, LOW);
      LowPower.sleep(10*1000);
      Serial.println(index);
      Serial.print(", index = ");
    }
    Serial.print("RESET");
    digitalWrite(RESET, LOW);
}

//LoRaModem modem;
SdsDustSensor sds(Serial1);
bool toggle = 0;
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
int count = 0;
//char msg[10];
String msg = "loic";
String stringOne =  String(5.698, 2) + msg;
float humidity = 0;
float temperature = 0;
float pm25 = 0;
float pm10 = 0;
float moyTemp = 0;
float moyHum = 0;
float moyPM25 = 0;
float moyPM10 = 0;
int countSleep = 0;  //sleep of 10 secondes

void setup() 
{
    digitalWrite(RESET, HIGH);
    
    pinMode(PULSE, OUTPUT); 
    digitalWrite(PULSE, LOW);
    delay(200); 
    
    pinMode(RESET, OUTPUT); 
  // Uncomment this function if you wish to attach function dummy when RTC wakes up the chip
 // LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, dummy, CHANGE);
  Serial.begin(115200);
  pinMode(ACTIVEPM, OUTPUT);    // sets the digital pin 13 as output
  pinMode(ACTIVETH, OUTPUT);    // sets the digital pin 13 as output
  digitalWrite(ACTIVEPM, HIGH);
  digitalWrite(ACTIVETH, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  sds.begin();
  
  if (!htu.begin()) {
  Serial.println("Couldn't find sensor!");
  }
  delay(1000);
  PmResult pm = sds.readPm();
  Serial.println(sds.queryFirmwareVersion().toString()); // prints firmware version
  Serial.println(sds.setActiveReportingMode().toString()); // ensures sensor is in 'active' reporting mode
  Serial.println(sds.setContinuousWorkingPeriod().toString()); // ensures sensor has continuous working period - default but not recommended
}

void loop() 
{
 // digitalWrite(ACTIVEPM, HIGH);
 // digitalWrite(ACTIVETH, HIGH);
 // digitalWrite(LORA_RESET, LOW);

  PmResult pm = sds.readPm();
  if (pm.isOk()) 
  {
    Serial.print("PM2.5 = ");
    Serial.print(pm.pm25);
    Serial.print(", PM10 = ");
    Serial.println(pm.pm10);
    
    // if you want to just print the measured values, you can use toString() method as well
    Serial.println(pm.toString());
  } else 
  {
    // notice that loop delay is set to 0.5s and some reads are not available
    Serial.print("Could not read values from sensor, reason: ");
    Serial.println(pm.statusToString());
  }
  if(toggle == 1)
  {
    toggle = 0;
    digitalWrite(LED_BUILTIN, 1);
  }
  else
  {
    toggle = 1;
    digitalWrite(LED_BUILTIN, 0);
  }
  
  float temp = htu.readTemperature();
  float rel_hum = htu.readHumidity();
  Serial.print("Temp: "); Serial.print(temp); Serial.print(" C");
  Serial.print("\t\t");
  Serial.print("Humidity: "); Serial.print(rel_hum); Serial.println(" \%");

  humidity = humidity + rel_hum;
  temperature = temperature + temp;
  pm25 = pm25 + pm.pm25;
  pm10 = pm10 + pm.pm10;

  count = count + 1;
  if(count >=100)
  {

    moyTemp = temperature/count;
    moyHum = humidity/count;
    moyPM25 = pm25/count;
    moyPM10 = pm10/count;
    Serial.print(", moyPM10 = ");
    Serial.println(moyPM10);
    Serial.print(", moyPM25 = ");
    Serial.println(moyPM25);
    Serial.print(", moyTemp = ");
    Serial.println(moyTemp);
    Serial.print(", moyHum = ");
    Serial.println(moyHum);

LoRaModem modem;
    count = 0;
    delay(500);
    stringOne =  String(moyTemp, 2) + " " + String(moyHum, 2) + " " + String(moyPM25, 2) + " " + String(moyPM10, 2);
    int err;
  if (!modem.begin(EU868)) {
    Serial.println("Failed to start module");
  };

  Serial.print("Your module version is: ");
  Serial.println(modem.version());
  Serial.print("Your device EUI is: ");
  Serial.println(modem.deviceEUI());

  int connected = modem.joinOTAA(appEui, appKey);
  if (!connected) {
    Serial.println("Something went wrong; are you indoor? Move near a window and retry");
  }

  modem.minPollInterval(60);
  modem.beginPacket();
  modem.print(stringOne);
  err = modem.endPacket(true);
  if (err > 0) {
    Serial.println("Message sent correctly!");
  } else {
    Serial.println("Error sending message :(");
    Serial.println("(you may send a limited amount of messages per minute, depending on the signal strength");
    Serial.println("it may vary from 1 message every couple of seconds to 1 message every minute)");
  }

    delay(5000);
    digitalWrite(LORA_RESET, LOW);
    digitalWrite(ACTIVEPM, LOW);
    digitalWrite(ACTIVETH, LOW);
    digitalWrite(LED_BUILTIN, 0);
    delay(200);

    //LowPower.sleep(5*60*1000);
   dopulse();
    
  }
  delay(1000);
}

void dummy() 
{
 digitalWrite(RESET, LOW);
}
