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



#include "objectfilterinterface.h"

#include <lax/interfaces/somedatafactory.h>
#include <lax/laxutils.h>
#include <lax/language.h>


//template implementation:
//#include <lax/lists.cc>
#include <lax/refptrstack.cc>


using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


namespace Laidout {


//--------------------------- ObjectFilterInterface -------------------------------------

/*! \class ObjectFilterInterface
 * Tool to provide easy access to filter nodes than can be edited non-nodally,
 * such as PerspectiveTransform.
 */


ObjectFilterInterface::ObjectFilterInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	interface_flags=0;

	showdecs   = 1;
	needtodraw = 1;

	dataoc     = NULL;
	data       = NULL;
	current    = -1;

	hover = OFI_None;
	hoverindex = -1;

	sc = NULL; //shortcut list, define as needed in GetShortcuts()
}

ObjectFilterInterface::~ObjectFilterInterface()
{
	if (dataoc) delete dataoc;
	if (data) { data->dec_count(); data=NULL; }
	if (sc) sc->dec_count();
}

const char *ObjectFilterInterface::whatdatatype()
{ 
	//return "ObjectFilterData";
	return NULL; // NULL means this tool is creation only, it cannot edit existing data automatically
}

/*! Name as displayed in menus, for instance.
 */
const char *ObjectFilterInterface::Name()
{ return _("Filter Editor"); }


//! Return new ObjectFilterInterface.
/*! If dup!=NULL and it cannot be cast to ObjectFilterInterface, then return NULL.
 */
anInterface *ObjectFilterInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup = new ObjectFilterInterface(NULL,id,NULL);
	else if (!dynamic_cast<ObjectFilterInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

//! Use the object at oc if it is an ObjectFilterData.
int ObjectFilterInterface::UseThisObject(ObjectContext *oc)
{
	if (!oc) return 0;

	DrawableObject *ndata=dynamic_cast<DrawableObject *>(oc->obj);
	if (!ndata) return 0;

	if (data && data!=ndata) Clear(NULL);
	if (dataoc) delete dataoc;
	dataoc = oc->duplicate();

	if (data != ndata) {
		data = ndata;
		data->inc_count();

		ObjectFilter *f = dynamic_cast<ObjectFilter *>(data->filter);
		if (f != NULL) {
			filternodes.flush();
			f->FindInterfaceNodes(filternodes);
		}
	}

	needtodraw=1;
	return 1;
}

int ObjectFilterInterface::UseThis(anObject *nobj, unsigned int mask)
{
	if (!nobj) return 1;
	ObjectFilter *f = dynamic_cast<ObjectFilter *>(nobj);
	if (f != NULL) {
		if (data && data->filter != f) {
			filternodes.flush();
			f->FindInterfaceNodes(filternodes);
		}

		needtodraw=1;
		return 1;
	}
	return 0;
}

/*! Return the object's ObjectContext to make sure that the proper context is already installed
 * before Refresh() is called.
 */
ObjectContext *ObjectFilterInterface::Context()
{
	return NULL;
}

/*! Called when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int ObjectFilterInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! Called when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int ObjectFilterInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	hover = OFI_None;
	hoverindex = -1;
	needtodraw=1;
	return 0;
}

/*! Clear references to d within the interface.
 */
void ObjectFilterInterface::Clear(SomeData *d)
{
	if (dataoc) { delete dataoc; dataoc=NULL; }
	if (data) { data->dec_count(); data=NULL; }
	filternodes.flush();
	current = -1;
	hover = OFI_None;
	hoverindex = -1;
}

void ObjectFilterInterface::ViewportResized()
{
	// if necessary, do stuff in response to the parent window size changed
}

/*! Return a context specific menu, typically in response to a right click.
 */
Laxkit::MenuInfo *ObjectFilterInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (!menu) menu=new MenuInfo;
	//if (!menu->n()) menu->AddSep(_("Some new menu header"));

	menu->AddItem(_("Edit Nodes..."), OFI_Edit_Nodes);

	//menu->AddToggleItem(_("New checkbox"), laximage_icon, YOUR_CHECKBOX_ID, checkbox_info, (istyle & STYLEFLAG) /*on*/, -1 /*where*/);

	return menu;
}

/*! Intercept events if necessary, such as from the ContextMenu().
 */
int ObjectFilterInterface::Event(const Laxkit::EventData *data, const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		 //these are sent by the ContextMenu popup
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(data);
		int i	= s->info2; //id of menu item
		//int info = s->info4; //info of menu item

		if (i == OFI_Edit_Nodes) {
			//...make child a NodeInterface with filter, with current node selected
			return 0;
		}

		return 0; 
	}

	return 1; //event not absorbed
}


///*! Draw some data other than the current data.
// * This is called during screen refreshes.
// */
//int ObjectFilterInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)
//{
////	if (!ndata || dynamic_cast<ObjectFilterData *>(ndata)==NULL) return 1;
////
////	ObjectFilterData *bzd=data;
////	data=dynamic_cast<ObjectFilterData *>(ndata);
////
////	 // store any other state we need to remember
////	 // and update to draw just the temporary object, no decorations
////	int td=showdecs,ntd=needtodraw;
////	showdecs=0;
////	needtodraw=1;
////
////	Refresh();
////
////	 //now restore the old state
////	needtodraw=ntd;
////	showdecs=td;
////	data=bzd;
//	return 1;
//}


int ObjectFilterInterface::Refresh()
{

	if (needtodraw==0) return 0;
	needtodraw=0;


	dp->DrawScreen();

	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	dp->NewFG(curwindow->win_colors->fg);

	dp->NewFG(coloravg(curwindow->win_colors->fg,curwindow->win_colors->bg, .25));
    dp->NewBG(coloravg(curwindow->win_colors->fg,curwindow->win_colors->bg, .75));


	double th = dp->textheight();
	double width;

	if (filternodes.n == 0) {
		const char *str = _("No filters");
		width = dp->textextent(str,-1, NULL,NULL);
		dp->drawRoundedRect(offset.x,offset.y, width+th,th*1.5, th/3, false, th/3, false, 2);
		dp->textout(offset.x + th/2, offset.y + th/4, str,-1, LAX_TOP|LAX_LEFT);

	} else {
		//draw one block for each filter
		double w, x=0;
		for (int c=0; c<filternodes.n; c++) {
			 //draw:
			 //   Name
			 //   [eyeball] [remove]
			width = 2*th;
			w = dp->textextent(filternodes.e[c]->Label(),-1, NULL,NULL);
			if (w > width) width = w;
			width += th;

			dp->NewFG(coloravg(curwindow->win_colors->fg,curwindow->win_colors->bg, .25));

			 //draw box
			dp->LineWidthScreen(current == c ? 3 : 1);
			dp->drawRoundedRect(x,0, width,th*2.5, th/3, false, th/3, false, 2);
			dp->LineWidthScreen(1);

			 //label
			dp->textout(x+width/2,th/4, filternodes.e[c]->Label(),-1, LAX_TOP|LAX_HCENTER);

			 //eyeball
			dp->NewFG(coloravg(curwindow->win_colors->fg,curwindow->win_colors->bg, .25));
			dp->NewBG(1.,1.,1.);
			dp->LineWidthScreen(hover == OFI_Mute && hoverindex == c ? 2 : 1);
			dp->drawthing(x+th, th*1.75, th/2,-th/2, 2, filternodes.e[c]->IsMuted() ? THING_Closed_Eye : THING_Open_Eye);
			dp->LineWidthScreen(1);

			 //remove
			if (hover == OFI_Remove && hoverindex == c) {
				dp->NewFG(1.,0.,0.);
				dp->drawcircle(x + width-th, th*1.75, th/2, 1);
				dp->NewFG(1.,1.,1.);

			} else dp->NewFG(curwindow->win_colors->fg);

			dp->drawthing(x+width - th, th*1.75, th/3,th/3, 0, THING_X);

			x += width;
		}
	}

	dp->DrawReal();

	return 0;
}

int ObjectFilterInterface::scan(int x, int y, unsigned int state, int *nhoverindex)
{
	*nhoverindex = -1;

	double th = dp->textheight();
	double width;
	double w, xx=0;


	if (filternodes.n == 0) {
		const char *str = _("No filters");
		width = dp->textextent(str,-1, NULL,NULL);
		if (x >= offset.x && x <= offset.x + width+th && y >= offset.y && y <= offset.y + th*1.5) {
			return OFI_New_Filter;
		}
		return OFI_None;
	}

	for (int c=0; c<filternodes.n; c++) {
		 //   Name
		 //   [eyeball] [remove]
		width = 2*th;
		w = dp->textextent(filternodes.e[c]->Label(),-1, NULL,NULL);
		if (w > width) width = w;
		width += th;

		if (x >= offset.x + xx && x <= offset.x + xx + width && y >= offset.y && y <= offset.y + th*2.5) {
			*nhoverindex = c;
			if (y > offset.y + 1.25*th) {
				if (x < offset.x + xx + 1.5*th) return OFI_Mute;
				if (x > offset.x + xx + width - 2*th) return OFI_Remove;
			}
			return OFI_On_Block;
		}
	}


	return OFI_None;
}

int ObjectFilterInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{

	int nhoverindex = -1;
	int nhover = scan(x,y,state, &nhoverindex);

	buttondown.down(d->id, LEFTBUTTON, x,y, nhover, nhoverindex);

	needtodraw=1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

/*! Return 0 for success.
 */
int ObjectFilterInterface::ActivateTool(int index)
{
	if (index < 0 || index >= filternodes.n) return 1;
	if (!dataoc) return 2;

	current = index;

	ObjectFilterNode *fnode = filternodes.e[index];
	anInterface *i = fnode->ObjectFilterInterface()->duplicate(NULL);
	//NodeProperty *in = fnode->FindProperty("in");
	//NodeProperty *out = fnode->FindProperty("out");

	ObjectFilterInfo info(dataoc, NULL, fnode);

	i->UseThis(&info);
	i->owner = this;
	child = i;
	viewport->Push(i,-1,0);
	const char *label = fnode->Label();
	if (!label) label="";
	char str[strlen(_("Edit %s") + strlen(label)) + 2];
	sprintf(str, _("Edit %s"), label);
	
	PostMessage(str);

	return 0;
}

int ObjectFilterInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	int hovered = -1, hoveredindex = -1;
	int dragged = buttondown.up(d->id,LEFTBUTTON, &hovered, &hoveredindex);
	//int nhoverindex = -1;
	//int nhover = scan(x,y,state, &nhoverindex);

	dragged = (dragged >= 5);

	if (!dragged && hovered == OFI_On_Block) { 
		if (current != hoveredindex) {
			ActivateTool(hoveredindex);
		}
		needtodraw=1;
		return 0;
	}

	if (hovered == OFI_Remove) {
		if (!dragged) {
			//***ok clicked down and up on same
		}
		return 0;

	} else if (hovered == OFI_Mute) {
		if (!dragged) {
			current = hoveredindex;
			if (current >= 0) {
				filternodes.e[current]->Mute(filternodes.e[current]->IsMuted());
				needtodraw=1;
			}
		}
		return 0;

	} else if (hovered == OFI_Rearrange) {
		//***
	}

	return 0; //return 0 for absorbing event, or 1 for ignoring
}


int ObjectFilterInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	DBG cerr << " **********************ofi "<< buttondown.any() << endl;

	if (!buttondown.any()) {
		// update any mouse over state
		int nhoverindex = -1;
		int nhover = scan(x,y,state, &nhoverindex);
		DBG cerr <<" ObjectFilterInterface::MouseMove: "<<nhover<<"  "<<nhoverindex<<endl;

		if (nhover != hover || nhoverindex != hoverindex) {
			hover = nhover;
			hoverindex = nhoverindex;
			if (nhover == OFI_Mute) PostMessage(_("Toggle this filter"));
			else if (nhover == OFI_Remove) PostMessage(_("Remove this filter"));
			else PostMessage("");
			needtodraw=1;
		}
		return 1;
	}

	//else deal with mouse dragging...
	int oldx,oldy;
    int oldhover, oldhoveri;
    buttondown.move(d->id,x,y, &oldx,&oldy);
    buttondown.getextrainfo(d->id,LEFTBUTTON, &oldhover, &oldhoveri);


	//needtodraw=1;
	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}


int ObjectFilterInterface::send()
{
//	if (owner) {
//		RefCountedEventData *data=new RefCountedEventData(paths);
//		app->SendMessage(data,owner->object_id,"ObjectFilterInterface", object_id);
//
//	} else {
//		if (viewport) viewport->NewData(paths,NULL);
//	}

	return 0;
}

int ObjectFilterInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	 //default shortcut processing

	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	return 1; //key not dealt with, propagate to next interface
}

Laxkit::ShortcutHandler *ObjectFilterInterface::GetShortcuts()
{
	if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;

    sc=new ShortcutHandler(whattype());

	//sc->Add([id number],  [key], [mod mask], [mode], [action string id], [description], [icon], [assignable]);
    //sc->Add(OBJECTFILTER_Something,  'B',ShiftMask|ControlMask,0, "BaselineJustify", _("Baseline Justify"),NULL,0);
    //sc->Add(OBJECTFILTER_Something2, 'b',ControlMask,0, "BottomJustify"  , _("Bottom Justify"  ),NULL,0);
    //sc->Add(OBJECTFILTER_Something3, 'd',ControlMask,0, "Decorations"    , _("Toggle Decorations"),NULL,0);
	//sc->Add(OBJECTFILTER_Something4, '+',ShiftMask,0,   "ZoomIn"         , _("Zoom in"),NULL,0);
	//sc->AddShortcut('=',0,0, OBJECTFILTER_Something); //add key to existing action

    manager->AddArea(whattype(),sc);
    return sc;
}

/*! Return 0 for action performed, or nonzero for unknown action.
 */
int ObjectFilterInterface::PerformAction(int action)
{
	//if (action == ***) {
	//}
	return 1;
}

} // namespace Laidout

