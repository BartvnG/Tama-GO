#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <bitMaps.h>
#include <SH1106Wire.h>
#include <WiFi.h>
#include <WiFiServer.h>

//Variables
//Button variables
#define button1 14
#define button2 12
#define button3 13
int buttons[] = {button1, button2, button3};
const int amountOfButtons = sizeof(buttons)/sizeof(buttons[0]);
int buttonReading;
int buttonStates[amountOfButtons];
int lastButtonStates[amountOfButtons];
int lastButtonDebounceTimes[amountOfButtons];
unsigned long buttonDebounceDelay = 50;

//Menu
String status[] = {"Mood: Happy", "Hunger: 3/10", "Thirst: 2/10", "Steps: 143" };
String food[] = {"Appel x 4", "Peer x 5", "Borgir x 2", "Salade x 5", "Frikandel x 6", "Kroket x 2", "Donut x 1" };
String drinks[] = {"Water x 8", "Fris x 3", "Ranja x 0", "Vitamine drink x 5" };
String settings[] = {"Volume: 5/10", "RESTART"};
String *menus[] = { status, food, drinks, settings };
int currentMenu = 4;
int menuIndex = 0;

//Character
int characterIndex = 0;
int hatIndex = 1;
int faceIndex = 0;
int shirtIndex = 0;
int pantsIndex = 0;
int characterIndexes[] = { characterIndex, hatIndex, faceIndex, shirtIndex, pantsIndex};
static unsigned char **bits[] = {characterBits, hatBits, faceBits, shirtBits, pantsBits};


//Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
SH1106Wire oled(0x3c, SDA, SCL);

const uint ServerPort = 23;
WiFiServer Server(ServerPort);
const char* ssid = "esp32_laptop";
const char* password = "E077p727";

String clientMessage = "";
const char startChar = '#';
const char endChar = '%';
bool communicationStarted = false;

WiFiClient client;
void CheckForConnections()
{
  if (Server.hasClient())
  {
    // If we are already connected to another computer,
    // then reject the new connection. Otherwise accept
    // the connection.
    if (client.connected())
    {
      Serial.println("Connection rejected");
      Server.available().stop();
    }
    else
    {
      Serial.println("Connection accepted");
      client = Server.available();
    }
  }
}

void HandleMessage(String message) {
  client.println(message);
  int index;
  int value;
  
  for(int i = 0; i < 5; i++) {
    index = message.indexOf("!");
    message = message.substring(index + 1, message.length());
    index = message.indexOf("!");
    value = message.substring(0, index).toInt();
    characterIndexes[i] = value;
  }
}

void ListenToClient() {
  if(client.connected() > 0) {
      while (client.available() > 0) {
        char a = client.read();
        clientMessage += a;
        }
        clientMessage = clientMessage.substring(0, clientMessage.length()-1);
        if (clientMessage.startsWith("#") && clientMessage.endsWith("%")) {
        HandleMessage(clientMessage.substring(1, clientMessage.length()-1));
      }
  }
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
    String selectedItem = menus[currentMenu][menuIndex];
    int newNumber = selectedItem.substring(selectedItem.length()-1).toInt()-1;
    selectedItem = selectedItem.substring(0, selectedItem.length()-1);
    selectedItem += newNumber;
    if (newNumber >= 0) {
      menus[currentMenu][menuIndex] = selectedItem;
    }
    DrawMenu(currentMenu);
  }
}

void DrawCharacter() {
  oled.clear();
  for (int i = 0; i < 5; i++) {
    oled.drawXbm(0, 0, 128, 64, bits[i][characterIndexes[i]]);
  }
  oled.display();
}

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
  oled.drawXbm(0, 16, 128, 30, logo);
  oled.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Server.begin();
  Serial.println(WiFi.localIP());
  DrawCharacter();
}

void ButtonLogic(int assignedButton) {
  //Read button value
  buttonReading = digitalRead(buttons[assignedButton]);

  //Reset debounce timer if state has changed
  if (buttonReading != lastButtonStates[assignedButton]) {
    lastButtonDebounceTimes[assignedButton] = millis();
  }

  //If the debounce time has passed, execute button logic
  if ((millis() - lastButtonDebounceTimes[assignedButton]) > buttonDebounceDelay) {
    if (buttonReading != buttonStates[assignedButton]) {
      buttonStates[assignedButton] = buttonReading;
      switch (assignedButton)
      {
      case 0:
        if (buttonStates[0] == HIGH) {
          Serial.println("button 1 on");
          SelectFromMenu();
        }
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
  lastButtonStates[assignedButton] = buttonReading;
}

void loop() {
  // put your main code here, to run repeatedly:
  CheckForConnections();
  for (int i = 0; i < amountOfButtons; i++)
  {
    ButtonLogic(i);
  }
  ListenToClient();
  if (currentMenu == 4) {
    DrawCharacter();
  }
}
