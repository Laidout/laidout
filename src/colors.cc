//
// $Id$
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2004-2009 by Tom Lechner
//

***todo 
// **************************this file is not used yet!!**************;

#include <lcms.h>



namespace Laidout {


/*! \defgroup colors Color Management
 *
 * These are functions and classes related to color management...
 *
 * *** please note that no kind of color management is currently functional
 */

 // TODO
 //
 // this must be flexible enough to allow sets of colors (like the X11 color names),
 // spot colors that cannot be scaled, Registration color, None color, and knockout, which is like
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
	
	Attribute atts; //*** this could be a ColorAttribute class, to allow ridiculously adaptable color systems
					//    like being able to define a sparkle or metal speck fill pattern 
	               // for each i'th attribute
	ColorPrimary();
	virtual ~ColorPrimary();
};

ColorPrimary::ColorPrimary()
{
	name=NULL;
	maxvalue=minvalue=0;
}

ColorPrimary::~ColorPrimary()
{
	if (name) delete[] name;
}


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
	unsigned int systemid;
	unsigned long style;
	
	cmsHPROFILE iccprofile;
	PtrStack<ColorPrimary> primaries;

	ColorSystem();
	virtual ~ColorSystem();

	virtual Color *newColor(int n,...);
};

ColorSystem::ColorSystem()
{
	name=NULL;
	iccprofile=NULL;

}

ColorSystem::~ColorSystem()
{
	if (name) delete[] name;
	cmsCloseProfile(iccprofile);
	primaries.flush();
}


//------------------------------- Color -------------------------------
/*! \class Color
 * \ingroup colors
 * \brief A base color, primaries only, without an alpha.
 */
class Color
{
 public:
	unsigned int systemid;
	ColorSystem *system;
	int n; // num values, put here so you don't have to always look them up in system definition
	int *value; // the values for each primary

	~Color();
};

Color::~Color()
{
	if (system) system->dec_count();
}


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
	char *name;
};

//------------------------------ ColorSet ----------------------------------
/*! \class ColorSet
 * \brief Holds a collection of colors.
 *
 * This can be used for palettes, for instance.
 */
class ColorSet : public Laxkit::anObject, public LaxFiles::DumpUtility
{
 public:
	char *name;
	RefPtrStack<ColorInstance> colors;
	ColorSet();
	virtual ~ColorSet();
};

ColorSet::ColorSet()
{
	name=NULL;
}

ColorSet::~ColorSet()
{
	if (name) delete[] name;
}


//------------------------------- ColorManager -------------------------------

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
	PtrStack<char> icc_paths;
	
};


} // namespace Laidout

