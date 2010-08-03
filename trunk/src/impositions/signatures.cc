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

#include <lax/lists.cc>


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


//------------------------------------- Signature --------------------------------------
/*! \class Signature
 * \brief A folding pattern used as the basis for an SignatureImposition. 
 *
 * The steps for creating a signature are:
 *
 * 1. Initial paper inset, with optional gap between the sections\n
 * 2. Sectioning, into (for now) equal sized sections (see tilex and tiley). The sections
 *    can be stacked onto each other (pilesections==1) or each section will use the
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

	numhfolds=numvfolds=0;
	foldedwidth=foldedheight=1;
	trimleft=trimright=trimtop=trimbottom=0;
	marginleft=marginright=margintop=marginbottom=0;

	up='t';         //which direction is up 'l|r|t|b', ie 'l' means points toward the left
	binding='l';    //direction to place binding 'l|r|t|b'
	positivex='r';  //direction of the positive x axis: 'l|r|t|b'
	positivey='u';  //direction of the positive x axis: 'l|r|t|b'
}

Signature::~Signature()
{
	if (paperbox) paperbox->dec_count();
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

//! Convert 't','b','l','r' to top,bottom,left,right.
const char *CtoStr(char c)
{
	if (c=='t') return "Top";
	if (c=='b') return "Bottom";
	if (c=='l') return "Left";
	if (c=='r') return "Right";
	return NULL;
}

void Signature::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{ // ***
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%ssheetspersignature 1  #The number of sheets of paper to stack before\n",spc);
		fprintf(f,"%s                      #applying inset or folding\n",spc);
		fprintf(f,"%sinsettop    0  #How much to trim off the top of paper before partitioning for folds\n",spc);
		fprintf(f,"%sinsetbottom 0  #How much to trim off the bottom of paper before partitioning for folds\n",spc);
		fprintf(f,"%sinsetleft   0  #How much to trim off the left of paper before partitioning for folds\n",spc);
		fprintf(f,"%sinsetright  0  #How much to trim off the right of paper before partitioning for folds\n",spc);
		fprintf(f,"%stilegapx 0     #How much space to put between folding areas horizontally\n",spc);
		fprintf(f,"%stilegapy 0     #How much space to put between folding areas vertically\n",spc);
		fprintf(f,"%stilex 1        #The number of folding sections horizontally to divide a piece of paper\n",spc);
		fprintf(f,"%stiley 1        #The number of folding sections vertically to divide a piece of paper\n",spc);
		fprintf(f,"\n");
		fprintf(f,"%snumhfolds 0    #The number of horizontal fold lines of a folding pattern\n",spc);
		fprintf(f,"%snumvfolds 0    #The number of vertical fold lines of a folding pattern\n",spc);
		fprintf(f,"%sfold           #There will be numhfolds+numvfolds fold blocks. When reading in, the number\n",spc);
		fprintf(f,"%s               #of these blocks will override values of numhfolds and numvfolds.\n",spc);
		fprintf(f,"%s  index        #which horizontal or vertical fold, counting from the left or the top\n",spc);
		fprintf(f,"%s  direction right  #Can be right, left, top, bottom, under right, under left,\n",spc);
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
		fprintf(f,"%sfold #%d\n",spc,c);
		fprintf(f,"%s  index %d\n",spc,folds.e[c]->whichfold);
		fprintf(f,"%s  direction %s\n",spc,FoldDirectionName(folds.e[c]->direction,0));
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
{ // ***
}

//! Ensure that the Signature's values are actually sane.
unsigned int Signature::Validity()
{
	 //inset, gap, trim and margin values all must be within the proper boundaries
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
/*! This is PatternHeight()/(numhfolds+1).
 */
double Signature::PageHeight()
{
	return PatternHeight()/(numhfolds+1);
}

//! The width of a single element of a folding section.
/*! This is PatternWidth()/(numvfolds+1).
 */
double Signature::PageWidth()
{
	return PatternWidth()/(numvfolds+1);
}

//! Return whether the resulting book should be considering folding vertically like a calendar, or horizontally like a book.
/*! If up and binding are both top or bottom, then return 1, else 0.
 */
int Signature::IsVertical()
{
	return (up=='t' || up=='b') && (binding=='t' || binding=='b');
}

//------------------------------------- SignatureImposition --------------------------------------
/*! \class SignatureImposition
 * \brief Imposition based on rectangular folded paper.
 */
/*! \var int Signature::autoaddsheets
 * \brief Whether to stack up more sheets of paper in a signature when adding pages.
 *
 * If nonzero, then any increase will only use more stacked sheets per signature. Otherwise, 
 * more signatures are used, and it is assumed that these will be bound back to back.
 */
/*! \var int Signature::showwholecover
 * \brief Whether to show the front and back pages together or not.
 */

SignatureImposition::SignatureImposition()
	: Imposition("Signature")
{
	showwholecover=0;
	autoaddsheets=0;

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

void SignatureImposition::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{ // ***
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%sshowwholecover no #Whether to show the front and back cover together or not\n",spc);
		signature->dump_out(f,indent,-1,NULL);
		return;
	}
	fprintf(f,"%sshowwholecover %s\n",spc,showwholecover?"yes":"no");
	signature->dump_out(f,indent,what,context);
}

void SignatureImposition::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{ // ***
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
	int left=((page+1)/2)*2-1+showwholecover;
	if (left==page && signature->IsVertical()) return 0; //top
	if (left==page) return 0; //left
	if (signature->IsVertical()) return 1; //bottom
	return 1; //right
}

int SignatureImposition::SpreadType(int spread)
{ // ***
	return 0;
}





