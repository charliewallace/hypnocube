// code to demonstrate the interface to the HypnoGadget library
// Copyright 2008 Chris Lomont 
// Visual Studio 2005 C++

// Compile as a console program

#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <conio.h>
#include <ctime>
#include <time.h>
//#include <WinBase.h>

#include "HypnoDemo.h" // include helper classes
#include "Gadget.h"    // include this to access the HypnoCube, HypnoSquare, etc.

using namespace std;
using namespace HypnoGadget; // the gadget interface is in this namespace

#pragma comment(lib,"winmm") // includes timeGetTime() from winmm.lib

/* set pixel function to demonstrate how to 
   put a pixel in the buffer for the cube
   */
void SetPixelCube(
		int i, int j, int k, // coordinates
		unsigned char red,   // colors
		unsigned char green, // colors
		unsigned char blue,  // colors
		uint8 * buffer       // where to draw
		)
{
	// each pixel on the gadget is RGB values 0-15, and packed
	// so each three bytes contains two pixels as in R1G1 B1R2 G2B2
	if ((i < 0) || (3 < i) || (j < 0) || (3 < j) || (k < 0) || (3 < k))
		return; // nothing to do

	j = 3-j; // reverse j to get right hand coord system used in cube

	buffer += k*24; // 3/2 bytes per pixel * 16 pixels per level  = 24 bytes
	buffer += i*6;  // 3/2 bytes per pixel * 4  pixels per column =  6 bytes
	buffer += 3*(j>>1);   // 3/2 bytes per pixel

	if (j&1)
	{ // odd pixel, ..R2 G2B2 above
		++buffer;            // next byte
		*buffer &= 0xF0;     // mask out low bits
		*buffer |= (red>>4); // set high red nibble to low part of byte
		++buffer;            // next byte
		// set high green nibble and high blue nibble
		*buffer  = (green&0xF0)|(blue>>4); 
	}
	else
	{ // even pixel, R1G1 B2.. above
		// set high red nibble and high green nibble
		*buffer  = (red&0xF0)|(green>>4); 
		++buffer;            // next byte
		*buffer &= 0x0F;     // mask out high bits
		*buffer |= (blue&0xF0); // set high blue nibble to high part of byte
	}			   
} // SetPixelCube




// This utility is used for "diagonal" clocks. Since the top of the 4x4x4 cube has
// 12 LEDs around the upper plane, you can do a one-to-one mapping and select a single
// led to corrspond to the hour (or min or sec) numeral of an analog clock.
// Since there's no led in the middle of a side, we need to turn the square 45 degrees
// so it forms a diamond, then the top can represent 12 on the analog clock.
//
// This maps only one of the 3 inputs (hour OR minute OR second) to one of the 
// 12 vertical 'z-columns' following the edge of the top plane of the hypnocube.
//
// We will convert the first input that's not -1.  If hour is provided it should be 24-hour.
// In addition we return the mrow and mcol, coords of one of the 4 z-columns in the middle,
// the one closest to the selected z-column.
//   
// The 12-oclock column is in the corner at back left when the 
// red power button is towards you;  rows run towards you 0 to 3, cols run 0 to 3 
// horiz left to right. "z-columns" are vertical, and run top to bottom 0 to 3.
void GetHandPos_Diagonal(int hour, int min, int sec, int* pRow, int* pCol, int* pMRow, int* pMCol)
{
	int idx = 0;
	if (hour >= 0)
	{
		idx = hour; // 24-hour time.  convert to 0-11
		if (idx>=12)
		{
			idx -= 12;
		}
	}
	else if (min >= 0) // incomng range is 0-59
	{
		idx = min/5;
	}
	else // use second
	{
		idx = sec/5;
	}

	int row, col, mrow, mcol;
	switch(idx)
	{
	case 0: // the 12 numeral of the clock.  Back left corner of cube as seen from button side.
		row = 0;
		col = 0;
		mrow = 1;
		mcol = 1;
		break;
	case 1:
		row = 1;
		col = 0;
		mrow = 1;
		mcol = 1;
		break;
	case 2:
		row = 2;
		col = 0;
		mrow = 2;
		mcol = 1;
		break;
	case 3:
		row = 3;
		col = 0;
		mrow = 2;
		mcol = 1;
		break;
	case 4:
		row = 3;
		col = 1;
		mrow = 2;
		mcol = 1;
		break;
	case 5:
		row = 3;
		col = 2;
		mrow = 2;
		mcol = 2;
		break;
	case 6:
		row = 3;
		col = 3;
		mrow = 2;
		mcol = 2;
		break;
	case 7:
		row = 2;
		col = 3;
		mrow = 2;
		mcol = 2;
		break;
	case 8:
		row = 1;
		col = 3;
		mrow = 1;
		mcol = 2;
		break;
	case 9:
		row = 0;
		col = 3;
		mrow = 1;
		mcol = 2;
		break;
	case 10:
		row = 0;
		col = 2;
		mrow = 1;
		mcol = 2;
		break;
	case 11:
		row = 0;
		col = 1;
		mrow = 1;
		mcol = 1;
		break;
	default:
		row = 0;
		col = 0;
		mrow = 0;
		mcol = 0;
	}

	*pRow = row;
	*pCol = col;

	*pMRow = mrow;
	*pMCol = mcol;

	return;
}

// Get Hand Position - for front-facing clock
// This maps only one of the 3 inputs (hour OR minute OR second) to one of the 
// digits of an analog clock; we will convert the first input that's not -1.  If hour is provided it should be 24-hour.
// Output is the positions of the 4 leds used to indicate the simulated hand position, returned via passed pointers.
// Since there is no LED in the middle of the top row, we use the two leds closest to center to indicate 12; and this
// set of two progresses around the edge of the vertical 4x4 plane to indicate the specified analog clock digit.
// IN addition to the two leds on the edge of the plane, we include 2 more inboard of the edge, for a total of 4. 
// These are in a square arrangement for 12, 3, 6, and 9, otherwise a zigzag trying to approximate a hand.
//
// ---- looking from side opposite red power button, vert row planes go rt to left (not front to back),
// ---- and col planes go back to front. 
//
// Defines the front-facing plane of leds as the one you see if the red power button is on the left.
// From that side, the vertical plane in back is col plane 0, col planes run back-to-front.  
// Vertical row planes run right to left, and horiz z-planes run bottom to top.
// The generated output doesn't specify the row plane; only the position in that plane, specified by the col-plane and the z-plane.
//
// Sublety: the order in which the leds are provided is the order they will light up as the between-clock-numerals
// increments proceed - like the 5 seconds between the 1 and the 2.  In the case of the 12 we have a square, and
// the leds should begin on the inside (closer to center) and should proceed clockwise.
void GetHandPosInRowPlane(int hour, int min, int sec, 
						   int* pLed1ColPlane, int* pLed1Zplane, 
						   int* pLed2ColPlane, int* pLed2Zplane,
						   int* pLed3ColPlane, int* pLed3Zplane,
						   int* pLed4ColPlane, int* pLed4Zplane)
{
	int idx = 0;
	if (hour >= 0)
	{
		idx = hour; // 24-hour time.  convert to 0-11
		if (idx>=12)
		{
			idx -= 12;
		}
	}
	else if (min >= 0) // incomng range is 0-59
	{
		idx = min/5;
	}
	else // use second
	{
		idx = sec/5;
	}

	int col1, col2, col3, col4;
	int z1, z2, z3, z4;
	z1 = col1 = z2 = col2 = z3 = col3 = z4 = col4 = 0;


	switch(idx)
	{
	case 0: // the 12 numeral of the clock.  Back left corner of cube as seen from button side.
		z1 = 3;
		col1 = 1;
		z2 = 3;
		col2 = 2;
		z3 = 2;
		col3 = 2;
		z4 = 2;
		col4 = 1;		
		break;
	case 1:
		z1 = 3;
		col1 = 0;
		z2 = 3;
		col2 = 1;
		z3 = 2;
		col3 = 1;
		z4 = 2;
		col4 = 2;
		break;
	case 2:
		z1 = 3;
		col1 = 0;
		z2 = 2;
		col2 = 0;
		z3 = 2;
		col3 = 1;
		z4 = 1;
		col4 = 1;
		break;
	case 3:
		z1 = 1;
		col1 = 0;
		z2 = 2;
		col2 = 0;
		z3 = 2;
		col3 = 1;
		z4 = 1;
		col4 = 1;
		break;
	case 4:
		z1 = 0;
		col1 = 0;
		z2 = 1;
		col2 = 0;
		z3 = 1;
		col3 = 1;
		z4 = 2;
		col4 = 1;
		break;
	case 5:
		z1 = 0;
		col1 = 0;
		z2 = 0;
		col2 = 1;
		z3 = 1;
		col3 = 1;
		z4 = 1;
		col4 = 2;
		break;
	case 6:
		z1 = 0;
		col1 = 2;
		z2 = 0;
		col2 = 1;
		z3 = 1;
		col3 = 1;
		z4 = 1;
		col4 = 2;
		break;
	case 7:
		z1 = 0;
		col1 = 3;
		z2 = 0;
		col2 = 2;
		z3 = 1;
		col3 = 2;
		z4 = 1;
		col4 = 1;
		break;
	case 8:
		z1 = 0;
		col1 = 3;
		z2 = 1;
		col2 = 3;
		z3 = 1;
		col3 = 2;
		z4 = 2;
		col4 = 2;
		break;
	case 9:
		z1 = 2;
		col1 = 3;
		z2 = 1;
		col2 = 3;
		z3 = 1;
		col3 = 2;
		z4 = 2;
		col4 = 2;
		break;
	case 10:
		z1 = 3;
		col1 = 3;
		z2 = 2;
		col2 = 3;
		z3 = 2;
		col3 = 2;
		z4 = 1;
		col4 = 2;
		break;
	case 11:
		z1 = 3;
		col1 = 3;
		z2 = 3;
		col2 = 2;
		z3 = 2;
		col3 = 2;
		z4 = 2;
		col4 = 1;
		break;
	default:
		z1 = 0;
		col1 = 0;
		z2 = 0;
		col2 = 0;
		z3 = 0;
		col3 = 0;
		z4 = 0;
		col4 = 0;
	}

	*pLed1ColPlane = col1;
	*pLed1Zplane = z1;

	*pLed2ColPlane = col2;
	*pLed2Zplane = z2;

	*pLed3ColPlane = col3;
	*pLed3Zplane = z3;

	*pLed4ColPlane = col4;
	*pLed4Zplane = z4;

	return;
}




// Draw a frame of animation on the gadget
void DrawFrame(GadgetControl & gadget, char theClockType, int updateCountThisSec)
{
	static int pos = 0; // position of pixel 0-63 - this drives this animation
	time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
	int hour = now->tm_hour;
	int minute = now->tm_min;
	int second = now->tm_sec;
	int tk = 0;
	static int lastSecond = 0;

	uint8 image[96]; // RGB buffer, 4 bits per color, packed

	// clear screen by setting all values to 0
	memset(image,0,sizeof(image));

	if (theClockType == '0')
	{
		//if (pos == 0)
		//{
		//	cout << "Press any key to continue: >>>\n";
		//	_getch();
		//}
		int x,y,z, ii;  // coordinates
		//for (ii=0; ii<=pos; ii++)
		for (ii=0; ii<=63; ii++)
		{
			x = ii/16; // convert ii 0-63 to x,y,z in [0,3]x[0,3]x[0,3]
			y = (ii/4)&3;
			z = ii&3;			
			//SetPixelCube(x,y,z,0,255,0,image); // green z axis
			SetPixelCube(x,y,z,x*x*28,y*y*28,z*z*28, image); // green z axis
		}

	}
	else if (theClockType == '1' || theClockType == '2')
	{

	/*		cout << (now->tm_year + 1900) << '-' 
			<< (now->tm_mon + 1) << '-'
			<<  now->tm_mday << ' '
			<< hour << ':' << minute << ':' << second
			<< endl; */

		int row=0, col=0;
		int mrow=0, mcol=0;

		// display the hour  ===================
		GetHandPos_Diagonal(hour, -1, -1, &row, &col, &mrow, &mcol);

		if (theClockType == '1')
		{
			for (int z = 0; z < 4; ++z)
			{
				SetPixelCube(col,row,z,0,255,0,image); // green z axis
				SetPixelCube(mcol,mrow,z,0,255,0,image); // green z axis
			}
		}
		else if (theClockType == '2')
		{
			for (int z = 0; z < 2; ++z)
			{
				SetPixelCube(col,row,z,0,255,0,image); // green z axis
				SetPixelCube(mcol,mrow,z,0,255,0,image); // green z axis
			}
		}

		// display the minute after the hour so it will take precedence===============
		// The GetHandPos utility maps the hour to one of the 12 vertical columns
		// of leds around the periphery of the cube, treating them like the 
		// numerals on an analog clock.  mrow/mcol are coords of the next 
		// led inward towards the middle of the cube.
		GetHandPos_Diagonal(-1, minute, -1, &row, &col, &mrow, &mcol);

		if (theClockType == '1')
		{
			// first fill out the entire minute hand in white. We'll overlay with current minute.
			for (int z =0; z <= 4; z++)
			{
				SetPixelCube(col,row,z, 255,255,255, image); // yellow z-col
				//SetPixelCube(mcol,mrow,z, 255,255,255, image); // yellow inner z-col
			}

			// draw in the leds in the returned z-column unless on last one.
			if (minute%5 > 0)
			{	
				for (int z =(4-minute%5); z <= 4; z++)
				{
					SetPixelCube(col,row,z, 255,0,0,image); // red z axis
					//SetPixelCube(mcol,mrow,z, 255,0,0,image); // red z axis
				}
			}
		}
		else if (theClockType == '2')
		{
			// For the minute we use the upper two planes.
			// first fill out the entire minute hand in white. We'll overlay with current minute.
			SetPixelCube(col,row,3, 255,255,255,image); // red z axis
			SetPixelCube(mcol,mrow,3, 255,255,255,image); // red z axis
			SetPixelCube(mcol,mrow,2, 255,255,255,image); // red z axis
			SetPixelCube(col,row,2, 255,255,255,image); // red z axis

			// draw in the leds in the returned z-column unless on last one.
			int minModulus = minute%5;
			//cout << "minModulus = " << minModulus << "\n";
			switch(minModulus)
			{
			case 4:
				SetPixelCube(col,row,2, 255,0,0,image); // red z axis
			case 3:
				SetPixelCube(mcol,mrow,2, 255,0,0,image); // red z axis
			case 2:
				SetPixelCube(mcol,mrow,3, 255,0,0,image); // red z axis
			case 1:
				SetPixelCube(col,row,3, 255,0,0,image); // red z axis
			case 0:
				break;
			}
		}


		// display the second last so it will take precedence ===================
		GetHandPos_Diagonal(-1, -1, second, &row, &col, &mrow, &mcol);

		// pre-paint entire second hand in yellow
		for (int z =0; z <= 4; z++)
		{
			SetPixelCube(col,row,z, 255,255,0, image); // yellow z-col
			//SetPixelCube(mcol,mrow,z, 255,255,0, image); // yellow inner z-col
		}

		// draw in the leds in the returned z-column unless on last one.
		if (second%5 > 0)
		{	
			for (int z =(4-second%5); z <= 4; z++)
			{
				SetPixelCube(col,row,z, 0,0,255,image); // blue z axis
				//SetPixelCube(mcol,mrow,z, 0,0,255,image); // blue inner z axis
			}
		}

		// each 1/12 second, advance to the next led running along outside edge of the bottom plane.
		GetHandPos_Diagonal(updateCountThisSec, -1, -1, &row, &col, &mrow, &mcol);
		SetPixelCube(col,row,0, 255,255,255, image); // yellow z-col


	}
	else if (theClockType == '3')
	{
		//cout << "Clock type 3: FrontClock >>>\n";
		//GetHandPosInRowPlane
		int row=0, col=0;
		int mrow=0, mcol=0;

		int col1=0, z1 = 0;

		int col2=0, z2 = 0;
		int col3=0, z3 = 0;
		int col4=0, z4 = 0;

		// I've chosen the 'front' of the cube to be the right side if viewed from
		// the red-button side of the cube.  With this 'front' facing toward you, the back plane
		// shows the hours, next closer is the minutes, then the second hand, then the 5-second 
		// indicator.

		// prefill the center 4 leds for the back 3 planes.
		//SetPixelCube(1,0,1, 255,0,255, image);
		//SetPixelCube(1,0,2, 255,0,255, image);
		//SetPixelCube(2,0,1, 255,0,255, image);
		//SetPixelCube(2,0,2, 255,0,255, image);

		//SetPixelCube(1,1,1, 255,0,0, image);
		//SetPixelCube(1,1,2, 255,0,0, image);
		//SetPixelCube(2,1,1, 255,0,0, image);
		//SetPixelCube(2,1,2, 255,0,0, image);

		//SetPixelCube(1,2,1, 0,0,255, image);
		//SetPixelCube(1,2,2, 0,0,255, image);
		//SetPixelCube(2,2,1, 0,0,255, image);
		//SetPixelCube(2,2,2, 0,0,255, image);

		// the front plane shows the current 5-second period within the minute
		// This divides the front plane into 5 4-led squares: one in each corner and 
		// the center.
		int sec0to4 = second % 5;
		switch (sec0to4)
		{
		case 0:
			SetPixelCube(1,3,1, 0,255,0, image);
			SetPixelCube(1,3,2, 0,255,0, image);
			SetPixelCube(2,3,1, 0,255,0, image);
			SetPixelCube(2,3,2, 0,255,0, image);
			break;
		case 1:
			SetPixelCube(3,3,3, 0,255,0, image);
			SetPixelCube(3,3,2, 0,255,0, image);
			SetPixelCube(2,3,3, 0,255,0, image);
			SetPixelCube(2,3,2, 0,255,0, image);
			break;		
		case 2:
			SetPixelCube(0,3,3, 0,255,0, image);
			SetPixelCube(1,3,3, 0,255,0, image);
			SetPixelCube(0,3,2, 0,255,0, image);
			SetPixelCube(1,3,2, 0,255,0, image);
			break;		
		case 3:
			SetPixelCube(1,3,1, 0,255,0, image);
			SetPixelCube(1,3,0, 0,255,0, image);
			SetPixelCube(0,3,1, 0,255,0, image);
			SetPixelCube(0,3,0, 0,255,0, image);
			break;		
		case 4:
			SetPixelCube(3,3,1, 0,255,0, image);
			SetPixelCube(3,3,0, 0,255,0, image);
			SetPixelCube(2,3,1, 0,255,0, image);
			SetPixelCube(2,3,0, 0,255,0, image);
			break;		
		default:
			break;		
		}

		// display the second hand  ===================
		GetHandPosInRowPlane(-1, -1, second, &col1, &z1, &col2, &z2, &col3, &z3, &col4, &z4);
		//cout << "Clock type 3: hour =" << hour << ", min =" << minute << ", second =" << second << " \n";

		// Draw the seconds hand in the 3rd-from-front plane.
		SetPixelCube(col1,2,z1, 255,255,0, image);
		SetPixelCube(col2,2,z2, 255,255,0, image);
		SetPixelCube(col3,2,z3, 255,255,0, image);
		SetPixelCube(col4,2,z4, 255,255,0, image);

		// change color based on second within the curent set of 5 seconds
		switch(sec0to4)
		{
		case 4:
			SetPixelCube(col1,2,z1, 0,0,255,image);
		case 3:
			SetPixelCube(col2,2,z2, 0,0,255,image);
		case 2:
			SetPixelCube(col3,2,z3, 0,0,255,image);
		case 1:
			SetPixelCube(col4,2,z4, 0,0,255,image);
		case 0:
			break;
		}

		// display the minute hand ===================
		GetHandPosInRowPlane(-1, minute, -1, &col1, &z1, &col2, &z2, &col3, &z3, &col4, &z4);

		SetPixelCube(col1,1,z1, 255,255,255, image);
		SetPixelCube(col2,1,z2, 255,255,255, image);
		SetPixelCube(col3,1,z3, 255,255,255, image);
		SetPixelCube(col4,1,z4, 255,255,255, image);

		int min0to4 = minute % 5;

		// change color based on second within the curent set of 5 seconds
		switch(min0to4)
		{
		case 4:
			SetPixelCube(col1,1,z1, 255,0,0,image);
		case 3:
			SetPixelCube(col2,1,z2, 255,0,0,image);
		case 2:
			SetPixelCube(col3,1,z3, 255,0,0,image);
		case 1:
			SetPixelCube(col4,1,z4, 255,0,0,image); 
		case 0:
			break;
		}

		// display the hour hand ===================
		GetHandPosInRowPlane(hour, -1, -1, &col1, &z1, &col2, &z2, &col3, &z3, &col4, &z4);

		SetPixelCube(col1,0,z1, 255,0,255, image);
		SetPixelCube(col2,0,z2, 255,0,255, image);
		SetPixelCube(col3,0,z3, 255,0,255, image);
		SetPixelCube(col4,0,z4, 255,0,255, image);

		int while0to4 = minute / 12;  // which set of 12 mins within the hour

		// change color based on second within the curent set of 5 seconds
		switch(while0to4)
		{
		case 4:
			SetPixelCube(col1,0,z1, 255,100,0,image);  // medium orange
		case 3:
			SetPixelCube(col2,0,z2, 255,100,0,image);
		case 2:
			SetPixelCube(col3,0,z3, 255,100,0,image);
		case 1:
			SetPixelCube(col4,0,z4, 255,100,0,image); 
		case 0:
			break;
		}

		// each 1/12 second, advance to the next led running along outside edge of the front plane.
		GetHandPos_Diagonal(updateCountThisSec, -1, -1, &row, &col, &mrow, &mcol);
		SetPixelCube(col,3,row, 255,255,255, image); // yellow z-col
	}
	else if (theClockType == '4')
	{
		//cout << "Clock type 3: FrontClock >>>\n";
		//GetHandPosInRowPlane
		int row=0, col=0;
		int mrow=0, mcol=0;

		int col1=0, z1 = 0;

		int col2=0, z2 = 0;
		int col3=0, z3 = 0;
		int col4=0, z4 = 0;

		// I've chosen the 'front' of the cube to be the right side if viewed from
		// the red-button side of the cube.  With this 'front' facing toward you, the back plane
		// shows the hours, next closer is the minutes, then the second hand, then the 5-second 
		// indicator.

		// prefill the center 4 leds for the back 3 planes.
		//SetPixelCube(1,0,1, 0,255,0, image);
		//SetPixelCube(1,0,2, 0,255,0, image);
		//SetPixelCube(2,0,1, 0,255,0, image);
		//SetPixelCube(2,0,2, 0,255,0, image);

		//SetPixelCube(1,1,1, 255,0,0, image);
		//SetPixelCube(1,1,2, 255,0,0, image);
		//SetPixelCube(2,1,1, 255,0,0, image);
		//SetPixelCube(2,1,2, 255,0,0, image);

		//SetPixelCube(1,2,1, 0,0,255, image);
		//SetPixelCube(1,2,2, 0,0,255, image);
		//SetPixelCube(2,2,1, 0,0,255, image);
		//SetPixelCube(2,2,2, 0,0,255, image);

		// the front plane shows the current 5-second period within the minute
		// This divides the front plane into 5 4-led squares: one in each corner and 
		// the center.
		int sec0to4 = second % 5;
		switch (sec0to4)
		{
		case 0:
			SetPixelCube(1,3,1, 255,255,0, image);
			SetPixelCube(1,3,2, 255,255,0, image);
			SetPixelCube(2,3,1, 255,255,0, image);
			SetPixelCube(2,3,2, 255,255,0, image);
			break;
		case 1:
			SetPixelCube(3,3,3, 255,255,0, image);
			SetPixelCube(3,3,2, 255,255,0, image);
			SetPixelCube(2,3,3, 255,255,0, image);
			SetPixelCube(2,3,2, 255,255,0, image);
			break;		
		case 2:
			SetPixelCube(0,3,3, 255,255,0, image);
			SetPixelCube(1,3,3, 255,255,0, image);
			SetPixelCube(0,3,2, 255,255,0, image);
			SetPixelCube(1,3,2, 255,255,0, image);
			break;		
		case 3:
			SetPixelCube(1,3,1, 255,255,0, image);
			SetPixelCube(1,3,0, 255,255,0, image);
			SetPixelCube(0,3,1, 255,255,0, image);
			SetPixelCube(0,3,0, 255,255,0, image);
			break;		
		case 4:
			SetPixelCube(3,3,1, 255,255,0, image);
			SetPixelCube(3,3,0, 255,255,0, image);
			SetPixelCube(2,3,1, 255,255,0, image);
			SetPixelCube(2,3,0, 255,255,0, image);
			break;		
		default:
			break;		
		}

		// display the second hand  ===================
		GetHandPosInRowPlane(-1, -1, second, &col1, &z1, &col2, &z2, &col3, &z3, &col4, &z4);
		//cout << "Clock type 3: hour =" << hour << ", min =" << minute << ", second =" << second << " \n";

		// Draw the seconds hand in the 3rd-from-front plane.
		SetPixelCube(col1,2,z1, 0,0,60, image);
		SetPixelCube(col2,2,z2, 0,0,60, image);
		SetPixelCube(col3,2,z3, 0,0,60, image);
		SetPixelCube(col4,2,z4, 0,0,60, image);

		switch(sec0to4)
		{
		case 4:
			SetPixelCube(col1,2,z1, 0,0,255,image);
		case 3:
			SetPixelCube(col2,2,z2, 0,0,255,image);
		case 2:
			SetPixelCube(col3,2,z3, 0,0,255,image);
		case 1:
			SetPixelCube(col4,2,z4, 0,0,255,image);
		case 0:
			break;
		}


		// display the minute hand ===================
		GetHandPosInRowPlane(-1, minute, -1, &col1, &z1, &col2, &z2, &col3, &z3, &col4, &z4);

		SetPixelCube(col1,1,z1, 60,0,0, image);
		SetPixelCube(col2,1,z2, 60,0,0, image);
		SetPixelCube(col3,1,z3, 60,0,0, image);
		SetPixelCube(col4,1,z4, 60,0,0, image);

		int min0to4 = minute % 5;

		// change color based on second within the curent set of 5 seconds
		switch(min0to4)
		{
		case 4:
			SetPixelCube(col1,1,z1, 255,0,0,image);
		case 3:
			SetPixelCube(col2,1,z2, 255,0,0,image);
		case 2:
			SetPixelCube(col3,1,z3, 255,0,0,image);
		case 1:
			SetPixelCube(col4,1,z4, 255,0,0,image); 
		case 0:
			break;
		}

		// display the hour hand ===================
		GetHandPosInRowPlane(hour, -1, -1, &col1, &z1, &col2, &z2, &col3, &z3, &col4, &z4);

		SetPixelCube(col1,0,z1, 0,60,0, image);
		SetPixelCube(col2,0,z2, 0,60,0, image);
		SetPixelCube(col3,0,z3, 0,60,0, image);
		SetPixelCube(col4,0,z4, 0,60,0, image);

		int while0to4 = minute / 12;  // which set of 12 mins within the hour

		// change color based on second within the curent set of 5 seconds
		switch(while0to4)
		{
		case 4:
			SetPixelCube(col1,0,z1, 0,255,0, image);
		case 3:
			SetPixelCube(col2,0,z2, 0,255,0, image);
		case 2:
			SetPixelCube(col3,0,z3, 0,255,0, image);
		case 1:
			SetPixelCube(col4,0,z4, 0,255,0, image); 
		case 0:
			break;
		}

		// each 1/12 second, advance to the next led running along outside edge of the front plane.
		GetHandPos_Diagonal(updateCountThisSec, -1, -1, &row, &col, &mrow, &mcol);
		SetPixelCube(col,3,row, 255,255,255, image); // yellow z-col
	}
	else if (theClockType == '5')
	{
		//cout << "Clock type 3: FrontClock >>>\n";
		//GetHandPosInRowPlane
		int row=0, col=0;
		int mrow=0, mcol=0;

		int col1=0, z1 = 0;

		int col2=0, z2 = 0;
		int col3=0, z3 = 0;
		int col4=0, z4 = 0;

		// I've chosen the 'front' of the cube to be the right side if viewed from
		// the red-button side of the cube.  With this 'front' facing toward you, the back plane
		// shows the hours, next closer is the minutes, then the second hand, then the 5-second 
		// indicator.

		// prefill the center 4 leds for the back 3 planes.
		//SetPixelCube(1,0,1, 0,255,0, image);
		//SetPixelCube(1,0,2, 0,255,0, image);
		//SetPixelCube(2,0,1, 0,255,0, image);
		//SetPixelCube(2,0,2, 0,255,0, image);

		//SetPixelCube(1,1,1, 255,0,0, image);
		//SetPixelCube(1,1,2, 255,0,0, image);
		//SetPixelCube(2,1,1, 255,0,0, image);
		//SetPixelCube(2,1,2, 255,0,0, image);

		//SetPixelCube(1,2,1, 0,0,255, image);
		//SetPixelCube(1,2,2, 0,0,255, image);
		//SetPixelCube(2,2,1, 0,0,255, image);
		//SetPixelCube(2,2,2, 0,0,255, image);

		// the front plane shows the current 5-second period within the minute
		// This divides the front plane into 5 4-led squares: one in each corner and 
		// the center.
		int sec0to4 = second % 5;
		switch (sec0to4)
		{
		case 0:
			SetPixelCube(1,3,1, 255,255,0, image);
			SetPixelCube(1,3,2, 255,255,0, image);
			SetPixelCube(2,3,1, 255,255,0, image);
			SetPixelCube(2,3,2, 255,255,0, image);
			break;
		case 1:
			SetPixelCube(3,3,3, 255,255,0, image);
			SetPixelCube(3,3,2, 255,255,0, image);
			SetPixelCube(2,3,3, 255,255,0, image);
			SetPixelCube(2,3,2, 255,255,0, image);
			break;		
		case 2:
			SetPixelCube(0,3,3, 255,255,0, image);
			SetPixelCube(1,3,3, 255,255,0, image);
			SetPixelCube(0,3,2, 255,255,0, image);
			SetPixelCube(1,3,2, 255,255,0, image);
			break;		
		case 3:
			SetPixelCube(1,3,1, 255,255,0, image);
			SetPixelCube(1,3,0, 255,255,0, image);
			SetPixelCube(0,3,1, 255,255,0, image);
			SetPixelCube(0,3,0, 255,255,0, image);
			break;		
		case 4:
			SetPixelCube(3,3,1, 255,255,0, image);
			SetPixelCube(3,3,0, 255,255,0, image);
			SetPixelCube(2,3,1, 255,255,0, image);
			SetPixelCube(2,3,0, 255,255,0, image);
			break;		
		default:
			break;		
		}

		// display the second hand  ===================
		GetHandPosInRowPlane(-1, -1, second, &col1, &z1, &col2, &z2, &col3, &z3, &col4, &z4);
		//cout << "Clock type 3: hour =" << hour << ", min =" << minute << ", second =" << second << " \n";

		// Draw the seconds hand in the 3rd-from-front plane.
		//SetPixelCube(col1,2,z1, 20,20,80, image);
		//SetPixelCube(col2,2,z2, 20,20,80, image);
		//SetPixelCube(col3,2,z3, 20,20,80, image);
		//SetPixelCube(col4,2,z4, 20,20,80, image);
		SetPixelCube(col1,2,z1, 0,0,255, image);
		SetPixelCube(col2,2,z2, 0,0,255, image);
		SetPixelCube(col3,2,z3, 0,0,255, image);
		SetPixelCube(col4,2,z4, 0,0,255, image);

		// change color based on second within the curent set of 5 seconds
		if (updateCountThisSec%2 > 0)
		{
			switch(sec0to4)
			{
			case 4:
				SetPixelCube(col1,2,z1, 0,0,255,image);
			case 3:
				SetPixelCube(col2,2,z2, 0,0,255,image);
			case 2:
				SetPixelCube(col3,2,z3, 0,0,255,image);
			case 1:
				SetPixelCube(col4,2,z4, 0,0,255,image);
			case 0:
				break;
			}
		}
		else
		{
			switch(sec0to4)
			{
			case 4:
				//SetPixelCube(col1,2,z1, 0,0,60,image);
				SetPixelCube(col1,2,z1, 20,20,120,image);
			case 3:
				//SetPixelCube(col2,2,z2, 0,0,60,image);
				SetPixelCube(col2,2,z2, 20,20,120,image);
			case 2:
				//SetPixelCube(col3,2,z3, 0,0,60,image);
				SetPixelCube(col3,2,z3, 20,20,120,image);
			case 1:
				//SetPixelCube(col4,2,z4, 0,0,60,image);
				SetPixelCube(col4,2,z4, 20,20,120,image);
			case 0:
				break;
			}
		}

		// display the minute hand ===================
		GetHandPosInRowPlane(-1, minute, -1, &col1, &z1, &col2, &z2, &col3, &z3, &col4, &z4);

		SetPixelCube(col1,1,z1, 255,0,0, image);
		SetPixelCube(col2,1,z2, 255,0,0, image);
		SetPixelCube(col3,1,z3, 255,0,0, image);
		SetPixelCube(col4,1,z4, 255,0,0, image);

		int min0to4 = minute % 5;

		// change color based on second within the curent set of 5 seconds
		if (updateCountThisSec%2 > 0)
		{
			switch(min0to4)
			{
			case 4:
				SetPixelCube(col1,1,z1, 255,0,0,image);
			case 3:
				SetPixelCube(col2,1,z2, 255,0,0,image);
			case 2:
				SetPixelCube(col3,1,z3, 255,0,0,image);
			case 1:
				SetPixelCube(col4,1,z4, 255,0,0,image); 
			case 0:
				break;
			}
		}
		else
			switch(min0to4)
			{
			case 4:
				SetPixelCube(col1,1,z1, 120,10,10,image);
			case 3:
				SetPixelCube(col2,1,z2, 120,10,10,image);
			case 2:
				SetPixelCube(col3,1,z3, 120,10,10,image);
			case 1:
				SetPixelCube(col4,1,z4, 120,10,10,image); 
			case 0:
				break;
			}

		// display the hour hand ===================
		GetHandPosInRowPlane(hour, -1, -1, &col1, &z1, &col2, &z2, &col3, &z3, &col4, &z4);

		SetPixelCube(col1,0,z1, 0,255,0, image);
		SetPixelCube(col2,0,z2, 0,255,0, image);
		SetPixelCube(col3,0,z3, 0,255,0, image);
		SetPixelCube(col4,0,z4, 0,255,0, image);

		int while0to4 = minute / 12;  // which set of 12 mins within the hour

		// change color based on second within the curent set of 5 seconds
		if (updateCountThisSec%2 > 0)
		{
			switch(while0to4)
			{
			case 4:
				SetPixelCube(col1,0,z1, 0,255,0, image);
			case 3:
				SetPixelCube(col2,0,z2, 0,255,0, image);
			case 2:
				SetPixelCube(col3,0,z3, 0,255,0, image);
			case 1:
				SetPixelCube(col4,0,z4, 0,255,0, image); 
			case 0:
				break;
			}
		}
		else
		{
			switch(while0to4)
			{
			case 4:
				SetPixelCube(col1,0,z1, 10,140,10, image);
			case 3:
				SetPixelCube(col2,0,z2, 10,140,10, image);
			case 2:
				SetPixelCube(col3,0,z3, 10,140,10, image);
			case 1:
				SetPixelCube(col4,0,z4, 10,140,10, image); 
			case 0:
				break;
			}		
		}

		// each 1/12 second, advance to the next led running along outside edge of the front plane.
		GetHandPos_Diagonal(updateCountThisSec, -1, -1, &row, &col, &mrow, &mcol);
		SetPixelCube(col,3,row, 255,255,255, image); // yellow z-col
	}
	else if (theClockType == '6')
	{
		// the Plane clock.  Each plane is a quarter of the circle, 15 min or 15 sec.
		// There a 16 per plane so one isn't used - it's the x=y=0 column, that's used
		// as the subsecond indicator

		int x,y,z, ii;  // coordinates
		int xMin,yMin,zMin, mm;  // coordinates

		// Fill in the cube in planes of 15 sec each.
		// Start ii at one so we actually will fill up the cube, othewise the last led
		// never gets turned on. So at start of minute the first led is already on.

		// treat second==zero as if it was 60 - we want all the leds on
		int altMin = minute;
		if (minute == 0)
		{
			altMin = 60;
		}
		// first calculate the index of the current minute, mm:
		mm = -1;
		for (int theMin=0; theMin <altMin; theMin++)
		{	
			mm++;
			zMin = mm/16; // convert ii 0-63 to x,y,z in [0,3]x[0,3]x[0,3]
			yMin = (mm/4)&3;
			xMin = mm&3;

			if (xMin==0 && yMin==0)
			{
				// the calculated position fell in he x=y=0 column that's reserved
				// for sub-second indicator.  Skip to next ii and recalc.
				mm++;
				zMin = mm/16; // convert ii 0-63 to x,y,z in [0,3]x[0,3]x[0,3]
				yMin = (mm/4)&3;
				xMin = mm&3;
			}	
			SetPixelCube(xMin,yMin,zMin, 0,0,255, image); 

		}
		//SetPixelCube(xMin,yMin,zMin, 0,0,255, image); 

		// at this point, mm is the index of the minute led.

		// treat second==zero as if it was 60 - we want all the leds on
		int altSec = second;
		if (second == 0)
		{
			altSec = 60;
		}

		ii=-1;
		for (int theSec=0; theSec <altSec; theSec++)
		{		
			ii++;
			z = ii/16; // convert ii 0-63 to x,y,z in [0,3]x[0,3]x[0,3]
			y = (ii/4)&3;
			x = ii&3;

			if (x==0 && y==0)
			{
				// the calculated position fell in he x=y=0 column that's reserved
				// for sub-second indicator.  Skip to next ii and recalc.
				ii++;
				z = ii/16; // convert ii 0-63 to x,y,z in [0,3]x[0,3]x[0,3]
				y = (ii/4)&3;
				x = ii&3;
			}


			if (ii <= mm)
			{
				SetPixelCube(x,y,z, 255,0,255, image); 
			}
			else 
			{
				SetPixelCube(x,y,z, 255,0,0, image); 
			}
		}
		int row=0, col=0, mrow=0, mcol=0;

		// display the hour as a single led along the edge of the top plane.
		GetHandPos_Diagonal(hour, -1, -1, &row, &col, &mrow, &mcol);
		SetPixelCube(col,row,3, 0,255,0, image); 

		// twinkle the current second and minute to make them easier to find.
		if (updateCountThisSec %2 > 0)
		{
			SetPixelCube(xMin,yMin,zMin, 0,0,60, image); 
			SetPixelCube(x,y,z, 60,0,0, image); 
		}


		// display the sub-second indicator updating 4 times a second.
		int howManyQuarterSecs = updateCountThisSec/3;
		SetPixelCube(0,0,howManyQuarterSecs, 255,255,255, image);
	}

	lastSecond = second;

	// next pixel for next frame of animation
	pos = (pos+1)&63; // count 0-63 and repeat
	//pos = (pos+1)&15; // count 0-63 and repeat

	// send the image
	gadget.SetFrame(image);

	// show the image
	gadget.FlipFrame();

} // DrawFrame


// attempt to login to a HypnoGadget.
// Return true on success, else return false
bool Login(GadgetControl & gadget)
	{
	// todo - try logging out and back in until works?
	gadget.Login();
	for (int pos = 0; pos < 100; ++pos)
		{
		gadget.Update(); // we need to do this ourselves since we are not multithreaded
		if (GadgetControl::LoggedIn == gadget.GetState())
			return true;
		Sleep(5);
		}

	return false;
	} // Login

// Run the gadget demo
// Assumes port is a string like COMx where x is a value
void RunDemo(const string & port)
{
	// Two classes we need to feed to the gadget control
	DemoGadgetIO     ioObj;	  // handles COM bytes
	DemoGadgetLock   lockObj; // handles threads

	// 1. Create a gadget
	GadgetControl gadget(ioObj,lockObj);

	// 2. Open the connection
	if (false == ioObj.Open(port))
	{
		cerr << "Error opening port " << port << endl;
		return;
	}

	// 3. Login to the gadget
	if (false == Login(gadget))
	{
		cerr << "Error: could not login to the gadget. Make sure you have the correct COM port.";
		ioObj.Close();
		return;
	}

	// 4. While no keys pressed, draw images


	// do not go faster than about 30 frames per second, 
	// or the device may lock up, so....
	unsigned long delay = 10;//1000/30; // milliseconds per frame
	char theKey = '\0';

	while (TRUE)
	{
		//cout << "Press any key to quit\n";
		cout << "|>=- H Y P N O  C L O C K -=<|\n";
		cout << "0:     Fill cube with all colors\n";
		cout << "1:     EarlyClock\n";
		cout << "2:     PaddleClock\n";
		cout << "3:     HandsClock Colorful\n";
		cout << "4:     HandsClock Monochrome\n";
		cout << "5:     HandsClock Blinky\n";
		cout << "6:     PlaneClock\n";
		cout << "q:     Quit\n";
		cout << "Enter: Quit\n";
		cout << ">>";
		theKey = _getch();
		if (theKey == 'q' || theKey=='Q' || theKey == 0x0D)
		{
			break;
		}

		clock_t last, finish;  // note, CLOCKS_PER_SEC should be predefined...
		last = clock();  // ticks since program launch
		const int UPDATES_PER_SEC = 12;
		const int TICKS_PER_UPDATE = CLOCKS_PER_SEC / UPDATES_PER_SEC;  // for me one clock is one ms
		finish = last + TICKS_PER_UPDATE;  // future tick count when it's time to update
		int updateCountThisSec = 0;  // counts the number of updates so far during current second

		// run the selected clock
		switch(theKey)
		{
		case '0':
			cout << "Running TestClock. Press any key to quit\n";

			// on each pass, this loop sets up the 64-led buffer and sends it to 
			// the cube.  Speed test results: it takes 3 seconds for the cube
			// to reach all-on.  that's 64 updates of 64 leds each.
			// So the time for a single update is just under 50ms.
			while (!_kbhit())
			{
				Sleep(20); // comment out for speed test

				// Draw a frame of the demo
				DrawFrame(gadget, theKey, 0);

				// Loop, processing read and written bytes
				// until time for next frame
				unsigned long start = timeGetTime();
				while (timeGetTime()-start < delay)
				{
					// be sure to call this often to process serial bytes
					gadget.Update(); 
					Sleep(1);
				}
			} 
			break;

		case '1':
			cout << "Running EarlyClock. Press any key to quit\n";

			while (!_kbhit())
			{
				// wait for time for next update

				// call drawframe only when clock() has progressed past finish;
				// repeat but let updateCountThisSec cycle
				while (clock() < finish)
				{
					// just wait
					Sleep(1);
				}

				// wait is finished; now advance the tick target 'finish' to 1/12 sec from now.
				finish = clock() + TICKS_PER_UPDATE;
				updateCountThisSec ++;
				if (updateCountThisSec >= UPDATES_PER_SEC)
				{
					updateCountThisSec = 0;
				}

				// Draw a frame of the demo
				DrawFrame(gadget, theKey, updateCountThisSec);

				// Loop, processing read and written bytes
				// until time for next frame
				unsigned long start = timeGetTime();
				while (timeGetTime()-start < delay)
				{
					// be sure to call this often to process serial bytes
					gadget.Update(); 
					Sleep(1);
				}
			} 
			break;

		case '2':
			cout << "Running PaddleClock. Press any key to quit\n";

			while (!_kbhit())
			{
				while (clock() < finish)
				{
					// just wait
					Sleep(1);
				}

				// wait is finished; now advance the tick target 'finish' to 1/12 sec from now.
				finish = clock() + TICKS_PER_UPDATE;
				updateCountThisSec ++;
				if (updateCountThisSec >= UPDATES_PER_SEC)
				{
					updateCountThisSec = 0;
				}

				// Draw a frame of the demo
				DrawFrame(gadget, theKey, updateCountThisSec);

				// Loop, processing read and written bytes
				// until time for next frame
				unsigned long start = timeGetTime();
				while (timeGetTime()-start < delay)
				{
					// be sure to call this often to process serial bytes
					gadget.Update(); 
					Sleep(1);
				}
			} 
			break;

		case '3':
			cout << "Running HandsClock colorful version. Press any key to quit\n";

			while (!_kbhit())
			{
				// wait for next tenth
				while (clock() < finish)
				{
					// just wait
					Sleep(1);
				}

				// wait is finished; now advance the tick target 'finish' to 1/12 sec from now.
				finish = clock() + TICKS_PER_UPDATE;
				updateCountThisSec ++;
				if (updateCountThisSec >= UPDATES_PER_SEC)
				{
					updateCountThisSec = 0;
				}

				// Draw a frame of the demo
				DrawFrame(gadget, theKey, updateCountThisSec);

				// Loop, processing read and written bytes
				// until time for next frame
				unsigned long start = timeGetTime();
				while (timeGetTime()-start < delay)
				{
					// be sure to call this often to process serial bytes
					gadget.Update(); 
					Sleep(1);
				}
			} 
			break;

		case '4':
			cout << "Running HandsClock Monochrome. Press any key to quit\n";

			while (!_kbhit())
			{
				// wait for next tenth
				while (clock() < finish)
				{
					// just wait
					Sleep(1);
				}

				// wait is finished; now advance the tick target 'finish' to 1/12 sec from now.
				finish = clock() + TICKS_PER_UPDATE;
				updateCountThisSec ++;
				if (updateCountThisSec >= UPDATES_PER_SEC)
				{
					updateCountThisSec = 0;
				}

				// Draw a frame of the demo
				DrawFrame(gadget, theKey, updateCountThisSec);

				// Loop, processing read and written bytes
				// until time for next frame
				unsigned long start = timeGetTime();
				while (timeGetTime()-start < delay)
				{
					// be sure to call this often to process serial bytes
					gadget.Update(); 
					Sleep(1);
				}
			} 
			break;


		case '5':
			cout << "Running HandsClock Blinky. Press any key to quit\n";

			while (!_kbhit())
			{
				// wait for next tenth
				while (clock() < finish)
				{
					// just wait
					Sleep(1);
				}

				// wait is finished; now advance the tick target 'finish' to 1/12 sec from now.
				finish = clock() + TICKS_PER_UPDATE;
				updateCountThisSec ++;
				if (updateCountThisSec >= UPDATES_PER_SEC)
				{
					updateCountThisSec = 0;
				}

				// Draw a frame of the demo
				DrawFrame(gadget, theKey, updateCountThisSec);

				// Loop, processing read and written bytes
				// until time for next frame
				unsigned long start = timeGetTime();
				while (timeGetTime()-start < delay)
				{
					// be sure to call this often to process serial bytes
					gadget.Update(); 
					Sleep(1);
				}
			} 
			break;

		case '6':
			cout << "Running Plane Clock. Press any key to quit\n";

			while (!_kbhit())
			{
				// wait for next tenth
				while (clock() < finish)
				{
					// just wait
					Sleep(1);
				}

				// wait is finished; now advance the tick target 'finish' to 1/12 sec from now.
				finish = clock() + TICKS_PER_UPDATE;
				updateCountThisSec ++;
				if (updateCountThisSec >= UPDATES_PER_SEC)
				{
					updateCountThisSec = 0;
				}

				// Draw a frame of the demo
				DrawFrame(gadget, theKey, updateCountThisSec);

				// Loop, processing read and written bytes
				// until time for next frame
				unsigned long start = timeGetTime();
				while (timeGetTime()-start < delay)
				{
					// be sure to call this often to process serial bytes
					gadget.Update(); 
					Sleep(1);
				}
			} 
			break;

		default:
			break;
		}
		
		cout << "\n";

		while (_kbhit()) 
			_getch(); // eat any keypresses
	}

	while (_kbhit()) 
		_getch(); // eat any keypresses

	// 5. Logout
	gadget.Logout();  // we assume it logs out
	for (int pos = 0; pos < 10; ++pos)
	{
		gadget.Update();  // we must call this to process bytes
		Sleep(10);        // slight delay for 
	}

	// 6. Close the connection
	ioObj.Close();
	Sleep(100);       // slight delay
} // RunDemo

// show the usage for the command line parameters
void ShowUsage(const string & programName)
	{
	cerr << "Usage: " << programName << " COMx\n";
	cerr << " Where COMx is the COM port with the gadget attached.\n";
	cerr << "Example: " << programName << " COM4\n";
	} // ShowUsage

// The program starts executing here (obviously...)
int main(int argc, char ** argv)
	{
	cout << "Visit www.HypnoCube.com or www.HypnoSquare.com for updates!\n";
	cout << "HypnoDemo version 1.0, March 2008, by Chris Lomont\n\n";
	if (2 != argc)
		{ // not enough command line parameters
		ShowUsage(argv[0]);
		exit(-1);
		}
	
	// get parameters
	string port(argv[1]);


	// clean them
	transform(port.begin(), port.end(), port.begin(), toupper);       // uppercase

	// sanity check the arguments
	if ((4 != port.length()) || ("COM" != port.substr(0,3)) || 
		(false == isdigit(port[3])) )
		{ // wrong command line parameters
		ShowUsage(argv[0]);
		exit(-2);
		}

	// finally - run the demo!
	RunDemo(port);


	return 0;
	} // main

// end - HypnoDemo.cpp