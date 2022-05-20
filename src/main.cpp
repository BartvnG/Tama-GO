#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <bitMaps.h>
#include <SH1106Wire.h>

//Variables
//Button variables
#define button1 14
#define button2 12
#define button3 13
int buttons[] = {button1, button2, button3};
const int amountOfButtons = sizeof(buttons)/sizeof(buttons[0]);
int buttonReading[amountOfButtons];
int buttonStates[amountOfButtons];
int lastButtonStates[amountOfButtons];
int lastButtonDebounceTimes[amountOfButtons];
unsigned long buttonDebounceDelay = 50;

//Menu
String status[] = {"Mood: Happy", "Hunger: 3/10", "Thirst: 2/10", "Steps: 143" };
String food[] = {"Appel x4", "Peer x5", "Burger x2", "Salade x5", "Frikandel x6", "Kroket x2", "Donut x1" };
String drinks[] = {"Water x8", "Fris x3", "Ranja x 0", "Vitamine drink x5" };
String settings[] = {"Volume: 5/10", "EXIT"};
String *menus[] = { status, food, drinks, settings };
int currentMenu = 4;
int menuIndex = 0;


//Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
SH1106Wire oled(0x3c, SDA, SCL);

void setup() {
  //put your setup code here, to run once:
  Serial.begin(9600);
  for (int i = 0; i < amountOfButtons; i++) {
    pinMode(buttons[i], INPUT_PULLUP);
  }
  oled.init();
  oled.flipScreenVertically();
  oled.setFont(ArialMT_Plain_10);
  oled.clear();
  oled.drawXbm(0, 0, 128, 64, logo);
  oled.display();
}

void DrawMenu(int menu) {
  int amountOfItems = 0;
  switch (menu)
  {
  case 0:
    amountOfItems = sizeof(status)/sizeof(String);
    break;
  case 1:
    amountOfItems = sizeof(food)/sizeof(String);
    break;
  case 2:
    amountOfItems = sizeof(drinks)/sizeof(String);
    break;
  case 3:
    amountOfItems = sizeof(settings)/sizeof(String);
    break;
  default:
    break;
  }
  oled.clear();
  oled.drawRect(0, 0, 120, 64);
  if (menuIndex == amountOfItems) {
    menuIndex = 0;
  }
  int i = menuIndex;
  int elementsDrawn = 0;
  while (i < amountOfItems) {
      if (elementsDrawn != 0) {
        oled.drawString(5, 10*elementsDrawn, menus[menu][i]);
      }
      else {
        String selectedItem = menus[menu][i];
        selectedItem.concat("<");
        oled.drawString(5, 10*elementsDrawn, selectedItem);
      }
      elementsDrawn++;
      if (elementsDrawn > 4) {
        oled.drawString(5, 50, "V");  
        break;
      }
      i++;
    }
    oled.display();
}

void SelectFromMenu() {
  if (currentMenu != 4) {
    oled.clear();
    oled.drawString(0, 0, menus[currentMenu][menuIndex]);
    oled.display();
  }
}

void ButtonLogic(int assignedButton) {
  //Read button value
  buttonReading[assignedButton] = digitalRead(buttons[assignedButton]);

  //Reset debounce timer if state has changed
  if (buttonReading[assignedButton] != lastButtonStates[assignedButton]) {
    lastButtonDebounceTimes[assignedButton] = millis();
  }

  //If the debounce time has passed, execute button logic
  if ((millis() - lastButtonDebounceTimes[assignedButton]) > buttonDebounceDelay) {
    if (buttonReading[assignedButton] != buttonStates[assignedButton]) {
      buttonStates[assignedButton] = buttonReading[assignedButton];
      //Code for button one
      switch (assignedButton)
      {
      case 0:
        Serial.println("changed 1");
        SelectFromMenu();
        break;
      case 1:
        if (buttonStates[1] == HIGH) {
          Serial.println("button 2 on");
          menuIndex++;
          DrawMenu(currentMenu);
        }
        break;
      case 2:
        // Serial.println("changed 3");
        if (buttonStates[2] == HIGH) {
          Serial.println("button 3 on");
          menuIndex = 0;
          currentMenu++;
          if (currentMenu > 4) {
            currentMenu = 0;
          }
          DrawMenu(currentMenu);
        }
        break;
      default:
        break;
      }
    }
  }
  //Update the button state
  lastButtonStates[assignedButton] = buttonReading[assignedButton];
}

void loop() {
  // put your main code here, to run repeatedly:
  for (int i = 0; i < amountOfButtons; i++)
  {
    ButtonLogic(i);
  }
  if (millis() >= 1000) {
    oled.clear();
  }
  // Serial.println("main loop active");
}
