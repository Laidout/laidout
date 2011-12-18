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
// Copyright (C) 2004-2007,2010-2011 by Tom Lechner
//
#ifndef OBJECTCONTAINER_H
#define OBJECTCONTAINER_H


#include <lax/anobject.h>
#include "styles.h"

//------------------------------ ObjectContainer ----------------------------------

#define Next_Success        1
#define Next_AtFirstObj     0
#define Next_NoSubObjects  -3 
#define Next_DoneWithLevel -2
#define Next_Error         -1

#define Next_Increment              (1<<0)
#define Next_Decrement              (1<<1)
#define Next_LeavesOnly             (1<<2)
#define Next_PlaceLevelOnly         (1<<3)
#define Next_PlaceLevelAndSubsOnly  (1<<4)
#define Next_SkipUnselectable       (1<<5)
#define Next_SkipZones              (1<<6)
#define Next_SkipLockedKids         (1<<7)

#define OBJ_Unselectable  (1<<0)
#define OBJ_Zone          (1<<1)
#define OBJ_ZoneTip       (1<<2)
#define OBJ_IgnoreKids    (1<<3)

class ObjectContainer : virtual public Laxkit::anObject
{
 public:
	ObjectContainer *parent;
	char *id;
	unsigned int obj_flags;
	virtual unsigned int object_flags() { return obj_flags; }
	
	ObjectContainer();
	virtual ~ObjectContainer();
	virtual int n() = 0;
	virtual Laxkit::anObject *object_e(int i) = 0;
	virtual const char *object_e_name(int i) = 0;
	virtual const double *object_transform(int i) = 0;

	virtual int contains(Laxkit::anObject *d,FieldPlace &place);
	virtual Laxkit::anObject *getanObject(FieldPlace &place,int offset=0,int nn=-1);
	virtual int nextObject(FieldPlace &place, int offset, unsigned int flags, Laxkit::anObject **d=NULL);
};








#endif

