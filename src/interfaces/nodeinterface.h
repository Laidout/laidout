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
#include "../filetypes/objectio.h"


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

class NodeGroup;
class NodeBase;
class NodeProperty;


//---------------------------- NodeConnection ------------------------------------
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
	virtual void RemoveConnection(int which=3);
};

//typedef int (*NodePropertyValidFunc)(NodeProperty *prop, const char **error_message);


//---------------------------- NodeProperty ------------------------------------
class NodeProperty
{
  public:
	enum PropertyTypes {
		PROP_Unknown,
		PROP_Input,
		PROP_Output,
		//PROP_Through,
		PROP_Block, //no in or out, but still maybe settable
		PROP_Preview,
		PROP_Button,
		PROP_Exec_In,
		PROP_Exec_Out,
		PROP_Exec_Through,
		PROP_MAX
	};

	char *name;
	char *label;
	char *tooltip;
	std::time_t modtime;

	NodeBase *owner;
	NodeProperty *frompropproxy, *topropproxy;
	Value *data;
	int *datatypes; //optional 0 terminated list of acceptible VALUE_* types
	int custom_info;

	PropertyTypes type;
	//bool is_input; //or output
	bool is_linkable; //default true for something that allows links in
	bool is_editable;
	bool hidden;

	double x,y,width,height;
	Laxkit::ScreenColor color;
	Laxkit::PtrStack<NodeConnection> connections; //input just has one

	flatpoint pos; //clickable spot relative to parent NodeBase origin

	NodeProperty();
	NodeProperty(PropertyTypes input, bool linkable, const char *nname, Value *ndata, int absorb_count,
					const char *nlabel=NULL, const char *ntip=NULL, int info=0, bool editable=true);
	virtual ~NodeProperty();
	virtual LaxInterfaces::anInterface *PropInterface(LaxInterfaces::anInterface *interface);
	virtual const char *Name()  { return name; }
	virtual const char *Label() { return label ? label : name; }
	virtual int IsConnected();
	virtual int IsInput()  { return type==PROP_Input;  }
	virtual int IsOutput() { return type==PROP_Output; }
	virtual int IsBlock()  { return type==PROP_Block; }
	virtual int IsEditable() { return is_editable && !(IsInput() && IsConnected()); }
	virtual int IsExecution() { return type==PROP_Exec_In || type==PROP_Exec_Out || type==PROP_Exec_Through; }
	virtual int IsHidden() { return hidden; }
	virtual int Hide();
	virtual int Show();
	virtual int AllowInput();
	virtual int AllowOutput();
	virtual bool AllowType(Value *ndata);
	virtual NodeBase *GetConnection(int connection_index, int *prop_index_ret);
	virtual Value *GetData();
	virtual NodeBase *GetDataOwner();
	virtual int SetData(Value *newdata, bool absorb);
};

//---------------------------- NodeColors ------------------------------------
class NodeColors : public Laxkit::anObject
{
  public:
	anObject *owner;

	Laxkit::LaxFont *font;
	unsigned int state; //normal | selected | mouseOver
	double slot_radius; //as fraction of text size

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

	Laxkit::ScreenColor fg_frame;
	Laxkit::ScreenColor bg_frame;

	Laxkit::ScreenColor fg_menu;
	Laxkit::ScreenColor bg_menu;

	Laxkit::ScreenColor selected_border;
	Laxkit::ScreenColor selected_bg;

	double mo_diff;


	NodeColors *next; //one node per state

	NodeColors();
	virtual ~NodeColors();
	virtual const char *whattype() { return "NodeColors"; }
	virtual int Font(Laxkit::LaxFont *newfont, bool absorb_count);
};


//---------------------------- NodeFrame ------------------------------------
class NodeFrame : public Laxkit::anObject,
				 public Laxkit::DoubleRectangle
{
  public:
	//ScreenColor bg, fg;

	NodeGroup *owner;
	char *label;
	char *comment;
	Laxkit::RefPtrStack<NodeBase> nodes; //nodes wrapped by this frame

	NodeFrame(NodeGroup *nowner, const char *nlabel=NULL, const char *ncomment=NULL);
	virtual ~NodeFrame();

	virtual const char *Label() { return label; }
	virtual const char *Label(const char *nlabel);
	virtual const char *Comment() { return comment; }
	virtual const char *Comment(const char *ncomment);
	virtual int AddNode(NodeBase *node);
	virtual int RemoveNode(NodeBase *node);
	virtual void Wrap(double gap=-1);
};

//---------------------------- NodeBase ------------------------------------
class NodeBase : public Laxkit::anObject,
				 public Laxkit::DoubleRectangle,
				 public Laxkit::Undoable
{
  public:
	 //state
	char *Name; //displayed name (see Label())
	char *type; //non translated type, like "Value", or "Math"
	ObjectDef *def; //optional

	int collapsed;
	double fullwidth; //uncollapsed

	bool deletable;

	bool show_preview;
	Laxkit::LaxImage *total_preview;
	double preview_area_height;

	Laxkit::PtrStack<NodeProperty> properties; //includes inputs and outputs
	std::time_t modtime; //time of last update

	NodeFrame *frame;
	NodeColors *colors;

	NodeBase();
	virtual ~NodeBase();
	virtual const char *whattype() { return "Nodes"; }
	virtual int InstallColors(NodeColors *newcolors, bool absorb_count);
	virtual const char *Label() { return Name; }
	virtual const char *Label(const char *nlabel);
	virtual const char *Description() { return NULL; }
	virtual const char *ScriptName() { return object_idstr; }
	virtual const char *Type() { return type; }
	virtual ObjectDef *GetDef() { return def; }
	virtual void InstallDef(ObjectDef *def, bool absorb_count);
	virtual LaxInterfaces::anInterface *PropInterface(LaxInterfaces::anInterface *interface);

	virtual int Undo(UndoData *data);
	virtual int Redo(UndoData *data);

	virtual int Update();
	virtual int UpdatePreview();
	virtual int GetStatus();
	virtual int UsesPreview() { return total_preview!=NULL && show_preview; }
	virtual int Wrap();
	virtual int WrapFull(bool keep_current_width);
	virtual int WrapCollapsed();
	virtual void UpdateLinkPositions();
	virtual void UpdateLayout();
	virtual int Collapse(int state); //-1 toggle, 0 open, 1 full collapsed, 2 collapsed to preview
	virtual NodeBase *Duplicate();

	virtual int IsConnected(int propindex); //0=no, -1=prop is connected input, 1=connected output
	virtual int HasConnection(NodeProperty *prop, int *connection_ret);
	virtual int Disconnected(NodeConnection *connection, int to_side);
	virtual int Connected(NodeConnection *connection);

	virtual int AddProperty(NodeProperty *newproperty, int where=-1);
	virtual NodeProperty *FindProperty(const char *prop);
	virtual int SetProperty(const char *prop, Value *value, bool absorb);
	virtual int SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att);
	virtual int NumInputs(bool connected);
	virtual int NumOutputs(bool connected);
	
	virtual int AssignFrame(NodeFrame *nframe);
	//virtual NodeColors *GetColors(); //return either this->colors, or the first defined one in owners

	virtual void       dump_out(FILE *f, int indent, int what, LaxFiles::DumpContext *context);
    virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att, int what, LaxFiles::DumpContext *context);
    virtual void dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context);
};


//---------------------------- NodeGroup ------------------------------------

class NodeGroup : public NodeBase, public LaxFiles::DumpUtility
{
	static Laxkit::SingletonKeeper factorykeeper;

  protected:
	virtual int CheckForward(NodeBase *node, NodeConnection *connection);
	virtual int CheckBackward(NodeBase *node, NodeConnection *connection);

  public:
	static Laxkit::RefPtrStack<ObjectIO> loaders;
	static int InstallLoader(ObjectIO *loader, int absorb_count);
	static int RemoveLoader(ObjectIO *loader);
	static Laxkit::ObjectFactory *NodeFactory(bool create=true);
	static void SetNodeFactory(Laxkit::ObjectFactory *newnodefactory);

	Laxkit::ScreenColor background;

	NodeBase *output, *input;
	Laxkit::Affine m;
	Laxkit::RefPtrStack<NodeBase> nodes; //nodes wrapped into this group
	Laxkit::PtrStack<NodeConnection> connections;
	Laxkit::RefPtrStack<NodeFrame> frames;

	NodeGroup();
	virtual ~NodeGroup();
	virtual const char *whattype() { return "NodeGroup"; }
	virtual int InstallColors(NodeColors *newcolors, bool absorb_count);
	virtual int DesignateOutput(NodeBase *noutput);
	virtual int DesignateInput(NodeBase *ninput);
	virtual NodeBase *FindNode(const char *name);
	virtual NodeBase *GetNode(int index);
	virtual NodeBase *NewNode(const char *type);
	virtual int AddNode(NodeBase *node);
	virtual NodeFrame *GetFrame(int index);
	virtual int NoOverlap(NodeBase *which, double gap);
	virtual void BoundingBox(DoubleBBox &box);

	virtual void InitializeBlank();
	virtual NodeProperty *AddGroupInput(const char *pname, const char *plabel, const char *ptip);
	virtual NodeProperty *AddGroupOutput(const char *pname, const char *plabel, const char *ptip);

	virtual int DeleteNodes(Laxkit::RefPtrStack<NodeBase> &selected);
	virtual NodeGroup *GroupNodes(Laxkit::RefPtrStack<NodeBase> &selected);
	virtual int UngroupNodes(Laxkit::RefPtrStack<NodeBase> &selected, bool update_selected);
	virtual NodeConnection *Connect(NodeProperty *from, NodeProperty *to);
	virtual int Disconnect(NodeConnection *connection);

	virtual void       dump_out(FILE *f, int indent, int what, LaxFiles::DumpContext *context);
    virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att, int what, LaxFiles::DumpContext *context);
    virtual void dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context);

};

int SetupDefaultNodeTypes(Laxkit::ObjectFactory *factory);
typedef NodeGroup Nodes;


//---------------------------- NodeInterface ------------------------------------

//these get tracked in lasthoverslot.
//They need to be negative to not conflict with slot index numbers.
enum NodeHover {
	NHOVER_SCAN_START = -10000, //this needs to be so negative as to make NODES_HOVER_MAX also be negative
	NHOVER_None          ,
	NHOVER_Label         ,
	NHOVER_LeftEdge      ,
	NHOVER_RightEdge     ,
	NHOVER_Collapse      ,
	NHOVER_TogglePreview ,
	NHOVER_PreviewResize ,
	NHOVER_Frame         ,
	NHOVER_Frame_Label   ,
	NHOVER_Frame_Comment ,
	NODES_VP_Top         ,
	NODES_VP_Top_Left    ,
	NODES_VP_Left        ,
	NODES_VP_Bottom_Left ,
	NODES_VP_Bottom      ,
	NODES_VP_Bottom_Right,
	NODES_VP_Right       ,
	NODES_VP_Top_Right   ,
	NODES_VP_Move        ,
	NODES_VP_Maximize    ,
	NODES_VP_Close       ,
	NODES_Navigator      ,
	NODES_HOVER_MAX
};

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
	NODES_Drag_Property,
	NODES_Resize_Left,
	NODES_Resize_Right,
	NODES_Resize_Top,
	NODES_Resize_Bottom,
	NODES_Resize_Top_Left,
	NODES_Resize_Top_Right,
	NODES_Resize_Bottom_Left,
	NODES_Resize_Bottom_Right,
	NODES_Resize_Preview,
	NODES_Center,
	NODES_Center_Selected,
	NODES_No_Overlap,
	NODES_Arrange_Grid,
	NODES_Arrange_Row,
	NODES_Arrange_Column,
	NODES_Toggle_Collapse,
	NODES_Save_With_Loader,
	NODES_Load_With_Loader,
	NODES_Clear,

	NODES_Duplicate,
	NODES_Group_Nodes,
	NODES_Ungroup_Nodes,
	NODES_Edit_Group,
	NODES_Leave_Group,
	NODES_Frame_Nodes,
	NODES_Unframe_Nodes,
	NODES_Add_Node,
	NODES_Delete_Nodes,
	NODES_Save_Nodes,
	NODES_Load_Nodes,
	NODES_Show_Previews,
	NODES_Hide_Previews,

	NODES_MAX
};

class NodeViewArea : public DoubleRectangle
{
  public:
	NodeGroup *group;
	flatpoint anchor;
	bool is_anchored;
	//bool is_minimized;
	Laxkit::Affine m;

	NodeViewArea();
	virtual ~NodeViewArea();
};

class NodeInterface : public LaxInterfaces::anInterface
{
  protected:
	int showdecs;

	Laxkit::ObjectFactory *node_factory; //usually, convenience cast to return of NodeBase::NodeFactory()

	Nodes *nodes;
	Laxkit::MenuInfo *node_menu;
	virtual void RebuildNodeMenu();

	Laxkit::RefPtrStack<NodeGroup> grouptree; //stack of nested groups we are working on
	Laxkit::RefPtrStack<NodeBase> selected;
	Laxkit::DoubleBBox selection_rect;
	int hover_action;
	int lasthover, lasthoverslot, lasthoverprop, lastconnection;
	flatpoint lastpos;
	int lastmenuindex;


	//Laxkit::Affine transform; //from nodes to screen coords

	 //style state:
	char *lastsave;
	Laxkit::LaxFont *font;
	double defaultpreviewsize;

	Laxkit::ScreenColor color_connection;
	Laxkit::ScreenColor color_connection_selected;
	Laxkit::ScreenColor color_node_selected;
	Laxkit::ScreenColor color_node_default;
	Laxkit::ScreenColor color_controls;
	Laxkit::ScreenColor color_background;
	Laxkit::ScreenColor color_grid;
	double draw_grid; //pixel spacing

	Laxkit::DoubleBBox viewport_bounds;
	double vp_dragpad;

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
	virtual int RBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int WheelUp  (int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);

	virtual void DrawConnection(NodeConnection *connection);
	virtual void DrawProperty(NodeBase *node, NodeProperty *prop, double y, int hoverprop, int hoverslot);
	virtual int scan(int x, int y, int *overpropslot, int *overproperty);
	virtual int IsSelected(NodeBase *node);

	virtual int ToggleCollapsed();
	virtual int EnterGroup(NodeGroup *group=NULL);
	virtual int LeaveGroup();
	virtual int SaveNodes(const char *file);
	virtual int LoadNodes(const char *file, bool append);
};


//--------------------------NodeLoader --------------------------------

class NodeLoader
{
  public:
	NodeLoader();
	virtual ~NodeLoader();
	virtual int CanLoad(const char *file);
	virtual int LoadNodes(const char *file, NodeBase *parent);
};


//--------------------------NodeUndo --------------------------------
class NodeUndo : public Laxkit::UndoData
{
  public:
    enum NodeUndoTypes {
        Moved,
        Scaled,
		Added,
		Deleted,
		Grouped,
		Ungrouped,
		Resized,
		Connected,
		Disconnected,
		Framed,
		Unframed,
		PropertyChange,
        MAX
    };

    int type;
	double *m;
	anObject *stuff;

    NodeUndo(int ntype, int nisauto, double *mm, anObject *nstuff);
	virtual ~NodeUndo();
    virtual const char *Description();
};


} // namespace Laidout

#endif

