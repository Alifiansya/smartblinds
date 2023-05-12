#include <Arduino_FreeRTOS.h>
#include <LiquidCrystal_SoftI2C.h>
#include <EEPROM.h>



void taskTimeCounter(void *pvParameters);
void taskLcdController(void *pvParameters);

void setup() 
{
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
    lcd.print("Hello, World!");
  }
}