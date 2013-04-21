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
// Copyright (C) 2013 by Tom Lechner
//
#ifndef LIMAGEINTERFACE_H
#define LIMAGEINTERFACE_H

#include <lax/interfaces/imageinterface.h>
#include "../calculator/values.h"


namespace Laidout {


//------------------------------- LImageInterface --------------------------------
class LImageInterface : public LaxInterfaces::ImageInterface,
						public Value
{
 protected:
	virtual void runImageDialog();
 public:
	LImageInterface(int nid,Laxkit::Displayer *ndp);
	virtual const char *whattype() { return "ImageInterface"; }
	virtual LaxInterfaces::anInterface *duplicate(LaxInterfaces::anInterface *dup);

	//from value
	virtual Value *duplicate();
	virtual ObjectDef *makeObjectDef();
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual Value *dereference(const char *extstring, int len);
};


} //namespace Laidout



#endif

