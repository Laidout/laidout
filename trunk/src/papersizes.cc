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



//#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>
#define DBG 
using namespace std;

#include "papersizes.h"
#include <lax/strmanip.h>

#include <lax/refptrstack.cc>

using namespace LaxFiles;
using namespace Laxkit;

//-------------------------------- GetBuiltinPaperSizes ------------------

//       PAPERSIZE    X inches   Y inches   X cm      Y cm
//       -----------------------------------------------------
const char *BuiltinPaperSizes[42*3]=
	{
		"Letter","8.5","11",
		"Legal","8.5","14",
		"Tabloid","11","17",
		"Ledger","17","11",
		"Index","3","5",
		"Executive","7.25","10.5",
		"A4","8.26389","11.6944",
		"A3","11.6944","16.5278",
		"A2","16.5278","23.3889",
		"A1","23.3889","33.0556",
		"A0","33.0556","46.7778",
		"A5","5.84722","8.26389",
		"A6","4.125","5.84722",
		"A7","2.91667","4.125",
		"A8","2.05556","2.91667",
		"A9","1.45833","2.05556",
		"A10","1.02778","1.45833",
		"B0","39.3889","55.6667",
		"B1","27.8333","39.3889",
		"B2","19.6944","27.8333",
		"B3","13.9167","19.6944",
		"B4","9.84722","13.9167",
		"B5","6.95833","9.84722",
		"ArchA","9","12",
		"ArchB","12","18",
		"ArchC","18","24",
		"ArchD","24","36",
		"ArchE","36","48",
		"Flsa","8.5","13",
		"Flse","8.5","13",
		"Halfletter","5.5","8.5",
		"Note","7.5","10",
		"4:3", "4","3",
		"16:9", "16","9",
		"640x480","640","480",
		"800x600","800","600",
		"1024x768","1024","768",
		"1280x1024","1280","1024",
		"1600x1200","1600","1200",
		"Custom","8.5","11",
		"Whatever","8.5","11",
		NULL,NULL,NULL
	};

//! Get a stack of PaperStyles with all the builtin paper sizes.
/*! \ingroup pools
 * If papers is NULL, then a new stack is created, filled and returned, otherwise,
 * the buitlins are pushed onto the given stack.
 * 
 * Currently, the builtin sizes (in inches X by Y) are:
 * <pre>
 *		"Letter","8.5","11",
 *		"Legal","8.5","14",
 *		"Tabloid","11","17",
 *		"Ledger","17","11",
 *		"Index","3","5",
 *		"A4","8.26389","11.6944",
 *		"A3","11.6944","16.5278",
 *		"A2","16.5278","23.3889",
 *		"A1","23.3889","33.0556",
 *		"A0","33.0556","46.7778",
 *		"A5","5.84722","8.26389",
 *		"A6","4.125","5.84722",
 *		"A7","2.91667","4.125",
 *		"A8","2.05556","2.91667",
 *		"A9","1.45833","2.05556",
 *		"A10","1.02778","1.45833",
 *		"B0","39.3889","55.6667",
 *		"B1","27.8333","39.3889",
 *		"B2","19.6944","27.8333",
 *		"B3","13.9167","19.6944",
 *		"B4","9.84722","13.9167",
 *		"B5","6.95833","9.84722",
 *		"ArchA","9","12",
 *		"ArchB","12","18",
 *		"ArchC","18","24",
 *		"ArchD","24","36",
 *		"ArchE","36","48",
 *		"Flsa","8.5","13",
 *		"Flse","8.5","13",
 *		"Halfletter","5.5","8.5",
 *		"Note","7.5","10",
 *		"4:3", "4","3",
 *		"16:9", "16","9",
 *		"scr:640x480","640","480",
 *		"scr:800x600","800","600",
 *		"scr:1024x768","1024","768",
 *		"scr:1280x1024","1280","1024",
 *		"scr:1600x1200","1600","1200",
 *		"Custom","-","-",
 *		"Whatever","-","-"
 * </pre>
 *
 * \todo *** add NTSC, HDTV, a "Monitor" setting 72dpi, etc.. This could also imply
 *   splitting dpi to xdpi and ydpi
 * \todo this needs nested organizing
 */
PtrStack<PaperStyle> *GetBuiltinPaperSizes(PtrStack<PaperStyle> *papers)
{
	if (papers==NULL) papers=new PtrStack<PaperStyle>;
	double x,y; 
	int dpi;
	for (int c=0; BuiltinPaperSizes[c]; c+=3) {
		 // x,y were in inches
		x=atof(BuiltinPaperSizes[c+1]);
		y=atof(BuiltinPaperSizes[c+2]);
		if (!strncmp(BuiltinPaperSizes[c],"scr:",4)) dpi=1; else dpi=360;
		papers->push(new PaperStyle(BuiltinPaperSizes[c],x,y,0,dpi));
	}
	return papers;
}
	
//---------------------------------- PaperStyle --------------------------------

/*! \class PaperStyle
 * \brief A simple class to hold the dimensions and orientation of a piece of paper.
 *
 * A number of standard sizes are built in. See GetBuiltinPaperSizes() for a list of them.
 * The only thing in flags is (flags&1) if it is landscape, or !(flags&1) if portrait.
 * Width/height will stay the same, but w() and h() will return the swapped values
 * for landscape.
 *
 * The extra peculiar paper type is "Whatever", which is used when you don't care about paper,
 * and don't want the paper outline drawn. Really this makes Laidout perform in full scratchboard mode.
 */
/*! \var unsigned int PaperStyle::flags
 * \brief flags&&1==landscape, !&&1=portrait
 */
/*! \var double PaperStyle::width
 * \brief The portrait style width of the paper.
 */
/*! \var double PaperStyle::height
 * \brief The portrait style height of the paper.
 */
/*! \fn double PaperStyle::w()
 * \brief If landscape (flags&&1), then return height, else return width.
 */
/*! \fn double PaperStyle::h()
 * \brief If landscape (flags&&1), then return width, else return height.
 */

//! Simple constructor, sets name, w, h, flags, dpi.
PaperStyle::PaperStyle(const char *nname,double w,double h,unsigned int nflags,int ndpi)
{
	if (nname) {
		name=new char[strlen(nname)+1];
		strcpy(name,nname);
	} else name=NULL;
	width=w;
	height=h;
	dpi=ndpi;
	flags=nflags;
	DBG cerr <<"PaperStyle created, obj "<<object_id<<endl;
}

PaperStyle::~PaperStyle()
{
	if (name) delete[] name;
	DBG cerr <<"PaperStyle destroyed, obj "<<object_id<<endl;
}


/*! Dump out like the following. Note that the width and height are for the portrait
 * style. For the adjusted heights based on whether it is landscape, w() and h() return
 * width and height swapped.
 * <pre>
 *   name Letter
 *   width 8.5
 *   height 11
 *   dpi 360
 *   landscape
 * </pre>
 */
void PaperStyle::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%sname Letter  #the name of the paper\n",spc);
		fprintf(f,"%swidth 8.5    #in inches\n",spc); 
		fprintf(f,"%sheight 11    #in inches\n",spc);
		fprintf(f,"%sdpi 360      #default dpi for the paper\n",spc);
		fprintf(f,"%slandscape    #could be portrait (the default) instead\n",spc);
		return;
	}
	if (name) fprintf(f,"%sname %s\n",spc,name);
	fprintf(f,"%swidth %.10g\n",spc,width); 
	fprintf(f,"%sheight %.10g\n",spc,height);
	fprintf(f,"%sdpi %d\n",spc,dpi);
	fprintf(f,"%s%s\n",spc,(flags&1?"landscape":"portrait"));
}

//! Basically reverse of dump_out.
void PaperStyle::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;
	char *aname,*value;
	for (int c=0; c<att->attributes.n; c++) {
		aname= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(aname,"name")) {
			if (value) makestr(name,value);
		} else if (!strcmp(aname,"width")) {
			DoubleAttribute(value,&width);
		} else if (!strcmp(aname,"height")) {
			DoubleAttribute(value,&height);
		} else if (!strcmp(aname,"dpi")) {
			IntAttribute(value,&dpi);
		} else if (!strcmp(aname,"landscape")) {
			flags|=1;//*** make this a define?
		} else if (!strcmp(aname,"portrait")) {
			flags&=~1;
		}
	}
}

//! Copy over name, width, height, dpi.
Style *PaperStyle::duplicate(Style *s)//s==NULL
{
	if (s==NULL) return (Style *)new PaperStyle(name,width,height,flags,dpi);
	if (!dynamic_cast<PaperStyle *>(s)) return NULL;
	PaperStyle *ps=dynamic_cast<PaperStyle *>(s);
	if (!ps) return NULL;
	makestr(ps->name,name);
	ps->width=width;
	ps->height=height;
	return s;
}


//------------------------------------- PaperBox --------------------------------------

/*! \class PaperBox 
 * \brief Wrapper around a paper style, for use in a PaperInterface.
 */
/*! \var Laxkit::DoubleBBox PaperBox::media
 * \brief Normally, this will be the same as paperstyle.
 */
/*! \var Laxkit::DoubleBBox PaperBox::printable
 * \brief Basically, the area of media that a printer can physically print on.
 */

/*! Incs count of paper.
 *
 * Create with media box based on paper. None of the other boxes defined.
 */
PaperBox::PaperBox(PaperStyle *paper)
{
	which=0; //a mask of which boxes are defined
	paperstyle=paper;
	if (paper) {
		paper->inc_count();
		which=MediaBox;
		media.minx=media.miny=0;
		media.maxx=paper->w(); //takes into account paper orientation
		media.maxy=paper->h();
	}
	DBG cerr <<"PaperBox created, obj "<<object_id<<endl;
}

/*! Decs count of paper.
 */
PaperBox::~PaperBox()
{
	if (paperstyle) paperstyle->dec_count();
	DBG cerr <<"PaperBox destroyed, obj "<<object_id<<endl;
}

//! Replace the current paper with the given paper.
/*! Updates media box, but not the other boxes.
 * Incs count of paper, and decs count of old paperstyle.
 *
 * Return 0 for success or nonzero error.
 */
int PaperBox::Set(PaperStyle *paper)
{
	if (!paper) return 1;
	if (paperstyle) paperstyle->dec_count();
	paperstyle=paper;

	paper->inc_count();
	which|=MediaBox;
	media.minx=media.miny=0;
	media.maxx=paper->w(); //takes into account paper orientation
	media.maxy=paper->h();

	return 0;
}

//------------------------------------- PaperBoxData --------------------------------------

/*! \class PaperBoxData
 * \brief Somedata Wrapper around a paper style, for use in a PaperInterface.
 */

/*! Incs count of paper.
 */
PaperBoxData::PaperBoxData(PaperBox *paper)
{
	red=green=0;
	blue=65535;

	box=paper;
	if (box) {
		box->inc_count();
		setbounds(&box->media);
	}
}

PaperBoxData::~PaperBoxData()
{
	if (box) box->dec_count();
}


//------------------------------------- PaperGroup --------------------------------------
	//char *name;
	//char *Name;
	//Laxkit::PtrStack<PaperBoxData> papers;
	//Laxkit::anObject *owner;

/*! \class PaperGroup
 * \brief Holds a collection of PaperBoxData objects.
 */

PaperGroup::PaperGroup()
{
	locked=0;
	name=Name=NULL;
	owner=NULL;

	DBG cerr <<"PaperGroup created, obj "<<object_id<<endl;
}

//! Create a PaperGroup with one paper based on paperstyle, with only media box defined.
/*! This inc_count()'s paperstyle, does not duplicate it.
 */
PaperGroup::PaperGroup(PaperStyle *paperstyle)
{
	locked=0;
	name=Name=NULL;
	owner=NULL;

	PaperBox *box=new PaperBox(paperstyle);
	PaperBoxData *data=new PaperBoxData(box);
	box->dec_count();
	papers.push(data);
	data->dec_count();

	DBG cerr <<"PaperGroup created, obj "<<object_id<<endl;
}

//! Create a PaperGroup with one paper based on boxdata.
/*! This inc_count()'s boxdata, does not duplicate it.
 */
PaperGroup::PaperGroup(PaperBoxData *boxdata)
{
	locked=0;
	name=Name=NULL;
	owner=NULL;

	papers.push(boxdata);

	DBG cerr <<"PaperGroup created, obj "<<object_id<<endl;
}

PaperGroup::~PaperGroup()
{
	if (name) delete[] name;
	if (Name) delete[] Name;

	DBG cerr <<"PaperGroup destroyed, obj "<<object_id<<endl;
}

/*!
 * <pre>
 *   name somename
 *   Name Descriptive name, human readable
 *   paper  #one or more of these
 *     matrix 1 0 0 1 0 0 #optional
 *     name Letter
 *     width 8.5
 *     height 11
 *     portrait #or landscape
 *     dpi 360
 * </pre>
 */
void PaperGroup::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%sname somename         #a string id. no whitespace\n",spc);
		fprintf(f,"%sName Descriptive Name #human readable name\n",spc);
		//if (owner) ***;
		fprintf(f,"%spaper                 #there can be 0 or more paper sections\n",spc);
		fprintf(f,"%s  matrix 1 0 0 1 0 0  #transform for the paper to limbo space\n",spc);
		PaperStyle paperstyle(NULL,0,0,0,0);
		paperstyle.dump_out(f,indent+2,-1,NULL);
		//fprintf(f,"%s  minx 0              #the bounds for the media box\n",spc);
		//fprintf(f,"%s  miny 0\n",spc);
		//fprintf(f,"%s  maxx 8.5\n",spc);
		//fprintf(f,"%s  minx 11\n",spc);
		return;
	}
	if (name) fprintf(f,"%sname %s\n",spc,name);
	if (Name) fprintf(f,"%sName %s\n",spc,Name);
	//if (owner) ***;
	
	double *m;
	for (int c=0; c<papers.n; c++) {
		fprintf(f,"%spaper\n",spc);
		m=papers.e[c]->m();
		fprintf(f,"%s  matrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
			spc, m[0],m[1],m[2],m[3],m[4],m[5]);
		papers.e[c]->box->paperstyle->dump_out(f,indent+2,0,context);
	}
}

void PaperGroup::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;

	char *nme,*value;
	for (int c=0; c<att->attributes.n; c++)  {
		nme=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(nme,"name")) {
			makestr(name,value);
		} else if (!strcmp(nme,"Name")) {
			makestr(Name,value);
		} else if (!strcmp(nme,"paper")) {
			PaperStyle *paperstyle=new PaperStyle(NULL,0,0,0,0);
			paperstyle->dump_in_atts(att->attributes.e[c],flag,context);
			PaperBox *paperbox=new PaperBox(paperstyle);
			paperstyle->dec_count();
			PaperBoxData *boxdata=new PaperBoxData(paperbox);
			paperbox->dec_count();
			boxdata->dump_in_atts(att->attributes.e[c],flag,context);
			papers.push(boxdata);
			boxdata->dec_count();
		}
	}
}

