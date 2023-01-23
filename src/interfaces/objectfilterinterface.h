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
#ifndef LO_OBJECTFILTERINTERFACE_H
#define LO_OBJECTFILTERINTERFACE_H


#include "../dataobjects/objectfilter.h"
#include <lax/interfaces/aninterface.h>


namespace Laidout { 


//--------------------------- ObjectFilterInterface -------------------------------------

class ObjectFilterInterface : public LaxInterfaces::anInterface
{
  protected:
	int showdecs;

	Laxkit::ShortcutHandler *sc;

	//ObjectFilter *filter; //points to data->filter
	Laxkit::RefPtrStack<ObjectFilterNode> filternodes;
	DrawableObject *data; //points to dataoc->obj
	LaxInterfaces::ObjectContext *dataoc;
	int current;
	Laxkit::flatpoint offset;

	int hover;
	int hoverindex;

	Laxkit::LaxImage *nodes_icon;

	virtual int scan(int x, int y, unsigned int state, int *nhoverindex);
	//virtual int OtherObjectCheck(int x,int y,unsigned int state);

	virtual int send();

  public:
	enum ObjectFilterActions {
		OFI_None=0,
		OFI_On_Block,
		OFI_New_Filter,
		OFI_Remove,
		OFI_Mute,
		OFI_Move_Panel,
		OFI_Edit_Nodes,
		OFI_Rearrange,
		OFI_Refresh,
		OFI_MAX
	};

	unsigned int interface_flags;

	ObjectFilterInterface(LaxInterfaces::anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~ObjectFilterInterface();
	virtual LaxInterfaces::anInterface *duplicate(LaxInterfaces::anInterface *dup);
	virtual const char *IconId() { return "Filters"; }
	virtual const char *Name();
	virtual const char *whattype() { return "ObjectFilterInterface"; }
	virtual const char *whatdatatype();
	virtual LaxInterfaces::ObjectContext *Context(); 
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);

	virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	virtual int UseThisObject(LaxInterfaces::ObjectContext *oc);
	virtual int SelectNode(NodeBase *which);
	virtual int ActivateTool(int index);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual int Refresh();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual void ViewportResized();
};

} // namespace Laidout

#endif

