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
//    Copyright (C) 2017 by Tom Lechner
//
#ifndef _LAX_NODEINTERFACE_H
#define _LAX_NODEINTERFACE_H

#include <ctime>

#include <lax/objectfactory.h>
#include <lax/interfaces/aninterface.h>
#include <lax/rectangles.h>
#include <lax/refptrstack.h>
#include <lax/singletonkeeper.h>

#include "../calculator/values.h"


using namespace LaxFiles;
using namespace Laxkit;

namespace Laidout { 

//default node types:
//  string
//  int
//  double
//  boolean
//  color
//  file
//  date
//  image


//---------------------------- Nodes ------------------------------------

class NodeBase;
class NodeProperty;

class NodeConnection
{
  public:
	//Laxkit::ScreenColor color;

	Laxkit::NumStack<flatpoint> path; //points between (and including) start and end points for custom winding,
									  //though start and end are taken from the connected node properties

	NodeBase     *from,     *to;
	NodeProperty *fromprop, *toprop;

	NodeConnection();
	NodeConnection(NodeBase *nfrom, NodeBase *nto, NodeProperty *nfromprop, NodeProperty *ntoprop);
	virtual ~NodeConnection();
};

class NodeProperty
{
  public:
	char *name;
	std::time_t modtime;

	NodeBase *owner;
	Value *data;
	bool is_input; //or output
	bool is_inputable; //default true for something that allows links in

	double x,y,width,height;
	Laxkit::ScreenColor color;
	Laxkit::PtrStack<NodeConnection> connections; //input just has one

	flatpoint pos; //clickable spot relative to parent NodeBase origin

	NodeProperty();
	NodeProperty(bool input, bool inputable, const char *nname, Value *ndata, int absorb_count);
	virtual ~NodeProperty();
	virtual LaxInterfaces::anInterface *PropInterface();
	virtual const char *Name() { return name; }
	virtual int IsConnected();
	virtual int IsInput() { return is_input; }
	virtual int AllowInput();
	virtual int AllowOutput();
	virtual NodeBase *GetConnection(int connection_index, int *prop_index_ret);
	virtual Value *GetData();
	virtual int SetData(Value *newdata, bool absorb);
};

class NodeColors : public Laxkit::anObject
{
  public:
	anObject *owner;

	Laxkit::LaxFont *font;
	unsigned int state; //normal | selected | mouseOver

	Laxkit::ScreenColor default_property;
	Laxkit::ScreenColor connection;
	Laxkit::ScreenColor sel_connection;

	Laxkit::ScreenColor label_fg;
	Laxkit::ScreenColor label_bg;
	Laxkit::ScreenColor fg;
	Laxkit::ScreenColor bg;
	Laxkit::ScreenColor text;
	Laxkit::ScreenColor border;
	Laxkit::ScreenColor error_border;

	Laxkit::ScreenColor fg_edit;
	Laxkit::ScreenColor bg_edit;

	Laxkit::ScreenColor fg_menu;
	Laxkit::ScreenColor bg_menu;

	Laxkit::ScreenColor selected_border;
	Laxkit::ScreenColor selected_bg;

	double mo_diff;


	NodeColors *next; //one node per state

	NodeColors();
	virtual ~NodeColors();
	virtual int Font(Laxkit::LaxFont *newfont, bool absorb_count);
};

class NodeBase : public Laxkit::anObject, public Laxkit::DoubleRectangle
{
  public:
	 //state
	char *Name; //displayed name
	char *type; //non translated type, like "Value", or "Math"
	ObjectDef *def;

	bool collapsed;
	bool deletable;
	Laxkit::LaxImage *total_preview;

	Laxkit::PtrStack<NodeProperty> properties; //includes inputs and outputs

	NodeColors *colors;

	NodeBase();
	virtual ~NodeBase();
	virtual const char *whattype() { return "Nodes"; }
	virtual int InstallColors(NodeColors *newcolors, bool absorb_count);
	virtual const char *Label() { return object_idstr; }
	virtual const char *Type() { return type; }
	virtual ObjectDef *GetDef() { return def; }

	virtual int Update();
	virtual int GetStatus();
	virtual int Wrap();
	virtual int Collapse(int state); //-1 toggle, 0 open, 1 collapsed

	virtual int IsConnected(int propindex); //0=no, -1=prop is connected input, 1=connected output
	virtual int HasConnection(NodeProperty *prop, int *connection_ret);

	virtual NodeProperty *FindProperty(const char *prop);
	virtual int SetProperty(const char *prop, Value *value, bool absorb);
	virtual int SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att);
};


/*! \class NodeGroup
 * Class to hold a collection of nodes.
 */

class NodeGroup : public NodeBase, public LaxFiles::DumpUtility
{
	static Laxkit::SingletonKeeper nodekeeper;

  protected:
	virtual int CheckForward(NodeBase *node, NodeConnection *connection);
	virtual int CheckBackward(NodeBase *node, NodeConnection *connection);

  public:
	static Laxkit::ObjectFactory *NodeFactory(bool create=true);
	static void SetNodeFactory(Laxkit::ObjectFactory *newnodefactory);
	
	Laxkit::ScreenColor background;

	NodeBase *output;
	Laxkit::Affine m;
	Laxkit::RefPtrStack<NodeBase> nodes; //nodes wrapped into this group
	Laxkit::PtrStack<NodeConnection> connections;

	NodeGroup();
	virtual ~NodeGroup();
	virtual int DesignateOutput(NodeBase *noutput);
	virtual NodeBase *FindNode(const char *name);
	NodeBase *NewNode(const char *type);

	virtual int DeleteNodes(Laxkit::RefPtrStack<NodeBase> &selected);
	virtual int Connect(NodeProperty *from, NodeProperty *to, NodeConnection *usethis);

	virtual void       dump_out(FILE *f, int indent, int what, LaxFiles::DumpContext *context);
    virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att, int what, LaxFiles::DumpContext *context);
    virtual void dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context);

};

int SetupDefaultNodeTypes(Laxkit::ObjectFactory *factory);
typedef NodeGroup Nodes;


//---------------------------- NodeInterface ------------------------------------

enum NodeInterfaceActions {
	NODES_None=0,
	NODES_Normal,
	NODES_Selection_Rect,
	NODES_Drag_Output,
	NODES_Drag_Input,
	NODES_Move_Nodes,
	NODES_Move_Or_Select,
	NODES_Cut_Connections,
	NODES_Property,
	NODES_Resize_Left,
	NODES_Resize_Right,
	NODES_Resize_Top,
	NODES_Resize_Bottom,
	NODES_Resize_Top_Left,
	NODES_Resize_Top_Right,
	NODES_Resize_Bottom_Left,
	NODES_Resize_Bottom_Right,

	NODES_Group_Nodes,
	NODES_Ungroup_Nodes,
	NODES_Add_Node,
	NODES_Delete_Nodes,
	NODES_Save_Nodes,
	NODES_Load_Nodes,
	NODES_MAX
};

class NodeInterface : public LaxInterfaces::anInterface
{
  protected:
	int showdecs;

	Laxkit::ObjectFactory *node_factory;

	Nodes *nodes;

	Laxkit::PtrStack<NodeBase> grouptree;
	Laxkit::RefPtrStack<NodeBase> selected;
	Laxkit::DoubleBBox selection_rect;
	int hover_action;
	int lasthover, lasthoverslot, lasthoverprop, lastconnection;
	flatpoint lastpos;


	double slot_radius; //as fraction of text size
	Laxkit::LaxFont *font;

	Laxkit::Affine transform; //from nodes to screen coords

	Laxkit::ScreenColor color_connection;
	Laxkit::ScreenColor color_connection_selected;
	Laxkit::ScreenColor color_node_selected;
	Laxkit::ScreenColor color_node_default;
	Laxkit::ScreenColor color_controls;
	Laxkit::ScreenColor color_background;
	Laxkit::ScreenColor color_grid;
	double draw_grid; //pixel spacing

	Laxkit::ShortcutHandler *sc;

	virtual int send();

  public:
	unsigned int node_interface_style;

	NodeInterface(LaxInterfaces::anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~NodeInterface();
	virtual LaxInterfaces::anInterface *duplicate(LaxInterfaces::anInterface *dup);
	virtual const char *IconId() { return "Node"; }
	const char *Name();
	const char *whattype() { return "NodeInterface"; }
	const char *whatdatatype();
	Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);

	virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual int Refresh();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int MBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int MBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int WheelUp  (int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);

	virtual void DrawConnection(NodeConnection *connection);
	virtual void DrawProperty(NodeBase *node, NodeProperty *prop, double y);
	virtual int scan(int x, int y, int *overpropslot, int *overproperty);
	virtual int IsSelected(NodeBase *node);
};

} // namespace Laidout

#endif

