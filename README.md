**Project Name: System Monitoring and Management**

---

### Overview

This repository contains the source code for a Mobile 5G Service Authorization System Simulator. The application consists of several components, including an authentication manager, a monitor engine, and a system manager. These components work together to monitor user activity, manage system resources, and handle authentication requests.

### Components

1. **Authentication Manager**: Responsible for managing authentication requests and creating authentication engines to handle user authentication.

2. **Monitor Engine**: Monitors user activity and system resources, sends alerts when predefined thresholds are reached, and generates periodic reports for system monitoring.

3. **System Manager**: Manages system resources, coordinates communication between different components, and handles the overall operation of the system.

### Files and Directories

- **auth_manager.c**: Source code for the Authentication Manager component.
- **auth_engine.c**: Source code for the Authentication Engine component.
- **backoffice_user.c**: Source code for the Backoffice User component.
- **logs.c**: Source code for the Logs component.
- **mobile_user.c**: Source code for the Mobile User component.
- **mobile_user.sh**: Shell script for managing Mobile User processes.
- **utils.c**: Source code utility functions used by the system.
- **monitorengine.c**: Source code for the Monitor Engine component.
- **sysmanager.c**: Source code for the System Manager component.
- **log.txt**: Log file used by the system to record events and errors.
- **config.txt**: Configuration file used to specify system parameters.

+ Header files for each corresponding `.c` file.

### Usage

To compile and run the system, follow these steps:

1. Clone the repository to your local machine.
2. Navigate to the project directory.
3. Compile the source code using a C compiler (e.g., gcc).
   ```
   MAKE
   ```
4. Run the following lines in different terminals.
   ```
   ./5g_auth_platform "config.txt"
   ./mobile_user 30 20 500 500 500 2
   ./backoffice
   ```

### Configuration

The system configuration is specified in the `config.txt` file. You can adjust parameters such as the number of users, queue size, number of authentication servers, etc., to customize the system behavior.

### Contributing

Contributions to the project are welcome. If you find any issues or have suggestions for improvements, please open an issue or submit a pull request.

### License

Done by [Pedro Lima](https://github.com/pseudolimonada) and Carlos Soares. All rights reserved.

---
