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
// Copyright (C) 2010 by Tom Lechner
//


#include "../language.h"
#include "signatureinterface.h"
#include "viewwindow.h"
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>

#include <lax/lists.cc>

using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


//------------------------------------- SignatureInterface --------------------------------------
	
/*! \class SignatureInterface 
 * \brief Interface to fold around a SignatureImposition on the edit space.
 *
 * WORK ON ME!!
 *
 * This tool lets you do partial folding of a SignatureImposition in an edit window.
 */

SignatureInterface::SignatureInterface(int nid,Displayer *ndp)
	: InterfaceWithDp(nid,ndp) 
{
	showdecs=0;
	doc=NULL;
	signature=new Signature;

	patternwidth=1;
	patternheight=1;
	
	foldlevel=0; //how many of the folds are active in display. must be < sig->folds.n
	foldinfo=NULL;
	reallocateFoldinfo();
}

void SignatureInterface::reallocateFoldinfo()
{
	if (foldinfo) {
		for (int c=0; foldinfo[c]; c++) delete[] foldinfo[c];
		delete[] foldinfo;
	}
	foldinfo=new FoldedPageInfo*[signature->numhfolds+2];
	int c;
	for (int c=0; c<signature->numhfolds+1; c++) foldinfo[c]=new FoldedPageInfo[signature->numvfolds+2];
	foldinfo[c]=NULL; //terminating NULL, so we don't need to remember sig->n
}

SignatureInterface::SignatureInterface(anInterface *nowner,int nid,Displayer *ndp)
	: InterfaceWithDp(nowner,nid,ndp) 
{
	showdecs=0;
	doc=NULL;
	signature=NULL;
}

SignatureInterface::~SignatureInterface()
{
	DBG cerr <<"SignatureInterface destructor.."<<endl;

	if (doc) doc->dec_count();
	if (signature) signature->dec_count();
}

/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *SignatureInterface::ContextMenu(int x,int y)
{
	return NULL;
//	MenuInfo *menu=new MenuInfo(_("Signature Interface"));
//	menu->AddItem(_("Paper Size"),999);
//	menu->SubMenu(_("Paper Size"));
//	for (int c=0; c<laidout->papersizes.n; c++) {
//		menu->AddItem(laidout->papersizes.e[c]->name,c,
//				LAX_ISTOGGLE
//				| (!strcmp(curboxes.e[0]->box->paperstyle->name,laidout->papersizes.e[c]->name)
//				  ? LAX_CHECKED : 0));
//	}
//	menu->EndSubMenu();
//	menu->AddSep();	
//	return menu;
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int SignatureInterface::Event(const Laxkit::EventData *data,const char *mes)
{
	return 1;
}

//int UseThisImposition(SignatureImposition *sig)
//{***
//}

/*! incs count of ndoc if ndoc is not already the current document.
 *
 * Return 0 for success, nonzero for fail.
 */
int SignatureInterface::UseThisDocument(Document *ndoc)
{
	if (ndoc==doc) return 0;
	if (doc) doc->dec_count();
	doc=ndoc;
	if (ndoc) ndoc->inc_count();
	return 0;
}

/*! PaperGroup or PaperBoxData.
 */
int SignatureInterface::draws(const char *atype)
{ return !strcmp(atype,"SignatureData"); }


//! Return a new SignatureInterface if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *SignatureInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new SignatureInterface(id,NULL);
	else if (!dynamic_cast<SignatureInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}


int SignatureInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

int SignatureInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void SignatureInterface::Clear(SomeData *d)
{
}

	
///*! This will be called by viewports, and the papers will be displayed opaque with
// * drop shadow.
// */
//int SignatureInterface::DrawDataDp(Laxkit::Displayer *tdp,SomeData *tdata,
//			Laxkit::anObject *a1,Laxkit::anObject *a2,int info)
//{
//	return 1;
//}

/*! \todo draw arrow to indicate paper up direction
 */
int SignatureInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	double ew=patternwidth/(signature->numvfolds+1);
	double eh=patternheight/(signature->numhfolds+1);
	double  w=patternwidth;
	double  h=patternheight;

	 //draw fold pattern outline
	dp->NewFG(1.,0.,0.);
	dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
	dp->drawline(0,0, w,0);
	dp->drawline(w,0, w,h);
	dp->drawline(w,h, 0,h);
	dp->drawline(0,h, 0,0);

	 //draw all fold lines
	dp->NewFG(.5,.5,.5);
	dp->LineAttributes(1,LineOnOffDash, CapButt, JoinMiter);
	for (int c=0; c<signature->numvfolds; c++) {
		dp->drawline((c+1)*ew,0, (c+1)*ew,h);
	}

	for (int c=0; c<signature->numhfolds; c++) {
		dp->drawline(0,(c+1)*eh, w,(c+1)*eh);
	}


	if (showdecs) {
		//hmmm...
	}

	return 1;
}

//! Return 0 for not in pattern, or the row and column.
int SignatureInterface::scan(int x,int y,int *row,int *col)
{
	flatpoint fp=screentoreal(x,y);
	DBG cerr <<"fp:"<<fp.x<<','<<fp.y<<endl;

	*row=fp.y*(signature->numhfolds+1)/patternheight;
	*col=fp.x*(signature->numvfolds+1)/patternwidth;

	if (fp.y<0) (*row)--;
	if (fp.x<0) (*col)--;

	return *row>=0 && *row<=signature->numhfolds && *col>=0 && *col<=signature->numvfolds;
}

int SignatureInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	flatpoint fp=screentoreal(x,y);

	int row,col;
	int over=scan(x,y, &row,&col);
	DBG cerr <<"over element "<<over<<": r,c="<<row<<','<<col<<endl;

	if (buttondown.any()) return 0;

	buttondown.down(d->id,LEFTBUTTON, x,y, row,col);


	return 0;
}

int SignatureInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!(buttondown.isdown(d->id,LEFTBUTTON))) return 1;
	buttondown.up(d->id,LEFTBUTTON);
	return 0;
}

//int SignatureInterface::MBDown(int x,int y,unsigned int state,int count)
//int SignatureInterface::MBUp(int x,int y,unsigned int state)
//int SignatureInterface::RBDown(int x,int y,unsigned int state,int count)
//int SignatureInterface::RBUp(int x,int y,unsigned int state)
//int SignatureInterface::But4(int x,int y,unsigned int state);
//int SignatureInterface::But5(int x,int y,unsigned int state);

int SignatureInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	int row,col;
	int over=scan(x,y,&row,&col);

	DBG cerr <<"over element "<<over<<": r,c="<<row<<','<<col<<endl;

	int mx,my;
	buttondown.move(mouse->id,LEFTBUTTON);
	buttondown.getinitial(mouse->id,LEFTBUTTON, &mx,&my);

	flatpoint d=screentoreal(x,y)-screentoreal(mx,my); 
	char dir=' ';

	if (fabs(d.x)>fabs(d.y)) {
		if (d.x>0) {
			dir='r';
		} else if (d.x<0) {
			dir='l';
		}
	} else {
		if (d.y>0) {
			dir='t';
		} else if (d.y<0) {
			dir='b';
		}
	}

	return 0;
}

/*!
 * 'a'          select all, or if some are selected, deselect all
 * del or bksp  delete currently selected papers
 *
 * \todo auto tile spread contents
 * \todo revert to other group
 * \todo edit another group
 */
int SignatureInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" SignatureInterface got ch:"<<ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;

	if (ch=='d' && (state&LAX_STATE_MASK)==0) {
		showdecs++;
		if (showdecs>2) showdecs=0;
		needtodraw=1;
		return 0;

	} else if (ch=='v') {
		signature->numvfolds++;
		needtodraw=1;
		return 0;

	} else if (ch=='V') {
		signature->numvfolds--;
		if (signature->numvfolds<=0) signature->numvfolds=0;
		needtodraw=1;
		return 0;

	} else if (ch=='h') {
		signature->numhfolds++;
		needtodraw=1;
		return 0;

	} else if (ch=='H') {
		signature->numhfolds--;
		if (signature->numhfolds<=0) signature->numhfolds=0;
		needtodraw=1;
		return 0;

	}

	return 1;
}

int SignatureInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
//	if (ch==LAX_Shift) {
//		return 0;
//	}
	return 1;
}


//} // namespace Laidout

