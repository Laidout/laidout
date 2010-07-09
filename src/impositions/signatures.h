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
#ifndef SIGNATURES_H
#define SIGNATURES_H

//#include <lax/anobject.h>
//#include <lax/dump.h>
//#include <lax/refcounted.h>
#include "imposition.h"


//------------------------------------- Fold --------------------------------------

enum FoldDirectionType {
	FOLD_Left_Over_To_Right,
	FOLD_Left_Under_To_Right,
	FOLD_Right_Over_To_Left,
	FOLD_Right_Under_To_Left,
	FOLD_Top_Over_To_Bottom,
	FOLD_Top_Under_To_Bottom,
	FOLD_Bottom_Over_To_Top,
	FOLD_Bottom_Under_To_Top,
};

class Fold
{
 public:
	FoldDirectionType folddirection; //l over to r, l under to r, rol, rul, tob, tub, bot, but
	int whichfold; //index from the left or top side of completely unfolded paper of which fold to act on
	Fold(FoldDirectionType f, int which) { folddirection=f; whichfold=which; }
};



//------------------------------------ Signature -----------------------------------------
class Signature : public Laxkit::anObject, public Laxkit::RefCounted, public LaxFiles::DumpUtility
{
 public:
	PaperStyle *paperbox; //optional

	int sheetsperpattern;
	double insetleft, insetright, insettop, insetbottom;
	double tilegapx, tilegapy;
	double rotation; //in practice, not really sure how this works, 
					 //it is mentioned here and there that minor rotation is sometimes needed
					 //for some printers
	int tilex, tiley; //how many partitions per sheet of paper

	char work_and_turn; //0 for no, 1 work and turn == rotate on axis || to major dim, 2 work and tumble=rot axis || minor dim
	int pilesections; //if tiling, whether to repeat content, or continue content (ignored currently)

	double creep;

	int numhfolds, numvfolds;
	Laxkit::PtrStack<Fold> folds;
	double foldedwidth, foldedheight;

	double trimleft, trimright, trimtop, trimbottom; // number<0 means don't trim that edge (for accordion styles)
	double marginleft, marginright, margintop, marginbottom;

	char updirection; //which direction is up 'l|r|t|b', ie 'l' means points toward the left
	char binding;    //direction to place binding 'l|r|t|b'
	char positivex;  //direction of the positive x axis: 'l|r|t|b'
	char positivey;  //direction of the positive x axis: 'l|r|t|b'

	Signature();
	virtual ~Signature();
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);

	unsigned int Validity();
};


//------------------------------------ SignatureImposition -----------------------------------------
class SignatureImposition : public Imposition
{
 protected:
	Signature *signature;      //folding pattern
	//PaperPartition *partition; //partition to insert folding pattern
 public:
	SignatureImposition();
	virtual ~SignatureImposition();
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};



#endif

