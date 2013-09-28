/*
	Jeelabs Library <www.jeelabs.org>
	AudioPlug Library <info@theater4all.nl> http://opensource.org/licenses/mit-license.php

	Wiring Connections
	- Pwr Relay - D4 Orange
	- CS - D7 Blue Pin 2
	- Clk - A2 Grey Pin 6
	- Data - D6 Orange Pin 3
	- Mute -  A3 White Pin 8
	- IR - A1 Port 2
  - Int1 - AC Detection D3 Purple
  - Mute Relay - D8 White
	
	Basic Functions
  - Detect AC Loss and Pressence to prevent speaker pop.
	- Control Power Relay - Done
	- Control Mute, Volume Increment and Decrement
		- Via IR - Done
		- Via RF - Note sure yet if I will implement this.
	- Learn IR Codes and Associate - Done
		- Store in Eprom - Done
	
	EEPROM Memory Mapping
	- 1 to 4 Volume Up IR
	- 5 to 8 Volume Down IR
	- 9 to 12 Mute IR
	- 13 to 16 Power IR
	- 17 Start Up Volume IR. Limited on recall. If Greater >150 then zero is returned. For sanity!
*/

//Jeelib Setup
#include <JeeLib.h>
#define IRPORT 2 		//IR Data Input Port
InfraredPlug ir (IRPORT);

//AudioPlug Setup
#include <AudioPlug.h>
#define INCR 3			//Volume Increments
AudioPlug Plug(PORTHIGH);

//IR Code Setup
#include <EEPROM.h>
byte VolIncr[4];
byte VolDecr[4];
byte MuteCode[4];
byte PwrCode[4];

byte count;
const byte* data;
byte lastValue[4];

byte volValue;

bool ValidCode;

//Other Setup
#define POWERRELAY 4 	//Power Relay IO Pin
#define MUTERELAY 8 //Mute Relay IO Pin
#define ACDETECT 3 //ACDetect IO Pin
bool muteRelayActive;
bool muteActive;
bool pwrActive;
byte value;

//End of Config

//Interupt Service Routine Vector - Called when IR data causes interupt
ISR(PCINT1_vect) {
  ir.poll();
}

//Setup Routine
void setup () 
{
	delay(500);
	Serial.begin(57600);
	Serial.println("\n[PGA2311 Amp]");
	
	//Enable pin change interrupts on PC1
	PCMSK1 = bit(1);
	PCICR |= bit(PCIE1);
  
  //Enable Int1 for AC Detection
  pinMode(ACDETECT, INPUT);
  attachInterrupt(ACDETECT - 2, handleAC, CHANGE);
	
	//init state variables
	ValidCode = false;
	muteActive = false;
	pwrActive = false;
  muteRelayActive = false;
	
	//Setup Relay Pin as Output and ensure the relay is off.
	pinMode(POWERRELAY, OUTPUT); 
	digitalWrite(POWERRELAY, pwrActive);
  
  //muteRelay
	pinMode(MUTERELAY, OUTPUT); 
	digitalWrite(MUTERELAY, muteRelayActive);
	
	// init audioPlug
	Plug.addBoard(TWOCHANNELS);
	setVolume(recallStartVolume());

	//recall IR codes in EEprom, these could be invalid if never learnt.
	recallIR();
}

void updateVolume(byte val)
{
  Serial.print(F("Volume: "));
  int f = 31 - ((255 - val) >> 1);

  if (f >= 0) 
  {
    Serial.print (" ");
    if (f < 10) Serial.print (" ");
    Serial.print(f,DEC);
  }
  else if (f > -10) {
    Serial.print("- ");
    Serial.print(-f,DEC);
  }
  else {
    Serial.print(f,DEC);
  }
  Serial.println("dB");
}

void setVolume(byte val)
{
	Plug.setVolume(0, val);
	Plug.setVolume(1, val);
	updateVolume(val);
	Plug.sendVolumeData();
}

void printHelp()
{
	Serial.println(F("Any Key - Help"));
	Serial.println(F("L - Learn IR"));
	Serial.println(F("nS - Store start volume,  1 = store, 0 = reset"));
	Serial.println(F("U - Volume Up"));
	Serial.println(F("D - Volume Down"));
	Serial.println(F("nV - Set Volume, 0-255"));
	Serial.println(F("nM - Mute, 0 or 1"));
	Serial.println(F("nP - Power, 0 or 1"));
  Serial.println(F("nX - MuteRelay, 0 or 1"));
}

void incrementVolume(bool d = 0)
{
	int val = Plug.getVolume(0);
	if (d == 1)
	{
		val += INCR;
		if (val > 255) val = 255;
	}
	else
	{
		val -= INCR;
		if (val < 0) val = 0;
	}
	
	setVolume(val);
}

//Handle Data on the Serial Port
void handleSerial (char chr)
{
	// Should Respond to the following commands
	// L - Learn IR Commands, follow on screen instructions
	// U - Volume Up
	// D - Volume Down
	// V - Set Volume, 0-255
	// G - Set Gain 
	// M - Mute, 0 or 1 State
	// P - Power, 0 or 1 State
	// Unknown Key - Help Text
		
	if (chr >= '0' & chr <= '9')
	{
		value = value * 10 + chr - '0';
	}
	else
	{
		switch (chr)
		{
			case 'L':
				learnIR();
			return;
			
			case 'U':
				incrementVolume(1);
			return;
			
			case 'D':
				incrementVolume();
			return;
			
			case 'V':
				setVolume(value);
				value = 0;
			return;
			
			case 'M':
				muteActive = value;
				setMute(muteActive);	
				value = 0;
			return;
			
			case 'P':
				setPowerRelay (value);
				value = 0;
			return;
			
			case 'S':
				setStartVolume (value);
				value = 0;
			return;
      
      case 'X':
        setMuteRelay (value);
        value = 0;
      return;
			
			default :
				printHelp();
		}
		value = 0;
	}
}

void loop () 
{
  if(Serial.available()) handleSerial(Serial.read()); //If Serial data is available go and work out what to do.

	count = ir.done();
	if (count > 0) 
	{
		data = ir.buffer();

		// check code
		if (ir.decoder(count) ==  InfraredPlug::NEC) 
		{
		  lastValue[2] = data[2];
		  lastValue[3] = data[3];
		  ValidCode = true;
		}
		else if (ir.decoder(count) == InfraredPlug::NEC_REP) 
		{
			// do not repeat Mute-toggle
			if (((lastValue[2] == MuteCode[2]) and (lastValue[3] == MuteCode[3])) or ((lastValue[2] == PwrCode[2]) and (lastValue[3] == PwrCode[3])))
			{
				ValidCode = !true;
			}
			else
			{
				ValidCode = true;
				delay(200);
			}
		}

		// got something ?
		if (ValidCode) 
		{
			ValidCode = false;
			if ((lastValue[2] == MuteCode[2]) and (lastValue[3] == MuteCode[3]))
			{
				setMute(!muteActive);				
			}
			else if ((lastValue[2] == VolIncr[2]) and (lastValue[3] == VolIncr[3]))
			{
				incrementVolume(1);
			}
			else if ((lastValue[2] == VolDecr[2]) and (lastValue[3] == VolDecr[3]))
			{
				incrementVolume();    
			}
			else if ((lastValue[2] == PwrCode[2]) and (lastValue[3] == PwrCode[3]))
			{			
				setPowerRelay(!pwrActive);  
			}
		}
	}
}

void setMute (bool state)
{
	Serial.println(state ? "Mute On" : "Mute Off");
	Plug.setMute(state ? MUTEON : MUTEOFF);
  muteActive = state;
}

void setPowerRelay (bool state)
{
	Serial.println(state ? "Power On" : "Power Off");
  if (state) 
  {
    digitalWrite(POWERRELAY, state); 
    delay(3000);
    setMuteRelay(state);
    setVolume(recallStartVolume());
    setMute(!state);
  }
  else
  {
    setMuteRelay(state);
    digitalWrite(POWERRELAY, state);   
    delay(5000);  
  }
  pwrActive = state;  
}

//handleAC detection
void handleAC()
{
  setMuteRelay (0); //Keep Mute relay off while AC is transcient.
  //setPowerRelay (0);
  //Serial.println("Power Disrupted");
  //delay (50); //Allow settling time (Probably a bit crude for an interrupt routine)
}

void setMuteRelay (bool state)
{
  digitalWrite(MUTERELAY, state); 
  muteRelayActive = state;
	Serial.println(muteRelayActive ? "Mute Relay On" : "Mute Relay Off");
}

//Recall start volume
byte recallStartVolume()
{
	byte v = EEPROM.read(17);
	return v > 150 ? 0 : v;
}

//Set the amplifier start up volume to current level
void setStartVolume(byte value)
{
	if (value == 1)
	{
		EEPROM.write(17, Plug.getVolume(0));
	}
	else
	{
		EEPROM.write(17, 0);
	}
	Serial.print("Start ");
	updateVolume(recallStartVolume());
}

//IR Routines
//Loop to wait during learning of IR codes, returns true if IR code is NEC.
bool getIRValue(void)
{
	while (1) 
	{
	count = ir.done();
	
		if (count > 0) 
		{
		data = ir.buffer();
		
			if (ir.decoder(count) ==  InfraredPlug::NEC) 
			{
				return true;
			}
			return false;
		}
	}
}

//Routine to learn IR commands
void learnIR ()
{
	Serial.println(F("IR Learning Mode"));
	
	Serial.println(F("Press:"));
	Serial.println(F("Volume Up"));
	if (getIRValue()) writeIR (1);
	delay(300);

	Serial.println(F("Volume Down"));
	if (getIRValue()) writeIR (5);
	delay(300);

	Serial.println(F("Press Mute"));
	if (getIRValue()) writeIR (9);
	delay(300);

	Serial.println(F("Press Pwr"));
	if (getIRValue()) writeIR (13);
	delay(300);
	
	Serial.println(F("Done"));
	delay(100);
	recallIR();
}

//Recall IR Commands in EEprom
void recallIR()
{
	readIR(1, VolIncr);
	readIR(5, VolDecr);
	readIR(9, MuteCode);
	readIR(13, PwrCode);
}

//Write current IR data buffer to EEProm at address specified
void writeIR (int Addr)
{
	for (int i = Addr; i < Addr + 4; i++)
	{
		EEPROM.write(i, data[i-Addr]);
	}
}

//Read an IR command from a location and store in array pointed to
void readIR (int Addr, byte* Arr)
{
	for (int i = Addr; i < Addr + 4; i++)
	{
		Arr[i-Addr] = EEPROM.read(i), HEX;
	}
}

//End of Code