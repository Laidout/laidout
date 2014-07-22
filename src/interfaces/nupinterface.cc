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
// Copyright (C) 2012 by Tom Lechner
//


#include "../language.h"
#include "nupinterface.h"
//#include "viewwindow.h"
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>


#include <lax/refptrstack.cc>

using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


namespace Laidout {



#define PAD 5
#define fudge 5.0

#define CONTROL_Skip 1


//  ^     3--->   ^          5
//  |       5     |3-->  <--3|
//  5       |     |          |
//3---->    v     5          v
//
//Moveable box around this ^^ adjuster
//Scaling box for preview of adjustment
//
//int direction; //lrtb, lrbt, rltb, rlbt, ...
//int rows, cols;
//DoubleBBox final_grid_bounds;
//double final_grid_offset[6];
//
//double ui_offset[6];




//------------------------------------- NUpInfo --------------------------------------
/*! \class NUpInfo
 * \brief Info about how to arrange in an n-up style.
 *
 * Set flow direction, number of rows and columns. If rows or cols is -1 as a
 * minor direction, then add as many as possible.
 *
 * See also NUpInterface.
 */


NUpInfo::NUpInfo()
{
	direction=LAX_LRTB;
	rows=3;
	cols=3;
	valign=halign=50;
	flowtype=NUP_Grid;
	name=NULL;
	defaultgap=0;

	scale=1;

	minx=0; maxx=100;
	miny=0; maxy=100;
}

NUpInfo::~NUpInfo()
{
	if (name) delete[] name;
}

void NUpInfo::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

    if (what==-1) {
        fprintf(f,"%sname Blah          #optional human readable name\n",spc);
        fprintf(f,"%srows 3             #number of rows, positive number, n, or ...\n",spc);
        fprintf(f,"%scolumns 3          #number of columns\n",spc);
        fprintf(f,"%sdirection lrtb     #flow direction, left to right, then top to bottom, or other combos\n",spc);
		fprintf(f,"%sflowtype grid      #how to arrange in target area. grid,sizedgrid,flowed,random,unclump\n",spc);
        fprintf(f,"%svalign 50          #alignment vertically in cells\n",spc);
        fprintf(f,"%shalign 50          #alignment horizontally in cells\n",spc);
        fprintf(f,"%sdefaultgap         #default gap around objects when laying out\n",spc);
        fprintf(f,"%sscale              #scale of the ui panel\n",spc);
		return;
	}

	if (name) fprintf(f,"%sname %s\n",spc,name);

	if (rows==0) fprintf(f,"%srows n\n",spc);
	else if (rows<0) fprintf(f,"%srows ...\n",spc);
	else fprintf(f,"%srows %d\n",spc,rows);

	if (cols==0) fprintf(f,"%scols n\n",spc);
	else if (cols<0) fprintf(f,"%scols ...\n",spc);
	else fprintf(f,"%scols %d\n",spc,cols);

	const char *s=NULL;
	if      (direction==LAX_LRTB) s="lrtb";
	else if (direction==LAX_RLTB) s="rltb";
	else if (direction==LAX_LRBT) s="lrbt";
	else if (direction==LAX_RLBT) s="rlbt";
	else if (direction==LAX_TBRL) s="tbrl";
	else if (direction==LAX_TBLR) s="tblr";
	else if (direction==LAX_BTRL) s="btrl";
	else if (direction==LAX_BTLR) s="btlr";
	fprintf(f,"%sdirection %s\n",spc,s);

	if      (flowtype==NUP_Noflow)     s="noflow";
	else if (flowtype==NUP_Grid)       s="grid";
	else if (flowtype==NUP_Sized_Grid) s="sizedgrid";
	else if (flowtype==NUP_Flowed)     s="flowed";
	else if (flowtype==NUP_Random)     s="random";
	else if (flowtype==NUP_Unclump)    s="unclump";
	else if (flowtype==NUP_Unoverlap)  s="unoverlap";
	fprintf(f,"%sflowtype %s\n",spc,s);

	fprintf(f,"%svalign %.10g\n",spc,valign);
	fprintf(f,"%shalign %.10g\n",spc,halign);
	fprintf(f,"%sdefaultgap %.10g\n",spc,defaultgap);
	fprintf(f,"%sscale     %.10g\n",spc,scale);
}

int AlignmentAttribute(const char *value,double *a)
{
	if (!value) return 0;
	if (isalpha(*value)) {
		if (!strcasecmp(value,"left"))   { *a=0;   return 1; }
		if (!strcasecmp(value,"bottom")) { *a=0;   return 1; }
		if (!strcasecmp(value,"center")) { *a=50;  return 1; }
		if (!strcasecmp(value,"top"))    { *a=100; return 1; }
		if (!strcasecmp(value,"right"))  { *a=100; return 1; }
	}
	return DoubleAttribute(value,a,NULL);
}

void NUpInfo::dump_in_atts(LaxFiles::Attribute *att, int, Laxkit::anObject*)
{
    if (!att) return;
    char *name,*value;
    for (int c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

        if (!strcmp(name,"name")) {
            makestr(this->name,value);

        } else if (!strcmp(name,"rows")) {
            IntAttribute(value,&rows);

        } else if (!strcmp(name,"cols")) {
            IntAttribute(value,&cols);

        } else if (!strcmp(name,"direction")) {
			if      (!strcasecmp(value,"lrtb")) direction=LAX_LRTB;
			else if (!strcasecmp(value,"rltb")) direction=LAX_RLTB;
			else if (!strcasecmp(value,"lrbt")) direction=LAX_LRBT;
			else if (!strcasecmp(value,"rlbt")) direction=LAX_RLBT;
			else if (!strcasecmp(value,"tbrl")) direction=LAX_TBRL;
			else if (!strcasecmp(value,"tblr")) direction=LAX_TBLR;
			else if (!strcasecmp(value,"btrl")) direction=LAX_BTRL;
			else if (!strcasecmp(value,"btlr")) direction=LAX_BTLR;

        } else if (!strcmp(name,"flowtype")) {
			if      (!strcasecmp(value,"noflow"   )) flowtype=NUP_Noflow;
			else if (!strcasecmp(value,"grid"     )) flowtype=NUP_Grid;
			else if (!strcasecmp(value,"sizedgrid")) flowtype=NUP_Sized_Grid;
			else if (!strcasecmp(value,"flowed"   )) flowtype=NUP_Flowed;
			else if (!strcasecmp(value,"random"   )) flowtype=NUP_Random;
			else if (!strcasecmp(value,"unclump"  )) flowtype=NUP_Unclump;
			else if (!strcasecmp(value,"unoverlap")) flowtype=NUP_Unoverlap;

        } else if (!strcmp(name,"valign")) {
            AlignmentAttribute(value,&valign);

        } else if (!strcmp(name,"halign")) {
            AlignmentAttribute(value,&halign);

        } else if (!strcmp(name,"defaultgap")) {
            DoubleAttribute(value,&defaultgap);

        } else if (!strcmp(name,"scale")) {
            DoubleAttribute(value,&scale);

		}
	}
}



//------------------------------------- NUpInterface --------------------------------------
	
/*! \class NUpInterface 
 * \brief Interface to define and modify n-up style arrangements. See also NUpInfo.
 */


enum NupInterfaceActions {
	NUPA_Apply=OIA_MAX,
	NUPA_NextLayoutType,
	NUPA_PrevLayoutType,
	NUPA_NextDir,
	NUPA_PrevDir,
	NUPA_MAX
};


NUpInterface::NUpInterface(int nid,Displayer *ndp)
	: ObjectInterface(nid,ndp) 
{
	tempdir=0;
	//style|=RECT_HIDE_CONTROLS;
	style=style&~RECT_FLIP_AT_SIDES;

	//nup_style=NUP_Has_Ok|NUP_Has_Type;
	firsttime=1;
	showdecs=0;
	color_arrow=rgbcolor(60,60,60);
	color_num=rgbcolor(0,0,0);
	overoverlay=-1;
	tempdir=-1;
	temparrowdir=-1;
	active=0;
	needtoresetlayout=1;

	nupinfo=new NUpInfo;
	nupinfo->uioffset=flatpoint(50,50);
	valignblock=halignblock=NULL;
	createControls();
}

NUpInterface::NUpInterface(anInterface *nowner,int nid,Displayer *ndp)
	: ObjectInterface(nid,ndp) 
{
	tempdir=0;
	//style|=RECT_HIDE_CONTROLS;
	style=style&~RECT_FLIP_AT_SIDES;

	//nup_style=NUP_Has_Ok|NUP_Has_Type;
	firsttime=1;
	showdecs=0;
	color_arrow=rgbcolor(60,60,60);
	color_num=rgbcolor(0,0,0);
	overoverlay=-1;
	tempdir=-1;
	temparrowdir=-1;
	active=0;
	needtoresetlayout=1;

	nupinfo=new NUpInfo;
	nupinfo->uioffset=flatpoint(50,50);
	valignblock=halignblock=NULL;
	createControls();
}

NUpInterface::~NUpInterface()
{
	DBG cerr <<"NUpInterface destructor.."<<endl;

	if (nupinfo) nupinfo->dec_count();

	//if (doc) doc->dec_count();
}

flatpoint *rect_coordinates(flatpoint *p, double x, double y,double w,double h)
{
	p[0]=flatpoint(x,y);
	p[1]=flatpoint(x+w,y);
	p[2]=flatpoint(x+w,y+h);
	p[3]=flatpoint(x,y+h);
	return p;
}

//! Define an arrow from to to, with tail width w. Puts 7 points in p. p must be allocated for at least that many.
/*! Arrow height will be 3*w.
 * Returns p.
 */
flatpoint *arrow_coordinates(flatpoint *p, flatpoint from, flatpoint to, double w)
{
	flatpoint x,y;
	x=to-from;
	double d=norm(x);
	x/=d;
	y=transpose(x);

	double a=w*2;

	p[0]=from +     0*x + -w/2*y;
	p[1]=from + (d-a)*x + -w/2*y;
	p[2]=from + (d-a)*x +   -2*w*y;
	p[3]=from +     d*x +    0*y;
	p[4]=from + (d-a)*x +    2*w*y;
	p[5]=from + (d-a)*x +  w/2*y;
	p[6]=from +     0*x +  w/2*y;

	return p;
}

const char *NUpInterface::controlTooltip(int action)
{
	for (int c=0; c<controls.n; c++) 
		if (controls.e[c]->action==action) return controls.e[c]->tip;
	return " ";
}

void NUpInterface::createControls()
{
	controls.push(new ActionArea(NUP_Major_Arrow        , AREA_Handle, NULL, _("Drag to adjust"),0,1,color_arrow,0));
	controls.push(new ActionArea(NUP_Minor_Arrow        , AREA_Handle, NULL, _("Drag to adjust"),0,1,color_arrow,0));
	controls.push(new ActionArea(NUP_Major_Number       , AREA_Handle, NULL, _("Wheel to change"),0,1,color_num,0));
	controls.push(new ActionArea(NUP_Minor_Number       , AREA_Handle, NULL, _("Wheel to change"),0,1,color_num,0));

	controls.push(new ActionArea(NUP_Ok                 , AREA_Handle, NULL, _("Wheel to change"),0,1,color_num,0));
	controls.push(new ActionArea(NUP_Type               , AREA_Handle, NULL, _("Wheel to change"),0,1,color_num,0));

	controls.push(new ActionArea(NUP_VAlign             , AREA_Handle, NULL, _("Drag to change"),0,1,color_arrow,0));
	controls.push(new ActionArea(NUP_HAlign             , AREA_Handle, NULL, _("Drag to change"),0,1,color_arrow,0));

	major      =controls.e[0];
	minor      =controls.e[1];
	majornum   =controls.e[2];
	minornum   =controls.e[3];
	okcontrol  =controls.e[4];
	typecontrol=controls.e[5];
	valignblock=controls.e[6];
	halignblock=controls.e[7];
}

void NUpInterface::remapControls(int tempdir)
{
	double x,y;
	double ww4, hh4;
	//double major_w=100;
	//double minor_w=75;
	flatpoint *p;

	double totalh=nupinfo->maxy-nupinfo->miny;
	double totalw=nupinfo->maxx-nupinfo->minx;
	ww4=totalw/4;
	hh4=totalh/4;

	int dir=(tempdir>=0?tempdir:nupinfo->direction);
	double majthick=ww4/4;
	double minthick=ww4/8;

	 //define major arrow
	if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
		 //major arrow is horizontal, adjust position downward if at bottom
		p=major->Points(NULL,7,0);
		if (dir==LAX_LRTB || dir==LAX_RLTB) y=hh4/2; else y=hh4*3.5;
		if (dir==LAX_LRTB || dir==LAX_LRBT) 
			 arrow_coordinates(p, flatpoint(ww4,y), flatpoint(4*ww4,y), majthick);
		else arrow_coordinates(p, flatpoint(3*ww4,y), flatpoint(0,y), majthick);
		major->FindBBox();

		p=majornum->Points(NULL,4,0);
		if (dir==LAX_LRTB || dir==LAX_LRBT) rect_coordinates(p, 0,y-hh4/2,ww4,hh4);
		else rect_coordinates(p, 3*ww4,y-hh4/2,ww4,hh4);
		majornum->FindBBox();

	} else {
		 //major arrow is vertical
		y=0;
		p=major->Points(NULL,7,0);
		if (dir==LAX_TBRL || dir==LAX_BTRL) x=ww4*3.5; else x=ww4/2;
		if (dir==LAX_TBRL || dir==LAX_TBLR)
			 arrow_coordinates(p, flatpoint(x,hh4), flatpoint(x,4*hh4), majthick);
		else arrow_coordinates(p, flatpoint(x,3*hh4), flatpoint(x,0), majthick);
		major->FindBBox();

		p=majornum->Points(NULL,4,0);
		if (dir==LAX_BTLR || dir==LAX_BTRL) rect_coordinates(p, x-ww4/2,y+3*hh4, ww4,hh4);
		else rect_coordinates(p, x-ww4/2,0,ww4,hh4);
		majornum->FindBBox();
	}

	 //define minor arrow
	if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
		 //minor arrow is vertical, adjust position downward if at bottom
		p=minor->Points(NULL,7,0);
		if (dir==LAX_LRTB || dir==LAX_RLTB)
			 arrow_coordinates(p, flatpoint(2*ww4,2*hh4), flatpoint(2*ww4,4*hh4), minthick);
		else arrow_coordinates(p, flatpoint(2*ww4,2*hh4), flatpoint(2*ww4,0), minthick);
		minor->FindBBox();

		p=minornum->Points(NULL,4,0);
		if (dir==LAX_LRTB || dir==LAX_RLTB) rect_coordinates(p, 1.5*ww4,hh4, ww4,hh4);
		else rect_coordinates(p, 1.5*ww4,2*hh4, ww4,hh4);
		minornum->FindBBox();

	} else {
		 //minor arrow is horizontal
		p=minor->Points(NULL,7,0);
		if (dir==LAX_TBRL || dir==LAX_BTRL) 
			 arrow_coordinates(p, flatpoint(2*ww4,2*hh4), flatpoint(0,2*hh4), minthick);
		else arrow_coordinates(p, flatpoint(2*ww4,2*hh4), flatpoint(4*ww4,2*hh4), minthick);
		minor->FindBBox();

		p=minornum->Points(NULL,4,0);
		if (dir==LAX_BTLR || dir==LAX_TBLR) rect_coordinates(p, ww4,1.5*hh4,ww4,hh4);
		else rect_coordinates(p, 2*ww4,1.5*hh4, ww4,hh4);
		minornum->FindBBox();
	}

	 //define alignment handles
	// ***

	 //type of flow
	p=typecontrol->Points(NULL,4,0);
	rect_coordinates(p, ww4*4/3,4*hh4, ww4*4/3,hh4*4/3);
	typecontrol->FindBBox();
	if (!(nup_style&NUP_Has_Type)) typecontrol->hidden=1;

	 //optional ok button
	if (nup_style&NUP_Has_Ok) {
		p=okcontrol->Points(NULL,4,0);
		rect_coordinates(p, ww4,6.5*hh4, 2*ww4,hh4);
		okcontrol->FindBBox();
		okcontrol->hidden=0;
	} else {
		okcontrol->hidden=1;
	}
}

const char *NUpInterface::Name()
{ return _("N-up"); }

//! Return and optionally set a flowtype status message.
const char *NUpInterface::flowtypeMessage(int set)
{
	const char *mes=NULL;
	int i=nupinfo->flowtype;
	if (i==NUP_Grid)            mes=_("Grid");
	else if (i==NUP_Sized_Grid) mes=_("Sized grid");
	else if (i==NUP_Flowed)     mes=_("Flowed");
	else if (i==NUP_Random)     mes=_("Randomize");
	else if (i==NUP_Unclump)    mes=_("Unclump");
	else if (i==NUP_Unoverlap)  mes=_("Unoverlap");
	if (set && mes) viewport->postmessage(mes);
	return mes;
}

const char *dirname(int dir)
{
	if (dir==LAX_LRTB) return _("Left to right, top to bottom");
	if (dir==LAX_LRBT) return _("Left to right, bottom to top");
	if (dir==LAX_RLTB) return _("Right to left, top to bottom");
	if (dir==LAX_RLBT) return _("Right to left, bottom to top");
	if (dir==LAX_TBLR) return _("Top to bottom, left to right");
	if (dir==LAX_BTLR) return _("Bottom to top, left to right");
	if (dir==LAX_TBRL) return _("Top to bottom, right to left");
	if (dir==LAX_BTRL) return _("Bottom to top, right to left");
	return NULL;
}

/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *NUpInterface::ContextMenu(int x,int y,int deviceid)
{
	MenuInfo *menu=new MenuInfo(_("N-up Interface"));

	menu->AddItem(dirname(LAX_LRTB), NULL, LAX_LRTB, LAX_ISTOGGLE|(nupinfo->direction==LAX_LRTB?LAX_CHECKED:0)|LAX_OFF, 1);
	menu->AddItem(dirname(LAX_LRBT), NULL, LAX_LRBT, LAX_ISTOGGLE|(nupinfo->direction==LAX_LRBT?LAX_CHECKED:0)|LAX_OFF, 1);
	menu->AddItem(dirname(LAX_RLTB), NULL, LAX_RLTB, LAX_ISTOGGLE|(nupinfo->direction==LAX_RLTB?LAX_CHECKED:0)|LAX_OFF, 1);
	menu->AddItem(dirname(LAX_RLBT), NULL, LAX_RLBT, LAX_ISTOGGLE|(nupinfo->direction==LAX_RLBT?LAX_CHECKED:0)|LAX_OFF, 1);
	menu->AddItem(dirname(LAX_TBLR), NULL, LAX_TBLR, LAX_ISTOGGLE|(nupinfo->direction==LAX_TBLR?LAX_CHECKED:0)|LAX_OFF, 1);
	menu->AddItem(dirname(LAX_BTLR), NULL, LAX_BTLR, LAX_ISTOGGLE|(nupinfo->direction==LAX_BTLR?LAX_CHECKED:0)|LAX_OFF, 1);
	menu->AddItem(dirname(LAX_TBRL), NULL, LAX_TBRL, LAX_ISTOGGLE|(nupinfo->direction==LAX_TBRL?LAX_CHECKED:0)|LAX_OFF, 1);
	menu->AddItem(dirname(LAX_BTRL), NULL, LAX_BTRL, LAX_ISTOGGLE|(nupinfo->direction==LAX_BTRL?LAX_CHECKED:0)|LAX_OFF, 1);

	menu->AddSep();

	menu->AddItem(_("Grid"),      NUP_Grid      );
	menu->AddItem(_("Sized Grid"),NUP_Sized_Grid);
	menu->AddItem(_("Flowed"),    NUP_Flowed    );
	menu->AddItem(_("Random"),    NUP_Random    );
	menu->AddItem(_("Unclump"),   NUP_Unclump   );
	menu->AddItem(_("Unoverlap"), NUP_Unoverlap );

	return menu;
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int NUpInterface::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i =s->info2; //id of menu item, 1 for direction, 0 for other 
		int ii=s->info4; //extra id, 1 for direction

		if (ii==1) {
			DBG cerr <<"change direction to "<<i<<endl;

			if (i!=nupinfo->direction) {
				nupinfo->direction=i;
				remapControls();
				validateInfo();
				needtodraw=1;
			}
			return 0;

		} else {
			if (i!=nupinfo->flowtype) {
				nupinfo->flowtype=i;
				flowtypeMessage(1);
				needtoresetlayout=1;
				if (active) Apply(0);
				needtodraw=1;
			}
			return 0;
		}

		return 0;
	}
	return 1;
}


//! Use a Document.
int NUpInterface::UseThis(Laxkit::anObject *ndata,unsigned int mask)
{
//	Document *d=dynamic_cast<Document *>(ndata);
//	if (!d && ndata) return 0; //was a non-null object, but not a document
//
//	UseThisDocument(d);
//	needtodraw=1;

	return 1;
}

/*! Will say it cannot draw anything.
 */
int NUpInterface::draws(const char *atype)
{ return 0; }


//! Return a new NUpInterface if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *NUpInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new NUpInterface(id,NULL);
	else if (!dynamic_cast<NUpInterface *>(dup)) return NULL;

	return anInterface::duplicate(dup);
}


int NUpInterface::InterfaceOn()
{
	DBG cerr <<"pagerangeinterfaceOn()"<<endl;

	showdecs=2;
	needtodraw=1;
	return 0;
}

int NUpInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void NUpInterface::Clear(SomeData *d)
{
}

int NUpInterface::Refresh()
{
	if (!needtodraw) return 0;

	if (firsttime) {
		firsttime=0;
		remapControls();
	}

	RectInterface::Refresh();
	needtodraw=0;

	DBG cerr <<"NUpInterface::Refresh()..."<<endl;

	dp->DrawScreen();

	int x=nupinfo->uioffset.x;
	int y=nupinfo->uioffset.y;
	int dir=nupinfo->direction;
	int majorn, minorn;
	unsigned int arrowcolor=rgbcolor(128,128,128);

	if (tempdir>=0) {
		arrowcolor=rgbcolor(0,200,0);
		dir=tempdir;
		remapControls(tempdir);
	}

	flatpoint p;
	char buffer[30];

	//draw ui outline
	dp->NewFG(rgbcolor(128,128,128));
	dp->DrawScreen();
	//--square:
	//dp->drawline(flatpoint(x+nupinfo->minx,y+nupinfo->miny),flatpoint(x+nupinfo->maxx,y+nupinfo->miny));
	//dp->drawline(flatpoint(x+nupinfo->maxx,y+nupinfo->miny),flatpoint(x+nupinfo->maxx,y+nupinfo->maxy));
	//dp->drawline(flatpoint(x+nupinfo->maxx,y+nupinfo->maxy),flatpoint(x+nupinfo->minx,y+nupinfo->maxy));
	//dp->drawline(flatpoint(x+nupinfo->minx,y+nupinfo->maxy),flatpoint(x+nupinfo->minx,y+nupinfo->miny));
	//--rect:

	 //the control box is drawn in the rectangle bounds of nupinfo
	double width =nupinfo->maxx-nupinfo->minx;
	double height=nupinfo->maxy-nupinfo->miny;
	dp->drawline(flatpoint(x+nupinfo->minx,y+nupinfo->miny),flatpoint(x+nupinfo->maxx,y+nupinfo->miny));
	dp->drawline(flatpoint(x+nupinfo->maxx,y+nupinfo->miny),flatpoint(x+nupinfo->maxx,y+nupinfo->maxy+height/4));
	dp->drawline(flatpoint(x+nupinfo->minx,y+nupinfo->maxy+height/4),flatpoint(x+nupinfo->minx,y+nupinfo->miny));
	dp->drawline(flatpoint(x+nupinfo->maxx-width/3,y+nupinfo->maxy+height/4),flatpoint(x+nupinfo->maxx,y+nupinfo->maxy+height/4));
	dp->drawline(flatpoint(x+nupinfo->minx+width/3,y+nupinfo->maxy+height/4),flatpoint(x+nupinfo->minx,y+nupinfo->maxy+height/4));

	 //with activator button, 1/6 of the total box width:
	if (active) dp->NewFG(0,200,0); else dp->NewFG(255,100,100);
	dp->LineAttributes(3,LineSolid, CapButt, JoinMiter);
	flatpoint cc=flatpoint(x+nupinfo->maxx-width/2,y+nupinfo->maxy+height/4);
	dp->drawellipse(cc.x,cc.y,
			width/6,width/6,
			0,2*M_PI,
			0);
	dp->NewFG(rgbcolor(128,128,128));
	dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
	if (nupinfo->flowtype==NUP_Grid)            sprintf(buffer,"#");
	else if (nupinfo->flowtype==NUP_Sized_Grid) sprintf(buffer,"+");
	else if (nupinfo->flowtype==NUP_Flowed)     sprintf(buffer,"=");
	else if (nupinfo->flowtype==NUP_Random)     sprintf(buffer,"?");
	else if (nupinfo->flowtype==NUP_Unclump)    sprintf(buffer,"o O");
	else if (nupinfo->flowtype==NUP_Unoverlap)  sprintf(buffer,"oO");
	else sprintf(buffer," ");
	dp->textout(cc.x,cc.y, buffer,-1,LAX_CENTER);


	 //draw major arrow number
	p=flatpoint(x+(majornum->minx+majornum->maxx)/2,y+(majornum->miny+majornum->maxy)/2);
	if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
		majorn=nupinfo->cols;
		minorn=nupinfo->rows;
	} else {
		majorn=nupinfo->rows;
		minorn=nupinfo->cols;
	}

	if (majorn>=1) sprintf(buffer,"%d",majorn);
	else if (majorn==0) sprintf(buffer,"n");
	else sprintf(buffer,"...");
	dp->textout(p.x,p.y, buffer,-1);

	 //draw minor arrow number
	p=flatpoint(x+(minornum->minx+minornum->maxx)/2,y+(minornum->miny+minornum->maxy)/2);
	if (minorn>=1) sprintf(buffer,"%d",minorn);
	else if (minorn==0) sprintf(buffer,"n");
	else sprintf(buffer,"...");
	dp->textout(p.x,p.y, buffer,-1);

	 //draw arrows
	drawHandle(major,arrowcolor,nupinfo->uioffset);
	drawHandle(minor,arrowcolor,nupinfo->uioffset);

	 //draw alignment handles
	if (!(nup_style&NUP_No_Align)) {
		//if (valignblock) drawHandle(valignblock,arrowcolor,nupinfo->uioffset);
		//if (halignblock) drawHandle(halignblock,arrowcolor,nupinfo->uioffset);
	}

	//draw ok
	//if (nup_style&NUP_Has_Ok) drawHandle(okcontrol,okcontrol->color,nupinfo->uioffset);

	//draw type
	//if (nup_style&NUP_Has_Type) drawHandle(typecontrol,typecontrol->color,nupinfo->uioffset);



	dp->DrawReal();

	if (tempdir>=0) {
		remapControls(nupinfo->direction);
	}

	return 1;
}

void NUpInterface::drawHandle(ActionArea *area, unsigned int color, flatpoint offset)
{
	if (area->real) dp->DrawReal(); else dp->DrawScreen();
	dp->NewFG(color);

	flatpoint p[10];
	memcpy(p,area->outline,7*sizeof(flatpoint));
	for (int c=0; c<area->npoints; c++) p[c]+=nupinfo->uioffset;

	int fill= area->action==overoverlay;
	if (area->action==NUP_Major_Arrow && overoverlay==NUP_Major_Tip) fill=1;
	dp->drawlines(p,area->npoints,1,fill);
}

//void NUpInterface::ShowOrder()
//{
//	flatpoint last;
//	int i=-1;
//	char buffer[20];
//	for (int c=0; c<selection->n(); c++) {
//		if (objcontrols.e[c]->flags&CONTROL_Skip) continue;
//		if (i>=0) {
//			dp->drawline(last,objcontrols.e[c]->new_center); //black line
//			dp->drawline(last,objcontrols.e[c]->new_center); //white line
//			sprintf(buffer,"%d",i);
//			*** circle
//			dp->drawellipse(***);
//			dp->textout(buffer,-1, last.x,last.y, LAX_CENTER);
//		}
//		i=c;
//	}
//}

//! Return a new flow direction, or -1 if not in the panel.
int NUpInterface::vscan(int x,int y)
{
	//flatpoint fp=screentoreal(x,y);
	int dir=nupinfo->direction;
	double w,h;


	x-=nupinfo->uioffset.x;
	y-=nupinfo->uioffset.y;

	if (!(x>=nupinfo->minx && x<=nupinfo->maxx && y>=nupinfo->miny && y<=nupinfo->maxy)) return -1;

	w=nupinfo->maxx-nupinfo->minx;
	h=nupinfo->maxy-nupinfo->miny;

	double xx,yy;
	xx=(x-nupinfo->minx)/w*4;
	yy=(y-nupinfo->miny)/h*4;

	int newdir=-1;
	int oldh=-1;
	if (dir==LAX_LRBT || dir==LAX_LRTB || dir==LAX_TBLR || dir==LAX_BTLR) oldh=NUP_LtoR; else oldh=NUP_RtoL;

	if (xx<1) {
		if (yy<2) newdir=LAX_BTLR;
		if (yy>=2) newdir=LAX_TBLR;
	}
	if (xx>3) {
		if (yy<2) newdir=LAX_BTRL;
		if (yy>=2) newdir=LAX_TBRL;
	}
	if (xx>=1 && xx<=3) {
		if (yy<2)  { if (oldh==NUP_LtoR) newdir=LAX_LRBT; else newdir=LAX_RLBT; }
		if (yy>=2) { if (oldh==NUP_LtoR) newdir=LAX_LRTB; else newdir=LAX_RLTB; }
	}

	return newdir;
}

//! Return a new flow direction, or -1 if not in the panel.
int NUpInterface::hscan(int x,int y)
{
	//flatpoint fp=screentoreal(x,y);
	int dir=nupinfo->direction;
	double w,h;


	x-=nupinfo->uioffset.x;
	y-=nupinfo->uioffset.y;

	if (!(x>=nupinfo->minx && x<=nupinfo->maxx && y>=nupinfo->miny && y<=nupinfo->maxy)) return -1;

	w=nupinfo->maxx-nupinfo->minx;
	h=nupinfo->maxy-nupinfo->miny;

	double xx,yy;
	xx=(x-nupinfo->minx)/w*4;
	yy=(y-nupinfo->miny)/h*4;

	int newdir=-1;
	int oldv=-1;
	if (dir==LAX_LRBT || dir==LAX_RLBT || dir==LAX_BTLR || dir==LAX_BTRL) oldv=NUP_BtoT; else oldv=NUP_TtoB;

	if (yy<1) {
		if (xx<2) newdir=LAX_RLTB;
		if (xx>=2) newdir=LAX_LRTB;
	}
	if (yy>3) {
		if (xx<2) newdir=LAX_RLBT;
		if (xx>=2) newdir=LAX_LRBT;
	}
	if (yy>=1 && yy<=3) {
		if (xx<2)  { if (oldv==NUP_BtoT) newdir=LAX_BTRL; else newdir=LAX_TBRL; }
		if (xx>=2) { if (oldv==NUP_BtoT) newdir=LAX_BTLR; else newdir=LAX_TBLR; }
	}

	return newdir;
}


//! Return which position and range mouse is over
int NUpInterface::scanNup(int x,int y)
{
	//flatpoint fp=screentoreal(x,y);
	int action=NUP_None, dir=nupinfo->direction;
	double w,h;
	w=nupinfo->maxx-nupinfo->minx;
	h=nupinfo->maxy-nupinfo->miny;


	x-=nupinfo->uioffset.x;
	y-=nupinfo->uioffset.y;

	double xx,yy;
	xx=(x-nupinfo->minx)/w*4;
	yy=(y-nupinfo->miny)/h*4;
	cerr <<"  over:"<<xx<<','<<yy<<endl;

	 //check activate button
	if (norm(flatpoint(nupinfo->minx+w/2,nupinfo->maxy+h/4)-flatpoint(x,y))<w/6) return NUP_Activate;


	 //check for position inside the panel
	if (x>=nupinfo->minx && x<=nupinfo->maxx && y>=nupinfo->miny && y<=nupinfo->maxy+h/4) {
		action=NUP_None;

		 //check against major arrow places
		if ((xx<1 && (dir==LAX_BTLR || dir==LAX_TBLR)) || (xx>=3 && (dir==LAX_BTRL || dir==LAX_TBRL))) {
			//major arrow is vertical..

			if (yy<1) {
				if (dir==LAX_BTLR || dir==LAX_BTRL) action=NUP_Major_Tip;
				else action=NUP_Major_Number;
			} else if (yy>=3) {
				if (dir==LAX_BTLR || dir==LAX_BTRL) action=NUP_Major_Number;
				else action=NUP_Major_Tip;
			} else action=NUP_Major_Arrow;

		} else if ((yy<1 && (dir==LAX_LRTB || dir==LAX_RLTB)) || (yy>=3 && (dir==LAX_RLBT || dir==LAX_LRBT))) {
			//major arrow is horizontal..

			if (xx<1) {
				if (dir==LAX_LRTB || dir==LAX_LRBT) action=NUP_Major_Number;
				else action=NUP_Major_Tip;
			} else if (xx>=3) {
				if (dir==LAX_LRTB || dir==LAX_LRBT) action=NUP_Major_Tip;
				else action=NUP_Major_Number;
			} else action=NUP_Major_Arrow;

		 //check against minor arrow places
		} else if (dir==LAX_BTLR || dir==LAX_TBLR) {
			if (yy>=1.5 && yy<2.5) {
				if (xx>=1 && xx<2) action=NUP_Minor_Number;
				else if (xx>=2) action=NUP_Minor_Arrow;
			}
		} else if (dir==LAX_BTRL || dir==LAX_TBRL) {
			if (yy>=1.5 && yy<2.5) {
				if (xx>=2 && xx<3) action=NUP_Minor_Number;
				else if (xx<2) action=NUP_Minor_Arrow;
			}
		} else if (dir==LAX_LRBT || dir==LAX_RLBT) {
			if (xx>=1.5 && xx<2.5) {
				if (yy>=2 && yy<3) action=NUP_Minor_Number;
				else if (yy<2) action=NUP_Minor_Arrow;
			}
		} else if (dir==LAX_LRTB || dir==LAX_RLTB) {
			if (xx>=1.5 && xx<2.5) {
				if (yy>=1 && yy<2) action=NUP_Minor_Number;
				else if (yy>=2) action=NUP_Minor_Arrow;
			}
		}


		if (action==NUP_None) action=NUP_Panel;
		return action;
	}

	x+=nupinfo->uioffset.x;
	y+=nupinfo->uioffset.y;

	 //search for other controls
	for (int c=0; c<controls.n; c++) {
		if (controls.e[c]->hidden) continue;

		if (controls.e[c]->category==0) {
			 //normal, single instance handles
			if (!controls.e[c]->real && controls.e[c]->PointIn(x,y)) {
				return action;
			}
		}
	}

	return NUP_None;
}

int NUpInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	DBG cerr <<"nup lbdown"<<endl;
	if (buttondown.any(0,LEFTBUTTON)) return 0; //only allow one button at a time

	int action=scanNup(x,y);
	if (action!=NUP_None) {
		buttondown.down(d->id,LEFTBUTTON,x,y,0,action);
		int dir=nupinfo->direction;
		if (action==NUP_Major_Arrow || action==NUP_Major_Tip) {
			if (dir==LAX_LRTB || dir==LAX_LRBT) temparrowdir=NUP_LtoR;
			else if (dir==LAX_RLTB || dir==LAX_RLBT) temparrowdir=NUP_RtoL;
			else if (dir==LAX_TBLR || dir==LAX_TBRL) temparrowdir=NUP_TtoB;
			else if (dir==LAX_BTRL || dir==LAX_BTLR) temparrowdir=NUP_BtoT;
		} else if (action==NUP_Minor_Arrow || action==NUP_Minor_Tip) {
			if (dir==LAX_LRTB || dir==LAX_RLTB) temparrowdir=NUP_TtoB;
			else if (dir==LAX_RLBT || dir==LAX_LRBT) temparrowdir=NUP_BtoT;
			else if (dir==LAX_TBLR || dir==LAX_BTLR) temparrowdir=NUP_LtoR;
			else if (dir==LAX_BTRL || dir==LAX_TBRL) temparrowdir=NUP_RtoL;
		}
		if (action!=NUP_Panel) tempdir=dir;
		return 0;
	}

	style|=RECT_OBJECT_SHUNT;
	return RectInterface::LBDown(x,y,state,count,d);
}

int NUpInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	DBG cerr <<"nup up"<<endl;
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	int action;
	int dragged=buttondown.up(d->id,LEFTBUTTON, NULL,&action);

	if (tempdir>=0 && tempdir!=nupinfo->direction) {
		nupinfo->direction=tempdir;
		remapControls();
		validateInfo();
		if (active) Apply(0);
	}
	tempdir=-1;
	temparrowdir=0;
	needtodraw=1;
	DBG cerr <<"NUpInterface::LBUp() dragged="<<dragged<<endl;

	if (action==NUP_Activate && !dragged) {
		active=!active;
		if (active) Apply(0);
		else Reset();
		needtodraw=1;
		return 0;
	}

	return RectInterface::LBUp(x,y,state,d);
}

//! Copy back original transforms from original transforms to the selection.
int NUpInterface::Reset()
{
	if (!data || !selection->n()) return 1;

	 //first reset positions to original state
	for (int c=0; c<selection->n(); c++) {
		selection->e(c)->obj->m(objcontrols.e[c]->original_transform->m());
	}

	RemapBounds();
	needtodraw=1;
	return 0;
}

int NUpInterface::validateInfo()
{
	int dir=nupinfo->direction;

	if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
		 //horizontal major axis
		if (nupinfo->rows<0) nupinfo->rows=0;
	} else {
		 //vertical major axis
		if (nupinfo->cols<0) nupinfo->cols=0;
	}
	needtodraw=1;
	return 0;
}

int NUpInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int action=scanNup(x,y);
	int dir=nupinfo->direction;

	if (action!=NUP_None && (state&ControlMask)) {
		nupinfo->maxx*=1.05;
		nupinfo->maxy*=1.05;
		remapControls();
		needtodraw=1;
		return 0;
	}

	if (action==NUP_Activate) {
		PerformAction(NUPA_NextLayoutType);
		needtodraw=1;
		return 0;
	}

	if (action==NUP_Major_Number || action==NUP_Major_Arrow || action==NUP_Major_Tip) {
		if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
			if (nupinfo->cols<0) nupinfo->cols=0; else nupinfo->cols++;
		} else {
			if (nupinfo->rows<0) nupinfo->rows=0; else nupinfo->rows++;
		}
		if (active) Apply(0);
		needtodraw=1;
		return 0;

	} else if (action==NUP_Minor_Number || action==NUP_Minor_Arrow || action==NUP_Minor_Tip) {
		if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
			if (nupinfo->rows<0) nupinfo->rows=0; else nupinfo->rows++;
		} else {
			if (nupinfo->cols<0) nupinfo->cols=0; else nupinfo->cols++;
		}
		if (active) Apply(0);
		needtodraw=1;
		return 0;
	}

	if (action!=NUP_None) return 0;
	return 1;
}

int NUpInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int action=scanNup(x,y);
	int dir=nupinfo->direction;

	if (action!=NUP_None && (state&ControlMask)) {
		nupinfo->maxx*=.95;
		nupinfo->maxy*=.95;
		remapControls();
		needtodraw=1;
		return 0;
	}

	if (action==NUP_Activate) {
		PerformAction(NUPA_PrevLayoutType);
		needtodraw=1;
		return 0;
	}


	 //the major axis cannot have infinity
	if (action==NUP_Major_Number || action==NUP_Major_Arrow || action==NUP_Major_Tip) {
		if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
			 //horizontal major axis
			nupinfo->cols--;
			if (nupinfo->cols<0) nupinfo->cols=0;
		} else {
			 //vertical major axis
			nupinfo->rows--;
			if (nupinfo->rows<0) nupinfo->rows=0;
		}
		if (active) Apply(0);
		needtodraw=1;
		return 0;

	} else if (action==NUP_Minor_Number || action==NUP_Minor_Arrow || action==NUP_Minor_Tip) {
		if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
			nupinfo->rows--;
			if (nupinfo->rows<0) nupinfo->rows=-1;
		} else {
			nupinfo->cols--;
			if (nupinfo->cols<0) nupinfo->cols=-1;
		}
		if (active) Apply(0);
		needtodraw=1;
		return 0;
	}

	if (action!=NUP_None) return 0;
	return 1;
}



DBG int lastx,lasty;

int NUpInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{

	DBG cerr <<"nup move"<<endl;
	int over=scanNup(x,y);

	DBG lastx=x; lasty=y;
	DBG cerr <<"over: "<<over<<endl;

	if (!buttondown.any()) {
		if (overoverlay!=over) {
			overoverlay=over;
			needtodraw=1;
			const char *mes=controlTooltip(over);
			if (mes) viewport->postmessage(mes);
		}
		return 0;
	}

	//so here the button must be down

	overoverlay=NUP_None;

	int oldx,oldy;
	int action=NUP_None;
	int rectaction=0;
	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &rectaction,&action);
	if (rectaction>0) {
		RectInterface::MouseMove(x,y,state,mouse);
		if (active) Apply(0);
		return 0;
	}
	buttondown.move(mouse->id,x,y,&oldx,&oldy);
	flatpoint d=flatpoint(x-oldx,y-oldy);
	
	if (action==NUP_None) return 0;

	if (action==NUP_Panel || (state&LAX_STATE_MASK)!=0) {
		nupinfo->uioffset+=d;
		needtodraw=1;
		return 0;

	} else if (action!=NUP_None) {
		if (temparrowdir>=0) {
			int d=-1;
			if (temparrowdir==NUP_LtoR || temparrowdir==NUP_RtoL) {
				d=hscan(x,y);
			} else if (temparrowdir==NUP_TtoB || temparrowdir==NUP_BtoT) {
				d=vscan(x,y);
			}
			if (d!=tempdir) {
				needtodraw=1;
				tempdir=d;
			}
			return 0;
		}
	}

//	 //^ scales
//	if ((state&LAX_STATE_MASK)==ControlMask) {
//	}
//
//	 //+^ rotates
//	if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) {
//	}

	return RectInterface::MouseMove(x,y,state,mouse);
}

/*! \class AlignInterface::ControlInfo
 *
 * Info about control points in AlignInterface. One per object.
 */

NUpInterface::ControlInfo::~ControlInfo()
{
	if (original_transform) original_transform->dec_count();
}


NUpInterface::ControlInfo::ControlInfo()
{
	amountx=amounty=0;
	flags=0;
	original_transform=NULL;
}

void NUpInterface::ControlInfo::SetOriginal(SomeData *o)
{
	if (original_transform) original_transform->dec_count();
	original_transform=o;
	if (original_transform) original_transform->inc_count();
}


int NUpInterface::AddToSelection(Selection *objs)
{
	int n=ObjectInterface::AddToSelection(objs);
	if (!n) return 0;

	objcontrols.flush();
	SomeData *t;
	for (int c=0; c<selection->n(); c++) {
		objcontrols.push(new ControlInfo(),1);

		t=new SomeData;
		t->m(selection->e(c)->obj->m());
		objcontrols.e[c]->SetOriginal(t);
		t->dec_count();
	}

	if (somedata) somedata->flags=somedata->flags&~(SOMEDATA_KEEP_ASPECT|SOMEDATA_KEEP_1_TO_1);

	return n;
}

//! Add objects to selection, then map default left and right bound.
int NUpInterface::AddToSelection(Laxkit::PtrStack<ObjectContext> &objs)
{
	int n=ObjectInterface::AddToSelection(objs);
	if (!n) return 0;
	//aligninfo->center=flatpoint((data->minx+data->maxx)/2,(data->miny+data->maxy)/2);
	//aligninfo->leftbound =-norm(flatpoint(data->minx,(data->miny+data->maxy)/2)-aligninfo->center);
	//aligninfo->rightbound= norm(flatpoint(data->maxx,(data->miny+data->maxy)/2)-aligninfo->center);

	objcontrols.flush();
	SomeData *t;
	for (int c=0; c<selection->n(); c++) {
		objcontrols.push(new ControlInfo(),1);

		t=new SomeData;
		t->m(selection->e(c)->obj->m());
		objcontrols.e[c]->SetOriginal(t);
		t->dec_count();
	}

	if (somedata) somedata->flags=somedata->flags&~(SOMEDATA_KEEP_ASPECT|SOMEDATA_KEEP_1_TO_1);

	return n;
}

int NUpInterface::FreeSelection()
{
	ObjectInterface::FreeSelection();
	controls.flush();
	return 0;
}

Laxkit::ShortcutHandler *NUpInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler("NUpInterface");
	if (sc) return sc;

	sc=RectInterface::GetShortcuts();

	sc->Add(NUPA_Apply,          LAX_Enter,0,0,    _("ToggleApply"),   _("Toggle apply"),NULL,0);
	sc->Add(NUPA_NextLayoutType, LAX_Left,0,0,     _("NextLayoutType"),_("Next layout type"),NULL,0);
	sc->Add(NUPA_PrevLayoutType, LAX_Right,0,0,    _("PrevLayoutType"),_("Previous layout type"),NULL,0);
	sc->Add(NUPA_NextDir,        LAX_Left,0,0,     _("NextDir"),       _("Next direction type"),NULL,0);
	sc->Add(NUPA_PrevDir,        LAX_Right,0,0,    _("PrevDir"),       _("Previous direction type"),NULL,0);

	//manager->AddArea("NUpInterface",sc); //added in RectInterface
	return sc;
}

/*! Return 0 for action performed, else 1.
 */
int NUpInterface::PerformAction(int action)
{
	if (action==NUPA_Apply) {
		active=!active;
		if (active) Apply(0);
		else Reset();
		needtodraw=1;
		return 0;

	} else if (action==NUPA_NextLayoutType) {
		nupinfo->flowtype++;
		if (nupinfo->flowtype>=NUP_MAX) nupinfo->flowtype=NUP_Grid;
		flowtypeMessage(1);
		needtoresetlayout=1;
		if (active) Apply(0);
		needtodraw=1;
		return 0;

	} else if (action==NUPA_PrevLayoutType) {
		nupinfo->flowtype--;
		if (nupinfo->flowtype<=NUP_Noflow) nupinfo->flowtype=NUP_MAX-1;
		flowtypeMessage(1);
		needtoresetlayout=1;
		if (active) Apply(0);
		needtodraw=1;
		return 0;

	} else if (action==NUPA_NextDir) {
		nupinfo->direction++;
		if (nupinfo->direction>7) nupinfo->direction=0;
		remapControls();
		viewport->postmessage(dirname(nupinfo->direction));
		needtoresetlayout=1;
		if (active) Apply(0);
		needtodraw=1;
		return 0;

	} else if (action==NUPA_PrevDir) {
		nupinfo->direction--;
		if (nupinfo->direction<0) nupinfo->direction=7;
		remapControls();
		viewport->postmessage(dirname(nupinfo->direction));
		needtoresetlayout=1;
		if (active) Apply(0);
		needtodraw=1;
		return 0;
	}

	int c=RectInterface::PerformAction(action);
	if (c==0 && active) Apply(0);
	return c;
}

int NUpInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" got ch:"<<ch<<"  "<<(state&LAX_STATE_MASK)<<endl;

	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	if (ch==LAX_Esc) {
		if (!strcmp(owner->whattype(),"ObjectInterface")) {
			 //revert control to owner
			ObjectInterface *gi=dynamic_cast<ObjectInterface*>(owner);
			gi->AddToSelection(selection);
			gi->RemoveChild();
		}
		return 0;


	} else if (ch=='o') {
		DBG cerr <<"--------------------------";
		DBG int action=scanNup(lastx,lasty);
		DBG cerr << " x,y:"<<lastx<<','<<lasty<<"  action:"<<action<<endl;
		return 0;


	} else if (ch==LAX_Shift) {
	} else if ((ch==LAX_Del || ch==LAX_Bksp) && (state&LAX_STATE_MASK)==0) {
	} else if (ch=='a' && (state&LAX_STATE_MASK)==0) {
	} else if (ch=='r' && (state&LAX_STATE_MASK)==0) {
	} else if (ch=='d' && (state&LAX_STATE_MASK)==0) {
	} else if (ch=='9' && (state&LAX_STATE_MASK)==0) {
	}
	return 1;
}

int NUpInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{ //***
	return 1;
}

void NUpInterface::WidthHeight(LaxInterfaces::ObjectContext *oc,flatvector x,flatvector y, double *width, double *height, flatpoint *cc)
{
	SomeData *o=oc->obj;
	flatpoint ul,ur,ll,lr;
	double min,max;
	double m[6];

	if (viewport) viewport->transformToContext(m,oc,0,1);
	else transform_copy(m,o->m());

	ul=transform_point(m,o->minx,o->maxy);
	ur=transform_point(m,o->maxx,o->maxy);
	ll=transform_point(m,o->minx,o->miny);
	lr=transform_point(m,o->maxx,o->miny);
	*cc=(ll+ur)/2;


	double dd=distparallel(ul,x);
	min=max=dd;
	dd=distparallel(ur,x);
	if (dd<min) min=dd; else if (dd>max) max=dd;
	dd=distparallel(ll,x);
	if (dd<min) min=dd; else if (dd>max) max=dd;
	dd=distparallel(lr,x);
	if (dd<min) min=dd; else if (dd>max) max=dd;
	*width=max-min;

	dd=distparallel(ul,y);
	min=max=dd;
	dd=distparallel(ur,y);
	if (dd<min) min=dd; else if (dd>max) max=dd;
	dd=distparallel(ll,y);
	if (dd<min) min=dd; else if (dd>max) max=dd;
	dd=distparallel(lr,y);
	if (dd<min) min=dd; else if (dd>max) max=dd;
	*height=max-min;
}

/*! Return 0 for success or 1 for unable to apply for some reason.
 */
int NUpInterface::Apply(int updateorig)
{
	if (!data || !selection->n()) return 1;

	double m[6],mm[6];
	transform_identity(m);
	transform_identity(mm);
	//data->m(m);


	 //first reset positions to original state
	for (int c=0; c<selection->n(); c++) {
		selection->e(c)->obj->m(objcontrols.e[c]->original_transform->m());
	}


	 //then find and set new transforms

	if (nupinfo->flowtype==NUP_Noflow) {
		//a no-op, nothing to do!

	} else if (nupinfo->flowtype==NUP_Grid) {
		ApplyGrid();

	} else if (nupinfo->flowtype==NUP_Sized_Grid) {
		ApplySizedGrid();

	} else if (nupinfo->flowtype==NUP_Flowed) {
		// ***

	} else if (nupinfo->flowtype==NUP_Random) {
		ApplyRandom();

	} else if (nupinfo->flowtype==NUP_Unclump) {
		 // spread out evenly
		//ApplyUnclump();
	
	} else if (nupinfo->flowtype==NUP_Unoverlap) {
		// *** move just enough to unobscure
	}


	if (updateorig)	for (int c=0; c<selection->n(); c++) {
		objcontrols.e[c]->original_transform->m(selection->e(c)->obj->m());
	}

	//RemapBounds();
	needtodraw=1;
	return 0;
}

void NUpInterface::ApplySizedGrid()
{
	 //strategy:
	 //1. Find how big each row and column must be, recording row/col divider lines for possible manual adjustment
	 //2. find position of object centers
	 //3. apply alignment within that cell


	//double wholew=data->maxx-data->minx;
	//double wholeh=data->maxy-data->miny;

	rowpos.flush();
	colpos.flush();

	flatpoint cc;
	flatpoint p, d;
	double w,h;
	double mm[6];
	transform_identity(mm);

	int i=0;
	int dir=nupinfo->direction;
	double rowheights[nupinfo->rows];
	for (int c=0; c<nupinfo->rows; c++) rowheights[c]=0;
	double colwidths [nupinfo->cols];
	for (int c=0; c<nupinfo->cols; c++) colwidths[c]=0;
	flatpoint dims[selection->n()];
	flatpoint centers[selection->n()];

	int ci,ri, r,c;
	int i1n, i2n;
	if (dir==LAX_LRTB || dir==LAX_RLTB || dir==LAX_LRBT || dir==LAX_RLBT) {
		i1n=nupinfo->rows; i2n=nupinfo->cols; //major dir horizontal
	} else { i2n=nupinfo->rows; i1n=nupinfo->cols; } //major dir vertical

	i=0;
	for (int i1=0; i<selection->n() && i1<i1n; i1++) {
		for (int i2=0; i<selection->n() && i2<i2n; i2++) {
			if (dir==LAX_LRTB || dir==LAX_RLTB || dir==LAX_LRBT || dir==LAX_RLBT) {
				r=i1; c=i2;
			} else { r=i2; c=i1; }

			WidthHeight(selection->e(i), flatpoint(1,0),flatpoint(0,1), &w,&h, &cc);
			centers[i]=cc;
			dims[i].x=w;
			dims[i].y=h;
			ci=c;
			ri=r;

			if (dir==LAX_RLBT || dir==LAX_RLTB || dir==LAX_BTRL || dir==LAX_TBRL) ci=nupinfo->cols-ci-1;
			if (dir==LAX_LRTB || dir==LAX_RLTB || dir==LAX_TBLR || dir==LAX_TBRL) ri=nupinfo->rows-ri-1;

			if (w>colwidths[ci])  colwidths[ci] =w;
			if (h>rowheights[ri]) rowheights[ri]=h;

			i++;
		}
	}

	 //figure out the gaps to make fit in rectangle
	flatpoint xv=data->xaxis(), yv=data->yaxis();
	double hscale, vscale;
	hscale=norm(xv);
	xv/=hscale;
	vscale=norm(yv);
	yv/=vscale;
	double hgap,vgap;
	double sum=0;
	for (int c=0; c<nupinfo->rows; c++) sum+=rowheights[c];
	vgap=(vscale*(data->maxy-data->miny)-sum)/nupinfo->rows;
	sum=0;
	for (int c=0; c<nupinfo->cols; c++) sum+=colwidths[c];
	hgap=(hscale*(data->maxx-data->minx)-sum)/nupinfo->cols;

	i=0;
	for (int i1=0; i<selection->n() && i1<i1n; i1++) {
		for (int i2=0; i<selection->n() && i2<i2n; i2++) {
			if (dir==LAX_LRTB || dir==LAX_RLTB || dir==LAX_LRBT || dir==LAX_RLBT) {
				r=i1; c=i2;
			} else { r=i2; c=i1; }

			ci=c; ri=r; 
			if (dir==LAX_RLBT || dir==LAX_RLTB || dir==LAX_BTRL || dir==LAX_TBRL) ci=nupinfo->cols-ci-1;
			if (dir==LAX_LRTB || dir==LAX_RLTB || dir==LAX_TBLR || dir==LAX_TBRL) ri=nupinfo->rows-ri-1;

			cc=centers[i];
			w=dims[i].x;
			h=dims[i].y;

			p=transform_point(data->m(),data->minx,data->miny);
			for (int pp=0; pp<ci; pp++) p+=(hgap+colwidths[pp])*xv;
			for (int pp=0; pp<ri; pp++) p+=(vgap+rowheights[pp])*yv;
			p+=((hgap+colwidths[ci])/2)*xv + ((vgap+rowheights[ri])/2)*yv;

			//now p is the real position of the center of the cell at ri,ci
			
			 //apply cell alignment
			//p+= (colwidths[ci]-w)/2*(nupinfo->halign-50)/100*xv + (rowheights[ri]-h)/2*(nupinfo->valign-50)/100*yv;

			//p=transform_point(data->m(),p);
			d=p-cc;
			mm[4]=d.x;
			mm[5]=d.y;
			TransformSelection(mm,i,i);
			DBG cerr <<"TRANSFORM major H  "<<i<<" at r,c: "<<r<<','<<c<<"  offset:"<<d.x<<','<<d.y<<endl;

			objcontrols.e[i]->flags=objcontrols.e[i]->flags&~CONTROL_Skip;
			objcontrols.e[i]->new_center=cc+d;

			i++;
		}
	}

	for ( ; i<selection->n(); i++) {
		objcontrols.e[i]->flags|=CONTROL_Skip;
	}
}

/*! For a rectangle centered at cc, with width w and height h, return the distance 
 * from the center to the rectangle edge, along direction v.
 */
double rect_radius(flatpoint cc,double w,double h,flatpoint v)
{
	// ****
	return 0;
}

void NUpInterface::ApplyUnclump()
{
	int maxiterations=100;

	flatpoint p1,p2, dist, force;
	double dd;

	double damp=10;
	double ff=25;
	double wholew=data->maxx-data->minx;
	double wholeh=data->maxy-data->miny;
	double mindist=sqrt(wholew*wholeh/selection->n());
	flatpoint pts[4];
	pts[0]=transform_point(data->m(),data->minx,data->miny);
	pts[1]=transform_point(data->m(),data->maxx,data->miny);
	pts[2]=transform_point(data->m(),data->maxx,data->maxy);
	pts[3]=transform_point(data->m(),data->minx,data->maxy);
	flatpoint datac=(pts[0]+pts[2])/2;

	flatpoint cc, cc1,cc2;
	flatpoint d,v;
	double w,h, w2,h2;
	flatpoint dims[selection->n()];
	flatpoint centers[selection->n()];
	double distbtwn;

	for (int c=0; c<selection->n(); c++) {
		WidthHeight(selection->e(c), flatpoint(1,0),flatpoint(0,1), &dims[c].x,&dims[c].y, &cc);
		objcontrols.e[c]->original_center=cc;
		centers[c]=cc;
	}
	
	for (int iterations=0; iterations<maxiterations; iterations++) {
		for (int c=0; c<selection->n(); c++) {
			d=flatpoint(0,0);

			cc1=centers[c];
			w=dims[c].x;
			h=dims[c].y;
			d.x=d.y=0;

			force=flatpoint(0,0);
			p1=cc1-flatpoint(w/2,h/2);
			p2=cc1+flatpoint(w/2,h/2);

			 //go toward screen
			if (!point_is_in(cc1, pts,4)) {
				v=cc1-datac;
				force+=v/norm(v)*damp;
			}

			for (int c2=0; c2<selection->n(); c2++) {
				if (c==c2) continue;

				WidthHeight(selection->e(c2), flatpoint(1,0),flatpoint(0,1), &w2,&h2, &cc2);
				dist=cc2-cc1;
				dd=norm(dist);
				distbtwn=rect_radius(cc1,w,h,dist)+rect_radius(cc2,w2,h2,dist);
				if (dd<2*(mindist + distbtwn)) force-=ff * dist/(dd*dd);
				if (dd<(mindist + distbtwn)) force-=3*(ff * dist/(dd*dd));
			}

			//forces.e[c]=force;

			cc1+=force;
			centers[c]=cc1;
		}
	}
}

void NUpInterface::ApplyGrid()
{
	double wholew=data->maxx-data->minx;
	double wholeh=data->maxy-data->miny;
	double rh=wholeh;
	double cw=wholew;
	if (nupinfo->rows>=1) rh=wholeh/nupinfo->rows;
	if (nupinfo->cols>=1) cw=wholew/nupinfo->cols;

	flatpoint cc;
	flatpoint p, d;
	double w,h;
	double mm[6];
	transform_identity(mm);

	int i=0;
	int dir=nupinfo->direction;

	if (dir==LAX_TBLR || dir==LAX_BTLR || dir==LAX_TBRL || dir==LAX_BTRL) {
		 //major direction vertical:
		i=0;
		for (int c=0; i<selection->n() && c<nupinfo->cols; c++) {
			for (int r=0; i<selection->n() && r<nupinfo->rows; r++) {
				WidthHeight(selection->e(i), flatpoint(1,0),flatpoint(0,1), &w,&h, &cc);

				if (dir==LAX_TBLR || dir==LAX_TBRL) {
					p.y=data->maxy-rh/2-r*rh;
				} else {
					p.y=data->miny+rh/2+r*rh;
				}
				if (dir==LAX_TBLR || dir==LAX_BTLR) {
					p.x=data->minx+cw/2+c*cw;
				} else {
					p.x=data->maxx-cw/2-c*cw;
				}

				p=transform_point(data->m(),p);
				d=p-cc;
				mm[4]=d.x;
				mm[5]=d.y;
				TransformSelection(mm,i,i);
				DBG cerr <<"TRANSFORM major V  "<<i<<" at r,c: "<<r<<','<<c<<"  offset:"<<d.x<<','<<d.y<<endl;

				objcontrols.e[i]->flags=objcontrols.e[i]->flags&~CONTROL_Skip;
				objcontrols.e[i]->new_center=cc+d;
				i++;
			}
		}
		for ( ; i<selection->n(); i++) {
			objcontrols.e[i]->flags|=CONTROL_Skip;
		}

	} else {
		 //major direction horizontal:
		i=0;
		for (int r=0; i<selection->n() && r<nupinfo->rows; r++) {
			for (int c=0; i<selection->n() && c<nupinfo->cols; c++) {
				WidthHeight(selection->e(i), flatpoint(1,0),flatpoint(0,1), &w,&h, &cc);

				if (dir==LAX_LRTB || dir==LAX_RLTB) {
					p.y=data->maxy-rh/2-r*rh;
				} else {
					p.y=data->miny+rh/2+r*rh;
				}
				if (dir==LAX_LRTB || dir==LAX_LRBT) {
					p.x=data->minx+cw/2+c*cw;
				} else {
					p.x=data->maxx-cw/2-c*cw;
				}

				p=transform_point(data->m(),p);
				d=p-cc;
				mm[4]=d.x;
				mm[5]=d.y;
				TransformSelection(mm,i,i);
				DBG cerr <<"TRANSFORM major H  "<<i<<" at r,c: "<<r<<','<<c<<"  offset:"<<d.x<<','<<d.y<<endl;

				objcontrols.e[i]->flags=objcontrols.e[i]->flags&~CONTROL_Skip;
				objcontrols.e[i]->new_center=cc+d;
				i++;
			}
		}
		for ( ; i<selection->n(); i++) {
			objcontrols.e[i]->flags|=CONTROL_Skip;
		}
	}

}

void NUpInterface::ApplyRandom()
{
	flatpoint cc;
	flatpoint p, d;
	double w,h;
	double mm[6];
	transform_identity(mm);
	double wholew=data->maxx-data->minx;
	double wholeh=data->maxy-data->miny;

	double randomx, randomy;
	for (int c=0; c<selection->n(); c++) {
		WidthHeight(selection->e(c), flatpoint(1,0),flatpoint(0,1), &w,&h, &cc);
		objcontrols.e[c]->original_center=cc;
		d.x=d.y=0;

		if (needtoresetlayout) {
			 //refresh object placement values
			randomx=((double)random()/RAND_MAX);
			randomy=((double)random()/RAND_MAX);
		} else {
			randomx=objcontrols.e[c]->amountx;
			randomy=objcontrols.e[c]->amounty;
		}

		p=data->origin() + (w/2+randomx*(wholew-w)+data->minx)*data->xaxis()
						 + (h/2+randomy*(wholeh-h)+data->miny)*data->yaxis();
		d=p-cc;
		objcontrols.e[c]->amountx=randomx;
		objcontrols.e[c]->amounty=randomy;

		mm[4]=d.x;
		mm[5]=d.y;
		TransformSelection(mm,c,c);
	}
}

} // namespace Laidout

