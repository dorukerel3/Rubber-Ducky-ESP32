# Rubber-Ducky-ESP32 (Silicon Ghost)

A custom **ESP32-S3** based hardware testing and **HID (Human Interface Device)** emulation tool, packaged in a footprint that closely resembles a standard USB flash drive. Developed as an independent hardware research project, the primary objective was to explore PCB design, microcontroller architecture, and rapid manufacturing processes end to end — from schematic capture through production-ready fabrication files. The board operates as a programmable USB device capable of emulating keyboard and mouse input while providing substantial onboard storage for executing complex payloads and scripts.

---

## Table of Contents

- [Project Overview](#project-overview)
- [Hardware Gallery](#hardware-gallery)
- [Key Features](#key-features)
- [Hardware Specifications](#hardware-specifications)
- [Storage and Payload Architecture](#storage-and-payload-architecture)
- [Design Methodology](#design-methodology)
- [Repository Contents](#repository-contents)
- [Manufacturing Instructions](#manufacturing-instructions)
- [Assembly Notes](#assembly-notes)
- [Disclaimer](#disclaimer)
- [Author and License](#author-and-license)

---

## Project Overview

The device is a self-contained USB peripheral built around the ESP32-S3 microcontroller. When connected to a host machine, it presents itself as a native Human Interface Device and is capable of injecting keyboard and mouse events without requiring any driver installation or background software on the target system.

The design prioritizes two things: a compact, unobtrusive form factor that mimics an ordinary flash drive, and enough onboard non-volatile capacity to hold and execute large, complex payloads. Every design and manufacturing artifact required to fabricate and assemble the board independently is included in this repository.

| Attribute | Detail |
|-----------|--------|
| Platform | ESP32-S3 microcontroller |
| Primary role | USB HID (keyboard / mouse) emulation |
| Form factor | Compact, USB flash drive profile |
| Storage | High-capacity onboard flash plus MicroSD expansion |
| Deliverables | Production-ready Gerber, BOM, and CPL files |

---

## Hardware Gallery

The physical production result of the custom-designed printed circuit board:

<img src="images/front.jpg" width="400" alt="Silicon Ghost — Front"> <img src="images/back.jpg" width="400" alt="Silicon Ghost — Back">

---

## Key Features

- **Native USB HID emulation** of keyboard and mouse input, executed directly by the ESP32-S3 with no host-side drivers or software required.
- **High-capacity payload storage**, using the 32 MB flash / 8 MB PSRAM module variant to accommodate large scripts and payload sets.
- **MicroSD expansion slot** for external data logging and additional payload storage beyond the onboard flash.
- **Compact flash-drive form factor**, engineered so the assembled board closely resembles a standard USB stick.
- **Stable onboard power delivery** through an integrated 5V-to-3.3V LDO regulator fed directly from the host USB port.
- **Fully documented for independent fabrication**, with verified Gerbers, a complete bill of materials, and a component placement file ready for automated SMT assembly.

---

## Hardware Specifications

| Component | Specification |
|-----------|---------------|
| Microcontroller | `ESP32-S3-WROOM-2-N32R8V` — 32 MB Flash / 8 MB PSRAM variant, selected to maximize payload capacity. The `N16R8` variant is a compatible alternative. |
| Power delivery | Integrated 5V-to-3.3V LDO regulator, providing stable voltage conversion from the host USB port. |
| Storage expansion | Onboard MicroSD card terminal for external data logging and payload storage. |
| Form factor | Compact footprint engineered to closely resemble a standard USB flash drive. |

---

## Storage and Payload Architecture

Storage capacity was the central selection criterion for the main module. The `ESP32-S3-WROOM-2-N32R8V` was chosen specifically for its 32 MB of flash and 8 MB of PSRAM, ensuring the device can hold and execute large payloads without running into capacity limits. Where maximum capacity is not required, the `N16R8` variant remains fully compatible and drops into the same footprint.

Beyond the onboard flash, the board exposes a MicroSD card terminal. This provides an expandable, removable storage tier suitable for external data logging and for staging larger or more numerous payloads than would comfortably fit in flash alone.

---

## Design Methodology

To facilitate rapid prototyping and quickly validate the core concept, the initial schematics and trace routing were executed using **Fritzing**. While industry-standard EDA tools are typically preferred for complex routing, prioritizing a rapid-development tool was a calculated decision to accelerate the first hardware iteration. The resulting Gerber files have been thoroughly reviewed, validated, and are fully production-ready.

---

## Repository Contents

The repository includes all documentation necessary for independent fabrication and assembly:

```text
.
├── BAD_USB_GERB.zip        Verified Gerber files, ready for PCB fabrication
├── BAD_USB.csv             Bill of Materials (BOM) — SMT components and datasheet references
├── BAD_USB-all-pos.csv     Component placement file (CPL/POS) for automated SMT assembly
└── README.md
```

| File | Purpose |
|------|---------|
| `BAD_USB_GERB.zip` | Verified Gerber files ready for PCB fabrication. |
| `BAD_USB.csv` (BOM) | The Bill of Materials, detailing all required surface-mount components and datasheet references. |
| `BAD_USB-all-pos.csv` (CPL/POS) | The component placement file required for automated SMT (Surface-Mount Technology) assembly. |

---

## Manufacturing Instructions

1. Provide the `BAD_USB_GERB.zip` archive to a standard PCB manufacturer.
2. For PCBA (Printed Circuit Board Assembly) services, supply the `BAD_USB.csv` and `BAD_USB-all-pos.csv` files.
3. Ensure the manufacturer specifies the `ESP32-S3-WROOM-2` module during assembly to maintain the intended high-capacity storage architecture. The footprint remains identical to the WROOM-1 series, requiring no physical alterations to the board design.

---

## Assembly Notes

- The `ESP32-S3-WROOM-2` and `WROOM-1` modules share an identical footprint, so substituting between them requires no changes to the PCB layout.
- Selecting the `N32R8V` module preserves the maximum-capacity storage architecture; the `N16R8` variant is acceptable where a smaller flash budget is sufficient.
- Power is derived entirely from the host USB port and regulated on-board, so no external supply is needed during operation.

---

## Disclaimer

This hardware was developed strictly for **educational purposes and authorized security research**. It is not intended for use on systems, networks, or hardware without the explicit consent of the owner. The author assumes no liability for any misuse or damage caused by the fabrication or deployment of this design.

---

## Author and License

Developed by **Doruk Erel**.

Released under the **MIT License**. See the `LICENSE` file for the full text.
