//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2005-2007 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
#ifndef GROUPINTERFACE_H
#define GROUPINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/objectinterface.h>
#include <lax/interfaces/somedata.h>



//----------------------------- GroupInterface -----------------------

class GroupInterface : public LaxInterfaces::ObjectInterface
{
  protected:
  public:
	GroupInterface(int nid,Laxkit::Displayer *ndp);
	virtual ~GroupInterface();
	//virtual const char *whattype() { return "ObjectInterface"; }
	virtual const char *whatdatatype() { return "Group"; }
	virtual anInterface *duplicate(anInterface *dup=NULL);
	virtual int UseThis(anObject *newdata,unsigned int);

	virtual int LBDown(int x, int y,unsigned int state, int count);
	//virtual int DrawData(anObject *ndata,anObject *a1,anObject *a2,int);
	//virtual int AddToSelection(ObjectContext *oc);
	//virtual int FreeSelection();
	virtual int GrabSelection(unsigned int state);
	virtual int ToggleGroup();
};


#endif

