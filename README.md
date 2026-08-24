# Fully-Autonomous-Battle-Bot (ESP32)

> An autonomous combat robotics control system built on the ESP32 utilizing dual-core processing and FreeRTOS for near zero-latency arena reaction times.

Developed for the **UBCO Battle Bots Club**.

## Core Architecture

This system avoids the standard bottlenecks of single-threaded microcontroller loops by splitting responsibilities across the ESP32's two cores.

* **Core 0 (The Vision Manifold):** Dedicated exclusively to polling four independent Adafruit VL53L0X Time-of-Flight (ToF) laser distance sensors via I2C. It filters out hardware noise and caches the raw spatial data.
* **Core 1 (The Combat Brain):** Runs the kinematic math and the Finite State Machine (FSM). It executes approximately every 20ms to guarantee instantaneous motor responses to new threats.
* **Mutex Memory Protection:** `SemaphoreHandle_t` locks the shared memory arrays. During data handoffs, Core 1 uses a hardware-level `memcpy()` to transfer sensor arrays in fractions of a millisecond, preventing data corruption without stalling Core 0.

## Kinematics & Data Processing

Instead of relying on raw distance, which can lead to false positives, this system uses first-principles kinematics to evaluate threats.

1. **Circular Ring Buffers:** Each sensor maintains a historical array of readings using modulo arithmetic `(head + 9) % 10` for fast, branchless lookbacks.
2. **Velocity Tracking:** The system calculates the approach velocity of targets in real-time (in mm/s) using the physical definition of velocity, giving the trade-off of a memory overload issue after approximately 4.6 hours of constant use (well beyond a 3 minute BattleBots match).
3. **Time-to-Collision (TTC):** By fusing distance and negative velocity arrays, the bot calculates the exact time until physical impact, allowing the drivetrain to react dynamically based on momentum rather than static distance.

## Combat Logic (Finite State Machine)

The robot's behavior is dictated by an egocentric, non-blocking Finite State Machine evaluated thousands of times per second. 

| State | Trigger Condition | Motor Action |
| :--- | :--- | :--- |
| **`SEARCH`** | Default state. All sensors read `> 1000mm`. | Pivot in place to sweep the arena with sensors. |
| **`ATTACK`** | Front target detected (`< 800mm`). | Full forward throttle to intercept target. |
| **`EVADE`** | Side sensors detect a `TTC < 1.0s`. | Hard reverse and pivot to dodge incoming flank attack. |
| **`BRACE`** | Point-blank impact detected on any side (`< 100mm`).| Max drivetrain torque & trigger weapon systems. |

## Hardware Requirements

* **Microcontroller:** ESP32 (Standard Dual-Core Module)
* **Sensors:** 4x Adafruit VL53L0X Time-of-Flight Sensors
* **Actuation:** Dual H-Bridge Motor Controllers (Code expansion required for specific PWM mapping)

### Pinout Configuration
* **XSHUT Pins (Sensor Boot Control):** `GPIO 15`, `GPIO 2`, `GPIO 4`, `GPIO 5`
* **I2C Pins:** Standard ESP32 SDA/SCL pins. 
*(Note: Sensors share a single I2C bus and are assigned unique hardware addresses `0x30` to `0x33` sequentially during boot).*

## Installation & Setup

1. Clone this repository to your local machine.
2. Open the project in the Arduino IDE or PlatformIO.
3. Install the required dependency: `Adafruit_VL53L0X` library.
4. Compile and upload to your ESP32 board. 

## Updates to Come in the Future
1. A continuous method of polling the Adafruit VL53L0X Time-of-Flight Sensors for faster reaction time.
2. Code to handle motor movement.
3. Further optimization of existing data structures.
4. An implementation of a Kalman filter for more accurate outputs. Particularly on estimated velocities for the bots own speed and direction paired with the existing Time-of-Flight Sensors.

## Author

**Connor Brake**  
*Electrical Engineering | University of British Columbia Okanagan (UBCO)*
