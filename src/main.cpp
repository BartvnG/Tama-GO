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
String items[] = {"bruh", "Yeah", "Aonther option", "Another option", "Yet another option", "the last option I swear"};
const int amountOfItems = sizeof(items) / sizeof(items[0]);
int menuIndex = 0;


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
SH1106Wire display(0x3c, SDA, SCL);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  for (int i = 0; i < amountOfButtons; i++) {
    pinMode(buttons[i], INPUT_PULLUP);
  }
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
}

void DrawMenu(String items[], int amountOfItems) {
  //fix for loop breaking
  display.clear();
  display.drawRect(0, 0, 120, 64);
  display.drawString(5, 0, "Bruh");
  display.drawString(5, 10, "Bruh");
  display.drawString(5, 20, "Bruh");
  // for(int i; i < 2; i++) {
  //   display.drawString(5, 10*i, "Bruh");
  // }
  // for (int i; i < amountOfItems; i++) {
  //   if (i != menuIndex) {
  //     display.drawString(5, 10*i, items[i]);
  //   }
  //   else {
  //     String selectedItem = items[i];
  //     selectedItem.concat("<");
  //     display.drawString(5, 10*i, selectedItem);
  //   }
  //   if (i == 4) {
  //     display.drawString(5, 50, "V");
  //     break;
  //   }
  // }
  display.display();
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
        menuIndex++;
        DrawMenu(items, amountOfItems);
        break;
      case 1:
        Serial.println("changed 2");
        break;
      case 2:
        Serial.println("changed 3");
        if (buttonStates[2] == HIGH) {
          Serial.println("button 3 on");
          DrawMenu(items, amountOfItems);
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
  
}
