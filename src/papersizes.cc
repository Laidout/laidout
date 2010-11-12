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
// Copyright (C) 2004-2007,2010 by Tom Lechner
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
#include "language.h"
#include "laidout.h"

using namespace LaxFiles;
using namespace Laxkit;


//-------------------------------- GetBuiltinPaperSizes ------------------

//       PAPERSIZE    X inches   Y inches   X cm      Y cm
//       -----------------------------------------------------
const char *BuiltinPaperSizes[58*4]=
	{
		"Letter"   ,"8.5" ,"11"  ,"in",
		"Legal"    ,"8.5" ,"14"  ,"in",
		"Tabloid"  ,"11"  ,"17"  ,"in",
		"A4"       ,"210" ,"297" ,"mm",
		"A3"       ,"297" ,"420" ,"mm",
		"A2"       ,"420" ,"594" ,"mm",
		"A1"       ,"594" ,"841" ,"mm",
		"A0"       ,"841" ,"1189","mm",
		"A5"       ,"148" ,"210" ,"mm",
		"A6"       ,"105" ,"148" ,"mm",
		"A7"       ,"74"  ,"105" ,"mm",
		"A8"       ,"52"  ,"74"  ,"mm",
		"A9"       ,"37"  ,"52"  ,"mm",
		"A10"      ,"26"  ,"37"  ,"mm",
		"B0"       ,"1000","1414","mm",
		"B1"       ,"707" ,"1000","mm",
		"B2"       ,"500" ,"707" ,"mm",
		"B3"       ,"353" ,"500" ,"mm",
		"B4"       ,"250" ,"353" ,"mm",
		"B5"       ,"176" ,"250" ,"mm",
		"B6"       ,"125" ,"176" ,"mm",
		"B7"       ,"88"  ,"125" ,"mm",
		"B8"       ,"62"  ,"88"  ,"mm",
		"B9"       ,"44"  ,"62"  ,"mm",
		"B10"      ,"31"  ,"44"  ,"mm",
		"C0"       ,"917" ,"1297","mm",
		"C1"       ,"648" ,"917" ,"mm",
		"C2"       ,"458" ,"648" ,"mm",
		"C3"       ,"324" ,"458" ,"mm",
		"C4"       ,"229" ,"324" ,"mm",
		"C5"       ,"162" ,"229" ,"mm",
		"C6"       ,"114" ,"162" ,"mm",
		"C7"       ,"81"  ,"114" ,"mm",
		"C8"       ,"57"  ,"81"  ,"mm",
		"C9"       ,"40"  ,"57"  ,"mm",
		"C10"      ,"28"  ,"40"  ,"mm",
		"ArchA"    ,"9"   ,"12"  ,"in",
		"ArchB"    ,"12"  ,"18"  ,"in",
		"ArchC"    ,"18"  ,"24"  ,"in",
		"ArchD"    ,"24"  ,"36"  ,"in",
		"ArchE"    ,"36"  ,"48"  ,"in",
		"Flsa"     ,"8.5" ,"13"  ,"in",
		"Flse"     ,"8.5" ,"13"  ,"in",
		"Index"    ,"3"   ,"5"   ,"in",
		"Executive","7.25","10.5","in",
		"Ledger"   ,"17"  ,"11"  ,"in",
		"Halfletter","5.5","8.5" ,"in",
		"Note"      ,"7.5","10"  ,"in",
		"4:3"       ,"4"  ,"3"   ,"in",
		"16:9"      ,"16" ,"9"   ,"in",
		"640x480"   ,"640","480" ,"px",
		"800x600"   ,"800","600" ,"px",
		"1024x768"  ,"1024","768","px",
		"1280x1024" ,"1280","1024","px",
		"1600x1200" ,"1600","1200","px",
		"Custom"    ,"8.5","11"   ,"in",
		"Whatever"  ,"8.5","11"   ,"in",
		NULL,NULL,NULL,NULL
	};

//! Get a stack of PaperStyles with all the builtin paper sizes.
/*! \ingroup pools
 * If papers is NULL, then a new stack is created, filled and returned, otherwise,
 * the buitlins are pushed onto the given stack.
 * 
 * See src/papersizes.cc for all the built in paper sizes. This includes common
 * US sizes, european A, B, and C sizes, some architectural sizes, and a sampling
 * of common tv and  computer aspect ratio sizes, such as 4:3 or 1024x768.
 *
 * The "Custom" and "Whatever" sizes are special cases. Custom allows any dimensions,
 * where the others are fixed. Whatever means you only want a big scratch space, and
 * displaying a paper outline (and thus using any imposition at all) is not important.
 *
 * \todo *** add NTSC, HDTV, a "Monitor" setting 72dpi, etc.. This could also imply
 *   splitting dpi to xdpi and ydpi
 * \todo this needs nested organizing
 * \todo is "whatever" useful anymore with the no-doc option in ViewWindow?
 */
PtrStack<PaperStyle> *GetBuiltinPaperSizes(PtrStack<PaperStyle> *papers)
{
	if (papers==NULL) papers=new PtrStack<PaperStyle>;
	double x,y; 
	double dpi;
	for (int c=0; BuiltinPaperSizes[c]; c+=4) {
		 // x,y were in inches
		x=atof(BuiltinPaperSizes[c+1]);
		y=atof(BuiltinPaperSizes[c+2]);

		if (!strcmp(BuiltinPaperSizes[c+3],"px")) dpi=1;  else dpi=360;

		papers->push(new PaperStyle(BuiltinPaperSizes[c],x,y,0,dpi,BuiltinPaperSizes[c+3]));
	}
	return papers;
}
	
//---------------------------------- PaperStyle --------------------------------

/*! \class PaperStyle
 * \brief A simple class to hold the dimensions, orientation, default dpi, and name of a piece of paper.
 *
 * For instance, a "Letter" paper is 8.5 x 11 inches, usually portrait.
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
/*! \var char *PaperStyle::defaultunits
 * \brief Hint for what should be default units for this paper type.
 * 
 * The width and height are still in inches. This is just a hint.
 */

PaperStyle::PaperStyle()
{
	name=NULL;
	width=height=0;
	dpi=360;
	flags=0;
	defaultunits=newstr("in");

	DBG cerr <<"blank PaperStyle created, obj "<<object_id<<endl;
}

//! Simple constructor, sets name, w, h, flags, dpi.
/*! w and h are in units. They are converted to inches internally,
 * and PaperStyle::defaultunits is only a hint.
 */
PaperStyle::PaperStyle(const char *nname,double w,double h,unsigned int nflags,double ndpi,const char *units)
{
	if (nname) {
		name=new char[strlen(nname)+1];
		strcpy(name,nname);
	} else name=NULL;
	width=w;
	height=h;
	dpi=ndpi;
	flags=nflags;
	defaultunits=newstr(units);
	if (!defaultunits) defaultunits=newstr("in");

	if (!strcmp(defaultunits,"mm")) { width/=25.4; height/=25.4; }
	else if (!strcmp(defaultunits,"cm")) { width/=2.54; height/=2.54; }
	else if (!strcmp(defaultunits,"pt")) { width/=72; height/=72; }

	DBG cerr <<"PaperStyle created, obj "<<object_id<<endl;
}

PaperStyle::~PaperStyle()
{
	if (name) delete[] name;
	if (defaultunits) delete[] defaultunits;

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
		fprintf(f,"%sname Letter     #the name of the paper\n",spc);
		fprintf(f,"%swidth 8.5       #in the default units\n",spc); 
		fprintf(f,"%sheight 11       #in the default units\n",spc);
		fprintf(f,"%sdpi 360         #default dpi for the paper\n",spc);
		fprintf(f,"%slandscape       #could be portrait (the default) instead\n",spc);
		fprintf(f,"%sunits in        #(optional) When reading in, width and height are converted from this\n",spc);
		return;
	}
	if (name) fprintf(f,"%sname %s\n",spc,name);
	double scale=1;
	if (defaultunits) {
		fprintf(f,"%sunits %s\n",spc,defaultunits);
		if (!strcmp(defaultunits,"mm")) scale=25.4;
		else if (!strcmp(defaultunits,"cm")) scale=2.54;
		else if (!strcmp(defaultunits,"pt")) scale=72;
	}
	fprintf(f,"%swidth %.10g\n",spc,width*scale); 
	fprintf(f,"%sheight %.10g\n",spc,height*scale);
	fprintf(f,"%sdpi %.10g\n",spc,dpi);
	fprintf(f,"%s%s\n",spc,(flags&1?"landscape":"portrait"));
}

//! Basically reverse of dump_out.
void PaperStyle::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;
	char *aname,*value;
	const char *convertunits=NULL;
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
			DoubleAttribute(value,&dpi);
		} else if (!strcmp(aname,"landscape")) {
			flags|=1;//*** make this a define?
		} else if (!strcmp(aname,"portrait")) {
			flags&=~1;
		} else if (!strcmp(aname,"defaultunits")) {
			makestr(defaultunits,value);
		} else if (!strcmp(aname,"units")) {
			convertunits=value;
		}
	}
	if (convertunits) {
		 // *** someday automate this adequately
		makestr(defaultunits,convertunits);
		if (!strcasecmp(convertunits,"cm")) { width/=2.54; height/=2.54; }
		else if (!strcasecmp(convertunits,"mm")) { width/=25.4; height/=25.4; }
		else if (!strcasecmp(convertunits,"pt")) { width/=72; height/=72; }
		//nothing special done for in or px
	}
}

//! Copy over name, width, height, dpi.
Style *PaperStyle::duplicate(Style *s)//s==NULL
{
	if (s==NULL) s=(Style *)new PaperStyle();

	if (!dynamic_cast<PaperStyle *>(s)) return NULL;
	PaperStyle *ps=dynamic_cast<PaperStyle *>(s);
	if (!ps) return NULL;
	makestr(ps->name,name);
	ps->width=width;
	ps->height=height;
	makestr(ps->defaultunits,defaultunits);
	ps->landscape(landscape());
	return s;
}

Style *NewPaperStyle(StyleDef *def)
{ return new PaperStyle; }

StyleDef *PaperStyle::makeStyleDef()
{ return makePaperStyleDef(); }

/*! If the paper name is in laidout->papersizes, then grab a duplicate of that,
 * and overwrite any differing settings.
 *
 * \todo if making a copy of a system known paper size, should restrict the w and h
 *   to the actual values for paper, or auto change the paper name for consistency
 */
int createPaperStyle(ValueHash *context, ValueHash *parameters, Value **value_ret, char **error_ret)
{
	if (!parameters) {
		if (value_ret) *value_ret=NULL;
		if (error_ret) appendline(*error_ret,_("Easy for you to say!"));
		return 1;
	}


	PaperStyle *paper=NULL;
	int err=0;
	try {
		int i=0;

		 //---name
		Value *v=parameters->find("paper");
		if (v) {
			if (v->type()==VALUE_String) {
				const char *str=(const char *)dynamic_cast<StringValue*>(v)->str;
				if (!str) throw _("Invalid name for paper!");
				for (int c=0; c<laidout->papersizes.n; c++) {
					if (strcasecmp(str,laidout->papersizes.e[c]->name)==0) {
						paper=(PaperStyle*)laidout->papersizes.e[c]->duplicate();
						break;
					}
				}
				if (!paper) paper=new PaperStyle(str,0,0,0,0,"in");
			} else throw  _("Invalid object for paper!");
		}


		 //----orientation
		int orientation=parameters->findInt("orientation",-1,&i);
		if (i==2) throw _("Invalid format for orientation!");
		if (i==0) {
			if (orientation==0) paper->flags&=~1;
			else paper->flags|=1;
		}

		 //---width
		double d=parameters->findIntOrDouble("width",-1,&i);
		if (i==0) {
			if (d<=0) throw _("Invalid width parameter!");
			paper->width=d;
		} else if (i==2) throw _("Invalid width parameter!");

		 //---height
		d=parameters->findIntOrDouble("height",-1,&i);
		if (i==0) {
			if (d<=0) throw _("Invalid height parameter!");
			paper->height=d;
		} else if (i==2) throw _("Invalid height parameter!");

		 //---dpi
		d=parameters->findIntOrDouble("dpi",-1,&i);
		if (i==0) {
			if (d<=0) throw _("Invalid dpi parameter!");
			paper->dpi=d;
		} else if (i==2) throw _("Invalid dpi parameter!");


	} catch (const char *str) {
		if (error_ret) appendline(*error_ret,str);
		if (paper) { paper->dec_count(); paper=NULL; }
		err=1;
	}


	if (err==0 && value_ret) {
		if (paper) *value_ret=new ObjectValue(paper);
		else *value_ret=NULL;
	}
	if (paper) paper->dec_count();

	return err;
}

StyleDef *makePaperStyleDef()
{
	StyleDef *sd=new StyleDef(NULL,
							  "Paper",
							  _("Paper"),
							  _("A basic rectangular paper with orientation"),
							  Element_Fields,
							  NULL, //range
							  NULL, //defval
							  NULL,0, //fields, flags
							  NewPaperStyle,
							  createPaperStyle);

	//int StyleDef::push(name,Name,ttip,ndesc,format,range,val,flags,newfunc,stylefunc);
	sd->push("name",
			_("Name"),
			_("Name of the paper, like A4 or Letter"),
			Element_String,
			NULL, //range
			NULL, //def value
			0,
			NULL);
	sd->push("orientation",
			_("Orientation"),
			_("Either portrait (0) or landscape (1)"),
			Element_Enum, NULL,"portrait",
			0,
			NULL);
	sd->push("width",
			_("Width"),
			_("Width of the paper, after orientation is applied"),
			Element_Real, NULL,NULL,
			0,
			NULL);
	sd->push("height",
			_("Height"),
			_("Height of the paper, after orientation is applied"),
			Element_Real, NULL,NULL,
			0,
			NULL);
	sd->push("dpi",
			_("Dpi"),
			_("Default dots per inch of the paper"),
			Element_Real, 
			NULL,
			NULL,
			0,
			NULL);
	return sd;
}


//------------------------------------- PaperBox --------------------------------------

/*! \class PaperBox 
 * \brief Wrapper around a PaperStyle, for use in a PaperInterface.
 *
 * PaperBox contains rectangles to define areas for media, printable, bleed, trim, crop, art.
 * These are typically based on the initial PaperStyle (PaperBox::paperstyle).
 *
 * These descriptive boxes can be transformed via a PaperBoxData, to be part of a PaperGroup.
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
 * \brief Somedata Wrapper around a PaperBox, for use in a PaperInterface.
 *
 * This lets all the various boxes of a PaperBox be transformed as part of a PaperGroup.
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

/*! \class PaperGroup
 * \brief An orientation of one or more papers to hold layouts.
 *
 * A PaperGroup holds a collection of PaperBoxData objects. Each PaperBoxData is
 * a wrapper around a PaperBox, which defines various rectangles for 
 * media, printable, bleed, trim, crop, and art boxes. These boxes are typically based on a base
 * PaperStyle object, which defines dimensions, orientation, default dpi, and a name for a particular
 * size of paper, such as Letter or A4.
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
		//fprintf(f,"%s  outlinecolor 65535 0 0 #color of the outline of a paper in the interface\n",spc);
		PaperStyle paperstyle(NULL,0,0,0,0,"in");
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
	
	const double *m;
	for (int c=0; c<papers.n; c++) {
		fprintf(f,"%spaper\n",spc);
		m=papers.e[c]->m();
		fprintf(f,"%s  matrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
			spc, m[0],m[1],m[2],m[3],m[4],m[5]);
		fprintf(f,"%s  outlinecolor %d %d %d\n",
			spc, papers.e[c]->red, papers.e[c]->green, papers.e[c]->blue);
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
			int foundcolor=0;
			PaperStyle *paperstyle=new PaperStyle(NULL,0,0,0,0,"in");
			paperstyle->dump_in_atts(att->attributes.e[c],flag,context);
			PaperBox *paperbox=new PaperBox(paperstyle);
			paperstyle->dec_count();
			PaperBoxData *boxdata=new PaperBoxData(paperbox);
			paperbox->dec_count();
			boxdata->dump_in_atts(att->attributes.e[c],flag,context);
			if (!foundcolor) {
				boxdata->red=65535;
				boxdata->green=0;
				boxdata->blue=65535;
			}
			papers.push(boxdata);
			boxdata->dec_count();
		}
	}
}

