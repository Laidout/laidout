//
//	
//    The Laxkit, a windowing toolkit
//    Please consult https://github.com/Laidout/laxkit about where to send any
//    correspondence about this software.
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 3 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; If not, see <http://www.gnu.org/licenses/>.
//
//    Copyright (C) 2018 by Tom Lechner
//



// Boolean path ops
// Seems like most widely used open source library (MIT licensed) is Andy Finnells:
// see Andy Finnell's three part tutorial: https://losingfight.com/blog/2011/07/
// Andy's bitbucket repo is gone, but found via figma: https://github.com/adamwulf/vectorboolean
// Swift port by Intitze: https://github.com/lrtitze/Swift-VectorBoolean


#include "pathintersectionsinterface.h"

#include <lax/interfaces/somedatafactory.h>
#include <lax/laxutils.h>
#include <lax/language.h>
#include <lax/bezutils.h>
#include <lax/vectors-out.h>


using namespace Laxkit;

#include <fstream>
#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {



//--------------------------- PathIntersectionsInterface -------------------------------------

/*! \class PathIntersectionsInterface
 * \ingroup interfaces
 * \brief Work in progress viewer for path boolean ops.
 */


PathIntersectionsInterface::PathIntersectionsInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	// interface_type = INTERFACE_Overlay;
	interface_flags=0;

	showdecs   = 1;
	needtodraw = 1;
	threshhold = 1e-5;
	maxdepth   = 0;

	//dataoc     = NULL;
	//data       = NULL;

	sc = NULL; //shortcut list, define as needed in GetShortcuts()

	paths.push(new Laxkit::NumStack<flatpoint>());
	paths.push(new Laxkit::NumStack<flatpoint>());

	pathi = 0;

	paths.e[1]->push(flatpoint(1,5));
	paths.e[1]->push(flatpoint(3,7));
	paths.e[1]->push(flatpoint(5,7));
	paths.e[1]->push(flatpoint(7,5));


	hover = -1;
}

PathIntersectionsInterface::~PathIntersectionsInterface()
{
	//if (dataoc) delete dataoc;
	//if (data) { data->dec_count(); data=NULL; }
	if (sc) sc->dec_count();
}

const char *PathIntersectionsInterface::whatdatatype()
{
	return nullptr;
}

/*! Name as displayed in menus, for instance.
 */
const char *PathIntersectionsInterface::Name()
{ return _("PathIntersections"); }


//! Return new PathIntersectionsInterface.
/*! If dup!=NULL and it cannot be cast to PathIntersectionsInterface, then return NULL.
 */
anInterface *PathIntersectionsInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new PathIntersectionsInterface(NULL,id,NULL);
	else if (!dynamic_cast<PathIntersectionsInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}


/*! Called when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int PathIntersectionsInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! Called when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int PathIntersectionsInterface::InterfaceOff()
{
	Clear(NULL);
	needtodraw=1;
	return 0;
}

/*! Clear references to d within the interface.
 */
void PathIntersectionsInterface::Clear(SomeData *d)
{
	intersections.flush();
	areas.flush();
	selected.flush();
	hover = -1;
}


///*! Return a context specific menu, typically in response to a right click.
// */
//Laxkit::MenuInfo *PathIntersectionsInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
//{ ***
//	if (no menu for x,y) return menu;
//
//	if (!menu) menu=new MenuInfo;
//	if (!menu->n()) menu->AddSep(_("Some new menu header"));
//
//	menu->AddToggleItem(_("New checkbox"), laximage_icon, YOUR_CHECKBOX_ID, checkbox_info, (istyle & STYLEFLAG) /*on*/, -1 /*where*/);
//	menu->AddItem(_("Some menu item"), YOUR_MENU_VALUE);
//	menu->AddSep(_("Some separator text"));
//	menu->AddItem(_("Et Cetera"), YOUR_OTHER_VALUE);
//	menp->AddItem(_("Item with info"), YOUR_ITEM_ID, LAX_OFF, items_info);
//
//	 //include <lax/iconmanager.h> if you want access to default icons
//	LaxImage icon = iconmanager->GetIcon("NewDirectory");
//	menp->AddItem(_("Item with icon"), icon, SOME_ITEM_ID, LAX_OFF, items_info);
//
//	return menu;
//}
//
///*! Intercept events if necessary, such as from the ContextMenu().
// */
//int PathIntersectionsInterface::Event(const Laxkit::EventData *evdata, const char *mes)
//{
//	if (!strcmp(mes,"menuevent")) {
//		 //these are sent by the ContextMenu popup
//		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(evdata);
//		int i	= s->info2; //id of menu item
//		int info = s->info4; //info of menu item
//
//		if ( i==SOME_MENU_VALUE) {
//			...
//		}
//
//		return 0; 
//	}
//
//	return 1; //event not absorbed
//}
//


#define INTERS_Normal  0
#define INTERS_Self    1

int PathIntersectionsInterface::Refresh()
{

	if (needtodraw==0) return 0;
	needtodraw=0;

	dp->NewBG(.2,0.,0.);
	dp->fontsize(.25);

	ScreenColor current(.3,1.,.3,1.);
	ScreenColor other(.6,.6,.6,1.);
	ScreenColor ipoints(.3,.3,1.,1.);

	// draw lines
	for (int c=0; c<paths.n; c++) {
		PointPath *path = paths.e[c];
		if (!path->n) continue;

		if (c == pathi) dp->NewFG(current);
		else dp->NewFG(other);
		dp->LineWidthScreen(c == pathi ? 2 : 1);

		dp->moveto(path->e[0]);
		for (int c2=1; c2<path->n; c2+=3) {
			dp->curveto(path->e[c2], path->e[(c2+1)%path->n], path->e[(c2+2)%path->n]);
		}
		dp->stroke(0);
	}

	// draw control points
	double radius = 5 / dp->Getmag();
	for (int c=0; c<paths.n; c++) {
		if (c != pathi) continue;

		if (c == pathi) dp->NewFG(current);
		else dp->NewFG(other);

		PointPath *path = paths.e[c];

		for (int c2=0; c2<path->n; c2++) {
			if (c2%3 == 1) {
				dp->drawline(path->e[c2], path->e[c2-1]);
			} else if (c2%3 == 2) {
				dp->drawline(path->e[c2], path->e[(c2+1)%path->n]);
			}
			dp->drawcircle(path->e[c2], radius, hover == c2);
		}
	}
	
	// draw intersection points
	point_radius = radius * .6;
	dp->NewFG(ipoints);
	for (int c=0; c<intersections.n; c++) {
		//IntersectionPoint *ip = intersections.e[c];
		//flatpoint p = ip->p;
		flatpoint p = intersections.e[c];

		if (p.info == INTERS_Normal) {
			//dp->NewFG(1., .6, .6);
			dp->drawcircle(p, point_radius, 0);
			//dp->DrawScreen();
			//flatpoint pp = dp->realtoscreen(p);
			//dp->drawnum(pp.x, pp.y, c);
			//dp->DrawReal();
			dp->drawnum(p.x, p.y, c);

		} else { //self intersection
			dp->drawrectangle(p.x - point_radius, p.y - point_radius, 2*point_radius, 2*point_radius, 0);
		}
	}

	// draw self intersections
	if (paths.e[1]->n > 3) {
		PointPath *path = paths.e[1];
		flatvector p;
		double t1=-1, t2=-1;
		if (bez_self_intersection(path->e[0], path->e[1], path->e[2], path->e[3], &p, &t1, &t2))
		{
			double rad = 2*point_radius;
			dp->drawrectangle(p.x - rad, p.y - rad, 2*rad, 2*rad, 0);
		}

	} 


	return 0;
}

int PathIntersectionsInterface::scan(int x, int y, unsigned int state)
{
	PointPath *path = paths.e[pathi];

	flatpoint p = dp->screentoreal(x,y);

	double threshhold2 = 10 / dp->Getmag();

	for (int c = 0; c < path->n; c++) {
		if (norm2(path->e[c] - p) < threshhold2) return c;
	}

	return -1;
}

int PathIntersectionsInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	int nhover = scan(x,y,state);
	buttondown.down(d->id,LEFTBUTTON,x,y, nhover);

	if (nhover != -1) {
		hover = nhover;

		if (!(state & ShiftMask)) {
			selected.flush();
			selected.pushnodup(nhover);
		}
	} else {
		if (!(state & ShiftMask)) {
			selected.flush();
			paths.e[pathi]->push(dp->screentoreal(x,y));
			selected.pushnodup(paths.e[pathi]->n-1);
		}
	}

	needtodraw=1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int PathIntersectionsInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	buttondown.up(d->id,LEFTBUTTON);
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int PathIntersectionsInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *m)
{
	if (!buttondown.any()) {
		// update any mouse over state
		int nhover = scan(x,y,state);
		if (nhover != hover) {
			hover = nhover;
			needtodraw = 1;
			return 0;
		}
		return 1;
	}

	//else deal with mouse dragging...

	int oldx, oldy;
    buttondown.move(m->id,x,y, &oldx,&oldy);
	//int hovered = -1;
    //buttondown.getextrainfo(m->id,LEFTBUTTON, &hovered);

	flatpoint d = dp->screentoreal(x,y) - dp->screentoreal(oldx,oldy);

	for (int c=0; c<selected.n; c++) {
		int i = selected.e[c];
		paths.e[pathi]->e[i] += d;
		if (i%3 == 0) {
			if (i > 0) {
				paths.e[pathi]->e[i-1] += d;
			}
			if (i < paths.e[pathi]->n-1) {
				paths.e[pathi]->e[i+1] += d;
			}
		}
	}

	DetectIntersections();

	needtodraw=1;
	return 0;
}

void PathIntersectionsInterface::ClearSelection()
{
	selected.flush();
	needtodraw = 1;
}

int PathIntersectionsInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask|AltMask|MetaMask)) {
		//deal with various modified keys...
	}

	if (ch==LAX_Esc) { //the various possible keys beyond normal ascii printable chars are defined in lax/laxdefs.h
		if (selected.n == 0) {
			if (paths.e[1]->n) paths.e[1]->flush();
			else if (paths.e[0]->n) paths.e[0]->flush();
			else return 1; //need to return on plain escape, so that default switching to Object tool happens
		} else ClearSelection();

		needtodraw=1;
		return 0;

	} else if (ch==LAX_Enter) {
		DetectIntersections();
		return 0;

	} else if (ch==LAX_Up) {
		maxdepth++;
		PostMessage2("Recurse %d", maxdepth);
		DetectIntersections();
		needtodraw = 1;
		return 0;

	} else if (ch==LAX_Down) {
		maxdepth--;
		if (maxdepth < 1) maxdepth = 1;
		else {
			PostMessage2("Recurse %d", maxdepth);
			DetectIntersections();
			needtodraw = 1;
		}
		return 0;
		
	} else if (ch >= '1' && ch <= '9') {
		int i = (int)(ch - '1');
		if (i >= paths.n) i = paths.n-1;
		pathi = i;
		selected.flush();
		PostMessage2("Edit path %d", pathi+1);
		needtodraw = 1;
		return 0;

	} else if (ch == 's') {
		//de castlejau subdivide
		PointPath *opath = paths.e[pathi];
		PointPath path;

		flatpoint pts[5];

		for (int c=0; c<opath->n; c+= 3) {
			path.push(opath->e[c]);

			if (c < opath->n-1) {
				bez_subdivide_decasteljau(opath->e[c], opath->e[(c+1)%opath->n], opath->e[(c+2)%opath->n], opath->e[(c+3)%opath->n],
					pts[0], pts[1], pts[2], pts[3], pts[4]);
				path.push(pts[0]); //c
				path.push(pts[1]); //c
				path.push(pts[2]); //p
				path.push(pts[3]); //c
				path.push(pts[4]); //c
			}
		}

		int n;
		flatpoint *pp = path.extractArray(&n);
		opath->insertArray(pp, n);

		hover = -1;
		needtodraw = 1;
		return 0;

	} else if (ch == 'd') { //dump
		ofstream out("point_dump.txt", std::ofstream::out);
		for (int c=0; c < paths.n; c++) {
			out<<c<<":"<<endl;
			for (int c2=0; c2< paths.e[c]->n; c2++) {
				out<<"  "<<paths.e[c]->e[c2].x<<", "<<paths.e[c]->e[c2].y<<", "<<endl;
			}
		}
		out.close();
		return 0;

	} else {
		 //default shortcut processing

		if (!sc) GetShortcuts();
		if (sc) {
			int action = sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
			if (action >= 0) {
				return PerformAction(action);
			}
		}
	}

	return 1; //key not dealt with, propagate to next interface
}

//Laxkit::ShortcutHandler *PathIntersectionsInterface::GetShortcuts()
//{ ***
//	if (sc) return sc;
//    ShortcutManager *manager=GetDefaultShortcutManager();
//    sc=manager->NewHandler(whattype());
//    if (sc) return sc;
//
//    //virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);
//
//    sc=new ShortcutHandler(whattype());
//
//	//sc->Add([id number],  [key], [mod mask], [mode], [action string id], [description], [icon], [assignable]);
//    sc->Add(PATHINTERSECTIONS_Something,  'B',ShiftMask|ControlMask,0, "BaselineJustify", _("Baseline Justify"),NULL,0);
//    sc->Add(PATHINTERSECTIONS_Something2, 'b',ControlMask,0, "BottomJustify"  , _("Bottom Justify"  ),NULL,0);
//    sc->Add(PATHINTERSECTIONS_Something3, 'd',ControlMask,0, "Decorations"    , _("Toggle Decorations"),NULL,0);
//	sc->Add(PATHINTERSECTIONS_Something4, '+',ShiftMask,0,   "ZoomIn"         , _("Zoom in"),NULL,0);
//	sc->AddShortcut('=',0,0, PATHINTERSECTIONS_Something); //add key to existing action
//
//    manager->AddArea(whattype(),sc);
//    return sc;
//}
//
///*! Return 0 for action performed, or nonzero for unknown action.
// */
//int PathIntersectionsInterface::PerformAction(int action)
//{ ***
//	return 1;
//}


int cmp_Direction(const void *v1, const void *v2)
{
	const PathIntersectionsInterface::BezEdge *e1 = *((const PathIntersectionsInterface::BezEdge**)v1);
	const PathIntersectionsInterface::BezEdge *e2 = *((const PathIntersectionsInterface::BezEdge**)v2);
	double a1 = atan2(e1->c1.x, e1->c1.y); //atan2 returns -pi..pi
	double a2 = atan2(e2->c1.x, e2->c1.y); //atan2 returns -pi..pi
	return a1 > a2 ? 1 : (a1 < a2 ? -1 : 0);
}

void PathIntersectionsInterface::SortEdges()
{
//	for (int c=0; c<intersections.n; c++) {
//		IntersectionPoint *ip = intersections.e[c];
//		//if (ip->type == 2) continue; //we know there's 1 or 2 edges, no sorting necessary
//		if (ip->edges.n <= 2) continue;
//
//		qsort(ip->edges.e, ip->edges.n, sizeof(BezEdge*), cmp_Direction);
//		//for (int c2=0; c2 < ip->edges.n; c2++) {
//		//}
//	}
}


int PathIntersectionsInterface::DetectIntersections()
{
	PointPath *path1 = paths.e[0];
	PointPath *path2 = paths.e[1];

	flatpoint pts[9];
	double t1[9], t2[9];
	int num;

	intersections.flush();

	for (int c = 0; c < path1->n-1; c += 3) {
		for (int c2 = 0; c2 < path2->n-1; c2 += 3) {
			num = 0;

			bez_intersect_bez(
					path1->e[c], path1->e[(c+1)%path1->n], path1->e[(c+2)%path1->n], path1->e[(c+3)%path1->n],
					path2->e[c], path2->e[(c+1)%path2->n], path2->e[(c+2)%path2->n], path2->e[(c+3)%path2->n],
					pts, t1, t2, num,
					threshhold,
					0,0,1,
					1, maxdepth
				);

			DBG cerr << "bez segment intersections "<<c<<" with "<<c2<<": "<<num<<", threshhold: "<<threshhold<<", maxdepth: " << maxdepth<<endl;
			for (int c3=0; c3<num; c3++) {
				intersections.push(pts[c3]);
				DBG flatpoint pp1 = bez_point(t1[c3], path1->e[c], path1->e[(c+1)%path1->n], path1->e[(c+2)%path1->n], path1->e[(c+3)%path1->n]);
				// DBG flatpoint pp2 = bez_point(t2[c3], path1->e[c], path1->e[(c+1)%path1->n], path1->e[(c+2)%path1->n], path1->e[(c+3)%path1->n]);
				DBG cerr << "bez  "<<c3<<", t: "<<t1[c3]<<", p: "<<pts[c3]<<"  p(t): "<<pp1<<"  diff: "<<norm(pp1-pts[c3])<<endl;
			}
		}
	}

	PostMessage2("Maxdepth: %d, %d intersections", maxdepth, intersections.n);

	return 0;
}



} // namespace LaxInterfaces

