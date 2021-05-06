#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t testValueCounter = 0;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"



class MyServerCallbacks: public BLEServerCallbacks //client connected callbacks
{
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }    
};
class MyCallbacks: public BLECharacteristicCallbacks //data from client callbacks
{
    void onWrite(BLECharacteristic *pCharacteristic) 
    {
        Serial.println("*********");
        Serial.println("Received From Client: ");
        Serial.println(pCharacteristic->getValue().c_str());
        Serial.println("*********");
    }
};



void setup() {
  setCpuFrequencyMhz(80); //240 default, 160, 80, 40, 20, 10, below 80 and WiFi/Bluetooth won't work, lowering freq saves energy
  Serial.begin(115200);
  
  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks()); //sets up BLE server to notify/indicate to client and get confirmation back from client on connect/disconnect

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902()); //0x2902 enables receipt of confirmation of connect/disconnect from client

  pCharacteristic->setCallbacks(new MyCallbacks()); //Sets receive from client callbacks
 
  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting for a Client to connect...");
}



void loop() {
    // notify changed value
    if (deviceConnected) {
        Serial.print("Connected to Client, notify: "); //notify Client that data (typically new data) is available to be read
        Serial.println(testValueCounter);
        String StringToClient = "This is the Server, here is data: " + String(testValueCounter); //make string to send to Client
        pCharacteristic->setValue(StringToClient.c_str()); //pCharacteristic->setValue((uint8_t*)&testValueCounter, 4) //can use if just want to send the numeric value to the Client
        pCharacteristic->notify(); //send notification that value changed
        testValueCounter++;
        delay(5000); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("Disconnected from Client, start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        Serial.println("Connecting to Client.....");
        oldDeviceConnected = deviceConnected;
    }
}
