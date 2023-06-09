#include <Arduino_FreeRTOS.h>
#include <LiquidCrystal_SoftI2C.h>
#include <EEPROM.h>
#include <ezButton.h>
#include <Servo.h>

#define JAM 0x00
#define MENIT 0x01
#define DETIK 0x02

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
enum LcdStates {COUNTING, CHANGING};

uint8_t timeCounter[3];
uint8_t timeOpen[2];
uint8_t timeClose[2];

uint8_t timeCounterAddr[] = {0x00, 0x01, 0x02};
uint8_t timeOpenAddr[] = {0x10, 0x11};
uint8_t timeCloseAddr[] = {0x20, 0x21};

uint8_t buttonClickCounter[3] = {0};

// checking if taskCounter running
// bool counterRunning = true;

// blinking signal
unsigned long blinkLast = 0;
bool blinkNow = true;

SoftwareWire *wire = new SoftwareWire(A0, A1);
LiquidCrystal_I2C lcd(0x27, 16, 2, wire);
LcdStates lcdState = COUNTING;
Servo servoBlinds;

ezButton button1(2, INPUT_PULLUP);
ezButton button2(3, INPUT_PULLUP);
ezButton button3(4, INPUT_PULLUP);

TaskHandle_t xTimeCounterHandle;
  
void taskTimeCounter(void *pvParameters);
void taskLcdController(void *pvParameters);
void taskButtonController(void *pvParameter);
void taskServoController(void *pvParameter);

void loadEepromData();
void blinkLcdTime();
void startLcdCounter();

void setup() 
{
  Serial.begin(9600);
  lcd.begin();
  loadEepromData();
//46:40
  button1.setDebounceTime(50);
  button2.setDebounceTime(50);
  button3.setDebounceTime(50);

  servoBlinds.attach(9);

  xTaskCreate(taskTimeCounter, "TimeCounter", 128, NULL, 1, &xTimeCounterHandle);
  xTaskCreate(taskLcdController, "LcdController", 128, NULL, 2, NULL);
  xTaskCreate(taskButtonController, "ButtonController", 128, NULL, 1, NULL);
  xTaskCreate(taskServoController, "ServoController", 128, NULL, 1, NULL);
}

void loop() {}


// ==================================================
// ===================== TASK =======================
// ==================================================

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
    for(int i = 0; i < 3; i++) Serial.print(buttonClickCounter[i]);
Serial.println(lcdState);
    switch(lcdState) 
    {
      case COUNTING:
        startLcdCounter();
        break;
      case CHANGING:
        blinkLcdTime();
        break;
      default:
        break;
    }
    vTaskDelay(1);
  }
}

/*
 * BUTTON USAGE
 */
void taskButtonController(void *pvParameter)
{
  while(true) 
  {
    button1.loop();
    button2.loop();
    button3.loop();
    // Button time counter/open/close choose
    if(button1.isPressed()) 
    {
      if(buttonClickCounter[0] == 0) 
      {
        vTaskSuspend(xTimeCounterHandle);
        lcdState = CHANGING;
      }
      if(++buttonClickCounter[0] > 3 || buttonClickCounter[1] > 0) 
      {
        updateEepromData(1);
        updateEepromData(2);
        vTaskResume(xTimeCounterHandle);
        lcdState = COUNTING;
        for(int i = 0; i < 3; i++) buttonClickCounter[i] = 0;
      }
    }
    // Button for hours/minutes choose
    if(button2.isPressed() && (buttonClickCounter[0] > 0))
    {
      if(++buttonClickCounter[1] > 2)
      {
        vTaskResume(xTimeCounterHandle);
        lcdState = COUNTING;
        for(int i = 0; i < 3; i++) buttonClickCounter[i] = 0;        
      }
    }
    // Button for hours/minutes increment
    if(button3.isPressed() && (buttonClickCounter[1] > 0))
    {
      switch(buttonClickCounter[0])
      {
        case 1:
          if(buttonClickCounter[1] == 1 && (++timeCounter[JAM] >= 24))
          {
            timeCounter[JAM] = 0;
          }   
          if(buttonClickCounter[1] == 2 && (++timeCounter[MENIT] >= 60)) 
          {
            timeCounter[MENIT] = 0;
          }      
          break;
        case 2:
          if(buttonClickCounter[1] == 1 && (++timeOpen[JAM] >= 24))
          {
            timeOpen[JAM] = 0;
          }   
          if(buttonClickCounter[1] == 2 && (++timeOpen[MENIT] >= 60)) 
          {
            timeOpen[MENIT] = 0;
          }      
          break;
        case 3:
          if(buttonClickCounter[1] == 1 && (++timeClose[JAM] >= 24))
          {
            timeClose[JAM] = 0;
          }   
          if(buttonClickCounter[1] == 2 && (++timeClose[MENIT] >= 60)) 
          {
            timeClose[MENIT] = 0;
          }      
          break;
        default:
          break;
      }
    }
  }
}

void taskServoController(void *pvParameter) 
{
  while(true)
  {
    if((timeCounter[JAM] == timeOpen[JAM]) && (timeCounter[MENIT] == timeOpen[MENIT]))
    {
      if(timeCounter[DETIK] == 0) servoBlinds.write(0);
    }
    else if((timeCounter[JAM] == timeClose[JAM]) && (timeCounter[MENIT] == timeClose[MENIT]))
    {
      if(timeCounter[DETIK] == 0) servoBlinds.write(90);
    }
    vTaskDelay(1);
  }
}

// ==================================================
// =================== FUNCTION =====================
// ==================================================

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
  switch(time) 
  {
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

void startLcdCounter()
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
}

void blinkLcdTime()
{
  if((millis() - blinkLast) > 500)
  {
    blinkLast = millis();
    blinkNow = !blinkNow;
  }
  switch(buttonClickCounter[0]) 
  {
    case 1:
      switch(buttonClickCounter[1]) 
      {
        case 1:
          // For the blinking JAM timeCounter data
          lcd.setCursor(5, 0);
          if(blinkNow) lcd.print("__");
          else lcd.print((timeCounter[JAM] > 9 ? String(timeCounter[JAM]) : "0" + String(timeCounter[JAM])));
          break;
        case 2:
          // For the blinking MENIT timeCounter data
          lcd.setCursor(8, 0);
          if(blinkNow) lcd.print("__");
          else lcd.print((timeCounter[MENIT] > 9 ? String(timeCounter[MENIT]) : "0" + String(timeCounter[MENIT])));
          break;
        default:
          // To Blink Full timeCounter data in LCD
          lcd.setCursor(5, 0);
          if(blinkNow) lcd.print("_____");
          else lcd.print((timeCounter[JAM] > 9 ? String(timeCounter[JAM]) : "0" + String(timeCounter[JAM])) + 
                        ":" + (timeCounter[MENIT] > 9 ? String(timeCounter[MENIT]) : "0" + String(timeCounter[MENIT])));
          break; 
      }      
      break;
    case 2:               
      switch(buttonClickCounter[1]) 
      {
        case 1:
          // For the blinking JAM timeOpen data
          lcd.setCursor(5, 1);
          if(blinkNow) lcd.print("__");
          else lcd.print((timeOpen[JAM] > 9 ? String(timeOpen[JAM]) : "0" + String(timeOpen[JAM])));
          break;
        case 2:
          // To keep showing JAM timeOpen data
          lcd.setCursor(5, 1);
          lcd.print((timeOpen[JAM] > 9 ? String(timeOpen[JAM]) : "0" + String(timeOpen[JAM])));          
          
          // For the blinking MENIT timeOpen data
          lcd.setCursor(8, 1);
          if(blinkNow) lcd.print("__");
          else lcd.print((timeOpen[MENIT] > 9 ? String(timeOpen[MENIT]) : "0" + String(timeOpen[MENIT])));
          break;
        default:
          // To blink all timeOpen data in LCD
          lcd.setCursor(5, 1);
          if(blinkNow) lcd.print("_____");
          else lcd.print((timeOpen[JAM] > 9 ? String(timeOpen[JAM]) : "0" + String(timeOpen[JAM])) + 
                        ":" + (timeOpen[MENIT] > 9 ? String(timeOpen[MENIT]) : "0" + String(timeOpen[MENIT])));
          break; 
      }    
      break;
    case 3:
      switch(buttonClickCounter[1]) 
      {
        case 1:
          lcd.setCursor(11, 1);
          if(blinkNow) lcd.print("__");
          else lcd.print((timeClose[JAM] > 9 ? String(timeClose[JAM]) : "0" + String(timeClose[JAM])));
          break;
        case 2:
          lcd.setCursor(14, 1);
          if(blinkNow) lcd.print("__");
          else lcd.print((timeClose[MENIT] > 9 ? String(timeClose[MENIT]) : "0" + String(timeClose[MENIT])));
          break;
        default:
          lcd.setCursor(11, 1);
          if(blinkNow) lcd.print("_____");
          else lcd.print((timeClose[JAM] > 9 ? String(timeClose[JAM]) : "0" + String(timeClose[JAM])) + 
                        ":" + (timeClose[MENIT] > 9 ? String(timeClose[MENIT]) : "0" + String(timeClose[MENIT])));
          break; 
      } 
      break;
    default:
      break;
  }
}