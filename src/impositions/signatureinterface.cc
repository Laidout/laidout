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
#include "../viewwindow.h"
#include "../headwindow.h"

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
 * \brief Interface to fold around a Signature on the edit space.
 */
/*! \var int SignatureInterface::foldindex
 * \brief Where a potential fold is.
 *
 * If moving up or down, foldindex is the row immediately above the fold.
 * If moving left or right, foldindex is the column immediately to the right of the fold.
 */
/*! \var int SignatureInterface::foldlevel
 * \brief The current unfolding. 0 is totally unfolded.
 */

SignatureInterface::SignatureInterface(int nid,Displayer *ndp,Signature *sig, PaperStyle *p)
	: InterfaceWithDp(nid,ndp) 
{
	showdecs=0;
	papersize=NULL;

	signature=new Signature;
	if (p) signature->SetPaper(p);

	foldr1=foldc1=foldr2=foldc2=-1;
	folddirection=0;
	lbdown_row=lbdown_col=-1;
	currentfold=-1;

	if (!p) {
		totalheight=totalwidth=5;
		signature->totalheight=totalheight;
		signature->totalwidth=totalwidth;
	} else {
		totalheight=signature->totalheight;
		totalwidth =signature->totalwidth;
	}
	
	foldlevel=0; //how many of the folds are active in display. must be < sig->folds.n
	foldinfo=NULL;
	hasfinal=0;
	reallocateFoldinfo();
}

SignatureInterface::SignatureInterface(anInterface *nowner,int nid,Displayer *ndp)
	: InterfaceWithDp(nowner,nid,ndp) 
{
	showdecs=0;
	signature=new Signature;
	papersize=NULL;

	foldr1=foldc1=foldr2=foldc2=-1;
	folddirection=0;
	lbdown_row=lbdown_col=-1;

	totalheight=totalwidth=5;
	signature->totalheight=totalheight;
	signature->totalwidth=totalwidth;
	
	foldlevel=0; //how many of the folds are active in display. must be < sig->folds.n
	foldinfo=NULL;
	reallocateFoldinfo();
}

SignatureInterface::~SignatureInterface()
{
	DBG cerr <<"SignatureInterface destructor.."<<endl;

	if (signature) signature->dec_count();
	if (papersize) papersize->dec_count();
}

//! Reallocate foldinfo, usually after adding fold lines.
/*! this will flush any folds stored in the signature.
 */
void SignatureInterface::reallocateFoldinfo()
{
	signature->folds.flush();
	hasfinal=0;

	if (foldinfo) {
		for (int c=0; foldinfo[c]; c++) delete[] foldinfo[c];
		delete[] foldinfo;
	}
	foldinfo=new FoldedPageInfo*[signature->numhfolds+2];
	int r;
	for (r=0; r<signature->numhfolds+1; r++) {
		foldinfo[r]=new FoldedPageInfo[signature->numvfolds+2];
		for (int c=0; c<signature->numvfolds+1; c++) {
			foldinfo[r][c].pages.push(r);
			foldinfo[r][c].pages.push(c);
		}
	}
	foldinfo[r]=NULL; //terminating NULL, so we don't need to remember sig->n
}

//! Flush all the foldinfo pages stacks, as if there have been no folds yet.
/*! Please note this does not create or reallocate foldinfo.
 */
void SignatureInterface::resetFoldinfo()
{
	for (int r=0; r<signature->numhfolds+1; r++) {
		for (int c=0; c<signature->numvfolds+1; c++) {
			foldinfo[r][c].pages.flush();
			foldinfo[r][c].pages.push(r);
			foldinfo[r][c].pages.push(c);
		}
	}
}

//! Low level flipping across folds.
/*! This will flip everything on one side of a fold to the other side (if possible).
 * It is not a selective flipping.
 *
 * This is called to ONLY apply the fold. It does not check and apply final index settings
 * or check for validity of the fold.
 */
void SignatureInterface::applyFold(char folddir, int index, int under)
{
	int newr,newc, tr,tc;
	int fr1,fr2, fc1,fc2;

	if (folddir=='l') {
		fr1=0;
		fr2=signature->numhfolds;
		fc1=index;
		fc2=signature->numvfolds;
	} else if (folddir=='r') {
		fr1=0;
		fr2=signature->numhfolds;
		fc1=0;
		fc2=index-1;
	} else if (folddir=='b') {
		fr1=index;
		fr2=signature->numhfolds;
		fc1=0;
		fc2=signature->numvfolds;
	} else if (folddir=='t') {
		fr1=0;
		fr2=index-1;
		fc1=0;
		fc2=signature->numvfolds;
	}

	for (int r=fr1; r<=fr2; r++) {
	  for (int c=fc1; c<=fc2; c++) {
		if (foldinfo[r][c].pages.n==0) continue; //skip blank cells

		 //find new positions
		if (folddir=='b') {
			newc=c;
			newr=index-(r-index+1);
		} else if (folddir=='r') {
			newr=r;
			newc=index+(index-c-1);
		} else if (folddir=='l') {
			newr=r;
			newc=index-(c-index+1);
		} else if (folddir=='t') {
			newc=c;
			newr=index+(index-r-1);
		}

		 //swap old and new positions
		if (foldunder) {
			while(foldinfo[r][c].pages.n) {
				tc=foldinfo[r][c].pages.pop(0);
				tr=foldinfo[r][c].pages.pop(0);
				foldinfo[newr][newc].pages.push(tc,0);
				foldinfo[newr][newc].pages.push(tr,0);
			}
		} else {
			while(foldinfo[r][c].pages.n) {
				tc=foldinfo[r][c].pages.pop();
				tr=foldinfo[r][c].pages.pop();
				foldinfo[newr][newc].pages.push(tr);
				foldinfo[newr][newc].pages.push(tc);
			}
		}
		 //flip the original place.
		if (!hasfinal) {
			if (folddir=='b' || folddir=='t') foldinfo[tr][tc].y_flipped=!foldinfo[r][c].y_flipped;
			else foldinfo[tr][tc].x_flipped=!foldinfo[r][c].x_flipped;
		}
	  }
	}
}

//! Check if the signature is totally folded or not.
/*! Remember that if there are no fold lines, then we need to be hasfinal==1 for
 * totally folded, letting us set margin, final trim, and binding.
 *
 * TODO: If update!=0, then if hasfinal==1, make sure binding and updirection is
 * applied to foldinfo.
 *
 * Returns foldlevel.
 */
int SignatureInterface::checkFoldlevel(int update)
{
	 //check the immediate neighors of the first cell with pages.
	 //If there are no neighbors, then we are totally folded.

	 //find a non blank cell
	int newr=0,newc=0;
	for (newr=0; newr<=signature->numhfolds; newr++) {
	  for (newc=0; newc<=signature->numvfolds; newc++) {
		if (foldinfo[newr][newc].pages.n!=0) break;
	  }
	  if (newc!=signature->numvfolds+1) break;
	}

	int stillmore=4;
	int tr,tc;

	 //check above
	tr=newr-1; tc=newc;
	if (tr<0 || foldinfo[tr][tc].pages.n==0) stillmore--;

	 //check below
	tr=newr+1; tc=newc;
	if (tr>signature->numhfolds || foldinfo[tr][tc].pages.n==0) stillmore--;
	
	 //check left
	tr=newr; tc=newc-1;
	if (tc<0 || foldinfo[tr][tc].pages.n==0) stillmore--;

	 //check if right
	tr=newr; tc=newc+1;
	if (tc>signature->numvfolds || foldinfo[tr][tc].pages.n==0) stillmore--;

	if (stillmore==0) {
		int finalr=newr;
		int finalc=newc;

		int page=0,xflip,yflip;
		for (int c=foldinfo[finalr][finalc].pages.n-2; c>=0; c-=2) {
			tr=foldinfo[finalr][finalc].pages.e[c];
			tc=foldinfo[finalr][finalc].pages.e[c+1];

			xflip=foldinfo[tr][tc].x_flipped;
			yflip=foldinfo[tr][tc].y_flipped;

			if ((xflip && !yflip) || (!xflip && yflip)) {
				 //back side of paper is up
				foldinfo[tr][tc].finalindexback=page;
				foldinfo[tr][tc].finalindexfront=page+1;
			} else {
				foldinfo[tr][tc].finalindexback=page+1;
				foldinfo[tr][tc].finalindexfront=page;
			}
			page+=2;
		}
		hasfinal=1;
	}

	return foldlevel;
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

	double patternheight=signature->PatternHeight();
	double patternwidth =signature->PatternWidth();

	double x,y,w,h;
	static char str[150];

	 //----------------draw whole outline
	dp->NewFG(1.,0.,1.); //purple for paper outline, like custom papergroup border color
	w=totalwidth;
	h=totalheight;
	dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
	dp->drawline(0,0, w,0);
	dp->drawline(w,0, w,h);
	dp->drawline(w,h, 0,h);
	dp->drawline(0,h, 0,0);
	
	 //----------------draw inset
	dp->NewFG(.5,0.,0.); //dark red for inset
	if (signature->insetleft) dp->drawline(signature->insetleft,0, signature->insetleft,h);
	if (signature->insetright) dp->drawline(w-signature->insetright,0, w-signature->insetright,h);
	if (signature->insettop) dp->drawline(0,h-signature->insettop, w,h-signature->insettop);
	if (signature->insetbottom) dp->drawline(0,signature->insetbottom, w,signature->insetbottom);


	 //------------------draw fold pattern in each tile
	double ew=patternwidth/(signature->numvfolds+1);
	double eh=patternheight/(signature->numhfolds+1);
	w=patternwidth;
	h=patternheight;

	x=signature->insetleft;
	flatpoint pts[4],fp;
	int facedown=0;
	int hasface;
	int rrr,ccc;
	double xx,yy;
	int xflip, yflip;
	for (int tx=0; tx<signature->tilex; tx++) {
	  y=signature->insetbottom;
	  for (int ty=0; ty<signature->tiley; ty++) {
		 //fill in light gray for elements with no current faces
		 //or draw orientation arrow and number for existing faces
		for (int rr=0; rr<signature->numhfolds+1; rr++) {
		  for (int cc=0; cc<signature->numvfolds+1; cc++) {
			hasface=foldinfo[rr][cc].pages.n;

			 //first draw filled face, grayed if no current faces
			dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
			pts[0]=flatpoint(x+cc*ew,y+rr*eh);
			pts[1]=pts[0]+flatpoint(ew,0);
			pts[2]=pts[0]+flatpoint(ew,eh);
			pts[3]=pts[0]+flatpoint(0,eh);

			if (hasface) dp->NewFG(1.,1.,1.);
			else dp->NewFG(.75,.75,.75);

			dp->drawlines(pts,4,1,1);

			if (hasface) {
				rrr=foldinfo[rr][cc].pages[foldinfo[rr][cc].pages.n-2];
				ccc=foldinfo[rr][cc].pages[foldinfo[rr][cc].pages.n-1];

				if (foldinfo[rr][cc].finalindexfront>=0) {
					 //there are faces in this spot, draw arrow and page number
					dp->NewFG(.75,.75,.75);
					xflip=foldinfo[rrr][ccc].x_flipped;
					yflip=foldinfo[rrr][ccc].y_flipped;
					facedown=((xflip && !yflip) || (!xflip && yflip));

					if (facedown) dp->LineAttributes(1,LineOnOffDash, CapButt, JoinMiter);
					else dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
					pts[0]=flatpoint(x+(cc+.5)*ew,y+(rr+.25+.5*(yflip?1:0))*eh);
					dp->drawarrow(pts[0],flatpoint(0,yflip?-1:1)*eh/4, 0,eh/2,1);
					fp=dp->realtoscreen(pts[0]);
					if (facedown) sprintf(str,"%d/%d",foldinfo[rrr][ccc].finalindexback,foldinfo[rrr][ccc].finalindexfront);
					else sprintf(str,"%d/%d",foldinfo[rrr][ccc].finalindexfront,foldinfo[rrr][ccc].finalindexback);
					dp->textout(fp.x,fp.y, str,-1, LAX_CENTER);
				}
			}

			 //draw markings for final page binding edge, up, trim, margin
			if (hasfinal && foldlevel==signature->folds.n && rr==finalr && cc==finalc) {
				dp->LineAttributes(2,LineSolid, CapButt, JoinMiter);
				
				xx=x+cc*ew;
				yy=y+rr*eh;

				 //draw gray margin edge
				dp->NewFG(.75,.75,.75);
				dp->drawline(xx,yy+signature->marginbottom,   xx+ew,yy+signature->marginbottom);			
				dp->drawline(xx,yy+eh-signature->margintop,   xx+ew,yy+eh-signature->margintop);			
				dp->drawline(xx+signature->marginleft,yy,     xx+signature->marginleft,yy+eh);			
				dp->drawline(xx+ew-signature->marginright,yy, xx+ew-signature->marginright,yy+eh);			

				 //draw red trim edge
				dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
				dp->NewFG(1.,0.,0.);
				if (signature->trimbottom>0) dp->drawline(xx,yy+signature->trimbottom, xx+ew,yy+signature->trimbottom);			
				if (signature->trimtop>0)    dp->drawline(xx,yy+eh-signature->trimtop, xx+ew,yy+eh-signature->trimtop);			
				if (signature->trimleft>0)   dp->drawline(xx+signature->trimleft,yy, xx+signature->trimleft,yy+eh);			
				if (signature->trimright>0)  dp->drawline(xx+ew-signature->trimright,yy, xx+ew-signature->trimright,yy+eh);			

				 //draw green binding edge
				dp->LineAttributes(2,LineSolid, CapButt, JoinMiter);
				dp->NewFG(0.,1.,0.);

				int b=signature->binding;
				if (b=='l')      dp->drawline(xx,yy, xx,yy+eh);
				else if (b=='r') dp->drawline(xx+ew,yy, xx+ew,yy+eh);
				else if (b=='t') dp->drawline(xx,yy+eh, xx+ew,yy+eh);
				else if (b=='b') dp->drawline(xx,yy, xx+ew,yy);

			}
		  } //cc
		}  //rr

		 //draw fold pattern outline
		dp->NewFG(1.,0.,0.);
		dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
		dp->drawline(x,    y, x+w,  y);
		dp->drawline(x+w,  y, x+w,y+h);
		dp->drawline(x+w,y+h, x  ,y+h);
		dp->drawline(x,  y+h, x,y);

		 //draw all fold lines
		dp->NewFG(.5,.5,.5);
		dp->LineAttributes(1,LineOnOffDash, CapButt, JoinMiter);
		for (int c=0; c<signature->numvfolds; c++) { //verticals
			dp->drawline(x+(c+1)*ew,y, x+(c+1)*ew,y+h);
		}

		for (int c=0; c<signature->numhfolds; c++) { //horizontals
			dp->drawline(x,y+(c+1)*eh, x+w,y+(c+1)*eh);
		}
		
		y+=patternheight+signature->tilegapy;
	  } //tx
	  x+=patternwidth+signature->tilegapx;
	} //ty

	 //draw in progress folding
	int device=0;
	DBG cerr <<"----------------any "<<buttondown.any(0,LEFTBUTTON,&device)<<endl;
	if (buttondown.any(0,LEFTBUTTON,&device) && folddirection && folddirection!='x') {
		DBG cerr <<"--------------------------------showing dir"<<endl;
		int mx,my;
		buttondown.getinitial(device,LEFTBUTTON,&mx,&my);

		flatpoint p=dp->screentoreal(mx,my);
		flatpoint dir;
		if (folddirection=='t') dir.y=1;
		else if (folddirection=='b') dir.y=-1;
		else if (folddirection=='l') dir.x=-1;
		else if (folddirection=='r') dir.x=1;

		dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
		//dp->drawarrow(p,dir,0,25,0);


		 //draw partially folded region foldr1..foldr2, foldc1..foldc2
		double ew=patternwidth/(signature->numvfolds+1);
		double eh=patternheight/(signature->numhfolds+1);
		w=ew*(foldc2-foldc1+1);
		h=eh*(foldr2-foldr1+1);

		double rotation=foldprogress*M_PI;
		if (folddirection=='r' || folddirection=='t') rotation=M_PI-rotation;
		flatpoint axis;
		if (folddirection=='l' || folddirection=='r')
			axis=rotate(flatpoint(1,0),rotation,0);
		else axis=rotate(flatpoint(0,1),-rotation,0);

		if (foldunder) {
			dp->NewFG(.2,.2,.2);
			dp->LineAttributes(2, LineOnOffDash, CapButt, JoinMiter);
			if (folddirection=='l' || folddirection=='r') axis.y=-axis.y;
			else axis.x=-axis.x;
		} else {
			dp->NewFG(.9,.9,.9);
			dp->LineAttributes(1, LineSolid, CapButt, JoinMiter);
		}

		x=signature->insetleft;
		flatpoint pts[4];
		for (int tx=0; tx<signature->tilex; tx++) {
		  y=signature->insetbottom;
		  for (int ty=0; ty<signature->tiley; ty++) {

			if (folddirection=='l' || folddirection=='r') { //horizontal fold
				pts[0]=flatpoint(x+foldindex*ew,y+eh*foldr1);
				pts[1]=pts[0]+axis*w;
				pts[2]=pts[1]+flatpoint(0,h);
				pts[3]=pts[0]+flatpoint(0,h);
			} else { 							//vertical fold
				pts[0]=flatpoint(x+ew*foldc1,y+foldindex*eh);
				pts[1]=pts[0]+axis*h;
				pts[2]=pts[1]+flatpoint(w,0);
				pts[3]=pts[0]+flatpoint(w,0);
			}
			if (foldunder) dp->drawlines(pts,4,0,0);
			else dp->drawlines(pts,4,1,1);
			
			y+=patternheight+signature->tilegapy;
		  }
		  x+=patternwidth+signature->tilegapx;
		}
	}

	 //draw fold indicator overlays on left side of screen
	dp->LineAttributes(1, LineSolid, CapButt, JoinMiter);

	int thing;
	for (int c=signature->folds.n-1; c>=-1; c--) {
		if (c==-1) thing=THING_Circle;
		else if (c==signature->folds.n-1 && hasfinal) thing=THING_Square;
		else thing=THING_Triangle_Down;
		getFoldIndicatorPos(c, &x,&y,&w,&h);

		 //color hightlighted to show which fold we are currently on
		if (foldlevel==c+1) dp->NewFG(1.,.5,1.);
		else dp->NewFG(1.,1.,1.);

		dp->drawthing(x+w/2,y+h/2, w/2,h/2, 1, thing); //filled
		dp->NewFG(1.,0.,1.);
		dp->drawthing(x+w/2,y+h/2, w/2,h/2, 0, thing); //outline
	}

	 //draw sheet per signature indicator
	dp->LineAttributes(1, LineSolid, CapButt, JoinMiter);
	dp->NewFG(.5,0.,0.); //dark red for inset
	y=-signature->totalheight*.1;
	dp->drawline(0,y, signature->totalwidth,y);
	y-=signature->totalheight*.01;
	for (int c=1; c<5 && c<signature->sheetspersignature; c++) {
		dp->drawline(0,y, signature->totalwidth,y);
		y-=signature->totalheight*.01;
	}
	if (signature->sheetspersignature==0) sprintf(str,_("Many sheets in a single signature"));
	else if (signature->sheetspersignature==1) sprintf(str,_("1 sheet per signature"));
	else sprintf(str,_("%d sheets per signature"),signature->sheetspersignature);
	pts[0]=dp->realtoscreen(signature->totalwidth/2,y);
	dp->textout(pts[0].x,pts[0].y, str,-1, LAX_HCENTER|LAX_TOP);
	

//	 //-----------------draw control handles
//	int handlepos=3;
//	if (foldlevel==0 && signature->folds.n==0) {
//		handlepos=4;
//	}
//
//	 //draw arrow handles for inset and tile gap
//	if (foldlevel==0) {
//		dp->NewFG(1.,0.,1.);
//	}
//
//	 //draw arrow handles for trim and margin
//	if (hasfinal && foldlevel==signature->folds.n-1) {
//		 //trim handle
//		dp->NewFG(1.,0.,0.);
//
//		 //margin handle
//		dp->NewFG(0.,0.,1.);
//
//		 //binding edge handle
//		dp->NewFG(1.,0.,0.);
//	}

	return 0;
}

/*! which==-1 means all unfolded. 0 or more means that fold.
 */
void SignatureInterface::getFoldIndicatorPos(int which, double *x,double *y, double *w,double *h)
{
	int radius=10;

	*x=dp->Minx;
	*y=(dp->Maxy+dp->Miny)/2 - (signature->folds.n+1)*(radius-3);
	*w=2*radius;
	*h=2*radius;

	*y+=(which+1)*(2*radius-3);
}


#define SP_None           0

#define SP_Tile_X         1
#define SP_Tile_Y         2
#define SP_Tile_Gap_X     3
#define SP_Tile_Gap_Y     4

#define SP_Inset_Top      5
#define SP_Inset_Bottom   6
#define SP_Inset_Left     7
#define SP_Inset_Right    8

#define SP_H_Folds        9
#define SP_V_Folds        10

#define SP_Trim_Top       11
#define SP_Trim_Bottom    12
#define SP_Trim_Left      13
#define SP_Trim_Right     14

#define SP_Margin_Top     15
#define SP_Margin_Bottom  16
#define SP_Margin_Left    17
#define SP_Margin_Right   18

#define SP_Binding        19
#define SP_Up             20
#define SP_X              21
#define SP_Y              22

#define SP_Sheets_Per_Sig 23
#define SP_Stack_Or_Add   24

#define SP_FOLDS          100



//! Scan for handles to control variables, not for row and column.
int SignatureInterface::scanhandle(int x,int y)
{
	flatpoint fp=screentoreal(x,y);

	//double xx=fp.x;
	//double yy=fp.y;

	int point=SP_None;

//	if (foldlevel==0) {
//		 //check for inset handles, tilex/y, tile gap x/y
//		if (xx<0 || xx>totalwidth) {
//			if (yy>0 && yy<totalheight/2) point=SP_Inset_Left;
//			else if (yy>=totalheight/2 && yy<totalheight) point=SP_Tile_Y;
//		} else if (yy<0 || yy>totalheight) {
//			if (xx>0 && xx<totalwidth/2) point=SP_Tile_Y;
//			else if (yy>=totalheight/2 && yy<totalheight) point=SP_Inset_;
//		}
//	}
//
//	if (foldlevel==signature->folds.n-1) {
//		 //check for final page trim lrtb, margin, binding edge, up, x,y
//		***
//	}

	return point;
}

//! Return 0 for not in pattern, or the row and column in a folding section.
/*! x and y are real coordinates.
 *
 * Returns the real coordinate within an element of the folding pattern in ex,ey.
 */
int SignatureInterface::scan(int x,int y,int *row,int *col,double *ex,double *ey, int *tile_row, int *tile_col)
{
	flatpoint fp=screentoreal(x,y);
	DBG cerr <<"fp:"<<fp.x<<','<<fp.y<<endl;

	fp.x-=signature->insetleft;
	fp.y-=signature->insetbottom;

	double patternheight=signature->PatternHeight();
	double patternwidth =signature->PatternWidth();
	double elementheight=patternheight/(signature->numhfolds+1);
	double elementwidth =patternwidth /(signature->numvfolds+1);

	int tilex,tiley;
	tilex=floorl(fp.x/(patternwidth +signature->tilegapx));
	tiley=floorl(fp.y/(patternheight+signature->tilegapy));
	if (tile_col) *tile_col=tilex;
	if (tile_row) *tile_row=tiley;

	fp.x-=tilex*(patternwidth +signature->tilegapx);
	fp.y-=tiley*(patternheight+signature->tilegapy);

	DBG cerr <<"tilex,y: "<<tilex<<","<<tiley<<endl;

	*row=floorl(fp.y/elementheight);
	*col=floorl(fp.x/elementwidth);

	 //find coordinates within an element cell
	if (ey) *ey=fp.y-(*row)*elementheight;
	if (ex) *ex=fp.x-(*col)*elementwidth;

	return *row>=0 && *row<=signature->numhfolds && *col>=0 && *col<=signature->numvfolds;
}

//! Respond to spinning controls.
int SignatureInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	flatpoint fp=screentoreal(x,y);
	if (fp.x>0 && fp.x<signature->totalwidth && fp.y<-signature->totalheight*.1 && fp.y>-signature->totalheight*.2) {
		signature->sheetspersignature--;
		if (signature->sheetspersignature<0) signature->sheetspersignature=0;
		needtodraw=1;
		return 0;
	}
	return 1;
}

//! Respond to spinning controls.
int SignatureInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	flatpoint fp=screentoreal(x,y);
	if (fp.x>0 && fp.x<signature->totalwidth && fp.y<-signature->totalheight*.1 && fp.y>-signature->totalheight*.2) {
		signature->sheetspersignature++;
		needtodraw=1;
		return 0;
	}
	return 1;
}

int SignatureInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int row,col,tilerow,tilecol;
	int over=scan(x,y, &row,&col, NULL,NULL, &tilerow,&tilecol);
	DBG cerr <<"over element "<<over<<": r,c="<<row<<','<<col<<endl;

	if (buttondown.any()) return 0;

	buttondown.down(d->id,LEFTBUTTON, x,y, row,col);

	 //check overlays first
	double xx,yy,w,h;
	onoverlay=SP_None;
	for (int c=signature->folds.n-1; c>=-1; c--) {
		getFoldIndicatorPos(c, &xx,&yy,&w,&h);
		if (x>=xx && x<xx+w && y>=yy && y<yy+h) {
			onoverlay=SP_FOLDS+1+c;
			foldprogress=-1;
			return 0;
		}
	}

	if (row<0 || row>signature->numhfolds || col<0 || col>signature->numvfolds
		  || foldinfo[row][col].pages.n==0
		  || tilerow<0 || tilecol<0
		  || tilerow>signature->tiley || tilecol>signature->tilex) {
		lbdown_row=lbdown_col=-1;
	} else {
		lbdown_row=row;
		lbdown_col=col;
		folddirection=0;
		foldprogress=0;
	}


	return 0;
}

int SignatureInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!(buttondown.isdown(d->id,LEFTBUTTON))) return 1;
	int dragged=buttondown.up(d->id,LEFTBUTTON);

	if (onoverlay) {
		if (onoverlay>=SP_FOLDS) {
			 //selecting different fold maybe...
			if (!dragged) {
				 //we clicked down then up on the same overlay

				int folds=onoverlay-SP_FOLDS; //0 means totally unfolded

				if (foldlevel==folds) return 0; //already at that fold level

				 //we must remap the folds to reflect the new fold level
				resetFoldinfo();
				FoldDirectionType f;
				char folddir;
				int under;
				for (int c=0; c<folds; c++) {
					f=signature->folds.e[c]->direction;
					if (f==FOLD_Left_Under_To_Right || f==FOLD_Right_Under_To_Left 
						 || f==FOLD_Top_Under_To_Bottom || f==FOLD_Bottom_Under_To_Top) under=1;
					else under=0;

					if (f==FOLD_Left_Under_To_Right || f==FOLD_Left_Over_To_Right) folddir='r';
					else if (f==FOLD_Right_Over_To_Left  || f==FOLD_Right_Under_To_Left) folddir='l';
					else if (f==FOLD_Bottom_Over_To_Top  || f==FOLD_Bottom_Under_To_Top) folddir='t';
					else if (f==FOLD_Top_Over_To_Bottom  || f==FOLD_Top_Under_To_Bottom) folddir='b';

					applyFold(folddir, signature->folds.e[c]->whichfold, under);
				}
				foldlevel=folds;
				//checkFoldlevel(1);
				needtodraw=1;
			}
		}
		return 0;
	}

	if (folddirection && folddirection!='x' && foldprogress>.9) {
		 //apply the fold...
		int newr,newc, tr,tc;
		FoldDirectionType foldtype;
		for (int r=foldr1; r<=foldr2; r++) {
		  for (int c=foldc1; c<=foldc2; c++) {
			 //find new positions
			if (folddirection=='b') {
				newc=c;
				newr=foldindex-(r-foldindex+1);
			} else if (folddirection=='r') {
				newr=r;
				newc=foldindex+(foldindex-c-1);
			} else if (folddirection=='l') {
				newr=r;
				newc=foldindex-(c-foldindex+1);
			} else if (folddirection=='t') {
				newc=c;
				newr=foldindex+(foldindex-r-1);
			}

			 //swap old and new positions
			if (foldunder) {
				while(foldinfo[r][c].pages.n) {
					tc=foldinfo[r][c].pages.pop(0);
					tr=foldinfo[r][c].pages.pop(0);
					foldinfo[newr][newc].pages.push(tc,0);
					foldinfo[newr][newc].pages.push(tr,0);
				}
			} else {
				while(foldinfo[r][c].pages.n) {
					tc=foldinfo[r][c].pages.pop();
					tr=foldinfo[r][c].pages.pop();
					foldinfo[newr][newc].pages.push(tr);
					foldinfo[newr][newc].pages.push(tc);
				}
			}
			 //flip the original place.
			if (folddirection=='b' || folddirection=='t') foldinfo[tr][tc].y_flipped=!foldinfo[r][c].y_flipped;
			else foldinfo[tr][tc].x_flipped=!foldinfo[r][c].x_flipped;
		  }
		}
		if (folddirection=='b') {
			if (foldunder) foldtype=FOLD_Top_Under_To_Bottom;
			else 		   foldtype=FOLD_Top_Over_To_Bottom;
		} else if (folddirection=='t') {
			if (foldunder) foldtype=FOLD_Bottom_Under_To_Top;
			else 		   foldtype=FOLD_Bottom_Over_To_Top;
		} else if (folddirection=='r') {
			if (foldunder) foldtype=FOLD_Left_Under_To_Right;
			else           foldtype=FOLD_Left_Over_To_Right;
		} else if (folddirection=='l') {
			if (foldunder) foldtype=FOLD_Right_Under_To_Left;
			else           foldtype=FOLD_Right_Over_To_Left;
		}
		if (foldlevel<signature->folds.n) {
			while (foldlevel<signature->folds.n) signature->folds.remove();
		}
		signature->folds.push(new Fold(foldtype,foldindex),1);
		foldlevel=signature->folds.n;

		 //check the immediate neighors of newr,newc. If none, then we are totally folded.

		int stillmore=4;

		 //check above
		tr=newr-1; tc=newc;
		if (tr<0 || foldinfo[tr][tc].pages.n==0) stillmore--;

		 //check below
		tr=newr+1; tc=newc;
		if (tr>signature->numhfolds || foldinfo[tr][tc].pages.n==0) stillmore--;
		
		 //check left
		tr=newr; tc=newc-1;
		if (tc<0 || foldinfo[tr][tc].pages.n==0) stillmore--;

		 //check if right
		tr=newr; tc=newc+1;
		if (tc>signature->numvfolds || foldinfo[tr][tc].pages.n==0) stillmore--;

		if (stillmore==0) {
			hasfinal=1;
			finalr=newr;
			finalc=newc;

			int page=0,xflip,yflip;
			for (int c=foldinfo[finalr][finalc].pages.n-2; c>=0; c-=2) {
				tr=foldinfo[finalr][finalc].pages.e[c];
				tc=foldinfo[finalr][finalc].pages.e[c+1];

				xflip=foldinfo[tr][tc].x_flipped;
				yflip=foldinfo[tr][tc].y_flipped;

				if ((xflip && !yflip) || (!xflip && yflip)) {
					 //back side of paper is up
					foldinfo[tr][tc].finalindexback=page;
					foldinfo[tr][tc].finalindexfront=page+1;
				} else {
					foldinfo[tr][tc].finalindexback=page+1;
					foldinfo[tr][tc].finalindexfront=page;
				}
				page+=2;
			}
		} else hasfinal=0;

		folddirection=0;
		foldprogress=0;
	}

	needtodraw=1;
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
	int row,col,tilerow,tilecol;
	flatpoint mm;
	int over=scan(x,y, &row,&col, &mm.x,&mm.y, &tilerow,&tilecol);
	//fp now holds coordinates relative to the element cell

	DBG cerr <<"over element "<<over<<": r,c="<<row<<','<<col<<"  mm="<<mm.x<<','<<mm.y<<"  tile r,c:"<<tilerow<<','<<tilecol;
	DBG if (row>=0 && row<signature->numhfolds+1 && col>=0 && col<signature->numvfolds+1)
	DBG    cerr <<"  xflip: "<<foldinfo[row][col].x_flipped<<"  yflip:"<<foldinfo[row][col].y_flipped
	DBG         <<"  pages:"<<foldinfo[row][col].pages.n<<endl;

	int mx,my;
	if (!buttondown.any()) return 0;
	buttondown.move(mouse->id,x,y);

	if (lbdown_row<0 || lbdown_col<0) return 0;
	if ((hasfinal && foldlevel==signature->folds.n-1)
			|| row<0 || row>signature->numhfolds || col<0 || col>signature->numvfolds) {
		if (folddirection!=0) {
			folddirection=0;
			needtodraw=1;
		}
		return 0;
	}
	foldunder=(state&LAX_STATE_MASK)!=0;

	buttondown.getinitial(mouse->id,LEFTBUTTON, &mx,&my);
	int ocol,orow, otrow,otcol;
	flatpoint om;
	scan(mx,my, &orow,&ocol, &om.x,&om.y, &otrow,&otcol);

	if (tilerow<otrow) row-=100;
	else if (tilerow>otrow) row+=100;
	if (tilecol<otcol) col-=100;
	else if (tilecol>otcol) col+=100;
	

	flatpoint d=screentoreal(x,y)-screentoreal(mx,my); 

	 //find the direction we are trying to fold in
	if (folddirection==0) {
		if (fabs(d.x)>fabs(d.y)) {
			if (d.x>0) {
				folddirection='r';
			} else if (d.x<0) {
				folddirection='l';
			}
		} else {
			if (d.y>0) {
				folddirection='t';
			} else if (d.y<0) {
				folddirection='b';
			}
		}
	}

	 //find how far we fold based on proximity of mouse to fold crease

	 //figure out which elements are affected by folding
	double elementwidth =signature->PageWidth();
	double elementheight=signature->PageHeight();
	if (folddirection=='r') {
		int adjacentcol=ocol+1; //edge is between ocol and adjacentcol
		int prevcol=ocol;
		while (prevcol>0 && foldinfo[orow][prevcol-1].pages.n!=0) prevcol--;
		int nextcol=adjacentcol;
		while (nextcol<signature->numvfolds) nextcol++; //it's ok to fold over onto blank areas

		if (nextcol>signature->numvfolds || nextcol-adjacentcol+1<ocol-prevcol+1
				|| (adjacentcol<=signature->numvfolds && foldinfo[orow][adjacentcol].pages.n==0)) {
			 //can't do the fold
			folddirection='x';
		} else {
			 //we can do the fold
			foldindex=ocol+1;

			 //find the fold progress
			if (col==ocol) foldprogress=.5-(elementwidth-mm.x)/(elementwidth-om.x)/2;
			else if (col==ocol+1) foldprogress=.5+(mm.x)/(elementwidth-om.x)/2;
			else if (col<ocol) foldprogress=0;
			else foldprogress=1;
			if (foldprogress>1) foldprogress=1;
			if (foldprogress<0) foldprogress=0;

			 //need to find upper and lower affected elements
			foldc1=prevcol;
			foldc2=ocol;
			foldr1=orow;
			foldr2=orow;
			while (foldr1>0 && foldinfo[foldr1-1][ocol].pages.n!=0) foldr1--;
			while (foldr2<signature->numhfolds && foldinfo[foldr2+1][ocol].pages.n!=0) foldr2++;
		}
		needtodraw=1;

	} else if (folddirection=='l') {
		int adjacentcol=ocol-1; //edge is between ocol and adjacentcol
		int nextcol=ocol;
		while (nextcol<signature->numvfolds && foldinfo[orow][nextcol+1].pages.n!=0) nextcol++;
		int prevcol=adjacentcol;
		while (prevcol>0) prevcol--; //it's ok to fold over onto blank areas

		if (prevcol<0 || adjacentcol-prevcol+1<nextcol-ocol+1
				|| (adjacentcol>=0 && foldinfo[orow][adjacentcol].pages.n==0)) {
			 //can't do the fold
			folddirection='x';
		} else {
			 //we can do the fold
			foldindex=ocol;

			 //find the fold progress
			if (col==ocol) foldprogress=.5-mm.x/om.x/2;
			else if (col==ocol-1) foldprogress=.5+(elementwidth-mm.x)/om.x/2;
			else if (col>ocol) foldprogress=0;
			else foldprogress=1;
			if (foldprogress>1) foldprogress=1;
			if (foldprogress<0) foldprogress=0;

			 //need to find upper and lower affected elements
			foldc1=ocol;
			foldc2=nextcol;
			foldr1=orow;
			foldr2=orow;
			while (foldr1>0 && foldinfo[foldr1-1][ocol].pages.n!=0) foldr1--;
			while (foldr2<signature->numhfolds && foldinfo[foldr2+1][ocol].pages.n!=0) foldr2++;
		}
		needtodraw=1;

	} else if (folddirection=='t') {
		int adjacentrow=orow+1; //edge is between ocol and adjacentcol
		int prevrow=orow;
		while (prevrow>0 && foldinfo[prevrow-1][ocol].pages.n!=0) prevrow--;
		int nextrow=adjacentrow;
		while (nextrow<signature->numhfolds) nextrow++; //it's ok to fold over onto blank areas

		if (nextrow>signature->numhfolds || nextrow-adjacentrow+1<orow-prevrow+1
				|| (adjacentrow<=signature->numhfolds && foldinfo[adjacentrow][ocol].pages.n==0)) {
			 //can't do the fold
			folddirection='x';
		} else {
			 //we can do the fold
			foldindex=orow+1;

			 //find the fold progress
			if (row==orow) foldprogress=.5-(elementheight-mm.y)/(elementheight-om.y)/2;
			else if (row==orow+1) foldprogress=.5+(mm.y)/(elementheight-om.y)/2;
			else if (row<orow) foldprogress=0;
			else foldprogress=1;
			if (foldprogress>1) foldprogress=1;
			if (foldprogress<0) foldprogress=0;

			 //need to find upper and lower affected elements
			foldr1=prevrow;
			foldr2=orow;
			foldc1=ocol;
			foldc2=ocol;
			while (foldc1>0 && foldinfo[orow][foldc1-1].pages.n!=0) foldc1--;
			while (foldc2<signature->numvfolds && foldinfo[orow][foldc2+1].pages.n!=0) foldc2++;
		}
		needtodraw=1;

	} else if (folddirection=='b') {
		int adjacentrow=orow-1; //edge is between orow and adjacentrow
		int nextrow=orow;
		while (nextrow<signature->numhfolds && foldinfo[nextrow+1][ocol].pages.n!=0) nextrow++;
		int prevrow=adjacentrow;
		while (prevrow>0) prevrow--; //it's ok to fold over onto blank areas

		if (prevrow<0 || adjacentrow-prevrow+1<nextrow-orow+1
				|| (adjacentrow>=0 && foldinfo[adjacentrow][ocol].pages.n==0)) {
			 //can't do the fold
			folddirection='x';
		} else {
			 //we can do the fold
			foldindex=orow;

			 //find the fold progress
			if (row==orow) foldprogress=.5-mm.y/om.y/2;
			else if (row==orow-1) foldprogress=.5+(elementheight-mm.y)/om.y/2;
			else if (row>orow) foldprogress=0;
			else foldprogress=1;
			if (foldprogress>1) foldprogress=1;
			if (foldprogress<0) foldprogress=0;

			 //need to find upper and lower affected elements
			foldr1=orow;
			foldr2=nextrow;
			foldc1=ocol;
			foldc2=ocol;
			while (foldc1>0 && foldinfo[orow][foldc1-1].pages.n!=0) foldc1--;
			while (foldc2<signature->numvfolds && foldinfo[orow][foldc2+1].pages.n!=0) foldc2++;
		}
		needtodraw=1;
	}

	DBG cerr <<"folding progress: "<<foldprogress<<",  om="<<om.x<<','<<om.y<<"  mm="<<mm.x<<','<<mm.y<<endl;


	return 0;
}

/*!
 * \todo inset shortcuts should be able to selectively adjust lrt or b insets and other things, not just all at once,
 * 		maybe have 'i' toggle which inset value to adjust, then subsequent arrow keys adjust values, modifiers adjust
 * 		the scale of the adjustment step
 */
int SignatureInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" SignatureInterface got ch:"<<ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;

	if (ch=='d' && (state&LAX_STATE_MASK)==0) {
		showdecs++;
		if (showdecs>2) showdecs=0;
		needtodraw=1;
		return 0;

		//--------------select how many folds to display
	} else if (ch>='0' && ch <='9') {
		// ***
		return 0;

		//--------------change inset
	} else if (ch=='i') {
		signature->insettop+=totalheight*.01;
		signature->insetbottom+=totalheight*.01;
		signature->insetleft+=totalheight*.01;
		signature->insetright+=totalheight*.01;
		needtodraw=1;
		return 0;

	} else if (ch=='I') {
		signature->insettop-=totalheight*.01;
		if (signature->insettop<0) signature->insettop=0;
		signature->insetbottom=signature->insetleft=signature->insetright=signature->insettop;
		needtodraw=1;
		return 0;

		//--------------change tile gap
	} else if (ch=='g') {
		signature->tilegapx+=totalheight*.01;
		signature->tilegapy+=totalheight*.01;
		needtodraw=1;
		return 0;

	} else if (ch=='G') {
		signature->tilegapx-=totalheight*.01;
		if (signature->tilegapx<0) signature->tilegapx=0;
		signature->tilegapy=signature->tilegapx;
		needtodraw=1;
		return 0;

		//-------------tilex and tiley
	} else if (ch=='x') {
		signature->tilex++;
		needtodraw=1;
		return 0;

	} else if (ch=='X') {
		signature->tilex--;
		if (signature->tilex<=1) signature->tilex=1;
		needtodraw=1;
		return 0;

	} else if (ch=='y') {
		signature->tiley++;
		needtodraw=1;
		return 0;

	} else if (ch=='Y') {
		signature->tiley--;
		if (signature->tiley<=1) signature->tiley=1;
		needtodraw=1;
		return 0;

		//-----------numhfolds and numvfolds
	} else if (ch=='v') {
		if (foldlevel!=0) return 0;
		signature->numvfolds++;
		reallocateFoldinfo();
		needtodraw=1;
		return 0;

	} else if (ch=='V') {
		if (foldlevel!=0) return 0;
		int old=signature->numvfolds;
		signature->numvfolds--;
		if (signature->numvfolds<=0) signature->numvfolds=0;
		if (old!=signature->numvfolds) {
			reallocateFoldinfo();
			needtodraw=1;
		}
		return 0;

	} else if (ch=='h') {
		if (foldlevel!=0) return 0;
		signature->numhfolds++;
		reallocateFoldinfo();
		needtodraw=1;
		return 0;

	} else if (ch=='H') {
		if (foldlevel!=0) return 0;
		int old=signature->numhfolds;
		signature->numhfolds--;
		if (signature->numhfolds<=0) signature->numhfolds=0;
		if (old!=signature->numhfolds) {
			reallocateFoldinfo();
			needtodraw=1;
		}
		return 0;

		//---------------move binding edge
	} else if (ch=='b') {
		if (signature->binding=='l') signature->binding='t';
		else if (signature->binding=='t') signature->binding='r';
		else if (signature->binding=='r') signature->binding='b';
		else if (signature->binding=='b') signature->binding='l';
		needtodraw=1;
		return 0;

	} else if (ch=='B') {
		if (signature->binding=='l') signature->binding='b';
		else if (signature->binding=='b') signature->binding='r';
		else if (signature->binding=='r') signature->binding='t';
		else if (signature->binding=='t') signature->binding='l';
		needtodraw=1;
		return 0;

		//---------------page orientation
	} else if (ch=='u') { //up direction
		if (signature->up=='l') signature->up='t';
		else if (signature->up=='t') signature->up='r';
		else if (signature->up=='r') signature->up='b';
		else if (signature->up=='b') signature->up='l';
		needtodraw=1;
		return 0;

		//--------------change page trim
	} else if (ch=='t') {
		signature->trimtop   +=signature->PageHeight()*.01;
		signature->trimbottom+=signature->PageHeight()*.01;
		signature->trimleft  +=signature->PageHeight()*.01;
		signature->trimright +=signature->PageHeight()*.01;
		needtodraw=1;
		return 0;

	} else if (ch=='T') {
		signature->trimtop-=signature->PageHeight()*.01;
		if (signature->trimtop<0) signature->trimtop=0;
		signature->trimbottom=signature->trimleft=signature->trimright=signature->trimtop;
		needtodraw=1;
		return 0;

		//--------------change page margin
	} else if (ch=='m') {
		signature->margintop   +=signature->PageHeight()*.01;
		signature->marginbottom+=signature->PageHeight()*.01;
		signature->marginleft  +=signature->PageHeight()*.01;
		signature->marginright +=signature->PageHeight()*.01;
		needtodraw=1;
		return 0;

	} else if (ch=='M') {
		signature->margintop-=signature->PageHeight()*.01;
		if (signature->margintop<0) signature->margintop=0;
		signature->marginbottom=signature->marginleft=signature->marginright=signature->margintop;
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


//----------------------- SignatureEditor --------------------------------------

/*! \class SignatureEditor
 * \brief A Laxkit::ViewerWindow that gets filled with stuff appropriate for signature editing.
 *
 * This creates the window with a SignatureInterface.
 */


//! Make the window using project.
/*! Inc count of ndoc.
 */
SignatureEditor::SignatureEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
						Laxkit::anXWindow *nowner, const char *mes,
						Signature *sig, PaperStyle *p)
	: ViewerWindow(parnt,nname,ntitle,
				   ANXWIN_REMEMBER
					|VIEWPORT_RIGHT_HANDED|VIEWPORT_BACK_BUFFER|VIEWPORT_NO_SCROLLERS|VIEWPORT_NO_RULERS, 
					0,0,500,500, 0, NULL)
{
	SetOwner(nowner,mes);

	//signature=sig;
	//if (signature) signature->inc_count();

	if (!viewport) viewport=new ViewportWindow(this,"signature-editor-viewport","signature-editor-viewport",
									ANXWIN_HOVER_FOCUS|VIEWPORT_RIGHT_HANDED|VIEWPORT_BACK_BUFFER|VIEWPORT_ROTATABLE,
									0,0,0,0,0,NULL);

	win_colors->bg=rgbcolor(200,200,200);
	viewport->dp->NewBG(200,200,200);

	needtodraw=1;
	AddTool(new SignatureInterface(1,viewport->dp,sig,p),1,1); // local, and select it
	// *** add signature and paper if any...
}

SignatureEditor::~SignatureEditor()
{ }

//! Passes off to SignatureInterface::dump_out().
void SignatureEditor::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	// *** ((SignatureInterface *)curtool)->dump_out(f,indent,what,context);
}

//! Passes off to SignatureInterface::dump_in_atts().
void SignatureEditor::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	// *** ((SignatureInterface *)curtool)->dump_in_atts(att,flag,context);
}

/*! Removes rulers and adds Apply, Reset, and Update Thumbs.
 */
int SignatureEditor::init()
{
	ViewerWindow::init();
	viewport->dp->NewBG(200,200,200);

	anXWindow *last=NULL;
	Button *tbut;

	last=tbut=new Button(this,"ok",NULL, 0, 0,0,0,0,1, last,object_id,"ok",0,_("Ok"));
	AddWin(tbut,tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0);

	last=tbut=new Button(this,"cancel",NULL, 0, 0,0,0,0,1, last,object_id,"cancel",0,_("Cancel"));
	AddWin(tbut,tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0);

	Sync(1);	

	SignatureInterface *si=dynamic_cast<SignatureInterface *>(tools.e[0]);
	int h=si->signature->totalheight;
	int w=si->signature->totalwidth;
	viewport->dp->Center(-w*.15,w*1.15, -h*.15,h*1.15);
	return 0;
}

/*! Responds to: "ok", "cancel"
 *
 */
int SignatureEditor::Event(const Laxkit::EventData *data,const char *mes)
{
	DBG cerr <<"SignatureEditor got message: "<<(mes?mes:"?")<<endl;

	if (!strcmp(mes,"ok")) {
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

	} else if (!strcmp("cancel",mes)) {
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

	}
	return 1;
}

int SignatureEditor::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (ch==LAX_Esc) {
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

//	} else if (ch==LAX_F1 && (state&LAX_STATE_MASK)==0) {
//		app->addwindow(new HelpWindow());
//		return 0;
	}
	return 1;
}



//} // namespace Laidout

