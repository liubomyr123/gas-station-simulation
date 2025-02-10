# Gas Station Simulation
## [Outdated, documentation in progress]

## Overview
This project simulates a gas station where different types of vehicles arrive to refuel. The simulation includes queue management, fuel consumption tracking, and various optimization strategies for prioritizing vehicles. The project is designed to run on Linux-based systems.

## Features
- Supports multiple vehicle types with different fuel consumption rates:
  - 🚗 Auto: 10L/100km
  - 🚙 Van: 15L/100km
  - 🚛 Truck: 30L/100km
- User-defined number of vehicles per type.
- Dynamic queue system where vehicles wait for available fuel points.
- Separate fuel points exclusively for trucks.
- Vehicles leave the station if fuel is insufficient.
- Logging system that tracks refueling events and waiting times.
- Results are saved to a file.

## Requirements
- Linux OS (a warning is displayed if a different OS is detected).
- A multi-threaded environment (number of threads is auto-detected).
- JSON file support for structured input.

## Setup & Compilation
To compile the project, use:
```sh
make
```

To build and run the simulation in one step:
```sh
make start
```

To run the simulation after building:
```sh
make run
```

For debugging purposes:
```sh
make start_debug
```

## Input Configuration
The user can either enter the number of cars manually or use a JSON file to specify details. Example JSON format:
```json
{
  "total_fuel": 999999999,
  "vehicles": [
    {
      "type": "auto",
      "fuel_by_car": 10,
      "waiting_time_sec": 2,
      "count": 5
    },
    {
      "type": "van",
      "fuel_by_car": 15,
      "waiting_list": [
        { "waiting_time_sec": 2, "count": 3 },
        { "waiting_time_sec": 5, "count": 7 }
      ]
    },
    {
      "type": "truck",
      "fuel_by_car": 30,
      "waiting_list": [
        { "waiting_time_sec": 15, "count": 10 },
        { "waiting_time_sec": 9, "count": 2 }
      ]
    }
  ]
}
```

## Simulation Logic
1. **Step 1**: Ask the user for the number of cars and the total available fuel.
2. **Step 2**: Introduce different car types and allow the user to specify how many of each type to include.
3. **Step 3**: Store all results into a file.
4. **Step 4**: Implement a queue where each vehicle has a waiting time. If a car exceeds this waiting time, it leaves the station.

## Output Example
```plaintext
[00:00:00] 🚗 Auto #1 is refueling...
[00:00:02] 🚙 Van #2 arrived at the station. Waiting...
[00:00:05] 🚛 Truck #3 left due to long waiting (timeout).
[00:00:07] 🚗 Auto #1 left the station. Fuel used: 10L
[00:00:10] 🚚 Tanker #1 arrived at the station to deliver fuel.
```

## Future Improvements
- Implement more advanced priority handling.
- Add support for real-time monitoring via a graphical interface.
- Optimize queue management using different scheduling algorithms.

## License
This project is open-source and available under the MIT License.

