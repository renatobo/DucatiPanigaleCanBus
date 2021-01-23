//TODO appeanance: 1344-18 = 1362 sensor - multisensor https://specificationrefs.bluetooth.com/assigned-values/Appearance%20Values.pdf

#include <Arduino.h>
#include <TaskScheduler.h>

// Scheduler
Scheduler ts;

#ifdef ARDUINO_LOLIN_D32_PRO
#define LED_BUILTIN GPIO_NUM_5
#else
#define LED_BUILTIN GPIO_NUM_2
#endif

/* ----------- CANBUS definitions ------------- */
#include "CAN.h"
#define RX_TASK_PRIO 9
#define TX_GPIO_NUM GPIO_NUM_21
#define RX_GPIO_NUM GPIO_NUM_22
#define LOGTAG "CAN Listen Only"

#define ID_FRAME80 0x80
#define ID_FRAME100 0x100
#define ID_FRAME18 0x18

#define CANBUS_MSGFAST_SIZE 6
#define CANBUS_MSGSLOW_SIZE 3

uint32_t counter_18;
uint32_t counter_80;
uint32_t counter_100;


// rpm, wheelspeed, tps, gear
typedef struct
{
  uint16_t rpm;
  uint16_t rearwheelspeed;
  uint8_t aps;
  int8_t gear;
} canbus_msgfast_t;

//  enginetemperature, ambientemperature, battery
typedef struct
{
  uint8_t enginetemp;
  uint8_t ambientemp;
  uint8_t battery;
} canbus_msgslow_t;

canbus_msgfast_t message_fast;
canbus_msgslow_t message_slow;

/* --------------------- BLE Definitions ------------------ */
#include <NimBLEDevice.h>

// Random UUID's, matched in the LUA Script
#define BLE_SERVICE_UUID "6E400001-59f2-4a41-9acd-cd56fb435d64"
#define BLE_SLOW_CHARACTERISTIC_UUID "6E400011-59f2-4a41-9acd-cd56fb435d64"
#define BLE_FAST_CHARACTERISTIC_UUID "6E400012-59f2-4a41-9acd-cd56fb435d64"
#define BLE_DEVICE_ID "DuCan00"

#define BLE_FASTTASK_INTERVAL 50
#define BLE_SLOWTASK_INTERVAL 1000

String deviceid = BLE_DEVICE_ID;
static NimBLEServer *pServer;
NimBLECharacteristic *pFastCharacteristic = NULL;
NimBLECharacteristic *pSlowCharacteristic = NULL;
bool BLEdeviceConnected = false;

uint8_t ble_msg_fast[CANBUS_MSGFAST_SIZE];
uint8_t ble_msg_slow[CANBUS_MSGSLOW_SIZE];

// notify a fast frequency message
void handle_ble_notify_msg_fast()
{
  if (BLEdeviceConnected)
  {
    memcpy(ble_msg_fast, &message_fast.rpm, sizeof message_fast.rpm);
    memcpy(ble_msg_fast + sizeof message_fast.rpm, &message_fast.rearwheelspeed, sizeof message_fast.rearwheelspeed);
    memcpy(ble_msg_fast + sizeof message_fast.rpm + sizeof message_fast.rearwheelspeed, &message_fast.aps, sizeof message_fast.aps);
    memcpy(ble_msg_fast + sizeof message_fast.rpm + sizeof message_fast.rearwheelspeed + sizeof message_fast.aps, &message_fast.gear, sizeof message_fast.gear);

    pFastCharacteristic->setValue(ble_msg_fast, CANBUS_MSGFAST_SIZE);
    pFastCharacteristic->notify();
  }
}
Task tNotify_fast(BLE_FASTTASK_INTERVAL, -1, &handle_ble_notify_msg_fast, &ts, false);

// notify a slow frequency message
void handle_ble_notify_msg_slow()
{
  if (BLEdeviceConnected)
  {
    memcpy(ble_msg_slow, &message_slow.enginetemp, sizeof message_slow.enginetemp);
    memcpy(ble_msg_slow + sizeof message_slow.enginetemp, &message_slow.ambientemp, sizeof message_slow.ambientemp);
    memcpy(ble_msg_slow + sizeof message_slow.enginetemp + sizeof message_slow.ambientemp, &message_slow.battery, sizeof message_slow.battery);

    pSlowCharacteristic->setValue(ble_msg_slow, CANBUS_MSGSLOW_SIZE);
    pSlowCharacteristic->notify();
  }
}
Task tNotify_slow(BLE_SLOWTASK_INTERVAL, -1, &handle_ble_notify_msg_slow, &ts, false);

// BLE CallBacks
class MyServerCallbacks : public NimBLEServerCallbacks
{
  void onConnect(NimBLEServer *pServer)
  {
    BLEdeviceConnected = true;
    log_i("Client connected");
    // NimBLEDevice::startAdvertising(); 
  };

  void onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc)
  {
    BLEdeviceConnected = true;
    log_i("Client address: %s", NimBLEAddress(desc->peer_ota_addr).toString().c_str());
    // NimBLEDevice::stopAdvertising();
  }

  void onDisconnect(NimBLEServer *pServer)
  {
    BLEdeviceConnected = false;
    tNotify_fast.disable();
    tNotify_slow.disable();
    log_i("Client disconnected - start advertising");
    // NimBLEDevice::startAdvertising();
  }
};
class MyCharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
  void onSubscribe(NimBLECharacteristic *pCharacteristic, ble_gap_conn_desc *desc, uint16_t subValue)
  {
    String str = "Client ID: ";
    str += desc->conn_handle;
    str += " Address: ";
    str += std::string(NimBLEAddress(desc->peer_ota_addr)).c_str();
    if (subValue == 0)
    {
      str += " Unsubscribed to ";
    }
    else if (subValue == 1)
    {
      str += " Subscribed to notifications for ";
    }
    else if (subValue == 2)
    {
      str += " Subscribed to indications for ";
    }
    else if (subValue == 3)
    {
      str += " Subscribed to notifications and indications for ";
    }
    str += std::string(pCharacteristic->getUUID()).c_str();
    // Start notifications when a client subscribes

    // Initialize APS to 1
    uint8_t aps_init =1;
    memcpy(ble_msg_fast + sizeof message_fast.rpm + sizeof message_fast.rearwheelspeed, &aps_init, sizeof message_fast.aps);
    pFastCharacteristic->setValue(ble_msg_fast, CANBUS_MSGFAST_SIZE);
    pFastCharacteristic->notify();
    delay(5);
    aps_init=100;
    memcpy(ble_msg_fast + sizeof message_fast.rpm + sizeof message_fast.rearwheelspeed, &aps_init, sizeof message_fast.aps);
    pFastCharacteristic->setValue(ble_msg_fast, CANBUS_MSGFAST_SIZE);
    pFastCharacteristic->notify();
    delay(5);
    tNotify_fast.enable();
    tNotify_slow.enable();
  };
};
static MyCharacteristicCallbacks chrCallbacks;


/* --------------------- CanBus Functions ------------------ */
#if !defined(CAN_SIMULATOR_MODE)
void onReceive(int packetSize)
{
  // received a packet
  long identifier = CAN.packetId();
  uint8_t message[packetSize];

  if (identifier == ID_FRAME80)
  {
    counter_80++;
    CAN.readBytes(message, packetSize);
    message_fast.aps = message[4] / 2;
    message_fast.rpm = ((uint16_t)message[5]) * 256 + (uint16_t)message[6];
  }
  else if (identifier == ID_FRAME18)
  {
    counter_18++;
    CAN.readBytes(message, packetSize);
    message_fast.rearwheelspeed = (((uint16_t)(message[4] & 0b00011111)) << 4) + (message[5] >> 4);
    message_fast.gear = message[4] / 32;
  }
  else if (identifier == ID_FRAME100)
  {
    counter_100++;
    CAN.readBytes(message, packetSize);
    message_slow.enginetemp = message[3] - 40;
    message_slow.ambientemp = message[5] - 40;
    message_slow.battery = message[4];
  }
}
#endif // #if !defined(CAN_SIMULATOR_MODE)

// Pretty print a message
bool to_hex(char* dest, size_t dest_len, const uint8_t* values, size_t val_len) {
    if(dest_len < (val_len*2+1)) /* check that dest is large enough */
        return false;
    *dest = '\0'; /* in case val_len==0 */
    while(val_len--) {
        /* sprintf directly to where dest points */
        sprintf(dest, "%02X", *values);
        dest += 2;
        ++values;
    }
    return true;
}

bool ledon;
void reportcounters(){
  #if (CORE_DEBUG_LEVEL > 0)
  char slowmessagehex[sizeof(ble_msg_slow)*2+1];
  to_hex(slowmessagehex, sizeof(slowmessagehex), ble_msg_slow, sizeof(ble_msg_slow));
  char fastmessagehex[sizeof(ble_msg_fast)*2+1];
  to_hex(fastmessagehex, sizeof(fastmessagehex), ble_msg_fast, sizeof(ble_msg_fast));

  log_i( "Counters [18: %d, 80: %d, 100: %d], values RPM: %d, Speed: %d, APS: %d, Gear: %d, ETemp: %d, ATemp %d, Battery: %d. SLOW [%s], FAST [%s]",
  counter_18, counter_80, counter_100,
  message_fast.rpm, message_fast.rearwheelspeed, message_fast.aps, message_fast.gear,
  message_slow.enginetemp, message_slow.ambientemp, message_slow.battery,
  slowmessagehex, fastmessagehex);
  #endif
  ledon = !ledon;
  digitalWrite(LED_BUILTIN, (ledon ? HIGH : LOW)); // set pin to the opposite state
}
Task tReport(1000, -1, &reportcounters, &ts, true);

/* --------------------- Simulation Functions ------------------ */

#if defined(CAN_SIMULATOR_MODE)

// this goes from 0 to 100 then it resets
uint base_counter = 1;
const uint base_cycles = 1000;

void handle_dm_increase()
{
  // complete update in 100 cycles
  message_fast.rearwheelspeed = 5+base_counter*(300-5)/base_cycles;
  message_fast.rpm            = 2000+base_counter*(12000-2000)/base_cycles;
  message_fast.aps            = 1+base_counter*99/base_cycles;
  message_fast.gear           = 1+base_counter*5/base_cycles;
  message_slow.enginetemp     = 70+base_counter*(110-70)/base_cycles;
  message_slow.ambientemp     = 16+base_counter*(40-16)/base_cycles;
  message_slow.battery        = 120+base_counter*(140-120)/base_cycles;
  if (base_counter++>base_cycles)
    base_counter=0;
}
Task tIncreaseDM(100, -1, &handle_dm_increase, &ts, true);

#endif

/* --------------------- Setup Loop ------------------ */

void setup()
{
  // Create the BLE Device
  NimBLEDevice::init("DuCan00");
  // NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM);
  // Optional: set the transmit power, default is 3db
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);                             /** +9db */
  NimBLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_ADV);       /** +9db */
  NimBLEDevice::setPower(ESP_PWR_LVL_P9, ESP_BLE_PWR_TYPE_CONN_HDL0); /** +9db */
  NimBLEDevice::setSecurityIOCap((uint8_t)BLE_HS_IO_NO_INPUT_OUTPUT);
  //NimBLEDevice::setMTU(185);
  // ESP_LE_AUTH_NO_BOND
  NimBLEDevice::setSecurityAuth(false,false,false);
  // NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ (uint8_t)BLE_SM_PAIR_AUTHREQ_SC);

  // Create the BLE Server
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  pServer->advertiseOnDisconnect(true);

  // Create the BLE Service
  NimBLEService *pService = pServer->createService(BLE_SERVICE_UUID);

  // Create a BLE Characteristic - fast frequency messages
  pFastCharacteristic = pService->createCharacteristic(BLE_FAST_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY);
  pFastCharacteristic->setCallbacks(&chrCallbacks);

  // Create a BLE Characteristic - slow frequency messages
  pSlowCharacteristic = pService->createCharacteristic(BLE_SLOW_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY);
  pSlowCharacteristic->setCallbacks(&chrCallbacks);

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml

  // Start the service
  pService->start();

  // Start advertising
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  // define appearance, from https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.gap.appearance.xml
  pAdvertising->setAppearance(1344); // this might crash esp32 ble stack
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->setScanResponse(true);
  // pAdvertising->setScanResponse(false);
  // pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue -> find out more at https://github.com/h2zero/NimBLE-Arduino/issues/129
  // pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter

  pAdvertising->start();

  log_i( "Waiting a client connection to notify...");

  // initialize values

  gpio_reset_pin(LED_BUILTIN);
  /* Set the GPIO as a push/pull output */
  gpio_set_direction(LED_BUILTIN, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_BUILTIN, LOW);

  #if !defined(CAN_SIMULATOR_MODE)
  log_i( "Setting up CANBUS");
  CAN.setPins(RX_GPIO_NUM, TX_GPIO_NUM);
  CAN.observe();
  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3))
  {
    log_e("Starting CAN failed!");
    while (1)
      ;
  }
  CAN.onReceive(onReceive);
  #else
  log_i("Running in simulation of CAN messages");
  #endif // #if !defined(CAN_SIMULATOR_MODE)
  log_i( "Starting main loop routine");
  tReport.enable();
}

void loop()
{
  ts.execute();
}
