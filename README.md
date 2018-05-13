Measurement in Zephyr RTOS
----------------------------------------------------------------------------------------------------------------------------------------- 

   Following project is used to develop an application to measure interrupt latency and context switching overhead of Zephyr RTOS (version 1.10.0) on Galileo Gen 2 board.

Getting Started :

    These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. 
    See deployment for notes on how to deploy the project on a live system.

Prerequisites :

  zephyr 1.10.0 source code
  zephyr sdk 0.9.2
  CMake version 3.8.2 or higher is required

Installing :

Unzip & download below files in user directory on your machine running linux distribution.

   1) README.md
   2) measure_n.zip
   3) Report_3


Deployment :

   Install zephyr source code with patched version & sdk. 
   
   Create the following directories
   efi
   efi/boot
   kernel

   Locate binary at $ZEPHYR_BASE/boards/x86/galileo/support/grub/bin/grub.efi and copy it to $SDCARD/efi/boot 
   
   Rename grub.efi to bootia32.efi

   Create a $SDCARD/efi/boot/grub.cfg file wich contains following :
   set default=0
   set timeout=10

   menuentry "Zephyr Kernel" {
   multiboot /kernel/zephyr.strip
   }

   Make following connections of GPIO : 
   IO5 => PWM
   IO0 => input pin.

Execution :
   
   Change directory to following :
   cd $ZEPHYR_BASE/samples/measure_n/build

   Use cmake command as below :
   cmake -DBOARD=galileo ..

   Make using following :
   make

   Locate zephyr.strip file generated in $ZEPHYR_BASE/samples/measure_n/build/zephyr 

   Copy this to $SDCARD/kernel 

   Insert SD card in Galileo board and reboot.

-----------------------------------------------------------------------------------------------

Expected results :

Three computation tasks execute one after other in sequence

In the end it asks user to enter command in shell prompt.
Select shell_mod
-> print 500


Built With :

  Source : Zephyr 1.10.0
  SDK : Zephyr-0.9.2
  64 bit x86 machine

Authors :

Sarvesh Patil 
Vishwakumar Doshi

Reference :
http://docs.zephyrproject.org/kernel/kernel.html

License :

This project is licensed under the ASU License

