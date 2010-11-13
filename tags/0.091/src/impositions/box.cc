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
// Copyright (C) 2008-2010 by Tom Lechner
//


#include "box.h"
#include "poly.h"
#include <lax/strmanip.h>

using namespace Laxkit;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;


#include <iostream>
using namespace std;
#define DBG 


//-------------------------- makeBox ---------------------------------------------

//! Return a net of a rectangular box with the given dimenions. 
/*! str either starts "box" or with a number. For instance a box with width 1, length 2, and
 * height 3 might be specified with "box 1 2 3" or simply "1 2 3".
 */
Net *makeBox(const char *str,double x,double y,double z)
{
	if (str) {
		 //skip any non-number
		while (!isdigit(*str) && *str!='.') str++;
		if (*str) {
			char *e;
			x=strtod(str,&e);
			if (e!=str) {
				str=e;
				if (*str==',') str++;
				y=strtod(str,&e);
				if (e!=str) {
					str=e;
					if (*str==',') str++;
					z=strtod(str,&e);
				}
			}
		}
	}
	if (z<=0) z=y;
	if (y<=0) z=y=x;
	if (x<=0) z=y=x=1;


	Polyhedron *poly=new Polyhedron;
	char tstr[100];
	if (x==y && y==z) sprintf(tstr,"Cube(%.10g)",x);
	else sprintf(tstr,"Box(%.10g,%.10g,%.10g)",x,y,z);
	makestr(poly->name,tstr);

	poly->AddPoint(spacevector( x, y, z)); //0
	poly->AddPoint(spacevector(-x, y, z)); //1
	poly->AddPoint(spacevector(-x,-y, z)); //2
	poly->AddPoint(spacevector( x,-y, z)); //3
	poly->AddPoint(spacevector(-x, y,-z)); //4
	poly->AddPoint(spacevector( x, y,-z)); //5
	poly->AddPoint(spacevector( x,-y,-z)); //6
	poly->AddPoint(spacevector(-x,-y,-z)); //7

	poly->AddFace("0 1 2 3");
	poly->AddFace("1 0 5 4");
	poly->AddFace("2 1 4 7");
	poly->AddFace("0 3 6 5");
	poly->AddFace("3 2 7 6");
	poly->AddFace("4 5 6 7");

	poly->makeedges();
	 
	Net *net=new Net;
	makestr(net->netname,poly->name);
	net->basenet=poly;
	net->TotalUnwrap();

	net->rebuildLines();

	return net;
}

