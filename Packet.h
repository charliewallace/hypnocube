// HypnoCOMM - serial communications for the HypnoGadgets
// Copyright Chris Lomont 2007-2008
// www.HypnoCube.com, www.HypnoSquare.com
// Packet handling header
#ifndef PACKET_H
#define PACKET_H

#include "defines.h"

#ifdef WIN32
namespace HypnoGadget {
#endif // WIN32

/*	BASIC OPERATION
Send bytes from port into PacketDecodeBytes. Whenever a complete
sequence of packets consisting one command is decoded, then 
PacketGetData will return true and return proper items
todo - more
*/

// some sizes and values for the packet encoder/decoder
enum {
	PacketSYNC      = 192,   // SYNC and ESC are for syncing information
	PacketESC       = 219,   // ESC code for encoding bytes
	PacketNotLast   = 2,     // not the last packet in a block of data
	PacketLast      = 3,     // the last packet in the block of data
	PacketPayLength = 50,    // maximum datasize for a packet
	PacketMaxCount  = 13,    // maximum number of packets in a command
	PacketSequenceMask = 31, // mask to get out sequence number from packet
	PacketTypeShift = 5,     // bits to shift to get type out
	PacketDataStart = 3,     // offset where data starts in a packet
	PacketOverhead  = 5,     // bytes overhead on data per packet
	PacketDestLoc   = 2      // byte with packet destination in header
	};


typedef enum
	{ // packet and command errors from the spec
	PacketErrorNone     =  0, // no error
	PacketErrorTimeout  =  1, // too long between packets
	PacketErrorMissing  =  2, // missing packet in sequence
	PacketErrorChecksum =  3, // Packet had a bad checksum (CRC)
	PacketErrorType     =  4, // type illegal
	PacketErrorSequence =  5, // out of sequence counter
	PacketErrorSYNC     =  6, // SYNC characters missed
	PacketErrorLength   =  7, // data inconsistency
	PacketErrorCommand  =  8, // illegal command
	PacketErrorData     =  9, // command legal, data illegal
	PacketErrorDecode   = 10, // decoding of ESC failed
	PacketErrorOverflow = 11, // too much data fed the packet
	PacketErrorNotImpl  = 12  // item not implemented
	} PacketError;

typedef struct {
	// global packet data - tracks statistics and errors
	uint32 byteCount_;
	uint32 packetCount_;
	uint32 errorCount_;
	PacketError packetError_;

	// decoded packets get merged here, until done, then signaled
	uint8  decodeDestination_; // assume broadcast
	uint16 decodeLength_; // length of decoded data
	uint16 packetEncodedCRC_; // last encoded CRC
	uint16 packetDecodedCRC_; // last decoded CRC
	uint8  decodeData_[(PacketPayLength+PacketOverhead+1)*PacketMaxCount];     // decoded data
	bool dataBlockReady_; // signals when a set of packets have been disassembled and are ready for consumption

	// decoded single packet
	uint8 packetData_[(PacketPayLength+PacketOverhead+1)*2];     // decoded data
	uint8 packetPos_;
	uint8 packetSequence_; // sequence counter for the packet decoding

	// state for byte decoder
	uint8 syncCounter_;  // keeps track of if we're between SYNC characters

	} PacketHandlerState;


// reset the packet handler - call if there is an error, packets might be lost
void PacketReset(PacketHandlerState * state);

// read this to see if there is any errors before getting or sending packet data
PacketError PacketGetError(PacketHandlerState * state);

// clear the internal error state
void PacketClearError(PacketHandlerState * state);

// passes data into the packet decoding system
// returns number of unprocessed bytes, which are likely the next packet. 
// After processing the packet data, the data is prepared to be read by 
// PacketGetData. 
uint16 PacketDecodeBytes(PacketHandlerState * state, const uint8 * data, uint16 length);

// send a block of data of given length
// to the destination item (0 = broadcast)
// return true iff sent ok
// todo- comment
bool PacketSendData(PacketHandlerState * state, void (*IOWriteByte)(void * param, uint8), void * ioParam, uint8 destination, const uint8 * data, uint16 length);

// see if a packet is ready, returns true iff one is ready
// sets a pointer to the decoded data
// if one was ready, resets internals to process the next packet
bool PacketGetData(PacketHandlerState * state, uint8 * destination, uint8 ** data, uint16 * length);

// get count of bytes decoded since last reset
uint32 PacketByteCount(PacketHandlerState * state);

// get count of packets seen since last reset
uint32 PacketCount(PacketHandlerState * state);

// get count of packet errors seen since last reset
uint32 PacketErrorCount(PacketHandlerState * state);

// sequence counter for previous packet
uint8 PacketSequence(PacketHandlerState * state);

// CRC for previous packet (encoded or decoded)
uint16 PacketCRC(PacketHandlerState * state, bool decoded);

#ifdef WIN32
}; // namespace HypnoGadget
#endif // WIN32

#endif // PACKET_H

// end - Packet.h
