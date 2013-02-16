// HypnoCOMM - serial communications for the HypnoGadgets
// Copyright Chris Lomont 2007-2008
// www.HypnoCube.com, www.HypnoSquare.com
// code to implement packet encoding, decoding
#include <assert.h>
#include "Packet.h"
#include "CRC16.h"

//#include <windows.h> // todo - remove - needed for Sleep

#ifdef WIN32
namespace HypnoGadget {
#endif // WIN32

#ifdef PIC18F
// tells where to put this item in RAM - needs large storage
// to make it work, you need to pack two RAM pages together
// in the linker script like so. Change
//DATABANK   NAME=gpr12      START=0xC00          END=0xCFF
//DATABANK   NAME=gpr13      START=0xD00          END=0xDFF
// to 
//DATABANK   NAME=gpr12      START=0xC00          END=0xDFF
#pragma idata packet=0xC00 
#endif


// reset the packet handler
void PacketReset(PacketHandlerState * state)
	{
	state->byteCount_   = 0;
	state->packetCount_ = 0;
	state->errorCount_  = 0;
	state->packetError_ = PacketErrorNone;

	// decoded information
	state->decodeDestination_ = 0; // assume broadcast
	state->decodeLength_ = 0; // length of decoded data
	state->packetDecodedCRC_ = 0;
	state->packetEncodedCRC_ = 0;
	state->dataBlockReady_ = false;

	// packet decoder
	state->packetPos_ = 0;
	state->packetSequence_ = 0; // sequence counter for the packet decoding

	state->syncCounter_ = 0;
	} // PacketReset

// call this to set a packet error
static void SetPacketError(PacketHandlerState * state, PacketError error)
	{
	++state->errorCount_;
	state->packetError_ = error;
	} // PacketError

// every time a packet is decoded, this merges it into a larger data store
// when done, it sets a flag. Routine also removes ESCaped characters from stream, 
// checks packet integrity, and sets internal error conditions as needed.
static void PacketDecode(PacketHandlerState * state)
	{
	uint16 pos, dst = 0, crc1, crc2;
	uint8 type;
	
	// error - no packet this small
	if (state->packetPos_ < PacketOverhead+1)
		{
		SetPacketError(state,PacketErrorLength);
		return;
		}

	// decode packet in packetData_ into itself, 
	// removing ESCaped bytes. 
	for (pos = 0; pos < state->packetPos_; ++pos)
		{
		uint8 byte = state->packetData_[pos];
		if (PacketESC == byte)
			{ // ESC sequence
			byte = state->packetData_[++pos];
			if (PacketESC+1 == byte)
				byte = PacketSYNC;
			else if (PacketESC+2 == byte)
				byte = PacketESC;
			else
				{ // error - unknown ESC sequence
				SetPacketError(state,PacketErrorDecode);
				return;
				}
			}
		state->packetData_[dst++] = byte; // save the byte out
		}

	state->packetPos_ = (uint8)dst; // set this

	// check checksum
	crc1 = CRC16(state->packetData_,state->packetPos_-2);
	crc2 = state->packetData_[state->packetPos_-2];
	crc2 <<= 8;
	crc2 += (state->packetData_[state->packetPos_-1]);
	state->packetDecodedCRC_ = crc2; // save last decoded CRC
	if (crc1 != crc2)
		{
		SetPacketError(state,PacketErrorChecksum);
		return;
		}

	// check sequence
	if (state->packetSequence_ != (state->packetData_[0] & PacketSequenceMask))
		{
		SetPacketError(state,PacketErrorSequence);
		return;
		}
	
	// check claimed and real length
	if (state->packetPos_ != state->packetData_[1] + PacketOverhead)
		{
		SetPacketError(state,PacketErrorLength);
		return;
		}

	if (0 == state->decodeLength_)
		{  // first packet in a possible sequence of them, so...
		// assume this packet and it's followers have same destination
		state->decodeDestination_ = state->packetData_[PacketDestLoc];
		}

	// decode the data portion into decodeData_
	for (pos = PacketDataStart; pos < state->packetPos_ - 2; ++pos)
		{
		if (state->decodeLength_ >= sizeof(state->decodeData_))
			{ // too big
			SetPacketError(state,PacketErrorOverflow);
			return;
			}
		state->decodeData_[state->decodeLength_++] = state->packetData_[pos];
		}

	// is it the last packet? If so, mark it
	type = state->packetData_[0] >> PacketTypeShift;
	if (PacketLast == type)
		{
		state->dataBlockReady_ = true;
		}
	else if (PacketNotLast != type)
		{ // not a legal type
		SetPacketError(state,PacketErrorType);
		return;
		}

	++state->packetSequence_; // decoded correctly, increment
	++state->packetCount_;    // another packet decoded
	state->packetPos_ = 0;    // reset this
	} // PacketDecode

// passes data into the packet decoding system
// returns number of unprocessed bytes, which are likely the next packet. 
// After processing the packet data, the data is prepared to be read by 
// PacketGetData. 
// If embedded text is found, return textLength which is the number of text 
// characters AFTER the other bytes were processed, and this count is not 
// included in the unprocessed byte count.
uint16 PacketDecodeBytes(PacketHandlerState * state, const uint8 * data, uint16 length)
	{
	if (true == state->dataBlockReady_)
		return length; // done - this needs to be handled before any more work can be done
	while (length)
		{
		uint8 byte = *data++;
		length--;
		++state->byteCount_; // one more eaten
		if (0 == (state->syncCounter_&1))
			{ // first byte better be a SYNC byte, else an error
			if (PacketSYNC != byte)
				{ // we are in a text stream, count em out and return
				// walk until buffer done or next byte is sync, throwing out bytes
				while ((length>0) && (*(data+1) != PacketSYNC))
					{
					length--;
					data++;
					++state->byteCount_;
					}			
				SetPacketError(state,PacketErrorSYNC);
				return length;
				}
			++state->syncCounter_;
			// check if next byte is sync, in which case we're off by one
			// so resync to the stream
			if ((PacketSYNC == *data) && (length>0))
				{
				byte = *data++;
				length--;
				++state->byteCount_;
				}
			// now in packet until next SYNC byte
			}
		else
			{ // between SYNCs:
			  // copy characters into buffer until second SYNC, or until full, in which case an error
			  // unpack ESCaped characters along the way
			if (PacketSYNC == byte)
				{ // second SYNC, if no errors, process packet
				++state->syncCounter_;
				if (PacketErrorNone == state->packetError_)
					{
					PacketDecode(state); // decode and place into proper place
					if (true == state->dataBlockReady_)
						return length; // done - this needs to be handled before any more work can be done
					}
				else
					return length; // if in an error state, return
				}
			else if (state->packetPos_ >= sizeof(state->packetData_))
				{ // overflow
				SetPacketError(state,PacketErrorOverflow);
				return length;
				}
			else
				{ // regular byte, store it
				state->packetData_[state->packetPos_++] = byte;
				}
			}
		}
	return 0;
	} // PacketDecodeBytes


// see if a packet is ready, returns true iff one is ready
// sets a pointer to the decoded data
bool PacketGetData(PacketHandlerState * state, uint8 * destination, uint8 ** data, uint16 * length)
	{
	if (false == state->dataBlockReady_)
		{
		*destination = 0; // default values
		*data  = 0;
		*length = 0;
		return false; // nothing to do
		}

	// fill in fields
	*destination = state->decodeDestination_;
	*data        = state->decodeData_;
	*length      = state->decodeLength_;
	
	// mark relevant internals as reset
	state->decodeLength_ = 0; // restart
	state->dataBlockReady_ = false;
	state->packetSequence_ = 0;
	
	return true;
	} // PacketGetData

// send a block of data of given length
// to the destination item (0 = broadcast)
// return true iff sent ok
bool PacketSendData(PacketHandlerState * state, void (*IOWriteByte)(void * param, uint8), void * ioParam, uint8 destination, const uint8 * data, uint16 length)
	{
	uint8 buffer[PacketOverhead+PacketPayLength+1]; // space for constructing a single packet
	uint8 sequence = 0; // number of packets sent this data block
	while (length > 0)
		{
		uint16 pos;       // general counter
		uint16 dest;      // where we are in the buffer
		uint16 curLength; // send to sent this packet
		uint16 crc;

		dest = 0; // start of packet data
		
		// compute length of data to send this packet
		curLength = length;
		if (curLength > PacketPayLength)
			curLength = PacketPayLength; // maximum length
		length -= curLength; // remaining is amount for later packets
	
		// create the block to send without the sync wrappers
		if (length != 0)
			buffer[dest] = PacketNotLast<<5;
		else
			buffer[dest] = PacketLast<<5;
		buffer[dest++] |= (sequence&PacketSequenceMask); // set type and sequence
		buffer[dest++] = (uint8)(curLength&255); // set length of data
		buffer[dest++] = destination;            // item id to talk to

		while (curLength--)
			buffer[dest++] = *data++;            // copy data bytes to packet

		// now checksum the packet
		crc = CRC16(buffer,dest);
		buffer[dest++] = (crc>>8);  // MSB
		buffer[dest++] = (crc&255); // LSB
		state->packetEncodedCRC_ = crc; // save this

		assert(dest < sizeof(buffer));

		// now send data, using SYNC and ESC bytes as needed
		IOWriteByte(ioParam,PacketSYNC); // initial SYNC
		for (pos = 0; pos < dest; ++pos)
			{
			if (PacketSYNC == buffer[pos])
				{ // replace with ESC, ESC+1
				IOWriteByte(ioParam,PacketESC);
				IOWriteByte(ioParam,PacketESC+1);
				}
			else if (PacketESC == buffer[pos])
				{ // replace with ESC, ESC+2
				IOWriteByte(ioParam,PacketESC);
				IOWriteByte(ioParam,PacketESC+2);
				}
			else
				IOWriteByte(ioParam,buffer[pos]);
			} 
		IOWriteByte(ioParam,PacketSYNC); // final SYNC

		// todo - how to deal with timeouts?
		++sequence; // next packet

//		::Sleep(100); // todo - remove, or fix timing elsewhere - helps with buffer overflow on PIC?

		} // while packets left to send
	assert(0 == length);
	return true;
	} // PacketSendData

// read this to see if there is any errors before getting or sending packet data
PacketError PacketGetError(PacketHandlerState * state)
	{
	return state->packetError_;
	} // PacketGetError

// get count of bytes decoded
uint32 PacketByteCount(PacketHandlerState * state)
	{
	return state->byteCount_;
	} // PacketByteCount

// get count of packets seen
uint32 PacketCount(PacketHandlerState * state)
	{
	return state->packetCount_;
	} // PacketCount

// get count of packet errors seen
uint32 PacketErrorCount(PacketHandlerState * state)
	{
	return state->errorCount_;
	} // PacketErrorCount

// sequence counter for previous packet
uint8 PacketSequence(PacketHandlerState * state)
	{
	return state->packetSequence_;
	}

// CRC for previous packet (encoded or decoded)
uint16 PacketCRC(PacketHandlerState * state, bool decoded)
	{
	if (true == decoded)
		return state->packetDecodedCRC_;
	return state->packetEncodedCRC_;
	}

// clear the internal error state
void PacketClearError(PacketHandlerState * state)
	{
	state->packetError_ = PacketErrorNone;
	state->syncCounter_ = 0;
	state->packetPos_   = 0;
	state->packetSequence_ = 0; 
	state->decodeLength_ = 0;
	state->dataBlockReady_ = false;
	}

#ifdef WIN32
};  // namespace HypnoGadget
#endif // WIN32

// end - Packet.cpp

