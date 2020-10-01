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
#ifndef _LAX_PATHINTERSECTIONSINTERFACE_H
#define _LAX_PATHINTERSECTIONSINTERFACE_H

#include <lax/interfaces/aninterface.h>


namespace LaxInterfaces { 


//--------------------------- PathIntersectionsInterface -------------------------------------

class PathIntersectionsInterface : public anInterface
{
  public:
	class BezEdge
	{
	  public:
		int p1, p2;
		flatpoint c1, c2; //relative to the points, for easy sorting
	};

	class IntersectionPoint
	{
	  public:
		flatpoint p;
		int type; // 0 bez on bez, 1 bez self intersection, 2 base point, not an intersection
		Laxkit::PtrStack<BezEdge> edges; //each edge radiates from p
	};

  protected:
	int showdecs;

	typedef Laxkit::NumStack<flatpoint> PointPath;
	Laxkit::PtrStack<PointPath> paths;

	Laxkit::PtrStack<PointPath> areas;


	Laxkit::NumStack<flatpoint> intersections;
	int pathi;

	double point_radius;
	int maxdepth;

	Laxkit::NumStack<int> selected;
	int hover;


	Laxkit::ShortcutHandler *sc;

	virtual int scan(int x, int y, unsigned int state);
	// virtual int OtherObjectCheck(int x,int y,unsigned int state);
	virtual void ClearSelection();

	// virtual int send();

	int DetectIntersections();
	void SortEdges();

  public:
	enum PathIntersectionsActions {
		INTERSECTIONS_None = 0,
		INTERSECTIONS_Something,
		INTERSECTIONS_MAX
	};

	unsigned int interface_flags;
	double threshhold;

	PathIntersectionsInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~PathIntersectionsInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "Path"; }
	virtual const char *Name();
	virtual const char *whattype() { return "PathIntersectionsInterface"; }
	virtual const char *whatdatatype();
	// virtual ObjectContext *Context(); 
	// virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	// virtual int Event(const Laxkit::EventData *data, const char *mes);
	//virtual Laxkit::ShortcutHandler *GetShortcuts();
	//virtual int PerformAction(int action);
	//virtual void deletedata();
	//virtual PathIntersectionsData *newData();

	// virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	// virtual int UseThisObject(ObjectContext *oc);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d);
	//virtual int DrawData(anObject *ndata,anObject *a1,anObject *a2,int info);
	virtual int Refresh();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
};

} // namespace LaxInterfaces

#endif

