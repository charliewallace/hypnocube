// HypnoCOMM - serial communications for the HypnoGadgets
// Copyright Chris Lomont 2007-2008
// www.HypnoCube.com, www.HypnoSquare.com
// commands to send to/get from the gadget
#ifndef COMMAND_H
#define COMMAND_H
#include "defines.h"

// commands
typedef enum {
	// version 0.3 of protocol
	CommandLogin        =  0, 
	CommandLogout       =  1, 
	CommandVersion      = 12, 
	CommandAck          = 25,
	CommandPing         = 60,
	CommandError        = 20,
	// version 0.4
	CommandSetFrame     = 81, 
	CommandFlipFrame    = 80, 

	// version 0.5
	CommandReset        = 10, 
	CommandOptions      = 15,
	CommandGetError     = 21, 
	CommandInfo         = 11, 

	// version 0.6
	CommandMaxVisIndex  = 40,
	CommandSelectVis    = 41,
	CommandMaxTranIndex = 42,
	CommandSelectTran   = 43,
	CommandGetFrame     = 82,

	// version 0.7
	CommandSetPixel     = 84,
	CommandGetPixel     = 85,
	CommandCurrentItem  = 30, 

	// version 0.8
	CommandSetRate      = 50,
	CommandDrawLine     = 86,
	CommandDrawBox      = 87,
	CommandFillImage    = 88,
	
	// version 0.9
	CommandSetPFrame    = 83,
	CommandScrollText   = 89,
	CommandLoadAnim     = 90,

	CommandUnknown      = 0xFF // used to mark an unknown command
	} CommandType ;

#endif // COMMAND_H
// end - Command.h