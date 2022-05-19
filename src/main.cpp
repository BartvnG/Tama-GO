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
String items[] = {"1", "2", "3", "4", "5", "6", "7", "8"};
const int amountOfItems = sizeof(items) / sizeof(items[0]);
int menuIndex = 0;
int menuPage = 0;


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

// void DrawMenu(String items[], int amountOfItems) {
//   display.clear();
//   display.drawRect(0, 0, 120, 64);
//   Serial.println(menuIndex);
//   if (menuIndex == amountOfItems) {
//     menuIndex = 0;
//   }
//   int i = menuIndex;
//   int elementsDrawn = 0;
//   while (i < amountOfItems) {
//       if (elementsDrawn != 0) {
//         display.drawString(5, 10*elementsDrawn, items[i]);
//       }
//       else {
//         String selectedItem = items[i];
//         selectedItem.concat("<");
//         display.drawString(5, 10*elementsDrawn, selectedItem);
//       }
//       elementsDrawn++;
//       if (elementsDrawn == 5) {
//         display.drawString(5, 50, "V");  
//         break;
//       }
//       i++;
//     }
    
//     display.display();
// }


void DrawMenu(String items[], int amountOfItems) {
  //fix for loop breaking
  display.clear();
  display.drawRect(0, 0, 120, 64);
  
  // int itemsOnSchreen = (amountOfItems - (5 * menuPage + 1) < 5) ? amountOfItems % 5 : 5;
  // int pagesNeeded = (amountOfItems / 5 == 0) ? amountOfItems / 5 - 1: amountOfItems / 5;
  int pagesNeeded = (amountOfItems / 3) + 1;
  if (amountOfItems % 3 == 0) {
    pagesNeeded = amountOfItems / 3;
  }
  // int itemsOnSchreen = (menuPage == pagesNeeded) ? amountOfItems % 5 : 5; 
  int itemsOnSchreen = 3;
  if (amountOfItems - (3 * menuPage) < 3) {
    itemsOnSchreen = amountOfItems % 3;
  }

  if (menuIndex >= itemsOnSchreen) {
    menuIndex = 0;
    menuPage++;
  }
  
  if (menuPage == pagesNeeded) {
      menuPage = 0;
  }
  else {
    display.drawString(5, 50, "V");
  }
  Serial.println();
  Serial.print("cal:");
  Serial.print(amountOfItems - (3 *menuPage));
  Serial.print(" ");
  Serial.print(amountOfItems / 3);
  Serial.print(" ");
  Serial.println(amountOfItems % 3);
  Serial.print("var:");
  Serial.print(itemsOnSchreen);
  Serial.print(" ");
  Serial.println(menuIndex);
  Serial.print(" ");
  Serial.print(pagesNeeded);
  Serial.print(" ");
  Serial.print(menuPage);
  Serial.println();
  int i = 0;
  int elementsDrawn = 0;
  while (i < itemsOnSchreen) {
    if (elementsDrawn != menuIndex) {
      display.drawString(5, 10*elementsDrawn, items[i+3*menuPage]);
    }
    else {
      String selectedItem = items[i+3*menuPage];
      selectedItem.concat("<");
      display.drawString(5, 10*i, selectedItem);
    }
    elementsDrawn++;
    i++;
  }
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
        break;
      case 1:
        Serial.println("changed 2");
        if (buttonStates[1] == HIGH) {
          menuIndex++;
          DrawMenu(items, amountOfItems);
        }
        break;
      case 2:
        // Serial.println("changed 3");
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
  // Serial.println("main loop active");
}
