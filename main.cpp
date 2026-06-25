/**
 * ESP32-C6 Zigbee Keypad with MPR121 Touch Sensor
 * 
 * Based on Espressif Arduino-ESP32 Zigbee examples
 * Uses proper initialization sequence: Configure endpoints -> Add to Zigbee -> Begin
 */

 #include "esp32-hal-gpio.h"
#ifndef ZIGBEE_MODE_ED
 #error "Zigbee end device mode is not selected in Tools->Zigbee mode"
 #endif



#include "Zigbee.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <esp_sleep.h>
#include <nvs_flash.h>
#include <nvs.h>

 

 uint8_t button = BOOT_PIN;

 // Pin Configuration
 const int SDA_PIN = 18;
 const int SCL_PIN = 4;
 const int IRQ_PIN = 6;
 const int BATTERY_PIN = 2;
 const int BUZZER_PIN = 15;
 const int BOOT_BUTTON_PIN = 9;
 
 // MPR121 object
 Adafruit_MPR121 cap = Adafruit_MPR121();
 
 // Last touch state
 uint16_t lastTouchState = 0;
 
 // Keypad input string
 String inputString = "1";
 const int MAX_INPUT_LENGTH = 8;
 
 // Deep sleep configuration
 const unsigned long SLEEP_TIMEOUT_MS = 10000; // 20 seconds
 unsigned long lastActivityTime = 0;
 
 // Debounce configuration
 const unsigned long DEBOUNCE_DELAY_MS = 100;
 unsigned long lastKeyPressTime = 0;
 
// Buzzer control
const unsigned long BEEP_DURATION_MS = 20;
unsigned long beepEndTime = 0;
bool beepActive = false;

// Zero code timeout (send zero 2 seconds after entering a code)
const unsigned long ZERO_CODE_TIMEOUT_MS = 2000;
unsigned long enterPressTime = 0;
bool waitingForZeroSend = false;
 
 // Zigbee endpoints - MUST be created as global variables
 #define BATTERY_ENDPOINT 1
 #define INPUT_ENDPOINT 2
 
 ZigbeeAnalog zbAnalogDevice = ZigbeeAnalog(BATTERY_ENDPOINT);
 ZigbeeAnalog zbAnalogSOC = ZigbeeAnalog(INPUT_ENDPOINT);
 
 // Map channel to keypad character
 char getKeypadChar(int channel) {
   if (channel >= 0 && channel <= 8) {
     return '1' + channel; // '1' to '9'
   } else if (channel == 9) {
     return '0';
   } else if (channel == 10) {
     return 'C'; // Clear
   } else if (channel == 11) {
     return 'E'; // Enter
   }
   return '\0';
 }

  int get_battery_SOC()
  {
     // Sample battery voltage 10 times with 10ms delay and take average
     long batteryRawSum = 0;
     for (int i = 0; i < 10; i++) {
       batteryRawSum += analogRead(BATTERY_PIN);
       delay(10);
     }
     int batteryRaw = batteryRawSum / 10;
     
     float batteryVoltage = (batteryRaw / 4095.0) * 3.3 * 2.0 *1.28;
    float batterySOC=(batteryVoltage-3.2);
    if (batterySOC<0.0)
      batterySOC=0.0;
    if (batterySOC>1.0)
      batterySOC=1.0;
    batterySOC=batterySOC*100.0;

    Serial.print("Battery Voltage: ");
    Serial.print(batteryVoltage, 2);
    Serial.print("Battery %: ");
    Serial.println(batterySOC);


    zbAnalogDevice.setBatteryPercentage(int(batterySOC));
    zbAnalogDevice.setBatteryVoltage(int(batteryVoltage*10.0));

    zbAnalogDevice.reportBatteryPercentage();


    return(batterySOC);
 }
 
 void handleKeypadInput(int channel) {
   char key = getKeypadChar(channel);
   
   if (key == '\0') return;
   
  // Trigger beep
  beepActive = true;
  beepEndTime = millis() + BEEP_DURATION_MS;
  digitalWrite(BUZZER_PIN, HIGH);
   
   if (key == 'C') {
     inputString = "1";
     Serial.println("Input: []");
  } else if (key == 'E') {
    Serial.print("ENTER - Input: [");
    Serial.print(inputString);
    Serial.println("]");
    
    long inputValue = inputString.length() > 0 ? inputString.toInt() : 0;
    
    
    
    // Report to Zigbee
    zbAnalogDevice.setAnalogInput((float)inputValue);
    zbAnalogDevice.reportAnalogInput();
    zbAnalogSOC.setAnalogInput(get_battery_SOC());
    zbAnalogSOC.reportAnalogInput();
    
    

    
    // Start timer to send zero code after 2 seconds
    enterPressTime = millis();
    waitingForZeroSend = true;
    
    inputString = "1";
   } else {
     if (inputString.length() < MAX_INPUT_LENGTH) {
       inputString += key;
       Serial.print("Input: [");
       Serial.print(inputString);
       Serial.println("]");
     } else {
       Serial.println("Max 8 digits reached!");
     }
   }
 }
 
void enterDeepSleep() {
  Serial.println("Setting MPR121 to slowest update rate for low power...");
  cap.writeRegister(MPR121_CONFIG2, 0x3F);
   
   Serial.println("Entering deep sleep...");
   Serial.flush();
   
   digitalWrite(BUZZER_PIN, LOW);
   //gpio_pullup_en(GPIO_NUM_6);

  // Initialize pins
  pinMode(IRQ_PIN, INPUT);
  pinMode(BUZZER_PIN, INPUT);
  pinMode(BOOT_BUTTON_PIN, INPUT);
  pinMode(SDA_PIN, INPUT);
  pinMode(SCL_PIN, INPUT);
  pinMode(IRQ_PIN, INPUT);

   
   esp_deep_sleep_enable_gpio_wakeup(1ULL << IRQ_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
   esp_deep_sleep_start();
 }
 
void setup() {
  // WRITE_PERI_REG(CONFIG_ESP_BROWNOUT_DET, 0);  // REMOVED - causes crash!

  Serial.begin(115200);
   //while (!Serial && millis() < 3000);
   Serial.println("ESP32-C6 Zigbee Keypad");
   //delay(1000);
   
   // Initialize pins
   pinMode(IRQ_PIN, INPUT_PULLUP);
   pinMode(BATTERY_PIN, INPUT);
   pinMode(BUZZER_PIN, OUTPUT);
   digitalWrite(BUZZER_PIN, LOW);
   pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
   digitalWrite(BUZZER_PIN, HIGH);
   

  // Optional: set Zigbee device name and model
   zbAnalogDevice.setManufacturerAndModel("DrDoms", "KeyPad2");
   zbAnalogDevice.setPowerSource(ZB_POWER_SOURCE_BATTERY, 100, 42);

 
   // Set up analog input
   zbAnalogDevice.addAnalogInput();
   zbAnalogDevice.setAnalogInputApplication(ESP_ZB_ZCL_AI_COUNT_UNITLESS_OTHER);
   zbAnalogDevice.setAnalogInputDescription("Passcode");
   zbAnalogDevice.setAnalogInputResolution(1);
   // Set up analog input
  zbAnalogSOC.addAnalogInput();
  zbAnalogSOC.setAnalogInputApplication(ESP_ZB_ZCL_AI_PERCENTAGE_OTHER);
  zbAnalogSOC.setAnalogInputDescription("Battery SOC");
  zbAnalogSOC.setAnalogInputResolution(1);


 // Add endpoints to Zigbee Core
 Zigbee.addEndpoint(&zbAnalogDevice);
 Zigbee.addEndpoint(&zbAnalogSOC);

  // Initialize NVS for Zigbee data persistence
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  Serial.println("Starting Zigbee...");
  // When all EPs are registered, start Zigbee in End Device mode
  if (!Zigbee.begin(ZIGBEE_END_DEVICE)) {
     Serial.println("Zigbee failed to start!");
     Serial.println("Rebooting...");
     ESP.restart();
   } else {
     Serial.println("Zigbee started successfully!");
   }
   Serial.println("Connecting to network");
   while (!Zigbee.connected()) {
     Serial.print(".");
    
     delay(100);

     delay(100);
     

   }
   Serial.println("Connected");
   // Initialize I2C and MPR121
   Serial.println("Initializing MPR121...");
   Wire.begin(SDA_PIN, SCL_PIN);
   Wire.setClock(100000);
   delay(100);
   if (!cap.begin(0x5A)) {
     Serial.println("MPR121 not found!");
     ESP.restart();
     
   }
   
  cap.setAutoconfig(true);
  cap.setThresholds(8, 4);  // 12 touch, 6 release
  Serial.println("MPR121 initialized!");
   
   Serial.println("");
   Serial.println("=== Keypad Layout ===");
   Serial.println("Channels 0-8: Numbers 1-9");
   Serial.println("Channel 9: Number 0");
   Serial.println("Channel 10: Clear");
   Serial.println("Channel 11: Enter");
   Serial.println("Max 8 digits");
   Serial.println("Press BOOT for 3s to factory reset");
   Serial.println("====================");
   
   lastActivityTime = millis();

   digitalWrite(BUZZER_PIN, LOW);


}
 
 void loop() {
   uint16_t touchState = cap.touched();
   unsigned long currentTime = millis();
   
   // Check for new touches
   for (int i = 0; i < 12; i++) {
     uint16_t mask = 1 << i;
     bool currentlyTouched = touchState & mask;
     bool wasTouched = lastTouchState & mask;
     
     if (currentlyTouched && !wasTouched) {
       lastActivityTime = currentTime;
       
       if (currentTime - lastKeyPressTime > DEBOUNCE_DELAY_MS) {
         handleKeypadInput(i);
         lastKeyPressTime = currentTime;
       }
     }
   }
   
   lastTouchState = touchState;
   
  // Handle buzzer
  if (beepActive && millis() >= beepEndTime) {
    digitalWrite(BUZZER_PIN, LOW);
    beepActive = false;
  }
  
  // Check if we need to send zero code after timeout
  if (waitingForZeroSend && (millis() - enterPressTime >= ZERO_CODE_TIMEOUT_MS)) {
    Serial.println("Sending zero code...");
    zbAnalogDevice.setAnalogInput(0.0);
    zbAnalogDevice.reportAnalogInput();
    waitingForZeroSend = false;
  }
  
  // Checking button for factory reset and reporting
   if (digitalRead(button) == LOW) {  // Push button pressed
    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(button) == LOW) {
      delay(50);
      if ((millis() - startTime) > 3000) {
        // If key pressed for more than 3secs, factory reset Zigbee and reboot
        Serial.println("Resetting Zigbee to factory and rebooting in 1s.");
        delay(1000);
        Zigbee.factoryReset();
      }
    }

  }
   // Check for sleep timeout
   if (millis() - lastActivityTime > SLEEP_TIMEOUT_MS) {
     enterDeepSleep();
   }
   
   delay(10);
 }
 