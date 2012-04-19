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


#include "gloverlay.h"

#include <lax/misc.h>

using namespace Laxkit;


namespace Polyptych {

/*! \class Overlay
 *  \brief Simple box to contain a clickable area meant for a gl based display.
 */


Overlay::Overlay(const char *str, ActionType a,int otype,int oindex)
{
	id=getUniqueNumber();
	text=str;
	action=a;
	group=-1;
	index=oindex;
	call_id=0;
	type=otype;
	info=0;
	info1=info2=0;
}

Overlay::~Overlay()
{
}

void Overlay::Draw(int state)
{
	if (call_id==0 && text.length()) {
		//***
	}
}

int Overlay::Create(int callnumber)
{
	return 1;
}

int Overlay::Install(int callnumber)
{
	return 1;
}

const char *Overlay::Text()
{
	return text.c_str();
}

int Overlay::PointIn(int x,int y)
{ return x>=minx && x<=maxx && y>=miny && y<=maxy; }


} //namespace Polyptych

