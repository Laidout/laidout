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
// Copyright (C) 2004-2007,2010 by Tom Lechner
//



#include <cstdlib>
#include <cstring>

#include <iostream>
#define DBG 

#include <lax/strmanip.h>
#include <lax/units.h>
#include <lax/interfaces/interfacemanager.h>

#include "papersizes.h"
#include "stylemanager.h"
#include "../language.h"
#include "../laidout.h"

//template implementation:
//#include <lax/refptrstack.cc>


using namespace std;
using namespace Laxkit;
using namespace LaxInterfaces;



namespace Laidout {

//-------------------------------- GetBuiltinPaperSizes ------------------

//      PAPERSIZE   Width  Height  Units  Category
//      ----------------------------------------
const char *BuiltinPaperSizes[65*5]=
	{
		"Letter"   ,"8.5" ,"11"  ,"in", "US",
		"Legal"    ,"8.5" ,"14"  ,"in", "US",
		"Tabloid"  ,"11"  ,"17"  ,"in", "US",
		"A4"       ,"210" ,"297" ,"mm", "A",
		"A3"       ,"297" ,"420" ,"mm", "A",
		"A2"       ,"420" ,"594" ,"mm", "A",
		"A1"       ,"594" ,"841" ,"mm", "A",
		"A0"       ,"841" ,"1189","mm", "A",
		"A5"       ,"148" ,"210" ,"mm", "A",
		"A6"       ,"105" ,"148" ,"mm", "A",
		"A7"       ,"74"  ,"105" ,"mm", "A",
		"A8"       ,"52"  ,"74"  ,"mm", "A",
		"A9"       ,"37"  ,"52"  ,"mm", "A",
		"A10"      ,"26"  ,"37"  ,"mm", "A",
		"B0"       ,"1000","1414","mm", "B",
		"B1"       ,"707" ,"1000","mm", "B",
		"B2"       ,"500" ,"707" ,"mm", "B",
		"B3"       ,"353" ,"500" ,"mm", "B",
		"B4"       ,"250" ,"353" ,"mm", "B",
		"B5"       ,"176" ,"250" ,"mm", "B",
		"B6"       ,"125" ,"176" ,"mm", "B",
		"B7"       ,"88"  ,"125" ,"mm", "B",
		"B8"       ,"62"  ,"88"  ,"mm", "B",
		"B9"       ,"44"  ,"62"  ,"mm", "B",
		"B10"      ,"31"  ,"44"  ,"mm", "B",
		"C0"       ,"917" ,"1297","mm", "C",
		"C1"       ,"648" ,"917" ,"mm", "C",
		"C2"       ,"458" ,"648" ,"mm", "C",
		"C3"       ,"324" ,"458" ,"mm", "C",
		"C4"       ,"229" ,"324" ,"mm", "C",
		"C5"       ,"162" ,"229" ,"mm", "C",
		"C6"       ,"114" ,"162" ,"mm", "C",
		"C7"       ,"81"  ,"114" ,"mm", "C",
		"C8"       ,"57"  ,"81"  ,"mm", "C",
		"C9"       ,"40"  ,"57"  ,"mm", "C",
		"C10"      ,"28"  ,"40"  ,"mm", "C",
		"ArchA"    ,"9"   ,"12"  ,"in", "Arch",
		"ArchB"    ,"12"  ,"18"  ,"in", "Arch",
		"ArchC"    ,"18"  ,"24"  ,"in", "Arch",
		"ArchD"    ,"24"  ,"36"  ,"in", "Arch",
		"ArchE"    ,"36"  ,"48"  ,"in", "Arch",
		"Flsa"     ,"8.5" ,"13"  ,"in", "US",
		"Flse"     ,"8.5" ,"13"  ,"in", "US",
		"Index"    ,"3"   ,"5"   ,"in", "US",
		"Executive","7.25","10.5","in", "US",
		"Ledger"   ,"17"  ,"11"  ,"in", "US",
		"Halfletter","5.5","8.5" ,"in", "US",
		"Note"      ,"7.5","10"  ,"in", "US",
		"1:1"       ,"1"  ,"1"   ,"in", "Aspect",
		"1:2"       ,"1"  ,"2"   ,"in", "Aspect",
		"4:3"       ,"4"  ,"3"   ,"in", "Aspect",
		"16:9"      ,"16" ,"9"   ,"in", "Aspect",
		"640x480"   ,"640","480" ,      "px", "Screen",
		"800x600"   ,"800","600" ,      "px", "Screen",
		"1024x768"  ,"1024","768",      "px", "Screen",
		"1280x1024" ,"1280","1024",     "px", "Screen",
		"1600x1200" ,"1600","1200",     "px", "Screen",
		"1920x1080" ,"1920","1080",     "px", "Screen",
		"1920x1200" ,"1920","1200",     "px", "Screen",
		"2560x1440 (2k)", "2560","1440","px", "Screen",
		"3840x2160 (4k)", "3840","2160","px", "Screen",
		"7680x4320 (8k)", "7680","4320","px", "Screen",
		"Custom"    ,"8.5","11"   ,"in", "", /* NOTE!!! these two must be last!! */
		"Whatever"  ,"8.5","11"   ,"in", "", /* NOTE!!! these two must be last!! */
		nullptr,nullptr,nullptr,nullptr
	};

//! Get a stack of PaperStyles with all the builtin paper sizes.
/*! \ingroup pools
 * If papers is nullptr, then a new stack is created, filled and returned, otherwise,
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
 * \todo this needs nested organizing
 * \todo is "whatever" useful anymore with the no-doc option in ViewWindow?
 */
PtrStack<PaperStyle> *GetBuiltinPaperSizes(PtrStack<PaperStyle> *papers)
{
	if (papers==nullptr) papers=new PtrStack<PaperStyle>;
	double x,y; 
	double dpi;

	setlocale(LC_ALL,"C"); //because "8.5" in the list above is not the same as "8,5" for some locales
	for (int c=0; BuiltinPaperSizes[c]; c += 5) {
		 // x,y were in inches
		x=atof(BuiltinPaperSizes[c+1]);
		y=atof(BuiltinPaperSizes[c+2]);

		if (!strcmp(BuiltinPaperSizes[c+3],"px")) dpi=1;  else dpi=300;

		papers->push(new PaperStyle(BuiltinPaperSizes[c],x,y,0,dpi,BuiltinPaperSizes[c+3]));
	}
	setlocale(LC_ALL,"");

	return papers;
}

/*! TODO!! USE THIS INSTEAD OF GetBuiltinPaperSizes!!!! */
int InstallPaperResources()
{
	double x,y; 
	double dpi;

	InterfaceManager *imanager = InterfaceManager::GetDefault(true);
    ResourceManager *rm = imanager->GetResourceManager();
    ResourceType *objs = rm->FindType("Papers");

	const char *menu, *name, *units;
	int num_added = 0;

	setlocale(LC_ALL,"C"); //because "8.5" in the list above is not the same as "8,5" for some locales
	for (int c=0; BuiltinPaperSizes[c]; c += 5) {
		name  = BuiltinPaperSizes[c];
		units = BuiltinPaperSizes[c+3];
		menu  = BuiltinPaperSizes[c+4];

		 // x,y were in inches
		x = atof(BuiltinPaperSizes[c+1]);
		y = atof(BuiltinPaperSizes[c+2]);

		if (!strcmp(units,"px")) dpi=1;  else dpi=300;

		PaperStyle *newpaper = new PaperStyle(name, x,y, 0, dpi, units);
		objs->AddResource(newpaper, nullptr, name, name, nullptr, nullptr, nullptr, true, menu);
		num_added++;
	}
	setlocale(LC_ALL,"");

	return num_added;
}

/*! Try to match width and height to a named paper.
 * is_landscape_ret gets 0 if match is same as returned object's orientation,
 * or 1 if opposite (i.e. is landscape mode for that paper).
 *
 * If startfrom != 0, then start the search starting from this index, instead of first paper.
 *
 * If epsilon==0 (the default), w and h must match exactly. Else the values must be at least that close.
 */
PaperStyle *GetNamedPaper(double width, double height, int *is_landscape_ret, int startfrom, int *index_ret, double epsilon)
{
	int match;
	if (startfrom<0) startfrom=0;
	for (int c = startfrom; c < laidout->papersizes.n; c++) {
		match = laidout->papersizes.e[c]->IsMatch(width, height, epsilon);
		if (match) {
			if (is_landscape_ret) *is_landscape_ret = (match == -1);
			if (index_ret) *index_ret = c;
			return laidout->papersizes.e[c];
		}
	}

	return nullptr;
}

PaperStyle *GetPaperFromName(const char *name)
{
	for (int c = 0; laidout->papersizes.n; c++) {
		if (!strcasecmp(name, laidout->papersizes.e[c]->name)) {
			return laidout->papersizes.e[c];
		}
	}
	return nullptr;
}


//---------------------------------- PaperStyle --------------------------------

/*! \class PaperStyle
 * \brief A simple class to hold the dimensions, orientation, default dpi, and name of a piece of paper.
 *
 * For instance, a "Letter" paper is 8.5 x 11 inches, usually portrait.
 *
 * A number of standard sizes are built in. See GetBuiltinPaperSizes() for a list of them.
 * Width/height will stay the same, but w() and h() will return the swapped values
 * for landscape.
 *
 * The extra peculiar paper type is "Whatever", which is used when you don't care about paper,
 * and don't want the paper outline drawn. Really this makes Laidout perform in full scratchboard mode.
 */
/*! \var unsigned bool PaperStyle::is_landscape
 * \brief Whether the paper should be used in landscape mode, with width and height flipped.
 */
/*! \var double PaperStyle::width
 * \brief The portrait style width of the paper.
 */
/*! \var double PaperStyle::height
 * \brief The portrait style height of the paper.
 */
/*! \fn double PaperStyle::w()
 * \brief If landscape, then return height, else return width.
 */
/*! \fn double PaperStyle::h()
 * \brief If landscape, then return width, else return height.
 */
/*! \var char *PaperStyle::defaultunits
 * \brief Hint for what should be default units for this paper type.
 * 
 * The width and height are still in inches. This is just a hint.
 */

/*! nname==nullptr uses laidout's default paper name.
 * Looks up name in laidout's list of papers, and sets data accordingly.
 */
PaperStyle::PaperStyle(const char *nname)
{
	if (!isblank(nname)) name=newstr(nname); else name=nullptr;

	favorite     = false;
	is_landscape = false;
	dpi          = 300;
	width        = 8.5;
	height       = 11;
	defaultunits = newstr("in");

	DBG cerr <<"blank PaperStyle created, obj "<<object_id<<endl;

	if (!name) name = newstr(laidout->prefs.defaultpaper);
	if (!name) name = newstr("letter");
	//if (!name) name=get_system_default_paper(nullptr);

	for (int c=0; c<laidout->papersizes.n; c++) {
		if (strcasecmp(name,laidout->papersizes.e[c]->name)==0) {
			width  = laidout->papersizes.e[c]->width;
			height = laidout->papersizes.e[c]->height;
			is_landscape = laidout->papersizes.e[c]->is_landscape;
			dpi    = laidout->papersizes.e[c]->dpi;
			makestr(defaultunits, laidout->papersizes.e[c]->defaultunits);
			break;
		}
	}
}

//! Simple constructor, sets name, w, h, flags, dpi.
/*! w and h are in units. They are converted to inches internally,
 * and PaperStyle::defaultunits is only a hint.
 */
PaperStyle::PaperStyle(const char *nname,double w,double h,unsigned int nis_landscape,double ndpi,const char *units)
{
	name         = newstr(nname);
	width        = w;
	height       = h;
	dpi          = ndpi;
	is_landscape = nis_landscape;
	defaultunits = newstr(units);
	if (!defaultunits) defaultunits = newstr("in");

	//convert to inches internally
	if      (!strcmp(defaultunits,"mm"))    { width/=25.4; height/=25.4; }
	else if (!strcmp(defaultunits,"cm"))    { width/=2.54; height/=2.54; }
	else if (!strcmp(defaultunits,"pt"))    { width/=72;   height/=72;   }
	else if (!strcmp(defaultunits,"svgpt")) { width/=90;   height/=90;   }
	else if (!strcmp(defaultunits,"px"))    { width/=96;   height/=96;   }

	DBG cerr <<"PaperStyle created, obj "<<object_id<<endl;
}

PaperStyle::~PaperStyle()
{
	if (name) delete[] name;
	if (defaultunits) delete[] defaultunits;

	DBG cerr <<"PaperStyle destroyed, obj "<<object_id<<endl;
}


/*! Return 1 if the paper matches this paper as is, -1 if it matches, but in opposite orientation.
 * 0 for no match.
 *
 * If epsilon==0 (the default), w and h must match exactly. Else the values must be at least that close.
 */
int PaperStyle::IsMatch(double ww, double hh, double epsilon)
{
	if (fabs(w()-ww) <= epsilon && fabs(h()-hh) <= epsilon) return 1;
	if (fabs(h()-ww) <= epsilon && fabs(w()-hh) <= epsilon) return -1;
	return 0;
}

/*! Set from something like "a4", "custom(5in,10in)" or "letter,portrait"
 * Must start with paper name (or "custom"). If not custom,
 * then name can be followed by either "portrait" or "landscape".
 *
 * If custom, it must be of format "custom(width,height)" where width and height
 * are numbers and optional units. No explicit units uses inches.
 */
int PaperStyle::SetFromString(const char *nname)
{
	if (!nname) return 1;

	is_landscape = false;
	if (!strncasecmp(nname,"custom",6)) {
		nname+=6;
		makestr(name,"Custom");

		if (*nname=='(') nname++;
		//if (!isdigit(*nname) && *nname!='.') {  *** maybe have extra name? //custom(name, 5,10)
		//	const char *end=strchr(nname,',');
		//	if (!end) end=strchr(nname,')');
		//	if (!end) makestr(name,nname);
		//	else {
		//		makenstr(name,nname,end-nname);
		//	}
		//}

		char *endptr = nullptr;
		double d = strtod(nname, &endptr);
		if (endptr == nname) return 2;
		width = d;
		nname = endptr;
		while (isspace(*nname)) nname++;

		int units = UNITS_None;
		UnitManager *unitmanager = GetUnitManager();
		if (isalpha(*nname)) {
			//read units
			const char *cendptr = nname;
			while (isalpha(*cendptr)) cendptr++;
			units = unitmanager->UnitId(nname, cendptr - nname);
			if (units == UNITS_None) return 3;
			width = unitmanager->Convert(width, units, UNITS_Inches, UNITS_Length, nullptr);
			makenstr(defaultunits, nname, cendptr - nname);
			nname = cendptr;
		}
		
		if (*nname==',') nname++;

		d = strtod(nname, &endptr);
		if (endptr == nname) return 4;
		height = d;
		nname = endptr;
		if (isalpha(*nname)) {
			//read units
			const char *cendptr = nname;
			while (isalpha(*cendptr)) cendptr++;
			units = unitmanager->UnitId(nname, cendptr - nname);
			if (units == UNITS_None) return 3;
			height = unitmanager->Convert(height, units, UNITS_Inches, UNITS_Length, nullptr);
			makenstr(defaultunits, nname, cendptr - nname);
			nname = cendptr;
		}

		if (*nname==')') nname++;

	} else { 
		for (int c=0; c<laidout->papersizes.n; c++) {
			if (strncasecmp(nname, laidout->papersizes.e[c]->name, strlen(laidout->papersizes.e[c]->name)) == 0) {
				makestr(name, laidout->papersizes.e[c]->name);
				width  = laidout->papersizes.e[c]->width;
				height = laidout->papersizes.e[c]->height;
				dpi    = laidout->papersizes.e[c]->dpi;
				is_landscape = laidout->papersizes.e[c]->is_landscape;
				makestr(defaultunits, laidout->papersizes.e[c]->defaultunits);
				break;
			}
		}
	}

	if (strcasestr(nname, "portrait"))       is_landscape = false;
	else if (strcasestr(nname, "landscape")) is_landscape = true;

	return 0;
}


/*! Dump out like the following. Note that the width and height are for the portrait
 * style. For the adjusted heights based on whether it is landscape, w() and h() return
 * width and height swapped.
 * <pre>
 *   name Letter
 *   width 8.5
 *   height 11
 *   dpi 300
 *   landscape
 * </pre>
 */
void PaperStyle::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%sname Letter     #the name of the paper\n",spc);
		fprintf(f,"%swidth 8.5       #in the default units\n",spc); 
		fprintf(f,"%sheight 11       #in the default units\n",spc);
		fprintf(f,"%sdpi 300         #default dpi for the paper\n",spc);
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
	fprintf(f,"%s%s\n",spc,(is_landscape ? "landscape" : "portrait"));
}

Laxkit::Attribute *PaperStyle::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext)
{
	if (!att) att=new Attribute;

	if (what==-1) {
		Value::dump_out_atts(att,-1,savecontext);
        return att;
	}

	if (name) att->push("name",name);
	double scale=1;
	if (defaultunits) {
		att->push("units",defaultunits);
		if (!strcmp(defaultunits,"mm")) scale=25.4;
		else if (!strcmp(defaultunits,"cm")) scale=2.54;
		else if (!strcmp(defaultunits,"pt")) scale=72;
	}
	att->push("width",width*scale); 
	att->push("height",height*scale);
	att->push("dpi",dpi);
	att->push("orientation",(is_landscape ? "landscape" : "portrait"));

	return att;
}

//! Basically reverse of dump_out.
void PaperStyle::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;

	char *aname,*value;
	const char *convertunits=nullptr;

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

		} else if (!strcmp(aname,"orientation")) {
			if (!strcasecmp(value,"portrait"))
				is_landscape = false;
			else  //landscape
				is_landscape = true;

		} else if (!strcmp(aname,"landscape")) {
			is_landscape = true;

		} else if (!strcmp(aname,"portrait")) {
			is_landscape = false;

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

int PaperStyle::type()
{
	ObjectDef *def=GetObjectDef();
	return def->fieldsformat;
}

//! Copy over name, width, height, dpi.
Value *PaperStyle::duplicate()
{
	PaperStyle *ps=new PaperStyle();

	makestr(ps->name,name);
	ps->width=width;
	ps->height=height;
	makestr(ps->defaultunits,defaultunits);
	ps->landscape(landscape());

	return ps;
}

int PaperStyle::getValueStr(char *buffer,int len)
{
	//"PaperStyle(width=8.4, height=11, orientation=portrait, dpi=300)"
	int needed=55+15+15+15+15+(name?strlen(name):0);
	if (len<needed) return needed;

	if (name) sprintf(buffer,"Paper(name=\"%s\", width=%.10g, height=%.10g, orientation=%s, dpi=%.10g)",
					name, width,height,landscape()?"landscape":"portrait",dpi);
	else sprintf(buffer,"Paper(width=%.10g, height=%.10g, orientation=%s, dpi=%.10g)",
					width,height,landscape()?"landscape":"portrait",dpi);
	return 0;
}

Value *PaperStyle::dereference(const char *extstring, int len)
{
	if (extequal(extstring,len, "name")) {
		return new StringValue(name);

	} else if (extequal(extstring,len, "width")) {
		return new DoubleValue(width);

	} else if (extequal(extstring,len, "height")) {
		return new DoubleValue(height);

	} else if (extequal(extstring,len, "dpi")) {
		return new DoubleValue(dpi);

	} else if (extequal(extstring,len, "orientation")) {
		return new StringValue(landscape()?"landscape":"portrait");

	} else if (extequal(extstring,len, "units")) {
		return new StringValue(defaultunits);
	}

	return nullptr;
}

int PaperStyle::Evaluate(const char *function,int len, ValueHash *context, ValueHash *pp, CalcSettings *settings,
                         Value **value_ret, ErrorLog *log)
{
	if (!pp || pp->n() == 0) return 1;
	if (isName(function, len, "SetFromName")) {
		const char *name = pp->findString("name");
		if (!name) return 1;
		return SetFromString(name);
	}

	return 1;
}

Value *NewPaperStyle()
{
	PaperStyle *d=new PaperStyle;
	return d;
}

/*! If the paper name is in laidout->papersizes, then grab a duplicate of that,
 * and overwrite any differing settings.
 *
 * \todo if making a copy of a system known paper size, should restrict the w and h
 *   to the actual values for paper, or auto change the paper name for consistency
 */
int createPaperStyle(ValueHash *context, ValueHash *parameters, Value **value_ret, ErrorLog &log)
{
	if (!parameters) {
		if (value_ret) *value_ret=nullptr;
		log.AddMessage(_("Easy for you to say!"),ERROR_Fail);
		return 1;
	}


	PaperStyle *paper=nullptr;
	int err=0;
	try {
		int i=0;

		 //---name
		Value *v=parameters->find("paper");
		const char *str=nullptr;
		if (v) {
			if (v->type()==VALUE_String) {
				const char *str=(const char *)dynamic_cast<StringValue*>(v)->str;
				if (!str) throw _("Invalid name for paper!");
			} else throw  _("Invalid object for paper!");
		}
		if (!str) str=laidout->prefs.defaultpaper;
		if (!str) str="letter";
		for (int c=0; c<laidout->papersizes.n; c++) {
			if (strcasecmp(str,laidout->papersizes.e[c]->name)==0) {
				paper=(PaperStyle*)laidout->papersizes.e[c]->duplicate();
				break;
			}
		}
		if (!paper) paper=new PaperStyle("Letter",8.5,11,0,300,"in");


		 //----orientation
		int orientation=parameters->findInt("orientation",-1,&i);
		if (i==2) throw _("Invalid format for orientation!");
		if (i==0) {
			if (orientation==0) paper->is_landscape = false;
			else paper->is_landscape = true;
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
		log.AddMessage(str,ERROR_Fail);
		if (paper) { paper->dec_count(); paper=nullptr; }
		err=1;
	}


	if (err==0) {
		if (value_ret) {
			if (paper) *value_ret=paper;
			else *value_ret=nullptr;
		}
	}

	return err;
}

ObjectDef *PaperStyle::makeObjectDef()
{
	ObjectDef *sd=stylemanager.FindDef("Paper");
	if (sd) {
		sd->inc_count();
		return sd;
	}


	sd=new ObjectDef(nullptr,
							  "Paper",
							  _("Paper"),
							  _("A basic rectangular paper with orientation"),
							  "class",
							  nullptr, //range
							  nullptr, //defval
							  nullptr,0, //fields, flags
							  NewPaperStyle,
							  createPaperStyle);

	sd->push("name",
			_("Name"),
			_("Name of the paper, like A4 or Letter"),
			"string",
			nullptr, //range
			nullptr, //def value
			0,
			nullptr);
	sd->pushEnum("orientation",
			_("Orientation"),
			_("Either portrait (0) or landscape (1)"),
			false, //whether is enumclass or enum instance
			"portrait",//defval
			nullptr,nullptr,//new funcs
			  "portrait",_("Portrait"),_("Width and height are as normal"),
			  "landscape",_("Landscape"),_("Width and height are swapped"),
			  nullptr);
	sd->push("width",
			_("Width"),
			_("Width of the paper, after orientation is applied"),
			"real", nullptr,nullptr,
			0,
			nullptr);
	sd->push("height",
			_("Height"),
			_("Height of the paper, after orientation is applied"),
			"real", nullptr,nullptr,
			0,
			nullptr);
	sd->push("dpi",
			_("Dpi"),
			_("Default dots per inch of the paper"),
			"real", 
			nullptr,
			nullptr,
			0,
			nullptr);
	sd->pushFunction("SetFromName", _("Set from name"), _("Set paper from common paper names like letter or A4"), 
			nullptr,
			"name", _("Paper Name"), _("Paper name"), "string", nullptr, nullptr,
			nullptr);

	stylemanager.AddObjectDef(sd,0);
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
PaperBox::PaperBox(PaperStyle *paper, bool absorb_count)
{
	which=0; //a mask of which boxes are defined
	paperstyle=paper;

	if (paper) {
		if (!absorb_count) paper->inc_count();
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

bool PaperBox::landscape()
{
	return paperstyle ? paperstyle->landscape() : false;
}

bool PaperBox::landscape(bool l)
{
	if (!paperstyle) return false;
	if (l == paperstyle->landscape()) return l;

	// else need to update box info
	paperstyle->landscape(l);
	media.minx = media.miny = 0;
	media.maxx = paperstyle->w(); //takes into account paper orientation
	media.maxy = paperstyle->h();
	return l;
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

anObject *PaperBox::duplicate(anObject *ref)
{
	PaperBox *box = nullptr;
	if (!ref) {
		box = new PaperBox(nullptr, 0);
		ref = box;
	} else {
		box = dynamic_cast<PaperBox*>(ref);
		if (!box) return nullptr; //wrong dup type!
	}

	box->which = which;
	box->paperstyle = (paperstyle ? dynamic_cast<PaperStyle*>(paperstyle->duplicate()) : nullptr);
	box->media = media;
	box->printable = printable;
	box->bleed = bleed;
	box->trim = trim;
	box->crop = crop;
	box->art = art;

	return box;
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
	outlinecolor.rgbf(0,0,1.0); //color of the outline, default is blue
	color.rgbf(1.0,1.0,1.0);

	index = index_back = -1;

	box = paper;
	if (box) {
		box->inc_count();
		setbounds(&box->media);
	}
}

PaperBoxData::~PaperBoxData()
{
	if (box) box->dec_count();
}

void PaperBoxData::FindBBox()
{
	minx = miny = 0;

	if (box && box->paperstyle) {
		maxx=box->paperstyle->w();
		maxy=box->paperstyle->h();

	} else {
		maxx = maxy = -1;
	}
}

LaxInterfaces::SomeData *PaperBoxData::duplicate(LaxInterfaces::SomeData *dup)
{
	PaperBoxData *pdata = nullptr;
	if (!dup) {
		pdata = new PaperBoxData(nullptr);
		dup = pdata;
	} else {
		pdata = dynamic_cast<PaperBoxData*>(dup);
		if (!pdata) return nullptr; //wrong dup type!
	}

	pdata->label        = label;
	pdata->color        = color;
	pdata->outlinecolor = outlinecolor;
	pdata->box          = dynamic_cast<PaperBox*>(box->duplicate(nullptr));
	pdata->index        = index;
	pdata->index_back   = index_back;
	pdata->m(m());
	// pdata->properties.CopyFrom(&properties);
	pdata->FindBBox();

	return dup;
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
	locked      = 0;
	name = Name = nullptr;
	obj_flags  |= OBJ_Zone | OBJ_Unselectable;

	DBG cerr <<"PaperGroup created, obj "<<object_id<<endl;
}

//! Create a PaperGroup with one paper based on paperstyle, with only media box defined.
/*! This inc_count()'s paperstyle, does not duplicate it.
 */
PaperGroup::PaperGroup(PaperStyle *paperstyle)
{
	locked      = 0;
	name = Name = nullptr;
	obj_flags  |= OBJ_Zone | OBJ_Unselectable;

	PaperBox *box=new PaperBox(paperstyle, false);
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
	locked      = 0;
	name = Name = nullptr;
	obj_flags  |= OBJ_Zone | OBJ_Unselectable;

	papers.push(boxdata);

	DBG cerr <<"PaperGroup created, obj "<<object_id<<endl;
}

PaperGroup::~PaperGroup()
{
	if (name) delete[] name;
	if (Name) delete[] Name;

	DBG cerr <<"PaperGroup destroyed, obj "<<object_id<<endl;
}

//! The number of extra objects in the group.
int PaperGroup::n()
{ return objs.n(); }

Laxkit::anObject *PaperGroup::object_e(int i)
{
	if (i>=0 && i<objs.n()) return objs.e(i);
	return nullptr;
}

const char *PaperGroup::object_e_name(int i)
{
	if (i>=0 && i<objs.n()) return objs.e(i)->Id();
	return nullptr;
}

const double *PaperGroup::object_transform(int i)
{
	if (i>=0 && i<objs.n()) return objs.e(i)->m();
	return nullptr;
}

//! Make the outline this color. Range [0..65535].
double PaperGroup::OutlineColor(double r,double g,double b)
{
	for (int c=0; c<papers.n; c++) {
		papers.e[c]->outlinecolor.rgbf(r,g,b);
	}
	return papers.n;
}

/*! Returns nullptr when out of range, or missing paper.
 */
PaperStyle *PaperGroup::GetBasePaper(int index)
{
	if (index<0 || index>=papers.n) return nullptr;
	if (papers.e[index]->box
			&& papers.e[index]->box->paperstyle
			&& papers.e[index]->box->paperstyle->width)
		return papers.e[index]->box->paperstyle;
	return nullptr;
}

/*! Fill in the bounds required to fit all the defined papers.
 *
 * Return 0 for success or nonzero for invalid set of papers.
 * -1 means there are no defined papers.
 *
 * box_ret must NOT be nullptr.
 */
int PaperGroup::FindPaperBBox(Laxkit::DoubleBBox *box_ret)
{
	if (papers.n == 0) return -1;

	for (int c=0; c<papers.n; c++) {
		papers.e[c]->FindBBox();
		if (!papers.e[c]->validbounds()) return 1;

		box_ret->addtobounds(papers.e[c]->m(), papers.e[c]);
	}

	return 0;
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
 *     dpi 300
 * </pre>
 */
void PaperGroup::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%sname somename         #a string id. no whitespace\n",spc);
		fprintf(f,"%sName Descriptive Name #human readable name\n",spc);
		//if (owner) ***;
		fprintf(f,"%spaper                 #there can be 0 or more paper sections\n",spc);
		fprintf(f,"%s  matrix 1 0 0 1 0 0  #transform for the paper to limbo space\n",spc);
		fprintf(f,"%smarks                 #any optional printer marks for the group\n",spc);
		fprintf(f,"%s  ...",spc);
		//fprintf(f,"%s  outlinecolor 65535 0 0 #color of the outline of a paper in the interface\n",spc);
		PaperStyle paperstyle(nullptr,0,0,0,0,"in");
		paperstyle.dump_out(f,indent+2,-1,nullptr);
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
		fprintf(f,"%s  outlinecolor rgbf(%.10g, %.10g, %.10g)\n",
			spc, papers.e[c]->outlinecolor.Red(),
				 papers.e[c]->outlinecolor.Green(),
				 papers.e[c]->outlinecolor.Blue());
		papers.e[c]->box->paperstyle->dump_out(f,indent+2,0,context);
	}
	if (objs.n()) {
		fprintf(f,"%smarks\n",spc);
		objs.dump_out(f,indent+2,0,context);
	}	
}

void PaperGroup::dump_in_atts(Attribute *att,int flag,Laxkit::DumpContext *context)
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

		} else if (!strcmp(nme,"marks")) {
			objs.dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(nme,"paper")) {
			PaperStyle *paperstyle=new PaperStyle(nullptr,0,0,0,0,"in");
			paperstyle->dump_in_atts(att->attributes.e[c],flag,context);
			PaperBox *paperbox=new PaperBox(paperstyle, true);
			PaperBoxData *boxdata=new PaperBoxData(paperbox);
			paperbox->dec_count();
			boxdata->dump_in_atts(att->attributes.e[c],flag,context);

			Attribute *foundcolor = att->attributes.e[c]->find("outlinecolor");
			if (foundcolor) {
				SimpleColorAttribute(foundcolor->value, nullptr, &boxdata->outlinecolor, nullptr);
			} else {
				boxdata->outlinecolor.rgbf(1.0, 0.0, 1.0);
			} 
			papers.push(boxdata);
			boxdata->dec_count();
		}
	}
}

int PaperGroup::AddPaper(const char *nme,double w,double h,const double *m, const char *label)
{
	int landscape = 0;
	PaperStyle *paperstyle = GetNamedPaper(w,h, &landscape, 0,nullptr, .0001);
	if (paperstyle) {
		paperstyle = dynamic_cast<PaperStyle*>(paperstyle->duplicate());
		paperstyle->landscape(landscape);
	} else {
		paperstyle = new PaperStyle(nme,w,h,0,72,nullptr);
	}
	PaperBox *box = new PaperBox(paperstyle, false);
	paperstyle->dec_count();

	PaperBoxData *boxdata = new PaperBoxData(box);
	box->dec_count();
	boxdata->m(m);
	if (label) boxdata->label = label;

	papers.push(boxdata);
	boxdata->dec_count();
	return 0;
}

int PaperGroup::AddPaper(double w,double h,double offsetx,double offsety)
{
	PaperStyle *paperstyle=new PaperStyle("paper",w,h,0,72,nullptr);
	PaperBox *box=new PaperBox(paperstyle, false);
	paperstyle->dec_count();

	PaperBoxData *boxdata=new PaperBoxData(box);
	box->dec_count();
	boxdata->origin(flatpoint(offsetx,offsety));

	papers.push(boxdata);
	return 0;
}

} // namespace Laidout
