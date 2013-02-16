// header for options stored in permanent store
#ifndef _OPTIONS_H
#define _OPTIONS_H

#include "defines.h"
#include "Command.h"

#ifdef WIN32
namespace HypnoGadget {
#endif // WIN32

#define VIS_MAX    60  // max number of visualizations
#define TRANS_MAX  15  // max number of transitions

#ifdef WIN32
#pragma pack(1) // must have everything byte aligned like the PIC does
#endif

// per visualization options
typedef struct 
	{
	uint8 minSpeed_, maxSpeed_; // speed, must satisfy 1<=min<=max<=255;
	uint8 frequency_;           // how often this occurs, relative to others
	uint16 count_;              // playback counts
	} VisOptions;
// size 5 bytes

// per transition counts
typedef struct
	{
	uint8 frequency_;           // how often this occurs, relative to others
	uint16 count_;              // playback counts
	} TransOptions;
// size 3 bytes

// global system options
typedef struct
	{
	// this must be first, and goes here to allow transfer directly into location
//	CommandType command_;  // WIN32 enlarges this enum type, making trouble for us
	uint8 command_; 
	
	// version of the options structure 0-255
	uint8 optionsVersion_; // loaded one must match or ignored on load
		
	// count number of times this has been stored
	// NOTE: not reset by reset function
	uint16 storeCount_;
	
	// random number generator state
	uint32 randState_[16];

	// set to true to indicate easter egg triggered
	uint8 easterCounter_; // counter for triggering easter egg

	// count of total cube runs - after powered up a few seconds this gets incremented and stored
	uint16 runCount_;

	// device id for networked devices
	uint16 deviceId_;   

	// allow user to set, used to secure (somewhat) the 
	// device from attacks. Used in a challenge mode.
	uint32 challenge_; 

	// when outer user logged in, requires ping to stay alive
	bool requirePing_; 
	uint8 pingDelay_; // max time in 16th of sec between needed pings
	
	/************ VISUALIZATION ITEMS ***************/
	bool playSequentialVis_;          // sequential or random order vis
	bool useGlobalSpeed_;             // true to override per item speeds
	uint8 minVisSpeed_,maxVisSpeed_;  // global speeds
	VisOptions visOptions_[VIS_MAX];  // this many
	uint8 visOrder_[VIS_MAX];         // the order to play them back

	/************ TRANSITION ITEMS ***************/
	bool playSequentialTrans_;        // sequential or random transition order
	TransOptions transOptions_[TRANS_MAX];
	uint8 transOrder_[TRANS_MAX];     // order to play them in

	} Options;
// total size 1 + 1 + 2 + 16*4 + 1 + 2 + 2 + 4 + 1 + 1 + 1 + 1 + 2 + 60*5 + 60 + 1 + 15*3 + 15
//  = 504 = 0x1F8

extern Options options_;

// loads options from permanent storage if available
// else resets them to default values
void OptionsLoad(void);

// reset to default values, does not store
void OptionsReset(void);

// store options to permanent storage
void OptionsStore(void);

#define OPTIONS_VERSION 1 // current version of options struct
#define OPTIONS_SIZE 504  // currently Options struct must be this size

#ifdef WIN32
#pragma pack() // default packing
#endif

#ifdef WIN32
}; // namespace HypnoGadget
#endif // WIN32


#endif // _OPTIONS_H
// end - Options.h

