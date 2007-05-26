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
// Copyright (C) 2004-2007 by Tom Lechner
//


#include "../language.h"
#include "papertileinterface.h"
#include <lax/strmanip.h>

using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG 


//------------------------------------- PaperBox --------------------------------------

/*! \class PaperBox 
 * \brief Wrapper around a paper style, for use in a PaperTileInterface.
 */

/*! Incs count of paper.
 */
PaperBox::PaperBox(PaperStyle *paper)
{
	paperstyle=paper;
	if (paper) paper->inc_count();
}

PaperBox::~PaperBox()
{
	if (paper) paper->dec_count();
}

//------------------------------------- PaperBoxData --------------------------------------

/*! \class PaperBoxData
 * \brief Somedata Wrapper around a paper style, for use in a PaperTileInterface.
 */

PaperBoxData::PaperBoxData(PaperStyle *paper)
	: PaperBox(paper)
{
	if (paperstyle) {
		minx=mix=0;
		maxx=paperstyle->w();
		maxy=paperstyle->h();
	}
}

//------------------------------------- PaperTileInterface --------------------------------------
	
/*! \class PaperTileInterface 
 * \brief Interface to arrange an arbitrary spread to print on many sheets of paper.
 */


PaperTileInterface::PaperTileInterface(int nid,Displayer *ndp)
	: InterfaceWithDp(nid,ndp) 
{ 
}

PaperTileInterface::PaperTileInterface(anInterface *nowner,int nid,Displayer *ndp)
	: InterfaceWithDp(nowner,nid,ndp) 
{
}

PaperTileInterface::~PaperTileInterface()
{
	DBG cout <<"PaperTileInterface destructor.."<<endl;
}


//! Return a new PaperTileInterface if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! Normally, this is called when setting up a ViewportWindow/ViewerWindow system.
 * In a scenario, the dp and the data if present should not be copied, as they will
 * be assigned new stuff by the window, thus those things are not transferred to the
 * duplicate. Typically, the specific interface will create their
 * own blank instance of themselves, and in doing so, the dp and and data will
 * be set to NULL there. Nothing is copied over in this function, so interfaces 
 * can call anInterface::duplicate() from their duplicate(), rather than this duplicate().
 *
 * Typical duplicate function in an interface looks like this:\n
 * <pre>
 * anInterface *TheInterface::duplicate()
 * {
 *    dup=new TheInterface();
 *    // add any other TheInterface specific initialization
 *    return anInterface::duplicate(dup);
 * }
 * </pre>
 *  
 *  This function does not allow creation of a blank PaperTileInterface object. If dup==NULL, then
 *  NULL is returned.
 */
anInterface *PaperTileInterface::duplicate(anInterface *dup)//dup=NULL
{***
	if (dup==NULL) return NULL;
	return anInterface::duplicate(dup);
}


int InterfaceOn()
{***
}

int InterfaceOff(); 
void Clear(SomeData *d)
{***
}

	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1
{***
}

int UseThis(Laxkit::anObject *ndata,unsigned int mask=0)
{***
}

int DrawData(Laxkit::anObject *ndata,
			Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0)
{***
}

int DrawDataDp(Laxkit::Displayer *tdp,SomeData *tdata,
			Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=1)
{***
}

int Refresh()
{***
}
	
int LBDown(int x,int y,unsigned int state,int count)
{***
}

int MBDown(int x,int y,unsigned int state,int count)
{***
}

int RBDown(int x,int y,unsigned int state,int count)
{***
}

int LBUp(int x,int y,unsigned int state)
{***
}

int MBUp(int x,int y,unsigned int state)
{***
}

int RBUp(int x,int y,unsigned int state)
{***
}

int But4(int x,int y,unsigned int state)
{***
}

int But5(int x,int y,unsigned int state)
{***
}

int MouseMove(int x,int y,unsigned int state)
{***
}

int CharInput(unsigned int ch,unsigned int state)
{***
}

int CharRelease(unsigned int ch,unsigned int state)
{***
}


//} // namespace Laidout

