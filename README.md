 Rubber-Ducky-ESP32 (Silicon Ghost)

This repository contains the design, component, and manufacturing files for a custom ESP32-S3 based hardware testing and HID (Human Interface Device) emulation tool. Developed as an independent hardware research project, the primary objective was to explore PCB design, microcontroller architecture, and rapid manufacturing processes.

 Project Overview
The board operates as a programmable USB device. Utilizing the ESP32-S3 microcontroller, it is capable of emulating keyboard and mouse inputs while providing substantial storage capacity for executing complex payloads and scripts.

 Hardware Specifications
*   **Microcontroller:** `ESP32-S3-WROOM-2-N32R8V` (Selected the 32MB Flash / 8MB PSRAM variant to ensure maximum capacity for large payloads. The N16R8 variant is also a compatible alternative.)
*   **Power Delivery:** Integrated 5V to 3.3V LDO regulator ensuring stable voltage conversion from the host USB port.
*   **Storage Expansion:** Onboard MicroSD card terminal for external data logging and payload storage.
*   **Form Factor:** Engineered for a compact footprint, closely resembling a standard USB flash drive.

 Repository Contents
The repository includes all necessary documentation for independent fabrication and assembly:

*   `BAD_USB_GERB.zip`: Verified Gerber files ready for PCB fabrication.
*   `BAD_USB.csv` (BOM): The Bill of Materials, detailing all required surface-mount components and datasheet references.
*   `BAD_USB-all-pos.csv` (CPL/POS): The component placement file required for automated SMT (Surface-Mount Technology) assembly.

 Design Methodology
To facilitate rapid prototyping and quickly validate the core concept, the initial schematics and trace routing were executed using Fritzing. While industry-standard EDA tools are typically preferred for complex routing, prioritizing a rapid development tool was a calculated decision to accelerate the first hardware iteration. The resulting Gerber files have been thoroughly reviewed, validated, and are fully production-ready.

 Manufacturing Instructions
1.  Provide the `BAD_USB_GERB.zip` archive to a standard PCB manufacturer.
2.  For PCBA (Printed Circuit Board Assembly) services, supply the `BAD_USB.csv` and `BAD_USB-all-pos.csv` files.
3.  Ensure the manufacturer specifies the `ESP32-S3-WROOM-2` module during assembly to maintain the intended high-capacity storage architecture. The footprint remains identical to the WROOM-1 series, requiring no physical alterations to the board design.

 Disclaimer
This hardware was developed strictly for educational purposes and authorized security research. It is not intended for use on systems, networks, or hardware without the explicit consent of the owner. I assume no liability for any misuse or damage caused by the fabrication or deployment of this design.
