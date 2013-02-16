// HypnoCOMM - serial communications for the HypnoGadgets
// Copyright Chris Lomont 2007-2008
// www.HypnoCube.com, www.HypnoSquare.com
// basic types to make gadget code portable
#ifndef DEFINES_H
#define DEFINES_H

#ifdef WIN32
namespace HypnoGadget {
#endif // WIN32

typedef unsigned long  uint32;
typedef unsigned short uint16;
typedef unsigned char  uint8;

#ifdef WIN32
}; // namespace
#endif // WIN32

#endif // defines.h
// end - defines.h