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
String status[] = {"Mood: 0/100", "Points: 143" };
String food[] = {"Appel x 4", "Peer x 5", "Burger x 2", "Salade x 5", "Frikandel x 6", "Kroket x 2", "Donut x 1" };
String drinks[] = {"Water x 8", "Fris x 3", "Limonade x 0", "Vitamine drink x 5",  "Thee x 2", "Koffie x 6"};
String settings[] = {"Volume: 5/10", "SEND DATA"};
String *menus[] = { status, food, drinks, settings };
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
int Steps = 0;

//Connection
//Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
SH1106Wire oled(0x3c, SDA, SCL);

const uint ServerPort = 23;
WiFiServer Server(ServerPort);
const char* ssid = "esp32_laptop";
const char* password = "E077p727";

//Communication
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
  // send the message back to client for debug
  client.println(message);
  int index;
  int value;
  
  // read all values into the character index array
  for(int i = 0; i < 5; i++) {
    // read the value that is between the "!"s into value
    index = message.indexOf("!");
    message = message.substring(index + 1, message.length());
    index = message.indexOf("!");
    value = message.substring(0, index).toInt();
    // store the value in the correct place
    characterIndexes[i] = value;
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
  // TinyWireM.begin();
  // TinyWireM.beginTransmission(mpu); 
  // TinyWireM.write(0x6B); //  Power setting address
  // TinyWireM.write(0b00000000); // Disable sleep mode (just in case)
  // TinyWireM.endTransmission();
  // TinyWireM.beginTransmission(mpu); 
  // TinyWireM.write(0x1B); // Config register for Gyro
  // TinyWireM.write(0x00000000); // 250° per second range (default)
  // TinyWireM.endTransmission();
  // TinyWireM.beginTransmission(mpu); //I2C address of the MPU
  // TinyWireM.write(0x1C); // Accelerometer config register
  // TinyWireM.write(0b00000000); // 2g range +/- (default)
  // TinyWireM.endTransmission();
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MPU6050 test!");

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
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  vector = sqrt( (a.acceleration.x * a.acceleration.x) + (a.acceleration.y * a.acceleration.y) + (a.acceleration.z * a.acceleration.z) );
  totalvector = vector - vectorprevious;
  // for debug purpose
  if (millis() - previousDebug >= 500) {
    Serial.println(Steps);
    previousDebug = millis();
  }
  if (totalvector > 6 && millis() - previousMeasure >= 1000){
    Steps++;
    previousMeasure = millis();
  }
  vectorprevious = vector;
}
