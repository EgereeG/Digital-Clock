Project: 12-Hour Digital Clock
By: Egeree Gemta

Purpose/Goal: The purpose of this project was to design and build a digital-clock using a PIC-24 Microcontroller and an LCD display (SSD 1803). The program
is written on the MPLAB X IDE software environment and in the c language. The clock uses the I2C communication protocol to control and facilitate
the PIC-24 to LCD transmission of data and commands. Hardware timers are configured to generate interrupts every minute and calls functions that update
the time displayed on the LCD accordingly. The PIC24's input capture modules and external pushbuttons work jointly for user-text rendering by generating an 
interrupt whenever the user presses the button and incrementing the time on the LCD by one minute or hour. This digital-clock project is used mainly for keeping
track of time.

Dependencies: There are no external libraries used.
