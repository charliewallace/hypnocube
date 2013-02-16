// HypnoCOMM - serial communications for the HypnoGadgets
// Copyright Chris Lomont 2007-2008
// www.HypnoCube.com, www.HypnoSquare.com
// header for computing CRC16 over a fixed block of memory
#ifndef CRC16_H
#define CRC16_H

#include "defines.h"

#ifdef WIN32
namespace HypnoGadget {
#endif // WIN32

uint16 CRC16(const uint8 * data, uint16 bytes);

#ifdef WIN32
}; // namespace
#endif // WIN32


#endif
// end - CRC16.h