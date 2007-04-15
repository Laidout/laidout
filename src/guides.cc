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
/********** guides.cc ****************/

/*! \class Guide
 * \brief Things snap to guides.
 *
 * Guides in most programs are either vertical or horizontal lines. Guides in Laidout
 * can be any arbitrary single path. Guide objects are used in a viewer to snap objects to them,
 * and they are also used as tabstops and margin indicators in text objects.
 * 
 * A guide can lock its rotation, scale, or position to be the constant relative to its containing
 * object, which might be an object, a page, a scratch space, or the viewport itself. 
 *
 * This class is for 1-dimensional guides. See class Arrangement for 2-d "guides".
 */
/*! \var unsigned int Guide::guidetype
 * \brief The type of guide.
 *
 * <pre>
 *  (1<<0) Lock orientation
 *  (1<<2) Repeat the path after endpoints, if any
 *  (1<<3) Extend flat lines with tangent at endpoints if any.
 *  (1<<4) attract objects horizontally
 *  (1<<5) attract objects vertically
 *  (1<<6) attract objects perpendicularly
 * </pre>
 */
class Guide : public LaxInterfaces::SomeData
{
 public:
	unsigned int guidetype;
};
