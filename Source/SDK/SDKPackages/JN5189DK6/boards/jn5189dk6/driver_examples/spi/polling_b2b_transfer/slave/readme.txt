Overview
========
The spi_polling_board2board_slave example shows how to use spi driver as slave to do board to board transfer with
polling:

Notice: The SPI slave of this example uses interrupt mode, as there is no polling mode for SPI slave.

In this example, one spi instance as slave and another spi instance on other board as master. Master sends a piece of
data to slave, and receive a piece of data from slave. This example checks if the data received from master is correct.

Toolchain supported
===================
- IAR embedded Workbench  8.50.6
- MCUXpresso  11.2.1

Hardware requirements
=====================
- 2 Mini USB cable
- 2 OM15076-3 Carrier Board
- 2 JN518x Module plugged on the Carrier Board
- Personal Computer

Board settings
==============
Connect SPI master on board to SPI slave on other board
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Pin Name   Board Location
MISO       J3 pin 8
MOSI       J3 pin 24
SCK        J3 pin 22
SEL        J3 pin 10

Prepare the Demo
================
1.  Connect a mini USB cable between the PC host and the USB port (J15) on the board.
2.  Open a serial terminal with the following settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control
3.  Download the program to the target board.
4.  Reset the SoC and run the project.

Running the demo
================
The following lines are printed to the serial terminal when the demo program is executed.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Slave is working....

Succeed!
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Customization options
=====================

