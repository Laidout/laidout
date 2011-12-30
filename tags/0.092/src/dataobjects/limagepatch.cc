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

#include "limagepatch.h"
#include "../laidout.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace LaxInterfaces;

//------------------------------------- LImagePatchInterface -------------------------
/*! \class LImagePatchInterface
 * 
 * Subclass LaxInterfaces::ImagePatchInterface so that a change to recurse affects
 * the pool class as well. 
 *
 * \todo *** this is a big hack... need a better way to control interface data drawing,
 *   if there is a draw in place, then a draw on top, then should be a flag to not draw
 *   the one beneath...
 */

LImagePatchInterface::LImagePatchInterface(int nid,Laxkit::Displayer *ndp)
	: ImagePatchInterface(nid,ndp)
{
}

anInterface *LImagePatchInterface::duplicate(anInterface *dup)
{
	return ImagePatchInterface::duplicate(new LImagePatchInterface(id,NULL));
}

int LImagePatchInterface::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *k)
{
	////DBG cerr <<"*****************in LImagePatchInterface::CharInput"<<endl;
	int r=recurse,m=rendermode;
	int cc=ImagePatchInterface::CharInput(ch,buffer,len,state,k);
	if (recurse!=r || m!=rendermode) {
		for (int c=0; c<laidout->interfacepool.n; c++) {
			if (!strcmp(laidout->interfacepool.e[c]->whattype(),"ImagePatchInterface")) {
				static_cast<ImagePatchInterface *>(laidout->interfacepool.e[c])->recurse=recurse;
				static_cast<ImagePatchInterface *>(laidout->interfacepool.e[c])->rendermode=rendermode;
				break;
			}
		}
	}
	if (cc==1) return 1;
	return cc;
}

//------------------------------------- LColorPatchInterface -------------------------
/*! \class LColorPatchInterface
 * 
 * Subclass LaxInterfaces::ColorPatchInterface so that a change to recurse affects
 * the pool class as well. 
 *
 * \todo *** this is a big hack... need a better way to control interface data drawing,
 *   if there is a draw in place, then a draw on top, then should be a flag to not draw
 *   the one beneath...
 */

LColorPatchInterface::LColorPatchInterface(int nid,Laxkit::Displayer *ndp)
	: ColorPatchInterface(nid,ndp)
{
}

anInterface *LColorPatchInterface::duplicate(anInterface *dup)
{
	return ColorPatchInterface::duplicate(new LColorPatchInterface(id,NULL));
}

int LColorPatchInterface::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *k)
{
	////DBG cerr <<"*****************in LColorPatchInterface::CharInput"<<endl;
	int r=recurse,m=rendermode;
	int cc=ColorPatchInterface::CharInput(ch,buffer,len,state,k);
	if (recurse!=r || m!=rendermode) {
		for (int c=0; c<laidout->interfacepool.n; c++) {
			if (!strcmp(laidout->interfacepool.e[c]->whattype(),"ColorPatchInterface")) {
				static_cast<ColorPatchInterface *>(laidout->interfacepool.e[c])->recurse=recurse;
				static_cast<ColorPatchInterface *>(laidout->interfacepool.e[c])->rendermode=rendermode;
				break;
			}
		}
	}
	if (cc==1) return 1;
	return cc;
}
