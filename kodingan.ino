#include <Arduino_FreeRTOS.h>
#include <LiquidCrystal_SoftI2C.h>
#include <EEPROM.h>

/*
 * EEPROM DATA ADDRESS LOCATION:
 * TIME COUNTER 0x00 - 0x02
 * TIME OPEN 0x10 - 0x12
 * TIME CLOSED 0x20 - 0x22
 */

/*
 * SELF NOTE:
 * Every task in FreeRTOS have allocated memory, smaller
 * than normal usage, becareful when allocating memory and
 * reuse allocated memory if possible
 */
#define JAM 0x00
#define MENIT 0x01
#define DETIK 0x02

uint8_t timeCounter[3];
uint8_t timeOpen[2];
uint8_t timeClose[2];

uint8_t timeCounterAddr[] = {0x00, 0x01, 0x02};
uint8_t timeOpenAddr[] = {0x10, 0x11};
uint8_t timeCloseAddr[] = {0x20, 0x21};

SoftwareWire *wire = new SoftwareWire(PIN_WIRE_SDA, PIN_WIRE_SCL);
LiquidCrystal_I2C lcd(0x27, 16, 2, wire);
  

void taskTimeCounter(void *pvParameters);
void taskLcdController(void *pvParameters);

void loadEepromData();


void setup() 
{
  Serial.begin(9600);
  lcd.begin();
  loadEepromData();
  xTaskCreate(taskTimeCounter, "TimeCounter", 128, NULL, 1, NULL);
  xTaskCreate(taskLcdController, "LcdController", 128, NULL, 2, NULL);
}

void loop() {}

void taskTimeCounter(void *pvParameters) 
{
  while(true)
  {
    timeCounter[DETIK]++;
    if(timeCounter[DETIK] >= 60) 
    {
      timeCounter[MENIT]++;
      timeCounter[DETIK] = 0;
    }
    if(timeCounter[MENIT] >= 60) 
    {
      timeCounter[JAM]++;
      timeCounter[MENIT] = 0;
    }
    if(timeCounter[JAM] >= 24) timeCounter[JAM] = 0;
    updateEepromData(0);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void taskLcdController(void *pvParameters) 
{

  while(true) 
  {
    // 5 lines below needs a callable function (COMMENTS NOT INCLUDED)
    String timeStr[3];
    // Output Time Counter to LCD
    for(int i = 0; i < 3; i++)
    {
      timeStr[i] = (timeCounter[i] > 9 ? String(timeCounter[i]) : "0" + String(timeCounter[i]));   
    }
    lcd.setCursor(0, 0);
    lcd.print("TIME=" + timeStr[JAM] + ":" + timeStr[MENIT] + ":" + timeStr[DETIK]);
    
    // Output Time Open to LCD
    for(int i = 0; i < 2; i++)
    {
      timeStr[i] = (timeOpen[i] > 9 ? String(timeOpen[i]) : "0" + String(timeOpen[i]));   
    }
    lcd.setCursor(0, 1);
    lcd.print("Open " + timeStr[JAM] + ":" + timeStr[MENIT] + "-"); //11 char
                      
    // Output Time Close to LCD
    for(int i = 0; i < 2; i++)
    {
      timeStr[i] = (timeClose[i] > 9 ? String(timeClose[i]) : "0" + String(timeClose[i]));   
    }
    lcd.setCursor(11, 1);
    lcd.print(timeStr[JAM] + ":" + timeStr[MENIT]); //5 char
    vTaskDelay(1);
  }
}

void loadEepromData()
{
  for(int i = 0; i < 2; i++) 
  {
    timeCounter[i] = EEPROM.read(timeCounterAddr[i]);
    timeOpen[i] = EEPROM.read(timeOpenAddr[i]);
    timeClose[i] = EEPROM.read(timeCloseAddr[i]);
  }
  timeCounter[2] = EEPROM.read(timeCounterAddr[2]);
}

void updateEepromData(uint8_t time) {
  // Switch statement, 0 = timeCounter, 1 = timeOpen, 2 = timeClose
  switch(time) {
    case 1:
      for(int i = 0; i < 2; i++) EEPROM.update(timeOpenAddr[i], timeOpen[i]);
      break;
    case 2:
      for(int i = 0; i < 2; i++) EEPROM.update(timeCloseAddr[i], timeClose[i]);
      break;
    default:
      for(int i = 0; i < 3; i++) EEPROM.update(timeCounterAddr[i], timeCounter[i]);
      break;
  }
}