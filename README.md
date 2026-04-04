# GDS Viewer

## Purpose
Viewing and analyzing GDSII layouts.

## Description
A Qt/C++20 project for viewing files in GDSII format.

The `gds_core` library reads and parses GDS files, `gds_viewer` [displays](screenshots/1.png) geometry by layers and supports layer visibility control, layer [color selection](screenshots/2.png), [object](screenshots/3.png) - [highlighting](screenshots/4.png), and viewing object [properties](screenshots/5.png) and User Properties.

A responsive coroutine-style pipeline is implemented for loading data and building the scene.

**Note.** The version presented here is **1.x.x** and contains code for **non-hierarchical GDSII**: only the first structure is displayed.
The GDSII format description is available [here](https://boolean.klaasholwerda.nl/interface/bnf/gdsformat.html).  
Support for **hierarchical GDSII** is implemented in version **2.x.x**. (For access to version 2.x.x, please contact me via private message).

(In the **Release** build, `gds_viewer` works faster with the `example_gds/1Kpolyg.gds` file than [KLayout](https://klayout.de/)).

### [GDSII Layout Reader Library](gds_core/)

### [GDSII Viewer Based on Qt Widgets](gds_viewer/)

##

License: MIT.  
This project uses Qt, which is licensed under its own terms.
See the [Qt licensing](https://www.qt.io/development/qt-framework/qt-licensing) information for details.