// header for some helper classes for the HypnoDemo
// Copyright Chris Lomont 2008
#ifndef HYPNODEMO_H
#define HYPNODEMO_H

#include <windows.h> // stuff for COM ports
#include <string>
#include <iostream>
#include "Gadget.h"  // the gadget interface


/* We need two classes to talk with the gadget. The first one is a mutex lock
   for multithreaded applications. Since we are not multithreaded, this class is 
   empty.
   NOTE: Since we are not multithreaded, we need to call gadget.Update() quite 
   often to process bytes. If we were mutithreaded, we'd create a thread to do 
   this loop continuously for us.
*/
class DemoGadgetLock : public HypnoGadget::GadgetLock
	{
public:
	void Lock(void) {};	   // do nothing
	void Unlock(void) {};  // do nothing
	};

/* The second class handles reading and writing bytes from the COM (serial) port.
   While we're at it we may as well encapsulate the Win32 COM port here for
   simplicity.
*/
class DemoGadgetIO : public HypnoGadget::GadgetIO
	{ 
public:
	// default constructor
	DemoGadgetIO(void) : handlePort_(INVALID_HANDLE_VALUE) {};

	// default destructor
	~DemoGadgetIO(void)
		{
		Close();
		Sleep(10);
		}

	// try to open a com port, given its name
	// return true on success, else false
	bool Open(const std::string & portName)
		{
		handlePort_ = CreateFileA(          // we force this to be the ASCII version
			portName.c_str(),               // port device, e.g., "COM1"
			GENERIC_READ | GENERIC_WRITE,   // device mode
			0,                              // device not shared
			NULL,                           // default security
			OPEN_EXISTING,                  // what to do to the file (port)
			0,                              // default
			NULL);                          // default

		// see if it opened
		if (INVALID_HANDLE_VALUE == handlePort_)
			{
			std::cerr << "Error: could not open port " << portName << std::endl;
			return false;
			}

		// get communication parameters
		DCB config;
		if (0 == GetCommState(handlePort_,&config))
			{
			std::cerr << "Error: GetCommState failed.\n";
			return false;
			}
		
		// set communication parameters we want
		config.BaudRate = CBR_38400;  
		config.StopBits = ONESTOPBIT;  
		config.Parity   = NOPARITY;
		config.ByteSize = 8;  

		if (0 == SetCommState(handlePort_,&config))
			{
			std::cerr << "Error: SetCommState failed.\n";
			return FALSE;
			}

		
		// final step - set timeout info
		COMMTIMEOUTS timeOut;
		timeOut.ReadIntervalTimeout = 3;
		timeOut.ReadTotalTimeoutMultiplier = 3;
		timeOut.ReadTotalTimeoutConstant = 2;
		timeOut.WriteTotalTimeoutMultiplier = 3;
		timeOut.WriteTotalTimeoutConstant = 2;

		// set the time-out parameter into device control.
		if (0 == SetCommTimeouts(handlePort_,&timeOut))
			{
			std::cerr << "Error: SetCommTimeouts failed.\n";
			return false;
			}

		return true;
		} // Open

	// close any open port
	void Close(void)
		{
		if ((INVALID_HANDLE_VALUE != handlePort_) && (0 == CloseHandle(handlePort_)))
			{
			DWORD err = GetLastError();
			std::cerr << "Error: Port Closing failed, error " << err << std::endl;
			}
		handlePort_ = INVALID_HANDLE_VALUE;
		} // Close

	// Needed by the GadgetControl class
	// read bytes from the IO buffer to be processed
	// of max given length, and return size read
	HypnoGadget::uint16 ReadBytes(HypnoGadget::uint8 * buffer, HypnoGadget::uint16 length)
		{
		DWORD bytesRead;

		if (0 == ReadFile(
			handlePort_,    // handle of file to read
			buffer,         // buffer to read into
			length,         // number of bytes to read
			&bytesRead,     // pointer to number of bytes read
			NULL))
			{
			std::cerr << "Error: Reading from serial port failed.\n";
			return 0;
			}		
		return (HypnoGadget::uint16)bytesRead;
		}

	// Needed by the GadgetControl class
	// write bytes to the IO buffer
	void WriteBytes(const HypnoGadget::uint8 * buffer, HypnoGadget::uint16 length)
		{
		DWORD writtenBytes;
		if (0 == WriteFile(
			handlePort_,     // handle to file to write to
			buffer,          // pointer to data to write to file
			length,          // number of bytes to write
			&writtenBytes,	 // pointer to number of bytes written
			NULL))           
			{
			std::cerr << "Error: Writing to serial port failed\n";
			}		
		} // WriteBytes


private:

	HANDLE handlePort_; // a handle to a COM port we read/write as a file
	}; // DemoGadgetIO 

#endif HYPNODEMO_H
// end - HypnoDemo.h