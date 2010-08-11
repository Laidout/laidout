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

#include "signatures.h"
#include "../language.h"

#include <lax/attributes.h>
#include <lax/lists.cc>


using namespace LaxFiles;


//------------------------------------- Fold --------------------------------------

/*! \enum FoldDirection
 * \brief Used in aFold.
 */

//! Return the name of the fold direction.
/*! This will be "Right" or "Under Left" or some such.
 *
 * If translate!=0, then return that translated, else return english.
 */
const char *FoldDirectionName(FoldDirectionType dir, int translate)
{
	const char *str=NULL;
	if (dir==FOLD_Left_Over_To_Right)       str="Right";
	else if (dir==FOLD_Left_Under_To_Right) str="Under Right";
	else if (dir==FOLD_Right_Over_To_Left)  str="Left";
	else if (dir==FOLD_Right_Under_To_Left) str="Under Left";
	else if (dir==FOLD_Top_Over_To_Bottom)  str="Bottom";
	else if (dir==FOLD_Top_Under_To_Bottom) str="Under Bottom";
	else if (dir==FOLD_Bottom_Over_To_Top)  str="Top";
	else if (dir==FOLD_Bottom_Under_To_Top) str="Under Top";

	return translate?_(str):str;
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
	if (c=='t') return "Top";
	if (c=='b') return "Bottom";
	if (c=='l') return "Left";
	if (c=='r') return "Right";
	return NULL;
}

/*! \class Fold
 * \brief Line description node in a Signature
 *
 * Each line can be folds or cuts. Cuts can be tiling cuts or finishing cuts.
 */
/*! \var FoldDirectionType Fold::folddirection
 * \brief l over to r, l under to r, rol, rul, tob, tub, bot
 */
/*! \var int FoldDirection::whichfold
 * \brief Index from the left or top side of completely unfolded paper of which fold to act on
 */


//------------------------------------- FoldedPageInfo --------------------------------------
/*! \class FoldedPageInfo
 * \brief Info about partial folded states of signatures.
 */

FoldedPageInfo::FoldedPageInfo()
{
	x_flipped=y_flipped=0;
	currentrow=currentcol=0;

	finalxflip=finalyflip=0;
	finalindexback=finalindexfront=-1;
}


//------------------------------------- Signature --------------------------------------
/*! \class Signature
 * \brief A folding pattern used as the basis for an SignatureImposition. 
 *
 * The steps for creating a signature are:
 *
 * 1. Initial paper inset, with optional gap between the sections\n
 * 2. Sectioning, into (for now) equal sized sections (see tilex and tiley). The sections
 *    can be stacked onto each other (pilesections==1 (unimplemented!)) or each section will use the
 *    same content (pilesections==0).
 * 3. folding per section\n
 * 4. finishing trim on the folded page size
 * 5. specify which edge of the folded up paper to use as the binding edge, if any\n
 * 6. specify a default margin area
 *
 * If Signature::paper is not NULL, then measurements such as margins are
 * absolute units of the paper. Otherwise measurements are in the range [0..1].*** have that as option,
 * not mandatory...
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
/*! \var double Signature::insetleft
 * \brief An initial left margin to chop off a paper before sectioning and folding.
 */
/*! \var double Signature::insetright
 * \brief An initial right margin to chop off a paper before sectioning and folding.
 */
/*! \var double Signature::insettop
 * \brief An initial top margin to chop off a paper before sectioning and folding.
 */
/*! \var double Signature::insetbottom
 * \brief An initial bottom margin to chop off a paper before sectioning and folding.
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
/*! \var int Signature::tilex
 * \brief After cutting off an inset, the number of horizontal sections to divide a paper.
 */
/*! \var int Signature::tiley
 * \brief After cutting off an inset, the number of vertical sections to divide a paper.
 */
/*! \var double Signature::creep
 * \brief Creep that occurs when folding.
 *
 * This will be approximately pi*(page thickness)/2.
 *
 * \todo this value is currently ignored, and should be thought out more to have a value
 *    that is easier to find in the real world without calipers
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
 * These keeps track of the actual number of sheets per signature. For arrangements where
 * one adds sheets to the same signature when more pages are added (ie saddle stitched booklets),
 * then autoaddsheets will be 1.
 */
/*! \var int Signature::autoaddsheets
 * \brief Whether to stack up more sheets of paper in a signature when adding pages.
 *
 * If nonzero, then any increase will only use more stacked sheets per signature. Otherwise, 
 * more signatures are used, and it is assumed that these will be bound back to back.
 */

Signature::Signature()
{
	paperbox=NULL;
	totalwidth=totalheight=1;

	sheetspersignature=1;
	insetleft=insetright=insettop=insetbottom=0;
	tilegapx=tilegapy=0;

	tilex=tiley=1;

	rotation=0; //***these 4 are unimplemented
	work_and_turn=0;
	pilesections=0;
	creep=0;
	autoaddsheets=0;

	numhfolds=numvfolds=0;
	trimleft=trimright=trimtop=trimbottom=0;
	marginleft=marginright=margintop=marginbottom=0;

	up='t';         //which direction is up 'l|r|t|b', ie 'l' means points toward the left
	binding='l';    //direction to place binding 'l|r|t|b'
	positivex='r';  //direction of the positive x axis: 'l|r|t|b'
	positivey='u';  //direction of the positive x axis: 'l|r|t|b', for when up might not be positivey!

	foldinfo=NULL;
}

Signature::~Signature()
{
	if (paperbox) paperbox->dec_count();

	if (foldinfo) {
		for (int c=0; foldinfo[c]; c++) delete[] foldinfo[c];
		delete[] foldinfo;
	}
}

const Signature &Signature::operator=(const Signature &sig)
{
	if (paperbox) paperbox->dec_count();
	if (sig.paperbox) paperbox=(PaperStyle*)sig.paperbox->duplicate();

	totalwidth=sig.totalwidth;
	totalheight=sig.totalheight;

	sheetspersignature=sig.sheetspersignature;
	autoaddsheets=sig.autoaddsheets;

	insetleft=sig.insetleft;
	insetright=sig.insetright;
	insettop=sig.insettop;
	insetbottom=sig.insetbottom;

	tilegapx=sig.tilegapx;
	tilegapy=sig.tilegapy;
	tilex=sig.tilex;
	tiley=sig.tiley;

	creep=sig.creep;
	rotation=sig.rotation;
	work_and_turn=sig.work_and_turn;
	pilesections=sig.pilesections;

	numhfolds=sig.numhfolds;
	numvfolds=sig.numvfolds;
	folds.flush();
	for (int c=0; c<sig.folds.n; c++) {
		folds.push(new Fold(sig.folds.e[c]->direction, sig.folds.e[c]->whichfold), 1);
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
	applyFold(NULL,-1,1);

	return *this;
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

/*! If foldlevel==0, then fill with info about when there are no folds.
 * foldlevel==1 means after the 1st fold in the folds stack, etc.
 * foldlevel==-1 means apply all the folds, and apply final page settings.
 *
 * If fromscratch==1, then reset the info, and apply each fold below foldlevel
 * before applying the fold at foldlevel.
 */
int Signature::applyFold(FoldedPageInfo **finfo, int foldlevel, int fromscratch)
{ // ***
	if (!finfo) {
		if (!foldinfo) reallocateFoldinfo();
		finfo=foldinfo;
	}
	cerr <<" **** need to implement Signature::applyFold()"<<endl;
	return 0;
}

//! Set the size of the signature to this paper.
/*! This will duplicate p. The count of p will not change.
 */
int Signature::SetPaper(PaperStyle *p)
{
	if (paperbox) paperbox->dec_count();
	paperbox=NULL;

	if (p) {
		paperbox=(PaperStyle*)p->duplicate();
		totalheight=paperbox->h();
		totalwidth =paperbox->w();
	}
	return 0;
}

void Signature::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{ // ***
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%ssheetspersignature 1  #The number of sheets of paper to stack before\n",spc);
		fprintf(f,"%s                      #applying inset or folding\n",spc);
		fprintf(f,"%sautoaddsheets no      #If no, then more pages means use more signatures.\n",spc);
		fprintf(f,"%s                      #If yes, then add more sheets, and fold all as a single signature.\n",spc);
		fprintf(f,"%sinsettop    0  #How much to trim off the top of paper before partitioning for folds\n",spc);
		fprintf(f,"%sinsetbottom 0  #How much to trim off the bottom of paper before partitioning for folds\n",spc);
		fprintf(f,"%sinsetleft   0  #How much to trim off the left of paper before partitioning for folds\n",spc);
		fprintf(f,"%sinsetright  0  #How much to trim off the right of paper before partitioning for folds\n",spc);
		fprintf(f,"%stilegapx 0     #How much space to put between folding areas horizontally\n",spc);
		fprintf(f,"%stilegapy 0     #How much space to put between folding areas vertically\n",spc);
		fprintf(f,"%stilex 1        #The number of folding sections horizontally to divide a piece of paper\n",spc);
		fprintf(f,"%stiley 1        #The number of folding sections vertically to divide a piece of paper\n",spc);
		fprintf(f,"\n");
		fprintf(f,"%snumhfolds 0       #The number of horizontal fold lines of a folding pattern\n",spc);
		fprintf(f,"%snumvfolds 0        #The number of vertical fold lines of a folding pattern\n",spc);
		fprintf(f,"%sfold 3 Under Left  #There will be numhfolds+numvfolds fold blocks. When reading in, the number\n",spc);
		fprintf(f,"%sfold 2 Top         #of these blocks will override values of numhfolds and numvfolds.\n",spc);
		fprintf(f,"%s                   #1st number is which horizontal or vertical fold, counting from the left or the top\n",spc);
		fprintf(f,"%s                   #The direction Can be right, left, top, bottom, under right, under left,\n",spc);
		fprintf(f,"%s                   #under top, under bottom. The \"under\" values fold in that\n",spc);
		fprintf(f,"%s                   #direction, but the fold is behind as you look at it,\n",spc);
		fprintf(f,"%s                   #rather than the default of over and on top.\n",spc);
		fprintf(f,"\n");
		fprintf(f,"%sbinding left  #left, right, top, or bottom. The side to expect a document to be bound.\n",spc);
		fprintf(f,"%s              #Any trim value for the binding edge will be ignored.\n",spc);
		fprintf(f,"%strimtop    0  #How much to trim off the top of a totally folded section\n",spc);
		fprintf(f,"%strimbottom 0  #How much to trim off the bottom of a totally folded section\n",spc);
		fprintf(f,"%strimleft   0  #How much to trim off the left of a totally folded section\n",spc);
		fprintf(f,"%strimright  0  #How much to trim off the right of a totally folded section\n",spc);
		fprintf(f,"%smargintop    0  #How much of a margin to apply to totally folded pages.\n",spc);
		fprintf(f,"%smarginbottom 0  #Inside and outside margins are automatically kept track of.\n",spc);
		fprintf(f,"%smarginleft   0\n",spc);
		fprintf(f,"%smarginright  0\n",spc);
		fprintf(f,"%sup top          #When displaying pages, this direction should be toward the top of the screen\n",spc);
		fprintf(f,"%spositivex right #(optional) Default is a right handed x axis with the up direction the y axis\n",spc);
		fprintf(f,"%spositivey top   #(optional) Default to the same direction as up\n",spc);
		return;
	}
	fprintf(f,"%ssheetspersignature %d\n",spc,sheetspersignature);
	fprintf(f,"%sautoaddsheets %s\n",spc,autoaddsheets?"yes":"no");

	fprintf(f,"%sinsettop    %.10g\n",spc,insettop);
	fprintf(f,"%sinsetbottom %.10g\n",spc,insetbottom);
	fprintf(f,"%sinsetleft   %.10g\n",spc,insetleft);
	fprintf(f,"%sinsetright  %.10g\n",spc,insetright);

	fprintf(f,"%stilegapx %.10g\n",spc,tilegapx);
	fprintf(f,"%stilegapy %.10g\n",spc,tilegapy);
	fprintf(f,"%stilex %d\n",spc,tilex);
	fprintf(f,"%stiley %d\n",spc,tiley);

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

void Signature::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	char *name,*value;
	int numhf=0, numvf=0;

	for (int c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"sheetspersignature")) {
			IntAttribute(value,&sheetspersignature);

		} else if (!strcmp(name,"autoaddsheets")) {
			autoaddsheets=BooleanAttribute(value);

		} else if (!strcmp(name,"insettop")) {
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
			LRTBAttribute(value,&binding);

		} else if (!strcmp(name,"positivex")) {
			LRTBAttribute(value,&positivex);

		} else if (!strcmp(name,"positivey")) {
			LRTBAttribute(value,&positivey);

		} else if (!strcmp(name,"fold")) {
			char *e=NULL;
			int index=-1;
			FoldDirectionType folddir;
			int under=0;
			char dir=0;
			IntAttribute(value,&index,&e);
			while (*e && isspace(*e)) e++;
			if (!strncasecmp(e,"under ",6)) { under=1; e+=6; }
			LRTBAttribute(e,&dir);
			if (under) {
				if      (dir=='r') { folddir=FOLD_Left_Under_To_Right; numvf++; }
				else if (dir=='l') { folddir=FOLD_Right_Under_To_Left; numvf++; }
				else if (dir=='b') { folddir=FOLD_Top_Under_To_Bottom; numhf++; }
				else if (dir=='t') { folddir=FOLD_Bottom_Under_To_Top; numhf++; }
			} else {
				if      (dir=='l') { folddir=FOLD_Right_Over_To_Left; numvf++; }
				else if (dir=='b') { folddir=FOLD_Top_Over_To_Bottom; numhf++; }
				else if (dir=='t') { folddir=FOLD_Bottom_Over_To_Top; numhf++; }
				else if (dir=='r') { folddir=FOLD_Left_Over_To_Right; numvf++; }
			}
			Fold *newfold=new Fold(folddir,index);
			folds.push(newfold);

		}
	}
	numhfolds=numhf; //note: overrides that stored in atts
	numvfolds=numvf;
}

//! Ensure that the Signature's values are actually sane.
/*! \todo TODO!
 */ 
unsigned int Signature::Validity()
{
	 //inset, gap, trim and margin values all must be within the proper boundaries
	cerr <<" *** need to implement Signature sanity check Signature::Validity()"<<endl;
	return 0;
}

//! Return height of a folding section.
/*! This is the total height minus insettop and bottom, divided by the number of vertical tiles.
 */
double Signature::PatternHeight()
{
	double h=totalheight;
	if (paperbox) h=paperbox->h();
	if (tiley>1) h-=(tiley-1)*tilegapy;
	h-=insettop+insetbottom;
	return h/tiley;
}

//! Return width of a folding section.
/*! This is the total width minus insetleft and right, divided by the number of horizontal tiles.
 */
double Signature::PatternWidth()
{
	double w=totalwidth;
	if (paperbox) w=paperbox->w();
	if (tilex>1) w-=(tilex-1)*tilegapx;
	w-=insetleft+insetright;
	return w/tilex;
}

//! The height of a single element of a folding section.
/*! For part==0, this is PatternHeight()/(numhfolds+1).
 * For part==1, this is like 0, but removing the top and bottom trim.
 */
double Signature::PageHeight(int part)
{
	if (part==0) return PatternHeight()/(numhfolds+1);
	return PatternHeight()/(numhfolds+1) - trimtop - trimbottom;
}

//! The width of a single element of a folding section.
/*! For part==0, this is PatternWidth()/(numvfolds+1).
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
	if (bbox) bbox=new Laxkit::DoubleBBox;

	if (part==0) { //trim box
		bbox->minx=bbox->miny=0;
		bbox->maxx=PageWidth(1);
		bbox->maxy=PageHeight(1);

	} else if (part==1) { //margin box
		bbox->minx=marginleft-trimleft;
		bbox->miny=marginbottom-trimbottom;
		bbox->maxx=bbox->minx + marginright-marginleft;
		bbox->maxy=bbox->miny + margintop-marginbottom;

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

//! Taking into account sheetspersignature, return the number of pages that can be arranged in one signature.
/*! So say you have a pattern with 2 sheets folded together. Then 2*PagesPerPattern() is returned.
 *
 * It is assumed that sheetspersignature reflects a desired number of sheets in the signature,
 * whether or not autoaddsheets==1.
 *
 * Please note this is only the number for ONE tile.
 */
int Signature::PagesPerSignature()
{
	return (sheetspersignature>0?sheetspersignature:1)*PagesPerPattern();
}


//------------------------------------- SignatureImposition --------------------------------------
/*! \class SignatureImposition
 * \brief Imposition based on rectangular folded paper.
 */
/*! \var int Signature::showwholecover
 * \brief Whether to show the front and back pages together or not.
 */


SignatureImposition::SignatureImposition()
	: Imposition("Signature")
{
	showwholecover=0;

	papersize=NULL;
	signature=NULL;
	//partition=NULL;
}

SignatureImposition::~SignatureImposition()
{
	if (papersize) papersize->dec_count();
	if (signature) signature->dec_count();
	//if (partition) partition->dec_count();
}

Style *SignatureImposition::duplicate(Style *s)
{// ***
	SignatureImposition *sn;
	if (s==NULL) {
		s=sn=new SignatureImposition();
	} else sn=dynamic_cast<SignatureImposition *>(s);
	if (!sn) return NULL;
	if (styledef) {
		styledef->inc_count();
		if (sn->styledef) sn->styledef->dec_count();
		sn->styledef=styledef;
	}

	sn->signature=signature;

	return Imposition::duplicate(s);  
}

//! The newfunc for Singles instances.
Style *NewSignature(StyleDef *def)
{ 
	SignatureImposition *s=new SignatureImposition;
	s->styledef=def;
	return s;
}

StyleDef *SignatureImposition::makeStyleDef()
{
	StyleDef *sd=new StyleDef(NULL,"Signature",
			_("Signature"),
			_("Imposition based on signatures"),
			Element_Fields,
			NULL,NULL,
			NULL,
			0, //new flags
			NewSignature);

	sd->push("name",
			_("Name"),
			_("Name of the imposition"),
			Element_String,
			NULL, //range
			"0",  //defvalue
			0,    //flags
			NULL);//newfunc

	cerr <<" *** finish implementing SignatureImposition::makeStyleDef()!!"<<endl;

	return sd;
}


void SignatureImposition::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%sshowwholecover no #Whether to let the front cover bleed over onto the back cover\n",spc);
		signature->dump_out(f,indent,-1,NULL);
		return;
	}
	fprintf(f,"%sshowwholecover %s\n",spc,showwholecover?"yes":"no");
	signature->dump_out(f,indent,what,context);
}

void SignatureImposition::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	char *name,*value;

	for (int c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"showwholecover")) {
			showwholecover=BooleanAttribute(value);

		} else if (!strcmp(name,"signature")) {
			signature->dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
}


//! Return 2, for Top/Left or Bottom/Right.
/*! Which of top or left is determined by signature->IsVertical(). Same for bottom/right.
 */
int SignatureImposition::NumPageTypes()
{ return 2; }

//! If signature->IsVertical(), return "Top" or "Bottom" else return "Left" or "Right".
/*! Returns NULL if not a valid pagetype.
 */
const char *SignatureImposition::PageTypeName(int pagetype)
{
	if (pagetype==0) return signature->IsVertical() ? _("Top") : _("Left");
	if (pagetype==1) return signature->IsVertical() ? _("Bottom") : _("Right");
	return NULL;
}

//! Return the page type for the given document page index.
/*! 0 is either top or left. 1 is either bottom or right.
 */
int SignatureImposition::PageType(int page)
{
	//int left=((page+1)/2)*2-1+showwholecover;  <-- *** showwholecover does not map page numbers!!
	int left=((page+1)/2)*2-1;
	if (left==page && signature->IsVertical()) return 0; //top
	if (left==page) return 0; //left
	if (signature->IsVertical()) return 1; //bottom
	return 1; //right
}

//! Return the type of spread that the given spread index is.
int SignatureImposition::SpreadType(int spread)
{ // ***
	return 0;
}


//--------------functions to locate spreads and pages...

//! Return which paper number the given document page lays on.
int SignatureImposition::PaperFromPage(int pagenumber)
{
	 //if a signature is 4x3 cells, each paper holds 24 pages. 12 front, and 12 back.
	 //whether a paper holds a page on the front or back alternates during those 24.
	int whichsig=pagenumber/signature->PagesPerSignature();
	int sigindex=pagenumber%signature->PagesPerSignature();

	//***
	

	return 0;
}

int SignatureImposition::SpreadFromPage(int layout, int pagenumber)
{
	if (layout==SINGLELAYOUT) return pagenumber;
	if (layout==PAPERLAYOUT) return PaperFromPage(pagenumber);
	if (layout==PAGELAYOUT) return (pagenumber+1-showwholecover)/2;
	return 0;
}


//-----------basic setup

int SignatureImposition::GetPagesNeeded(int npapers)
{
	npapers/=signature->sheetspersignature;
	if (npapers==0) npapers=1;
	return npapers * signature->PagesPerSignature();
}

//! Return the number of paper spreads needed to hold that many pages.
/*! Paper back and front count as two different papers.
 *
 * Note this is paper spreads, which is twice the number of physical
 * pieces of paper.
 */
int SignatureImposition::GetPapersNeeded(int npages)
{
	if (signature->autoaddsheets) return 2*(npages/signature->PagesPerPattern() + 1);
	return 2*(npages/signature->PagesPerSignature() + 1);
}

int SignatureImposition::GetSpreadsNeeded(int npages)
{ return (npages)/2+1; } 

int *SignatureImposition::PrintingPapers(int frompage,int topage)
{// ***
	cerr <<" *** note to self: is Imposition::PrintingPapers() needed?"<<endl;
	return NULL;
}


int SignatureImposition::SetPaperSize(PaperStyle *npaper)
{
	Imposition::SetPaperSize(npaper); //sets imposition::paperbox and papergroup
	signature->SetPaper(npaper);
	return 0;
}

int SignatureImposition::SetPaperGroup(PaperGroup *ngroup)
{
	Imposition::SetPaperGroup(ngroup);
	signature->SetPaper(paper->paperstyle);
	return 1;
}

PageStyle *SignatureImposition::GetPageStyle(int pagenum,int local)
{// ***
	return NULL;
}

int SignatureImposition::NumSpreads(int layout)
{// ***
	if (layout==PAPERLAYOUT) ;
	if (layout==PAGELAYOUT || layout==LITTLESPREADLAYOUT) ;
	if (layout==SINGLELAYOUT) return numpages;
	return 0;
}

int SignatureImposition::NumSpreads(int layout,int setthismany)
{// ***
	return 0;
}

int SignatureImposition::NumPapers()
{// ***
	return numpapers;
}

//! Set the number of papers.
/*! Returns the number of papers the imposition thinks it needs.
 * This will not be less than npapers, but might be more.
 */
int SignatureImposition::NumPapers(int npapers)
{// ***
	return 0;
}

int SignatureImposition::NumPages()
{// ***
	return numpages;
}

//! Set the number of document pages that must be fit into the imposition.
/*! Returns the new value of numpages.
 */
int SignatureImposition::NumPages(int npages)
{ //***
	//update the number of papers to accomodate npages
	return 0;
}


//---------------doc pages maintenance
Page **SignatureImposition::CreatePages()
{// ***
	return NULL;
}

int SignatureImposition::SyncPageStyles(Document *doc,int start,int n)
{// ***
	return NULL;
}

	
//! Return outline of page in page coords.
LaxInterfaces::SomeData *SignatureImposition::GetPageOutline(int pagenum,int local)
{// ***
	return NULL;
}

LaxInterfaces::SomeData *SignatureImposition::GetPageMarginOutline(int pagenum,int local)
{// ***
	return NULL;
}


LaxInterfaces::SomeData *SignatureImposition::GetPrinterMarks(int papernum)
{// ***
	return NULL;
}
	
//---------------spread generation
Spread *SignatureImposition::Layout(int layout,int which)
{// ***
	return NULL;
}

//! Return 3 plus the total number of folds.
int SignatureImposition::NumLayoutTypes()
{
	return 3;
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

Spread *SignatureImposition::PageLayout(int whichspread)
{// ***
	return NULL;
}

Spread *SignatureImposition::PaperLayout(int whichpaper)
{// ***
	return NULL;
}

Spread *SignatureImposition::GetLittleSpread(int whichspread)
{
	return PageLayout(whichspread);
}



