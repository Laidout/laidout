//
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2017-2018 by Tom Lechner
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


namespace Laidout { 


//---------------------------- Nodes ------------------------------------

class NodeGroup;
class NodeBase;
class NodeProperty;
class NodeColors;
class NodeInterface;


//---------------------------- NodeConnection ------------------------------------
class NodeConnection : public Laxkit::RefCounted
{
  public:
	//Laxkit::ScreenColor color;

	Laxkit::NumStack<Laxkit::flatpoint> path; //points between (and including) start and end points for custom winding,
									  //though start and end are taken from the connected node properties

	NodeBase     *from,     *to;
	NodeProperty *fromprop, *toprop;

	NodeConnection();
	NodeConnection(NodeBase *nfrom, NodeBase *nto, NodeProperty *nfromprop, NodeProperty *ntoprop);
	virtual ~NodeConnection();
	virtual void SetFrom(NodeBase *nfrom, NodeProperty *nfprop) { from = nfrom; fromprop = nfprop; }
	virtual void SetTo  (NodeBase *nto,   NodeProperty *nfto  ) { to   = nto;   toprop   = nfto;   }
	virtual int IsExec();
};

//typedef int (*NodePropertyValidFunc)(NodeProperty *prop, const char **error_message);


//---------------------------- NodeProperty ------------------------------------
class NodeProperty : public Laxkit::RefCounted
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
		PROP_New_In,
		PROP_New_Out,
		PROP_MAX
	};
	enum PropertyFlags {
		PROPF_New_In   = (1<<0),
		PROPF_List_In  = (1<<1),
		PROPF_New_Out  = (1<<2),
		PROPF_List_Out = (1<<3),
		PROPF_Label_From_Data = (1<<4),
		PROPF_Y_Resizeable = (1<<5),
		PROPF_Button_Grayed = (1<<6),
		PROPF_MAX
	};

	char *name;
	char *label;
	char *tooltip;
	std::clock_t modtime;

	NodeBase *owner;
	NodeProperty *frompropproxy, *topropproxy; //connects to outer group ins and outs
	NodeProperty *mute_to;
	Value *data;
	bool data_is_linked;
	int *datatypes; //optional 0 terminated list of acceptible VALUE_* types
	int custom_info;

	PropertyTypes type;
	//bool is_input; //or output
	bool is_linkable; //default true for something that allows links in
	bool is_editable;
	bool hidden;
	unsigned int flags; //see PropertyFlags

	double x,y,width,height;
	Laxkit::ScreenColor color;
	Laxkit::RefPtrStack<NodeConnection> connections; //input just has one

	Laxkit::flatpoint pos; //clickable spot relative to parent NodeBase origin

	NodeProperty();
	NodeProperty(PropertyTypes input, bool linkable, const char *nname, Value *ndata, int absorb_count,
					const char *nlabel=NULL, const char *ntip=NULL, int info=0, bool editable=true);
	virtual ~NodeProperty();
	virtual const char *PropInterfaceName() { return NULL; }
	virtual LaxInterfaces::anInterface *PropInterface(LaxInterfaces::anInterface *interface, Laxkit::Displayer *dp);
	virtual bool HasInterface() { return false; }
	virtual void Draw(Laxkit::Displayer *dp, int hovered) {}
	virtual void SetExtents(NodeColors *colors);
	virtual const char *Name()  { return name; }
	virtual const char *Name(const char *nname);
	virtual const char *Label();
	virtual const char *Label(const char *nlabel);
	virtual const char *Tooltip() { return tooltip; }
	virtual const char *Tooltip(const char *nttip);
	virtual int AddConnection(NodeConnection *connection, int absorb);
	virtual int RemoveConnection(NodeConnection *connection);
	virtual int IsConnected();
	virtual int IsInput()  { return type==PROP_Input;  }
	virtual int IsOutput() { return type==PROP_Output; }
	virtual int IsBlock()  { return type==PROP_Block; }
	virtual int IsEditable() { return is_editable && !(IsInput() && IsConnected()); }
	virtual int IsExec() { return type==PROP_Exec_In || type==PROP_Exec_Out || type==PROP_Exec_Through; }
	virtual int IsExecOut() { return type==PROP_Exec_Out || type==PROP_Exec_Through; }
	virtual int IsExecIn()  { return type==PROP_Exec_In  || type==PROP_Exec_Through; }
	virtual int IsExecThrough()  { return type==PROP_Exec_Through; }
	virtual int IsNewInput() { return flags & PROPF_New_In;  }
	virtual int IsNewOutput() { return flags & PROPF_New_Out;  }
	virtual int IsHidden() { return hidden; }
	virtual void SetFlag(unsigned int which, bool on);
	virtual bool HasFlag(unsigned int which);
	virtual int Hide();
	virtual int Show();
	virtual int AllowInput();
	virtual int AllowOutput();
	virtual bool AllowType(Value *ndata);
	virtual NodeBase *GetConnection(int connection_index, int *prop_index_ret);
	virtual Value *GetData();
	virtual NodeBase *GetDataOwner();
	virtual int SetData(Value *newdata, bool absorb);
	virtual void Touch();
};


//-------------------------------------- NodeThread --------------------------

class NodeThread
{
  public:
	int thread_id;
	std::clock_t start_time;
	std::clock_t process_time;
	std::clock_t last_tick_end_time;
	int tick;
	ValueHash *data;
	NodeBase *next;
	NodeProperty *property; //property from preceding node
	Laxkit::RefPtrStack<NodeBase> scopes;

	NodeThread(NodeBase *next, NodeProperty *prop, ValueHash *payload, int absorb);
	virtual ~NodeThread();
	virtual int UpdateThread(NodeBase *node, NodeConnection *gonext);
};


//---------------------------- NodeColors ------------------------------------
class NodeColors : public Laxkit::anObject
{
  public:
	anObject *owner;

	Laxkit::LaxFont *font;
	unsigned int state; //normal | selected | mouseOver
	double slot_radius; //as fraction of text size
	double mo_diff; //amount to change color channel when mouse over
	double preview_dims; //default preview dimension in pixels
	double frame_label_size; //multiple of default font size

	Laxkit::ScreenColor default_property;
	Laxkit::ScreenColor connection;
	Laxkit::ScreenColor sel_connection;
	Laxkit::ScreenColor number;
	Laxkit::ScreenColor vector;
	Laxkit::ScreenColor color; //includes image
	Laxkit::ScreenColor exec;

	Laxkit::ScreenColor label_fg;
	Laxkit::ScreenColor label_bg;
	Laxkit::ScreenColor fg;
	Laxkit::ScreenColor bg;
	Laxkit::ScreenColor text;
	Laxkit::ScreenColor border;
	Laxkit::ScreenColor error_border;
	Laxkit::ScreenColor update_border;

	Laxkit::ScreenColor fg_edit;
	Laxkit::ScreenColor bg_edit;

	Laxkit::ScreenColor fg_frame;
	Laxkit::ScreenColor bg_frame;

	Laxkit::ScreenColor fg_menu;
	Laxkit::ScreenColor bg_menu;

	Laxkit::ScreenColor selected_border;
	Laxkit::ScreenColor selected_bg;


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
	Laxkit::Color *bg, *fg; //border, comment, label_bg;
	double label_size; //factor above default frame label size

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
	virtual int NumNodes() { return nodes.n; }
	virtual void Wrap(double gap=-1);

    virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att, int what, Laxkit::DumpContext *context);
    virtual void dump_in_atts(Laxkit::Attribute *att, int flag, Laxkit::DumpContext *context);
};


//---------------------------- NodeBase ------------------------------------
class NodeBase : virtual public Laxkit::anObject,
				 public Laxkit::DoubleRectangle,
				 public Laxkit::Undoable
{
  protected:
  	char *UniquePropName(const char *oldname);

  public:
	 //state
	char *Name; //displayed name (see Label())
	char *type; //non translated type, like "Value", or "Math"
	int special_type;
	ObjectDef *def; //optional
	char *error_message;
	int muted;
	bool manual_update; //flag to prevent excessive updating while loading

	int collapsed;
	double fullwidth; //uncollapsed
	double collapsedwidth;

	bool deletable;

	bool show_preview;
	Laxkit::LaxImage *total_preview;
	double psamplew, psampleh;
	double preview_area_height;

	Laxkit::PtrStack<NodeProperty> properties; //includes inputs and outputs
	std::clock_t modtime; //time of last update

	NodeFrame *frame;
	NodeColors *colors;

	NodeBase *resource_proxy; //when nodes are moved/sized/linked, relay these changes to the resource we are copied from

	NodeBase();
	virtual ~NodeBase();
	virtual const char *whattype() { return "Node"; }
	virtual int InstallColors(NodeColors *newcolors, bool absorb_count);
	virtual const char *Label() { return Name; }
	virtual const char *Label(const char *nlabel);
	virtual const char *Description() { return NULL; }
	virtual const char *ScriptName() { return object_idstr; } //not localized
	virtual const char *Type() { return type; } //not localized
	virtual const char *ErrorMessage() { return error_message; }
	virtual const char *Error(const char *error_msg);
	virtual void ClearError();
	virtual ObjectDef *GetDef() { return def; }
	virtual void InstallDef(ObjectDef *def, bool absorb_count);
	virtual LaxInterfaces::anInterface *GetInterface(LaxInterfaces::anInterface *interface);
	virtual int InterfaceEvent(NodeProperty *prop, LaxInterfaces::anInterface *interf, const Laxkit::EventData *data, const char *mes);

	virtual int Undo(Laxkit::UndoData *data);
	virtual int Redo(Laxkit::UndoData *data);

	virtual int Update();
	virtual int UpdateRecursively();
	virtual void MarkMustUpdate();
	virtual void PropagateUpdate();
	virtual int UpdatePreview();
	virtual void ManualUpdate(bool yes);
	virtual Value *PreviewFrom() { return nullptr; }
	virtual void PreviewSample(double w, double h, bool is_shift);
	virtual int GetStatus(); //0 ok, -1 bad ins, 1 just needs updating
	virtual void Touch();
	virtual clock_t MostRecentIn(int *index);
	virtual int UsesPreview() { return total_preview!=NULL && show_preview; }
	virtual int Wrap();
	virtual int WrapFull(bool keep_current_width);
	virtual int WrapCollapsed();
	virtual int Collapse(int state); //-1 toggle, 0 open, 1 full collapsed, 2 collapsed to preview
	virtual void UpdateLinkPositions();
	virtual void UpdateLayout();
	virtual NodeBase *Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks);
	virtual void ExecuteReset();
	virtual void ButtonClick(NodeProperty *prop, NodeInterface *controller) {}

	virtual NodeBase *Duplicate();
	virtual void DuplicateBase(NodeBase *from);
	virtual void DuplicateProperties(NodeBase *from);

	virtual int IsConnected(int propindex); //0=no, -1=prop is connected input, 1=connected output
	virtual int HasConnection(NodeProperty *prop, int *connection_ret);
	virtual int Disconnected(NodeConnection *connection, bool from_will_be_replaced, bool to_will_be_replaced);
	virtual int Connected(NodeConnection *connection);

	virtual int AddProperty(NodeProperty *newproperty, int where=-1);
	virtual NodeProperty *AddNewIn (int is_for_list, const char *nname, const char *nlabel, const char *ttip, int where=-1);
	virtual NodeProperty *AddNewOut(int is_for_list, const char *nname, const char *nlabel, const char *ttip, int where=-1);
	virtual int RemoveProperty(NodeProperty *prop);
	virtual NodeProperty *FindProperty(const char *prop, int *index_ret = nullptr);
	virtual int SetProperty(const char *prop, Value *value, bool absorb);
	virtual int SetPropertyFromAtt(const char *propname, Laxkit::Attribute *att, Laxkit::DumpContext *context);
	virtual int NumInputs(bool connected, bool include_execin);
	virtual int NumOutputs(bool connected);

	virtual int IsMuted();
    virtual int Mute(bool yes=true);


	virtual int AssignFrame(NodeFrame *nframe);
	//virtual NodeColors *GetColors(); //return either this->colors, or the first defined one in owners

	virtual void       dump_out(FILE *f, int indent, int what, Laxkit::DumpContext *context);
    virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att, int what, Laxkit::DumpContext *context);
    virtual void dump_in_atts(Laxkit::Attribute *att, int flag, Laxkit::DumpContext *context);
};


//---------------------------- NodeGroup ------------------------------------

class NodeGroup : public NodeBase, public Laxkit::DumpUtility
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
	static NodeGroup *Import(const char *format, const char *file, int file_is_string_data, NodeGroup *append_in_this);
	static int Export(NodeGroup *group, const char *format, const char *file);

	Laxkit::ScreenColor background;

	NodeBase *output, *input;
	Laxkit::Affine m;
	Laxkit::RefPtrStack<NodeBase> nodes; //nodes wrapped into this group
	Laxkit::RefPtrStack<NodeFrame> frames;

	NodeGroup();
	virtual ~NodeGroup();
	virtual const char *whattype() { return "NodeGroup"; }
	virtual int Undo(Laxkit::UndoData *data);
	virtual int Redo(Laxkit::UndoData *data);
	virtual int InstallColors(NodeColors *newcolors, bool absorb_count);
	virtual int DesignateOutput(NodeBase *noutput);
	virtual int DesignateInput(NodeBase *ninput);
	virtual NodeBase *FindNode(const char *name);
	virtual NodeBase *FindNodeByType(const char *type, int start_index);
	virtual NodeBase *GetNode(int index);
	virtual NodeBase *NewNode(const char *type);
	virtual int AddNode(NodeBase *node);
	virtual NodeFrame *GetFrame(int index);
	virtual int NoOverlap(NodeBase *which, double gap);
	virtual void BoundingBox(Laxkit::DoubleBBox &box);
	virtual NodeBase *Duplicate();
	virtual NodeBase *DuplicateGroup(NodeGroup *newgroup);
	virtual bool HasAlternateInterface() { return false; }
	virtual LaxInterfaces::anInterface *AlternateInterface() { return nullptr; }

	virtual void InitializeBlank();
	virtual NodeProperty *AddGroupInput(const char *pname, const char *plabel, const char *ptip);
	virtual NodeProperty *AddGroupOutput(const char *pname, const char *plabel, const char *ptip);

	virtual int DeleteNodes(Laxkit::RefPtrStack<NodeBase> &selected);
	virtual NodeGroup *GroupNodes(Laxkit::RefPtrStack<NodeBase> &selected);
	virtual int UngroupNodes(Laxkit::RefPtrStack<NodeBase> &selected, bool update_selected);
	virtual NodeConnection *Connect(NodeProperty *from, NodeProperty *to);
	virtual NodeConnection *Connect(NodeConnection *newcon, int absorb);
	virtual int Disconnect(NodeConnection *connection, bool from_will_be_replaced, bool to_will_be_replaced);
	virtual int ForceUpdates();
	virtual int UpdateAllRecursively();
	virtual void ManualUpdate(bool yes);
	virtual void SoftUpdate(int reason);

	virtual void       dump_out(FILE *f, int indent, int what, Laxkit::DumpContext *context);
    virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att, int what, Laxkit::DumpContext *context);
    virtual void dump_in_atts(Laxkit::Attribute *att, int flag, Laxkit::DumpContext *context);

};

int SetupDefaultNodeTypes(Laxkit::ObjectFactory *factory);
typedef NodeGroup Nodes;


//--------------------------- NodeExportContext ---------------------------------
class NodeExportContext : public Laxkit::anObject
{
  public:
	bool pipe;
	Laxkit::Attribute *passthrough;
	NodeGroup *group;
	NodeGroup *top;
	NodeColors *colors;
	Laxkit::RefPtrStack<NodeBase> *selection;

	NodeExportContext();
	NodeExportContext(Laxkit::RefPtrStack<NodeBase> *selected, Laxkit::Attribute *passth,
			NodeGroup *ngroup, NodeGroup *top, NodeColors *ncolors, bool npipe);
	virtual ~NodeExportContext();
	virtual void SetTop(NodeGroup *ntop);
};


//---------------------------- NodeViewArea ------------------------------------

class NodeViewArea : public Laxkit::DoubleRectangle
{
  public:
	NodeGroup *group;
	Laxkit::flatpoint anchor;
	bool is_anchored;
	//bool is_minimized;
	Laxkit::Affine m;

	NodeViewArea();
	virtual ~NodeViewArea();
};

//---------------------------- NodeInterface ------------------------------------

//these get tracked in lasthoverslot.
//They need to be negative to not conflict with slot index numbers.
enum NodeHover {
	NODES_SCAN_START = -10000, //this needs to be so negative as to make NODES_HOVER_MAX also be negative
	NODES_PropResize    ,
	NODES_Label         ,
	NODES_LeftEdge      ,
	NODES_RightEdge     ,
	NODES_Collapse      ,
	NODES_TogglePreview ,
	NODES_PreviewResize ,
	NODES_Frame         ,
	NODES_Connection    ,
	NODES_Frame_Label   ,
	NODES_Frame_Comment ,
	NODES_Frame_Label_Size,
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
	NODES_Jump_Back      ,
	NODES_Jump_Forward   ,
	NODES_Jump_Nearest   ,
	NODES_Thread_Controls,
	NODES_Thread_Next    ,
	NODES_Thread_Run     ,
	NODES_Thread_Reset   ,
	NODES_Thread_Break   ,
	NODES_Reroute        , //a special_type
	NODES_Debug          , //a special_type
	NODES_Global_Var     , //a special_type
	NODES_HOVER_MAX
};

enum NodeInterfaceActions {
	NODES_None=0,
	NODES_Normal,
	NODES_Selection_Rect,
	NODES_Drag_Output,
	NODES_Drag_Input,
	NODES_Drag_Exec_In,
	NODES_Drag_Exec_Out,
	NODES_Move_Nodes,
	NODES_Move_Or_Select,
	NODES_Cut_Connections,
	NODES_Add_Reroute,
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
	NODES_Resize_Property,
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
	NODES_Property_Interface,
	NODES_New_Nodes,

	NODES_Duplicate,
	NODES_Group_Nodes,
	NODES_Ungroup_Nodes,
	NODES_Edit_Group,
	NODES_Leave_Group,
	NODES_Frame_Nodes,
	NODES_Unframe_Nodes,
	NODES_Toggle_Frame,
	NODES_Move_Frame,
	NODES_Add_Node,
	NODES_Delete_Nodes,
	NODES_Save_Nodes,
	NODES_Load_Nodes,
	NODES_Show_Previews,
	NODES_Hide_Previews,
	NODES_Find,
	NODES_Find_Next,
	NODES_Force_Updates,
	NODES_Edit_With_Tool,
	
	NODES_MAX
};

class NodeInterface : public LaxInterfaces::anInterface
{
  private:
	Laxkit::flatpoint bezbuf[4];
	Laxkit::flatpoint panpath[4];
	int    pan_timer;
	double pan_current; //0..1
	double pan_duration; //seconds
	int    pan_tick_ms;
	char scratch[200];

	Laxkit::PtrStack<char> recent_node_types;

	Laxkit::PtrStack<NodeThread> forks; //used each Execute tick

	Laxkit::Attribute *passthrough; //pipe io helper data
	NodeConnection *onconnection;

	Laxkit::anObject *originating_data; //for when we are used as an on the spot editor for another tool

  protected:
	void GetConnectionBez(NodeConnection *connection, Laxkit::flatpoint *pts);

	bool try_refresh;
	virtual void NodesChanged();

	double play_fps, cur_fps;
	int playing;
	int play_timer;
	clock_t elapsed_time, elapsed_wall_time, run_start_time, time_at_pause;
	virtual int IsLive(NodeConnection *con);
	virtual int Play();
	virtual int TogglePause();
	virtual int Stop(); //resets threads
	virtual int FindThreads(bool flush);
	virtual int ExecuteThreads();
	virtual int IsThread(NodeBase *node);

	int show_threads;

	Laxkit::ObjectFactory *node_factory; //usually, convenience cast to return of NodeBase::NodeFactory()

	Nodes *nodes; //current group
	NodeColors *default_colors;

	Laxkit::MenuInfo *node_menu;
	virtual void RebuildNodeMenu();

	Laxkit::RefPtrStack<NodeGroup> grouptree; //stack of nested groups we are working on
	Laxkit::RefPtrStack<NodeBase> selected;
	Laxkit::DoubleBBox selection_rect;
	Laxkit::DoubleBBox thread_controls;
	double thread_run, thread_reset; //offsets in thread_controls
	NodeFrame *selected_frame;
	NodeConnection *tempconnection;
	int hover_action;
	int lasthover, lasthoverslot, lasthoverprop, lastconnection;
	Laxkit::flatpoint lastpos;
	int lastmenuindex;
	char *search_term;
	int last_search_index;

	Laxkit::PtrStack<NodeThread> threads;

	//Laxkit::Affine transform; //from nodes to screen coords

	//---user config:
	char *lastsave; //last file nodes were saved/loaded
	Laxkit::LaxFont *font;
	double defaultpreviewsize;

	Laxkit::ScreenColor color_connection;
	Laxkit::ScreenColor color_connection_selected;
	Laxkit::ScreenColor color_node_selected;
	Laxkit::ScreenColor color_node_default;
	Laxkit::ScreenColor color_controls;
	Laxkit::ScreenColor color_background;
	Laxkit::ScreenColor color_grid;
	double draw_grid;  //pixel spacing
	double vp_dragpad; //screen pixel amount for drag area around viewport_bounds
	//---end user config

	Laxkit::DoubleBBox viewport_bounds;

	Laxkit::ShortcutHandler *sc;

	virtual int FreshNodes(bool asresource);
	virtual int EditProperty(int nodei, int propertyi);
	virtual int send();
	virtual void UpdateCurrentColor(const Laxkit::ScreenColor &col);
	virtual void DebugOut(double x, double &y, Value *v);
	virtual void PlaceNewNode(NodeBase *newnode, Laxkit::flatpoint pos, bool place_near_mouse, bool is_new);

  public:
	unsigned int node_interface_style;

	NodeInterface(LaxInterfaces::anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~NodeInterface();
	virtual LaxInterfaces::anInterface *duplicate(LaxInterfaces::anInterface *dup);
	virtual const char *IconId() { return "Node"; }
	const char *Name();
	const char *whattype() { return "NodeInterface"; }
	const char *whatdatatype();
	virtual int  Idle(int tid, double delta);
	Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);

	virtual int InitializeResources();
	virtual int UseThis(Laxkit::anObject *nobj, unsigned int mask=0);
	virtual int UseResource(const char *id);
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

	virtual void SetOriginatingData(Laxkit::anObject *data, int absorb);

	virtual void DrawConnection(NodeConnection *connection);
	virtual void DrawPropertySlot(NodeBase *node, NodeProperty *prop, int hoverprop, int hoverslot);
	virtual void DrawProperty(NodeBase *node, NodeProperty *prop, double y, int hoverprop, int hoverslot);
	virtual int scan(int x, int y, int *overpropslot, int *overproperty, int *overconnection, unsigned int state);
	virtual int scanFrames(int x, int y, unsigned int state);
	virtual int IsSelected(NodeBase *node);

	virtual int ToggleCollapsed();
	virtual int EnterGroup(NodeGroup *group=NULL);
	virtual int LeaveGroup();
	virtual int DuplicateNodes();
	virtual int CutConnections(Laxkit::flatpoint p1,Laxkit::flatpoint p2);
	virtual int SaveNodes(const char *file);
	virtual int ExportNodes(const char *file, const char *format);
	virtual int LoadNodes(const char *file, bool append, int file_is_string_data, bool keep_passthrough);
	virtual int FindNext();
	virtual int Find(const char *what);
	virtual NodeGroup *GetCurrent() { return nodes; }
	virtual int SelectNode(NodeBase *what, bool also_center);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *savecontext);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *loadcontext);

	//system dnd
	virtual int selectionDropped(const unsigned char *data,unsigned long len,const char *actual_type,const char *which);
	virtual bool DndWillAcceptDrop(int x, int y, const char *action, Laxkit::IntRectangle &rect, char **types, int *type_ret);
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



//------------------- Property parsing helpers ---------------------

int DetermineSetIns(int num_ins, Value **ins, SetValue **setins, int &max, bool &dosets);

int GetInNumber(int index, bool dosets, double &prev, Value *in, SetValue *setin);


template<typename T>
T *GetInValue(int index, bool dosets, T *prev, Value *in, SetValue *setin)
{
	if (dosets && setin) {
		if (index < setin->n()) return dynamic_cast<T*>(setin->e(index));
		else return prev;
	} else if (!prev) prev = dynamic_cast<T*>(in);
	return prev;
}


template<typename T>
void GetOutValue(int index, bool dosets, T *&out, SetValue *setout)
{
	if (!dosets) return; //just go with what's there
	if (!setout) return;
	if (index >= setout->n()) return; //is probably an old value

	out = dynamic_cast<T*>(setout->e(index));	
}

/*! Make the property data be either the correct type or a set.
 * If doset, then make sure set has max number in it, and they are all
 * type T.
 */
template<typename T>
T *UpdatePropType(NodeProperty *prop, bool doset, int max, SetValue *&setout_ret)
{
	Value *v = prop->GetData();
	T *vv = nullptr;

	if (doset) {
		SetValue *set = dynamic_cast<SetValue*>(v);
		if (!set) {
			set = new SetValue();
			prop->SetData(set, 1);
		}

		//fill with correct data type
		for (int c=0; c<max; c++) {
			if (c >= set->n()) {
				vv = new T();
				set->Push(vv, 1);
			} else {
				vv = dynamic_cast<T*>(set->e(c));
				if (!vv) {
					vv = new T();
					set->Set(c, vv, 1);
				}
			}
		}

		//clamp to max
		while (set->n() > max) set->Remove(set->n()-1);
		setout_ret = set;

	} else {
		//single item
		vv = dynamic_cast<T*>(v);
		if (!vv) {
			vv = new T;
			prop->SetData(vv, 1);
		}
	}

	prop->Touch();
	return vv;
}

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
	Laxkit::anObject *stuff;

    NodeUndo(int ntype, int nisauto, double *mm, Laxkit::anObject *nstuff);
	virtual ~NodeUndo();
    virtual const char *Description();
};


} // namespace Laidout

#endif

