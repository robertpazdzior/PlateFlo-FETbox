# PlateFlo FETbox - Hardware Controller
FETbox hardware controller for the PlateFlo plate-scale perfusion tissue culture
system.

See [publication](https://doi.org/10.1016/j.ohx.2021.e00222) and [ReadTheDocs](https://plateflo.readthedocs.io) for a detailed description of the whole
system and documentation of its components. ![](./imgs/fetbox_real.png)

## Description
Hosted here are all of the necessary files to build a FETbox hardware
controller. Source files are also included should you need to make 
modifications for your application.
* [Enclosure](./Enclosure/STLs)
    * [STLs]() - for 3D printing
    * [CAD](./Enclosure/CAD) - STEP and F3D complete device models
* [PCB](./PCB)
    * [JLCPCB](https://jlcpcb.com/) CAM files (for PCB ordering)
    * [EAGLE](https://www.autodesk.com/products/eagle/overview) `.SCH` and `.BRD` [source files](./PCB/EAGLE)
* Arduino Nano FETbox [firmware](./Firmware_FETbox)

## License
This project is copyright of Robert Pazdzior (2020-2021)

It is provided under the terms of the [CERN Open Hardware License
(CERN-OHL-W)](https://cern-ohl.web.cern.ch/home). A copy of the license is
included [here](LICENSE).
