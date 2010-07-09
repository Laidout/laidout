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

#include <lax/lists.cc>


//------------------------------------- Fold --------------------------------------

/*! \enum FoldDirection
 * \brief Used in aFold.
 */
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

Signature::Signature()
{
	paperbox=NULL;

	sheetsperpattern=1;
	insetleft=insetright=insettop=insetbottom=0;
	tilegapx=tilegapy=0;
	rotation=0;

	tilex=tiley=1;

	work_and_turn=0;
	pilesections=0;
	creep=0;

	numhfolds=numvfolds=0;
	foldedwidth=foldedheight=1;
	trimleft=trimright=trimtop=trimbottom=-1;
	marginleft=marginright=margintop=marginbottom=0;

	updirection='t'; //which direction is up 'l|r|t|b', ie 'l' means points toward the left
	binding='l';    //direction to place binding 'l|r|t|b'
	positivex='r';  //direction of the positive x axis: 'l|r|t|b'
	positivey='u';  //direction of the positive x axis: 'l|r|t|b'
}

Signature::~Signature()
{
}

void Signature::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{ // ***
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		return;
	}
}

void Signature::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{ // ***
}

//! Ensure that the Signature's values are actually sane.
unsigned int Signature::Validity()
{
	return 0;
}

//------------------------------------- SignatureImposition --------------------------------------
/*! \class SignatureImposition
 * \brief Imposition based on rectangular folded paper.
 */

SignatureImposition::SignatureImposition()
	: Imposition("Signature")
{
	signature=NULL;
	//partition=NULL;
}

SignatureImposition::~SignatureImposition()
{
	if (signature) signature->dec_count();
	//if (partition) partition->dec_count();
}

void SignatureImposition::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{ // ***
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		return;
	}
}

void SignatureImposition::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{ // ***
}





