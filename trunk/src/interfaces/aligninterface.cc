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
#include "aligninterface.h"
#include "../viewwindow.h"
#include "../drawdata.h"

//#include "viewwindow.h"
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>
#include <lax/interfaces/rectpointdefs.h>

#include <lax/refptrstack.cc>
#include <lax/lists.cc>

using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


#define PAD 5
#define fudge 5.0

//align circle control is RADIUS*uiscale
#define RADIUS 2
//align drag bar lengths are BAR*uiscale
#define BAR    12

DBG NumStack<flatpoint> pp1,pp2;

//
//               |-| <- grab tip to rotate snap direction
//               | | <- move bar in snap direction
//               | |
//              /---\ <- drag center to reposition destination
//      --------|   |---------
//              \---/
//               | |
//               | |
//               |-|


//------------------------------------- AlignInfo --------------------------------------
/*! \class AlignInfo
 * \brief Info about how to align collection of objects.
 *
 * center, snap_direction, layout_direction, and path are all in real coordinates, not screen.
 * See also AlignInterface.
 */
/*! \var int AlignInfo::visual_align
 * \brief How to rotate objects so they lay on paths
 * Can be:
 *  - FALIGN_Visual, for just shift, no rotate
 *  - FALIGN_VisualRotate, for shift and rotate without regard for object's actual transform
 *  - FALIGN_ObjectRotate, for forcing the object x axis to conform to line tangent, and y is perpendicular.
 */


AlignInfo::AlignInfo()
{
	name=NULL;
	custom_icon=NULL;

	snap_align_type=FALIGN_Align;
	snapalignment=50;
	snap_direction=flatpoint(0,1);
	visual_align=FALIGN_Visual;

	final_layout_type=FALIGN_None;//flow+gap, random, even placement, aligned
	layout_direction=flatpoint(1,0);
	leftbound=-100;
	rightbound=100; //line parameter for path, dist between vertices is 1
	finalalignment=50;//for when not flow and gap based

	gaps=NULL; //if all custom, final gap is weighted to fit in left/rightbounds?
	defaultgap=0;
	gaptype=0; //whether custom for whole list (weighted or absolute), or single value gap, or function gap

	center=flatpoint(100,100);
	uiscale=10; //width of main alignment bar

	path=NULL; //custom alignment path
}

AlignInfo::~AlignInfo()
{
	if (path) path->dec_count();
	if (name) delete[] name;
}

void AlignInfo::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

    if (what==-1) {
        fprintf(f,"%sname Blah          #optional human readable name\n",spc);
        fprintf(f,"%ssnap align         #Type of snapping, can be align, proportional, or none\n",spc);
        fprintf(f,"%ssnapdir 1,0        #vector of the snapping direction\n",spc);
        fprintf(f,"%ssnapamount 50      #Usually in 0..100, for full left to full right, for instance\n",spc);
        fprintf(f,"%svisualalign shift  #(default) \"shift\" to only shift objects.\n",spc);
        fprintf(f,"%s                   #\"rotate\" to rotate objects visually\n",spc);
        fprintf(f,"%s                   #\"box\" to align object axes to the path tangent\n",spc);
        fprintf(f,"%slayout align       #Type of final layout. Can be align, proportional, none, gaps, grid, random, unoverlap\n",spc);
        fprintf(f,"%slayoutdir 0,1      #default vector of the layout direction\n",spc);
        fprintf(f,"%slayoutamount 50    #Usually in 0..100, for full left to full right, for instance\n",spc);
        fprintf(f,"%sleftbound 0        #One bound as a distance along the path for layout\n",spc);
        fprintf(f,"%srightbound 1       #Another bound as a distance along the path for layout\n",spc);
        fprintf(f,"%scenter 0,0         #Hint for position of control panel\n",spc);
        fprintf(f,"%suiscale 10         #How big to make the panel, the pixel width of an alignment bar\n",spc);
        fprintf(f,"%sdefaultgap 5       #\n",spc);
        //fprintf(f,"%sgaptype ???        #\n",spc);
        fprintf(f,"%sgaps 4 5 6 7       #List of current gaps between objects\n",spc);
        fprintf(f,"%spath               #a PathsData object defining the layout path, if not just along layoutdir\n",spc);
        fprintf(f,"%s  ...              \n",spc);
        return;
    }


    //fprintf(f,"%slabelbase ",spc);
    //dump_out_escaped(f,labelbase,-1);
    //fprintf(f,"\n");

	if (name) fprintf(f,"%sname %s\n",spc,name);

	if (snap_align_type==FALIGN_Align) fprintf(f,"%ssnap align\n",spc);
	else if (snap_align_type==FALIGN_Proportional) fprintf(f,"%ssnap proportional\n",spc);
	else if (snap_align_type==FALIGN_None) fprintf(f,"%ssnap none\n",spc);
	fprintf(f,"%ssnapdir %.10g,%.10g\n",spc,snap_direction.x,snap_direction.y);
	fprintf(f,"%ssnapamount %.10g\n",spc,snapalignment);
	fprintf(f,"%svisualalign shift\n",spc); //*** shift, rotate, or box

	if (snap_align_type==FALIGN_Align) fprintf(f,"%slayout align\n",spc);
	else if (snap_align_type==FALIGN_Proportional) fprintf(f,"%slayout proportional\n",spc);
	else if (snap_align_type==FALIGN_None) fprintf(f,"%slayout none\n",spc);
	else if (snap_align_type==FALIGN_Random) fprintf(f,"%slayout random\n",spc);
	else if (snap_align_type==FALIGN_Gap) fprintf(f,"%slayout gaps\n",spc);
	else if (snap_align_type==FALIGN_Grid) fprintf(f,"%slayout grid\n",spc);
	else if (snap_align_type==FALIGN_Unoverlap) fprintf(f,"%slayout unoverlap\n",spc);
	fprintf(f,"%slayoutdir %.10g,%.10g\n",spc,layout_direction.x,layout_direction.y);
	fprintf(f,"%slayoutamount %.10g\n",spc,finalalignment);

	fprintf(f,"%sleftbound %.10g\n",spc,leftbound);
	fprintf(f,"%srightbound %.10g\n",spc,rightbound);
	fprintf(f,"%scenter %.10g,%.10g\n",spc,center.x,center.y);
	fprintf(f,"%suiscale %.10g\n",spc,uiscale);

	fprintf(f,"%sdefaultgap %.10g\n",spc,defaultgap);
	//fprintf(f,"%sgaptype ???        #\n",spc);
	if (gaps) {
		//fprintf(f,"%sgaps 4 5 6 7       #List of current gaps between objects\n",spc);
	}
	if (path) {
		fprintf(f,"%spath\n",spc);
		path->dump_out(f,indent+2,0,context);
	}

}

void AlignInfo::dump_in_atts(LaxFiles::Attribute*, int, Laxkit::anObject*)
{
}



//------------------------------------- AlignInterface --------------------------------------
	
/*! \class AlignInterface 
 * \brief Interface to define and modify n-up style arrangements. See also AlignInfo.
 */


AlignInterface::AlignInterface(int nid,Displayer *ndp,Document *ndoc)
	: ObjectInterface(nid,ndp) 
{
	controlcolor.rgbf(.5,.5,.5,1);
	patheditcolor.rgbf(0,.7,0,1);

	style|=RECT_HIDE_CONTROLS;
	showdecs=0;
	firsttime=1;
	active=0;
	snapto_lrc_amount=0; //pixel distance to snap to the standard left/top, center, or right/bottom points, 0 for none
	boundstep=.1;

	hover=-1;
	hoverindex=-1;
	needtoresetlayout=1;

	aligninfo=new AlignInfo;
	//SetupBoxes();
}

AlignInterface::AlignInterface(anInterface *nowner,int nid,Displayer *ndp)
	: ObjectInterface(nid,ndp) 
{
	controlcolor.rgbf(.5,.5,.5,1);
	patheditcolor.rgbf(0,.7,0,1);

	style|=RECT_HIDE_CONTROLS;
	showdecs=0;
	firsttime=1;
	active=0;
	snapto_lrc_amount=0; //pixel distance to snap to the standard left/top, center, or right/bottom points, 0 for none
	boundstep=.1;

	aligninfo=new AlignInfo;
	//SetupBoxes();
}

AlignInterface::~AlignInterface()
{
	DBG cerr <<"AlignInterface destructor.."<<endl;

	if (aligninfo) aligninfo->dec_count();
}

//scan/hover ids
#define ALIGN_None                1000
#define ALIGN_Move                1001
#define ALIGN_RotateSnapDir       1002
#define ALIGN_RotateAlignDir      1003
#define ALIGN_RotateSnapAndAlign  1004
#define ALIGN_MoveSnapAlign       1005
#define ALIGN_MoveFinalAlign      1006
#define ALIGN_MoveGap             1007
#define ALIGN_MoveGrid            1008
#define ALIGN_MoveLeftBound       1009
#define ALIGN_MoveRightBound      1010
#define ALIGN_LayoutType          1011
#define ALIGN_Path                1012
#define ALIGN_Randomize           1013
#define ALIGN_LineControl         1014


//void AlignInterface::createControls()
//{
//	controls.push(new ActionArea(ALIGN_Move              , AREA_Handle, NULL, _("Drag to move alignment tool"),0,1,controlcolor,0));
//	controls.push(new ActionArea(ALIGN_RotateSnapDir     , AREA_Handle, NULL, _("Drag to rotate alignment"),0,1,controlcolor,0));
//	controls.push(new ActionArea(ALIGN_RotateAlignDir    , AREA_Handle, NULL, _("Drag to rotate layout direction"),0,1,controlcolor,0));
//	controls.push(new ActionArea(ALIGN_RotateSnapAndAlign, AREA_Handle, NULL, _("Drag to rotate alignment"),0,1,controlcolor,0));
//	controls.push(new ActionArea(ALIGN_MoveSnapAlign     , AREA_Handle, NULL, _("Drag to move snap alignment"),0,1,controlcolor,0));
//	controls.push(new ActionArea(ALIGN_MoveFinalAlign    , AREA_Handle, NULL, _("Drag to move layout alignment"),0,1,controlcolor,0));
//	controls.push(new ActionArea(ALIGN_MoveGap           , AREA_Handle, NULL, _("Drag to resize gap"),0,1,controlcolor,0));
//	controls.push(new ActionArea(ALIGN_MoveGrid          , AREA_Handle, NULL, _("Drag to resize positions"),0,1,controlcolor,0));
//	controls.push(new ActionArea(ALIGN_MoveLeftBound     , AREA_Handle, NULL, _("Drag to move boundary"),0,1,controlcolor,0));
//	controls.push(new ActionArea(ALIGN_MoveRightBound    , AREA_Handle, NULL, _("Drag to move boundary"),0,1,controlcolor,0));
//
//
//}
//
//void AlignInterface::remapControls()
//{
////	 //draw snap to path
////	dp->drawline(aligninfo->center+aligninfo->leftbound*aligninfo->layout_direction,
////				 aligninfo->center+aligninfo->rightbound*aligninfo->layout_direction);
//
//	 //snap direction rectangle
//	double w=aligninfo->uiscale;
//
//	flatpoint v=aligninfo->snap_direction;
//	flatpoint vt=transpose(aligninfo->snap_direction);
//	flatpoint p1=aligninfo->center - w/2*vt - w*RADIUS*v - w*(aligninfo->snapalignment/100*(BAR-2*RADIUS))*v;
//	flatpoint p2=p1+vt*w;
//	flatpoint p3=p1+w*BAR*aligninfo->snap_direction;
//	flatpoint p4=p3+vt*w;
//		
//	dp->drawline(p1,p2);
//	dp->drawline(p2,p4);
//	dp->drawline(p4,p3);
//	dp->drawline(p3,p1);
//
//	 //draw rotate handles on ends of bar
//	dp->drawline(p1,p1-v*w); //the one
//	dp->drawline(p1-v*w,p1-v*w+w*vt);
//	dp->drawline(p1-v*w+w*vt,p2);
//
//	dp->drawline(p3,p3+v*w); //and the other
//	dp->drawline(p3+v*w,p3+v*w+w*vt);
//	dp->drawline(p3+v*w+w*vt,p4);
//
//	***
//}

const char *AlignInterface::Name()
{ return _("Align"); }


//Context menu ids, MUST NOT CONFLICT with the ALIGN_* for scan()
#define ALIGN_Save                   1
#define ALIGN_Load                   2

#define ALIGN_Aligned                3
#define ALIGN_AlignedProportional    4
#define ALIGN_Grid                   5
#define ALIGN_Gaps                   6
#define ALIGN_Random                 7
#define ALIGN_Unoverlap              8
#define ALIGN_Final_None             9

#define ALIGN_Snap_None              10
#define ALIGN_Snap_Align             11
#define ALIGN_Snap_AlignProportional 12

/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *AlignInterface::ContextMenu(int x,int y,int deviceid)
{
	MenuInfo *menu=new MenuInfo(_("Align Interface"));

	//menu->AddItem(_("Save"),ALIGN_Save); // *** save to <laidout->config_dir>/tools/AlignInterface/*
	//menu->AddItem(_("Load"),ALIGN_Load);
	//presets...
	//menu->AddSep(_("Presets"));

	menu->AddSep(_("Snap"));
	menu->AddItem(_("None"),ALIGN_Snap_None, LAX_ISTOGGLE|(aligninfo->snap_align_type==FALIGN_None?LAX_CHECKED:0));
	menu->AddItem(_("Aligned"),ALIGN_Snap_Align, LAX_ISTOGGLE|(aligninfo->snap_align_type==FALIGN_Align?LAX_CHECKED:0));
	menu->AddItem(_("Aligned proportionally"),ALIGN_Snap_AlignProportional, LAX_ISTOGGLE|(aligninfo->snap_align_type==FALIGN_Proportional?LAX_CHECKED:0));

	menu->AddSep(_("Final"));
	menu->AddItem(_("None"),ALIGN_Final_None, LAX_ISTOGGLE|(aligninfo->final_layout_type==FALIGN_None?LAX_CHECKED:0));
	menu->AddItem(_("Aligned"),ALIGN_Aligned, LAX_ISTOGGLE|(aligninfo->final_layout_type==FALIGN_Align?LAX_CHECKED:0));
	menu->AddItem(_("Aligned proportionally"), ALIGN_AlignedProportional, LAX_ISTOGGLE|(aligninfo->final_layout_type==FALIGN_Proportional?LAX_CHECKED:0));
	menu->AddItem(_("Grid"), ALIGN_Grid, LAX_ISTOGGLE|(aligninfo->final_layout_type==FALIGN_Gap?LAX_CHECKED:0));
	menu->AddItem(_("Gaps"), ALIGN_Gaps, LAX_ISTOGGLE|(aligninfo->final_layout_type==FALIGN_Grid?LAX_CHECKED:0));
	menu->AddItem(_("Random"), ALIGN_Random, LAX_ISTOGGLE|(aligninfo->final_layout_type==FALIGN_Random?LAX_CHECKED:0));
	menu->AddItem(_("Unoverlap"), ALIGN_Unoverlap, LAX_ISTOGGLE|(aligninfo->final_layout_type==FALIGN_Unoverlap?LAX_CHECKED:0));

	//Visual align, or rotate so object axes are in line with path, or rotate visually only as path bends
	//
	//menu->AddSep(_("Finishing")); <- *** make this an overlay panel?
	//menu->AddDouble(_("Rotation"),0);
	//menu->AddDouble(_("X shift"),0);
	//menu->AddDouble(_("Y shift"),0);
	//menu->AddDouble(_("X scale"),1);
	//menu->AddDouble(_("Y scale"),1);
	//menu->AddDouble(_("Shear"),0);
	//menu->AddItem(_("Random Rotation"),0); //...or random within range
	//menu->AddItem(_("Random X shift"),0);
	//menu->AddItem(_("Random Y shift"),0);
	//menu->AddItem(_("Random X scale"),1);
	//menu->AddItem(_("Random Y scale"),1);
	//menu->AddItem(_("Random Shear"),0);

	return menu;
}

int AlignInterface::UpdateFromPath()
{
	DBG cerr <<" ** ** ** ** align path modified!!!"<<endl;

	 //update control position
	//double d=0;
	//flatpoint p=ClosestPoint(aligninfo->center,&d);
	//aligninfo->center=p;
	//needtodraw=1;

	if (active) ApplyAlignment(0);

	return 0;
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int AlignInterface::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"child")) {
		UpdateFromPath();
		return 0;
	}

	if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i=s->info2; //id of menu item

		int fa=-1, sa=-1; 
		if (i==ALIGN_Final_None)     fa=FALIGN_None;
		else if (i==ALIGN_Aligned)   fa=FALIGN_Align;
		else if (i==ALIGN_AlignedProportional) fa=FALIGN_Proportional;
		else if (i==ALIGN_Grid)      fa=FALIGN_Grid;
		else if (i==ALIGN_Gaps)      fa=FALIGN_Gap;
		else if (i==ALIGN_Random)    fa=FALIGN_Random;
		else if (i==ALIGN_Unoverlap) fa=FALIGN_Unoverlap;

		else if (i==ALIGN_Snap_None)  sa=FALIGN_None;
		else if (i==ALIGN_Snap_Align) sa=FALIGN_Align;
		else if (i==ALIGN_Snap_AlignProportional) sa=FALIGN_Proportional;
		
		if (fa>=0) aligninfo->final_layout_type=fa;
		if (sa>=0) aligninfo->snap_align_type=sa;
		if ((sa || fa) && active) ApplyAlignment(0);

		return 0;
	}
	return 1;
}

//! Use a Document.
int AlignInterface::UseThis(Laxkit::anObject *ndata,unsigned int mask)
{
	return 0;
}

/*! Will say it cannot draw anything.
 */
int AlignInterface::draws(const char *atype)
{ return 0; }


//! Return a new AlignInterface if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *AlignInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new AlignInterface(id,NULL);
	else if (!dynamic_cast<AlignInterface *>(dup)) return NULL;
	
	return ObjectInterface::duplicate(dup);
}


int AlignInterface::InterfaceOn()
{
	DBG cerr <<"pagerangeinterfaceOn()"<<endl;

	showdecs=2;
	needtodraw=1;
	return 0;
}

int AlignInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void AlignInterface::Clear(SomeData *d)
{
}

//! Draw an alignment slider rectangle along line through aligninfo->center in dir direction.
/*! Draw thick lines for hover.
 *
 * \todo not draw rect when in the circle?
 */
void AlignInterface::DrawAlignBox(flatpoint dir,
								  double amount,
								  int aligntype, //!< If to draw arrows at tips to indicate proportional alignment, or thin rect for none
								  int with_rotation_handles, //!< Whether to add little boxes at ends of rectangle
								  int hover //!< 0 for none, 1 for bar, 2 for rotation handle
								 )
{
	double w=aligninfo->uiscale;

	flatpoint v=transform_vector(dp->Getctm(),dir);  v/=norm(v);
	flatpoint vt=transpose(v);
	flatpoint p1=dp->realtoscreen(aligninfo->center) - w/2*vt - w*RADIUS*v - w*(amount/100*(BAR-2*RADIUS))*v;
	flatpoint p2=p1+vt*w;
	flatpoint p3=p1+w*BAR*v;
	flatpoint p4=p3+vt*w;

	 //draw rectangle
	if (aligntype==FALIGN_None) {
		dp->drawline(p1+.4*w*vt,p2-.4*w*vt);
		dp->drawline(p2-.4*w*vt,p4-.4*w*vt);
		dp->drawline(p4-.4*w*vt,p3+.4*w*vt);
		dp->drawline(p3+.4*w*vt,p1+.4*w*vt);

	} else if (aligntype==FALIGN_Proportional) {
		if (hover==1) dp->LineAttributes(2,LineSolid, CapButt, JoinMiter);
		else dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);

		flatpoint p[12];
		p[ 0]=(with_rotation_handles ? p1 : p1-w/2*v+w/2*vt);
		p[ 1]=p1 +w*v -w*vt;
		p[ 2]=p1 +w*v      ;
		p[ 3]=p3 -w*v      ;
		p[ 4]=p3 -w*v -w*vt;
		p[ 5]=(with_rotation_handles ? p3 : p3+w/2*v+w/2*vt);
		p[ 6]=(with_rotation_handles ? p4 : p3+w/2*v+w/2*vt);
		p[ 7]=p4 -w*v +w*vt;
		p[ 8]=p4 -w*v      ;
		p[ 9]=p2 +w*v      ;
		p[10]=p2 +w*v +w*vt;
		p[11]=(with_rotation_handles ? p2 : p1-w/2*v+w/2*vt);
		dp->drawlines(p,12,1,0);

	} else {//FALIGN_Align
		if (hover==1) dp->LineAttributes(2,LineSolid, CapButt, JoinMiter);
		else dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);

		dp->drawline(p1,p2);
		dp->drawline(p2,p4);
		dp->drawline(p4,p3);
		dp->drawline(p3,p1);
	}

	 //draw rotate handles on ends of bar
	if (with_rotation_handles) {
		if (hover==2) dp->LineAttributes(2,LineSolid, CapButt, JoinMiter);
		else dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);

		dp->drawline(p1,p1-v*w); //the one
		dp->drawline(p1-v*w,p1-v*w+w*vt);
		dp->drawline(p1-v*w+w*vt,p2);
		dp->drawline(p3,p3+v*w); //and the other
		dp->drawline(p3+v*w,p3+v*w+w*vt);
		dp->drawline(p3+v*w+w*vt,p4);
	}

	dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
}

/*! Draws maybebox if any, then DrawGroup() with the current papergroup.
 */
int AlignInterface::Refresh()
{
	if (!needtodraw) return 0;

	RectInterface::Refresh();
	needtodraw=0;

	if (firsttime) {
		firsttime=0;
	}

	DBG cerr <<"AlignInterface::Refresh()..."<<endl;

	dp->DrawScreen();
	dp->NewFG(&controlcolor);
	dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);


	if (!child) {
		//only draw the controls when not editing path

		 //draw snap to path
		if (!aligninfo->path) {
			if (hover==ALIGN_Path) dp->LineAttributes(3,LineSolid, CapButt, JoinMiter);
			//dp->drawline(aligninfo->center+aligninfo->leftbound*aligninfo->layout_direction, <- screen, not real
			//			 aligninfo->center+aligninfo->rightbound*aligninfo->layout_direction);
			dp->drawline(dp->realtoscreen(aligninfo->center+aligninfo->leftbound*aligninfo->layout_direction),
						 dp->realtoscreen(aligninfo->center+aligninfo->rightbound*aligninfo->layout_direction));
			dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
		} else {
			 // *** draw the PathsData
			if (!child) {
				LineStyle ls(*aligninfo->path->linestyle);
				ls.width=(hover==ALIGN_Path?3:1);
				::DrawData(dp,aligninfo->path,&ls,NULL,0);
				dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
				dp->NewFG(&controlcolor);
				dp->DrawScreen();
			}
		}

		 //draw snap direction rectangle
		DrawAlignBox(aligninfo->snap_direction, aligninfo->snapalignment,aligninfo->snap_align_type,1,
						hover==ALIGN_MoveSnapAlign?1:(hover==ALIGN_RotateSnapAndAlign?2:0));


		 //draw layout direction rectangle, or other final layout items
		if (aligninfo->final_layout_type==FALIGN_Align ||aligninfo->final_layout_type==FALIGN_Proportional) { 
			 //rect
			DrawAlignBox(aligninfo->layout_direction, aligninfo->finalalignment,aligninfo->final_layout_type,0,
						hover==ALIGN_MoveFinalAlign?1:0);

		} else {
			 //other layout types selector
			double w=aligninfo->uiscale;
			double x1=1/sqrt(2)*w*RADIUS;
			double x2=(aligninfo->final_layout_type==FALIGN_None?1.5:2)*w*RADIUS;
			double yy=x1;
			flatpoint v=transform_vector(dp->Getctm(),aligninfo->layout_direction);
			v/=norm(v);
			flatpoint vt=transpose(v);
			//flatpoint p1(x1,yy), p2(x2,yy);
			flatpoint cc2,cc(dp->realtoscreen(aligninfo->center));


			 //left side
			if (hover==ALIGN_LayoutType) dp->LineAttributes(3,LineSolid, CapButt, JoinMiter);
			dp->drawline(cc + x1*v + yy*vt,  cc + x2*v + yy*vt);
			dp->drawline(cc + x2*v + yy*vt,  cc + x2*v - yy*vt);
			dp->drawline(cc + x2*v - yy*vt,  cc + x1*v - yy*vt);
			 //right side
			dp->drawline(cc - x1*v + yy*vt,  cc - x2*v + yy*vt);
			dp->drawline(cc - x2*v + yy*vt,  cc - x2*v - yy*vt);
			dp->drawline(cc - x2*v - yy*vt,  cc - x1*v - yy*vt);
			if (hover==ALIGN_LayoutType) dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);

			const char *buf;
			if (aligninfo->final_layout_type==FALIGN_Gap) {
				buf="|-|";
			} else if (aligninfo->final_layout_type==FALIGN_Grid) { 
				buf="#";
			} else if (aligninfo->final_layout_type==FALIGN_Random) { 
				buf="?";
			} else if (aligninfo->final_layout_type==FALIGN_Unoverlap) { 
				buf="oO";
			} else buf="";

			dp->NewFG(&controlcolor);
			cc2=cc+(1.5*w*RADIUS*v);
			dp->textout(cc2.x,cc2.y, buf,-1, LAX_CENTER);
			cc2=cc-(1.5*w*RADIUS*v);
			dp->textout(cc2.x,cc2.y, buf,-1, LAX_CENTER);
		}


		 //draw control circle
		flatpoint cc(dp->realtoscreen(aligninfo->center));
		if (active) dp->NewFG(0,200,0); else dp->NewFG(255,100,100);
		dp->LineAttributes(3,LineSolid, CapButt, JoinMiter);
		dp->drawellipse(cc.x,cc.y,
						aligninfo->uiscale*RADIUS,aligninfo->uiscale*RADIUS,
						0,2*M_PI,
						0);


		 //draw left and right bounds
		dp->NewFG(&controlcolor);
		dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
		flatpoint p;
		if (!child) {
			PointAlongPath(aligninfo->leftbound,0, p, NULL);
			dp->drawpoint(dp->realtoscreen(p),5,hover==ALIGN_MoveLeftBound);
			PointAlongPath(aligninfo->rightbound,0, p, NULL);
			dp->drawpoint(dp->realtoscreen(p),5,hover==ALIGN_MoveRightBound);
		}


		 //draw indicator from original position to current position
		DBG if (pp1.n>=selection.n) for (int c=0; c<selection.n; c++) {
		DBG 	if (aligninfo->final_layout_type==FALIGN_None) dp->drawline(dp->realtoscreen(pp1.e[c]),dp->realtoscreen(pp2.e[c]));
		DBG 	dp->drawpoint(dp->realtoscreen(pp2.e[c]),3,0);
		DBG }


		 //draw extra controls
		for (int c=0; c<controls.n; c++) {
			p=dp->realtoscreen(controls.e[c]);
			if (aligninfo->final_layout_type==FALIGN_Random) {
				if (hover==ALIGN_Randomize && hoverindex==c) {
					dp->NewFG(coloravg(dp->BG(),dp->FG(), .5));
					dp->drawpoint(p, dp->textheight()/2,1);
				}
				dp->NewFG(&controlcolor);
				dp->textout(p.x,p.y, "?",1, LAX_CENTER);

			} else if (aligninfo->final_layout_type==FALIGN_Grid) {
				if (hover==ALIGN_MoveGrid && hoverindex==c) {
					dp->NewFG(coloravg(dp->BG(),dp->FG(), .5));
					dp->drawpoint(p, dp->textheight()/2,1);
				}
				dp->NewFG(&controlcolor);
				dp->textout(p.x,p.y, "#",1, LAX_CENTER);
			}
		}
	} //if to draw any controls


	dp->DrawReal();

	return 1;
}

//! Return which alignment handle mouse is over
/*! If state&ControlMask, then search for line controls first.
 */
int AlignInterface::scan(int x,int y, int &index, unsigned int state)
{
	flatpoint fp(x,y);


	 //optionally scan for extra line controls first off, since these should be accessible on top of everything
	if (state&ControlMask) {
		int a=scanForLineControl(x,y,index);
		if (a!=ALIGN_None) return a;
	}

	fp-=dp->realtoscreen(aligninfo->center);
	if (norm(fp)<aligninfo->uiscale*RADIUS) return ALIGN_Move; //align move circle


	 //transform in snap direction 
	double w=aligninfo->uiscale;
	flatpoint v=transform_vector(dp->Getctm(),aligninfo->snap_direction);
	v/=norm(v);
	flatpoint vt=transpose(v);

	double xx,yy;
	xx=vt*fp;
	yy=v*fp;



	 //scan for controls along snap direction
	if (xx>=-w/2 && xx<=w/2) {
	 	 //scan for snap alignment bar and rotate handles
		double barbottom=-w*RADIUS - w*(aligninfo->snapalignment/100*(BAR-2*RADIUS));
		double bartop   =barbottom + w*BAR;
		if (yy>=barbottom && yy<bartop) {
			 //on the bar
			return ALIGN_MoveSnapAlign;
		} else if ((yy<=barbottom && yy>barbottom-w) || (yy>bartop && yy<bartop+w)) {
			 //on a bar rotate handle
			//if ((state&LAX_STATE_MASK)==ShiftMask) return ALIGN_RotateSnapDir;
			return ALIGN_RotateSnapAndAlign;
		}
	}


	 // scan for layout type controls:
	 //find transform in layout direction 
	vt=transform_vector(dp->Getctm(),aligninfo->layout_direction);
	vt/=norm(vt);
	v=transpose(vt);
	xx=vt*fp;
	yy=v*fp;

	 //scan for controls along layout direction
	if (aligninfo->final_layout_type==FALIGN_Align || aligninfo->final_layout_type==FALIGN_Proportional) {
		 //scan for layout alignment bar 
		if (yy>=-w/2 && yy<=w/2) {
			double barbottom=-w*RADIUS - w*(aligninfo->finalalignment/100*(BAR-2*RADIUS));
			double bartop   =barbottom + w*BAR;
			if (xx>=barbottom && xx<bartop) {
				 //on the bar
				return ALIGN_MoveFinalAlign;
			}
		}
	} else {
		 //scan for layout type 
		if (yy<w*RADIUS && yy>-w*RADIUS && ((xx>-2*w*RADIUS && xx<-w*RADIUS) || (xx<2*w*RADIUS && xx>w*RADIUS)))
			return ALIGN_LayoutType;
	}

	 // *** scan for particular controls of final layout type, like gaps, grid points, random bits
	//gaps
	//grid points
	//randomize buttons


	 //scan for left boundary
	flatpoint p;
	PointAlongPath(aligninfo->leftbound,0, p, NULL);
	if (norm(dp->realtoscreen(p)-flatpoint(x,y))<PAD) {
		return ALIGN_MoveLeftBound;
	}

	 //scan for right boundary
	PointAlongPath(aligninfo->rightbound,0, p, NULL);
	if (norm(dp->realtoscreen(p)-flatpoint(x,y))<PAD) {
		return ALIGN_MoveRightBound;
	}

	 //if not search for line controls first, search for them now
	if (!(state&ControlMask)) {
		int a=scanForLineControl(x,y,index);
		if (a!=ALIGN_None) return a;
	}

	 //scan for path
	if (!child && onPath(x,y)) return ALIGN_Path;

	return RectInterface::scan(x,y);
}

int AlignInterface::scanForLineControl(int x,int y, int &index)
{
	flatpoint fp(x,y);
	for (int c=0; c<controls.n; c++) {
		if (norm(dp->realtoscreen(controls.e[c])-fp)<PAD) {
			index=c;
			if (aligninfo->final_layout_type==FALIGN_Random) return ALIGN_Randomize;
			if (aligninfo->final_layout_type==FALIGN_Grid) return ALIGN_MoveGrid;
			return ALIGN_LineControl;
		}
	}
	return ALIGN_None;
}

int AlignInterface::onPath(int x,int y)
{
	if (!aligninfo->path) {
		 //find distance to the segment from left to right
		int dist=distance(flatpoint(x,y),
						  dp->realtoscreen(aligninfo->center+aligninfo->leftbound *aligninfo->layout_direction),
						  dp->realtoscreen(aligninfo->center+aligninfo->rightbound *aligninfo->layout_direction));
		if (dist<PAD) return 1;
		return 0;
	}

	 //else check for point on PathsData
	flatpoint fp(x,y);
	flatpoint closest=ClosestPoint(dp->screentoreal(fp),NULL);
	if (norm(fp-dp->realtoscreen(closest))<PAD) return 1;
	
	return 0;
}

//! Remove a child interface. In this case,a PathInterface.
/*! Redefine to change the path color from green back to control color.
 */
int AlignInterface::RemoveChild()
{
	int c=anInterface::RemoveChild();
	if (!aligninfo->path) return c;
	aligninfo->path->line(-1,-1,-1,&controlcolor); //draw path in the boring color again, not the edit color

	 //update panel position
	double d=0;
	flatpoint p=ClosestPoint(aligninfo->center,&d);
	aligninfo->center=p;
	ClampBoundaries(0);
	ApplyAlignment(0);
	needtodraw=1;

	viewport->postmessage("");

	return c;
}

int AlignInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.isdown(0,LEFTBUTTON)) return 1;

    buttondown.down(d->id,LEFTBUTTON,x,y);
    //dragmode=DRAG_NONE;
    
	
	int index=-1;
	int over=scan(x,y, index,state);
	if (over==RP_None) over=ALIGN_None;

	if (over<ALIGN_None && data) { //unobscured click on a rect control
		 //there is already a selection
		style|=RECT_OBJECT_SHUNT;
		RectInterface::LBDown(x,y,state,count,d);

		int curpoint;
		buttondown.getextrainfo(d->id,LEFTBUTTON,&curpoint);
		if (curpoint!=RP_None && !(curpoint==RP_Move && !PointInSelection(x,y))) return 0;
	}

	if (child && over==ALIGN_Move) {
		RemoveChild();
	}

	if (over==ALIGN_Move && (!active) && count==2) {
		DBG pp1.flush();
		DBG pp2.flush();

		active=1;
		ApplyAlignment(1);
		needtodraw=1;
		return 0;
	}


	buttondown.down(d->id,LEFTBUTTON,x,y,over,index);
	//flatpoint fp=dp->screentoreal(x,y);

	if ((state&LAX_STATE_MASK)==ShiftMask && over<0) {
	}

	if (count==2) { // ***
	}

	return 0;
}

int AlignInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	int action=-1,i2=-1;
	int dragged=buttondown.up(d->id,LEFTBUTTON,&action,&i2);

	if (action<ALIGN_None) return RectInterface::LBUp(x,y,state,d);

	if (!dragged) {
		if (action==ALIGN_Move) {
			active=!active;
			if (active) ApplyAlignment(0);
			else ResetAlignment();
			needtodraw=1;
			return 0;

		} else if (action==ALIGN_Randomize) {
			needtoresetlayout=1;
			hover=0;
			ApplyAlignment(0);
			needtodraw=1;
			return 0;

		} else if (action==ALIGN_Path) {
			if (child) return 0;
			
			if (!aligninfo->path) createPath();
			aligninfo->path->line(-1,-1,-1,&patheditcolor);

			PathInterface *pathi=new PathInterface(getUniqueNumber(), dp);
			pathi->pathi_style=PATHI_One_Path_Only|PATHI_Esc_Off_Sub|PATHI_Two_Point_Minimum|PATHI_Path_Is_M_Real;
			pathi->primary=1;
			VObjContext voc;
			voc.SetObject(aligninfo->path);
			pathi->UseThisObject(&voc);//copies voc

			child=pathi;
			pathi->owner=this;
			viewport->Push(pathi,1,-1);
			hover=0;
			needtodraw=1;

			viewport->postmessage(_("Press Enter to finish editing"));
			return 0;

		}

		if ((state&LAX_STATE_MASK)==ControlMask) {
		} else {
		}
	}

	//***
	//if (curbox) { curbox->dec_count(); curbox=NULL; }
	//if (curboxes.n) curboxes.flush();

	return 0;
}

//! Create aligninfo->path if it does not exist.
/*! Return 1 for path existed. 0 for new path created.
 */
int AlignInterface::createPath()
{
	if (aligninfo->path) return 1;

	PathsData *path=new PathsData;

	path->append(aligninfo-> leftbound*aligninfo->layout_direction + aligninfo->center);
	path->append(aligninfo->rightbound*aligninfo->layout_direction + aligninfo->center);
	path->FindBBox();

	double d=aligninfo->rightbound-aligninfo->leftbound;
	aligninfo->leftbound=0;
	aligninfo->rightbound=d;

	path->line(1,CapButt,JoinMiter,&patheditcolor);
	path->linestyle->widthtype=0;
	aligninfo->path=path;

	return 0;
}

int AlignInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int index=-1;
	int over=scan(x,y, index, state);
	if (over<=ALIGN_None) return 1;

	if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) {
		aligninfo->uiscale*=1.05;
		needtodraw=1;
		return 0;
	}

	if (over==ALIGN_MoveSnapAlign) {
		int t=aligninfo->snap_align_type;
		if (t==FALIGN_None) t=FALIGN_Align;
		else if (t==FALIGN_Align) t=FALIGN_Proportional;
		else if (t==FALIGN_Proportional) t=FALIGN_None;

		aligninfo->snap_align_type=t;
		if (active) ApplyAlignment(0);
		needtodraw=1;
		return 0;
	}

	if (over==ALIGN_LayoutType || over==ALIGN_MoveFinalAlign) {
		int t=aligninfo->final_layout_type;
		if (t==FALIGN_None) t=FALIGN_Align;
		else if (t==FALIGN_Align) t=FALIGN_Proportional;
		else if (t==FALIGN_Proportional) t=FALIGN_Gap;
		else if (t==FALIGN_Gap) t=FALIGN_Grid;
		else if (t==FALIGN_Grid) t=FALIGN_Random;
		else if (t==FALIGN_Random) t=FALIGN_Unoverlap;
		else if (t==FALIGN_Unoverlap) t=FALIGN_None;

		aligninfo->final_layout_type=t;
		needtoresetlayout=1;
		if (active) ApplyAlignment(0);
		needtodraw=1;
		return 0;
	}

	return 0;
}

int AlignInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int index=-1;
	int over=scan(x,y, index, state);
	if (over<=ALIGN_None) return 1;

	if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) {
		aligninfo->uiscale*=.95;
		needtodraw=1;
		return 0;
	}

	if (over==ALIGN_MoveSnapAlign) {
		int t=aligninfo->snap_align_type;
		if (t==FALIGN_None) t=FALIGN_Proportional;
		else if (t==FALIGN_Proportional) t=FALIGN_Align;
		else if (t==FALIGN_Align) t=FALIGN_None;

		aligninfo->snap_align_type=t;
		if (active) ApplyAlignment(0);
		needtodraw=1;
		return 0;
	}

	if (over==ALIGN_LayoutType || over==ALIGN_MoveFinalAlign) {
		int t=aligninfo->final_layout_type;
		if (t==FALIGN_None) t=FALIGN_Unoverlap;
		else if (t==FALIGN_Align) t=FALIGN_None;
		else if (t==FALIGN_Proportional) t=FALIGN_Align;
		else if (t==FALIGN_Gap) t=FALIGN_Proportional;
		else if (t==FALIGN_Grid) t=FALIGN_Gap;
		else if (t==FALIGN_Random) t=FALIGN_Grid;
		else if (t==FALIGN_Unoverlap) t=FALIGN_Random;

		aligninfo->final_layout_type=t;
		needtoresetlayout=1;
		if (active) ApplyAlignment(0);
		needtodraw=1;
		return 0;
	}

	return 0;
}

void AlignInterface::postHoverMessage()
{
	const char *m;
	switch (hover) {
		case ALIGN_None	: m=NULL; break;
		case ALIGN_Move:  m=_("Move alignment controller"); break;
		case ALIGN_RotateSnapDir: m=_("Rotate snap direction"); break;
		case ALIGN_RotateAlignDir: m=_("Rotate layout direction"); break;
		case ALIGN_RotateSnapAndAlign: m=_("Rotate"); break;
		case ALIGN_MoveSnapAlign: m=_("Move snap alignment"); break;
		case ALIGN_MoveFinalAlign: m=_("Move final alignment"); break;
		case ALIGN_MoveGap: m=_("Drag to adjust gap"); break;
		case ALIGN_MoveGrid: m=_("Drag to change grid size"); break;
		case ALIGN_MoveLeftBound: m=_("Move boundary"); break;
		case ALIGN_MoveRightBound: m=_("Move boundary"); break;
		case ALIGN_LayoutType: m=_("Wheel to change layout type"); break;
		case ALIGN_Path: m=_("Click to edit path"); break;
		case ALIGN_Randomize: m=_("Click to shuffle"); break;
		case ALIGN_LineControl: m=_(" *** line control ***"); break;
		default: m=NULL; break;
	}
	viewport->postmessage(m?m:"");
}

int AlignInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	//int over=scan(x,y);

	if (child) return 0;

	int action=-1, index=-1;
	DBG action=scan(x,y,index, state);
	DBG cerr <<"Align move: "<<action<<','<<index<<endl;

	if (!buttondown.any()) {
		if (action!=hover) {
			hover=action;
			hoverindex=index;
			postHoverMessage();
			needtodraw=1;
		}
		return 0;
	}

	int lx,ly;
	buttondown.getextrainfo(mouse->id,LEFTBUTTON,&action,NULL);
	if (action<ALIGN_None) return RectInterface::MouseMove(x,y,state,mouse);

	buttondown.move(mouse->id,x,y, &lx,&ly);

	flatpoint dd=dp->screentoreal(x,y)-dp->screentoreal(lx,ly);
	if (action==ALIGN_Move) {
		aligninfo->center+=dd;
		if (aligninfo->path && (state&LAX_STATE_MASK)==0) {
			aligninfo->path->origin(aligninfo->path->origin() + dd);
		}
		if (active) ApplyAlignment(0);
		needtodraw=1;
		return 0;
	}

	if (action==ALIGN_MoveLeftBound) {
		flatpoint p;
		PointAlongPath(aligninfo->leftbound,0, p, NULL);
		p+=dd;
		ClosestPoint(p,&aligninfo->leftbound);

		if (active) ApplyAlignment(0);
		needtodraw=1;
		return 0;
	}

	if (action==ALIGN_MoveRightBound) {
		flatpoint p;
		PointAlongPath(aligninfo->rightbound,0, p, NULL);
		p+=dd;
		ClosestPoint(p,&aligninfo->rightbound);

		if (active) ApplyAlignment(0);
		needtodraw=1;
		return 0;
	}

	if (action==ALIGN_MoveSnapAlign) {
		 //move snap align
		double adjust=1;
		if ((state&LAX_STATE_MASK)==ShiftMask) adjust=.25;
		if ((state&LAX_STATE_MASK)==ControlMask) adjust=.1;
		if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) adjust=.01;

		flatpoint v=transform_vector(dp->Getctm(),aligninfo->snap_direction);
		v/=norm(v);
		aligninfo->snapalignment-=v*(flatpoint(x,y)-flatpoint(lx,ly))*adjust/(BAR-2*RADIUS)/aligninfo->uiscale*100;
		if (active) ApplyAlignment(0);

		char buffer[100];
		sprintf(buffer,"snapalign %f",aligninfo->snapalignment);
		viewport->postmessage(buffer);

		needtodraw=1;
		return 0;
	}

	if (action==ALIGN_MoveFinalAlign) {
		 //move final align
		double adjust=1;
		if ((state&LAX_STATE_MASK)==ShiftMask) adjust=.25;
		if ((state&LAX_STATE_MASK)==ControlMask) adjust=.1;
		if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) adjust=.01;

		flatpoint v=transform_vector(dp->Getctm(),aligninfo->layout_direction);
		v/=norm(v);
		aligninfo->finalalignment-=v*(flatpoint(x,y)-flatpoint(lx,ly))*adjust/(BAR-2*RADIUS)/aligninfo->uiscale*100;
		if (active) ApplyAlignment(0);

		char buffer[100];
		sprintf(buffer,"final align %f",aligninfo->finalalignment);
		viewport->postmessage(buffer);

		needtodraw=1;
		return 0;
	}

	if ((state&LAX_STATE_MASK)==0 && action==ALIGN_RotateSnapAndAlign) {
		//double angle=(x-lx)/180.;
		double angle=angle2(dp->screentoreal(flatpoint(lx,ly))-aligninfo->center,
							dp->screentoreal(flatpoint(x,y))-aligninfo->center);
		aligninfo->snap_direction=rotate(aligninfo->snap_direction,angle);
		aligninfo->layout_direction=rotate(aligninfo->layout_direction,angle);
		if (active) ApplyAlignment(0);

		char buffer[100];
		sprintf(buffer,"rotate %f degrees",angle*180/M_PI);
		viewport->postmessage(buffer);
		
		needtodraw=1;
		return 0;
	}

	 //^ scales
	if ((state&LAX_STATE_MASK)==ControlMask) {
	}

	 //+^ rotates
	if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) {
	}

	return 0;
}

/*!
 */
int AlignInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" aligninterface got ch:"<<ch<<"  "<<(state&LAX_STATE_MASK)<<endl;

	if (ch==LAX_Esc) {
		if (!strcmp(owner->whattype(),"ObjectInterface")) {
			 //revert control to owner
			ObjectInterface *gi=dynamic_cast<ObjectInterface*>(owner);
			gi->AddToSelection(selection);
			gi->RemoveChild();
		}
		return 0;

	} else if (ch==LAX_Up && (aligninfo->final_layout_type==FALIGN_Align || aligninfo->final_layout_type==FALIGN_Proportional)) {
		 //move final align bar
		double adjust=1;
		if ((state&LAX_STATE_MASK)==ShiftMask) adjust=.1;
		if ((state&LAX_STATE_MASK)==ControlMask) adjust=.01;
		if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) adjust=.001;

		aligninfo->finalalignment+=10*adjust;
		if (active) ApplyAlignment(0);

		char buffer[100];
		sprintf(buffer,_("Final align %f"),aligninfo->finalalignment);
		viewport->postmessage(buffer);

		needtodraw=1;
		return 0;

	} else if (ch==LAX_Down && (aligninfo->final_layout_type==FALIGN_Align || aligninfo->final_layout_type==FALIGN_Proportional)) {
		 //move final align bar
		double adjust=1;
		if ((state&LAX_STATE_MASK)==ShiftMask) adjust=.1;
		if ((state&LAX_STATE_MASK)==ControlMask) adjust=.01;
		if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) adjust=.001;

		aligninfo->finalalignment-=10*adjust;
		if (active) ApplyAlignment(0);

		char buffer[100];
		sprintf(buffer,_("Final align %f"),aligninfo->finalalignment);
		viewport->postmessage(buffer);

		needtodraw=1;
		return 0;

	} else if (ch==LAX_Right) {
		 //move snap align bar
		double adjust=1;
		if ((state&LAX_STATE_MASK)==ShiftMask) adjust=.1;
		if ((state&LAX_STATE_MASK)==ControlMask) adjust=.01;
		if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) adjust=.001;

		aligninfo->snapalignment+=10*adjust;
		if (active) ApplyAlignment(0);

		char buffer[100];
		sprintf(buffer,_("Snap align %f"),aligninfo->snapalignment);
		viewport->postmessage(buffer);

		needtodraw=1;
		return 0;

	} else if (ch==LAX_Left) {
		 //move snap align bar
		double adjust=1;
		if ((state&LAX_STATE_MASK)==ShiftMask) adjust=.1;
		if ((state&LAX_STATE_MASK)==ControlMask) adjust=.01;
		if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) adjust=.001;

		aligninfo->snapalignment-=10*adjust;
		if (active) ApplyAlignment(0);

		char buffer[100];
		sprintf(buffer,_("snapalign %f"),aligninfo->snapalignment);
		viewport->postmessage(buffer);

		needtodraw=1;
		return 0;

	} else if (ch=='x') {
		 //make snap direction vertical, layout direction horizontal
		aligninfo->snap_direction=flatpoint(0,1);
		aligninfo->layout_direction=flatpoint(1,0);
		if (active) ApplyAlignment(0);

		needtodraw=1;
		return 0;

	} else if (ch=='y') {
		 //make snap direction horizontal, layout direction vertical
		aligninfo->snap_direction=flatpoint(1,0);
		aligninfo->layout_direction=flatpoint(0,1);
		if (active) ApplyAlignment(0);

		needtodraw=1;
		return 0;

	} else if (ch=='r' && (state&LAX_STATE_MASK)==0) {
		 //align right edges, snap=lr, layout=tb
		aligninfo->snap_direction=flatpoint(1,0);
		aligninfo->layout_direction=flatpoint(0,1);
		aligninfo->snapalignment=100;
		if (active) ApplyAlignment(0);

		needtodraw=1;
		return 0;

	} else if (ch=='l' && (state&LAX_STATE_MASK)==0) {
		 //align left edges, snap=lr, layout=tb
		aligninfo->snap_direction=flatpoint(1,0);
		aligninfo->layout_direction=flatpoint(0,1);
		aligninfo->snapalignment=0;
		if (active) ApplyAlignment(0);

		needtodraw=1;
		return 0;

	} else if (ch=='c' && (state&LAX_STATE_MASK)==0) {
		 //align centers vertically, snap=lr, layout=tb
		aligninfo->snap_direction=flatpoint(1,0);
		aligninfo->layout_direction=flatpoint(0,1);
		aligninfo->snapalignment=50;
		if (active) ApplyAlignment(0);

		needtodraw=1;
		return 0;


	} else if (ch=='t' && (state&LAX_STATE_MASK)==0) {
		 //align top edges, snap=tb, layout=lr
		aligninfo->snap_direction=flatpoint(0,1);
		aligninfo->layout_direction=flatpoint(1,0);
		aligninfo->snapalignment=0;
		if (active) ApplyAlignment(0);

		needtodraw=1;
		return 0;

	} else if (ch=='b' && (state&LAX_STATE_MASK)==0) {
		 //align bottom edges, snap=tb, layout=lr
		aligninfo->snap_direction=flatpoint(0,1);
		aligninfo->layout_direction=flatpoint(1,0);
		aligninfo->snapalignment=100;
		if (active) ApplyAlignment(0);

		needtodraw=1;
		return 0;

	} else if (ch=='C' && (state&LAX_STATE_MASK)==ShiftMask) {
		 //align centers horizontally, snap=tb, layout=lr
		aligninfo->snap_direction=flatpoint(0,1);
		aligninfo->layout_direction=flatpoint(1,0);
		aligninfo->snapalignment=50;
		if (active) ApplyAlignment(0);

		needtodraw=1;
		return 0;

	} else if (ch=='[' && ((state&LAX_STATE_MASK)==0 || (state&LAX_STATE_MASK)==ControlMask)) {
		 //adjust leftbound
		aligninfo->leftbound-=(state&LAX_STATE_MASK)==0?-boundstep:boundstep;
		ClampBoundaries(0);
		needtodraw=1;
		return 0;

	} else if (ch==']' && ((state&LAX_STATE_MASK)==0 || (state&LAX_STATE_MASK)==ControlMask)) {
		 //adjust rightbound
		aligninfo->rightbound-=(state&LAX_STATE_MASK)==0?boundstep:-boundstep;
		ClampBoundaries(0);
		needtodraw=1;
		return 0;

	} else if (ch=='{' || ch=='}') {
		boundstep*=(ch=='{'?.9:1.1);
		char buffer[100];
		sprintf(buffer,_("bound step %f"),boundstep);
		viewport->postmessage(buffer);
		return 0;

	} else if (ch=='0') {
		ClampBoundaries(1);
		needtodraw=1;
		return 0;

	} else if (ch=='a' && (state&LAX_STATE_MASK)==0) {
		//override default
		return 0;

	}
	return 1;
}

//! Clamp left and right boundaries to actually fit on the line.
/*! If fill, then fill the boundaries to the whole path. Wrap totally around for closed paths.
 */
int AlignInterface::ClampBoundaries(int fill)
{
	if (!aligninfo->path) return 0; //nothing to do if no path!

	if (fill) {
		 //fill to whole path
		aligninfo->leftbound=0;
		double e;
		if (aligninfo->path->paths.e[0]->path->isClosed()) e=0; else e=1;
		aligninfo->rightbound=aligninfo->path->paths.e[0]->path->NumPoints(1)-e;

	} else {
		 //clamp to path
		int closed=(aligninfo->path->paths.e[0]->path->isClosed());

		if (aligninfo->leftbound<0) aligninfo->leftbound=0;
		else if (!closed && aligninfo->leftbound>aligninfo->path->paths.e[0]->path->NumPoints(1)-1)
			aligninfo->leftbound=aligninfo->path->paths.e[0]->path->NumPoints(1)-1;

		if (aligninfo->rightbound<0) aligninfo->rightbound=0;
		else if (!closed && aligninfo->rightbound>aligninfo->path->paths.e[0]->path->NumPoints(1)-1)
			aligninfo->rightbound=aligninfo->path->paths.e[0]->path->NumPoints(1)-1;
	}

	if (active) ApplyAlignment(0);
	return 0;
}

int AlignInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	return 1;
}

//! Add objects to selection, then map default left and right bound.
int AlignInterface::AddToSelection(Laxkit::PtrStack<ObjectContext> &objs)
{
	int n=ObjectInterface::AddToSelection(objs);
	if (!n) return 0;
	aligninfo->center=flatpoint((data->minx+data->maxx)/2,(data->miny+data->maxy)/2);
	aligninfo->leftbound =-norm(flatpoint(data->minx,(data->miny+data->maxy)/2)-aligninfo->center);
	aligninfo->rightbound= norm(flatpoint(data->maxx,(data->miny+data->maxy)/2)-aligninfo->center);

	//***
	original_transforms.flush();
	original_centers.flush();
	SomeData *t;
	for (int c=0; c<selection.n; c++) {
		t=new SomeData;
		t->m(selection.e[c]->obj->m());
		original_transforms.push(t);
		t->dec_count();
	}

	return n;
}

int AlignInterface::FreeSelection()
{
	ObjectInterface::FreeSelection();
	original_transforms.flush();
	original_centers.flush();
	return 0;
}

//! Copy back original transforms from original_transforms to the selection.
int AlignInterface::ResetAlignment()
{
	if (!data || !selection.n) return 1;

	 //first reset positions to original state
	for (int c=0; c<selection.n; c++) {
		selection.e[c]->obj->m(original_transforms.e[c]->m());
	}

	RemapBounds();
	needtodraw=1;
	return 0;
}

/*! From original_transforms, apply alignment by first copying back original to actual object,
 * then figuring out new placement, then take that new placement and put in original_transforms.
 *
 * Return 0 for success or 1 for unable to apply for some reason.
 */
int AlignInterface::ApplyAlignment(int updateorig)
{
	if (!data || !selection.n) return 1;

	double m[6],mm[6];
	transform_identity(m);
	transform_identity(mm);
	data->m(m);

	 //first reset positions to original state
	for (int c=0; c<selection.n; c++) {
		selection.e[c]->obj->m(original_transforms.e[c]->m());
	}

	 //then find and set new transforms
	flatpoint cc, ul,ur,ll,lr;
	flatpoint v,p, d,ac;
	double dul,dur,dll,dlr;
	double min,max;
	SomeData *o;

	if (needtoresetlayout) controls.flush();
	DBG while (pp1.n<selection.n) { pp1.push(flatpoint()); pp2.push(flatpoint()); }

	if (aligninfo->final_layout_type==FALIGN_Align
			|| aligninfo->final_layout_type==FALIGN_Proportional
			|| aligninfo->final_layout_type==FALIGN_None) {

		//We don't need to mess with the path when final is just align or none. We need only
		//snap and align when appropriate to snap_direction and layout_direction

		for (int c=0; c<selection.n; c++) {
			o=selection.e[c]->obj;
			if (viewport) viewport->transformToContext(m,selection.e[c],0,1);
			else transform_copy(m,o->m());

			ul=transform_point(m,o->minx,o->maxy);
			ur=transform_point(m,o->maxx,o->maxy);
			ll=transform_point(m,o->minx,o->miny);
			lr=transform_point(m,o->maxx,o->miny);
			cc=(ul+lr)/2; //original center of the object in dp real coordinates

			d.x=d.y=0;

			 //first snap in snap_direction
			if (aligninfo->snap_align_type==FALIGN_Align || aligninfo->snap_align_type==FALIGN_Proportional) {
				 //find min and max along snap direction
				v=aligninfo->snap_direction;
				//v=transform_vector(dp->Getictm(),v);
				p=aligninfo->center;

				//ac=dp->screentoreal(aligninfo->center);
				ac=aligninfo->center;
				dul=distparallel((ac-ul),v);
				min=max=dul;
				dur=distparallel((ac-ur),v);
				if (dur<min) min=dur; else if (dur>max) max=dur;
				dll=distparallel((ac-ll),v);
				if (dll<min) min=dll; else if (dll>max) max=dll;
				dlr=distparallel((ac-lr),v);
				if (dlr<min) min=dlr; else if (dlr>max) max=dlr;

				if (aligninfo->final_layout_type==FALIGN_None) {
					 //normally use only layout_direction, but for none makes more sense to use path
					if (!PointToPath(cc,p)) continue;
				} else if (!PointToLine(cc,p,0)) continue; //makes p the point on layout line that cc snaps to

				DBG pp1.e[c]=p;  //center snapped to path
				DBG pp2.e[c]=cc; //original center

				 //apply snap alignment
				d+=p-cc;
				if (aligninfo->snap_align_type==FALIGN_Proportional) d-=(aligninfo->snapalignment/100)*(p-cc);
				else d-=v/norm(v)*(max-min)*(aligninfo->snapalignment-50)/100;
			}

			 //then snap in layout_direction
			if (aligninfo->final_layout_type==FALIGN_Align || aligninfo->final_layout_type==FALIGN_Proportional) {
				v=aligninfo->layout_direction;
				//v=transform_vector(dp->Getictm(),v);
				p=aligninfo->center;

				 //find min and max along snap direction
				//ac=dp->screentoreal(aligninfo->center);
				ac=aligninfo->center;
				dul=distparallel((ac-ul),v);
				min=max=dul;
				dur=distparallel((ac-ur),v);
				if (dur<min) min=dur; else if (dur>max) max=dur;
				dll=distparallel((ac-ll),v);
				if (dll<min) min=dll; else if (dll>max) max=dll;
				dlr=distparallel((ac-lr),v);
				if (dlr<min) min=dlr; else if (dlr>max) max=dlr;

				if (!PointToLine(cc,p,1)) continue; //makes p the screen point on snap line that cc snaps to

				DBG pp1.e[c]=p;  //center snapped to path
				DBG pp2.e[c]=cc; //original center

				 //apply snap alignment
				d+=p-cc;
				if (aligninfo->final_layout_type==FALIGN_Proportional) d-=(aligninfo->finalalignment/100)*(p-cc);
				else d-=v/norm(v)*(max-min)*(aligninfo->finalalignment-50)/100;
			}

			mm[4]=d.x;
			mm[5]=d.y;
			TransformSelection(mm,c,c);
		}
	} //if final align is align, align proportional, or none


	//the rest of the layout types require positioning on the path

	if (       aligninfo->final_layout_type==FALIGN_Random
			|| aligninfo->final_layout_type==FALIGN_Grid) {

		flatpoint point,tangent;

		for (int c=0; c<selection.n; c++) {
			o=selection.e[c]->obj;
			if (viewport) viewport->transformToContext(m,selection.e[c],0,1);
			else transform_copy(m,o->m());

			ul=transform_point(m,o->minx,o->maxy);
			ur=transform_point(m,o->maxx,o->maxy);
			ll=transform_point(m,o->minx,o->miny);
			lr=transform_point(m,o->maxx,o->miny);
			cc=(ul+lr)/2; //original center of object in dp real coordinates

			d.x=d.y=0;

			point=cc;
			double lbound=aligninfo->leftbound, rbound=aligninfo->rightbound;

			if (aligninfo->final_layout_type==FALIGN_Random) {
				 // establish random control point
				double dist;
				if (needtoresetlayout) dist=lbound+((double)random()/RAND_MAX)*(rbound-lbound);
				else dist=controlamount.e[c];
				PointAlongPath(dist,1, point,&tangent);

				if (c>=controls.n) {
					controls.push(flatpoint());
					controlamount.push(0);
				}
				controls.e[c]=point;
				controlamount.e[c]=dist; //remember so if we adust simple things, remember where we randomized to!

			} else if (aligninfo->final_layout_type==FALIGN_Grid) {
				 // establish grid control point
				PointAlongPath(lbound+((double)c/(selection.n>1?selection.n-1:1))*(rbound-lbound), 0, point,&tangent);
				if (c>=controls.n) {
					controls.push(flatpoint());
					controlamount.push(0);
				}
				controls.e[c]=point;
			}


			if (aligninfo->snap_align_type==FALIGN_Align || aligninfo->snap_align_type==FALIGN_Proportional) {
				 //find min and max along snap direction
				v=aligninfo->snap_direction;
				p=aligninfo->center;

				ac=aligninfo->center;
				dul=distparallel((ac-ul),v);
				min=max=dul;
				dur=distparallel((ac-ur),v);
				if (dur<min) min=dur; else if (dur>max) max=dur;
				dll=distparallel((ac-ll),v);
				if (dll<min) min=dll; else if (dll>max) max=dll;
				dlr=distparallel((ac-lr),v);
				if (dlr<min) min=dlr; else if (dlr>max) max=dlr;

				p=point; //the snapped destination

				DBG pp1.e[c]=p;  //center snapped to path
				DBG pp2.e[c]=cc; //original center

				 //apply snap alignment
				d+=p-cc;
				if (aligninfo->final_layout_type==FALIGN_Proportional) d-=(aligninfo->finalalignment/100)*(p-cc);
				else d-=transpose(tangent)/norm(transpose(tangent))*(max-min)*(aligninfo->snapalignment-50)/100;
			}

			if (d.x!=0 || d.y!=0) { //finally apply the shift
				mm[4]=d.x;
				mm[5]=d.y;
				TransformSelection(mm,c,c);
			}
		} //foreach selection.e
		needtoresetlayout=0;
	} //if final random or grid

	// ***
	//FALIGN_Gap,
	//FALIGN_Unoverlap



	if (updateorig)	for (int c=0; c<selection.n; c++) {
		original_transforms.e[c]->m(selection.e[c]->obj->m());
	}

	RemapBounds();

	needtodraw=1;
	return 0;
}

//! The point lying on either snap_direction, or layout_direction found by snapping real point p to it.
/*! If isfinal!=0, then swap layout_direction and snap_direction.
 *
 * Return 0 for directions do not intersect, or 1 for point found.
 */
int AlignInterface::PointToLine(flatpoint p, flatpoint &ip, int isfinal)
{
	flatline l1(p, p+(isfinal?aligninfo->layout_direction:aligninfo->snap_direction));
	flatline l2(aligninfo->center, aligninfo->center+(isfinal?aligninfo->snap_direction:aligninfo->layout_direction));

	double t1,t2;
	if (intersection(l1,l2,&ip,&t1,&t2)!=0) return 0; //lines do not intersect
	return 1;
}

//! Snap real point p to the path along snap_direction, and put that point in ip. Return 0 for does not intersect path.
int AlignInterface::PointToPath(flatpoint p, flatpoint &ip)
{
	if (!aligninfo->path) return PointToLine(p,ip,0);

	 //else intersect snap line with path
	flatpoint pt;
	int num=aligninfo->path->Intersect(0,p,p+aligninfo->snap_direction,1, 0,&pt,1, NULL,0);
	if (num==0) return 0;
	ip=pt;
	return 1;
}

//! Find real point along path at distance t. Put found point in point.
/*! t is either the t path parameter, or it tisdistance!=0, then it is the distance along the path.
 *
 * If tangent!=NULL, then also find the tangent of the line at the point.
 *
 * If the path is not defined long enough for dist, then return 0. Else return 1.
 */
int AlignInterface::PointAlongPath(double t,int tisdistance, flatpoint &point, flatpoint *tangent)
{
	if (!aligninfo->path) {
		point=aligninfo->center + t*aligninfo->layout_direction;
		if (tangent) *tangent=aligninfo->layout_direction;
		return 0;
	}

	// else find along PathsData
	flatpoint p, tt;
	int c=aligninfo->path->PointAlongPath(0, t,tisdistance, &p, &tt);
	point=transform_point(aligninfo->path->m(),p);
	if (tangent) *tangent=transform_vector(aligninfo->path->m(),tt);
	return c;
}

//! Find the point on the layout path closest to real point p.
/*! If d!=NULL, then make *d the t parameter of that point such as is suitable for left and right bounds.
 *
 * This is kind of the reverse of PointAlongPath().
 *
 * Returns the real point closest to the path.
 */
flatpoint AlignInterface::ClosestPoint(flatpoint p, double *d)
{
	if (!aligninfo->path) {
		double dist=distparallel(p-aligninfo->center,aligninfo->layout_direction);
		if (d) *d=dist;
		return aligninfo->center+dist*aligninfo->layout_direction;
	}

	 //else find along PathsData
	p=transform_point_inverse(aligninfo->path->m(),p);
	flatpoint found=aligninfo->path->ClosestPoint(p,NULL,NULL,d,NULL);
	return transform_point(aligninfo->path->m(),found);
}


//} // namespace Laidout
