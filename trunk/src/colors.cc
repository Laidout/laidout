
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/************** colors.cc *************/


/*! \defgroup colors Color Management
 *
 * These are functions and classes related to color management...
 */

 // TODO
 //
 // this must be flexible enough to allow sets of colors (like the X11 color names),
 // spot colors that cannot be scaled, Registration color, and None color, which is like
 // an erase as opposed to simply being paper color...


 // Though XColor is used, it is used differently than X. X uses values [0..65535] always,
 // but it is more convenient to expect them as the current scaling(***??? is that wise), 
 // typically [0..255]. Also flag==1 is
 // used to signal the program that pixel still has to be computed for the values of red,gree,blue.
//  typedef struct {
//            unsigned long pixel;
//            unsigned short red, green, blue;
//            char flags;
//            char pad;
//       } XColor;
//
typedef XColor ScreenColor; 



//------------------------------- ColorPrimary -------------------------------
/*! \class ColorPrimary
 * \ingroup colors
 * \brief Defines a primary color of a color system.
 *
 * For instance, for an RGB red, name="red" or "R", minvalue, maxvalue might be 0,255.
 * The screencolor is a representation of how the color should appear on the computer screen.
 * Red, for instance would be (255,0,0) for TrueColor visuals in Xo
 *
 * ***Attributes would tell extra information about how to display the colors. For instance,
 * to simulate differences between transparent and opaque inks, you might specify 
 * reflectance/absorption values. These are separate from any alpha or tint values defined
 * elsewhere.
 * 
 * <pre>
 * ***
 *  // sample attributes would be:
 *  // 	Reflectance: 128,128,0 (these would be based on the min/max for system, so for [0..255], 0=0%, 255=100%
 *  // 	Absorption: 0,0,0
 * </pre> 
 */
class ColorPrimary
{
 public:
	char *name;
	int maxvalue;
	int minvalue;
	ScreenColor screencolor;
	
	int natts;   //*** this could be a ColorAttribute class, to allow ridiculously adaptable color systems
	char **atts; //    like being able to define a sparkle or metal speck fill pattern 
	             // for each i'th attribute, atts[2*i]==attribute name, atts[2*i8+1]==attribute value
};


//------------------------------- ColorSystem -------------------------------

/*! \class ColorSystem
 * \ingroup colors
 * \brief Defines a color system, like RGB, CMYK, etc.
 *
 * \code
 *  //ColorSystem::style:
 *  #define COLOR_ADDITIVE    (1<<0)
 *  #define COLOR_SUBTRACTIVE (1<<1)
 *  #define COLOR_SPOT        (1<<2)
 *  #define COLOR_ALPHAOK     (1<<3)
 *  #define COLOR_TINTOK      (1<<4)
 *  #define COLOR_SCALABLE    (1<<3|1<<4)
 * \endcode
 */
class ColorSystem
{
 public:
	char *name;
	HProfile iccprofile; //***
	unsigned int systemid;
	unsigned long style;
	int nprimaries;
	ColorPrimary *primaries;
};


//------------------------------- Color -------------------------------
/*! \class Color
 * \ingroup colors
 * \brief A base color, primaries only, without an alpha.
 */
class Color
{
 public:
	unsigned int system;
	int n; // num values, put here so you don't have to always look them up in system definition
	int *value; // the values for each primary
};


//------------------------------- ColorInstance -------------------------------
/*! \class ColorInstance
 * \ingroup colors
 * \brief A full instance of color, with a Color, plus alpha and tint.
 */
class ColorInstance
{
 public:
	Color *color;
	int alpha;
	int tint;
	char *instancename;
};



//------------------------------- ColorManager -------------------------------
#include <lcms.h>

/*! \class ColorManager
 * \ingroup colors
 * \brief Keeps track of colors and color systems.
 *
 * *** it will be ColorManager's responsibility to read in all the icc profiles,
 * and possibly create color systems from them(? is that reasonable??).
 *
 * Basics are rgb, cmy, cmyk, yuv, hsv.
 */
class ColorManager
{
 protected:
 public:
	const char *icc_path;
	
};
