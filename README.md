Amp_Control
===========

Control of a Texas Instruments PGA2311 or PGA2310 with an Arduino UNO

Makes use of the ports library from Jeelib. see https://github.com/jcw/jeelib for details.
Also uses the PGA2311 / PGA2310 library from Theater4All http://www.scenelight.nl/downloads/ 
available here http://www.scenelight.nl/download/libs/AudioPlug.zip

Wiring Connections
  - Pwr Relay - D4
	- CS - D7 to PGA Pin 2
	- Clk - A2 to PGA Pin 6
	- Data - D6 to PGA Pin 3
	- Mute -  A3 tp PGA Pin 8
	- IR - A1 Port 2
 	- AC Detect - D3
  	- Mute Relay - D8
	
	Basic Functions
	- Control Power Relay - Done
	- Control Mute, Volume Increment and Decrement
		- Via IR - Done
		- Via Serial - Done
	- Learn IR Codes and Associate - Done
		- Store in Eprom - Done  
  	- Loss of AC detection and Control of mute relay - Done
	
	EEPROM Memory Mapping
	- 1 to 4 Volume Up IR
	- 5 to 8 Volume Down IR
	- 9 to 12 Mute IR
	- 13 to 16 Power IR
	- 17 Start Up Volume IR. Limited on recall. If Greater >150 then zero is returned. For sanity!
