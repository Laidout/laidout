//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
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

#include "groupinterface.h"
#include "../project.h"
#include "../viewwindow.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace LaxInterfaces;

//----------------------------- GroupInterface -----------------------

/*! \class GroupInterface
 * \brief Interface for selecting multiple things, grouping, and ungrouping.
 */



GroupInterface::GroupInterface(int nid,Laxkit::Displayer *ndp)
	: ObjectInterface(nid,ndp)
{
}

GroupInterface::~GroupInterface()
{
	DBG cout <<"---- in GroupInterface destructor"<<endl;
}

anInterface *GroupInterface::duplicate(anInterface *dup)
{
	GroupInterface *g;
	if (dup==NULL) g=new GroupInterface(id,dp);
	else {g=dynamic_cast<GroupInterface *>(dup);
		if (g==NULL) return NULL;
	}
	return ObjectInterface::duplicate(g);
}

int GroupInterface::ToggleGroup()
{
	cout <<"*******togglegroup"<<endl;

	 // check that all selected objects are same level, 
	 // then make new Group object and fill in selection,
	 // updating LaidoutViewport and Document

	if (selection.n==1 && !strcmp(selection.e[0]->whattype(),"Group")) {
		//((Group *)selection.e[0])->UnGroup();
		((LaidoutViewport *)viewport)->doc->UnGroup(((LaidoutViewport *)viewport)->curobj.context);
		return 1;
	}
	
	return 0;
}




