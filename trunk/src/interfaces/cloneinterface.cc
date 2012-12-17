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
// Copyright (C) 2011 by Tom Lechner
//



namespace Laidout {



// 0. rotate around p, by angle
// 1.  p1 translate by v
// 2.  pm translate by v, reflect every d perp to v
// 3.  pg glide reflection dir, reflection line
// 4.  cm 
// 5.  p2
// 6.  p2mm
// 7.  p2mg
// 8.  p2gg
// 9.  c2mm
// 10. p3
// 11. p3m1
// 12. p31m
// 13. p4
// 14. p4mm
// 15. p4gm
// 16. p6
// 17. p6mm
// 18. penrose p1: pentagons, pentagrams, rhombus, sliced pentagram
// 19. penrose p2: kite and dart
// 20. penrose p3: 2 rhombs
// uniform tilings
// uniform tiling colorings

// tiling along a path -- frieze patterns
// tiling along a circle
// wallpaper


/*! \class TilingOp
 * \brief Take one shape, and transform it to other places. See also Tiling.
 */
class TilingOp
{
  public:
	
	int id;
	Path celloutline;

	virtual int NumTransforms() = 0
	virtual Affine Transform(int which) = 0;
	virtual int isRecursive(int which);
	virtual int RecurseInfo(int which, int *numtimes, double *minsize, double *maxsize);
};

/*! \class Tiling
 * \brief Repeat any number of base shapes into a pattern.
 */
class Tiling
{
  public:
	PtrStack<TilingOp> basecells; //typically basecells must share same coordinate system
	//PtrStack<CellCombineRules> cell_combine_rules; //for penrose

	virtual int isRepeatable();
	virtual flatpoint repeatXDir(); //length is distance to translate
	virtual flatpoint repeatYDir();

	virtual const double *finalTransform(); //transform applied after tiling to entire pattern, to squish around
};


} //namespace Laidout

