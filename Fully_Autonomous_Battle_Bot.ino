#include "Adafruit_VL53L0X.h"

// Mutex Key
SemaphoreHandle_t dataMutex;

const int pin1 = 15;
const int pin2 = 2;
const int pin3 = 4;
const int pin4 = 5;

Adafruit_VL53L0X sensor1 = Adafruit_VL53L0X();
Adafruit_VL53L0X sensor2 = Adafruit_VL53L0X();
Adafruit_VL53L0X sensor3 = Adafruit_VL53L0X();
Adafruit_VL53L0X sensor4 = Adafruit_VL53L0X();

// Sensor Variables
//float sensor1Distance = 0;
//float sensor2Distance = 0;
//float sensor3Distance = 0;
//float sensor4Distance = 0;

//Approximating Distances
float sensor1DistanceCollectionUnsorted[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
float sensor2DistanceCollectionUnsorted[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
float sensor3DistanceCollectionUnsorted[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
float sensor4DistanceCollectionUnsorted[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

int headSensor1 = 0;
int headSensor2 = 0;
int headSensor3 = 0;
int headSensor4 = 0;

float sensor1History[] = {0, 0};
float sensor2History[] = {0, 0};
float sensor3History[] = {0, 0};
float sensor4History[] = {0, 0};

enum BotState 
{
  SEARCH,
  ATTACK,
  EVADE,
  BRACE
};

BotState currentState = SEARCH;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  dataMutex = xSemaphoreCreateMutex();

  while (!Serial) {
    delay(1);
  }

  pinMode(pin1, OUTPUT);
  pinMode(pin2, OUTPUT);
  pinMode(pin3, OUTPUT);
  pinMode(pin4, OUTPUT);

  digitalWrite(pin1, LOW);
  digitalWrite(pin2, LOW);
  digitalWrite(pin3, LOW);
  digitalWrite(pin4, LOW);
  delay(10);

  digitalWrite(pin1, HIGH);
  sensor1.begin(0x30);
  delay(10);
  digitalWrite(pin2, HIGH);
  sensor2.begin(0x31);
  delay(50);
  digitalWrite(pin3, HIGH);
  sensor3.begin(0x32);
  delay(10);
  digitalWrite(pin4, HIGH);
  sensor4.begin(0x33);
  delay(10);

  // Pin the Manifold (Sensory (vision) Data) to Core 0
  xTaskCreatePinnedToCore(
    VisionTask,     // The function name
    "VisionCore",   // The task name
    10000,          // Stack size (memory allocated)
    NULL,           // Parameters
    1,              // Priority (1 is standard)
    NULL,           // Task handle
    0               // PIN TO CORE 0
  );
}

void VisionTask(void * pvParameters) {
  // FreeRTOS tasks must have an infinite loop
  for(;;) {
    VL53L0X_RangingMeasurementData_t measure1;
    VL53L0X_RangingMeasurementData_t measure2;
    VL53L0X_RangingMeasurementData_t measure3;
    VL53L0X_RangingMeasurementData_t measure4;
    
    // 1. Read the hardware (Slow - takes ~100ms for all 4)
    sensor1.rangingTest(&measure1, false);
    sensor2.rangingTest(&measure2, false);
    sensor3.rangingTest(&measure3, false);
    sensor4.rangingTest(&measure4, false);

    // Filter out out-of-range (4) and hardware errors (8191)
    float sensor1Distance = (measure1.RangeStatus != 4 && measure1.RangeMilliMeter != 8191) ? measure1.RangeMilliMeter : sensor1DistanceCollectionUnsorted[(headSensor1 + 9) % 10];
    float sensor2Distance = (measure2.RangeStatus != 4 && measure2.RangeMilliMeter != 8191) ? measure2.RangeMilliMeter : sensor2DistanceCollectionUnsorted[(headSensor2 + 9) % 10];
    float sensor3Distance = (measure3.RangeStatus != 4 && measure3.RangeMilliMeter != 8191) ? measure3.RangeMilliMeter : sensor3DistanceCollectionUnsorted[(headSensor3 + 9) % 10];
    float sensor4Distance = (measure4.RangeStatus != 4 && measure4.RangeMilliMeter != 8191) ? measure4.RangeMilliMeter : sensor4DistanceCollectionUnsorted[(headSensor4 + 9) % 10];

    // 2. REQUEST THE MUTEX KEY
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      // Door is locked! Core 1 cannot read while we write.
      assignValue(sensor1DistanceCollectionUnsorted, sensor1Distance, headSensor1);
      assignValue(sensor2DistanceCollectionUnsorted, sensor2Distance, headSensor2);
      assignValue(sensor3DistanceCollectionUnsorted, sensor3Distance, headSensor3);
      assignValue(sensor4DistanceCollectionUnsorted, sensor4Distance, headSensor4);
      
      // Unlock the door so Core 1 can use the data
      xSemaphoreGive(dataMutex);
    }

    // FreeRTOS requires a tiny delay to feed the Watchdog Timer, 
    // otherwise the core will panic thinking it's frozen.
    vTaskDelay(5 / portTICK_PERIOD_MS); 
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  float sensor1DistanceCollection[10];
  float sensor2DistanceCollection[10];
  float sensor3DistanceCollection[10];
  float sensor4DistanceCollection[10];
  // 2. REQUEST THE MUTEX KEY
  if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
    // Door is locked! Core 0 cannot overwrite data while we copy it.
    memcpy(sensor1DistanceCollection, sensor1DistanceCollectionUnsorted, sizeof(sensor1DistanceCollectionUnsorted));
    memcpy(sensor2DistanceCollection, sensor2DistanceCollectionUnsorted, sizeof(sensor2DistanceCollectionUnsorted));
    memcpy(sensor3DistanceCollection, sensor3DistanceCollectionUnsorted, sizeof(sensor3DistanceCollectionUnsorted));
    memcpy(sensor4DistanceCollection, sensor4DistanceCollectionUnsorted, sizeof(sensor4DistanceCollectionUnsorted));
    // Instantly return the key. (This copy takes less than 0.01 milliseconds)
    xSemaphoreGive(dataMutex);
  }
  insertionSort(sensor1DistanceCollection);
  insertionSort(sensor2DistanceCollection);
  insertionSort(sensor3DistanceCollection);
  insertionSort(sensor4DistanceCollection);
  //for (int i = 0; i < 10; i++)
  //{
    //Serial.print(sensor2DistanceCollectionUnsorted[i]);
    //Serial.print(", ");
  //}
  //Serial.println();
  //Serial.println("Sensor 1: " + String(truncateMean(sensor1DistanceCollection)));
  //Serial.println(truncateMean(sensor2DistanceCollection));
  //Serial.println("Sensor 3: " + String(truncateMean(sensor3DistanceCollection)));
  //Serial.println("Sensor 4: " + String(truncateMean(sensor4DistanceCollection)));

  float sensor1Distance = truncateMean(sensor1DistanceCollection);
  float sensor2Distance = truncateMean(sensor2DistanceCollection);
  float sensor3Distance = truncateMean(sensor3DistanceCollection);
  float sensor4Distance = truncateMean(sensor4DistanceCollection);

  float sensor1Velocity = determineVelocity(sensor1History, sensor1Distance);
  float sensor2Velocity = determineVelocity(sensor2History, sensor2Distance);
  float sensor3Velocity = determineVelocity(sensor3History, sensor3Distance);
  float sensor4Velocity = determineVelocity(sensor4History, sensor4Distance);

  float sensor1TTC = TTC(sensor1Distance, sensor1Velocity);
  float sensor2TTC = TTC(sensor2Distance, sensor2Velocity);
  float sensor3TTC = TTC(sensor3Distance, sensor3Velocity);
  float sensor4TTC = TTC(sensor4Distance, sensor4Velocity);
  //Serial.println(determineVelocity(sensor1History, sensor1Distance));
  //Serial.println(determineVelocity(sensor2History, sensor2Distance));
  //Serial.println(determineVelocity(sensor3History, sensor3Distance));
  //Serial.println(determineVelocity(sensor4History, sensor4Distance));
  //Serial.println(sensor3TTC);
  //delay(100);

  // FSM (Finite-State Machine)
  switch (currentState) {

    case SEARCH:
      // ACTION: Spin in place looking for a target
      //executeMotorCommand("SPIN_RIGHT");

      // TRANSITIONS: What causes us to leave this state?
      if (sensor2Distance < 800.0f) { 
        // We see something in front of us!
        currentState = ATTACK;
      } else if (sensor4TTC < 1.0f || sensor1TTC < 1.0f) {
        // Something is about to hit our side in less than 1 second!
        currentState = EVADE;
      }
      break;


    case ATTACK:
      // ACTION: Full throttle forward
      //executeMotorCommand("FORWARD_MAX");

      // TRANSITIONS:
      if (sensor1Distance > 1000.0f && sensor2Distance > 1000.0f && sensor3Distance > 1000.0f && sensor4Distance > 1000.0f) {
        // We lost the target, go back to searching
        currentState = SEARCH;
      } else if (sensor1Distance < 100.0f || sensor2Distance < 100.0f || sensor3Distance < 100.0f || sensor4Distance < 100.0f) {
        // We have made physical contact!
        currentState = BRACE;
      }
      break;


    case EVADE:
      // ACTION: Hard reverse to dodge the side attack
      //executeMotorCommand("REVERSE_FAST");

      // TRANSITIONS:
      if (sensor4TTC > 2.0f && sensor1TTC > 2.0f) {
        // The threat has passed or we backed away far enough
        currentState = SEARCH;
      }
      break;


    case BRACE:
      // ACTION: Pushing match! Max torque, fire weapons.
      //executeMotorCommand("WEAPON_FIRE");
      
      // TRANSITIONS:
      if (sensor2Distance > 200.0f) {
        // Opponent got away or was knocked back
        currentState = ATTACK;
      }
      break;
  }

  vTaskDelay(20 / portTICK_PERIOD_MS);
}

// Create an Array of Sensor Data for a Specific Sensor
void assignValue(float exArray[], float num, int& head)
{
  exArray[head] = num;
  if(head == 9)
  {
    head = 0;
  }
  else
  {
    head += 1;
  }
}

// Insertion Sort
void insertionSort(float exArray[])
{
  for(int i = 1; i < 10; i++)
  {
    float key = exArray[i];
    for(int j = i - 1; j >= 0; j--)
    {
      if(exArray[j] > key)
      {
        exArray[j + 1] = exArray[j];
        exArray[j] = key;
      }
      else
      {
        break;
      }
    }
  }
}

// Truncate and Mean Average a Sorted Array of Sensor Data
float truncateMean(float exArray[])
{
  float sum = 0;
  for (int i = 0; i < 6; i++)
  {
    sum += exArray[i + 2];
  }
  return sum * 0.1666667f;
}

// Approximate Velocity
float determineVelocity(float exArray[], float sensorDistance)
{
  unsigned long currentTime = millis();
  float velocity = 0;
  if (exArray[0] == 0) {
    exArray[0] = currentTime;
    exArray[1] = sensorDistance;
    return 0.0f; 
  }

  if(currentTime - exArray[0] != 0)
  {
    velocity = (sensorDistance - exArray[1]) / (currentTime - exArray[0]);
    exArray[0] = currentTime;
    exArray[1] = sensorDistance;
  }
  return velocity * 1000; 
}

// Time-to-Collision Approximation
float TTC(float sensorDistance, float sensorVelocity)
{
  if(sensorVelocity >= 0)
  {
    return 9999.9f;
  }
  return (sensorDistance/sensorVelocity) * -1;
}
