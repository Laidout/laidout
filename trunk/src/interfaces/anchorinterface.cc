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
// Copyright (C) 2013 by Tom Lechner
//


#include "../language.h"
#include "../drawdata.h"
#include "../dataobjects/datafactory.h"
#include "anchorinterface.h"
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>


#include <lax/refptrstack.cc>

using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


namespace Laidout {



#define PAD 5
#define fudge 5.0






const char *AnchorRegionName(int a)
{
	if (a==ANCHOR_Parents)       return "Parents";
	if (a==ANCHOR_Page_Area)     return "Page Area";
	if (a==ANCHOR_Margin_Area)   return "Margin Area";
	if (a==ANCHOR_Paper)         return "Paper";
	if (a==ANCHOR_Selection)     return "Selection";
	if (a==ANCHOR_Other_Objects) return "Other Objects";
	if (a==ANCHOR_Guides)        return "Guides";
	return "";
}

enum AnchorShortcuts {
	ANCHORA_ToggleRegions,
	ANCHORA_NextType,
	ANCHORA_PreviousType,
	ANCHORA_DeleteLastRule,
	ANCHORA_MAX
};



//------------------------------------- AnchorInterface --------------------------------------
	
/*! \class AnchorInterface 
 * \brief Interface to build nets out of various shapes. See also AnchorInfo.
 */


AnchorInterface::Anchors::Anchors(PointAnchor *aa, int oon)
{
	anchor=aa;
	on=oon;
	anchorsource=0;
}

AnchorInterface::Anchors::~Anchors()
{
	if (anchor) anchor->dec_count();
}


AnchorInterface::AnchorInterface(anInterface *nowner,int nid,Displayer *ndp)
	: anInterface(nowner,nid,ndp)
{
	firsttime=true;

	show_region_selector=true;
	regions.AddItem(_("Parents"),  (LaxImage*)NULL, ANCHOR_Parents,       LAX_ON|LAX_ISTOGGLE);
	regions.AddItem(_("Page"),     (LaxImage*)NULL, ANCHOR_Page_Area,     LAX_ON|LAX_ISTOGGLE);
	regions.AddItem(_("Margin"),   (LaxImage*)NULL, ANCHOR_Margin_Area,   LAX_ON|LAX_ISTOGGLE);
	//regions.AddItem(_("Paper"),    (LaxImage*)NULL, ANCHOR_Paper,         LAX_ON|LAX_ISTOGGLE);
	regions.AddItem(_("Selection"),(LaxImage*)NULL, ANCHOR_Selection,     LAX_ON|LAX_ISTOGGLE);
	regions.AddItem(_("Objects"),  (LaxImage*)NULL, ANCHOR_Other_Objects, LAX_ON|LAX_ISTOGGLE);
	//regions.AddItem(_("Guides"),   (LaxImage*)NULL, ANCHOR_Guides,        LAX_ON|LAX_ISTOGGLE);

	hover_item=-1;
	hover_anchor=-1;

	active_anchor=-1;
	active_i1=active_i2=-1;
	active_type=ALIGNMENTRULE_None;
	active_match=-1;
	last_type=ALIGNMENTRULE_Move;
	current_rule=NULL;

	sc=NULL;
	selection=NULL;
	cur_oc=NULL;
	proxy=NULL; //only active while button down
}

AnchorInterface::~AnchorInterface()
{
	DBG cerr <<"AnchorInterface destructor.."<<endl;

	if (sc) sc->dec_count();
	if (selection) selection->dec_count();
	if (cur_oc) delete cur_oc;
	if (proxy) proxy->dec_count(); //shouldn't happen, but dec just in case mouse input error
	//if (doc) doc->dec_count();
}


const char *AnchorInterface::Name()
{ return _("Anchor"); }



/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *AnchorInterface::ContextMenu(int x,int y,int deviceid)
{
	//MenuInfo *menu=new MenuInfo(_("Anchor Interface"));

	//menu->AddItem(dirname(LAX_BTRL),LAX_BTRL,LAX_ISTOGGLE|(shapeinfo->direction==LAX_BTRL?LAX_CHECKED:0)|LAX_OFF,1);
	//menu->AddSep();

	//return menu;
	return NULL;
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int AnchorInterface::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i =s->info2; //id of menu item
		int ii=s->info4; //extra id, 1 for direction
		if (ii==0) {
			cerr <<"change direction to "<<i<<endl;
			return 0;

		} else {
			return 0;

		}

		return 0;
	}
	return 1;
}


/*! Will say it cannot draw anything.
 */
int AnchorInterface::draws(const char *atype)
{ return 0; }


//! Return a new AnchorInterface if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *AnchorInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new AnchorInterface(NULL,id,NULL);
	else if (!dynamic_cast<AnchorInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}


int AnchorInterface::InterfaceOn()
{
	DBG cerr <<"pagerangeinterfaceOn()"<<endl;

	needtodraw=1;
	return 0;
}

int AnchorInterface::InterfaceOff()
{
	anchors.flush();
	active_anchor=-1;
	hover_anchor=hover_item=-1;
	Clear(NULL);
	needtodraw=1;
	return 0;
}

void AnchorInterface::Clear(SomeData *d)
{
	if (cur_oc) delete cur_oc;
	cur_oc=NULL;
}

/*! p is real space.
 * Return 0 for success.
 */
int AnchorInterface::AddAnchor(flatpoint p,const char *name, int source, int id)
{
	PointAnchor *a=new PointAnchor(name,PANCHOR_Absolute,p,flatpoint(),id);
	anchors.push(new Anchors(a,1));
	anchors.e[anchors.n-1]->anchorsource=source;
	//a->dec_count();
	return 0;
}

/*! Return number of anchors added.
 */
int AnchorInterface::AddAnchors(VObjContext *context, int source)
{ 
	if (!dynamic_cast<DrawableObject*>(context->obj)) return 0;
	double m[6];
	viewport->transformToContext(m,context,0,1);
	DrawableObject *d=dynamic_cast<DrawableObject*>(context->obj);

	flatpoint p,pp;
	int id;
	const char *name;

	for (int c=0; c<d->NumAnchors(); c++) {
		d->GetAnchorInfoI(c, &id,&name,&p, false);
		pp=transform_point(m,p);
		AddAnchor(pp,name,source,id);
	}

	return d->NumAnchors();
}

/*! Remove anchors owned by oc->obj.
 * Return number of anchors removed.
 */
int AnchorInterface::RemoveAnchors(LaxInterfaces::ObjectContext *oc)
{
	int n=0;
	for (int c=anchors.n-1; c>=0; c--) {
		if (oc->obj==anchors.e[c]->anchor->owner) { anchors.remove(c); n++; }

	}
	return n;
}

int AnchorInterface::UpdateAnchors(int region)
{
	// ***
	//ANCHOR_Margin_Area,
	//ANCHOR_Paper,
	//ANCHOR_Other_Objects,
	//ANCHOR_Guides,

	bool active=true;
	if (region!=ANCHOR_Object) active=RegionActive(region);
	
	if (region==ANCHOR_Selection) {
		UpdateSelectionAnchors();
		return 0;
	}

	if (region==ANCHOR_Page_Area) {
		for (int c=anchors.n-1; c>=0; c--) { if (anchors.e[c]->anchorsource==ANCHOR_Page_Area) anchors.remove(c); }

		if (active) {
			if (!cur_oc) return 0;
			//int p=dynamic_cast<LaidoutViewport*>(viewport)->curobjPage();
			int pg=cur_oc->spreadpage();
			if (pg<0) return 0;

			LaidoutViewport *vp=dynamic_cast<LaidoutViewport*>(viewport);
			PageLocation *pl=vp->spread->pagestack.e[pg];
			DrawableObject d;
			d.clear();
			d.m(pl->outline->m());
			d.setbounds(pl->outline);

			flatpoint p;
			int id;
			const char *name;
			for (int c=0; c<d.NumAnchors(); c++) {
				d.GetAnchorInfoI(c, &id,&name,&p, false);
				p=d.transformPoint(p);
				AddAnchor(p,name,ANCHOR_Page_Area,id);
			}
		}
		return 0;
	}

	if (region==ANCHOR_Margin_Area) {
		for (int c=anchors.n-1; c>=0; c--) { if (anchors.e[c]->anchorsource==ANCHOR_Margin_Area) anchors.remove(c); }

		if (active) {
			if (!cur_oc) return 0;
			//int p=dynamic_cast<LaidoutViewport*>(viewport)->curobjPage();
			int pg=cur_oc->spreadpage();
			if (pg<0) return 0;

			LaidoutViewport *vp=dynamic_cast<LaidoutViewport*>(viewport);
			PageLocation *pl=vp->spread->pagestack.e[pg];
			if (pl->index<0) return 0;
			if (!vp->doc->pages.e[pl->index]->pagestyle->margin) return 0;

			DrawableObject d;
			d.clear();
			d.m(vp->doc->pages.e[pl->index]->pagestyle->margin->m());
			d.setbounds(pl->outline);

			flatpoint p;
			int id;
			const char *name;
			for (int c=0; c<d.NumAnchors(); c++) {
				d.GetAnchorInfoI(c, &id,&name,&p, false);
				p=d.transformPoint(p);
				AddAnchor(p,name,ANCHOR_Margin_Area,id);
			}
		}
		return 0;
	}

	if (region==ANCHOR_Object) {
		for (int c=anchors.n-1; c>=0; c--) { if (anchors.e[c]->anchorsource==ANCHOR_Object) anchors.remove(c); }
		if (active) {
			if (cur_oc) AddAnchors(cur_oc, ANCHOR_Object);
		}
		return 0;
	}

	if (region==ANCHOR_Parents) {
		for (int c=anchors.n-1; c>=0; c--) { if (anchors.e[c]->anchorsource==ANCHOR_Parents) anchors.remove(c); }
		if (active) {
			if (!cur_oc || (dynamic_cast<DrawableObject*>(cur_oc->obj) && !dynamic_cast<DrawableObject*>(cur_oc->obj)->parent)) return 0;

			VObjContext *oc=dynamic_cast<VObjContext*>(cur_oc->duplicate());
			oc->pop();
			oc->SetObject(dynamic_cast<DrawableObject*>(cur_oc->obj)->parent);
			AddAnchors(oc, ANCHOR_Parents);
			delete oc;
		}
		return 0;
	}

	if (region==ANCHOR_Other_Objects) {
		for (int c=anchors.n-1; c>=0; c--) { if (anchors.e[c]->anchorsource==ANCHOR_Other_Objects) anchors.remove(c); }
		if (active && selection) {
			for (int c=0; c<selection->n(); c++) {
				AddAnchors(dynamic_cast<VObjContext*>(selection->e(c)), ANCHOR_Other_Objects);
			}
		}
	}

	return 1;
}

/*! Update selection anchors AND other object anchors since they are based on the selection.
 */
void AnchorInterface::UpdateSelectionAnchors()
{
	for (int c=anchors.n-1; c>=0; c--) {
		if (anchors.e[c]->anchorsource==ANCHOR_Selection) anchors.remove(c);
		else if (anchors.e[c]->anchorsource==ANCHOR_Other_Objects) anchors.remove(c);
	}

	bool active =RegionActive(ANCHOR_Selection);
	bool oactive=RegionActive(ANCHOR_Other_Objects);

	if (!selection || selection->n()==0) return;
	if (!active && !oactive) return;

	DrawableObject o;
	o.clear();
    double m[6];
    for (int c=0; c<selection->n(); c++) {
		if (oactive) AddAnchors(dynamic_cast<VObjContext*>(selection->e(c)), ANCHOR_Other_Objects);

		if (active) {
			viewport->transformToContext(m,selection->e(c),0,1);
			o.addtobounds(m, selection->e(c)->obj);
		}
    }

	if (active) {
		selection->setbounds(&o);

		flatpoint p;
		int id;
		const char *name;
		for (int c=0; c<o.NumAnchors(); c++) {
			o.GetAnchorInfoI(c, &id,&name,&p, false);
			AddAnchor(p,name,ANCHOR_Selection,id);
		}
	}
}

bool AnchorInterface::RegionActive(int region)
{
	MenuItem *mi;
	for (int c=0; c<regions.n(); c++) {
		mi=regions.e(c);
		if (mi->id==region && mi->isSelected()) return true;
	}
	return false;
}

void AnchorInterface::RefreshMenu()
{
	double th=dp->textheight();

	MenuItem *mi;
	for (int c=0; c<regions.n(); c++) {
		mi=regions.e(c);

		dp->NewFG(.3,.3,.3);
		if (hover_item==ANCHOR_Region && hover_anchor==mi->id) {
			dp->NewBG(1.,1.,1.);
			dp->NewFG(.8,.8,.8);
			dp->drawrectangle(0,(c+1)*th, mi->w+th,th, 2);
			dp->NewFG(0.,0.,0.);
		}
		if (mi->isSelected()) dp->drawthing(th/2,th*(c+1)+th/2, th/2,-th/2, 1, THING_Check);
		dp->textout(th,th*(c+1), mi->name,-1, LAX_TOP|LAX_LEFT);
	}

	//dp->NewFG(0.,0.,0.);
	//dp->drawthing(th/2,th/2, th/2,-th/2, 1, THING_Check);
}


void AnchorInterface::DrawSelectedIndicator(LaxInterfaces::ObjectContext *oc)
{
    if (!oc || !oc->obj) return;

	dp->DrawReal();
	
    double m[6];
    viewport->transformToContext(m,oc,0,1);
    dp->PushAndNewTransform(m);

    dp->NewFG(.5,.5,.5);

     //draw corners just outside bounding box
    SomeData *data=oc->obj;
    dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
    double o=5/dp->Getmag(), //5 pixels outside, 15 pixels long
           ow=(data->maxx-data->minx)/5,
           oh=(data->maxy-data->miny)/5;
    dp->drawline(data->minx-o,data->miny-o, data->minx+ow,data->miny-o);
    dp->drawline(data->minx-o,data->miny-o, data->minx-o,data->miny+oh);
    dp->drawline(data->minx-o,data->maxy+o, data->minx-o,data->maxy-oh);
    dp->drawline(data->minx-o,data->maxy+o, data->minx+ow,data->maxy+o);
    dp->drawline(data->maxx+o,data->maxy+o, data->maxx-ow,data->maxy+o);
    dp->drawline(data->maxx+o,data->maxy+o, data->maxx+o,data->maxy-oh);
    dp->drawline(data->maxx+o,data->miny-o, data->maxx-ow,data->miny-o);
    dp->drawline(data->maxx+o,data->miny-o, data->maxx+o,data->miny+oh);

    dp->PopAxes();
	dp->DrawScreen();
}


int AnchorInterface::Refresh()
{
	if (!needtodraw) return 0;

	DBG cerr <<"AnchorInterface::Refresh()..."<<endl;


	double th=dp->textheight();
	if (firsttime) {
		firsttime=false;
		MenuItem *mi;
		int w=0;
		for (int c=0; c<regions.n(); c++) {
			mi=regions.e(c);
			mi->w=dp->textextent(mi->name,-1,NULL,NULL);
			mi->h=th;
			if (mi->w>w) w=mi->w;
		}
		for (int c=0; c<regions.n(); c++) {
			mi=regions.e(c);
			mi->w=w;
		}
	}


	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	dp->DrawScreen();
	

	if (proxy) {
		dp->DrawReal();
		Laidout::DrawData(dp, proxy, NULL,NULL,0);
		dp->DrawScreen();
	}

	 //draw known active anchors
	flatpoint p;
	for (int c=0; c<anchors.n; c++) {
		if (active_anchor>=0 && anchors.e[c]->anchorsource==ANCHOR_Object) continue;

		p=dp->realtoscreen(anchors.e[c]->anchor->p);

		//dp->NewFG(&anchors.e[c]->anchor->color);
		if (anchors.e[c]->anchorsource==ANCHOR_Object) 
			dp->NewFG(0.,0.,1.);
		else dp->NewFG(0.,0.,0.);

		dp->drawthing(p.x,p.y, 4,4, 1, THING_Circle);
		dp->NewFG(~0);
		dp->drawthing(p.x,p.y, 5,5, 0, THING_Circle);

//		if (anchors.e[c]->anchor->name) {
//			dp->textout(p.x+5,p.y, anchors.e[c]->anchor->name,-1, LAX_LEFT|LAX_VCENTER);
//		}
	}

	if (hover_item==ANCHOR_Anchor && hover_anchor>=0) {
		flatpoint p=dp->realtoscreen(anchors.e[hover_anchor]->anchor->p);
		const char *aa=anchors.e[hover_anchor]->anchor->name;
		DrawText(p,aa?aa:"(anchor)");
		DrawText(flatpoint(p.x,p.y-th), AnchorRegionName(anchors.e[hover_anchor]->anchorsource));

		 //put extra green circle around matched anchor
		if (active_match==hover_anchor) {
			dp->NewFG(0.,1.,0.);
			dp->drawthing(p.x,p.y, 10,10, 0, THING_Circle);
		}
	}


	 //draw box around current object
	if (cur_oc) DrawSelectedIndicator(cur_oc);

	if (active_anchor>=0) {
		
		if (proxy) {
			double m[6];
			viewport->transformToContext(m,cur_oc,0,1);
			p=transform_point_inverse(m,anchors.e[active_anchor]->anchor->p);
			p=proxy->transformPoint(p);
			p=dp->realtoscreen(p);
		} else p=dp->realtoscreen(anchors.e[active_anchor]->anchor->p);

		bool hasmatch=(active_match>=0);

		 //draw linkages
		flatpoint p2;
		int cc;
		for (int c=0; c<2; c++) {
			if (c==0) cc=active_i1;
			if (c==1) cc=active_i2;
			if (cc<0) continue;

			p2=dp->realtoscreen(anchors.e[cc]->anchor->p);

   			dp->LineAttributes(3,LineSolid,LAXCAP_Round,LAXJOIN_Round);
			if (hasmatch) {
				 //draw solid line
				dp->NewFG(0.,1.,0.);
				dp->drawline(p,p2);
			} else {
				 //draw gradient line
				unsigned long m1=rgbcolorf(1.,0.,0.);//red
				unsigned long m2=rgbcolorf(0.,1.,0.);//green
				
				int nn=15;
				for (int c=0; c<nn; c++) {
					dp->NewFG(coloravg(m1,m2,c/(double)nn));
					dp->drawline(p+(p2-p)/nn*c, p+(p2-p)/nn*(c+1)); 
				}
			}

   			dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);

			dp->NewFG(0.,1.,0.);
			dp->drawthing(p2.x,p2.y, 5,5, 1, THING_Triangle_Down);
		}

		 //draw stuff around active anchor
		dp->NewFG(~0);
		dp->drawthing(p.x,p.y, 5,5, 1, THING_Circle);
		if (hasmatch) dp->NewFG(0.,1.,0.);
		else dp->NewFG(1.,0.,0.);
		dp->drawthing(p.x,p.y, 4,4, 1, THING_Circle);
		DrawText(p+flatpoint(0,7+th), AlignmentRuleName(active_type));

		 //draw other selectable object anchors
		if ((NumInvariants()==2 && active_i2<0) || (NumInvariants()==1 && active_i1<0)) {
			for (int c=0; c<anchors.n; c++) {
				if (c==active_anchor) continue;
				if (anchors.e[c]->anchorsource!=ANCHOR_Object) continue;

				p2=dp->realtoscreen(anchors.e[c]->anchor->p);

				dp->NewFG(~0);
				dp->drawthing(p2.x,p2.y, 5,5, 1, THING_Circle);
				dp->NewFG(0.,1.,0.);
				dp->drawthing(p2.x,p2.y, 4,4, 1, THING_Circle);
			}
		}
	} //if active_anchor


	 //draw region menu
	dp->NewFG(.5,.5,.5);
	dp->drawrectangle(th/5,th/5, th,th*4/5, 0);
	if (show_region_selector) RefreshMenu();


	 //draw list of rules of current object
	if (cur_oc) {
		AlignmentRule *rule=dynamic_cast<DrawableObject*>(cur_oc->obj)->parent_link;
		int y=5;
		int i=1;
		char *str;
		while (rule) {
			str=new char[strlen(AlignmentRuleName(rule->type))+20];
			sprintf(str,"%s (%d)", AlignmentRuleName(rule->type), i);
			i++;
			dp->textout(dp->Maxx-5,y, str,-1, LAX_RIGHT|LAX_TOP);
			y+=th;
			rule=rule->next;
		}
	}

	dp->DrawReal();


	needtodraw=0;
	return 1;
}

void AnchorInterface::DrawText(flatpoint p,const char *s)
{
	if (isblank(s)) return;

	dp->NewFG(1.,1.,1.);
	//-----
	dp->textout(p.x-1,p.y-5-1, s,-1, LAX_HCENTER|LAX_BOTTOM); //quick+dirty way to draw halo around text
	dp->textout(p.x-1,p.y-5+1, s,-1, LAX_HCENTER|LAX_BOTTOM);
	dp->textout(p.x+1,p.y-5+1, s,-1, LAX_HCENTER|LAX_BOTTOM);
	dp->textout(p.x+1,p.y-5-1, s,-1, LAX_HCENTER|LAX_BOTTOM);
	//------
	//double w,h;
	//dp->textextent(s,-1, &w,&h);
	//dp->drawrectangle(p.x-w/2,p.y-h-5, w,h, 1);
	//-----

	dp->NewFG(0,0,0);
	dp->textout(p.x,p.y-5, s,-1, LAX_HCENTER|LAX_BOTTOM);
}

int AnchorInterface::scan(int x,int y, int *anchor)
{
	if (firsttime) return -1;

	double th=dp->textheight();

	if (y<th && x<th*2) return ANCHOR_Regions;
	if (show_region_selector) {
		int i=y/th;

		if (x>=0 && x<regions.e(0)->w+th && i>=0 && i<regions.n()) {
			*anchor=regions.e(i)->id;
			return ANCHOR_Region;
		}
	}

	flatpoint p=flatpoint(x,y);
	double dist2=25; //*** 5 px, make this an option somewhere? tied to finger thickness??
	double d;
	 //search object first
	for (int c=0; c<anchors.n; c++) {
		if (anchors.e[c]->anchorsource!=ANCHOR_Object) continue;

		if (active_anchor>=0) {
			if (c==active_anchor || c==active_i1 || c==active_i2) {
				// ...it's ok to select these, but not extraneous ones:
			} else if (NumInvariants()==0 || (NumInvariants()==2 && active_i2>=0) || (NumInvariants()==1 && active_i1>=0))
				continue;
		}

		d=norm2(p-dp->realtoscreen(anchors.e[c]->anchor->p));
		if (d<dist2) {
			*anchor=c;
			return ANCHOR_Anchor;
		}
	}

	 //search everything else second
	for (int c=0; c<anchors.n; c++) {
		if (anchors.e[c]->anchorsource==ANCHOR_Object) continue;

		d=norm2(p-dp->realtoscreen(anchors.e[c]->anchor->p));
		if (d<dist2) {
			*anchor=c;
			return ANCHOR_Anchor;
		}
	}

	return -1;
}

int AnchorInterface::NumInvariants()
{
	if (active_anchor<0) return 0;

	if (active_type==ALIGNMENTRULE_Shear) return 2;
	if (active_type==ALIGNMENTRULE_Move) return 0;
	if (active_type==ALIGNMENTRULE_None) return 0;
	return 1;
}

int AnchorInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.any(0,LEFTBUTTON)) return 0; //only allow one button at a time

	int anchor=-1;
	int control=scan(x,y, &anchor);

	if (control<0) {
		 //add object to selection
        SomeData *obj=NULL;
        ObjectContext *oc=NULL;
        int c=viewport->FindObject(x,y,NULL,NULL,1,&oc);
        if (c>0) obj=oc->obj;

        if (obj) {
			int oldindex=(selection ? selection->FindIndex(oc) : -1);

			if ((state&LAX_STATE_MASK)==0) {
				if (selection) {
					selection->Flush();
				}
				for (int c=anchors.n-1; c>=0; c--) {
					if (anchors.e[c]->anchorsource==ANCHOR_Selection) anchors.remove(c);
				}
			}

			if (!cur_oc) {
				viewport->ChangeObject(oc,0);
				cur_oc=dynamic_cast<VObjContext*>(oc->duplicate());
				AddAnchors(cur_oc, ANCHOR_Object);
				UpdateAnchors(ANCHOR_Page_Area);
				UpdateAnchors(ANCHOR_Margin_Area);
				UpdateAnchors(ANCHOR_Parents);
				needtodraw=1;
				return 0;
			}

			if (oc->isequal(cur_oc)) return 0; //don't add base object to selection

			if (!selection) {
				selection=new Selection;
				selection->Add(oc,-1);
				UpdateSelectionAnchors();

			} else {
				int i=oldindex;

				if (i>=0 && ((state&LAX_STATE_MASK)==ShiftMask || selection->n()==1)) {
					 //remove object
					//RemoveAnchors(oc);
					selection->Remove(i);
					UpdateSelectionAnchors();
				} else if (i==-1) {
					 //new item
					int current=selection->CurrentObjectIndex();
					selection->Add(oc,-1);
					selection->CurrentObject(current);

					UpdateSelectionAnchors();
					//AddAnchors(dynamic_cast<VObjContext*>(oc),ANCHOR_Selection);
				}
			}

            needtodraw=1;
            return 0;
        }

		return 0;
	}

	if (control==ANCHOR_Anchor && anchor>=0 && anchors.e[anchor]->anchorsource==ANCHOR_Object) {
		newpoint=false;
		
		if (active_anchor<0) {
			 //select active anchor
			active_anchor=anchor;
			active_i2=active_i1=-1;
			if (active_type==ALIGNMENTRULE_None) active_type=ALIGNMENTRULE_Move;
			newpoint=true;

		} else if (anchor==active_anchor) {
			// ...

		} else if (anchor==active_i1) {
			// ...

		} else if (anchor==active_i2) {
			// ...

		} else {
			 //add points
			int n=NumInvariants();
			if (n==1) {
				active_i1=anchor;
				newpoint=true;

			} else if (n==2) {
				if (active_i1==-1) active_i1=anchor;
				else if (active_i2==-1) active_i2=anchor;
				newpoint=true;
			}
		}

		if (NumInvariants()==0
			  || (NumInvariants()==1 && active_i1>=0)
			  || (NumInvariants()==2 && active_i1>=0 && active_i2>=0)) {
			proxy=dynamic_cast<SomeDataRef*>(LaxInterfaces::somedatafactory->newObject("SomeDataRef"));
			proxy->Set(cur_oc->obj,1);
			double m[6];
			viewport->transformToContext(m,cur_oc,0,1);
			//transform_mult(m2, cur_oc->obj->m(),m);
			proxy->m(m);
		}

		needtodraw=1;
	}

	buttondown.down(d->id,LEFTBUTTON, x,y, control,anchor);
	return 0;
}

int AnchorInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	int control=-1, index=-1;
	int dragged=buttondown.up(d->id,LEFTBUTTON, &control, &index);

	if (!dragged && control==ANCHOR_Regions) {
		PerformAction(ANCHORA_ToggleRegions);
	} else if (!dragged && control==ANCHOR_Region) {
		MenuItem *mi;
		for (int c=0; c<regions.n(); c++) {
			mi=regions.e(c);
			if (mi->id==index) {
				if (mi->state&LAX_ON) { mi->SetState(LAX_ON,0); mi->SetState(LAX_OFF,1); }
				else { mi->SetState(LAX_ON,1); mi->SetState(LAX_OFF,0); }
				UpdateAnchors(mi->id);
				needtodraw=1;
			}
		}

	} else if (control==ANCHOR_Anchor) {
		if (!dragged && index>=0 && !newpoint) {
			if (index==active_anchor) {
				//turn off active anchor
				active_anchor=-1;
				active_i2=active_i1=-1;
				needtodraw=1;

			} else if (index==active_i1) {
				//turn off invariant point
				if (active_i2>=0) { active_i1=active_i2; active_i2=-1; }
				else active_i1=-1;
				needtodraw=1;

			} else if (index==active_i2) {
				active_i2=-1;
				needtodraw=1;

			}
		} else if (active_match>=0) {
			 // install rule
            active_anchor=anchors.e[active_anchor]->anchor->id;
            if (active_i1>=0) active_i1=anchors.e[active_i1]->anchor->id;
            if (active_i2>=0) active_i2=anchors.e[active_i2]->anchor->id;

			AlignmentRule *rule=new AlignmentRule;
			rule->type=active_type;
            rule->object_anchor=active_anchor;
            rule->invariant1=active_i1;
            rule->invariant2=active_i2;
            //rule->offset=offset;
            //rule->offset_units=offsetunits;
            rule->target=anchors.e[active_match]->anchor;

            dynamic_cast<DrawableObject*>(cur_oc->obj)->AddAlignmentRule(rule, false,-1); // append to end

			UpdateAnchors(ANCHOR_Object);
			active_anchor=findAnchor(active_anchor);
			if (active_i1>=0) active_i1=findAnchor(active_anchor);
			if (active_i2>=0) active_i2=findAnchor(active_anchor);

			active_match=-1;
			hover_item=-1;
			hover_anchor=-1;
			needtodraw=1;
		}
	}

	if (proxy) {
		proxy->dec_count();
		proxy=NULL;
		current_rule=NULL;
		needtodraw=1;
	}

	return 0;
}

//! Return the index of the anchor with id, or -1 if not found.
int AnchorInterface::findAnchor(int id)
{
	for (int c=0; c<anchors.n; c++)
		if (anchors.e[c]->anchor->owner==cur_oc->obj && anchors.e[c]->anchor->id==id) return c;
	return -1;
}

int AnchorInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int anchor=-1;
	int over=scan(x,y, &anchor);
	if (over==ANCHOR_Anchor && anchor==active_anchor) {
		 //change rule type
		PerformAction(ANCHORA_NextType);
		needtodraw=1;
		return 0;
	}

	return 1;
}

int AnchorInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int anchor=-1;
	int over=scan(x,y, &anchor);
	if (over==ANCHOR_Anchor && anchor==active_anchor) {
		 //change rule type
		PerformAction(ANCHORA_PreviousType);
		needtodraw=1;
		return 0;
	}

	return 1;
}


int AnchorInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	int anchor=-1;
	int over=scan(x,y, &anchor);

	if (over!=hover_item || anchor!=hover_anchor) needtodraw=1;
	hover_item=over;
	hover_anchor=anchor;

	if (!buttondown.any()) {
		return 0;
	}

	int lover, lanchor;
	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &lover,&lanchor);
	buttondown.move(mouse->id, x,y);

	if (lover==ANCHOR_Anchor) {
		if (lanchor!=active_anchor) {
			if (lanchor==active_i1) {
				int i=active_i1;
				active_i1=active_anchor;
				active_anchor=i;
			} else {
				int i=active_i2;
				active_i2=active_anchor;
				active_anchor=i;
			}
		}

		if (hover_item==ANCHOR_Anchor && hover_anchor!=active_match) {
			DBG cerr <<"*** maybe match?? "<<hover_anchor<<endl;
			if (hover_anchor>=0 && anchors.e[hover_anchor]->anchorsource!=ANCHOR_Object) {
				active_match=hover_anchor;
				needtodraw=1;
			}
		} else {
			if (hover_item!=ANCHOR_Anchor && active_match!=-1) { active_match=-1; needtodraw=1; }
		}

		if (proxy) {
			// ***shouldn't recreate this each time, just update a proxy rule
			AlignmentRule *rule=new AlignmentRule;
			rule->type=active_type;
			rule->object_anchor=anchors.e[active_anchor]->anchor->id;
			if (active_i1>=0) rule->invariant1=anchors.e[active_i1]->anchor->id;
			if (active_i2>=0) rule->invariant2=anchors.e[active_i2]->anchor->id;

			temptarget.p=dp->screentoreal(x,y);
			rule->target=&temptarget;
			dynamic_cast<DrawableObject*>(proxy)->AddAlignmentRule(rule, true, -1);
			needtodraw=1;
		}
	}

	return 0;
}

int AnchorInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	if (ch==LAX_Esc) {
		if (active_anchor>=0) {
			if (active_i2>=0) active_i2=-1;
			else if (active_i1>=0) active_i1=-1;
			else { active_anchor=-1; active_i1=active_i2=-1; }
			needtodraw=1;
			return 0;
		}
		if (selection && selection->n()) {
			selection->Flush();
			UpdateSelectionAnchors();
			needtodraw=1;
			return 0;
		}
		if (cur_oc) {
			//else remove cur obj
			delete cur_oc;
			cur_oc=NULL;
			UpdateAnchors(ANCHOR_Object);
			UpdateAnchors(ANCHOR_Page_Area);
			UpdateAnchors(ANCHOR_Parents);
			needtodraw=1;
			return 0;
		}
	}

	return 1;
}

Laxkit::ShortcutHandler *AnchorInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler("AnchorInterface");
	if (sc) return sc;


	sc=new ShortcutHandler("AnchorInterface");

	sc->Add(ANCHORA_ToggleRegions, 'd',0,0,      _("ToggleRegions"),  _("Toggle region menu"),NULL,0);
	sc->Add(ANCHORA_NextType,      LAX_Right,0,0,_("NextType"),       _("Use next link type"),NULL,0);
	sc->Add(ANCHORA_PreviousType,  LAX_Left,0,0, _("PreviousType"),   _("Use previous link type"),NULL,0);
	sc->Add(ANCHORA_DeleteLastRule,'x',0,0,      _("DeleteLastRule"), _("Delete last rule, if any"),NULL,0);

	manager->AddArea("AnchorInterface",sc);
	return sc;
}

int AnchorInterface::PerformAction(int action)
{
	if (action==ANCHORA_ToggleRegions) {
		show_region_selector=!show_region_selector;
		needtodraw=1;
		return 0;

	} else if (action==ANCHORA_NextType) {
		if (active_type==ALIGNMENTRULE_None || active_type==ALIGNMENTRULE_Matrix || active_type==ALIGNMENTRULE_Align)
			active_type=ALIGNMENTRULE_Move;
		else active_type++;
		if (active_type==ALIGNMENTRULE_EdgeMagnet) active_type=ALIGNMENTRULE_Move;
		if (NumInvariants()==0) { active_i1=active_i2=-1; }
		else if (NumInvariants()==1) { active_i2=-1; }
		PostMessage(AlignmentRuleName(active_type));
		needtodraw=1;
		return 0;

	} else if (action==ANCHORA_PreviousType) {
		if (active_type==ALIGNMENTRULE_None || active_type==ALIGNMENTRULE_Matrix || active_type==ALIGNMENTRULE_Align)
			active_type=ALIGNMENTRULE_Move;
		else active_type--;
		if (active_type==ALIGNMENTRULE_Align) active_type=ALIGNMENTRULE_Shear;
		if (NumInvariants()==0) { active_i1=active_i2=-1; }
		else if (NumInvariants()==1) { active_i2=-1; }
		PostMessage(AlignmentRuleName(active_type));
		needtodraw=1;
		return 0;

	} else if (action==ANCHORA_DeleteLastRule) {
		if (!cur_oc) return 0;
		DrawableObject *o=dynamic_cast<DrawableObject*>(cur_oc->obj);
		o->RemoveAlignmentRule(-1);
		needtodraw=1;
		return 0;
	}

	return 1;
}

/*! Make oc the current object if possible.
 * If oc is in selection, remove it from the selection.
 */
int AnchorInterface::SetCurrentObject(LaxInterfaces::ObjectContext *oc)
{
	if (!oc) return 1;

	 //remove from selection if there
	if (selection) {
		for (int c=0; c<selection->n(); c++) {
			if (oc->isequal(selection->e(c))) {
				selection->Remove(c);
				break;
			}
		}
	}

	if (cur_oc) delete cur_oc;
	cur_oc=dynamic_cast<VObjContext*>(oc->duplicate());
	AddAnchors(cur_oc, ANCHOR_Object);
	UpdateAnchors(ANCHOR_Page_Area);
	UpdateAnchors(ANCHOR_Margin_Area);
	UpdateAnchors(ANCHOR_Parents);

	return 0;
}

int AnchorInterface::AddToSelection(ObjectContext *oc, int where)
{
	if (!selection) selection=new Selection;
	return selection->Add(oc,where);
}

int AnchorInterface::AddToSelection(Laxkit::PtrStack<ObjectContext> &selection)
{
    int n=0;
    for (int c=0; c<selection.n; c++) {
        n+=AddToSelection(selection.e[c],-1);
    }
    return n;
}



} // namespace Laidout

