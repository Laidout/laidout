//
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
/********** guides.cc ****************/

/*! \class Guide
 * \brief Things snap to guides.
 *
 * Guides in most programs are either vertical or horizontal lines. Guides in Laidout
 * can be any arbitrary path. Guide objects are used in a viewer to snap objects to them,
 * and they are also used as tabstops and margin indicators in text objects.
 * 
 * A guide can lock its rotation or its scale to be the same relative to the screen. The
 * guide's origin is always transformed with the view space.
 */
/*! \var unsigned int Guide::guidetype
 * \brief The type of guide.
 *
 * <pre>
 *  (1<<0) Keep same angle on screen
 *  (1<<1) Keep same scale on screen
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
