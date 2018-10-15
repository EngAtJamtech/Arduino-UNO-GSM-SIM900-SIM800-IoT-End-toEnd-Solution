# Introduction
This article will show you how to roll-your-own simple end-to-end internet-of-things (IoT) solution without the need for IoT platforms such as thingspeak or cloudMQTT at virtually no cost. If that appeals to you then read on. 
For simplicity we’ll assume you have a hosting account free or otherwise supporting php and MySql and has some form of control panel to aid setting up the database and uploading and modifying files.
The three main aspects of the end-to-end solution are:
•	Sensor and gateway for sending data to the remote server (Device)
•	Server side application end point (API)
•	Server side user interface (Dashboard)


![Image of Yaktocat](https://github.com/EngAtJamtech/Arduino-UNO-GSM-SIM900-SIM800-IoT-End-toEnd-Solution/blob/master/images/iot-image-1.png)

# Pre-requisites
You will need to have, or obtain, skills in:
-	Arduino hardware and Arduino shields
-	Arduino programming
-	Arduino sketch (basic knowledge)
-	Some PHP understanding
-	Uploading files to a hosting account
-	Setting up MySql database

# SIM900 Set up
Pictured below is the SIM900 shield mounted on top of the Arduino UNO (hidden from view). 
Note:
1.	The required jumper settings to allow software serial to be use IO pins 8 & 7. 
2.	External power is needed from a 9V to 12V 2A (or better) power supply. Make sure the power switch is the position shown. 
3.	Always have an antenna connected when powered, not doing so could cause damage to the RF power stage of the modem. The antenna connector has been removed from the board as shown due to short comings in the board layout, not doing so may result in unpredictable operation.
4.	Once power is applied the GSM modem it will need turning on by pressing the power key.
5.	Not visible on the underside is the SIM socket.


![Image of Yaktocat](https://github.com/EngAtJamtech/Arduino-UNO-GSM-SIM900-SIM800-IoT-End-toEnd-Solution/blob/master/images/iot-image-2.png)

**See full article 'Arduino SIM900 SIM800 IoT End-to-end solution Rev1.pdf' in the repository.**

