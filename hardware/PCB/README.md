# PCB Manufacturing

This PCB can be easily manufactured and ordered through online PCB services such as JLCPCB, PCBWay, or similar providers.

Version 1 is a bit buggy but can work.

The push button print is not right for the push button I had in stock, and the microphone board needs to be mounted on pins. Next version should take care of this and maybe accomodate several push buttons sizes.

## How to Order

Simply upload the Gerber files to your chosen PCB manufacturer's website. The Gerber files contain all the necessary information for fabrication including traces, layers, holes, and other manufacturing specifications.

## Assembly

Three tricky parts:
- the battery pads underneath the ESP32: Put the ESP32 in place and make sure it does not move, then from the other side of the PCB heat the holes joining the pads and put solder. It worked for me. Alternatively, use solder paste and hot air, but I didn't try it yet.
- the SD board: soldered without pins, just heat the holes and put solder from the top, capilarity will do the rest.
- the microphone board: will be similar in the next pcb version, for now mount it on pins but the pins should not emerge on the other side of the pcb since the battery rests agains the pcb.

## Schematics

<img src="img/schematics.jpg" alt="Schematics" width="400" />

## PCB Views

Dimensions: 51.9 mm* 21.7 mm

Top side

<img src="img/topSide.jpg" alt="Top Side" width="400" />

Bottom side

<img src="img/bottomSide.jpg" alt="Bottom Side" width="400" />


