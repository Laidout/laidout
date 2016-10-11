//
//	
//    The Laxkit, a windowing toolkit
//    Please consult http://laxkit.sourceforge.net about where to send any
//    correspondence about this software.
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    Copyright (C) 2016 by Tom Lechner
//



#include "nodeinterface.h"

#include <lax/laxutils.h>
#include <lax/bezutils.h>
#include <lax/popupmenu.h>
#include <lax/language.h>


//Template implementation:
#include <lax/lists.cc>
#include <lax/refptrstack.cc>


using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


namespace Laidout {


//-------------------------------------- NodeColors --------------------------
/*! \class NodeColors
 * Holds NodeInterface styling.
 */

NodeColors::NodeColors()
  : default_property(1.,1.,1.,1.),
	connection(.5,.5,.5,1.),
	sel_connection(1.,0.,1.,1.),

	fg(.2,.2,.2,1.),
	bg(.8,.8,.8,1.),
	text(0.,0.,0.,1.),
	border(.2,.2,.2,1.),

	mo_border(.3,.3,.3,1.),
	mo_bg(.7,.7,.7,1.),

	selected_border(.4,.4,0.,1.),
	selected_bg(.9,.9,.9,1.),

	selected_mo_border(.35,.35,.2,1.),
	selected_mo_bg(.85,.85,.85,1.)
{
	state=0;
	font=NULL;
	next=NULL;
}

NodeColors::~NodeColors()
{
	if (font) font->dec_count();
	if (next) next->dec_count();
}

int NodeColors::Font(Laxkit::LaxFont *newfont, bool absorb_count)
{
	if (font) font->dec_count();
	font=newfont; 
	if (font && !absorb_count) font->inc_count();
	return 0;
}

//-------------------------------------- NodeConnection --------------------------

/*! \class NodeConnection
 */

NodeConnection::NodeConnection()
{
	from=to=NULL;
	fromprop=toprop=NULL;
}

NodeConnection::NodeConnection(NodeBase *nfrom, NodeBase *nto, NodeProperty *nfromprop, NodeProperty *ntoprop)
{
	from=nfrom;
	to=nto;
	fromprop=nfromprop;
	toprop=ntoprop;
}

NodeConnection::~NodeConnection()
{
}


//-------------------------------------- NodeProperty --------------------------

/*! \class NodeProperty
 * Base class for properties of nodes, either input or output.
 */

NodeProperty::NodeProperty()
{
	color.rgbf(1.,1.,1.,1.);

	owner=NULL;
	data=NULL;
	name=NULL;

	is_input=false;
	is_inputable=false; //default true for something that allows links in
}

/*! If !absorb_count, then ndata's count gets incremented.
 */
NodeProperty::NodeProperty(bool input, bool inputable, const char *nname, Laxkit::anObject *ndata, int absorb_count)
{
	color.rgbf(1.,1.,1.,1.);

	owner=NULL;
	data=ndata;
	if (data && !absorb_count) data->inc_count();

	is_input = input;
	is_inputable = inputable;

	name=newstr(nname);
}

NodeProperty::~NodeProperty()
{
	delete[] name;
	if (data) data->dec_count();
}

/*! Return an interface if you want to have a custom for changing properties.
 */
anInterface *NodeProperty::PropInterface()
{ 
	return NULL;
}


//-------------------------------------- NodeBase --------------------------

/*! \class NodeBase
 *
 * Class to hold node information for a NodeInterface.
 */


NodeBase::NodeBase()
{
	Name = NULL;
	total_preview = NULL;
	colors = NULL;
	collapsed = false;
	deletable = true; //usually at least one node will not be deletable
}

NodeBase::~NodeBase()
{
	delete[] Name;
	if (colors) colors->dec_count();
	if (total_preview) total_preview->dec_count();
}


/*! Passing in NULL will set this->colors to NULL.
 */
int NodeBase::InstallColors(NodeColors *newcolors, bool absorb_count)
{
	if (colors) colors->dec_count();
	colors = newcolors;
	if (colors && !absorb_count) colors->inc_count();
	return 0;
}

/*! Return whether the node has acceptable values.
 * Default is to return 0 for success. -1 means bad inputs. 1 means needs updating.
 */
int NodeBase::GetStatus()
{
	return 0;
}

/*! Call whenever any of the inputs change, update outputs.
 * Default placeolder is to do nothing.
 *
 * Return 1 for successful update, or 0 for unable to update for some reason.
 */
int NodeBase::Update()
{
	return 1;
}

/*! Update the bounds to be just enough to encase everything.
 */
int NodeBase::Wrap()
{
	if (!colors) return -1;

	 //find overall width and height
	double th = colors->font->textheight();

	height = th*(1+.5+properties.n);
	width = colors->font->extent(Name,-1);

	double w;
	for (int c=0; c<properties.n; c++) {
		w=colors->font->extent(properties.e[c]->Name(),-1);
		if (w>width) width=w;
	}

	width += 3*th;

	 //update link position
	for (int c=0; c<properties.n; c++) {
		properties.e[c]->pos.y = th*(1+.5+c+.5);
		if (properties.e[c]->is_input) properties.e[c]->pos.x = 0;
		else properties.e[c]->pos.x = width;
	} 

	return 0;
}

/*! -1 toggle, 0 open, 1 collapsed
 */
int NodeBase::Collapse(int state)
{
	if (state==-1) state = !collapsed;
	if (state) collapsed=true; else collapsed=false;
	return collapsed;
}

int NodeBase::HasConnection(NodeProperty *prop)
{
	***
}


//-------------------------------------- Common Node Types --------------------------


//------------ DoubleNode

Laxkit::anObject *newDoubleNode(Laxkit::anObject *ref)
{
	NodeBase *node = new NodeBase;
	node->Id("Value");
	makestr(node->Name, _("Value"));
	node->properties.push(new NodeProperty(false, false, _("Value"), new DoubleValue(0), 1)); 
	return node;
}


//------------ MathNode

class MathNode : public NodeBase
{
  public:
	int operation; // + - * / %
	double a,b,result;
	MathNode(int op=0, double aa=0, double bb=0);
	virtual int Update();
	virtual int GetStatus();
};

#define OP_Add      0
#define OP_Subtract 1
#define OP_Multiply 2
#define OP_Divide   3
#define OP_Mod      4

MathNode::MathNode(int op, double aa, double bb)
{
	makestr(Name, _("Math"));
	a=aa;
	b=bb;
	operation = op;
	properties.push(new NodeProperty(true, false, "Op", NULL, 0));
	properties.push(new NodeProperty(true, true, "A", new DoubleValue(a), 1));
	properties.push(new NodeProperty(true, true, "B", new DoubleValue(b), 1));
	properties.push(new NodeProperty(false, true, "Result", NULL, 0));
}

int MathNode::GetStatus()
{
	if ((operation==OP_Divide || operation==OP_Mod) && b==0) return -1;
	if (!properties.e[3]->data) return 1;
	return 0;
}

int MathNode::Update()
{
	if      (operation==OP_Add) result = a+b;
	else if (operation==OP_Subtract) result = a-b;
	else if (operation==OP_Multiply) result = a*b;
	else if (operation==OP_Divide) {
		if (b!=0) result = a/b;
		else {
			result=0;
			return 0;
		}

	} else if (operation==OP_Mod) {
		if (b!=0) result = a-b*int(a/b);
		else {
			result=0;
			return 0;
		}
	}

	if (!properties.e[3]->data) properties.e[3]->data=new DoubleValue(result);
	else dynamic_cast<DoubleValue*>(properties.e[3]->data)->d = result;

	return 1;
}

Laxkit::anObject *newMathNode(Laxkit::anObject *ref)
{
	return new MathNode();
}


//--------------------------- SetupDefaultNodeTypes()

int SetupDefaultNodeTypes(Laxkit::ObjectFactory *factory)
{
	 //--- DoubleNode
	factory->DefineNewObject(getUniqueNumber(), "Value", newDoubleNode, NULL);

	 //--- MathNode
	factory->DefineNewObject(getUniqueNumber(), "Math",  newMathNode,   NULL);

	return 0;
}


//-------------------------------------- NodeInterface --------------------------

/*! \class NodeInterface
 * \ingroup interfaces
 */


NodeInterface::NodeInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	node_interface_style=0;

	node_factory = new ObjectFactory;
	SetupDefaultNodeTypes(node_factory);

	nodes=NULL;

	lasthover=-1;
	lasthoverprop=-1;
	lastconnection=-1;
	hover_action=NODES_None;
	showdecs=1;
	needtodraw=1;
	font = app->defaultlaxfont;
	font->inc_count();
	slot_radius=.25;

	color_controls.rgbf(.7,.5,.7,1.);

	sc=NULL; //shortcut list, define as needed in GetShortcuts()
}

NodeInterface::~NodeInterface()
{
	if (nodes) nodes->dec_count();
	if (font) font->dec_count();
	if (node_factory) node_factory->dec_count();
	if (sc) sc->dec_count();
}

const char *NodeInterface::whatdatatype()
{
	return "Nodes";
}

/*! Name as displayed in menus, for instance.
 */
const char *NodeInterface::Name()
{ return _("Node tool"); }


//! Return new NodeInterface.
/*! If dup!=NULL and it cannot be cast to NodeInterface, then return NULL.
 */
anInterface *NodeInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new NodeInterface(NULL,id,NULL);
	else if (!dynamic_cast<NodeInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

/*! Normally this will accept some common things like changes to line styles, like a current color.
 */
int NodeInterface::UseThis(anObject *nobj, unsigned int mask)
{
	if (!nobj) return 1;
//	LineStyle *ls=dynamic_cast<LineStyle *>(nobj);
//	if (ls!=NULL) {
//		if (mask&GCForeground) { 
//			linecolor=ls->color;
//		}
////		if (mask&GCLineWidth) {
////			linecolor.width=ls->width;
////		}
//		needtodraw=1;
//		return 1;
//	}
	return 0;
}

/*! Any setup when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int NodeInterface::InterfaceOn()
{ 
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! Any cleanup when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int NodeInterface::InterfaceOff()
{ 
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
	
	// *** need to clear any unattached connections
}

void NodeInterface::Clear(SomeData *d)
{
	selected.flush();
	grouptree.flush();
}

Laxkit::MenuInfo *NodeInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	//if (no menu for x,y) return NULL;

	if (!menu) menu=new MenuInfo;
	if (!menu->n()) menu->AddSep(_("Nodes"));

	menu->AddItem(_("Add node..."), NODES_Add_Node);


	return menu;
}

int NodeInterface::Event(const Laxkit::EventData *data, const char *mes)
{
    if (!strcmp(mes,"menuevent")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
        int i =s->info2; //id of menu item

        if (i==NODES_Add_Node) {
			PerformAction(NODES_Add_Node);
		}

		return 0;

	} else if (!strcmp(mes,"addnode")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);

		const char *what = s->str;
		if (isblank(what)) return 0;

		if (!nodes) {
			nodes = new Nodes;
			nodes->InstallColors(new NodeColors, true);
			nodes->colors->Font(font, false);
		}

		ObjectFactoryNode *type;
		for (int c=0; c<node_factory->types.n; c++) {
			type=node_factory->types.e[c];
			if (!strcmp(type->name, what)) {
				anObject *obj = type->newfunc(NULL);
				NodeBase *newnode = dynamic_cast<NodeBase*>(obj);
				flatpoint p = nodes->m.transformPointInverse(lastpos);
				newnode->x=p.x;
				newnode->y=p.y;
				newnode->InstallColors(nodes->colors, false);
				newnode->Wrap();

				nodes->nodes.push(newnode);
				newnode->dec_count();

				break;
			}
		}

		return 0;
	}

	return 1; //event not absorbed
}

///*! Draw an open path such that first point is BG() and last point is FG().
// * Assume points are a bez path, v-c-c-v-...-v-c-c-v.
// *
// * n is the number of points in pts.
// *
// * Pretty slow for long paths.
// */
//void DrawColoredPath(flatpoint *pts, int n)
//{
//	flatpoint bpts[20*(n)/3];
//
//	 //find length
//	double length=0, l=0;
//
//	 //grab polyline from bez
//	bez_points(bpts, (n-1)/3+1, pts, 20);
//
//	for (int c2=1; c2<((n-1)/3+1)*20; c2++) {
//		length += norm(bpts[c2]-bpts[c2-1]);
//	}
//
//	 //draw points based on percent total length
//	for (int c2=0; c2<((n-1)/3+1)*20; c2++) {
//		dp->NewFG(coloravg(bg, fg, l/length));
//		dp->drawline(bpts[c2], bpts[c2+1]);
//
//		l+=norm(bpts[c2+1]-bpts[c2]):
//	}
//}

int NodeInterface::IsSelected(NodeBase *node)
{
	return selected.findindex(node) >= 0;
}

int NodeInterface::Refresh()
{

	if (needtodraw==0) return 0;
	needtodraw=0;


	if (!nodes) return 0;

	dp->PushAxes();
	dp->NewTransform(nodes->m.m());
	dp->font(font);

	 //---draw connections
	dp->NewFG(&nodes->colors->connection);

	for (int c=0; c<nodes->connections.n; c++) {
		//dp->DrawColoredPath(nodes->nodes.e[c]->connections.e[c]->path.e,
		//					nodes->nodes.e[c]->connections.e[c]->path.n);
		dp->drawlines(nodes->connections.e[c]->path.e,
					  nodes->connections.e[c]->path.n,
					  false, 0);
	}

	 //---draw nodes:
	 //  box+border
	 //  label
	 //  expanded arrow
	 //  preview
	 //  ins/outs
	NodeBase *node;
	ScreenColor *border, *bg, *fg;
	NodeColors *colors=NULL;
	double th = dp->textheight();

	for (int c=0; c<nodes->nodes.n; c++) {
		node = nodes->nodes.e[c];

		 //set up colors based on whether the node is selected or mouse overed
		if (node->colors) colors=node->colors;
		else colors = nodes->colors;

		border = &colors->border;
		bg = &colors->bg;
		fg = &colors->fg;

		if (IsSelected(node))  { 
			border = &colors->selected_border;
			bg     = &colors->selected_bg;
		}
		if (lasthover == c) { //mouse is hovering over this node
			border = &colors->mo_border;
			bg     = &colors->mo_bg;
			//fg     = &colors->mo_fg;
		}
		if (IsSelected(node) && lasthover == c) { //mouse is hovering over this selected node
			border = &colors->selected_mo_border;
			bg     = &colors->selected_mo_bg;
		}
		dp->NewFG(border);
		dp->NewBG(bg);
		flatpoint p;

		 //draw whole rect
		dp->drawRoundedRect(node->x, node->y, node->width, node->height,
							th/3, false, th/3, false, 2); 

		 //draw label
		dp->NewBG(fg);
		dp->textout(node->x+node->width/2-th/2, node->y, node->Name, -1, LAX_TOP|LAX_HCENTER);


		 //draw the properties (or not)
		p.set(node->x+node->width, node->y);
		if (node->collapsed) dp->drawthing(p.x-th/2, p.y+th/2, th/4,th/4, 1, THING_Triangle_Right);
		else {
			 //node is expanded, draw all the properties...
			dp->drawthing(p.x-th, p.y+th/2, th/4,th/4, 1, THING_Triangle_Down);

			double y=node->y+th*1.5;

			 //draw preview
			if (node->total_preview) {
				double ph = (node->width-th)*node->total_preview->h()/node->total_preview->w();
				dp->imageout(node->total_preview, node->x, node->y+th, node->width, ph);
				y+=ph;
			}

			 //draw ins and outs
			NodeProperty *prop;
			for (int c2=0; c2<node->properties.n; c2++) {
				prop = node->properties.e[c2];
				dp->NewBG(&prop->color);

				if (prop->is_input) {
					 dp->textout(node->x+th/2, y, prop->Name(),-1, LAX_LEFT|LAX_TOP); 
				} else {
					dp->textout(node->x+node->width-th/2, y, prop->Name(),-1, LAX_RIGHT|LAX_TOP);
				}

				if (!(prop->is_input && !prop->is_inputable))
					dp->drawellipse(prop->pos+flatpoint(node->x,node->y), th*slot_radius,th*slot_radius, 0,0, 2);
				y+=th;
			}
		}
	}

	if (hover_action==NODES_Cut_Connections) {
		dp->LineWidthScreen(1);
		dp->NewFG(&color_controls);
		dp->drawline(selection_rect.minx,selection_rect.miny, selection_rect.maxx,selection_rect.maxy);

	} else if (hover_action==NODES_Selection_Rect) {
		dp->LineWidthScreen(1);
		dp->NewFG(&color_controls);
		dp->drawrectangle(selection_rect.minx,selection_rect.miny, selection_rect.maxx-selection_rect.minx,selection_rect.maxy-selection_rect.miny, 0);

	} else if (hover_action==NODES_Drag_Input || hover_action==NODES_Drag_Output) {
		NodeBase *node = nodes->nodes.e[lasthover];
		NodeConnection *connection = node->properties.e[lasthoverprop]->connections.e[lastconnection];

		flatpoint p1,p2;
		flatpoint last = nodes->m.transformPointInverse(lastpos);
		if (connection->fromprop) p1=flatpoint(connection->from->x,connection->from->y)+connection->fromprop->pos; else p1=last;
		if (connection->toprop)   p2=flatpoint(connection->to->x,  connection->to->y)  +connection->toprop->pos;   else p2=last;

		dp->NewFG(&color_controls);
		dp->moveto(p1);
		dp->curveto(p1+flatpoint((p2.x-p1.x)/3, 0),
					p2-flatpoint((p2.x-p1.x)/3, 0),
					p2);
		dp->stroke(0);

	}

	dp->PopAxes();

	return 0;
}

/*! Return the node under x,y, or -1 if no node there.
 */
int NodeInterface::scan(int x, int y, int *overproperty) 
{
	if (!nodes) return -1;

	flatpoint p=nodes->m.transformPointInverse(flatpoint(x,y));
	if (overproperty) *overproperty=-1;

	NodeBase *node;
	double th = font->textheight();

	for (int c=nodes->nodes.n-1; c>=0; c--) {
		node = nodes->nodes.e[c];

		if (p.x >= node->x-th/2 &&  p.x <= node->x+node->width+th/2 &&  p.y >= node->y &&  p.y <= node->y+node->height) {

			NodeProperty *prop;
			for (int c2=0; c2<node->properties.n; c2++) {
				prop = node->properties.e[c2];

				if (!(prop->is_input && !prop->is_inputable)) { //only if the input is not exclusively internal
				  if (p.y >= node->y+(1.5+c2)*th && p.y < node->y+(1.5+1+c2)*th) {
					if (p.x >= node->x+prop->pos.x-th/2 && p.x <= node->x+prop->pos.x+th/2) {
						if (overproperty) *overproperty = c2;
					}
				  }
				}
			}
			return c; 
		}
	}

	return -1;
}

//! Start a new freehand line.
int NodeInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{

	int action = NODES_None;
	int overproperty=-1; 
	int overnode = scan(x,y, &overproperty);

	if (count==2 && overnode>=0 && nodes && dynamic_cast<NodeGroup*>(nodes->nodes.e[overnode])) {
		PostMessage("Need to implement jump into group");
		needtodraw=1;
		return 0;
	}

	if (((state&LAX_STATE_MASK) == 0 || (state&ShiftMask)!=0) && overnode==-1) {
		 //shift drag adds, shift-control drag removes, plain drag replaces selection 
		action = NODES_Selection_Rect;
		selection_rect.minx=selection_rect.maxx=x;
		selection_rect.miny=selection_rect.maxy=y;
		needtodraw=1;

	} else if ((state&LAX_STATE_MASK) == ControlMask && overnode==-1) {
		action = NODES_Cut_Connections;
		selection_rect.minx=selection_rect.maxx=x;
		selection_rect.miny=selection_rect.maxy=y;
		needtodraw=1;

	} else if (overnode>=0 && overproperty==-1) {
		 //not clicking on a property, so add or remove this node to selection
		if ((state&LAX_STATE_MASK) == ShiftMask) {
			 //add to selection
			selected.pushnodup(nodes->nodes.e[overnode]);
			action = NODES_Move_Nodes;
			needtodraw=1;

		} else if ((state&LAX_STATE_MASK) == ControlMask) {
			selected.remove(selected.findindex(nodes->nodes.e[overnode]));
			needtodraw=1;

		} else {
			 //plain click, make this node the only one selected
			selected.flush();
			selected.push(nodes->nodes.e[overnode]);
			action = NODES_Move_Nodes;
			needtodraw=1;
		}

	} else if (overnode>=0 && overproperty>=0) {
		 //drag out a property to connect to another node
		NodeProperty *prop = nodes->nodes.e[overnode]->properties.e[overproperty];

		if (nodes->nodes.e[overnode]->properties.e[overproperty]->is_input) {
			action = NODES_Drag_Input;

			if (prop->connections.n) {
				// if dragging input that is already connected, then disconnect from current node,
				// and drag output at the other end
				action = NODES_Drag_Output;
				overnode = nodes->nodes.findindex(prop->connections.e[0]->from);
				overproperty = nodes->nodes.e[overnode]->properties.findindex(prop->connections.e[0]->fromprop);
				prop->connections.e[0]->to = NULL;
				prop->connections.e[0]->toprop = NULL;

				lastconnection = 0;
				lasthover      = overnode;
				lasthoverprop  = overproperty;

			} else {
				 //connection didn't exist, so install a half connection to current node
				prop->connections.push(new NodeConnection(NULL, nodes->nodes.e[overnode], NULL,prop), 0);
				nodes->connections.push(prop->connections.e[prop->connections.n-1], 1);
				lastconnection = prop->connections.n-1;
				lasthover      = overnode;
				lasthoverprop  = overproperty;
			}

		} else {
			 // if dragging output, create new connection
			action = NODES_Drag_Output;
			prop->connections.push(new NodeConnection(nodes->nodes.e[overnode],NULL, prop,NULL), 0);
			nodes->connections.push(prop->connections.e[prop->connections.n-1], 1);
			lastconnection = prop->connections.n-1;
			lasthoverprop  = overproperty;
			lasthover      = overnode;
		}

		if (action==NODES_Drag_Output) PostMessage(_("Drag output..."));
		else PostMessage(_("Drag input..."));
	}

	if (action != NODES_None) {
		buttondown.down(d->id, LEFTBUTTON, x,y, action);
		hover_action = action;
	}


	return 0; //return 0 for absorbing event, or 1 for ignoring
}

//! Finish a new freehand line by calling newData with it.
int NodeInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	int action=NODES_None;
	buttondown.up(d->id, LEFTBUTTON, &action);

	int overproperty=-1; 
	int overnode = scan(x,y, &overproperty);

	if (action == NODES_Cut_Connections) {
		 //cut any connections that cross the line between selection_rect.min to max
		// ***
		PostMessage("Need to implement Cut Connections!!!");
		needtodraw=1;

	} else if (action == NODES_Selection_Rect) {
		 //cut any connections that cross the line between selection_rect.min to max
		// ***
		PostMessage("Need to implement Selection Rect!!!");
		needtodraw=1;

	} else if (action == NODES_Drag_Input) {
		//check if hovering over the output of some other node
		// *** need to ensure there is no circular linking

	} else if (action == NODES_Drag_Output) {
		//check if hovering over the input of some other node
		// *** need to ensure there is no circular linking

		int remove=0;

		if (overnode>=0 && overproperty>=0) {
			NodeProperty *toprop = nodes->nodes.e[overnode]->properties.e[overproperty];
			if (toprop->is_input) {
				if (toprop->connections.n) {
					 //clobber any other connection going into the input. Only one input allowed.
					toprop->connections.flush();
				}
				*** connect lasthover.lasthoverprop.lastconnection to toprop
			} else {
				remove=1;
			}
		}

		if (remove) {
			*** remove the unconnected connection
		}
	}

	hover_action = NODES_None;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int NodeInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *mouse)
{
	if (!buttondown.any()) {
		// update any mouse over state
		// ...

		int newhoverprop=-1;
		int newhover = scan(x,y, &newhoverprop);
		lastpos.x=x; lastpos.y=y;

		if (newhover!=lasthover || newhoverprop!=lasthoverprop) {
			needtodraw=1;
			lasthoverprop = newhoverprop;
			lasthover = newhover;
			if (lasthover<0) PostMessage("");
			else {
				char scratch[200];
				sprintf(scratch, "%s.%d", nodes->nodes.e[lasthover]->Name, lasthoverprop);
				PostMessage(scratch);
			}
		}

		return 1;
	}

	int lx,ly, action;
	buttondown.move(mouse->id, x,y, &lx,&ly);
	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &action);


	if (action == NODES_Cut_Connections) {
		 //control mouse drag over non-nodes breaks any connections
		 //update so cut line is selection_rect min to max
		selection_rect.maxx=x;
		selection_rect.maxy=y;
		needtodraw=1;
		return 0;

	} else if (action == NODES_Selection_Rect) {	
		 //drag out selection area for selecting or deselecting
		selection_rect.maxx=x;
		selection_rect.maxy=y;
		needtodraw=1;
		return 0;

	} else if (action == NODES_Move_Nodes) {	
		if (selected.n && nodes) {
			flatpoint d=nodes->m.transformPointInverse(flatpoint(x,y)) - nodes->m.transformPointInverse(flatpoint(lx,ly));

			for (int c=0; c<selected.n; c++) {
				selected.e[c]->x += d.x;
				selected.e[c]->y += d.y;
			}
		}
		needtodraw=1;
		return 0;

	} else if (action == NODES_Drag_Input) {
		lastpos.x=x; lastpos.y=y;
		needtodraw=1;
		return 0;

	} else if (action == NODES_Drag_Output) {
		lastpos.x=x; lastpos.y=y;
		needtodraw=1;
		return 0;
	}


	//needtodraw=1;
	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}

int NodeInterface::WheelUp(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{ // ***
	return 1; //wheel up ignored
}

int NodeInterface::WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{ // ***
	return 1; //wheel down ignored
}


int NodeInterface::send()
{
//	if (owner) {
//		RefCountedEventData *data=new RefCountedEventData(paths);
//		app->SendMessage(data,owner->object_id,"NodeInterface", object_id);
//
//	} else {
//		if (viewport) viewport->NewData(paths,NULL);
//	}

	return 0;
}

int NodeInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask|AltMask|MetaMask)) {
		//deal with various modified keys...
	}

	if (ch==LAX_Esc) { //the various possible keys beyond normal ascii printable chars are defined in lax/laxdefs.h
		if (selected.n==0) return 1;
		selected.flush();
		needtodraw=1;
		return 0;
	}

	 //default shortcut processing 
	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch, state&LAX_STATE_MASK, 0);
	if (action>=0) {
		return PerformAction(action);
	}


	return 1; //key not dealt with, propagate to next interface
}

int NodeInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	return 1; //key not dealt with
}

Laxkit::ShortcutHandler *NodeInterface::GetShortcuts()
{
	if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;

    //virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

    sc=new ShortcutHandler(whattype());

    //sc->Add([id number],  [key], [mod mask], [mode], [action string id], [description], [icon], [assignable]);
    sc->Add(NODES_Group_Nodes,   'g',ControlMask,0, "GroupNodes"   , _("Group Nodes"  ),NULL,0);
    sc->Add(NODES_Ungroup_Nodes, 'g',ShiftMask|ControlMask,0, "UngroupNodes" , _("Ungroup Nodes"),NULL,0);
    sc->Add(NODES_Add_Node,      'A',ShiftMask,  0, "AddNode"      , _("Add Node"     ),NULL,0);

    manager->AddArea(whattype(),sc);
    return sc;

}

int NodeInterface::PerformAction(int action)
{
	if (action==NODES_Group_Nodes) {
		//***

	} else if (action==NODES_Ungroup_Nodes) {
		//***

	} else if (action==NODES_Add_Node) {
		 //Pop up menu to select new node..
		//int mx=-1,my=-1;
		//int status=mouseposition(0, curwindow, &mx, &my, NULL, NULL, NULL);
		//if (mx<0 || mx>curwindow->win_w || my<0 || my>curwindow->win_h) {
		//	mx=curwindow->win_w/2;
		//	my=curwindow->win_h/2;
		//}

		MenuInfo *menu=new MenuInfo;
		ObjectFactoryNode *type;
		for (int c=0; c<node_factory->types.n; c++) {
			type=node_factory->types.e[c];
			menu->AddItem(type->name, c);
		}

       PopupMenu *popup=new PopupMenu(NULL,_("Add node..."), 0,
                        0,0,0,0, 1,
                        object_id,"addnode",
                        0, //mouse to position near?
                        menu,1, NULL,
                        MENUSEL_LEFT|MENUSEL_CHECK_ON_LEFT|MENUSEL_DESTROY_ON_LEAVE);
        popup->pad=5;
        popup->WrapToMouse(0);
        app->rundialog(popup);

		return 0;
	}

	return 1;
}

} // namespace Laidout

