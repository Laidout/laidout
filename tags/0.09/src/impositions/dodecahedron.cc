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
// Copyright (C) 2004-2010 by Tom Lechner
//


#include "dodecahedron.h"
#include "poly.h"
#include <lax/strmanip.h>

using namespace Laxkit;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;


#include <iostream>
using namespace std;
#define DBG 


//-------------------------- Dodecahedron ---------------------------------------------

//! Return a totally uwrapped net based on a dodecahedron abstract net.
/*! \todo unwrap in a more pleasing manner
 */
Net *makeDodecahedronNet(double ww,double hh)
{
	Polyhedron *poly=new Polyhedron;
	makestr(poly->name,"Dodecahedron");

	double t=(sqrt(5)+1)/2;
	double tt=t*t;
	poly->AddPoint(spacevector(  0, -1, tt)); //0
	poly->AddPoint(spacevector(  0,  1, tt)); //1
	poly->AddPoint(spacevector(  0,  1,-tt)); //2
	poly->AddPoint(spacevector(  0, -1,-tt)); //3
	poly->AddPoint(spacevector( tt,  0, -1)); //4
	poly->AddPoint(spacevector( tt,  0,  1)); //5
	poly->AddPoint(spacevector(-tt,  0,  1)); //6
	poly->AddPoint(spacevector(-tt,  0, -1)); //7
	poly->AddPoint(spacevector( -1,-tt,  0)); //8
	poly->AddPoint(spacevector(  1,-tt,  0)); //9
	poly->AddPoint(spacevector( -1, tt,  0)); //10
	poly->AddPoint(spacevector(  1, tt,  0)); //11
	poly->AddPoint(spacevector(  t,  t,  t)); //12
	poly->AddPoint(spacevector( -t,  t,  t)); //13
	poly->AddPoint(spacevector( -t, -t,  t)); //14
	poly->AddPoint(spacevector(  t, -t,  t)); //15
	poly->AddPoint(spacevector(  t,  t, -t)); //16
	poly->AddPoint(spacevector( -t,  t, -t)); //17
	poly->AddPoint(spacevector( -t, -t, -t)); //18
	poly->AddPoint(spacevector(  t, -t, -t)); //19

	poly->AddFace("0 1 13 6 14");
	poly->AddFace("0 15 5 12 1");
	poly->AddFace("0 14 8 9 15");
	poly->AddFace("6 7 18 8 14");
	poly->AddFace("6 13 10 17 7");
	poly->AddFace("1 12 11 10 13");
	poly->AddFace("3 19 9 8 18");
	poly->AddFace("4 5 15 9 19");
	poly->AddFace("4 16 11 12 5");
	poly->AddFace("2 17 10 11 16");
	poly->AddFace("2 3 18 7 17");
	poly->AddFace("2 16 4 19 3");

	poly->makeedges();

	Net *net=new Net;
	makestr(net->netname,poly->name);
	net->basenet=poly;

	 //unwrap nicely
	net->Anchor(0);
	net->Unwrap(0,-1);
	net->Unwrap(1,2);
	net->Unwrap(7,0);
	net->Unwrap(15,-1);

	net->rebuildLines();
	
	return net;
}



