# 🚀 Bluetooth-to-UART Gateway with ESP32-C

This project implements a **Bluetooth-to-UART gateway** using two **ESP32-C** chips running **FreeRTOS**. It acts as a simple wireless messaging system, enabling communication between two laptops via Bluetooth using ESP32 modules.

---

## 🔧 How It Works

1. A message is entered on **Laptop A** via a serial terminal.
2. The message is sent via **UART** to the first **ESP32-C**.
3. The ESP32 transmits the message over **Bluetooth** to a second ESP32.
4. The second ESP32 sends the message over **UART** to **Laptop B**, where it’s displayed.

### 🛠️ Supported Runtime Commands

You can issue the following commands over UART to retrieve system info:

| Command            | Description                                                                 |
|--------------------|-----------------------------------------------------------------------------|
| `CHIP_INFO`        | Displays ESP32 chip details: model, features, revision, and core count.     |
| `IDF_VERSION`      | Shows the ESP-IDF version used in the current build.                         |
| `FREE_HEAP`        | Returns the current free heap memory (in bytes).                            |
| `FREE_INTERNAL_HEAP` | Reports free internal RAM (excluding external PSRAM).                     |
| `FREE_MIN_HEAP`    | Shows the minimum heap available since startup.                             |
| `CLOCK_SPEED`      | Displays the CPU clock speed (in MHz).                                      |
| `RESET`            | Performs a software reset of the ESP32 chip.                                |

---

## ⚙️ Build & Flash Instructions

You can build and flash the firmware using either **Docker** (recommended) or a native ESP-IDF setup.

---

### 🐳 Option 1: Docker in VS Code (Recommended)
The detailed documentation can be found on the [Espressif’s Docker Setup Guide](https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/additionalfeatures/docker-container.html).

1. Windows WSL (WSL version 2 required)
   - If WSL is not installed, run:
      ```bash
      wsl --install
      ```
   - Update the WSL kernel with:
      ```bash
      wsl --update
      ```
   - To install a Ubuntu distribution in WSL on Windows, type the following command:
      ```bash
      wsl --install --distribution Ubuntu
      ```
2. [VS Code](https://code.visualstudio.com/)
   - Install Extension: [Dev Containers](https://marketplace.visualstudio.com/items/?itemName=ms-vscode-remote.remote-containers)
3. [usbipd-win](https://github.com/dorssel/usbipd-win/releases)
4. [Docker Desktop For Windows](https://hub.docker.com/)

#### 🔌 Connect USB Serial Devices in WSL

1. Open PowerShell as Admin and list USB devices:
   ```bash
   usbipd list
   ```

2. Bind the desired device:
   ```bash
   usbipd bind --busid <BUSID>
   ```

3. Attach device to WSL:
   ```bash
   usbipd attach --wsl --busid <BUSID> --auto-attach
   ```
> 💡 This command can be canceled and after that the usb device should be seen in the wsl:
> ```bash
> lsusb
> ```

#### 🚀 Flash & Monitor

1. Open the project in VS Code.
2. Click the bottom-left **remote connection icon** → *Reopen in Container*.
   - The environment sets up automatically and the doxygen documentation will be automaticly be generated
3. (Preconfigured): Select target **ESP32C6**, flash method **JTAG**, and the correct port.
4. Use the **ESP-IDF buttons** (status bar or command palette) to:
   - Build  
   - Flash  
   - Monitor  

---

### 💻 Option 2: Native ESP-IDF + VS Code

Use the ESP-IDF Extension for VS Code ([installation guide](https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/installation.html)).

#### ✅ Tools Required

- [VS Code](https://code.visualstudio.com/)
- ESP-IDF VS Code Extension

#### 🚀 Flash & Monitor

1. Open the project in VS Code.
2. (Preconfigured): Select target **ESP32C6**, method **JTAG**, and your device port.
3. Use the ESP-IDF buttons to:
   - Build  
   - Flash  
   - Monitor  

---

## 📚 Generate Documentation (Doxygen + Graphviz)

The project includes a preconfigured `Doxyfile` for generating full HTML documentation with class diagrams.

### 📄 Features of Generated Docs

- Function/class/module descriptions
- Cross-referenced source code
- UML-style class diagrams (if Graphviz is installed)

---

### 🐳 Option 1: Generate Docs Inside Docker

1. The dev container includes **Doxygen** and **Graphviz**.
2. Once your container is running in VS Code, run:

   ```bash
   ./generate_docs.sh
   ```

3. Output will appear in:

   ```
   docs/html/index.html
   ```

> ✅ No extra installation required – fully integrated into the container!

---

### 🧩 Option 2: Generate Docs Locally

#### ✅ Install Required Tools

| Tool       | Purpose                 | Download Link                           |
|------------|-------------------------|------------------------------------------|
| **Doxygen** | Documentation generator | https://www.doxygen.nl/download.html     |
| **Graphviz**| Diagram support         | https://graphviz.org/download/           |

> 💡 On Windows: ensure **"Add to PATH"** is checked during installation.

#### 🔧 Generate

1. In the root project directory, run:

   ```bash
   doxygen Doxyfile
   ```

2. If successful, the documentation will be generated in the folder specified by the `OUTPUT_DIRECTORY` in the `Doxyfile`.

   By default, this is:

   ```
   docs/html/index.html
   ```

3. Open `index.html` in your browser to view the docs.
