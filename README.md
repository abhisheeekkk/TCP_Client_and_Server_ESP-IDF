# TCP_Client_and_Server_ESP32

TCP_Client software to e flashed in one ESP32 device and TCP_Server software to be flashed in another device.
TCP_Client device acts as an AP, TCP_Server acts as STA.
DHCP Disabled so that dynamic allocation is prevented.
We statically asign IP using esp-idf apis.

TCP_Client sends >1000 bytes of data at 1Hz rate and then prints the ack of TCP_Server.
TCP_Sever prints recieved data which is over 1000 bytes and then sends ack to TCP_Client.
