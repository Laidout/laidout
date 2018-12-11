//
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2010-2013 by Tom Lechner
//

#include "signatures.h"
#include "signatureinterface.h"
#include "stylemanager.h"
#include "../language.h"

#include <lax/interfaces/pathinterface.h>
#include <lax/attributes.h>
#include <lax/transformmath.h>
#include <lax/lists.cc>


using namespace Laxkit;
using namespace LaxInterfaces;
using namespace LaxFiles;

#define DBG 



namespace Laidout {


//for automarks: gap is 1/8 inch, mwidth is about 1/72.. at some point these will be customizable
#define GAP .0625
#define MWIDTH  .014

//----------------------------- naming helper functions -----------------------------

//! Return the name of the fold direction.
/*! This will be "Right" or "Under Left" or some such.
 *
 * If translate!=0, then return that translated, else return english.
 */
const char *FoldDirectionName(char dir, int under, int translate)
{
	const char *str=NULL;

	if (under) {
		if (dir=='r')      str= translate ? _("Under Right") :"Under Right";
		else if (dir=='l') str= translate ? _("Under Left")  :"Under Left";
		else if (dir=='b') str= translate ? _("Under Bottom"):"Under Bottom";
		else if (dir=='t') str= translate ? _("Under Top")   :"Under Top";
	} else {
		if (dir=='r')      str= translate ? _("Right") :"Right";
		else if (dir=='l') str= translate ? _("Left")  :"Left";
		else if (dir=='b') str= translate ? _("Bottom"):"Bottom";
		else if (dir=='t') str= translate ? _("Top")   :"Top";
	}

	return str;
}

/*! \ingroup misc
 *
 * For case insensitive comparisons to "left", "right", "top", "bottom", return
 * 'l', 'r', 't', 'b'.
 *
 * If one of those are found, return 0, else 1.
 */
int LRTBAttribute(const char *v, char *ret)
{
	if (!strcasecmp(v,"left"))   { *ret='l'; return 0; }
	if (!strcasecmp(v,"right"))  { *ret='r'; return 0; }
	if (!strcasecmp(v,"top"))    { *ret='t'; return 0; }
	if (!strcasecmp(v,"bottom")) { *ret='b'; return 0; }

	return 1;
}

//! Convert 't','b','l','r' to top,bottom,left,right.
/*! \ingroup misc
 */
const char *CtoStr(char c)
{
	if (c=='t' || c=='u') return "Top";
	if (c=='b' || c=='d') return "Bottom";
	if (c=='l') return "Left";
	if (c=='r') return "Right";
	return NULL;
}


//------------------------------------- Fold --------------------------------------

/*! \class Fold
 * \brief Line description node in a Signature
 *
 * Each line can be folds or cuts. Cuts can be tiling cuts or finishing cuts.
 */
/*! \var FoldDirectionType Fold::folddirection
 * \brief l over to r, l under to r, rol, rul, tob, tub, bot
 */
/*! \var int FoldDirection::whichfold
 * \brief Index from the left or bottom side of completely unfolded paper of which fold to act on.
 *
 * 1 is the left (or bottom) most fold. numhfolds is the top most fold, and numvfolds is the right most fold.
 */


Value *Fold::duplicate()
{ return new Fold(direction,under,whichfold); }

/*! Create a new ObjectDef of Fold.
 */
ObjectDef *Fold::makeObjectDef()
{
	ObjectDef *foldd=stylemanager.FindDef("Fold");
	if (foldd) return foldd;

	foldd=new ObjectDef(NULL,"Fold",
			_("Fold"),
			_("Info about a fold in a signature"),
			"class",
			NULL,NULL, //range, default value
			NULL, //fields
			0, //new flags
			NULL, //newfunc
			NULL /*createFold*/); //newfunc with parameters

	foldd->push("index", _("Index"), _("The index of the fold, starting from 0, from the top or left."),
			"int", "[0..", "0", 0, NULL);

	foldd->pushEnum("direction", _("Direction"), _("Direction of the fold: left, right, top, or bottom."),
				 NULL, NULL, NULL,
				 "Left",_("Left"),_("Right over to Left"),
				 "UnderLeft",_("Under Left"),_("Right under to Left"),
				 "Right",_("Right"),_("Left over to Right"),
				 "UnderRight",_("Right"),_("Left under to Right"),
				 "Top",_("Top"),_("Bottom over to Top"),
				 "UnderTop",_("Top"),_("Bottom under to Top"),
				 "Bottom",_("Bottom"),_("Top over to Bottom"),
				 "UnderBottom",_("Bottom"),_("Top under to Bottom"),
				 NULL
				);

	stylemanager.AddObjectDef(foldd,0);
	foldd->dec_count();

	return foldd;
}

/*! Return a ValueObject with a Fold, for use in scripting.
 */
int createFold(ValueHash *context, ValueHash *parameters,
			   Value **value_ret, ErrorLog &log)
{
	if (!parameters || !parameters->n()) {
		if (value_ret) *value_ret=NULL;
		log.AddMessage(_("Missing parameters!"),ERROR_Fail);
		return 1;
	}

	int index=-1;
	int under=0;
	char dir=0;

	char error[100];
	int err=0;
	try {
		int i, e;

		 //---index
		i=parameters->findInt("index",-1,&e);
		if (e==0) { 
			if (i<0) {
				sprintf(error, _("Index out of range!")); 
				throw error;
			} else index=i; 
		}
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"index"); throw error; }

		 //---direction
		i=parameters->findInt("direction",-1,&e); //enums are converted to integers in LaidoutCalculator
		if (e==0) {
			if (i==0) { dir='l'; }
			else if (i==1) { dir='l'; under=1; }
			else if (i==2) { dir='r'; }
			else if (i==3) { dir='r'; under=1; }
			else if (i==4) { dir='t'; }
			else if (i==6) { dir='t'; under=1; }
			else if (i==5) { dir='b'; }
			else if (i==7) { dir='b'; under=1; }
		} else if (e==2) { sprintf(error, _("Invalid format for %s!"),"direction"); throw error; }


	} catch (const char *str) {
		log.AddMessage(str,ERROR_Fail);
		err=1;
	}

	if (value_ret && err==0) {
		*value_ret=NULL;

		if (dir!=0 && index>=0) {
			*value_ret=new Fold(dir,under,index);

		} else {
			log.AddMessage(_("Incomplete Fold definition!"),ERROR_Fail);
			err=1;
		}
		
	}

	return err;
}



//------------------------------------- FoldedPageInfo --------------------------------------
/*! \class FoldedPageInfo
 * \brief Info about partial folded states of Signature.
 */
/*! \var int FoldedPageInfo::rotation
 * Minute adjustment to make to a final cell, as viewed clockwise on the front side.
 * Sometimes predictable minute adjustments must be made to pages due to the effects
 * of paper thickness on folding.
 */
/*! \var int FoldedPageInfo::shift
 * Minute adjustment to make to a final cell, as viewed on the front side.
 * An example would be creep introduced from a fold on many sheets.
 */

FoldedPageInfo::FoldedPageInfo()
{
	x_flipped=y_flipped=0;
	currentrow=currentcol=0;

	finalxflip=finalyflip=0;
	finalindexback=finalindexfront=-1;

	rotation=0;
}

//void dumpfoldinfo(FoldedPageInfo **finfo, int numhfolds, int numvfolds)
//{
//	cerr <<" -- foldinfo: --"<<endl;
//	for (int r=0; r<numhfolds+1; r++) {
//		for (int c=0; c<numvfolds+1; c++) {
//			cerr <<"  ";
//			for (int i=0; i<finfo[r][c].pages.n; i++) {
//				cerr<<finfo[r][c].pages.e[i]<< (i<finfo[r][c].pages.n-1?"/":"");
//			}
//			//finfo[r][c].x_flipped=0;
//			//finfo[r][c].y_flipped=0;
//		}
//		cerr <<endl;
//	}
//}


//------------------------------------- Signature --------------------------------------
/*! \class Signature
 * \brief A folding pattern used as the basis for an SignatureImposition. 
 *
 * Note Signature objects are ONLY the folding pattern, not how many sheets, or paper information.
 */
/*! \var double Signature::marginleft
 * \brief A final left page margin.
 */
/*! \var double Signature::marginright
 * \brief A final right page margin.
 */
/*! \var double Signature::margintop
 * \brief A final top page margin.
 */
/*! \var double Signature::marginbottom
 * \brief A final bottom margin.
 */
/*! \var double Signature::trimleft
 * \brief A final left trim.
 */
/*! \var double Signature::trimright
 * \brief A final right trim.
 */
/*! \var double Signature::trimtop
 * * \brief A final top trim.
 */
/*! \var double Signature::trimbottom
 * \brief A final bottom trim.
 */
/*! \var int Signature::numhfolds
 * \brief The number of horizontal fold lines in the folding pattern.
 */
/*! \var int Signature::numvfolds
 * \brief The number of vertical fold lines in the folding pattern.
 */
/*! \var int Signature::sheetspersignature
 * \brief The number of sheets to stack in a signature before folding.
 *
 * This is a hint only. The actual used value is in SignatureInstance.
 *
 * These keeps track of the actual number of sheets per signature. For arrangements where
 * one adds sheets to the same signature when more pages are added (ie saddle stitched booklets),
 * then autoaddsheets will be 1.
 *
 * Note that there are 2 paper spreads per sheet.
 */
/*! \var int Signature::autoaddsheets
 * \brief Whether to stack up more sheets of paper in a signature when adding pages.
 *
 * If nonzero, then any increase will only use more stacked sheets per signature. Otherwise, 
 * more signatures are used, and it is assumed that these will be bound back to back.
 */

Signature::Signature()
{
	description=NULL;
	name=NULL;

	patternwidth=patternheight=1;

	sheetspersignature=1;

	numhfolds=numvfolds=0;
	trimleft=trimright=trimtop=trimbottom=0;
	marginleft=marginright=margintop=marginbottom=0;

	up='t';         //which direction is up 'l|r|t|b', ie 'l' means points toward the left
	binding='l';    //direction to place binding 'l|r|t|b'
	positivex='r';  //direction of the positive x axis: 'l|r|t|b'
	positivey='t';  //direction of the positive x axis: 'l|r|t|b', for when up might not be positivey!

	foldinfo=NULL;
	reallocateFoldinfo();
	foldinfo[0][0].finalindexfront=0;
	foldinfo[0][0].finalindexback=1;

}

Signature::~Signature()
{
	if (description) delete[] description;
	if (name) delete[] name;

	if (foldinfo) {
		for (int c=0; foldinfo[c]; c++) delete[] foldinfo[c];
		delete[] foldinfo;
	}
}

const Signature &Signature::operator=(const Signature &sig)
{
	patternwidth=sig.patternwidth;
	patternheight=sig.patternheight;

	sheetspersignature=sig.sheetspersignature;

	numhfolds=sig.numhfolds;
	numvfolds=sig.numvfolds;
	folds.flush();
	for (int c=0; c<sig.folds.n; c++) {
		folds.push(new Fold(sig.folds.e[c]->direction, sig.folds.e[c]->under, sig.folds.e[c]->whichfold), 1);
	}

	trimleft=sig.trimleft;
	trimright=sig.trimright;
	trimtop=sig.trimtop;
	trimbottom=sig.trimbottom;
	marginleft=sig.marginleft;
	marginright=sig.marginright;
	margintop=sig.margintop;
	marginbottom=sig.marginbottom;

	up=sig.up;
	binding=sig.binding;
	positivex=sig.positivex;
	positivey=sig.positivey;

	reallocateFoldinfo();
	applyFold(NULL,-1);
	checkFoldLevel(NULL,NULL,NULL);

	return *this;
}

Value *Signature::duplicate()
{
	Signature *sig=new Signature;
	*sig=*this;
	return sig;
}


//! Reallocate and map foldinfo.
/*! This will base the new foldinfo on numhfolds and numvfolds.
 *
 * Each fold info cell will have only itself in its pages stack.
 * To apply folds, use applyFold().
 */
void Signature::reallocateFoldinfo()
{
	if (foldinfo) {
		for (int c=0; foldinfo[c]; c++) delete[] foldinfo[c];
		delete[] foldinfo;
	}
	foldinfo=new FoldedPageInfo*[numhfolds+2];
	int r;
	for (r=0; r<numhfolds+1; r++) {
		foldinfo[r]=new FoldedPageInfo[numvfolds+2];
		for (int c=0; c<numvfolds+1; c++) {
			foldinfo[r][c].pages.push(r);
			foldinfo[r][c].pages.push(c);
		}
	}
	foldinfo[r]=NULL; //terminating NULL, so we don't need to remember sig->numhfolds
}

//! Flush all the foldinfo pages stacks, as if there have been no folds yet.
/*! Please note this does not create or reallocate foldinfo. 
 * If finfo==NULL then use this->foldinfo.
 *
 * It is assumed that finfo is allocated properly for the number of folds.
 *
 * Note that this does not alter this->folds or the final info in finfo.
 */
void Signature::resetFoldinfo(FoldedPageInfo **finfo)
{
	if (!finfo) finfo=foldinfo;

	for (int r=0; r<numhfolds+1; r++) {
		for (int c=0; c<numvfolds+1; c++) {
			finfo[r][c].pages.flush();
			finfo[r][c].pages.push(r);
			finfo[r][c].pages.push(c);

			finfo[r][c].x_flipped=0;
			finfo[r][c].y_flipped=0;
		}
	}
}

/*! If foldlevel==0, then fill with info about when there are no folds.
 * foldlevel==1 means after the 1st fold in the folds stack, etc.
 * foldlevel==-1 means apply all the folds, and apply final page settings.
 *
 * First the fold info is reset, then each fold is applied up to foldlevel.
 * If foldlevel==0, then the result is for no folds done.
 */
int Signature::applyFold(FoldedPageInfo **finfo, int foldlevel)
{
	if (!finfo) {
		if (!foldinfo) reallocateFoldinfo();
		finfo=foldinfo;
	}

	resetFoldinfo(finfo);

	if (foldlevel<0) foldlevel=folds.n;
	Fold *fold;
	for (int c=0; c<foldlevel; c++) {
		fold=folds.e[c];
		applyFold(finfo, fold->direction, fold->whichfold, fold->under);
	}
	return 0;
}

//! Low level flipping across folds.
/*! This will flip everything on one side of a fold to the other side (if possible).
 * It is not a selective flipping.
 *
 * This is called to ONLY apply the fold. It does not check and apply final index settings
 * or check for validity of the fold.
 *
 * If finfo==NULL, then use this->foldinfo.
 *
 * index==1 is the first fold from left or bottom.
 */
void Signature::applyFold(FoldedPageInfo **finfo, char folddir, int index, int under)
{
	if (!finfo) finfo=foldinfo;

	int newr,newc, tr,tc;
	int fr1,fr2, fc1,fc2;

	 //find the cells that must be moved
	if (folddir=='r') {
		fr1=0;
		fr2=numhfolds;
		fc1=0;
		fc2=index-1;
	} else if (folddir=='b') {
		fr1=index;
		fr2=numhfolds;
		fc1=0;
		fc2=numvfolds;
	} else if (folddir=='t') {
		fr1=0;
		fr2=index-1;
		fc1=0;
		fc2=numvfolds;
	} else { //if (folddir=='l') {
		fr1=0;
		fr2=numhfolds;
		fc1=index;
		fc2=numvfolds;
	}

	 //move the cells
	for (int r=fr1; r<=fr2; r++) {
	  for (int c=fc1; c<=fc2; c++) {
		if (finfo[r][c].pages.n==0) continue; //skip blank cells

		 //find new positions
		if (folddir=='b') {
			newc=c;
			newr=index-(r-(index-1));
		} else if (folddir=='t') {
			newc=c;
			newr=index+(index-1-r);
		} else if (folddir=='l') {
			newr=r;
			newc=index-(c-(index-1));
		} else if (folddir=='r') {
			newr=r;
			newc=index+(index-1-c);
		}

		 //swap old and new positions
		if (under) {
			while(finfo[r][c].pages.n) {
				tr=finfo[r][c].pages.pop(0);
				tc=finfo[r][c].pages.pop(0);
				finfo[newr][newc].pages.push(tc,0);
				finfo[newr][newc].pages.push(tr,0);

				 //flip the original place.
				if (folddir=='b' || folddir=='t') finfo[tr][tc].y_flipped=!finfo[tr][tc].y_flipped;
				else finfo[tr][tc].x_flipped=!finfo[tr][tc].x_flipped;
			}
		} else {
			while(finfo[r][c].pages.n) {
				tc=finfo[r][c].pages.pop();
				tr=finfo[r][c].pages.pop();
				finfo[newr][newc].pages.push(tr);
				finfo[newr][newc].pages.push(tc);

				 //flip the original place.
				if (folddir=='b' || folddir=='t') finfo[tr][tc].y_flipped=!finfo[tr][tc].y_flipped;
				else finfo[tr][tc].x_flipped=!finfo[tr][tc].x_flipped;
			}
		}
	  }
	}
}

/*! Return 1 for final info found in foldinfo.
 * Very simple check, just whether foldinfo[0][0]->finalindexfront is >=0.
 */
int Signature::HasFinal()
{
	return foldinfo[0][0].finalindexfront >= 0;
}

//! Check if the signature is totally folded or not.
/*! If we are totally folded, then apply the page indices to finfo.
 *
 * Returns 1 if we are totally folded. Otherwise 0. If we are totally folded, then
 * return the final position in finalrow,finalcol. Put -1 in each if we are not totally folded.
 *
 * If finfo==NULL, then use this->foldinfo.
 */
int Signature::checkFoldLevel(FoldedPageInfo **finfo, int *finalrow,int *finalcol)
{
	if (!finfo) finfo=foldinfo;

	 //check the immediate neighors of the first cell with pages.
	 //If there are no neighbors, then we are totally folded.

	 //find a non blank cell
	int newr=0,newc=0;
	for (newr=0; newr<=numhfolds; newr++) {
	  for (newc=0; newc<=numvfolds; newc++) {
		if (finfo[newr][newc].pages.n!=0) break;
	  }
	  if (newc!=numvfolds+1) break;
	}

	int hasfinal=0;
	int stillmore=4;
	int tr,tc;

	 //check above
	tr=newr-1; tc=newc;
	if (tr<0 || finfo[tr][tc].pages.n==0) stillmore--;

	 //check below
	tr=newr+1; tc=newc;
	if (tr>numhfolds || finfo[tr][tc].pages.n==0) stillmore--;
	
	 //check left
	tr=newr; tc=newc-1;
	if (tc<0 || finfo[tr][tc].pages.n==0) stillmore--;

	 //check if right
	tr=newr; tc=newc+1;
	if (tc>numvfolds || finfo[tr][tc].pages.n==0) stillmore--;

	if (stillmore==0) {
		 //apply final flip values
		int finalr=newr;
		int finalc=newc;

		int page=0,xflip,yflip;
		for (int c=finfo[finalr][finalc].pages.n-2; c>=0; c-=2) {
			tr=finfo[finalr][finalc].pages.e[c];
			tc=finfo[finalr][finalc].pages.e[c+1];

			xflip=finfo[tr][tc].x_flipped;
			yflip=finfo[tr][tc].y_flipped;
			finfo[tr][tc].finalyflip=finfo[tr][tc].y_flipped;
			finfo[tr][tc].finalxflip=finfo[tr][tc].x_flipped;

			if ((xflip && !yflip) || (!xflip && yflip)) {
				 //back side of paper is up
				finfo[tr][tc].finalindexback=page;
				finfo[tr][tc].finalindexfront=page+1;
			} else {
				finfo[tr][tc].finalindexback=page+1;
				finfo[tr][tc].finalindexfront=page;
			}
			page+=2;
		}
		hasfinal=1;
	} else hasfinal=0;

	if (finalrow) *finalrow= hasfinal ? newr : -1;
	if (finalcol) *finalcol= hasfinal ? newc : -1;
	return hasfinal;
}

//! Set the size of the signature.
/*! Just transfers w and h to patternwidth and patternheight.
 */
int Signature::SetPatternSize(double w,double h)
{
	patternwidth=w;
	patternheight=h;
	return 0;
}

//! Return height of a folding section.
/*! Just returns patternheight.
 */
double Signature::PatternHeight()
{ return patternheight; }

//! Return width of a folding section.
/*! Just returns patternwidth.
 */
double Signature::PatternWidth()
{ return patternwidth; }

//! The height of a single element of a folding section.
/*! For part==0, this is the cell height: PatternHeight()/(numhfolds+1).
 * For part==1, this is like 0, but removing the top and bottom trim.
 */
double Signature::PageHeight(int part)
{
	if (part==0) return PatternHeight()/(numhfolds+1);
	return PatternHeight()/(numhfolds+1) - trimtop - trimbottom;
}

//! The width of a single element of a folding section.
/*! For part==0, this is the cell width: PatternWidth()/(numvfolds+1).
 * For part==1, this is like 0, but removing the left and right trim.
 */
double Signature::PageWidth(int part)
{
	if (part==0) return PatternWidth()/(numvfolds+1);
	return PatternWidth()/(numvfolds+1) - trimleft - trimright;
}

//! Return the bounds for various parts of a final folded page.
/*! If part==0, then return the bounds of a trimmed cell. This has minumums of 0,
 * and maximums of PageWidth(1),PageHeight(1).
 *
 * If part==1, then return the margin area, as it would sit in a region defined by part==0.
 *
 * If part==2, then return the whole page cell, as it would sit around a region defined by part==0.
 *
 * If bbox!=NULL, then set in that. If bbox==NULL, then return a new DoubleBBox.
 */
Laxkit::DoubleBBox *Signature::PageBounds(int part, Laxkit::DoubleBBox *bbox)
{
	if (!bbox) bbox=new Laxkit::DoubleBBox;

	if (part==0) { //trim box
		bbox->minx=bbox->miny=0;
		bbox->maxx=PageWidth(1);
		bbox->maxy=PageHeight(1);

	} else if (part==1) { //margin box
		bbox->minx=marginleft-trimleft;
		bbox->miny=marginbottom-trimbottom;
		bbox->maxx=PageWidth(0)  - marginright - trimleft;
		bbox->maxy=PageHeight(0) - margintop - trimbottom;

	} else { //page cell box
		bbox->minx=-trimleft;
		bbox->miny=-trimbottom;
		bbox->maxx=-trimleft+PageWidth(0);
		bbox->maxy=-trimbottom+PageHeight(0);
	}

	return bbox;
}

//! Return whether the resulting book should be considering folding vertically like a calendar, or horizontally like a book.
/*! If up and binding are both top or bottom, then return 1, else 0.
 */
int Signature::IsVertical()
{
	if  ((up=='t' || up=='b') && (binding=='t' || binding=='b')) return 1;
	if  ((up=='l' || up=='r') && (binding=='r' || binding=='l')) return 1;
	return 0;
}

/*! Say a pattern is 4x3 cells, then 2*(3*4)=24 is returned.
 */
int Signature::PagesPerPattern()
{
	return 2*(numvfolds+1)*(numhfolds+1);
}

/*! Return whether the page is on the front (0) or back (1). If num_sheets>1, then
 * pretend there are that many sheets stacked up for this signature.
 *
 *  If row or col are not NULL, then return which cell the page is in.
 * Note that this row,col is for a paper spread, and by convention the backside of a sheet of
 * paper is flipped left to right in relation to the front side.
 *
 * If pagenumber is greater than PagesPerPattern(), it is modded to be within.
 */
int Signature::locatePaperFromPage(int pagenumber, int *row, int *col, int num_sheets)
{
	int pageindex=pagenumber%PagesPerPattern();//page index within the signature
	int pagespercell=2*num_sheets; //total pages per cell
	int sigindex      =pageindex/(pagespercell/2);//page index assuming a single page in signature
	int sigindexoffset=pageindex%(pagespercell/2);//index within cell of the page

	 //sigindex is the same as pageindex when there is only 1 sheet per signature.
	 //Now we need to find which side of the paper sigindex is on, then map that if necessary
	 //to the right piece of paper.
	 //
	 //To do this, we find where it is in the pattern...

	DBG int front;  //Whether sigindex is on top or bottom of unfolded pattern
	int countdir; //Whether a pattern cell has a higher page number on top (1) or not (0).
	int rr=-1, cc=-1;
	for (rr=0; rr<numhfolds+1; rr++) {
	  for (cc=0; cc<numvfolds+1; cc++) {
		if (sigindex==foldinfo[rr][cc].finalindexfront) {
			DBG front=1;
			countdir=(foldinfo[rr][cc].finalindexfront>foldinfo[rr][cc].finalindexback);
			break;
		} else if (sigindex==foldinfo[rr][cc].finalindexback) {
			DBG front=0;
			countdir=(foldinfo[rr][cc].finalindexfront>foldinfo[rr][cc].finalindexback);
			break;
		}
	  } //cc
	  if (cc!=numvfolds+1) break;
	}  //rr

	DBG if (rr==numhfolds+1) { 
	DBG 	cerr << " *** could not find place "<<sigindex<<" in rr,cc"<<endl;
	DBG 	exit(0);
	DBG }
	DBG cerr <<"front:"<<front<<endl;

	 //now rr,cc is the cell that contains sigindex.
	 //We must figure out how it maps to pieces of paper
	int papernumber;
	if (countdir==1) papernumber=pagespercell-1-sigindexoffset;
	else papernumber=sigindexoffset;

	if (row) *row=rr;
	if (col) *col=((sigindexoffset%1) ? (numvfolds-cc) : cc);

	return PagesPerPattern() + papernumber;
}

//! Ensure that the Signature's values are actually sane.
/*! \todo TODO!
 *
 * Return 0 for totally valid.
 */ 
unsigned int Signature::Validity()
{
	 //inset, gap, trim and margin values all must be within the proper boundaries.
	 //fold indices and directions must make sense, and fold down to a single page

	cerr <<" *** need to implement proper Signature sanity check Signature::Validity()"<<endl;
	if (numvfolds+numhfolds!=folds.n) return 1;
	return 0;
}

void Signature::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%sname \"Some name\"      #short name of the signature\n",spc);
		fprintf(f,"%sdescription \"Huh\"     #a one line description of the signature\n",spc);
		//fprintf(f,"%ssheetspersignature 1  #The number of sheets of paper to stack before\n",spc);
		//fprintf(f,"%s                      #applying inset or folding\n",spc);
		fprintf(f,"%sautoaddsheets no      #If no, then more pages means use more signatures.\n",spc);
		fprintf(f,"%s                      #If yes, then add more sheets, and fold all as a single signature.\n",spc);
		fprintf(f,"\n");
		fprintf(f,"%snumhfolds 0        #The number of horizontal fold lines of a folding pattern\n",spc);
		fprintf(f,"%snumvfolds 0        #The number of vertical fold lines of a folding pattern\n",spc);
		fprintf(f,"%sfold 3 Under Left  #There will be numhfolds+numvfolds fold blocks. When reading in, the number\n",spc);
		fprintf(f,"%sfold 2 Top         #of these blocks will override values of numhfolds and numvfolds.\n",spc);
		fprintf(f,"%s                   #1st number is which horizontal or vertical fold, counting from the left or the top\n",spc);
		fprintf(f,"%s                   #The direction Can be right, left, top, bottom, under right, under left,\n",spc);
		fprintf(f,"%s                   #under top, under bottom. The \"under\" values fold in that\n",spc);
		fprintf(f,"%s                   #direction, but the fold is behind as you look at it,\n",spc);
		fprintf(f,"%s                   #rather than the default of over and on top.\n",spc);
		fprintf(f,"\n");
		fprintf(f,"%sbinding left       #left, right, top, or bottom. The side to expect a document to be bound.\n",spc);
		fprintf(f,"%s                   #Any trim value for the binding edge will be ignored.\n",spc);
		fprintf(f,"%strimtop    0       #How much to trim off the top of a totally folded section\n",spc);
		fprintf(f,"%strimbottom 0       #How much to trim off the bottom of a totally folded section\n",spc);
		fprintf(f,"%strimleft   0       #How much to trim off the left of a totally folded section\n",spc);
		fprintf(f,"%strimright  0       #How much to trim off the right of a totally folded section\n",spc);
		fprintf(f,"%smargintop    0     #How much of a margin to apply to totally folded pages.\n",spc);
		fprintf(f,"%smarginbottom 0     #Inside and outside margins are automatically kept track of.\n",spc);
		fprintf(f,"%smarginleft   0\n",spc);
		fprintf(f,"%smarginright  0\n",spc);
		fprintf(f,"%sup top             #When displaying pages, this direction should be toward the top of the screen\n",spc);
		fprintf(f,"%spositivex right    #(optional) Default is a right handed x axis with the up direction the y axis\n",spc);
		fprintf(f,"%spositivey top      #(optional) Default to the same direction as up\n",spc);
		return;
	}

	if (name) {
		fprintf(f,"%sname ",spc);
		dump_out_escaped(f,name,-1);
		fprintf(f,"\n");
	}

	if (description) {
		fprintf(f,"%sdescription ",spc);
		dump_out_escaped(f,description,-1);
		fprintf(f,"\n");
	}

	//fprintf(f,"%ssheetspersignature %d\n",spc,sheetspersignature);

	fprintf(f,"%snumhfolds %d\n",spc,numhfolds);
	fprintf(f,"%snumvfolds %d\n",spc,numvfolds);
	
	for (int c=0; c<folds.n; c++) {
		fprintf(f,"%sfold %d %s #%d\n",spc,folds.e[c]->whichfold, FoldDirectionName(folds.e[c]->direction,0), c);
		//fprintf(f,"%s  index %d\n",spc,folds.e[c]->whichfold);
		//fprintf(f,"%s  direction %s\n",spc,FoldDirectionName(folds.e[c]->direction,0));
	}

	fprintf(f,"%sbinding %s\n",spc,CtoStr(binding));

	fprintf(f,"%strimtop    %.10g\n",spc,trimtop);
	fprintf(f,"%strimbottom %.10g\n",spc,trimbottom);
	fprintf(f,"%strimleft   %.10g\n",spc,trimleft);
	fprintf(f,"%strimright  %.10g\n",spc,trimright);

	fprintf(f,"%smargintop    %.10g\n",spc,margintop);
	fprintf(f,"%smarginbottom %.10g\n",spc,marginbottom);
	fprintf(f,"%smarginleft   %.10g\n",spc,marginleft);
	fprintf(f,"%smarginright  %.10g\n",spc,marginright);

	fprintf(f,"%sup %s\n",spc,CtoStr(up));
	if (positivex) fprintf(f,"%spositivex %s\n",spc,CtoStr(positivex));
	if (positivey) fprintf(f,"%spositivey %s\n",spc,CtoStr(positivey));

}

Attribute *Signature::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	if (!att) att=new Attribute;
	if (what==-1) {
		
		att->push("name \"Some name\"",   "short name of the signature ");
		att->push("description \"Huh\"",  "a one line description of the signature ");
//		att->push("sheetspersignature 1", "The number of sheets of paper to stack before "
//										  "applying inset or folding ");
	
		att->push("numhfolds 0", "The number of horizontal fold lines of a folding pattern ");
		att->push("numvfolds 0", "The number of vertical fold lines of a folding pattern ");
		att->push("fold 3 Under Left", "There will be numhfolds+numvfolds fold blocks. When reading in, the number ");
		att->push("fold 2 Top", "of these blocks will override values of numhfolds and numvfolds. "
								"1st number is which horizontal or vertical fold, counting from the left or the top "
								"The direction Can be right, left, top, bottom, under right, under left, "
								"under top, under bottom. The \"under\" values fold in that "
								"direction, but the fold is behind as you look at it, "
								"rather than the default of over and on top. ");
		
		att->push("binding left", "left, right, top, or bottom. The side to expect a document to be bound. "
								  "Any trim value for the binding edge will be ignored. ");
		att->push("trimtop    0", "How much to trim off the top of a totally folded section ");
		att->push("trimbottom 0", "How much to trim off the bottom of a totally folded section ");
		att->push("trimleft   0", "How much to trim off the left of a totally folded section ");
		att->push("trimright  0", "How much to trim off the right of a totally folded section ");
		att->push("margintop    0", "How much of a margin to apply to totally folded pages. ");
		att->push("marginbottom 0", "Inside and outside margins are automatically kept track of. ");
		att->push("marginleft   0 ");
		att->push("marginright  0 ");
		att->push("up top", "When displaying pages, this direction should be toward the top of the screen ");
		att->push("positivex right", "(optional) Default is a right handed x axis with the up direction the y axis ");
		att->push("positivey top", "(optional) Default to the same direction as up ");
		return att;
	}

	if (name) att->push("name",name);
	if (description) att->push("description",description);

	att->push("numhfolds",numhfolds);
	att->push("numvfolds",numvfolds);
	
	char scratch[100];
	for (int c=0; c<folds.n; c++) {
		sprintf(scratch,"%d %s #%d\n",folds.e[c]->whichfold, FoldDirectionName(folds.e[c]->direction,0), c);
		att->push("fold",scratch);
	}

	att->push("binding",CtoStr(binding));

	att->push("trimtop",   trimtop);
	att->push("trimbottom",trimbottom);
	att->push("trimleft",  trimleft);
	att->push("trimright", trimright);

	att->push("margintop"   ,margintop);
	att->push("marginbottom",marginbottom);
	att->push("marginleft"  ,marginleft);
	att->push("marginright", marginright);

	att->push("up",CtoStr(up));
	if (positivex) att->push("positivex",CtoStr(positivex));
	if (positivey) att->push("positivey",CtoStr(positivey));

	return att;
}

void Signature::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	char *name,*value;
	int numhf=0, numvf=0;

	for (int c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"name")) {
			makestr(name,value);

		} else if (!strcmp(name,"description")) {
			makestr(description,value);

		} else if (!strcmp(name,"sheetspersignature")) {
			IntAttribute(value,&sheetspersignature);

		} else if (!strcmp(name,"numhfolds")) {
			IntAttribute(value,&numhfolds);

		} else if (!strcmp(name,"numvfolds")) {
			IntAttribute(value,&numvfolds);

		} else if (!strcmp(name,"trimtop")) {
			DoubleAttribute(value,&trimtop);

		} else if (!strcmp(name,"trimbottom")) {
			DoubleAttribute(value,&trimbottom);

		} else if (!strcmp(name,"trimleft")) {
			DoubleAttribute(value,&trimleft);

		} else if (!strcmp(name,"trimright")) {
			DoubleAttribute(value,&trimright);

		} else if (!strcmp(name,"margintop")) {
			DoubleAttribute(value,&margintop);

		} else if (!strcmp(name,"marginbottom")) {
			DoubleAttribute(value,&marginbottom);

		} else if (!strcmp(name,"marginleft")) {
			DoubleAttribute(value,&marginleft);

		} else if (!strcmp(name,"marginright")) {
			DoubleAttribute(value,&marginright);

		} else if (!strcmp(name,"binding")) {
			LRTBAttribute(value,&binding);

		} else if (!strcmp(name,"up")) {
			LRTBAttribute(value,&up);

		} else if (!strcmp(name,"positivex")) {
			LRTBAttribute(value,&positivex);

		} else if (!strcmp(name,"positivey")) {
			LRTBAttribute(value,&positivey);

		} else if (!strcmp(name,"fold")) {
			 //fold 3 under right
			 //fold 4 top
			char *e=NULL;
			int index=-1;
			int under=0;
			char dir=0;
			IntAttribute(value,&index,&e);
			while (*e && isspace(*e)) e++;
			if (!strncasecmp(e,"under ",6)) { under=1; e+=6; }
			LRTBAttribute(e,&dir);
			if (dir==0) {
				cerr <<" *** WARNING! corrupt fold designation in file"<<endl;
			}
			if (under) {
				if      (dir=='r') { numvf++; }
				else if (dir=='l') { numvf++; }
				else if (dir=='b') { numhf++; }
				else if (dir=='t') { numhf++; }
			} else {
				if      (dir=='l') { numvf++; }
				else if (dir=='b') { numhf++; }
				else if (dir=='t') { numhf++; }
				else if (dir=='r') { numvf++; }
			}
			Fold *newfold=new Fold(dir,under,index);
			folds.push(newfold);

		}
	}

	reallocateFoldinfo();
	applyFold(NULL,-1);
	checkFoldLevel(NULL,NULL,NULL);
}

ObjectDef *Signature::makeObjectDef()
{
	ObjectDef *sd=stylemanager.FindDef("Signature");
	if (sd) { sd->inc_count(); return sd; }

	sd=new ObjectDef(NULL,"Signature",
			_("Signature"),
			_("Folds of paper in a SignatureImposition"),
			"class",
			NULL,NULL, //range, default value
			NULL, //fields
			0, //new flags
			NULL, //newfunc
			NULL /*createSignature*/);

	sd->push("name", _("Name"), _("Name of the imposition"),
			"string",
			NULL, //range
			"0",  //defvalue
			0,    //flags
			NULL);//newfunc

	sd->push("description", _("Description"), _("Brief, one line description of the imposition"),
			"string",
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc


	sd->push("numhfolds", _("Horizontal Folds"), _("The number of horizontal fold lines of a folding pattern"),
			"int", "[0..", "0", 0, NULL);

	sd->push("numvfolds", _("Vertical Folds"), _("The number of vertical fold lines of a folding pattern"),
			"int", "[0..", "0", 0, NULL);

	 //------make Fold ObjectDef if necessary
	if (stylemanager.FindDef("Signature")==NULL) {
		Fold fold(0,0,0);
		fold.GetObjectDef(); //forces creation and global install of fold definition
	}

	sd->push("folds", _("Folds"), _("Set of the folds making the signature"),
			"set", "Fold", NULL, 0, NULL);



	sd->pushEnum("binding", _("Binding"), _("left, right, top, or bottom. The side to expect a document to be bound."),
				 "Left", NULL, NULL, //defvalue, newfunc, newstylefunc
				 "Left",_("Left"),_("Left"),
				 "Right",_("Right"),_("Right"),
				 "Top",_("Top"),_("Top"),
				 "Bottom",_("Bottom"),_("Bottom"),
				 NULL
				 );

	sd->push("trimtop", _("Top Trim"), _("How much to trim off the top of a totally folded section"),
			"real", "[0..", "0", 0, NULL);

	sd->push("trimbottom", _("Bottom Trim"), _("How much to trim off the bottom of a totally folded section"),
			"real", "[0..", "0", 0, NULL);

	sd->push("trimleft", _("Left Trim"), _("How much to trim off the left of a totally folded section"),
			"real", "[0..", "0", 0, NULL);

	sd->push("trimright", _("Right Trim"), _("How much to trim off the right of a totally folded section"),
			"real", "[0..", "0", 0, NULL);

	sd->push("margintop", _("Top Margin"), _("Default top margin on a totally folded section"),
			"real", "[0..", "0", 0, NULL);

	sd->push("marginbottom", _("Bottom Margin"), _("Default bottom margin on a totally folded section"),
			"real", "[0..", "0", 0, NULL);

	sd->push("marginleft", _("Left Margin"), _("Default left margin on a totally folded section"),
			"real", "[0..", "0", 0, NULL);

	sd->push("marginright", _("Right Margin"), _("Default right margin on a totally folded section"),
			"real", "[0..", "0", 0, NULL);


	stylemanager.AddObjectDef(sd,0);
	return sd;
}

//! Constructor for Signature objects in scripting.
/*! This does not throw an error for having an incomplete set of parameters.
 * It just fills what's given.
 */
int createSignature(ValueHash *context, ValueHash *parameters,
					   Value **value_ret, ErrorLog &log)
{
	if (!parameters || !parameters->n()) {
		if (value_ret) *value_ret=new Signature;
		//log.AddMessage(_("Missing parameters!"),ERROR_Fail);
		return 0;
	}

	Signature *sig=new Signature;
	Value *v=NULL;

	char error[100];
	int err=0;
	try {
		int i, e;
		double d;

		 //---marginleft
		d=parameters->findDouble("marginleft",-1,&e);
		if (e==0) sig->marginleft=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"marginleft"); throw error; }

		 //---marginright
		d=parameters->findDouble("marginright",-1,&e);
		if (e==0) sig->marginright=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"marginright"); throw error; }

		 //---margintop
		d=parameters->findDouble("margintop",-1,&e);
		if (e==0) sig->margintop=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"margintop"); throw error; }

		 //---marginbottom
		d=parameters->findDouble("marginbottom",-1,&e);
		if (e==0) sig->marginbottom=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"marginbottom"); throw error; }

		 //---trimleft
		d=parameters->findDouble("trimleft",-1,&e);
		if (e==0) sig->trimleft=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"trimleft"); throw error; }

		 //---trimright
		d=parameters->findDouble("trimright",-1,&e);
		if (e==0) sig->trimright=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"trimright"); throw error; }

		 //---trimtop
		d=parameters->findDouble("trimtop",-1,&e);
		if (e==0) sig->trimtop=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"trimtop"); throw error; }

		 //---trimbottom
		d=parameters->findDouble("trimbottom",-1,&e);
		if (e==0) sig->trimbottom=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"trimbottom"); throw error; }

		 //---numhfolds
		i=parameters->findInt("numhfolds",-1,&e);
		if (e==0) sig->numhfolds=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"numhfolds"); throw error; }

		 //---numvfolds
		i=parameters->findInt("numvfolds",-1,&e);
		if (e==0) sig->numvfolds=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"numvfolds"); throw error; }

		 //---folds
		v=parameters->find("folds");
		if (v) {
			if (v->type()!=VALUE_Set) { sprintf(error, _("Invalid format for %s!"),"folds"); throw error; }
			SetValue *set=dynamic_cast<SetValue*>(v);
			ObjectValue *o;
			Fold *fold;
			for (int c=0; c<set->values.n; c++) {
				o=dynamic_cast<ObjectValue*>(set->values.e[c]);
				if (!o) { sprintf(error, _("Invalid format for %s!"),"folds"); set->dec_count(); throw error; }
				fold=dynamic_cast<Fold*>(o->object);
				if (!fold) { sprintf(error, _("Expecting %s!"),"Fold"); set->dec_count(); throw error; }

				sig->folds.push(new Fold(fold->direction,fold->under,fold->whichfold));
			}
		}

		sig->reallocateFoldinfo();
		sig->applyFold(NULL,-1);
		sig->checkFoldLevel(NULL,NULL,NULL);


	} catch (const char *str) {
		log.AddMessage(str,ERROR_Fail);
		err=1;
	}

	if (value_ret && err==0) {
		if (sig->Validity()==0) { *value_ret=sig; sig->inc_count(); }
		else {
			log.AddMessage(_("Signature has invalid configuration!"),ERROR_Fail);
			err=1;
			*value_ret=NULL;
		}
	}
	sig->dec_count();

	return err;
}


//------------------------------------ PaperPartition -----------------------------------------
/*! \class PaperPartition
 * Description of how to cut a piece of paper into equal sections.
 */
/*! \var double PaperPartition::insetleft
 * \brief An initial left margin to chop off a paper before sectioning and folding.
 */
/*! \var double PaperPartition::insetright
 * \brief An initial right margin to chop off a paper before sectioning and folding.
 */
/*! \var double PaperPartition::insettop
 * \brief An initial top margin to chop off a paper before sectioning and folding.
 */
/*! \var double PaperPartition::insetbottom
 * \brief An initial bottom margin to chop off a paper before sectioning and folding.
 */
/*! \var int PaperPartition::tilex
 * \brief After cutting off an inset, the number of horizontal sections to divide a paper.
 */
/*! \var int PaperPartition::tiley
 * \brief After cutting off an inset, the number of vertical sections to divide a paper.
 */
/*! \var int PaperPartition::work_and_turn
 * Work and turn turns around a vertical axis at the exact center of the paper, front is
 * on the left. SIGT_Work_and_Turn_BF has back on the left.
 * Work and tumble turns around a horizontal axis at center, front is on the top.
 * SIGT_Work_and_Tumble_BF has back on top.
 */


PaperPartition::PaperPartition()
{
	paper=new PaperStyle;

	totalwidth=paper->w();
	totalheight=paper->h();

	insetleft=insetright=insettop=insetbottom=0;

	tilex=tiley=1;
	tilegapx=tilegapy=0;

	work_and_turn=SIGT_None;
}

PaperPartition::~PaperPartition()
{
	paper->dec_count();
}

Value *PaperPartition::duplicate()
{
	PaperPartition *p=new PaperPartition;
	p->SetPaper(paper);
	//p->totalwidth =totalwidth;
	//p->totalheight=totalheight;
	p->insetleft  =insetleft;
	p->insetright =insetright;
	p->insettop   =insettop;
	p->insetbottom=insetbottom;

	p->tilex=tilex;
	p->tiley=tiley;
	p->tilegapx=tilegapx;
	p->tilegapy=tilegapy;

	p->work_and_turn=work_and_turn;

	return p;
}

//! Return height of a folding section.
/*! This is the total height minus insettop and bottom, minus gaps, divided by the number of vertical tiles.
 */
double PaperPartition::PatternHeight()
{
	double h=totalheight;
	if (paper) h=paper->h();
	if (tiley>1) h-=(tiley-1)*tilegapy;
	h-=insettop+insetbottom;
	return h/tiley;
}

//! Return width of a folding section.
/*! This is the total width minus insetleft and right, minus gaps, divided by the number of horizontal tiles.
 */
double PaperPartition::PatternWidth()
{
	double w=totalwidth;
	if (paper) w=paper->w();
	if (tilex>1) w-=(tilex-1)*tilegapx;
	w-=insetleft+insetright;
	return w/tilex;
}

void PaperPartition::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
    Attribute att;
	dump_out_atts(&att,0,context);
    att.dump_out(f,indent);
}

LaxFiles::Attribute *PaperPartition::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
    if (!att) att=new Attribute("PaperPartition",NULL);
	
	if (what==-1) {
		Value::dump_out_atts(att,-1,context);
		return att;
	}


    char scratch[100];


	//att->push("name",name);
	//att->push("description",description);

	Attribute *att2=NULL;
	if (paper) {
		att2=paper->dump_out_atts(NULL,what,context);
		makestr(att2->name,"paper");
	}
	if (paper) att->push(att2, -1); 

	if (work_and_turn!=SIGT_None) {
		if (work_and_turn==SIGT_Work_and_Turn)           att->push("work_and_turn", "turn", -1); 
		else if (work_and_turn==SIGT_Work_and_Turn_BF)   att->push("work_and_turn", "turnBF", -1); 
		else if (work_and_turn==SIGT_Work_and_Tumble)    att->push("work_and_turn", "tumble", -1); 
		else if (work_and_turn==SIGT_Work_and_Tumble_BF) att->push("work_and_turn", "tumbleBF", -1); 
	}


    sprintf(scratch,"%.10g",insettop);
	att->push("insettop",insettop,-1);

    sprintf(scratch,"%.10g",insetbottom);
	att->push("insetbottom",insetbottom,-1);

    sprintf(scratch,"%.10g",insetleft);
	att->push("insetleft",insetleft,-1);

    sprintf(scratch,"%.10g",insetright);
	att->push("insetright",insetright,-1);


    sprintf(scratch,"%.10g",tilegapx);
	att->push("tilegapx",tilegapx,-1);

    sprintf(scratch,"%.10g",tilegapy);
	att->push("tilegapy",tilegapy,-1);


    sprintf(scratch,"%d",tilex);
	att->push("tilex",tilex,-1);

    sprintf(scratch,"%d",tiley);
	att->push("tiley",tiley,-1);


	return att;
}

void PaperPartition::dump_in_atts(Attribute *att,int what,LaxFiles::DumpContext *context)
{
	char *name,*value;

	for (int c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		//if (!strcmp(name,"name")) {
		//	makestr(name,value);
		//
		//} else if (!strcmp(name,"description")) {
		//	makestr(description,value);


		if (!strcmp(name,"insettop")) {
			DoubleAttribute(value,&insettop);

		} else if (!strcmp(name,"insetbottom")) {
			DoubleAttribute(value,&insetbottom);

		} else if (!strcmp(name,"insetleft")) {
			DoubleAttribute(value,&insetleft);

		} else if (!strcmp(name,"insetright")) {
			DoubleAttribute(value,&insetright);

		} else if (!strcmp(name,"tilegapx")) {
			DoubleAttribute(value,&tilegapx);

		} else if (!strcmp(name,"tilegapy")) {
			DoubleAttribute(value,&tilegapy);

		} else if (!strcmp(name,"tilex")) {
			IntAttribute(value,&tilex);

		} else if (!strcmp(name,"tiley")) {
			IntAttribute(value,&tiley);

		} else if (!strcmp(name,"paper")) {
			if (!isblank(value)) {
			 	 //handle "paper Letter, landscape", assume no subattributes
				cout << " *** must implement search for existing paper for PaperPartition::dump_in"<<endl;
			} else {
				PaperStyle *pp=new PaperStyle;
				pp->dump_in_atts(att->attributes.e[c], what,context);
				if (paper) paper->dec_count();
				paper=pp;
			}

		} else if (!strcmp(name,"work_and_turn")) {
			if      (!strcasecmp(value,"turn"    )) work_and_turn=SIGT_Work_and_Turn     ;
			else if (!strcasecmp(value,"turnBF"  )) work_and_turn=SIGT_Work_and_Turn_BF  ;
			else if (!strcasecmp(value,"tumble"  )) work_and_turn=SIGT_Work_and_Tumble   ;
			else if (!strcasecmp(value,"tumbleBF")) work_and_turn=SIGT_Work_and_Tumble_BF;

		}
	}

	totalwidth=paper->w();
	totalheight=paper->h();
}

//! Installs duplicate.
int PaperPartition::SetPaper(PaperStyle *p)
{
	if (p!=paper) {
		if (paper) paper->dec_count();
		paper=dynamic_cast<PaperStyle*>(p->duplicate());
	}
	totalwidth=paper->w();
	totalheight=paper->h();
	return 0;
}

/*! Create a new PaperPartition ObjectDef.
 * Returns an inc counted reference to it if found in stylemanager.
 */
ObjectDef *PaperPartition::makeObjectDef()
{
	ObjectDef *def=stylemanager.FindDef("PaperPartition");
	if (def) { def->inc_count(); return def; }

	def=new ObjectDef(NULL,"PaperPartition",
			_("PaperPartition"),
			_("How to partition papers for Signatures"),
			"class",
			NULL,NULL, //range, default value
			NULL, //fields
			0, //new flags
			NULL, //newfunc
			NULL /*createPaperPartition*/); //newfunc with parameters

	def->push("paper", _("Paper"), _("Which paper size to use"),
			"Paper", NULL, NULL, 0, NULL);

	def->push("insettop", _("Top Inset"), _("Space at the top of a paper before tiling for signatures"),
			"real", "[0..", "0", 0, NULL);

	def->push("insetbottom", _("Bottom Inset"), _("Space at the bottom of a paper before tiling for signatures"),
			"real", "[0..", "0", 0, NULL);

	def->push("insetleft", _("Left Inset"), _("Space at the left of a paper before tiling for signatures"),
			"real", "[0..", "0", 0, NULL);

	def->push("insetright", _("Right Inset"), _("Space at the right of a paper before tiling for signatures"),
			"real", "[0..", "0", 0, NULL);

	def->push("tilex", _("Horizontal Tiles"), _("The number of folding sections horizontally to divide a piece of paper"),
			"int", "[1..", "0", 0, NULL);

	def->push("tiley", _("Vertical Tiles"), _("The number of folding sections vertically to divide a piece of paper"),
			"int", "[1..", "0", 0, NULL);

	def->push("tilegapx", _("H Tile Gap"), _("How much space to put between folding areas horizontally"),
			"real", "[0..", "0", 0, NULL);

	def->push("tilegapy", _("V Tile Gap"), _("How much space to put between folding areas vertically"),
			"real", "[0..", "0", 0, NULL);

	def->pushEnum("work_and_turn",_("Work and turn"), _("Work and turn"), NULL,NULL,NULL,
					"turn",_("Turn"),_("Flip paper along vertical axis"),
					"turnBF",_("Turn BF"),_("Flip paper along vertical axis with front on the right"),
					"tumble",_("Tumble"),_("Flip paper along horizontal axis"),
					"tumbleBF",_("Tumble BF"),_("Flip paper along horizontal axis with front on the bottom"),
					NULL);


	stylemanager.AddObjectDef(def,0);
	return def;
}


/*! Creation function for use in scripting.
 */
int createPaperPartition(ValueHash *context, ValueHash *parameters,
					   Value **value_ret, ErrorLog &log)
{
	if (!parameters || !parameters->n()) {
		if (value_ret) *value_ret=new PaperPartition;
		return 0;
	}

	PaperPartition *partition=new PaperPartition;

	char error[100];
	int err=0;
	try {
		int i, e;
		double d;

		 //---tilegapx
		d=parameters->findDouble("tilegapx",-1,&e);
		if (e==0) partition->tilegapx=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"tilegapx"); throw error; }

		 //---tilegapy
		d=parameters->findDouble("tilegapy",-1,&e);
		if (e==0) partition->tilegapy=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"tilegapy"); throw error; }

		 //---tilex
		i=parameters->findInt("tilex",-1,&e);
		if (e==0) partition->tilex=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"tilex"); throw error; }

		 //---tiley
		i=parameters->findInt("tiley",-1,&e);
		if (e==0) partition->tiley=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"tiley"); throw error; }

		 //---insetleft
		d=parameters->findDouble("insetleft",-1,&e);
		if (e==0) partition->insetleft=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"insetleft"); throw error; }

		 //---insetright
		d=parameters->findDouble("insetright",-1,&e);
		if (e==0) partition->insetright=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"insetright"); throw error; }

		 //---insettop
		d=parameters->findDouble("insettop",-1,&e);
		if (e==0) partition->insettop=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"insettop"); throw error; }

		 //---insetbottom
		d=parameters->findDouble("insetbottom",-1,&e);
		if (e==0) partition->insetbottom=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"insetbottom"); throw error; }

	} catch (const char *str) {
		log.AddMessage(str,ERROR_Fail);
		err=1;
	}

	if (value_ret && err==0) {
		if (1) {
			*value_ret=partition;
			partition->inc_count();

		} else {
			log.AddMessage(_("Incomplete PaperPartition definition!"),ERROR_Fail);
			err=1;
		}
	}
	partition->dec_count();

	return err;
}


//------------------------------------ SignatureInstance -----------------------------------------
/* \class SignatureInstance
 * Hold a Signature, plus how many sheets in the pattern, and a PaperPartition.
 */

SignatureInstance::SignatureInstance(Signature *sig, PaperPartition *paper)
{
	pagestyle=pagestyleodd=NULL;

	if (sig) sig->inc_count();
	if (!sig) sig=new Signature;
	pattern=sig;

	partition=paper;
	if (paper) paper->inc_count();
	else partition=new PaperPartition;

	pattern->patternheight=partition->PatternHeight();
	pattern->patternwidth =partition->PatternWidth();

	sheetspersignature=1;
	autoaddsheets=1;
	creep=0;

	next_insert=NULL;
	prev_insert=NULL;
	next_stack =NULL;
	prev_stack =NULL;

	//automarks=AUTOMARK_Margins|AUTOMARK_InnerDot; //1.margin marks, 2.interior dotted line, 4.interior dots
	//automarks=AUTOMARK_Margins|AUTOMARK_InnerDottedLines; //1.margin marks, 2.interior dotted line, 4.interior dots
	automarks=0; //1.margin marks, 2.interior dotted line, 4.interior dots
	linestyle=NULL;
}

/*! will delete next_insert and next_stack.
 */
SignatureInstance::~SignatureInstance()
{
	if (pagestyle) pagestyle->dec_count();
	if (pagestyleodd) pagestyleodd->dec_count();

	if (next_insert) delete next_insert;
	if (next_stack)  delete next_stack;
	if (partition) partition->dec_count();
	if (pattern) pattern->dec_count();

	if (linestyle) linestyle->dec_count();
}

/*! Add insert as the innermost insert of *this.
 * Takes possession of insert. Calling code shouldn't delete it.
 * Return index number within stack of insert.
 */
int SignatureInstance::AddInsert(SignatureInstance *insert)
{
	SignatureInstance *s=this;
	int i=1;
	while (s->next_insert) { i++; s=s->next_insert; }
	s->next_insert=insert;
	return i;
}

/*! Add insert as the innermost insert of *this.
 * Takes possession of stack. Calling code shouldn't delete it.
 * Return index number of stack.
 */
int SignatureInstance::AddStack(SignatureInstance *stack)
{
	SignatureInstance *s=this;
	int i=1;
	while (s->next_stack) { i++; s=s->next_stack; }
	s->next_stack=stack;
	return i;
}



//! With the final trimmed page size (w,h), set the paper size to the proper size to just contain it, maintaining current inset sizes.
int SignatureInstance::SetPaperFromFinalSize(double w,double h, int all)
{
	 //find cell dims
	w+=pattern->trimleft+pattern->trimright;
	h+=pattern->trimtop +pattern->trimbottom;

	 //find pattern dims
	w*=(pattern->numvfolds+1);
	h*=(pattern->numhfolds+1);

	 //find whole dims
	w=w*partition->tilex + (partition->tilex-1)*partition->tilegapx + partition->insetleft + partition->insetright;
	h=h*partition->tiley + (partition->tiley-1)*partition->tilegapy + partition->insettop  + partition->insetbottom;

	PaperStyle p("Custom",w,h,0,300,"in");
	if (partition->work_and_turn==SIGT_Work_and_Turn || partition->work_and_turn==SIGT_Work_and_Turn_BF)
		w*=2;
	else if (partition->work_and_turn==SIGT_Work_and_Tumble || partition->work_and_turn==SIGT_Work_and_Tumble_BF)
		h*=2;

	if (all) {
		if (next_stack)  next_stack ->SetPaperFromFinalSize(w,h,all);
		if (next_insert) next_insert->SetPaperFromFinalSize(w,h,all);
	}

	return partition->SetPaper(&p);
}

/*! Duplicate *this, AND creates duplicates of inserts and next_stack.
 */
Value *SignatureInstance::duplicate()
{
	SignatureInstance *sig=new SignatureInstance;

	if (partition) sig->partition=(PaperPartition*)partition->duplicate();
	if (pattern) { 	sig->UseThisSignature(pattern,0); }

	if (next_insert) {
		sig->next_insert=(SignatureInstance*)next_insert->duplicate();
		sig->next_insert->prev_insert=sig;
	}
	if (next_stack) {
		sig->next_stack =(SignatureInstance*)next_stack->duplicate();
		sig->next_stack->prev_stack=sig;
	}

	sig->pattern->patternheight=partition->PatternHeight();
	sig->pattern->patternwidth =partition->PatternWidth();

	sig->sheetspersignature=sheetspersignature;
	sig->autoaddsheets=autoaddsheets;
	sig->creep=creep;

	sig->automarks=automarks;

	return sig;
}

/*! Duplicate *this, and DOES NOT create duplicates of inserts and next_stack.
 */
SignatureInstance *SignatureInstance::duplicateSingle()
{
	SignatureInstance *sig=new SignatureInstance;

	if (partition) sig->partition=(PaperPartition*)partition->duplicate();
	if (pattern) { 	sig->UseThisSignature(pattern,0); }

	sig->pattern->patternheight=partition->PatternHeight();
	sig->pattern->patternwidth =partition->PatternWidth();

	sig->sheetspersignature=sheetspersignature;
	sig->autoaddsheets=autoaddsheets;
	sig->creep=creep;

	sig->automarks=automarks;

	return sig;
}

/*! If link!=0, then inc_count on it when installing, otherwise install a duplicate.
 * Return 0 for success.
 */
int SignatureInstance::UseThisSignature(Signature *sig, int link)
{
	if (pattern) pattern->dec_count();
	if (link) {
		if (sig!=pattern) {
			pattern=sig;
			pattern->inc_count();
		}
	} else {
		pattern=(Signature*)sig->duplicate();
	}
	return 0;
}


/*! Return the number of pages that can be arranged in one signature stack.
 * Also add the PagesPerSignature of any inserts.
 *
 * So say you have a pattern with 2 sheets folded together. Then 2*pattern->PagesPerPattern() is returned.
 *
 * If whichstack<0, then return the total number of pages within all stacks. Otherwise, just that stack and its inserts.
 *
 * It is assumed that sheetspersignature reflects a desired number of sheets in the signature,
 * whether or not autoaddsheets==1.
 *
 * Please note this is only the number for ONE tile.
 */
int SignatureInstance::PagesPerSignature(int whichstack, int ignore_inserts)
{
	int num=0;
	SignatureInstance *sig=this;
	while (sig) {
		if (whichstack<=0) {
			num+=sig->sheetspersignature*sig->pattern->PagesPerPattern();
			if (!ignore_inserts && sig->next_insert) num+=sig->next_insert->PagesPerSignature(0,0);
		}
		if (whichstack==0) break; //we were looking for specific stack.
		whichstack--;
		sig=sig->next_stack;
	}

	return num;
}

/*! Like PagesPerSignature(), but return the 2*(number of sheets of paper) required.
 */
int SignatureInstance::PaperSpreadsPerSignature(int whichstack, int ignore_inserts)
{
	int num=0;
	SignatureInstance *sig=this;
	while (sig) {
		if (whichstack<=0) {
			num+=sig->sheetspersignature*2;
			if (!ignore_inserts && sig->next_insert) num+=sig->next_insert->PaperSpreadsPerSignature(0,0);
		}
		if (whichstack==0) break; //we were looking for specific stack.
		whichstack--;
		sig=sig->next_stack;
	}

	return num;
}

/*! Locate which SignatureInstance insert the given page number is contained in.
 * Note this ONLY looks in inserts, not adjacent stacks.
 */
SignatureInstance *SignatureInstance::locateInsert(int pagenumber, //!< a page number for somewhere in this stack+inserts
												   int *insertnum, //!< What would be the page number if found insert was isolated (includes subinserts)
												   int *deep)      //!< Insert number, this==0.
{
	//   \/    Insert 2, 4 pages, 1 sheet
	//  \\//   Insert 1, 8 pages, 2 sheets
	// \\\///  Insert 0, 12 pages, 3 sheets
	//

	int pages_per_sig=sheetspersignature*pattern->PagesPerPattern();

	 //check if in this->signature, but just on one side
	if (pagenumber<pages_per_sig/2) {
		if (deep) *deep=0;
		if (insertnum) *insertnum=pagenumber;
		return this;
	}

	 //check inserts
	if (next_insert) {
		SignatureInstance *found=next_insert->locateInsert(pagenumber-pages_per_sig/2, insertnum, deep);
		if (found) {
			if (deep) (*deep)++;
			return found;
		}
		pagenumber-=next_insert->PagesPerSignature(0,0);
	}

	//check if in this->signature, but on other side from above

	if (pagenumber>=pages_per_sig) {
		 //pagenumber did not fit in this stack
		if (deep) *deep=-1;
		if (insertnum) *insertnum=-1;
		return NULL;
	}

	if (deep) *deep=0;
	if (insertnum) *insertnum=pagenumber;
	return this;
}

//! Return which paper spread contains the given document page number (each sheet of paper is two paper spreads).
/*! If row or col are not NULL, then return which cell the page is in, and which stack and insert thereof.
 * Note that this row,col is for a paper spread, and by convention the backside of a sheet of
 * paper is flipped left to right in relation to the front side.
 */
int SignatureInstance::locatePaperFromPage(int pagenumber,
										   int *stack,      //!< return index of stack, 0 for this
										   int *insert,     //!< return index of insert, 0 for this
										   int *insertpage, //!< return page number on the insert if insert were isolated
										   int *row,        //!< return row and column of page in pattern, seen from front side
										   int *col,
										   SignatureInstance **ssig)
{
	int total_pages_all_stacks=PagesPerSignature(-1,0); //total number in signatures configuration
	int sigsoffset=pagenumber/total_pages_all_stacks; //number of complete configs to pass over
	
	pagenumber%=total_pages_all_stacks;
	int num=PagesPerSignature(0,0);
	if (pagenumber>=num) {
		 //page was in future adjacent stack
		int pp=next_stack->locatePaperFromPage(pagenumber-num, stack, insert, insertpage, row, col, ssig);
		pp+=2*sheetspersignature + sigsoffset*PaperSpreadsPerSignature(-1,0);
		if (stack) (*stack)++; //add one for *this
		return pp;
	}

	//pagenumber numerically is now somewhere in this stack, or its inserts

	if (stack) *stack=0;

	int deep=0;
	int ii=-1;
	SignatureInstance *whichinsert=locateInsert(pagenumber, &ii, &deep);
	int paper=whichinsert->pattern->locatePaperFromPage(ii, row, col, whichinsert->sheetspersignature);

	if (insertpage) *insertpage=ii;
	if (insert) *insert=deep;

	SignatureInstance *sig=whichinsert;
	while (sig && deep) {
		paper+=sig->sheetspersignature*2;
		deep--;
		if (deep) sig=sig->next_insert;
	}

	if (ssig) *ssig=sig;
	return paper;
}

SignatureInstance *SignatureInstance::InstanceFromPage(int pagenumber,
									   int *stack,      //!< return index of stack, 0 for this
									   int *insert,     //!< return index of insert, 0 for this
									   int *insertpage, //!< return page number on the insert if insert were isolated
									   int *row,        //!< return row and column of page in pattern, seen from front side
									   int *col,
									   int *paper)
{
	SignatureInstance *sig=NULL;
	int ppaper=locatePaperFromPage(pagenumber,stack,insert,insertpage,row,col, &sig);
	if (paper) *paper=ppaper;
	return sig;
}

//! Return pointer to the specified SignatureInstance.
/*! If stack<0, then return last one. If insert<0, return innermost for stack.
 */
SignatureInstance *SignatureInstance::GetSignature(int stack,int insert)
{
	if (stack<0) stack=NumStacks(-1)-1;
	if (insert<0) insert=NumStacks(stack)-1;

	SignatureInstance *sig=this;

	while (sig->next_stack  && stack>0)  { sig=sig->next_stack;  stack--;  }
	while (sig->next_insert && insert>0) { sig=sig->next_insert; insert--; }

	return sig;
}

//! Return SignatureInstance that holds paper spread whichpaper.
/*! There are two paper spreads per physical piece of paper.
 */
SignatureInstance *SignatureInstance::InstanceFromPaper(int whichpaper,
												int *stack_ret, //!< Index of returned stack within stack group
												int *insert_ret, //!< Index of returned insert within stack
												int *sigpaper_ret, //!< index of paper spread within returned instance
												int *pageoffset, //!< Page number offset to first page in instance
												int *inneroffset, //!< Page number offset of pages past middle
												int *groups) //!< Return how many complete stack groups precede containing group
{
	if (whichpaper<0) whichpaper=0;

	//find which SignatureInstance contains whichpaper. This assumes all sheetspersignature have correct values
	int totalpapers=PaperSpreadsPerSignature(-1,0);
	int sigpaper=whichpaper;

	int stackpageoffset=0; //first page in signature stacks is this number
	int totalpages=PagesPerSignature(-1,0);
	if (groups) *groups=0;
	while (sigpaper>=totalpapers) {
		stackpageoffset+=totalpages;
		sigpaper-=totalpapers;
		if (groups) (*groups)++;
	}

	 //find stack containing our spread
	SignatureInstance *stack=this;
	int n;
	if (stack_ret) *stack_ret=0;
	while (stack) {
		n=stack->PaperSpreadsPerSignature(0,0);
		if (sigpaper<n) break; //found!

		sigpaper-=n;
		stackpageoffset+=stack->PagesPerSignature(0,0);
		stack=stack->next_stack;
		if (stack_ret) (*stack_ret)++;
	}

	 //find insert containing the spread
	SignatureInstance *insert=stack;
	int insert_offset=0;
	//int total_pages_in_stack=stack->PagesPerSignature(0,0);
	if (insert_ret) *insert_ret=0;
	while (insert) {
		if (sigpaper<2*insert->sheetspersignature) break; //found!
		insert_offset+=insert->PagesPerSignature(0,1)/2;
		sigpaper-=2*insert->sheetspersignature;
		insert=insert->next_insert;
		if (insert_ret) (*insert_ret)++;
	}

	if (sigpaper_ret) *sigpaper_ret=sigpaper;

	int opposite_offset=0; //offset for pages on opposite right side (of left to right book, for instance)
	if (insert->next_insert) opposite_offset=insert->next_insert->PagesPerSignature(0,0);

	//Now, we should have non-null stack and insert.
	if (pageoffset)  *pageoffset =stackpageoffset+insert_offset;
	if (inneroffset) *inneroffset=stackpageoffset+insert_offset+opposite_offset;
	
	return insert;
}

int SignatureInstance::IsVertical()
{
	return pattern->IsVertical();
}

/*! Replace paper size. If all_with_same_size, replace any next connected instances that have the 
 * same paper dimensions, AND the same final page sizes.
 *
 * Returns the number of signature instances changed
 */
int SignatureInstance::SetPaper(PaperStyle *p, int all_with_same_size)
{
	double paper_w=partition->paper->w();
	double paper_h=partition->paper->h();
	double page_w =pattern->PageWidth(1);
	double page_h =pattern->PageHeight(1);

	int n=1;

	partition->SetPaper(p);
	pattern->patternwidth =partition->PatternWidth();
	pattern->patternheight=partition->PatternHeight();
	if (!all_with_same_size) return n;

	SignatureInstance *sig=next_stack;
	while (sig) {
		if (sig->partition->paper->w()==paper_w && sig->partition->paper->h()==paper_h
				&& sig->pattern->PageWidth(1)==page_w && sig->pattern->PageHeight(1)==page_h)
			n+=sig->SetPaper(p,1);
		sig=sig->next_stack;
	}
	sig=next_insert;
	while (sig) {
		if (sig->partition->paper->w()==paper_w && sig->partition->paper->h()==paper_h
				&& sig->pattern->PageWidth(1)==page_w && sig->pattern->PageHeight(1)==page_h)
			n+=sig->SetPaper(p,0);
		sig=sig->next_insert;
	}
	return n;
}

/*! Return the number of vertical stacks. Any inserts are considered part of one stack.
 * Note this counts in the next_stack direction. If next_stack==NULL, 1 is returned.
 * Any prev_stack objects are ignored.
 */
int SignatureInstance::NumStacks()
{
	SignatureInstance *sig=this;
	int n=0;
	while (sig) {
		n++;
		sig=sig->next_stack;
	}
	return n;
}

/*! If which<0, then return the number of defined stacks not including inserts.
 * If which>=0, then return the number of inserts on the designated stack (including the head).
 *
 * Assumes *this in the 0'th stack.
 */
int SignatureInstance::NumStacks(int which)
{
	int num=0;
	SignatureInstance *sig=this;
	while (sig) {
		if (which>=0 && which==num) {
			 //return inserts size on stack which
			num=0;
			while (sig) {
				num++;
				sig=sig->next_insert;
			}
			return num;
		}
		num++;
		sig=sig->next_stack;
	}

	return num;
}

/*! Return the number of inserts of this. If only *this exists, then 0 is returned.
 */
int SignatureInstance::NumInserts()
{
	SignatureInstance *sig=next_insert;
	int n=0;
	while (sig) {
		n++;
		sig=sig->next_insert;
	}
	return n;
}

/*! Compute and return the size the pattern should take up.
 */
double SignatureInstance::PatternHeight()
{ return partition->PatternHeight(); }

/*! Compute and return the size the pattern should take up.
 */
double SignatureInstance::PatternWidth()
{ return partition->PatternWidth(); }

//! Create or recreate pagestyle and pagestyleodd.
/*! This facilitates sharing the same PageStyle objects across all pages.
 * There are only ever 2 different page styles per signature.
 *
 * If force_new==0 and pagestyle and pagestyleodd are non-null, then do nothing and return.
 *
 * \todo *** this fails when margin settings are different for different sig instances
 */
void SignatureInstance::setPageStyles(int force_new)
{
	if (!force_new && pagestyle && pagestyleodd) return;

	if (pagestyle) pagestyle->dec_count();
	if (pagestyleodd) pagestyleodd->dec_count();

	pagestyle=new RectPageStyle((IsVertical()?(RECTPAGE_LRIO|RECTPAGE_LEFTPAGE):(RECTPAGE_IOTB|RECTPAGE_TOPPAGE)));
	pagestyle->pagetype=(IsVertical()?2:1);
	pagestyle->outline=dynamic_cast<PathsData*>(GetPageOutline());
	pagestyle->margin =dynamic_cast<PathsData*>(GetPageMarginOutline(0));
	pagestyle->ml=pagestyle->margin->minx;
	pagestyle->mr=pagestyle->outline->maxx-pagestyle->margin->maxx;
	pagestyle->mt=pagestyle->outline->maxy-pagestyle->margin->maxy;
	pagestyle->mb=pagestyle->margin->miny;
	pagestyle->width =pagestyle->outline->maxx;
	pagestyle->height=pagestyle->outline->maxy;


	pagestyleodd=new RectPageStyle((IsVertical()?(RECTPAGE_LRIO|RECTPAGE_RIGHTPAGE):(RECTPAGE_IOTB|RECTPAGE_BOTTOMPAGE)));
	pagestyleodd->pagetype=(IsVertical()?3:0);
	pagestyleodd->outline=dynamic_cast<PathsData*>(GetPageOutline());
	pagestyleodd->margin =dynamic_cast<PathsData*>(GetPageMarginOutline(1));
	pagestyleodd->ml=pagestyleodd->margin->minx;
	pagestyleodd->mr=pagestyleodd->outline->maxx-pagestyleodd->margin->maxx;
	pagestyleodd->mt=pagestyleodd->outline->maxy-pagestyleodd->margin->maxy;
	pagestyleodd->mb=pagestyleodd->margin->miny;
	pagestyleodd->width =pagestyleodd->outline->maxx;
	pagestyleodd->height=pagestyleodd->outline->maxy;
}

LaxInterfaces::SomeData *SignatureInstance::GetPageOutline()
{
	PathsData *newpath=new PathsData();//count==1
	newpath->style |= PathsData::PATHS_Ignore_Weights;

	double pw=pattern->PageWidth(1),
		   ph=pattern->PageHeight(1);

	newpath->appendRect(0,0,pw,ph);
	newpath->maxx=pw;
	newpath->maxy=ph;
	//nothing special is done when local==0
	return newpath;
}

/*! The origin is the page origin, which is lower left of trim box.
 */
LaxInterfaces::SomeData *SignatureInstance::GetPageMarginOutline(int pagenum)
{
	int oddpage=pagenum%2;
	DoubleBBox box;
	pattern->PageBounds(1,&box); //margin box

	double w=box.maxx-box.minx, //margin box width
		   h=box.maxy-box.miny, //margin box height
		   pw=pattern->PageWidth(1), //trim box width
		   ph=pattern->PageHeight(1);//trim box height

	PathsData *newpath=new PathsData();//count==1
	newpath->style |= PathsData::PATHS_Ignore_Weights;

	if (pattern->binding=='l') {
		if (oddpage) newpath->appendRect(pw-w-box.minx,box.miny, w,h);
		else         newpath->appendRect(box.minx,box.miny, w,h);

	} else if (pattern->binding=='r') {
		if (oddpage) newpath->appendRect(pw-w-box.minx,box.miny, w,h);
		else         newpath->appendRect(box.minx,box.miny, w,h);

	} else if (pattern->binding=='t') {
		if (oddpage) newpath->appendRect(box.minx,ph-h-box.miny, w,h);
		else         newpath->appendRect(box.minx,box.miny, w,h);

	} else if (pattern->binding=='b') {
		if (oddpage) newpath->appendRect(box.minx,ph-h-box.miny, w,h);
		else         newpath->appendRect(box.minx,box.miny, w,h);
	}

	newpath->FindBBox();
	//nothing special is done when local==0
	return newpath;
}

void SignatureInstance::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
    Attribute att;
	dump_out_atts(&att,0,context);
    att.dump_out(f,indent);
}

LaxFiles::Attribute *SignatureInstance::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	if (!att) att=new Attribute;

	if (what==-1) {
		Attribute *subatt;

		Value::dump_out_atts(att,-1,context);
		for (int c=0; c<att->attributes.n; c++) {
			subatt=att->attributes.e[c];

			if (!strcmp(subatt->name, "partition")) {
				subatt->attributes.flush();
				PaperPartition p;
				p.dump_out_atts(subatt,-1,context);

			} else if (!strcmp(subatt->name, "pattern")) {
				subatt->attributes.flush();
				Signature pt;
				pt.dump_out_atts(subatt,-1,context);
			}
		}

		return att;
	}
	
	if (automarks) {
		char str[100]; str[0]='\0';

		if (automarks&AUTOMARK_Margins)  strcat(str,"outer ");
		if (automarks&AUTOMARK_InnerDot) strcat(str,"innerdot ");
		//if (automarks&AUTOMARK_InnerDottedLines) strcat(str,"innerdotlines ");

		if (str[0]!='\0') att->push("automarks",str);
	}

	att->push("autoaddsheets",autoaddsheets?"yes":"no");
	char ii[10];
	sprintf(ii,"%d",sheetspersignature);
	att->push("sheetspersignature",ii);

	Attribute *satt=att->pushSubAtt("pattern",NULL);
	pattern->dump_out_atts(satt,what,context);

	satt=att->pushSubAtt("partition",NULL);
	partition->dump_out_atts(satt,what,context);

	SignatureInstance *sig=next_insert;
	Attribute *aa;
	while (sig) {
		aa=sig->dump_out_atts(NULL,what,context);
		makestr(aa->name,"insert");
		att->push(aa,-1);
		sig=sig->next_insert;
	}

	return att;
}


void SignatureInstance::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	char *name,*value;
	int nump=-1;

	for (int c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"automarks")) {
			automarks=0;
			if (strstr(value,"outer")) automarks|=AUTOMARK_Margins;
			if (strstr(value,"innerdot")) automarks|=AUTOMARK_InnerDot;
			//else if (strstr(value,"innerdotlines")) automarks|=AUTOMARK_InnerDottedLines;

		} else if (!strcmp(name,"numpages")) {
			IntAttribute(value, &nump);

		} else if (!strcmp(name,"sheetspersignature")) {
			IntAttribute(value, &sheetspersignature);

		} else if (!strcmp(name,"autoaddsheets")) {
			autoaddsheets=BooleanAttribute(value);

		} else if (!strcmp(name,"pattern")) {
			Signature *sig=new Signature;
			sig->dump_in_atts(att->attributes.e[c],flag,context);
			if (pattern) pattern->dec_count();
			pattern=sig;

		} else if (!strcmp(name,"partition")) {
			if (!partition) partition=new PaperPartition();
			partition->dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"insert")) {
			SignatureInstance *insert=new SignatureInstance;
			insert->dump_in_atts(att->attributes.e[c],flag,context);
			AddInsert(insert);
		}
	}

	pattern->patternheight=partition->PatternHeight();
	pattern->patternwidth =partition->PatternWidth();
	setPageStyles(1);
}

/*! Always makes new one, does not consult stylemanager.
 */
ObjectDef *SignatureInstance::makeObjectDef()
{
	ObjectDef *sd=stylemanager.FindDef("SignatureInstance");
	if (sd) return sd;

	sd=new ObjectDef(NULL,"SignatureInstance",
			_("Signature Instance"),
			_("Combination of a PaperPartition and a Signature"),
			"class",
			NULL,NULL, //range, default value
			NULL, //fields
			0, //new flags
			NULL, //newfunc
			NULL);
			

	sd->push("sheetspersignature", _("Sheets per signature"), _("Sheets of paper per signature"),
			"int", NULL, "1", 0, NULL);

	sd->push("autoaddsheets", _("Auto add sheets"), _("Auto add or remove sheets to cover all actual pages"),
			"boolean", NULL, "false", 0, NULL);

	sd->push("automarks", _("Automarks"), _("outer|innerdot: Whether to automatically apply some printer marks."),
			"string", NULL, NULL, 0, NULL);

	sd->push("partition", _("Partition"), _("Sectioning info of the piece of paper"),
			"PaperPartition", NULL, NULL, 0, NULL);

	sd->push("pattern", _("Pattern"), _("The folding pattern"),
			"Signature", NULL, NULL, 0, NULL);


	return sd;
}

int createSignatureInstance(ValueHash *context, ValueHash *parameters,
					   Value **value_ret, ErrorLog &log)
{
	if (!parameters || !parameters->n()) {
		if (value_ret) *value_ret=new SignatureInstance;
		//log.AddMessage(_("Missing parameters!"),ERROR_Fail);
		return 0;
	}

	SignatureInstance *sig=new SignatureInstance;

	char error[100];
	int err=0;
	try {
		int i, e;

		 //---sheetspersignature
		i=parameters->findInt("sheetspersignature",-1,&e);
		if (e==0) sig->sheetspersignature=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"sheetspersignature"); throw error; }

		 //---autoaddsheets
		i=parameters->findBoolean("autoaddsheets",-1,&e);
		if (e==0) sig->autoaddsheets=(i?1:0);
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"autoaddsheets"); throw error; }

		 //---pattern
		Value *v=parameters->find("pattern");
		if (dynamic_cast<Signature*>(v)) {
			if (sig->pattern) sig->pattern->dec_count();
			sig->pattern=dynamic_cast<Signature*>(v);
			sig->pattern->inc_count();
		} else { sprintf(error, _("Invalid format for %s!"),"pattern"); throw error; }

	} catch (const char *str) {
		log.AddMessage(str,ERROR_Fail);
		err=1;
	}

	if (value_ret && err==0) {
		*value_ret=sig;
		sig->inc_count();

		//} else {
			//log.AddMessage(_("Incomplete SignatureInstance definition!"),ERROR_Fail);
			//err=1;
		//}
	}
	sig->dec_count();

	return err;
}

//------------------------------------- SignatureImposition --------------------------------------
/*! \class SignatureImposition
 * \brief Imposition based on rectangular folded paper.
 *
 * The steps for creating a signature based imposition are:
 *
 * 1. Tile paper into partitions, with a PaperPartition.
 * 2. Define folding per partition, with a Signature.\n
 * 3. Define any finishing trim and margin within the Signature
 * 4. specify which edge of the folded up paper to use as the binding edge, if any\n
 * 5. stack signatures back to back, or insert them into each other. See SignatureInstance.
 *
 * If any trim value is less than 0, then no trim is done on that edge. This lets users define
 * accordion impositions, potentially, by folding many times, but only trimming the top and bottom,
 * for instance. The trimmed area is considered a bleed area.
 *
 * The trim value along an edge that is the binding edge will not be considered to be actually trimmed,
 * but page content will not extend into those areas, except to bleed. In other words, page spread views will show
 * the pages touching each other, but pages will be laid down into a signature with a bit of a gap.
 *
 */
/*! \var int Signature::showwholecover
 * \brief Whether to show the front and back pages together or not.
 */


/*! This will increment the count of newsig.
 */
SignatureImposition::SignatureImposition(SignatureInstance *newsig)
	: Imposition("SignatureImposition")
{
	showwholecover=0;

	signatures=NULL;
	if (newsig) signatures=(SignatureInstance*)newsig->duplicate();
	
	objectdef=stylemanager.FindDef("SignatureImposition");
	if (objectdef) objectdef->inc_count(); 
	else {
		objectdef=makeObjectDef();
		if (objectdef) stylemanager.AddObjectDef(objectdef,0);
	}

	name=description=NULL;
}

SignatureImposition::~SignatureImposition()
{
	if (signatures) delete signatures;
	//if (partition) partition->dec_count();

}

ImpositionInterface *SignatureImposition::Interface()
{
	return new SignatureInterface();
}

//! Static imposition resource creation function.
/*! Returns NULL terminated list of default resources.
 *
 * \todo return resources for double sided singles, booklet, calendar, 2 fold, 3 fold
 */
ImpositionResource **SignatureImposition::getDefaultResources()
{
	ImpositionResource **r=new ImpositionResource*[3];

	Attribute *att =new Attribute;
	Attribute *satt=att->pushSubAtt("signature",NULL);      
	satt=satt->pushSubAtt("pattern");
	satt->push("binding","left");
	r[0]=new ImpositionResource("SignatureImposition",
								  _("Double Sided Singles"),
								  NULL,
								  _("Imposition of single pages meant to be next to each other"),
								  att,1);

	att=new Attribute;
	satt=att->pushSubAtt("signature",NULL);      
	satt->push("autoaddsheets");
	satt=satt->pushSubAtt("pattern");
	satt->push("fold","1 Right");
	satt->push("numvfolds","1");
	satt->push("binding","left");

	DBG cerr <<"-----** Booklet att resource:"<<endl;
	DBG att->dump_out(stderr, 0);

	r[1]=new ImpositionResource("SignatureImposition",
								  _("Booklet"),
								  NULL,
								  _("Imposition for a stack of sheets, folded down the middle"),
								  att,1);
	r[2]=NULL;
	return r;
}

/*! Return total number of SignatureInstances in signatures.
 */
int SignatureImposition::TotalNumStacks()
{
	SignatureInstance *sig=signatures;
	SignatureInstance *s2;
	int num=0;
	while (sig) {
		s2=sig;
		while (s2) {
			num++;
			s2=s2->next_insert;
		}
		sig=sig->next_stack;
	}

	return num;
}

/*! If which<0, then return the number of defined stacks not including inserts.
 * If which>=0, then return the number of inserts on the designated stack (including the head).
 */
int SignatureImposition::NumStacks(int which)
{
	int num=0;
	SignatureInstance *sig=signatures;
	while (sig) {
		if (which>=0 && which==num) {
			 //return inserts size on stack which
			num=0;
			while (sig) {
				num++;
				sig=sig->next_insert;
			}
			return num;
		}
		num++;
		sig=sig->next_stack;
	}

	return num;
}

//! Install a particular creep value for which signature.
/*! Creep is how much horizontally the middle of a signature will poke out
 * relative to the outer page. The task is to adjust trim areas per page to reflect this,
 * by chopping off bits of page opposite to the binding edge.
 * 
 * Note we can approximate the creep as pi*(numsheets)*(paper thickness)/2.
 *
 * Return 0 for success.
 */
int SignatureImposition::Creep(int which,double d)
{
	DBG cerr <<" *** need to implement SignatureImposition::Creep()!"<<endl;
	return 1;
}

//! Return pointer to the specified SignatureInstance.
/*! If stack<0, then return last one. If insert<0, return innermost for stack.
 */
SignatureInstance *SignatureImposition::GetSignature(int stack,int insert)
{
	if (stack<0) stack=NumStacks(-1)-1;
	if (insert<0) insert=NumStacks(stack)-1;

	if (!signatures) {
		signatures=new SignatureInstance;
	}
	SignatureInstance *sig=signatures;

	while (sig->next_stack  && stack>0)  { sig=sig->next_stack;  stack--;  }
	while (sig->next_insert && insert>0) { sig=sig->next_insert; insert--; }

	return sig;
}

/*! Remove current signature stack, and replace with one based on newsig. Partition is not changed.
 * Return 0 for success, 1 for error and no change.
 */
int SignatureImposition::UseThisSignature(Signature *newsig)
{
	if (!newsig) return 1;

	PaperPartition *paper=NULL;
	if (signatures) paper=signatures->partition;
	if (paper) paper->inc_count();

	if (signatures) signatures->dec_count();
	signatures=new SignatureInstance(newsig,paper);
	if (paper) paper->dec_count();

	return 0;
}

//! Return default paper dimensions if paperorpage==0, or page dimensions for paperorpage==1.
void SignatureImposition::GetDimensions(int paperorpage, double *x, double *y)
{
	if (paperorpage==0) {
		 //paper dimensions
		*x=signatures->partition->totalwidth;
		*y=signatures->partition->totalheight;
		return;
	}

	*x=signatures->pattern->PageWidth(1);
	*y=signatures->pattern->PageHeight(1);
}

//! Return something like "2 fold, 8 page signature".
const char *SignatureImposition::BriefDescription()
{
	if (description) return description;

	static char briefdesc[100]; //note this is not threadsafe, so be quick!!
	if (signatures->partition->tilex>1 || signatures->partition->tiley>1) 
		sprintf(briefdesc,_("%d page signature, tiled %dx%d"),
					signatures->PagesPerSignature(-1,0),
					signatures->partition->tilex, signatures->partition->tiley);
	else sprintf(briefdesc,_("%d page signature"),signatures->PagesPerSignature(-1,0));

	return briefdesc;
}

Value *SignatureImposition::duplicate()
{
	SignatureImposition *sn;
	sn=new SignatureImposition(signatures);

	sn->showwholecover=showwholecover;

	sn->name=increment_file(name);
	makestr(sn->description,description);

	sn->NumPages(NumPages());
	return sn;
}

ObjectDef *SignatureImposition::makeObjectDef()
{
	return makeSignatureImpositionObjectDef();
}

/*! Please note this returns vertical ONLY on first pattern. Normally this is ok, as
 * all patterns in single stacks are supposed to have the same verticality.
 */
int SignatureImposition::IsVertical()
{
	return signatures->pattern->IsVertical();
}

//! Return 2, for Top/Left or Bottom/Right.
/*! Which of top or left is determined by pattern->IsVertical(). Same for bottom/right.
 */
int SignatureImposition::NumPageTypes()
{ return 2; }

//! If IsVertical(), return "Top" or "Bottom" else return "Left" or "Right".
/*! Returns NULL if not a valid pagetype.
 */
const char *SignatureImposition::PageTypeName(int pagetype)
{
	if (pagetype==0) return IsVertical() ? _("Top") : _("Left");
	if (pagetype==1) return IsVertical() ? _("Bottom") : _("Right");
	return NULL;
}

//! Return the page type for the given document page index.
/*! 0 is either top or left. 1 is either bottom or right.
 */
int SignatureImposition::PageType(int page)
{
	//Note that showwholecover is irrelevant here.
	int left=((page+1)/2)*2-1;
	if (left==page && IsVertical()) return 0; //top
	if (left==page) return 0; //left
	if (IsVertical()) return 1; //bottom
	return 1; //right
}

//! Return the type of spread that the given spread index is, always 0 here.
int SignatureImposition::SpreadType(int spread)
{
	return 0;
}


//--------------functions to locate spreads and pages...

//! Return which paper number the given document page lays on.
int SignatureImposition::PaperFromPage(int pagenumber)
{
	int stack, insert, insertpage, row, col;
	int pp=signatures->locatePaperFromPage(pagenumber, &stack, &insert, &insertpage, &row, &col, NULL);
	return pp;
}

int SignatureImposition::SpreadFromPage(int layout, int pagenumber)
{
	if (layout==SINGLELAYOUT) return pagenumber;
	if (layout==PAPERLAYOUT) return PaperFromPage(pagenumber);
	if (layout==PAGELAYOUT) return (pagenumber+1-showwholecover)/2;
	return 0;
}


//-----------basic setup

//! Return the number of pages needed to fill this many paper spreads.
/*! Note that there are 2 paper spreads per actual piece of paper.
 */
int SignatureImposition::GetPagesNeeded(int npapers)
{
 
	int pp=signatures->PaperSpreadsPerSignature(-1,0);
	pp/=npapers+1; //how many complete signature stacks needed
	int npages=signatures->PagesPerSignature(-1,0);
	return npages * pp;
}

//! Return the number of paper spreads needed to hold that many pages.
/*! Paper back and front count as two different papers.
 *
 * Note this is paper spreads, which is twice the number of physical
 * pieces of paper.
 */
int SignatureImposition::GetPapersNeeded(int npages)
{
	int pp=signatures->PagesPerSignature(-1,0);
	return ((npages-1)/pp+1)*pp;
}

int SignatureImposition::GetSpreadsNeeded(int npages)
{ return (npages)/2+1; } 


//! With the final trimmed page size (w,h), set the paper size to the proper size to just contain it.
/*! This adjusts all SignatureInstance objects in signatures.
 */
int SignatureImposition::SetPaperFromFinalSize(double w,double h)
{
	signatures->SetPaperFromFinalSize(w,h, 1);
	return 0;
}

/*! Return that stored in Imposition::papergroup if any. Else the paper in the first siginstance.
 */
PaperStyle *SignatureImposition::GetDefaultPaper()
{
	PaperStyle *p=Imposition::GetDefaultPaper();
	if (p) return p;
	return signatures->partition->paper;
}

//! This will duplicate npaper.
/*! Note this will ONLY affect papers of instances is signatures that have the same
 * page dimensions and final page sizes as this->signtaures.
 *
 * More thorough changes must be done manually, as different signature instances can have
 * different paper sizes.
 */
int SignatureImposition::SetPaperSize(PaperStyle *npaper)
{
	Imposition::SetPaperSize(npaper); //sets imposition::paperbox and papergroup
	signatures->SetPaper(npaper, 1);
	return 0;
}

int SignatureImposition::SetDefaultPaperSize(PaperStyle *npaper)
{
	Imposition::SetPaperSize(npaper); //sets imposition::paperbox and papergroup
	return 0;
}

/*! Signatures will be set only with the paper style of the first paper in group.
 */
int SignatureImposition::SetPaperGroup(PaperGroup *ngroup)
{
	Imposition::SetPaperGroup(ngroup);
	signatures->SetPaper(paper->paperstyle,1);
	return 1;
}

//! Return the number of spreads for the given type that currently exist.
int SignatureImposition::NumSpreads(int layout)
{
	if (layout==PAPERLAYOUT) return NumPapers();

	if (layout==PAGELAYOUT || layout==LITTLESPREADLAYOUT) {
		//if (pattern->numhfolds+pattern->numvfolds==0 && numdocpages==2) return 2;
		return numpages/2+1;
	}

	if (layout==SINGLELAYOUT) return numdocpages;
	return 0;
}


//! Return the number of paper spreads.
/*! This is twice the number of physical pieces of paper.
 */
int SignatureImposition::NumPapers()
{
	int pp=signatures->PaperSpreadsPerSignature(-1,0);
	int numsignatures= (numpages-1)/signatures->PagesPerSignature(-1,0) + 1;
	return pp * numsignatures;
}

//! Set the number of papers.
/*! Returns the number of papers the imposition thinks it needs.
 * This will not be less than npapers, but might be more.
 */
int SignatureImposition::NumPapers(int npapers)
{
	int pp=signatures->PaperSpreadsPerSignature(-1,0);
	numpapers=((npapers-1)/pp + 1) * pp;
	return numpapers;
}

//! Returns the number of pages the imposition thinks there should be.
/*! Please note this might be different than the number of actual document pages.
 */
int SignatureImposition::NumPages()
{
	return numpages; //note this is not numdocpages, which is a hint for current document
}

//! Set the number of document pages that must be fit into the imposition.
/*! Returns the number of pages the imposition thinks there should be, and
 * makes it so the other NumPages() returns how many the imposition thinks there should be.
 * This might be more than npages.
 *
 * For each SignatureInstance that has autoaddsheets, distribute new sheets among them,
 * by adding one then going down inserts, then across stacks. No instance can have
 * fewer than one sheet.
 */
int SignatureImposition::NumPages(int npages)
{
	//update the number of papers to accomodate npages

	//if (numdocpages==npages) return numpages;
	numdocpages=npages;
	if (!signatures) signatures=new SignatureInstance();

	//all SignatureInstance objects with autoaddsheets==true get excess pages distributed
	//between them. If there are no autoaddsheets, then whole blocks of signatures
	//need to be added

	 //first pass, find out how many autoaddsheets instances there are
	int numauto=0;
	int numstatic=0;
	int n;
	SignatureInstance *s1=signatures,*s2;
	while (s1) {
		s2=s1;
		while (s2) {
			if (s2->sheetspersignature<=0) s2->sheetspersignature=1; //check for corrupt sheetspersignature
			n=s2->pattern->PagesPerPattern()*s2->sheetspersignature;
			if (s2->autoaddsheets) { numauto+=n; s2->sheetspersignature=1; }
			else numstatic+=n;
			
			s2=s2->next_insert;
		}
		s1=s1->next_stack;
	}

	if (numauto==0) {
		 //Hooray, the simple case!
		int nper=signatures->PagesPerSignature(-1,0);
		numpages=(npages/nper + 1)*nper;
		return numpages;
	}

	npages-=numstatic;

	//Second pass: now we must distribute the remaining pages equally across the auto signatures,
	//with at least 1 paper in each auto... Above each sheetspersignature was set to one, so on
	//first go through, just remove number of pages.
	int firsttime=1;
	while (npages>0) {
		s1=signatures;
		while (s1 && npages>0) {
			s2=s1;
			while (s2) {
				if (s2->autoaddsheets) {
					if (firsttime) s2->sheetspersignature=1;
					else s2->sheetspersignature++;

					npages-=s2->pattern->PagesPerPattern();
					if (npages<=0) break;
				}
				
				s2=s2->next_insert;
			}
			s1=s1->next_stack;
		}
		if (firsttime) firsttime=0;
	}

	numpages=signatures->PagesPerSignature(-1,0);
	return numpages;
}


//---------------doc Page[] maintenance

//! Create or recreate pagestyle and pagestyleodd within each stack and insert.
/*! This facilitates sharing the same PageStyle objects across all pages.
 * There are only ever 2 different page styles per signature instance.
 *
 * This function just steps through each siginstance and calls sig->setPageStyles().
 */
void SignatureImposition::setPageStyles(int force_new)
{
	if (!signatures) return;

	SignatureInstance *sig=signatures, *si;

	while (sig) {
		si=sig;
		while (si) {
			si->setPageStyles(force_new);
			si=si->next_insert;
		}
		sig=sig->next_stack;
	}
}

//! Return NULL terminated list of Page objects.
Page **SignatureImposition::CreatePages(int npages)
{
	if (npages>0) NumPages(npages);

	if (numdocpages==0) return NULL;
	 //create if they were null
	setPageStyles(1);
	
	Page **newpages=new Page*[numdocpages+1];
	int c;
	SignatureInstance *sig;
	for (c=0; c<numdocpages; c++) {
		sig=signatures->InstanceFromPage(c,NULL,NULL,NULL,NULL,NULL,NULL);
		newpages[c]=new Page(((c%2)?sig->pagestyleodd:sig->pagestyle),c); // this incs count of pagestyle

		 //add bleed information
		fixPageBleeds(c,newpages[c],false);
	}
	newpages[c]=NULL;

	return newpages;
}

//! Make sure the page bleeding is set up correctly for the specified document page.
/*! There are 4 different possible page arrangements for signatures,
 * l->r, r->l, t->b, b->t.
 *
 * \todo assumption is that each signature has the same final page size.. maybe this isn't necessary?
 */
void SignatureImposition::fixPageBleeds(int index, //!< Document page index
										Page *page,
										bool update_pagestyle)//!< Actual document page
{
	 //fix pagestyle
	if (!signatures) signatures=new SignatureInstance();
	SignatureInstance *sig=signatures->InstanceFromPage(index,NULL,NULL,NULL,NULL,NULL,NULL);

	if (update_pagestyle) {
		page->InstallPageStyle((index%2)?sig->pagestyleodd:sig->pagestyle, true);
		page->pagebleeds.flush();
	}

	 //fix page bleed info
	int adjacent=-1;	
	double m[6];
	transform_identity(m);
	char dir=0;
	int odd=(index%2);
	double pw=signatures->pattern->PageWidth(1);
	double ph=signatures->pattern->PageHeight(1);

	char binding = signatures->pattern->binding;
	if      (binding=='l') { if (odd) { dir='r'; adjacent=index+1; } else { dir='l'; adjacent=index-1; } }
	else if (binding=='r') { if (odd) { dir='l'; adjacent=index+1; } else { dir='r'; adjacent=index-1; } }
	else if (binding=='t') { if (odd) { dir='b'; adjacent=index+1; } else { dir='t'; adjacent=index-1; } }
	else {                   if (odd) { dir='t'; adjacent=index+1; } else { dir='b'; adjacent=index-1; } } //botom binding

	m[4]=m[5]=0;
	if      (dir=='l') { m[4] = -pw; }
	else if (dir=='r') { m[4] = pw; }
	else if (dir=='t') { m[5] = -ph; }
	else               { m[5] = ph; } //botom binding

	if (adjacent<0 && showwholecover) {
		adjacent = numpages-1;
	} else if (adjacent == numpages && showwholecover) {
		adjacent = 0;
	}
	if (adjacent>=0 && adjacent < numpages) page->pagebleeds.push(new PageBleed(adjacent,m));
}

//! Ensure that each page has a proper pagestyle and bleed information.
/*! This is called when pages are added or removed. It replaces the pagestyle for
 *  each page with the pagestyle returned by GetPageStyle(c,0).
 */
int SignatureImposition::SyncPageStyles(Document *doc,int start,int n, bool shift_within_margins)
{
	setPageStyles(1);

	int status=Imposition::SyncPageStyles(doc,start,n, shift_within_margins);

	for (int c=start; c<doc->pages.n; c++) {
		fixPageBleeds(c,doc->pages.e[c],false);
	}

	return status;
}

//! Return default page style for the specified document page index.
/*! Returns an inc_counted version.
 */
PageStyle *SignatureImposition::GetPageStyle(int pagenum,int local)
{
	setPageStyles(0); //create if they were null

	SignatureInstance *sig=signatures->InstanceFromPage(pagenum,NULL,NULL,NULL,NULL,NULL,NULL);
		
	PageStyle *style= (pagenum%2) ? sig->pagestyleodd : sig->pagestyle;
	style->inc_count();
	return style;
}

//! Return outline of page in page coords.
/*! \todo *** assumes final page size same across stacks
 */
LaxInterfaces::SomeData *SignatureImposition::GetPageOutline(int pagenum,int local)
{
	SignatureInstance *sig=signatures->InstanceFromPage(pagenum,NULL,NULL,NULL,NULL,NULL,NULL);
	return sig->GetPageOutline();
}

/*! The origin is the page origin, which is lower left of trim box.
 */
LaxInterfaces::SomeData *SignatureImposition::GetPageMarginOutline(int pagenum,int local)
{
	SignatureInstance *sig=signatures->InstanceFromPage(pagenum,NULL,NULL,NULL,NULL,NULL,NULL);
	return sig->GetPageMarginOutline(pagenum);
}


LaxInterfaces::SomeData *SignatureImposition::GetPrinterMarks(int papernum)
{
	return NULL;
}
	
//---------------spread generation
Spread *SignatureImposition::Layout(int layout,int which)
{
	if (layout==PAPERLAYOUT) return PaperLayout(which);
	if (layout==PAGELAYOUT) return PageLayout(which);
	if (layout==SINGLELAYOUT) return SingleLayout(which);
	if (layout==LITTLESPREADLAYOUT) return PageLayout(which);
	return NULL;
}

//! Returns 3, for the usual single, page, and paper.
int SignatureImposition::NumLayoutTypes()
{
	return 3; //paper, pages, single
	//return 2+signature->numhfolds+signature->numvfolds;
}

const char *SignatureImposition::LayoutName(int layout)
{
	if (layout==PAPERLAYOUT) return _("Papers");
	if (layout==PAGELAYOUT) return _("Pages");
	if (layout==SINGLELAYOUT) return _("Singles");
	if (layout==LITTLESPREADLAYOUT) return _("Little Spreads");

//----for kicks, maybe allow showing spreads of partially folded?
//	layout-=MAXDEFAULTLAYOUTNUMBER;
//	if (layout>=0 && layout<signature->numhfolds+signature->numvfolds) {
//		static char str[100]; //not thread safe, but probably ok.
//		if (layout==0) sprintf(str,_("After 1 fold"));
//		else sprintf(str,_("After %d folds"),layout+1);
//		return str;
//	}

	return NULL;
}

Spread *SignatureImposition::SingleLayout(int whichpage)
{
	return Imposition::SingleLayout(whichpage);
}

//! Return a spread with either one or two pages on it.
/*! If showwholecover!=0, then the first spread has the first page and the last one
 * in one spread, and objects can bleed to each other. If not, spread 0 has only page
 * 0, and the final spread only has the back cover (final page).
 *
 *  \todo *** assumes final page size same across stacks
 */
Spread *SignatureImposition::PageLayout(int whichspread)
{
	Spread *spread=new Spread();
	spread->spreadtype=2;
	spread->style=SPREAD_PAGE;
	spread->mask=SPREAD_PATH|SPREAD_MINIMUM|SPREAD_MAXIMUM;


	 //first figure out which pages should be on the spread
	int page1=whichspread*2; //eventually, page1 is the one with lower left corner at origin.
	int page2=-1;           //and numerically page2 will be > page1

	SignatureInstance *sig=signatures->InstanceFromPage(page1,NULL,NULL,NULL,NULL,NULL,NULL);
	double pw=sig->pattern->PageWidth(1);
	double ph=sig->pattern->PageHeight(1);

	double page2offsetx=0, page2offsety=0;

	char binding=signatures->pattern->binding;
	if      (binding=='l') { page2offsetx=pw; page2=page1; page1--; }
	else if (binding=='r') { page2offsetx=pw; page2=page1-1; }
	else if (binding=='t') { page2offsety=ph; page2=page1-1; }
	else                   { page2offsety=ph; page2=page1; page1--; } //bottom binding

	 //wrap back cover to front cover ONLY IF the final document page is actually physically on the back cover..
	if (showwholecover) {
		if (page1<0) page1=numpages-1;
		else if (page2==numpages) page2=0;
	}
	if (page1>=numpages) page1=-1;
	if (page2>=numpages) page2=-1;

	PathsData *newpath=new PathsData(); //newpath has all the paths used to draw the whole spread
	newpath->style |= PathsData::PATHS_Ignore_Weights;

	if (binding=='l' || binding=='r') {
		double o=0, w=pw;
		if (page1>=0 && page2>=0) w+=pw;
		if (page1<0) o += pw;

		spread->path = (SomeData *)newpath;
		newpath->appendRect(o,0, w,ph);
		if (page1>=0 && page2>=0) {
			newpath->pushEmpty();
			newpath->append(pw,0);
			newpath->append(pw,ph);
		}

	} else {
		double o=0, h=ph;
		if (page1>=0 && page2>=0) h += ph;
		if (page1<0) o += ph;

		spread->path = (SomeData *)newpath;
		newpath->appendRect(0,o, pw,h);
		if (page1>=0 && page2>=0) {
			newpath->pushEmpty();
			newpath->append(0,ph);
			newpath->append(pw,ph);
		}
	}
	newpath->FindBBox();


	 // Now setup spread->pagestack with the single pages.
	 // page width/height must map to proper area on page.
	if (page1>=0) {
		newpath=new PathsData;  // 1 count
		newpath->appendRect(0,0, pw,ph);
		spread->pagestack.push(new PageLocation((page1<numdocpages?page1:-1),NULL,newpath)); // incs count of g (to 2)
		newpath->dec_count(); // remove extra tick
		newpath=NULL;
	}

	if (page2>=0) {
		newpath=new PathsData;  // 1 count
		newpath->appendRect(0,0, pw,ph);
		newpath->origin(flatpoint(page2offsetx,page2offsety));
		spread->pagestack.push(new PageLocation((page2<numdocpages?page2:-1),NULL,newpath)); // incs count of g (to 2)
		newpath->dec_count(); // remove extra tick
		newpath=NULL;
	}

	 //set minimum and maximum
	if (binding=='l') {
		if (page1>=0) spread->minimum=flatpoint(pw/5,ph/2);
		else spread->minimum=flatpoint(pw*1.2,ph/2);
		if (page2>=0) spread->maximum=flatpoint(pw*1.8,ph/2);
		else spread->maximum=flatpoint(pw*.8,ph/2);

	} else if (binding=='r') {
		if (page1>=0) spread->maximum=flatpoint(pw/5,ph/2);
		else spread->maximum=flatpoint(pw*1.2,ph/2);
		if (page2>=0) spread->minimum=flatpoint(pw*1.8,ph/2);
		else spread->minimum=flatpoint(pw*.8,ph/2);

	} else if (binding=='t') {
		if (page1>=0) spread->maximum=flatpoint(pw/2,ph*.2);
		else spread->maximum=flatpoint(pw/2,ph*1.2);
		if (page2>=0) spread->minimum=flatpoint(pw/2,ph*1.8);
		else spread->minimum=flatpoint(pw/2,ph*.8);

	} else { //botom binding
		if (page1>=0) spread->minimum=flatpoint(pw/2,ph*.2);
		else spread->minimum=flatpoint(pw/2,ph*1.2);
		if (page2>=0) spread->maximum=flatpoint(pw/2,ph*1.8);
		else spread->maximum=flatpoint(pw/2,ph*.8);
	}

	return spread;
}

//! Return spread with all the pages properly flipped.
/*! There are two paper spreads per physical piece of paper.
 *
 * The back side of a paper is constructed as if you flipped the paper over left to right.
 */
Spread *SignatureImposition::PaperLayout(int whichpaper)
{
	if (whichpaper<0) whichpaper=0;

	 //find containing siginstance
	int mainpageoffset=0, opposite_offset=0;
	int sigpaper;
	SignatureInstance *sig = InstanceFromPaper(whichpaper, NULL,NULL, &sigpaper, &mainpageoffset, &opposite_offset, NULL);
	int front = (1+sigpaper)%2; //whether to horizontally flip columns


	//Create the actual Spread...

	int num_pages_in_insert = sig->PagesPerSignature(0,1);
	Signature *signature = sig->pattern;
	PaperStyle *paper = sig->partition->paper;

	Spread *spread = new Spread();
	spread->spreadtype = 1;
	spread->style = SPREAD_PAPER;
	spread->mask = SPREAD_PATH| SPREAD_PAGES| SPREAD_MINIMUM| SPREAD_MAXIMUM;


	spread->papergroup=new PaperGroup(paper);


	 // define max/min points
	spread->minimum = flatpoint(paper->w()/5,  paper->h()/2);
	spread->maximum = flatpoint(paper->w()*4/5,paper->h()/2);
	//double paperwidth = paper->w();

//	if (foldinfo[0][0].finalindexfront<0) {
//		*** signature is incomplete, should do something meaningful?
//	}
	
	 //--- make the paper outline
	PathsData *newpath = new PathsData();
	//newpath->appendRect(0,0,paper->media.maxx,paper->media.maxy);
	//newpath->FindBBox();
	spread->path = (SomeData *)newpath;


	 //---- make the pagelocation stack
	double x,y;
	double xx,yy;
	int xflip, yflip;

	double patternheight = signature->PatternHeight();
	double patternwidth  = signature->PatternWidth();
	double ew = patternwidth /(signature->numvfolds+1);//width  of a cell in a pattern
	double eh = patternheight/(signature->numhfolds+1);//height of a cell in a pattern
	double pw = ew-signature->trimleft-signature->trimright;//page width == cell - page trim
	double ph = eh-signature->trimtop -signature->trimbottom;

	 //in a signature, if there is only one sheet, each page cell can map to 2 pages,
	 //the front and the back, whose document page indices are adjacent. When there are
	 //more than 1 paper sheet per signature, then each cell maps to (num sheets)*2 pages.
	int rangeofpages = sig->sheetspersignature*2;
	int pageindex;
	int ff,tt;

	DBG cerr <<" signature pattern for paper spread "<<whichpaper<<":"<<endl;

	 //for each tile:
	//x = sig->partition->insetleft;
	//x = (front ? sig->partition->insetleft : sig->partition->insetright);
	x = 0;
	PathsData *pageoutline;
	for (int tx=0; tx<sig->partition->tilex; tx++) {
	  y = sig->partition->insetbottom;
	  for (int ty=0; ty<sig->partition->tiley; ty++) {

		 //for each cell within each tile:
		for (int rr=0; rr<signature->numhfolds+1; rr++) {
		  for (int cc=0; cc<signature->numvfolds+1; cc++) {

			xflip = signature->foldinfo[rr][cc].finalxflip;
			yflip = signature->foldinfo[rr][cc].finalyflip;

			DBG cerr <<signature->foldinfo[rr][cc].finalindexfront<<"/"<<signature->foldinfo[rr][cc].finalindexback<<"  ";

			xx = x + (front ? signature->numvfolds-cc : cc)*ew;
			yy = y + rr*eh; //coordinates of corner of page cell

			 //take in to account the final trim
			if (xflip) xx += signature->trimright;
			else       xx += signature->trimleft;
			//if (xflip) xx += (front ? signature->trimright : signature->trimleft);
			//else       xx += (front ? signature->trimleft  : signature->trimright);
			if (yflip) yy += signature->trimtop;   else yy += signature->trimbottom;

			 //flip horizontally for odd numbered paper spreads (the backs of papers)
			if (front) xx += sig->partition->insetleft;
			else xx += sig->partition->insetright;

			ff = signature->foldinfo[rr][cc].finalindexback;
			tt = signature->foldinfo[rr][cc].finalindexfront;
			//---------
			if (ff>tt) {
				tt*=rangeofpages/2;
				ff=tt + rangeofpages - 1;
			} else {
				ff*=rangeofpages/2;
				tt=ff + rangeofpages-1;
			}
			if (ff>tt) pageindex=ff-sigpaper;
			else pageindex=ff+sigpaper;
//			---------
//			if (ff > tt) {
//				pageindex= tt*rangeofpages/2 + rangeofpages-1 - sigpaper;
//			} else {
//				pageindex= tt*rangeofpages/2 + sigpaper;
//			}
//			----------
			if (pageindex >= num_pages_in_insert/2) pageindex += opposite_offset;
			else pageindex += mainpageoffset;


			pageoutline=new PathsData();//count of 1
			pageoutline->appendRect(0,0, pw,ph); //page outline
			pageoutline->FindBBox();

			if (yflip) {
				 //rotate 180 degrees when page is upside down
				pageoutline->origin(flatpoint(xx+pw,yy+ph));
				pageoutline->xaxis(flatpoint(-1,0));
				pageoutline->yaxis(flatpoint(0,-1));
			} else {
				pageoutline->origin(flatpoint(xx,yy)); // *** final page trim needs to be applied
			}

			spread->pagestack.push(new PageLocation((pageindex<numdocpages?pageindex:-1),NULL,pageoutline));
			pageoutline->dec_count();//remove extra count

		  } //cc
		}  //rr

		y += patternheight+sig->partition->tilegapy;
		DBG cerr <<endl;
	  } //tx
	  x += patternwidth+sig->partition->tilegapx;
	} //ty



	//PageLocation stack created, now need to add any special cut/fold/other printer marks
	

	 //------Apply any automatic cut and fold marks
	if (sig->automarks) {

		 //In the inset areas, draw solid lines for cut marks, dotted lines for fold lines

		PathsData *cut=NULL, *fold=NULL, *dots=NULL;


		 //loop along the left and right
		double y,y2, xl, xr;
		for (int c=0; c<sig->partition->tiley+1; c++) {
			y = sig->partition->insetbottom+c*(signature->PatternHeight()+sig->partition->tilegapy);
			y2 = -1;

			if (c>0 && c<sig->partition->tiley && sig->partition->tilegapy) {
				y2 = y-sig->partition->tilegapy;
			} else if (c==sig->partition->tiley) y -= sig->partition->tilegapy;

			if (front) { xl = sig->partition->insetleft;  xr = sig->partition->insetright; }
			else       { xl = sig->partition->insetright; xr = sig->partition->insetleft;  }

			if (sig->automarks & AUTOMARK_Margins) { //cut marks in inset region
				if (!cut) cut=new PathsData;

				if (xl >= 2*GAP || xr >= 2*GAP)  {
					if (xl > GAP) {
						cut->pushEmpty();
						cut->append(GAP, y);
						cut->append(xl - GAP,y);
					}
					if (xr > GAP) {
						cut->pushEmpty();
						cut->append(sig->partition->totalwidth - xr + GAP,y);
						cut->append(sig->partition->totalwidth - GAP,y);
					}
					if (y2>0) {
						if (xl > GAP) {
							cut->pushEmpty();
							cut->append(GAP, y2);
							cut->append(xl-GAP,y2);
						}
						if (xr > GAP) {
							cut->pushEmpty();
							cut->append(sig->partition->totalwidth - xr + GAP,y2);
							cut->append(sig->partition->totalwidth - GAP,y2);
						}
					}

					 //dotted fold lines in inset area
					if (signature->numhfolds && c<sig->partition->tiley) {
						if (!fold) fold = new PathsData;
						double yf;
						for (int f=1; f <= signature->numhfolds; f++) {
							yf = y+f*signature->PageHeight(0);

							if (xl) {
								fold->pushEmpty();
								fold->append(GAP,yf);
								fold->append(xl - GAP,yf);
							}
							if (xr) {
								fold->pushEmpty();
								fold->append(sig->partition->totalwidth - xr + GAP,yf);
								fold->append(sig->partition->totalwidth - GAP,yf);
							}
						}
					}
				}
			}//cut marks in inset region

			if (sig->automarks & AUTOMARK_InnerDot) {
				if (!dots) dots=new PathsData;

				 //dots on left
				dots->pushEmpty();
				dots->append(xl + GAP,y);
				dots->append(xl + GAP*1.001,y);
				 //dots on right
				dots->pushEmpty();
				dots->append(sig->partition->totalwidth - xr - GAP,y);
				dots->append(sig->partition->totalwidth - xr - GAP*1.001,y);
				
				if (y2>0) {
					if (xl) {
						dots->pushEmpty();
						dots->append(xl + GAP,y2);
						dots->append(xl + GAP*1.001,y2);
					}
					if (xr) {
						dots->pushEmpty();
						dots->append(sig->partition->totalwidth - xr - GAP,y2);
						dots->append(sig->partition->totalwidth - xr - GAP*1.001,y2);
					}
				}
			}

			//if (signature->automarks&AUTOMARK_InnerDottedLines) {}
		} //loop

		 //Loop along top and bottom
		if (!cut) cut=new PathsData;
		double x,x2;
		for (int c=0; c<sig->partition->tilex+1; c++) {
			x = (front ? sig->partition->insetleft : sig->partition->insetright) + c*(signature->PatternWidth()+sig->partition->tilegapx);
			x2 = -1;
			if (c>0 && c<sig->partition->tilex && sig->partition->tilegapx) {
				x2 = x-sig->partition->tilegapx;
			} else if (c==sig->partition->tilex) x -= sig->partition->tilegapx;

			if ((sig->automarks & AUTOMARK_Margins)
				  && (sig->partition->insettop>2*GAP || sig->partition->insetbottom>2*GAP))  {
				if (!cut) cut=new PathsData;

				 //cut marks in inset region
				if (sig->partition->insetbottom) {
					cut->pushEmpty();
					cut->append(x,GAP);
					cut->append(x,sig->partition->insetbottom-GAP);
				}
				if (sig->partition->insettop) {
					cut->pushEmpty();
					cut->append(x,sig->partition->totalheight-sig->partition->insettop+GAP);
					cut->append(x,sig->partition->totalheight-GAP);
				}
				if (x2>0) {
					if (sig->partition->insetbottom) {
						cut->pushEmpty();
						cut->append(x2,GAP);
						cut->append(x2,sig->partition->insetbottom-GAP);
					}
					if (sig->partition->insettop) {
						cut->pushEmpty();
						cut->append(x2,sig->partition->totalheight-sig->partition->insettop+GAP);
						cut->append(x2,sig->partition->totalheight-GAP);
					}
				}

				 //dotted fold lines in inset area
				if (signature->numvfolds && c<sig->partition->tilex) {
					if (!fold) fold=new PathsData;
					double xf;
					for (int f=1; f<=signature->numvfolds; f++) {
						xf=x+f*signature->PageWidth(0);

						if (sig->partition->insetbottom) {
							fold->pushEmpty();
							fold->append(xf,GAP);
							fold->append(xf,sig->partition->insetbottom-GAP);
						}
						if (sig->partition->insettop) {
							fold->pushEmpty();
							fold->append(xf,sig->partition->totalheight-sig->partition->insettop+GAP);
							fold->append(xf,sig->partition->totalheight-GAP);
						}
					}
				}
			} //cut marks outside

			if (sig->automarks & AUTOMARK_InnerDot) {
				if (!dots) dots=new PathsData;

				 //dots on bottom
				dots->pushEmpty();
				dots->append(x,sig->partition->insetbottom+GAP);
				dots->append(x,sig->partition->insetbottom+GAP*1.001);
				 //dots on top
				dots->pushEmpty();
				dots->append(x,sig->partition->totalheight-sig->partition->insettop-GAP);
				dots->append(x,sig->partition->totalheight-sig->partition->insettop-GAP*1.001);
				
				if (x2>0) {
					if (sig->partition->insetbottom) {
						dots->pushEmpty();
						dots->append(x2,sig->partition->insetbottom+GAP);
						dots->append(x2,sig->partition->insetbottom+GAP*1.001);
					}
					if (sig->partition->insettop) {
						dots->pushEmpty();
						dots->append(x2,sig->partition->totalheight-sig->partition->insettop-GAP);
						dots->append(x2,sig->partition->totalheight-sig->partition->insettop-GAP*1.001);
					}
				}
			}

			//if (signature->automarks&AUTOMARK_InnerDottedLines) {}
		}//loop

		Group *g=NULL;
		if (cut || fold || dots) spread->marks = g = new Group;
		if (g) g->obj_flags|=OBJ_Unselectable;
		if (cut) {
			cut->flags|=SOMEDATA_LOCK_CONTENTS|SOMEDATA_UNSELECTABLE;
			ScreenColor color(0,0,0,0xffff);
			cut->line(MWIDTH,-1,-1,&color);
			g->push(cut);
			cut->dec_count();
			cut->FindBBox();
		}
		if (fold) {
			fold->flags|=SOMEDATA_LOCK_CONTENTS|SOMEDATA_UNSELECTABLE;
			ScreenColor color(0,0,0,0xffff);
			fold->line(MWIDTH,CapButt,JoinMiter,&color);
			fold->linestyle->dotdash=715827882;
			g->push(fold);
			fold->dec_count();
			fold->FindBBox();
		}
		if (dots) {
			dots->flags|=SOMEDATA_LOCK_CONTENTS|SOMEDATA_UNSELECTABLE;
			ScreenColor color(0,0,0,0xffff);
			dots->line(MWIDTH,CapRound,JoinRound,&color);
			g->push(dots);
			dots->dec_count();
			dots->FindBBox();
		}
		if (spread->marks) spread->mask|=SPREAD_PRINTERMARKS;
	} //automarks


	return spread;
}

//! Return SignatureInstance that holds paper spread whichpaper.
/*! There are two paper spreads per physical piece of paper.
 */
SignatureInstance *SignatureImposition::InstanceFromPaper(int whichpaper,
												int *stack, //!< Index of returned stack within stack group
												int *insert, //!< Index of returned insert within stack
												int *sigpaper, //!< index of paper spread within returned instance
												int *pageoffset, //!< Page number offset to first page in instance
												int *inneroffset, //!< Page number offset of pages past middle
												int *groups) //!< Return how many complete stack groups precede containing group
{
	return signatures->InstanceFromPaper(whichpaper,stack,insert,sigpaper,pageoffset,inneroffset,groups);
}


Spread *SignatureImposition::GetLittleSpread(int whichspread)
{
	return PageLayout(whichspread);
}

/*! If sn==NULL, then dup the nearest instance, otherwise insert sn.
 * Add a new stack at position stack, insert.
 * If stack==-1, then insert at front.
 * If stack<-1 or stack>=NumStacks(), add at end.
 * If insert<0 or insert>NumInserts(), add at innermost of that stack.
 *
 * NOTE that sn must NOT have any attached instances. NULL is returned
 * and nothing otherwise done if so.
 *
 * Return reference to the new SignatureInstance or NULL on failure (such as sn
 * having attachments).
 */
SignatureInstance *SignatureImposition::AddStack(int stack, int insert, SignatureInstance *sn)
{
	if (sn) {
		if (sn->next_insert || sn->prev_insert || sn->next_stack || sn->prev_stack) return NULL;
	}

	if (!signatures) {
		if (sn) signatures=sn;
		else signatures=new SignatureInstance();
		NumPages(numdocpages);
		return signatures;
	}

	if (stack==-1) {
		 //add new stack at front, duplicate first
		if (!sn) sn=signatures->duplicateSingle();
		sn->next_stack=signatures;
		signatures->prev_stack=sn;
		signatures=sn;
		NumPages(numdocpages);
		return signatures;
	}

	if (stack<-1 || stack>=NumStacks(-1)) {
		 //add new stack at end, duplicate last
		SignatureInstance *s=signatures;
		while (s->next_stack) s=s->next_stack;
		if (!sn) sn=s->duplicateSingle();
		s->next_stack=sn;
		sn->prev_stack=s;
		NumPages(numdocpages);
		return sn;
	}

	 //else dup and add an insert
	SignatureInstance *s=NULL;
	if (insert<0 || insert>=NumStacks(stack)) {
		 //add at end of stack
		s=GetSignature(stack,NumStacks(stack)-1);
		insert=-1;
	} else {
		 //dup stack at stack,insert, place at that position, 
		s=GetSignature(stack,insert);
	}
	if (!sn) sn=s->duplicateSingle();
	if (insert<0) {
		 //adding at innermost, easier than inserting midstream
		s->next_insert=sn;
		sn->prev_insert=s;
	} else {
		sn->next_insert=s;
		s->prev_insert=sn;
		if (s->next_stack || s->prev_stack) {
			sn->next_stack=s->next_stack;
			sn->prev_stack=s->prev_stack;
			s->next_stack=s->prev_stack=NULL;
		}
	}
	while (s->prev_insert) s=s->prev_insert;
	while (s->prev_stack)  s=s->prev_stack;
	if (s!=signatures) signatures=s;
	NumPages(numdocpages);

	return sn;
}

/*! If stack and insert does not refer to an existing signatureinstance, do nothing and return 1.
 * If there is only one signature instance, do not delete and return 2.
 * Else return 0 for success.
 */
int SignatureImposition::RemoveStack(int stack, int insert)
{
	if (stack<0 || stack>=NumStacks(-1)) return 1;
	if (insert<0 || insert>=NumStacks(stack)) return 1;
	if (!signatures->next_stack && !signatures->next_insert) return 2; //don't delete only one!

	SignatureInstance *si=NULL;
	SignatureInstance *s=GetSignature(stack,insert);

	if (s->prev_insert==NULL) {
		 //we are at a stack head
		if (s->next_insert==NULL) {
			 //remove this stack entirely, only this one instance in stack, no inserts
			if (s->prev_stack) { s->prev_stack->next_stack=s->next_stack; }
			if (s->next_stack) { si=s->next_stack; s->next_stack->prev_stack=s->prev_stack; }
 			s->prev_stack=NULL;
 			s->next_stack=NULL;
		} else {
		 	 //we are at the top of a stack, replace if any inserts...
			s->next_insert->prev_stack=s->prev_stack;
			s->next_insert->next_stack=s->next_stack;
			if (s->prev_stack) { s->prev_stack->next_stack=s->next_insert; s->prev_stack=NULL; }
			if (s->next_stack) { s->next_stack->prev_stack=s->next_insert; s->next_stack=NULL; }
			si=s->next_insert;
			s->next_insert->prev_insert=NULL;
			s->next_insert=NULL;
		}
	} else {
		 //we are at an insert somewhere, just remove from insert chain
		s->prev_insert->next_insert=s->next_insert;
		if (s->next_insert) s->next_insert->prev_insert=s->prev_insert;
		s->next_insert=s->prev_insert=NULL;
	}
	s->dec_count();

	if (si) {
		while (si->prev_insert) si=si->prev_insert;
		while (si->prev_stack)  si=si->prev_stack;
		if (si!=signatures) signatures=si;
	}
	NumPages(numdocpages);

	return 0;
}


//----------------- signatureimposition i/o

void SignatureImposition::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	if (what==-1) {
		Value::dump_out(f,indent,-1,context);
		return;
	}

    Attribute att;
	dump_out_atts(&att,what,context);
    att.dump_out(f,indent);
}

LaxFiles::Attribute *SignatureImposition::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	if (!att) att=new Attribute;

	if (what==-1) {
		//Value::dump_out_atts(att,-1,context);

		att->push("name \"Blah\"","Name of the impostion");
		att->push("description \"blah blah\"", "Description");
		att->push("numpages 5", "Hint of number of document pages the imposition has to cover");
		att->push("showwholecover yes", "yes|no, whether to show front and back of whole thing.");
		Attribute *subatt=att->pushSubAtt("signature", "One or more, defines the actual foldings. If defining an insert, this block is called "
													   "\"insert\" rather than \"signature\".");
		SignatureInstance s;
		s.dump_out_atts(subatt,-1,context);
		return att;
	}

	if (name) att->push("name",name);
	if (description) att->push("description",description);
	att->push("numpages",numpages);
	att->push("showwholecover",showwholecover?"yes":"no");
	Attribute *aa;
	SignatureInstance *sig=signatures;
	while (sig) {
		aa=sig->dump_out_atts(NULL,what,context);
		makestr(aa->name,"signature");
		att->push(aa,-1);
		sig=sig->next_stack;
	}

	return att;
}


void SignatureImposition::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	char *name,*value;
	int nump=-1;

	if (signatures) { delete signatures; signatures=NULL; }

	for (int c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"name")) {
			makestr(name,value);

		} else if (!strcmp(name,"description")) {
			makestr(description,value);

		} else if (!strcmp(name,"numpages")) {
			IntAttribute(value, &nump);

		} else if (!strcmp(name,"showwholecover")) {
			showwholecover=BooleanAttribute(value);

		} else if (!strcmp(name,"signature")) {
			SignatureInstance *sig=new SignatureInstance;
			sig->dump_in_atts(att->attributes.e[c],flag,context);
			if (!signatures) signatures=sig;
			else signatures->AddStack(sig);

		}
	}
}

//------------------------------- ObjectDefs


/*! Always makes new one, does not consult stylemanager.
 */
ObjectDef *makeSignatureImpositionObjectDef()
{
	ObjectDef *sd=stylemanager.FindDef("SignatureImposition");
	if (sd) return sd;

	sd=new ObjectDef(NULL,"SignatureImposition",
			_("SignatureImposition"),
			_("Imposition based on signatures"),
			"class",
			NULL,NULL, //range, default value
			NULL, //fields
			0, //new flags
			NULL, //newfunc
			NULL /*newSignatureImposition*/);

	sd->push("name", _("Name"), _("Name of the imposition"),
			"string",
			NULL, //range
			"0",  //defvalue
			0,    //flags
			NULL);//newfunc

	sd->push("description", _("Description"), _("Brief, one line description of the imposition"),
			"string",
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc

	sd->push("showwholecover", _("Show whole cover"), _("Whether to let the front cover bleed over onto the back cover"),
			"boolean",
			NULL,
			"0",
			0,
			NULL);

	sd->push("signatures", _("Signatures"), _("The signature stack"),
			"SignatureInstance",
			NULL,
			"0",
			0,
			NULL);


	return sd;
}





int createSignatureImposition(ValueHash *context, ValueHash *parameters,
					   Value **value_ret, ErrorLog &log)
{
	if (!parameters || !parameters->n()) {
		if (value_ret) *value_ret=new SignatureImposition;
		return 0;
	}

	SignatureImposition *imp=new SignatureImposition;

	char error[100];
	int err=0;
	
	try {
		int i, e;
		const char *s;

		 //---name
		s=parameters->findString("name",-1,&e);
		if (e==0) makestr(imp->name,s);
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"name"); throw error; }

		 //---description
		s=parameters->findString("description",-1,&e);
		if (e==0) makestr(imp->description,s);
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"description"); throw error; }

		 //---showwholecover
		i=parameters->findBoolean("showwholecover",-1,&e);
		if (e==0) imp->showwholecover=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"showwholecover"); throw error; }


	} catch (const char *str) {
		log.AddMessage(str,ERROR_Fail);
		err=1;
	}



	if (value_ret && err==0) {
		*value_ret=imp;
		imp->inc_count();

		//} else {
		//	log.AddMessage(_("Incomplete SignatureImposition definition!"),ERROR_Fail);
		//	err=1;
		//}
	}
	imp->dec_count();

	return err;
}


} // namespace Laidout


