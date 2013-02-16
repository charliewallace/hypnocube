// HypnoCOMM - serial communications for the HypnoGadgets
// Copyright Chris Lomont 2007-2008
// www.HypnoCube.com, www.HypnoSquare.com
// header for controlling the Hypno* gadget
#ifndef GADGET_H
#define GADGET_H

#include "defines.h"
#include "Options.h"
#include <string>

namespace HypnoGadget {

// abstract base class that handles reading and writing bytes
// must be threadsafe internally
class GadgetIO
	{ 
public:

	// read bytes from the IO buffer to be processed
	// of max given length, and return size read
	virtual uint16 ReadBytes(uint8 * buffer, uint16 length) = 0;

	// write bytes to the IO buffer
	virtual void WriteBytes(const uint8 * buffer, uint16 length) = 0;

	};

// for thread locking
// Derive from this class, creating mutex with a lock and unlock command
// if your program is not multithreaded your derived class can do nothing
class GadgetLock
	{
public:
	virtual void Lock(void) = 0;
	virtual void Unlock(void) = 0; 
	};

/* the main class that controls the gadget                                          */
/* Needs an IO object derived from GadgetIO for communications with the device      */
/* Needs a thread locking class derived from GadgetLock if you want multi-threaded  */
/* usually a thread (thread A) loops over the Update function, and thread B is the  */
/* User Interface. Bytes from the serial port must be fed into the GadgetIO derived */
/* class, and the gadget object writes bytes back out GadgetIO                      */
class GadgetControl
	{
public:

	// constructor/destructor
	GadgetControl(GadgetIO & gadgetIO, GadgetLock & lock);
	~GadgetControl(void);

	// the gadget is in Console mode (bytes going back and forth are treated as text)
	// or Packet mode (bytes are binary data)
	// Default is Console mode
	// Packet mode starts whenever a SYNC character is transferred and ends when SYNC
	// ends the packet, unless logged in, in which case packet mode until logged out.
	// Console mode returns on a Logout command
	enum ByteMode
		{
		ConsoleMode,   
		PacketMode
		};

	// read the state of the gadget
	ByteMode GetByteMode(void) const;

	// state of the gadget
	// state of the serial connection and login information
	enum LoginState {
		Disconnected, // no serial connection yet
		LoggedOut,    // logged out (console mode active)
		LoggedIn      // completely logged in (packet mode)
		};

	// read/write state of the gadget
	LoginState GetState(void) const;
	void SetState(LoginState state); // todo - external should not be able to set internal states?

	// console bytes are logged, this retrieves a copy of the current log
	void ConsoleLog(std::string & text);
	// only store so many characters (0 for infinite, default 10000)
	void ConsoleReset(uint32 size);
	// clear the console text
	void ConsoleClear(void);

	// get message from log
	// -1 index resets the log
	// return true iff valid index
	// max length is 1000 lines
	bool GetMessage(std::string & message, int index);


	// return true if there has been an error, get the last error message
	// resets error message
	bool Error(std::string & errMsg);

	// process commands being sent back and forth to the gadget
	// call fairly often
	// call on thread A, all other functions call from thread B.
	void Update(void);

	// Here is the ability to read and write options as a block
	// get/set a copy of the options stored in the class
	// to get them from the device, use the Options command
	bool GetOptions(Options & opts); // return true if the options have been loaded internally
	void SetOptions(const Options & opts);

	// text info, available after appropriate Info commands sent
	std::string GetDescription(void);
	std::string GetDevice(void);
	std::string GetCopyright(void);
	
	// get versions after Version called
	enum VersionType
		{
		SoftwareVersion,HardwareVersion,ProtocolVersion
		};
	void GetVersion(VersionType type, uint8 & major, uint8 & minor);

	enum InfoType
		{
		VisualizationType, TransitionType
		};
	// get the count of items loaded
	uint8 GetCount(InfoType type);
	// get 0 numbered item, return blank string if out of bounds
	void GetName(InfoType type, std::string & name, uint8 index);

	// return true if a frame ready to read
	// resets internal flag when read
	// returns pointer to internal buffer and size of buffer
	bool GetFrame(uint8 ** buffer, int & size);

	// commands that can be called on the gadget
	void Login(uint32 val = 0xABADC0DE); // Login with given challenge value, default
	void Logout(void);
	void GetFrame(void);	// todo - describe all, order by spec
	void MaxVisIndex(void);
	void SelectVis(uint8 vis);
	void MaxTranIndex(void);
	void SelectTran(uint8 trans);
	void Version(void);
	void Info(uint8 type, uint8 index);
	void Ping(void);
	void Reset(void);
	void Options(bool write);
	void SetFrame(const uint8 * buffer);
	void FlipFrame(void);

	class GadgetImpl;
private:
	GadgetImpl * pImpl_;
	mutable GadgetLock & lock_;
	
	// get, release lock for threading
	void Lock(void) const
		{
		lock_.Lock();
		}
	void Unlock(void) const
		{
		lock_.Unlock();
		}

	}; // class GadgetControl

	}; // namespace HypnoGadget
#endif // GADGET_H
// end - Gadget.h