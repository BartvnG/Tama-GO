#include <Arduino.h>
#include <SPI.h>
#include <bitMaps.h>
#include <SH1106Wire.h>
#include <WiFi.h>
#include <WiFiServer.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

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
String status[] = {"Happieness:", "Points:" };
String food[] = {"Appel", "Peer", "Burger", "Salade", "Frikandel", "Kroket", "Donut" };
String drinks[] = {"Water", "Fris", "Limonade", "Vitamine drink",  "Thee", "Koffie"};
String settings[] = {"SEND DATA"};
String *menus[] = { status, food, drinks, settings };
int statusAmount[] = {0, 143};
int foodAmount[] = {5, 0, 3, 4, 5, 9, 8};
int drinksAmount[] = {2, 0, 3, 8, 0, 9};
int settingsAmount[] = {5};
int *menuAmounts[] = {statusAmount, foodAmount, drinksAmount, settingsAmount};
int currentMenu = 4;
int menuIndex = 0;

//Personal
int characterIndex = 0;
int hatIndex = 0;
int faceIndex = 0;
int shirtIndex = 0;
int pantsIndex = 0;
int characterIndexes[] = { characterIndex, hatIndex, faceIndex, shirtIndex, pantsIndex};
static unsigned char **bits[] = {characterBits, hatBits, faceBits, shirtBits, pantsBits};
//mpu
Adafruit_MPU6050 mpu;
int accelX, accelY, accelZ;
unsigned long previousMeasure;
unsigned long previousDebug;
float vectorprevious;
float vector;
float totalvector;

//Connection
//Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
SH1106Wire oled(0x3c, SDA, SCL);

const uint ServerPort = 23;
WiFiServer Server(ServerPort);
const char* ssid = "esp32_laptop";
const char* password = "E077p727";

//Communication
int typeOfMessage;
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

void ExtractData(String message, int arr[], int arrSize) {
  int index;
  int val;
  // read all values into the character index array
  for(int i = 0; i < arrSize; i++) {
    // read the value that is between the "!"s into value
    index = message.indexOf("!");
    message = message.substring(index + 1, message.length());
    index = message.indexOf("!");
    val = message.substring(0, index).toInt();
    // store the value in the correct place
    arr[i] = val;
  }
}

void HandleMessage(String message) {
  // send the message back to client for debug
  client.println(message);
  Serial.println(message);
  typeOfMessage = message.substring(0, 1).toInt();
  switch (typeOfMessage)
  {
  case 0:
    ExtractData(message, characterIndexes, sizeof(characterIndexes)/sizeof(characterIndexes[1]));
    break;
  case 1:
    ExtractData(message, statusAmount, sizeof(statusAmount)/sizeof(statusAmount[1]));
    break;
  case 2:
    ExtractData(message, foodAmount, sizeof(foodAmount)/sizeof(foodAmount[1]));
  case 3:
    ExtractData(message, drinksAmount, sizeof(drinksAmount)/sizeof(drinksAmount[1]));
  default:
    break;
  }
}

void ListenToClient() {
  if(client.connected() > 0) {
      // add the client input to string while its sending something
      while (client.available() > 0) {
        char a = client.read();
        clientMessage += a;
        }
        // remove the control char at the end
        clientMessage = clientMessage.substring(0, clientMessage.length()-1);
        // if input is a protocol, handle the message inside
        if (clientMessage.startsWith("#") && clientMessage.endsWith("%")) {
        HandleMessage(clientMessage.substring(1, clientMessage.length()-1));
        Serial.println(clientMessage);
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
    String stringToDraw = menus[menu][i];
    if (menu != 0 && menu != 3) {
      stringToDraw += " x";
    }
    stringToDraw += " ";
    if (menu != 3) {
      stringToDraw += menuAmounts[menu][i];
    }
    if (elementsDrawn != 0) {
      oled.drawString(5, 10*elementsDrawn, stringToDraw);
    }
    else {
      if (menu != 0) {
        stringToDraw += "<";
      }
      else {
        stringToDraw += "/100";
      }
      oled.drawString(5, 10*elementsDrawn, stringToDraw);
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
  if (currentMenu != 4 && currentMenu != 0) {
    if (menuAmounts[currentMenu][menuIndex]-1 >= 0) {
    menuAmounts[currentMenu][menuIndex]--;
    }
    DrawMenu(currentMenu);
  }
  else if (currentMenu == 3) {
    // Send data to software after checking connection
  }
}

void DrawCharacter() {
  oled.clear();
  for (int i = 0; i < 5; i++) {
    oled.drawXbm(0, 0, 128, 64, bits[i][characterIndexes[i]]);
  }
  oled.display();
}

void MesurePoints() {
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  vector = sqrt( (a.acceleration.x * a.acceleration.x) + (a.acceleration.y * a.acceleration.y) + (a.acceleration.z * a.acceleration.z) );
  totalvector = vector - vectorprevious;
  if (totalvector > 4 && millis() - previousMeasure >= 1000){
    statusAmount[1]++;
    previousMeasure = millis();
    DrawMenu(currentMenu);
  }
  vectorprevious = vector;
}

void GetData() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Server.begin();
  Serial.println(WiFi.localIP());
  while (!client.connected()) {
    CheckForConnections();
  }
  // read client stuff untill everything is in
  while (typeOfMessage < 3) {
    ListenToClient();
  }
}

void setup() {
  //put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  for (int i = 0; i < amountOfButtons; i++) {
    pinMode(buttons[i], INPUT_PULLUP);
  }

  Serial.println("");
  delay(100);

  oled.init();
  oled.flipScreenVertically();
  oled.setFont(ArialMT_Plain_10);
  oled.clear();
  oled.drawXbm(0, 16, 128, 30, logo);
  oled.display();

  GetData();
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
          if (currentMenu != 0) {
            menuIndex++;
            DrawMenu(currentMenu);
          }
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
  // check for incoming tcp client connections:
  CheckForConnections();

  for (int i = 0; i < amountOfButtons; i++) {
    ButtonLogic(i);
  }

  // listen to input from a client
  ListenToClient();

  if (currentMenu == 4) {
    DrawCharacter();
  }

  MesurePoints();
}
