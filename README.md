# AI-Powered Energy Optimization Platform for Smart Homes

> An integrated smart-energy platform combining **Smart Energy Metering, IoT-Based Home Automation, Hybrid Solar–Battery–Grid Energy Management, ML-Based Source Optimization, and Priority-Based Load Management**.

---

## Project Overview

The **AI-Powered Energy Optimization Platform for Smart Homes** is developed to improve the way residential electricity is **measured, controlled, and optimized**.

The project combines three major systems in one platform:

1. **Smart Energy Meter**
2. **IoT-Based Home Automation**
3. **Hybrid Energy Management System**

The platform monitors electricity consumption in real time, allows users to control household loads remotely and physically, manages multiple energy sources, and optimizes the use of available energy.

A key part of the project is the combination of **source optimization** and **load optimization**. Solar PV, Battery, and Grid are considered as available energy sources, while household loads are prioritized according to their importance.


## Project Objectives

The main objectives of this project are to:

- Monitor household electrical parameters in real time.
- Display voltage, current, power, energy consumption, and estimated electricity cost.
- Allow users to control household appliances remotely.
- Keep both physical and web-based appliance control working together.
- Manage **Solar PV, Battery, and Grid** as hybrid energy sources.
- Use an **ML-based approach for source optimization**.
- Prioritize household loads according to their importance.
- Reduce unnecessary energy consumption.
- Increase the utilization of renewable energy.
- Reduce unnecessary dependency on the utility grid.
- Develop a scalable system that can later be converted into a compact commercial product.

---

# System Architecture

The complete platform is divided into three interconnected functional systems.

```text
                    ┌──────────────────────────┐
                    │      Web Application     │
                    │ Monitoring + Control     │
                    └────────────┬─────────────┘
                                 │
                                 │
             ┌───────────────────┼────────────────────┐
             │                   │                    │
             ▼                   ▼                    ▼
 ┌───────────────────┐  ┌──────────────────┐  ┌──────────────────────┐
 │ Smart Energy      │  │ Home Automation  │  │ Hybrid Energy        │
 │ Meter             │  │ System           │  │ Management System    │
 └─────────┬─────────┘  └────────┬─────────┘  └──────────┬───────────┘
           │                     │                       │
     Voltage/Current       Appliance Control        Source + Load
     Power/Energy          Physical + Web           Optimization
                                                     │
                                           ┌─────────┼─────────┐
                                           ▼         ▼         ▼
                                         Solar    Battery     Grid
```

---

# 1. Smart Energy Meter

The Smart Energy Meter is responsible for measuring and displaying the electrical parameters of the connected load.

## Main Functions

The system measures:

- **Voltage**
- **Current**
- **Power**
- **Accumulated Energy Consumption**
- **Estimated Electricity Cost**

The measured values are displayed locally on an LCD and are also sent to the web platform for monitoring.

## Hardware Used

| Component | Purpose |
|---|---|
| ESP32 | Main controller and Wi-Fi communication |
| ZMPT101B | AC voltage measurement |
| ACS712 | AC current measurement |
| 16×2 I2C LCD | Local display |
| Power Supply | Powers the controller and sensors |

## Basic Working Process

```text
AC Supply
   │
   ├── ZMPT101B ──> Voltage
   │
   └── ACS712 ────> Current
                         │
                         ▼
                       ESP32
                         │
             ┌───────────┴───────────┐
             ▼                       ▼
          LCD Display           Web Backend
                                     │
                                     ▼
                               User Dashboard
```

The ESP32 continuously reads the voltage and current sensors. From these measurements, the system calculates power, energy consumption, and estimated electricity cost.

The accumulated energy value is stored so that it is not immediately lost when the device restarts.

---

# 2. IoT-Based Home Automation

The Home Automation section allows household electrical appliances to be controlled through both **physical buttons** and the **web application**.

## Main Functions

- Remote appliance ON/OFF control
- Physical push-button control
- Real-time appliance status synchronization
- ESP32-based Wi-Fi communication
- Relay-based load switching

## Working Principle

```text
                    Web Application
                           │
                           ▼
                       Backend API
                           │
                           ▼
                         ESP32
                       /       \
                      /         \
             Physical Button   Relay Module
                                  │
                                  ▼
                           Household Load
```

If a user changes the state of an appliance from the web application, the command is sent to the ESP32 and the corresponding relay is switched.

Similarly, if the user presses a physical button, the appliance state changes locally and the updated status is synchronized with the web system.

This allows the physical control system and the online control system to operate together.

## Purpose

The system helps reduce unnecessary energy consumption caused by appliances being left ON unintentionally.

For example, if a user leaves home and later realizes that a light or fan is still running, the appliance can be turned OFF remotely.

---

# 3. Hybrid Energy Management System

The Hybrid Energy Management System is responsible for managing multiple available energy sources and optimizing how energy is supplied to household loads.

## Available Energy Sources

The current system considers three sources:

```text
Solar PV
Battery
Grid
```

The hybrid system performs two major optimization tasks:

1. **Source Optimization**
2. **Load Optimization**

---

## Source Optimization

The source-management section is designed to use an **ML model** together with real-time and historical data to determine the most suitable source for supplying power.

Possible input information includes:

- Solar availability
- Solar generation pattern
- Battery voltage / battery condition
- Battery current
- Grid availability
- Current household demand
- Historical energy consumption
- Time of day
- Previous source-usage data

Conceptually:

```text
Solar Data ────────────┐
Battery Data ──────────┤
Grid Status ───────────┤
Load Demand ───────────┼──> ML-Based Decision Model
Historical Data ───────┤              │
Time Information ──────┘              ▼
                             Recommended Source
                                      │
                         ┌────────────┼────────────┐
                         ▼            ▼            ▼
                       Solar       Battery        Grid
```

The objective is to increase the use of renewable energy, reduce unnecessary grid dependency, and maintain a reliable power supply.

---

## Load Optimization and Load Prioritization

Source optimization alone is not enough. The platform also manages the demand side through **load prioritization**.

Household loads can be divided into different priority levels.

| Priority | Example |
|---|---|
| **Critical** | Essential lighting, router, emergency devices |
| **Important** | Fan, computer, selected household devices |
| **Non-Critical** | Loads that can temporarily be disconnected |

When sufficient energy is available, all permitted loads can operate normally.

When available energy becomes limited, the system can maintain higher-priority loads while reducing or disconnecting lower-priority loads.

```text
Available Energy
       │
       ▼
Load Priority Controller
       │
       ├── Critical Load       → Highest Priority
       │
       ├── Important Load      → Medium Priority
       │
       └── Non-Critical Load   → Lowest Priority
```

By combining **ML-based source selection** with **priority-based load management**, the system can optimize both the supply side and the demand side.

---

# Web Application

A web-based platform is used to provide a common interface for monitoring and controlling the complete system.

The dashboard is designed to provide information such as:

- Real-time voltage
- Current
- Power
- Energy consumption
- Estimated electricity cost
- Appliance status
- Appliance ON/OFF control
- Hybrid energy-source information
- Energy-management information

---

# Technologies Used

| Category | Technology |
|---|---|
| Microcontroller | ESP32 |
| Embedded Development | Arduino IDE / C++ |
| Communication | Wi-Fi |
| Data Communication | HTTP / REST API |
| Energy Measurement | ZMPT101B + ACS712 |
| Display | 16×2 I2C LCD |
| Load Control | Relay Modules |
| User Interface | Web Application |
| Intelligence | Machine Learning |
| Energy Sources | Solar PV, Battery, Grid |
| Optimization | Source Selection + Load Prioritization |

---

# Current Prototype

The current prototype demonstrates the major functional sections of the proposed platform.

### Smart Energy Meter
The energy meter provides real-time measurement of household electrical parameters and sends the information to the web platform.

### Home Automation
Connected loads can be operated using both the web interface and physical controls.

### Hybrid Energy Management
The hybrid system manages Solar PV, Battery, and Grid sources and combines source management with priority-based load control.

The project is currently implemented as a prototype using separate hardware sections. The next stage is to combine these sections into a more compact and product-oriented design.


# Prototype Images

Add your project photographs in an `images` folder and replace the example paths below.

```markdown
![Complete Prototype](images/complete-prototype.jpg)
![Smart Energy Meter](images/smart-energy-meter.jpg)
![Home Automation](images/home-automation.jpg)
![Hybrid Energy Management](images/hybrid-energy-management.jpg)
![Web Dashboard](images/web-dashboard.jpg)
```

# Expected Impact

The project aims to contribute to:

- Reduced unnecessary household electricity consumption.
- Better awareness of real-time energy usage.
- Improved utilization of solar energy.
- Better management of battery-stored energy.
- Reduced unnecessary grid dependency.
- Smarter control of household appliances.
- Better utilization of limited available energy through load prioritization.
- Development of intelligent residential energy-management solutions.


# Team

**Project Name:** AI-Powered Energy Optimization Platform for Smart Homes

**Team Members:**  
`[MD. Rased Khan]`  
`[Md. Sultanul Arefin Akil]`  
`[Md. Mostafizur Rahaman]`
`[Ridowanul Islam]`

**Institution:**  
`[University of Chittagong]`

---

# Related Areas

- Smart Energy Management
- Internet of Things (IoT)
- Embedded Systems
- Smart Home Automation
- Renewable Energy
- Solar Energy
- Energy Efficiency
- Machine Learning
- Smart Grid
- Demand-Side Management

