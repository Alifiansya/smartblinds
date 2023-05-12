#include <Arduino_FreeRTOS.h>
#include <LiquidCrystal_SoftI2C.h>
#include <EEPROM.h>

#define JAM 0x00
#define MENIT 0x01
#define DETIK 0x02

uint8_t timeCounter[3];
uint8_t timeOpen[2];
uint8_t timeClose[2];

void taskTimeCounter(void *pvParameters);
void taskLcdController(void *pvParameters);

void loadEepromData();


void setup() 
{
  loadEepromData();
  xTaskCreate(taskLcdController, "LcdController", 128, NULL, 2, NULL);
}

void loop() {}

void taskLcdController(void *pvParameters) 
{
  SoftwareWire *wire = new SoftwareWire(PIN_WIRE_SDA, PIN_WIRE_SCL);
  LiquidCrystal_I2C lcd(0x27, 16, 2, wire);
  lcd.begin();
  while(true) 
  {
    String timeStr[3];
    for(int i = 0; i < 3; i++)
      timeStr[i] = (timeCounter[i] > 9 ? String(timeCounter[i]) : "0" + String(timeCounter[i]));
    
    lcd.setCursor(0, 0);
    lcd.print("TIME=" + timeStr[JAM] + ":" + timeStr[MENIT] + ":" + timeStr[DETIK]);
    
  }
}

void loadEepromData()
{
  uint8_t timeCounterAddr[] = {0x00, 0x01, 0x02};
  uint8_t timeOpenAddr[] = {0x10, 0x11};
  uint8_t timeCloseAddr[] = {0x20, 0x21};
  for(int i = 0; i < 2; i++) 
  {
    timeCounter[i] = EEPROM.read(timeCounterAddr[i]);
    timeOpen[i] = EEPROM.read(timeOpenAddr[i]);
    timeClose[i] = EEPROM.read(timeCloseAddr[i]);
  }
  timeCounter[2] = EEPROM.read(timeCounterAddr[2]);
}

