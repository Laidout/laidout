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



#include "nodeinterface.h"
#include "nodes.h"
#include "../utils.h"
#include "../version.h"
#include "../utils.h"

#include <lax/interfaces/interfacemanager.h>
#include <lax/laxutils.h>
#include <lax/bezutils.h>
#include <lax/popupmenu.h>
#include <lax/colorsliders.h>
#include <lax/filedialog.h>
#include <lax/language.h>

#include <string>


//Template implementation:
#include <lax/lists.cc>
#include <lax/refptrstack.cc>


using namespace Laxkit;
using namespace LaxFiles;
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
  : owner(NULL),

	mo_diff(.05),
	preview_dims(100),

	default_property(1.,1.,1.,1.),
	connection(.5,.5,.5,1.),
	sel_connection(1.,0.,1.,1.),
	number(.8,.8,.8,1.),
	vector(.9,.5,.9,1.),
	color(1.,1.,0.,1.),
	exec(1.,1.,1.,1.),

	label_fg(.2,.2,.2,1.),
	label_bg(.7,.7,.7,1.),
	fg(.2,.2,.2,1.),
	bg(.8,.8,.8,1.),
	text(0.,0.,0.,1.),
	border(.2,.2,.2,1.),
	error_border(.5,.0,.0,1.),
	update_border(1.,1.,0.,1.),

	fg_edit(.2,.2,.2,1.),
	bg_edit(.9,.9,.9,1.),

	fg_frame(.2,.2,.2,1.),
	bg_frame(.65,.65,.65,1.),

	fg_menu(.2,.2,.2,1.),
	bg_menu(.7,.7,.7,1.),

	selected_border(1.,.8,.1,1.),
	selected_bg(.9,.9,.9,1.)
{
	state = 0;
	font  = NULL;
	next  = NULL;
	slot_radius=.25; //portion of text height
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
 *
 * Note connections are not reference counted.
 * This is ok since the connections only exist in relation to the nodes, and nodes don't get stranded
 * when a connection deletes itself, as they are in a list in a NodeGroup.
 *
 * Connections in and out of groups get special treatment with proxies. If either is not NULL, then
 * they point to properties of the container group.
 */

NodeConnection::NodeConnection()
{
	from     = to     = NULL;
	fromprop = toprop = NULL;
}

NodeConnection::NodeConnection(NodeBase *nfrom, NodeBase *nto, NodeProperty *nfromprop, NodeProperty *ntoprop)
{
	from     = nfrom;
	to       = nto;
	fromprop = nfromprop;
	toprop   = ntoprop;
}

/*! Remove connection refs from any connected props.
 */
NodeConnection::~NodeConnection()
{
	DBG cerr << "NodeConnection destructor "<<(from ? from->Name : "?")<<" -> "<<(to ? to->Name : "?")<<endl;
}

int NodeConnection::IsExec()
{
	if (fromprop && fromprop->IsExec()) return 1;
	if (toprop   && toprop  ->IsExec()) return 1;
	return 0;
}


//-------------------------------------- NodeProperty --------------------------


/*! \class NodeProperty
 * Base class for properties of nodes, either input or output.
 */

NodeProperty::NodeProperty()
{
	color.rgbf(1.,1.,1.,1.);

	owner     = NULL;
	data      = NULL;
	datatypes = NULL;
	name      = NULL;
	label     = NULL;
	tooltip   = NULL;
	modtime   = 0;
	custom_info= 0;
	frompropproxy = NULL;
	topropproxy   = NULL;
	x=y=width=height = 0;

	type = PROP_Unknown;
	is_linkable = false; //default true for something that allows links in
	is_editable = true;
	hidden      = false;
}

/*! If !absorb_count, then ndata's count gets incremented.
 */
NodeProperty::NodeProperty(PropertyTypes input, bool linkable, const char *nname, Value *ndata, int absorb_count,
							const char *nlabel, const char *ntip, int info, bool editable)
{
	color.rgbf(1.,1.,1.,1.);

	owner     = NULL;
	datatypes = NULL;
	data      = ndata;
	if (data && !absorb_count) data->inc_count();
	custom_info = info;
	modtime   = 0;
	frompropproxy = NULL;
	topropproxy   = NULL;
	x=y=width=height = 0;

	type      = input;
	is_linkable = linkable;
	if (type == PROP_Block) is_linkable = false;
	is_editable = editable;
	hidden      = false;

	name      = newstr(nname);
	label     = newstr(nlabel);
	tooltip   = newstr(ntip);
}

NodeProperty::~NodeProperty()
{
	delete[] name;
	delete[] label;
	delete[] tooltip;
	delete[] datatypes;
	if (data) data->dec_count();
}

const char *NodeProperty::Name(const char *nname)
{
	makestr(name, nname);
	return name;
}

const char *NodeProperty::Label(const char *nlabel)
{
	makestr(label, nlabel);
	return label;
}

/*! Set default width and height for this property.
 * Default is to get extent of label + (number | string | enum).
 */
void NodeProperty::SetExtents(NodeColors *colors)
{
	double w = colors->font->extent(Label(),-1);
	double th = colors->font->textheight();

	Value *v = dynamic_cast<Value*>(data);
	if (v) {
		if (v->type()==VALUE_Real || v->type()==VALUE_Int) {
			w += 3*th;

		} else if (v->type()==VALUE_Color) {
			w += 3*th;

		} else if (v->type()==VALUE_String) {
			StringValue *s = dynamic_cast<StringValue*>(v);
			w += th + colors->font->extent(s->str, -1);

		} else if (v->type()==VALUE_Enum) {
			EnumValue *ev = dynamic_cast<EnumValue*>(v);
			const char *nm=NULL, *Nm=NULL;
			double ew=0, eww;

			for (int c=0; c<ev->GetObjectDef()->getNumEnumFields(); c++) {
				ev->GetObjectDef()->getEnumInfo(c, &nm, &Nm);
				if (!Nm) Nm = nm;
				if (isblank(Nm)) continue;
				eww = colors->font->extent(Nm,-1);
				if (eww>ew) ew=eww;
			}
			w += ew;
		}

		x      = 0;
		width  = w;
		height = 1.5*th;

	} else {
		 //set a default height for assuming just writing out label
		if (height==0) height = 1.5*th;
	}
}

/*! Return an interface if you want to have a custom interface for this property.
 * If interface!=NULL, try to update (and return) that one. If provided
 * interface is the wrong type of interface, then return NULL.
 * Adjust interface to use dp.
 *
 * Default is to return NULL, for no special interface necessary.
 */
anInterface *NodeProperty::PropInterface(LaxInterfaces::anInterface *interface, Laxkit::Displayer *dp)
{ 
	return NULL;
}

/*! 0=no, -1=prop is connected input, >0 == how many connected output
 */
int NodeProperty::IsConnected()
{
	if (IsInput() || IsExecIn()) return -connections.n;
	return connections.n;
}

int NodeProperty::AllowInput()
{
	return IsInput() && is_linkable;
}

int NodeProperty::AllowOutput()
{
	return IsOutput();
}

/*! Make hidden flag be true.
 */
int NodeProperty::Hide()
{
	hidden = true;
	return hidden;
}

/*! Make hidden flag be false.
 */
int NodeProperty::Show()
{
	hidden = false;
	return hidden;
}

/*! Return true if it is ok to attach ndata to this property.
 *
 * Default is to check ndata->type() against datatypes.
 * If datatypes==NULL, then assume ok.
 */
bool NodeProperty::AllowType(Value *ndata)
{
	if (!ndata) return false;
	if (!datatypes) return true;
	int type = ndata->type();
	for (int c=0; datatypes[c] != VALUE_None; c++) {
		if (type == datatypes[c]) return true;
	}
	return false;
}

/*! Return the node and property index in that node of the specified connection.
 */
NodeBase *NodeProperty::GetConnection(int connection_index, int *prop_index_ret)
{
	if (connection_index<0 || connection_index>=connections.n) {
		if (prop_index_ret) *prop_index_ret=-1;
		return NULL;
	}

	NodeConnection *connection = connections.e[connection_index];
	if (prop_index_ret) {
		if (IsInput()) {
			NodeBase *node = connection->from;
			if (node) *prop_index_ret = node->properties.findindex(connection->fromprop);
			else *prop_index_ret = -1;

		} else if (IsOutput()) {
			NodeBase *node = connection->to;
			if (node) *prop_index_ret = node->properties.findindex(connection->toprop);
			else *prop_index_ret = -1;
		}
	}

	if (IsInput()) return connection->from;
	return connection->to;
}

/*! Return the data associated with this property.
 * If it is a connected input, then get the corresponding output data from the connected node,
 * or the internal data if the node is not connected.
 * The data's count is not modified, so if you want to use it for a while, you should
 * increment its count as soon as you get it.
 */
Value *NodeProperty::GetData()
{
	 //note: this assumes fromprop is a pure output, not a through
	if (IsInput()) {
		if (connections.n && connections.e[0]->fromprop) {
			Value *v = connections.e[0]->fromprop->GetData();
			if (v) return v;
			if (connections.e[0]->fromprop->frompropproxy) {
				v = connections.e[0]->fromprop->frompropproxy->GetData();
				if (v) return v;
			}
			return NULL; //missing input variable, probably an error!
		}
	}
	return data;
}

NodeBase *NodeProperty::GetDataOwner()
{
	 //note: this assumes fromprop is a pure output, not a through
	if (IsInput() && connections.n && connections.e[0]->fromprop) return connections.e[0]->fromprop->GetDataOwner();
	return owner;
}

/*! Returns 1 for successful setting, or 0 for not set. newdata not absorbed on failure.
 */
int NodeProperty::SetData(Value *newdata, bool absorb)
{
	if (newdata==data) {
		if (absorb) data->dec_count();
		return 1;
	}
	if (data) data->dec_count();
	data = newdata;
	if (data && !absorb) data->inc_count();
	return 1;
}

/*! Update mod time to now.
 */
void NodeProperty::Touch()
{
	modtime = times(NULL);
}


//-------------------------------------- NodeThread --------------------------
/*! \class NodeThread
 * Payload for execution connections.
 */


/*! If payload==NULL, then a new ValueHash is created.
 */
NodeThread::NodeThread(NodeBase *node, NodeProperty *prop, ValueHash *payload, int absorb)
{
	thread_id = getUniqueNumber();
	start_time = times(NULL);
	data = NULL;
	property = prop;
	next = node;
	if (next) next->inc_count();

	data = payload;
	if (data) {
		if (!absorb) data->inc_count();
	} else {
		data = new ValueHash();
	}
}

NodeThread::~NodeThread()
{
	if (data) data->dec_count();
	if (next) next->dec_count();
}

/*! Update thread with connection for next execution path.
 */
int NodeThread::UpdateThread(NodeBase *node, NodeConnection *gonext)
{
	if (next) next->dec_count();
	next = node;
	if (next) next->inc_count();
	return 0;
}


//---------------------------- NodeFrame ------------------------------------
/*! \class NodeFrame
 * A kind of node comment to display nodes in a bubble.
 */

NodeFrame::NodeFrame(NodeGroup *nowner, const char *nlabel, const char *ncomment)
{
	owner   = nowner;
	label   = newstr(nlabel);
	comment = newstr(ncomment);
}

NodeFrame::~NodeFrame()
{
	for (int c=nodes.n-1; c>=0; c--) {
		RemoveNode(nodes.e[c]);
	}
	delete[] label;
	delete[] comment;
}

const char *NodeFrame::Label(const char *nlabel)
{
	makestr(label, nlabel);
	return label;
}

const char *NodeFrame::Comment(const char *ncomment)
{
	makestr(comment, ncomment);
	return comment;
}

/*! incs count of node.
 */
int NodeFrame::AddNode(NodeBase *node)
{
	if (node->frame && node->frame != this) {
		node->frame->RemoveNode(node);
	}
	node->AssignFrame(this);
	return nodes.push(node);
}

/*! Return 0 for success, or 1 for not found.
 */
int NodeFrame::RemoveNode(NodeBase *node)
{
	if (node->frame != this) return 1;
	node->AssignFrame(NULL);
	return nodes.remove(nodes.findindex(node));
}

void NodeFrame::Wrap(double gap)
{
	if (!nodes.n) return;

	double th = (owner && owner->colors ? owner->colors->font->textheight() : 20);
	if (gap<0) gap = th;

	DoubleBBox box;
	for (int c=0; c<nodes.n; c++) {
		box.addtobounds(*nodes.e[c]);
	}

	double lw = (label && owner && owner->colors ? owner->colors->font->extent(label,-1) : 100);
	if (box.boxwidth() < lw) box.maxx = box.minx + lw;
	box.ShiftBounds(-gap,gap, -(th*1.5 + gap),gap);

	x = box.minx;
	y = box.miny;
	width  = box.maxx - box.minx;
	height = box.maxy - box.miny;
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
	psamplew = psampleh = -1; //for when no defined rectangle to sample. -1 means use default
	preview_area_height = -1;
	show_preview = true;
	colors    = NULL;
	collapsed = 0;
	fullwidth = 0;
	deletable = true; //often at least one node will not be deletable, like group output/inputs
	modtime   = 0;

	frame = NULL;

	type = NULL;
	def  = NULL;
	error_message = NULL;
}

NodeBase::~NodeBase()
{
	delete[] Name;
	delete[] type;
	delete[] error_message;
	if (def) def->dec_count();
	if (colors) colors->dec_count();
	if (total_preview) total_preview->dec_count();
}

/*! An execution path leads here.
 * Return 1 for done with thread.
 * 0 for successful run.
 */
NodeBase *NodeBase::Execute(NodeThread *thread)
{
	return NULL;
}

/*! Dec old def, install new one and inc count if !absorb_count.
 */
void NodeBase::InstallDef(ObjectDef *ndef, bool absorb_count)
{
	if (def) def->dec_count();
	def = ndef;
	if (!absorb_count && def) def->inc_count();
}

int NodeBase::Undo(UndoData *data)
{
	cerr << " *** need to implement NodeBase::Undo()!!"<<endl;
	return 1;
}

int NodeBase::Redo(UndoData *data)
{
	cerr << " *** need to implement NodeBase::Redo()!!"<<endl;
	return 1;
}

/*! Change the label text. Returns result.
 */
const char *NodeBase::Label(const char *nlabel)
{
	makestr(Name, nlabel);
	return Name;
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

/*! Return a custom interface to use for this whole node.
 * If non-null is returned, then it is assumed that interface will render the entire
 * node, as well as handle all mouse and key events.
 *
 * If interface!=NULL, then update to use *this, and return it. If it is the wrong type
 * of interface, return NULL.
 *
 * If interface == NULL, then return a new instance of a proper interface.
 *
 * Return NULL for default node rendering.
 */
LaxInterfaces::anInterface *NodeBase::GetInterface(LaxInterfaces::anInterface *interface)
{
	return NULL;
}

/*! Return whether the node has valid values, or the outputs are older than inputs.
 * Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 *
 * Default placeholder behavior is to return 1 if any output property has modtime less than
 * any input modtime. Else return 0. Thus subclasses need only redefine to catch error states.
 */
int NodeBase::GetStatus()
{ 
	 //find newest mod time of inputs
	std::clock_t t=0;
	for (int c=0; c<properties.n; c++) {
		if (!properties.e[c]->IsOutput() && properties.e[c]->modtime > t) t = properties.e[c]->modtime;
	}
	if (t==0) return 0; //earliest output time is beginning, or no outputs, so nothing to be done!

	 //if any outputs older than newest mod, then return 1
	for (int c=0; c<properties.n; c++) {
		if (properties.e[c]->IsOutput()) continue;
		if (properties.e[c]->modtime < t) return 1; 
	}
	return 0;
}

/*! Return the most recent modtime for input or block properties.
 * Optionally return the index of that property.
 */
clock_t NodeBase::MostRecentIn(int *index)
{
	clock_t time = 0;
	int i = -1;
	for (int c=0; c<properties.n; c++) {
		if (!properties.e[c]->IsInput() && !properties.e[c]->IsBlock()) continue;
		if (properties.e[c]->modtime > time) {
			time = properties.e[c]->modtime;
			i = c;
		}
	}
	if (index) *index = i;
	return time;
}

/*! Call Update() on all connected output properties.
 * Just like Update(), but don't call GetStatus() and doesn't update this->modtime.
 */
void NodeBase::PropagateUpdate()
{
	NodeProperty *prop;

	for (int c=0; c<properties.n; c++) {
		prop = properties.e[c];

		if (!prop->IsOutput()) continue;
		if (prop->connections.n==0) continue;

		for (int c2=0; c2<prop->connections.n; c2++) {
			if (prop->connections.e[c2]->to) {
				prop->connections.e[c2]->to->Update();
				if (prop->connections.e[c2]->toprop->topropproxy)
					prop->connections.e[c2]->toprop->topropproxy->owner->Update();
			}
		}
	}
}

/*! Call whenever any of the inputs change, update outputs.
 * Default placeolder is to update modtime to now, call PropagateUpdate(), and return GetStatus().
 * Subclasses should redefine to actually update the outputs based on the inputs
 * or any other internal state, as well as the overall preview (if any).
 *
 * Returns GetStatus().
 */
int NodeBase::Update()
{
	modtime = times(NULL);
	PropagateUpdate();
	return GetStatus();
}

int NodeBase::UpdatePreview()
{
	return 1;
}

/*! Change default sample size for previews.
 * For instance, many gegl nodes are unbound by default, so we sample a specified area for previews.
 * Only changes psamplew,h. Does not actually update preview.
 * 
 * If is_shift, then add to existing values, relative to displayed preview size. Clamped at 1.
 * Otherwise, set absolutely.
 */
void NodeBase::PreviewSample(double w, double h, bool is_shift)
{
	if (is_shift) {
		if (psamplew <=0) psamplew = colors->preview_dims;
		if (psampleh <=0) psampleh = colors->preview_dims;
		double scale = psampleh / preview_area_height;
		psamplew += w * scale;
		psampleh += h * scale;
	} else {
		psamplew = w;
		psampleh = h;
	}
	if (psamplew < 1) psamplew = 1;
	if (psampleh < 1) psampleh = 1;
}

/*! Update the bounds to be just enough to encase everything.
 */
int NodeBase::Wrap()
{
	if (preview_area_height < 0 && colors && colors->font) preview_area_height = 3*colors->font->textheight();
	if (collapsed) return WrapCollapsed();
	return WrapFull(fullwidth > 0 ? true : false);
}

/*! Lay out properties, but don't change the width.
 */
void NodeBase::UpdateLayout()
{
	if (preview_area_height < 0 && colors && colors->font) preview_area_height = 3*colors->font->textheight();
	if (collapsed) WrapCollapsed();
	else WrapFull(true);
}

int NodeBase::WrapFull(bool keep_current_width)
{
	if (!colors || !colors->font) return -1;

	 //find overall width and height
	double th = colors->font->textheight();

	height = th*1.5;

	 //make sure all properties have some height
	NodeProperty *prop;
	for (int c=0; c<properties.n; c++) {
		prop = properties.e[c];
		if (prop->height == 0) prop->SetExtents(colors);
	}

	if (!keep_current_width) {
		width = colors->font->extent(Name,-1);

		 //find wrap width
		for (int c=0; c<properties.n; c++) {
			prop = properties.e[c];

			prop->SetExtents(colors);
			if (prop->width > width) width = prop->width;
		}

		width += 3*th;
		if (fullwidth > width) width = fullwidth;

	} else {
		//keep width
	}
	double propwidth = width; //the max prop width.. make them all the same width

	if (fullwidth == 0) fullwidth = width;

	 //update link positions
	height = 1.5*th;
	if (UsesPreview()) height += preview_area_height;
	//if (UsesPreview()) height += total_preview->h();
	double y = height;

	for (int c=0; c<properties.n; c++) {
		prop = properties.e[c];
		prop->x = 0;
		prop->y = y;
		prop->pos.y = y+prop->height/2;

		if (prop->IsInput() || prop->IsExecIn()) prop->pos.x = 0;
		else prop->pos.x = width;

		y+=prop->height;
		height += prop->height;

		prop->width = propwidth;
	} 

	return 0;
}


/*! Update the bounds to be the collapsed version, and set props->pos
 * to be squashed along the edges.
 */
int NodeBase::WrapCollapsed()
{
	if (!colors) return -1;

	 //find overall width and height
	double th = colors->font->textheight();

	width = 3*th + colors->font->extent(Name,-1);
	height = th*1.5;
	if (UsesPreview()) {
		height += preview_area_height;
		//height += total_preview->h();
		double nw = total_preview->w() * preview_area_height/total_preview->h();
		if (width < nw) width = nw;
	}

	NodeProperty *prop;
	int num_in = 0, num_out = 0;

	for (int c=0; c<properties.n; c++) {
		prop = properties.e[c];
		if (prop->AllowInput()  || prop->IsExecIn())  num_in++;
		if (prop->AllowOutput() || prop->IsExecOut()) num_out++;
	}

	int max = num_in;
	if (num_out > max) max = num_out;

	double slot_radius = colors->slot_radius;
	if (height < th/2 + max*th*2*slot_radius) height = th/2 + max*th*2*slot_radius;

	 //update link position
	double in_y  = height/2 - num_in *th*slot_radius;
	double out_y = height/2 - num_out*th*slot_radius;

	for (int c=0; c<properties.n; c++) {
		prop = properties.e[c];

		if (prop->AllowInput() || prop->IsExecIn()) {
			prop->pos.x = 0;
			prop->pos.y = in_y+th*slot_radius;
			in_y += 2*th*slot_radius;

		} else if (prop->AllowOutput() || prop->IsExecOut()) {
			prop->pos.x = width;
			prop->pos.y = out_y+th*slot_radius;
			out_y += 2*th*slot_radius;
		} 
	} 

	return 0;
}

/*! This will adjust the x and y positions for prop->pos.
 * Call this whenever you resize a node and the node is not collapsed.
 * Assumes properties already has proper bounding boxes set.
 * By default Inputs go on middle of left edge. Outputs middle of right.
 */
void NodeBase::UpdateLinkPositions()
{
	NodeProperty *prop;
	for (int c=0; c<properties.n; c++) {
		prop = properties.e[c];

		prop->pos.y = prop->y + prop->height/2;

		if (prop->IsInput() || prop->IsExecIn()) prop->pos.x = 0;
		else prop->pos.x = width;

	} 
}

/*! -1 toggle, 0 open, 1 collapsed
 */
int NodeBase::Collapse(int state)
{
	if (state==-1) state = !collapsed;

	if (state==true && state!=collapsed) {
		 //collapse!
		collapsed = true;
		fullwidth = width;
		Wrap();

	} else if (state==false && state!=collapsed) {
		 //expand!
		collapsed = false;
		Wrap();
	}

	return collapsed;
}

/*! Copy over dimensions and colors, and other display related settings.
 * Does NOT copy properties. 
 */
void NodeBase::DuplicateBase(NodeBase *from)
{
	Id(from->Id());
	x = from->x;
	y = from->y;
	width = from->width;
	height = from->height;
	collapsed = from->collapsed;
	fullwidth = from->fullwidth;
	show_preview = from->show_preview;
	preview_area_height = from->preview_area_height;

	if (from->colors) InstallColors(from->colors, 0);
}

/*! Copies over properties from from.
 * Duplicates the data contained in the property.
 */
void NodeBase::DuplicateProperties(NodeBase *from)
{
	for (int c=0; c < from->properties.n; c++) {
		NodeProperty *property = from->properties.e[c];
		Value *v = NULL;

		 //always dup data, just in case
		v = property->GetData();
		if (v) v = v->duplicate();

		NodeProperty *prop = new NodeProperty(
									property->type,
									property->is_linkable,
									property->name,
									v,1,
									property->Label(),
									property->tooltip,
									property->custom_info,
									property->is_editable);

		prop->color = property->color;
		//prop->color.rgbf(property->color.Red(), property->color.Green(), property->color.Blue(), property->color.Alpha());
		AddProperty(prop);
	}
}

/*! Return a new duplicate node not connected or owned by anything.
 * Subclasses need to redefine this, or a plain NodeBase will be returned.
 * 
 * Beware that the returned node here will have the same Id() name.
 */
NodeBase *NodeBase::Duplicate()
{
	DBG cerr << "NodeBase::Duplicate() from id: "<<Id()<<", label: "<<Label()<<endl;

	NodeBase *newnode = new NodeBase();

	if (def) { newnode->def = def; def->inc_count(); }
	makestr(newnode->type, type);
	makestr(newnode->Name, Name);
	newnode->Id(Id());
	newnode->DuplicateBase(this);

	for (int c=0; c<properties.n; c++) {
		NodeProperty *property = properties.e[c];
		Value *v = NULL;
		//if (property->IsInput() || property->IsBlock()) {
			v = property->GetData();
			if (v) v = v->duplicate();
		//}

		NodeProperty *prop = new NodeProperty(
									property->type,
									property->is_linkable,
									property->name,
									v,1,
									property->Label(),
									property->tooltip,
									property->custom_info,
									property->is_editable);
		prop->color.rgbf(property->color.Red(), property->color.Green(), property->color.Blue(), property->color.Alpha());
		newnode->AddProperty(prop);
	}

	newnode->Wrap();
	newnode->Update();

	return newnode;
}

/*! Simple assigns the reference. Does not do ref upkeep in frame.
 */
int NodeBase::AssignFrame(NodeFrame *nframe)
{
	frame = nframe;
	return 0;
}

/*! Check whether a specified property is connected to anything.
 * 0=no, -1=prop is connected input, 1=connected output
 */
int NodeBase::IsConnected(int propindex)
{
	if (propindex<0 || propindex>=properties.n) return -1;
	return properties.e[propindex]->IsConnected();
}

/*! Return the property index of the first property that has a connection to 
 * another node's property.
 */
int NodeBase::HasConnection(NodeProperty *prop, int *connection_ret)
{
	for (int c=0; c<properties.n; c++) {
		for (int c2=0; c2<properties.e[c]->connections.n; c2++) {
			if (properties.e[c]->connections.e[c2]->toprop  ==prop ||
				properties.e[c]->connections.e[c2]->fromprop==prop) {
					*connection_ret = c2;
					return c;
			}
		}
	}

	*connection_ret = -1;
	return -1;
}

///*! Connect src property to properties.e[propertyindex].
// *
// * Return 0 for success or nonzero error.
// */
//int NodeBase::Connect(NodeConnection *connection, NodeProperty *srcproperty, int propertyindex)
//{
//	if (propertyindex < 0 || propertyindex >= properties.n) return 1;
//
//	NodeProperty *prop = properties.e[propertyindex];
//
//	***
//	return 0;
//}

//NodeConnection *NodeBase::Connect(NodeBase *from, const char *fromprop, const char *toprop)
//{ ***
//}

/*! Remove property[propertyindex].connections[connectionindex].
 * Note this does not 
 * return 0 for success or nonzero error.
 *
 * Subclasses may redefine to have custom behavior on disconnects, but can still call
 * this last to actually remove the connection.
 */
//int NodeBase::Disconnect(int propertyindex, int connectionindex)
//{
//	if (propertyindex < 0 || propertyindex >= properties.n) return 1;
//	if (connectionindex < 0 || connectionindex >= properties.e[propertyindex]->connections.n) return 1;
//	properties.e[propertyindex]->connections.remove(connectionindex);
//	return 0;
//}

/*! A notification that this connection is being removed, right before the actual removal.
 * Actual removal is done elsewhere.
 * *_will_be_replaced are hints to not drastically restructure if the link is merely to be replaced,
 * rather than removed.
 *
 * Connected() and Disconnected() calls are usually followed shortly after by a call to Update(), so
 * these functions are for low level linkage maintenance, and Update()
 * is the bigger trigger for updating outputs.
 *
 * Return 1 if something changed that needs a screen refresh. else 0.
 */
int NodeBase::Disconnected(NodeConnection *connection, bool from_will_be_replaced, bool to_will_be_replaced)
{
//	int propertyindex = properties.findindex(connection->from == this ? connection->fromprop : connection->toprop);
//	if (propertyindex < 0) return 0; //property not found!
//	int connectionindex = property->connections.findindex(connection);
//	if (connectionindex < 0) return 0; //connection not found!
//
//	if (to_side && connection->to == this) {
//		properties.e[propertyindex]->connections.remove(connectionindex);
//	}
	return 0;
}

/*! A notification that happens right after a connection is added.
 * Connected() and Disconnected() calls are usually followed shortly after by a call to Update(), so
 * these functions are for low level linkage maintenance (but not messing with
 * the connection themselves), and Update() is the bigger trigger for updating outputs.
 *
 * Return 1 to hint that a refresh is needed. Else 0.
 */
int NodeBase::Connected(NodeConnection *connection)
{
	return 0;
}

/*! Push this property onto the properties stack, and make sure owner points to this.
 * This does not check for prior existence of similar properties, always adds.
 *
 * Return 0 for succes, or nonzero for some kind of error.
 */
int NodeBase::AddProperty(NodeProperty *newproperty, int where)
{
	properties.push(newproperty, 1, where);
	newproperty->owner = this;
	return 0;
}

int NodeBase::RemoveProperty(NodeProperty *prop)
{
	return properties.remove(prop);
}

/*! Return the property with prop as name, or NULL if not found.
 */
NodeProperty *NodeBase::FindProperty(const char *prop)
{
	if (!prop) return NULL;
	for (int c=0; c<properties.n; c++) {
		if (!strcmp(prop, properties.e[c]->name)) {
			return properties.e[c];
		}
	}
	return NULL;
}

/*! Return 1 for property set, 0 for could not set.
 */
int NodeBase::SetProperty(const char *prop, Value *value, bool absorb)
{
	for (int c=0; c<properties.n; c++) {
		if (!strcmp(prop, properties.e[c]->name)) {
			return properties.e[c]->SetData(value, absorb);
		}
	}
	return 0;
}

/*! Set an existing property from att.
 * This function aids dump_in_atts. Default will handle any builtin Value types, except enums.
 * Subclasses should redefine to catch these.
 *
 * Return 1 for property set, 0 for could not set.
 */
int NodeBase::SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att)
{
	if (att->attributes.n == 0) return 0; //nothing to do if no data!

	NodeProperty *prop = FindProperty(propname);
	if (!prop) return 0; //only work on existing props.

	 //check for EnumValue
    if (att->attributes.n == 1 && !strcmp(att->attributes.e[0]->name,"EnumValue")) {
		if (isblank(att->attributes.e[0]->value)) return 0; //missing actual value, corrupt file!

		 //we hope current value was constructed with an EnumValue in place
		EnumValue *ev  = dynamic_cast<EnumValue*>(prop->GetData());
		if (ev) {
			ObjectDef *def = ev->GetObjectDef();

			 //value should be like "GeglSamplerType.Cubic"
			const char *nval = lax_extension(att->attributes.e[0]->value);
			if (!nval) nval = att->attributes.e[0]->value;
			if (isblank(nval)) return 0;
			// *** should probably check that the first part is actually the enum in def

			const char *nm;
			for (int c=0; c<def->getNumEnumFields(); c++) {
				def->getEnumInfo(c, &nm); //grabs id == name in the def
				if (!strcmp(nm, nval)) {
					ev->value = c; //note: makes enum value the index of the enumval def, not the id
					break;
					return 1;
				}
			}
		}
		return 0; //couldn't set it!
	}


	Value *val = AttributeToValue(att->attributes.e[0]);
	if (val) {
		if (!prop->SetData(val, true)) {
			val->dec_count();
			return 0;
		}
	}

	return 1;
}

/*! If connected, then return the number of connected inputs.
 * Else just the number of input properties.
 */
int NodeBase::NumInputs(bool connected)
{
	int n = 0;
	for (int c=0; c<properties.n; c++) {
		if (properties.e[c]->IsInput()) {
			if (connected) {
				n += properties.e[c]->connections.n;
			} else n++;
		}
	}
	return n;
}

/*! If connected, then return the number of connected outputs.
 * Else just the number of output properties.
 */
int NodeBase::NumOutputs(bool connected)
{
	int n = 0;
	for (int c=0; c<properties.n; c++) {
		if (properties.e[c]->IsOutput()) {
			if (connected) {
				n += properties.e[c]->connections.n;
			} else n++;
		}
	}
	return n;
}

void NodeBase::dump_out(FILE *f, int indent, int what, LaxFiles::DumpContext *context)
{
    Attribute att;
    dump_out_atts(&att,what,context);
    att.dump_out(f,indent);
}

LaxFiles::Attribute *NodeBase::dump_out_atts(LaxFiles::Attribute *att, int what, LaxFiles::DumpContext *context)
{
   if (!att) att=new Attribute();

    if (what==-1) {
        att->push("id", "some_name");
        att->push("label", "Displayed label");
        att->push("collapsed");
        att->push("show_preview");
        //att->push("", "");
        return att;
    }

    att->push("id", Id()); 
	att->push("label", Label());
	if (collapsed) att->push("collapsed");
	if (show_preview) att->push("show_preview");
	att->push("fullwidth", fullwidth);
	att->push("preview_height", preview_area_height);
	if (psamplew > 0) att->push("psamplew", psamplew);
	if (psampleh > 0) att->push("psampleh", psampleh);

	char s[200];
	sprintf(s,"%.10g %.10g %.10g %.10g", x,y,width,height);
	att->push("xywh", s);

	 //colors
	//NodeColors *colors;

	 //properties
	NodeProperty *prop;
	Attribute *att2;
	for (int c2=0; c2<properties.n; c2++) { 
		prop = properties.e[c2]; 
		att2 = NULL;

		if (prop->IsInput()) {
			//provide for reference if connected...
			//if (prop->IsConnected()) continue; //since it'll be recomputed after read in
			att2 = att->pushSubAtt("in", prop->name);
		} else if (prop->IsOutput()) {
			att2 = att->pushSubAtt("out", prop->name); //just provide for reference
		} else if (prop->IsBlock()) {
			att2 = att->pushSubAtt("block", prop->name);
		//} else if (prop->IsExec()) att2 = att->pushSubAtt("exec",
		//		prop->type==PROP_Exec_Through ? "through" : (prop->type==PROP_Exec_In ? "in" : "out"));
		} else continue;

		if ((prop->IsBlock() || (prop->IsInput() && !prop->IsConnected()) || (prop->IsOutput())) && prop->GetData()) {
			prop->GetData()->dump_out_atts(att2, what, context);
		} else {
			DBG cerr <<" not data for node out: "<<prop->name<<endl;
			//att2->push(prop->name, "arrrg! todo!");
		}
	}

	return att;
}

void NodeBase::dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context)
{
	if (!att) return;

    char *name,*value;

	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"id")) {
			Id(value);

		} else if (!strcmp(name,"label")) {
			Label(value);

		} else if (!strcmp(name,"xywh")) {
			double xywh[4];
			int l = DoubleListAttribute(value,xywh,4);
			if (l==4) {
				x = xywh[0];
				y = xywh[1];
				width  = xywh[2];
				height = xywh[3];
			}

		} else if (!strcmp(name,"x")) {
			DoubleAttribute(value, &x);

		} else if (!strcmp(name,"y")) {
			DoubleAttribute(value, &y);

		} else if (!strcmp(name,"width")) {
			DoubleAttribute(value, &width);

		} else if (!strcmp(name,"height")) {
			DoubleAttribute(value, &height);

		} else if (!strcmp(name,"fullwidth")) {
			DoubleAttribute(value, &fullwidth);

		} else if (!strcmp(name,"collapsed")) {
			collapsed = BooleanAttribute(value);

		} else if (!strcmp(name,"show_preview")) {
			show_preview = BooleanAttribute(value);

		} else if (!strcmp(name,"preview_height")) {
			DoubleAttribute(value, &preview_area_height);

		} else if (!strcmp(name,"psamplew")) {
			DoubleAttribute(value, &psamplew);

		} else if (!strcmp(name,"psampleh")) {
			DoubleAttribute(value, &psampleh);

		} else if (!strcmp(name,"in") || !strcmp(name,"out")) {
			NodeProperty *prop = FindProperty(value);
			if (!prop) {
				 //create property if not found. This happens for bare custom nodes, and ins/outs
				// *** this is inadequate for fully custom nodes!
				//prop = new NodeProperty(NodeProperty::PROP_Input, true, value, NULL,1, prop->label, prop->tooltip);
				prop = new NodeProperty(!strcmp(name,"in") ? NodeProperty::PROP_Input : NodeProperty::PROP_Output, true, value, NULL,1, NULL, NULL);
				AddProperty(prop);
			}

			SetPropertyFromAtt(value, att->attributes.e[c]); 
		}
	}
}

//-------------------------------------- NodeGroup --------------------------
/*! \class NodeGroup
 * Class to hold a collection of nodes.
 */

/*! Static keeper of default node factory.
 */
Laxkit::SingletonKeeper NodeGroup::factorykeeper;

/*! Static stack of node loaders.
 */
Laxkit::RefPtrStack<ObjectIO> NodeGroup::loaders;

int NodeGroup::InstallLoader(ObjectIO *loader, int absorb_count)
{
	int status = loaders.push(loader);
	if (absorb_count) loader->dec_count();
	return status;
}

int NodeGroup::RemoveLoader(ObjectIO *loader)
{
	return loaders.remove(loaders.findindex(loader));
}



/*! Return the current default node factory. If the default is null, then make a new default if create==true.
 */
Laxkit::ObjectFactory *NodeGroup::NodeFactory(bool create)
{
	ObjectFactory *node_factory = dynamic_cast<ObjectFactory*>(factorykeeper.GetObject());
	if (!node_factory && create) {
		node_factory = new ObjectFactory; 
		factorykeeper.SetObject(node_factory, true);

		SetupDefaultNodeTypes(node_factory);
	}

	return node_factory;
}

/*! Install newnodefactory. If it is null, then remove the default. Else inc count on it (and install as default).
 */
void NodeGroup::SetNodeFactory(Laxkit::ObjectFactory *newnodefactory)
{
	factorykeeper.SetObject(newnodefactory, false);
}

NodeGroup::NodeGroup()
{
	background.rgbf(0,0,0,.5);
	output= NULL;
	input = NULL;
	makestr(type, "NodeGroup");
}

NodeGroup::~NodeGroup()
{
	if (output) output->dec_count();
	if (input)  input ->dec_count();
}

/*! Returns a deep copy, but colors are linked.
 */
NodeBase *NodeGroup::Duplicate()
{
	NodeGroup *newgroup = new NodeGroup();
	makestr(newgroup->type, type);
	makestr(newgroup->Name, Name);

	newgroup->DuplicateBase(this);
	newgroup->DuplicateProperties(this);

	 //copy child nodes
	for (int c=0; c<nodes.n; c++) {
		NodeBase *nnode = nodes.e[c]->Duplicate();
		nnode->Wrap();
		newgroup->nodes.push(nnode);
		if (nodes.e[c] == input)  newgroup->DesignateInput (nnode);
		if (nodes.e[c] == output) newgroup->DesignateOutput(nnode);
		nnode->dec_count();
	}

	 //add connections to new
	NodeProperty *prop, *fromp, *top;
	NodeBase *node, *from, *to;
	NodeConnection *con;

	for (int c=0; c<nodes.n; c++) {
		node = nodes.e[c];

		for (int c2=0; c2<node->properties.n; c2++) {
			prop = node->properties.e[c2];
			if (!prop->IsOutput()) continue;

			for (int c3=0; c3<prop->connections.n; c3++) {
				con = prop->connections.e[c3];
				from = newgroup->FindNode(con->from->Id());
				to   = newgroup->FindNode(con->to  ->Id());
				if (!from && con->from == input ) from = input;
				if (!to   && con->to   == output) to   = input;
				if (from && to) {
					fromp = from->FindProperty(con->fromprop->name);
					top   = to  ->FindProperty(con->toprop  ->name);
					if (fromp && top) newgroup->Connect(fromp, top);
				}
			}
		}
	}

	newgroup->Wrap();
	return newgroup;
}

/*! Set up new initial nodes for input and output.
 * Will flush all properties of this.
 */
void NodeGroup::InitializeBlank()
{
	Label(_("Group"));

	NodeBase *ins  = new NodeBase();
	ins->Id("Inputs");
	ins->Label(_("Inputs"));
	ins->deletable = false;
	if (colors) ins->InstallColors(colors, 0);
	NodeBase *outs = new NodeBase();
	outs->Id("Outputs");
	outs->Label(_("Outputs"));
	outs->deletable = false;
	if (colors) outs->InstallColors(colors, 0);

	ins->Wrap();
	outs->Wrap();
	outs->x = ins->x + ins->width*1.1;
	nodes.push(ins);
	nodes.push(outs);

	if (output) output->dec_count();
	output = outs;
	if (input) input->dec_count();
	input  = ins;

	properties.flush();
}

/*! Returns the new property of *this. The input's new property is prop_ret->topropproxy.
 */
NodeProperty *NodeGroup::AddGroupInput(const char *pname, const char *plabel, const char *ptooltip)
{
	if (!input) return NULL;

	NodeProperty *insprop = new NodeProperty(NodeProperty::PROP_Output, true, pname, NULL,1, plabel, ptooltip);
	input->AddProperty(insprop);

	NodeProperty *groupprop = new NodeProperty(NodeProperty::PROP_Input, true, pname, NULL,1, plabel, ptooltip);
	AddProperty(groupprop);

	groupprop->topropproxy = insprop;
	insprop->frompropproxy = groupprop;

	return groupprop;
}

/*! Returns the new property of *this. The output's new property is prop_ret->frompropproxy.
 */
NodeProperty *NodeGroup::AddGroupOutput(const char *pname, const char *plabel, const char *ptooltip)
{
	if (!output) return NULL;

	NodeProperty *outsprop = new NodeProperty(NodeProperty::PROP_Input, true, pname, NULL,1, plabel, ptooltip);
	output->AddProperty(outsprop);

	NodeProperty *groupprop = new NodeProperty(NodeProperty::PROP_Output, true, pname, NULL,1, plabel, ptooltip);
	AddProperty(groupprop);

	groupprop->frompropproxy = outsprop;
	outsprop->topropproxy = groupprop;

	return groupprop;
}

/*! Install noutput as the group's pinned output.
 * Basically just dec_count the old, inc_count the new.
 * It is assumed noutput is in the nodes stack already.
 *
 * If you pass in noutput, then it is assumed you don't want a designated output.
 */
int NodeGroup::DesignateOutput(NodeBase *noutput)
{
	 // *** needs updating to make sure no errant connections
	if (output) output->dec_count();
	output=noutput;
	if (output) output->inc_count();

	// *** connect output's properties with group output properties
	for (int c=0; c<output->properties.n; c++) {
		NodeProperty *prop = output->properties.e[c];
		NodeProperty *groupprop = FindProperty(prop->name);
		if (groupprop) {
			prop->topropproxy = groupprop;
		}
	}

	return 0;
}

int NodeGroup::DesignateInput(NodeBase *ninput)
{
	if (input) input->dec_count();
	input=ninput;
	if (input) input->inc_count();

	// *** connect output's properties with group output properties
	for (int c=0; c<input->properties.n; c++) {
		NodeProperty *prop = input->properties.e[c];
		NodeProperty *groupprop = FindProperty(prop->name);
		if (groupprop) {
			prop->frompropproxy = groupprop;
		}
	}
	return 0;
}

/*! Delete any nodes and related connections of any in selected that are node->deletable.
 * selected will be empty afterwards.
 */
int NodeGroup::DeleteNodes(Laxkit::RefPtrStack<NodeBase> &selected)
{
	int numdel = 0;
	NodeBase *node;

	for (int c=selected.n-1; c>=0; c--) {
		node = selected.e[c];
		if (!node->deletable) continue;

		for (int c2 = 0; c2 < node->properties.n; c2++) {
			while (node->properties.e[c2]->connections.n) {
				Disconnect(node->properties.e[c2]->connections.e[node->properties.e[c2]->connections.n-1], false, false);
			}
		}

		nodes.remove(nodes.findindex(node));
		selected.remove(c);
		numdel++;
	}

	return numdel;
}

//class NodeGroupProperty : public NodeProperty
//{
//  public:
//	NodeGroupProperty *connected_property;
//	NodeGroupProperty() { connected_property = NULL; }
//	virtual ~NodeGroupProperty();
//};

/*! Takes all in selected, and puts them inside a new node that's a child of this.
 * Connections are updated to reflect the new order.
 * Return the newly created node.
 * If no selected, nothing is done and NULL is returned.
 */
NodeGroup *NodeGroup::GroupNodes(Laxkit::RefPtrStack<NodeBase> &selected)
{
	//find all connected links to inputs not in group, and map to group ins
	//find all connected links from outputs not in group, map to group outs
	//after group creation, check that any external nodes are NOT connected to
	//both inputs and outputs.. sever one or the other

	if (selected.n==0) return NULL;

	NodeGroup *group = new NodeGroup;
	group->m = m;
	group->Label(_("Group"));
	group->InstallColors(colors, 0);
	NodeBase *node, *ins, *outs;
	DoubleBBox box;
	//NodeConnection *connection;

	ins  = new NodeBase();
	makestr(ins->type, "GroupInputs");
	ins->Id("Inputs");
	ins->Label(_("Inputs"));
	ins->deletable = false;
	ins->InstallColors(colors, 0);
	outs = new NodeBase();
	makestr(outs->type, "GroupOutputs");
	outs->Id("Outputs");
	outs->Label(_("Outputs"));
	outs->deletable = false;
	outs->InstallColors(colors, 0);
	group->nodes.push(ins);
	group->nodes.push(outs);
	group->output = outs;
	group->input  = ins;
	int propi = 0;

	for (int c=0; c<selected.n; c++) {
		node = selected.e[c];
		group->nodes.push(node);

		box.addtobounds(node->x, node->y);
		box.addtobounds(node->x+node->width, node->y+node->height);

		char scratch[50];

		 //check inputs
		for (int c2=0; c2<node->properties.n; c2++) {
			NodeProperty *prop = node->properties.e[c2];
			if (prop->connections.n == 0 || !prop->IsInput()) continue;

			NodeConnection *connection = prop->connections.e[0];

			if (selected.findindex(connection->from) >= 0) {
				 //another selected node is connecting to the input. Keep the connection, so nothing to do here

			} else {
				 //a non-selected node is connecting to an input of current node
				 //we need to transfer the connection to a group property, and set up a relay through ins

				NodeProperty *fromprop = connection->fromprop;

				 //remove old connection
				Disconnect(connection, true, true);


				 //install new input property on group node
				propi++;
				sprintf(scratch, "prop%d", propi);
				NodeProperty *groupprop = new NodeProperty(NodeProperty::PROP_Input, true, scratch, NULL,1, prop->label, prop->tooltip);
				group->AddProperty(groupprop);

				 //install a new output property on ins
				NodeProperty *insprop = new NodeProperty(NodeProperty::PROP_Output, true, scratch, NULL,1, prop->label, prop->tooltip);
				ins->AddProperty(insprop);

				 //set up the new connections
				group->Connect(insprop, prop);
				Connect(fromprop, groupprop);
				groupprop->topropproxy = insprop;
				insprop->frompropproxy = groupprop;


				// **** How to set up the relay??

			}
		}

		 //check outputs
		for (int c2=0; c2<node->properties.n; c2++) {
			NodeProperty *prop = node->properties.e[c2];
			if (prop->connections.n == 0 || !prop->IsOutput()) continue;

			for (int c3=0; c3<prop->connections.n; c3++) {
				NodeConnection *connection = prop->connections.e[c3];

				if (selected.findindex(connection->to) >= 0) {
					 //node connects to another selected node's input. Keep the connection.

				} else {
					 //node connects out to a non-selected node, we need to transfer the
					 //connection to a group property, and set up a relay through outs

					NodeProperty *toprop = connection->toprop;

					 //remove old connection
					Disconnect(connection, true, true);

					 //install new output property on group node
					propi++;
					sprintf(scratch, "prop%d", propi);
					NodeProperty *groupprop = new NodeProperty(NodeProperty::PROP_Output, true, scratch, NULL,1, prop->label, prop->tooltip);
					group->AddProperty(groupprop);

					 //install a new input property on outs
					NodeProperty *outsprop = new NodeProperty(NodeProperty::PROP_Input, true, scratch, NULL,1, prop->label, prop->tooltip);
					outs->AddProperty(outsprop);

					 //set up the new connections
					group->Connect(prop, outsprop);
					Connect(groupprop, toprop);
					groupprop->frompropproxy = outsprop;
					outsprop->frompropproxy = groupprop;

					// **** How to set up the relay??
				}
			}
		}
	}

	 //remove nodes from this
	for (int c=0; c<selected.n; c++) {
		nodes.remove(nodes.findindex(selected.e[c]));
	}
	selected.flush();

	group->Wrap();
	group->x = (box.minx+box.maxx)/2 - group->width /2;
	group->y = (box.miny+box.maxy)/2 - group->height/2;

	 //position ins centered to the left of the nodes, outs to the right.
	ins->Wrap();
	ins->x = box.minx - 2*ins->width;
	ins->y = (box.miny + box.maxy)/2 - ins->height/2;
	outs->Wrap();
	outs->x = box.maxx + outs->width;
	outs->y = (box.miny + box.maxy)/2 - outs->height/2;

	nodes.push(group);

	return group;
}

/*! Ungroup any groups in selected. Return number of nodes in selected, or 0.
 * If update_selected, then replace selected to contain all the ungrouped nodes.
 */
int NodeGroup::UngroupNodes(Laxkit::RefPtrStack<NodeBase> &selected, bool update_selected)
{
	Laxkit::RefPtrStack<NodeBase> nselected;

	int n=0;

	for (int c=0; c<selected.n; c++) {
		NodeGroup *group = dynamic_cast<NodeGroup*>(selected.e[c]);
		if (!group) continue;
		n++;

		for (int c2=0; c2<group->nodes.n; c2++) {
			if (group->nodes.e[c2] == group->input) {
				continue; //this node and output node will vanish when group is destructed
			} else if (group->nodes.e[c2] == group->output) {
				continue;
			} else {
				nodes.push(group->nodes.e[c2]);
				nselected.push(group->nodes.e[c2]);
			}
		}

		for (int c2=0; c2<group->input->properties.n; c2++) {
			NodeProperty *prop = group->input->properties.e[c2];
			NodeProperty *propmain = group->FindProperty(prop->name);

			 //note: assumes there is only one input
			NodeConnection *conmain = (propmain->connections.n > 0 ? propmain->connections.e[0] : NULL);
			NodeProperty *mainoutprop = (conmain ? conmain->fromprop : NULL);
			if (conmain) Disconnect(conmain, true, true);

			for (int c3=0; c3<prop->connections.n; c3++) {
				NodeConnection *con = prop->connections.e[c3];
				NodeProperty *inprop = con->toprop;
				group->Disconnect(con, true, true);
				if (mainoutprop) Connect(mainoutprop, inprop);
			}
		}

		for (int c2=0; c2<group->output->properties.n; c2++) {
			NodeProperty *prop = group->output->properties.e[c2]; //should be an input
			NodeProperty *propmain = group->FindProperty(prop->name); //should be an output

			 //note: assumes there is only one input
			NodeConnection *con     = (prop->connections.n > 0 ? prop->connections.e[0] : NULL);
			NodeProperty *subinprop   = (con ? con->fromprop : NULL);
			if (con) group->Disconnect(con, true, true);

			for (int c3=0; c3<propmain->connections.n; c3++) {
				NodeConnection *conmain = propmain->connections.e[c3];
				NodeProperty *mainoutprop = conmain->toprop;

				Disconnect(conmain, true, true);
				if (subinprop) Connect(subinprop, mainoutprop);
			}
		}
		
		nodes.remove(nodes.findindex(group)); 
	}

	 //have the ungrouped nodes be selected
	selected.flush();
	if (update_selected) for (int c=0; c<nselected.n; c++) selected.push(nselected.e[0]);

	return n;
}

/*!
 * Create and install a new NodeConnection.
 * Return the new connection, or NULL for couldn't connect.
 * fromprop and toprop should have everything set properly.
 */
NodeConnection *NodeGroup::Connect(NodeProperty *fromprop, NodeProperty *toprop)
{
	if (!fromprop || !toprop) return NULL;

	NodeBase *from = fromprop->owner;
	NodeBase *to   = toprop->owner;

	NodeConnection *newcon = new NodeConnection(from, to, fromprop, toprop);

	return Connect(newcon, 1);
//
//	fromprop->AddConnection(newcon, 0);
//	toprop  ->AddConnection(newcon, 1);
//
//	from->Connected(newcon);
//	to  ->Connected(newcon);
//
//	return newcon;
}

/*! Lower level Connect() for an already constructed NodeConnection.
 * It is assumed that newcon is already set to reasonable things,
 * but the properties do not have the connection in their list.
 *
 * Calls Connected() on the nodes.
 *
 * Does not Update() anything.
 */
NodeConnection *NodeGroup::Connect(NodeConnection *newcon, int absorb)
{
	if (!newcon) return NULL;

	newcon->fromprop->AddConnection(newcon, 0);
	newcon->toprop  ->AddConnection(newcon, absorb);

	newcon->from->Connected(newcon);
	newcon->to  ->Connected(newcon);

	return newcon;
}


/*! Append connection to connections list.
 * ONLY affects *this, NOT the property on the other end.
 * connection needs to be set properly before calling this.
 */
int NodeProperty::AddConnection(NodeConnection *connection, int absorb)
{
	connections.push(connection);
	if (absorb) connection->dec_count();
	return 0;
}

/*! Remove connection from connections list.
 * ONLY affects *this, NOT the property on the other end.
 */
int NodeProperty::RemoveConnection(NodeConnection *connection)
{
	if (connection->fromprop == this) {
		connection->from = NULL;
		connection->fromprop = NULL;
	}
	connections.remove(connection);
	return 0;
}


/*! Remove the connection from the associated properties.
 * Call Disconnected() on the affected nodes.
 * If will_be_replaced, then pass this hint to NodeBase::Disconnected() so
 * that it won't try to drastically restructure if it doesn't have to.
 */
int NodeGroup::Disconnect(NodeConnection *connection, bool from_will_be_replaced, bool to_will_be_replaced)
{
	connection->to      ->Disconnected(connection, from_will_be_replaced, to_will_be_replaced);
	connection->from    ->Disconnected(connection, from_will_be_replaced, to_will_be_replaced);

	 //these might have been removed during Disconnected() above
	if (connection->fromprop) connection->fromprop->RemoveConnection(connection);
	if (connection->toprop)   connection->toprop  ->RemoveConnection(connection);
	return 0;
}

/*! Move things around so there is no overlap.
 * If which==NULL then shift so no overlap anywhere.
 * If which != NULL, then only shift so that one is not overlapped with anything else.
 */
int NodeGroup::NoOverlap(NodeBase *which, double gap)
{
	int num_adjusted = 0;

	DoubleBBox box, box2;
	NodeBase *node, *node2;

	for (int c=0; c<nodes.n; c++) {
		if (which && nodes.e[c] != which) continue;
		node = nodes.e[c];
		box.setbounds(*node);
		box.ExpandBounds(gap);

		for (int c2=c+1; c2<nodes.n; c2++) {
			node2 = nodes.e[c2];
			box2.setbounds(*node2);
			if (!box.intersect(&box2)) continue;

			 //found an overlap

			int dir = 0;
			for (int c2=0; c2<node->properties.n; c2++) {
				for (int c3=0; c3<node->properties.e[c2]->connections.n; c3++) {
					if (node->properties.e[c2]->IsOutput() && CheckForward(node2, node->properties.e[c2]->connections.e[c3])) 
						dir = 1;
					else if (node->properties.e[c2]->IsInput() && CheckBackward(node2, node->properties.e[c2]->connections.e[c3]))
						dir = 1;
					if (dir) break;
				}
			}

			if (!dir) {
				 //don't seem to have direct connection, push up or down
				if (node2->y+node2->height/2 < node->y+node->height/2) dir = 2;
				else dir = -2;
			}

			if (dir == 1) node2->x = node->x + node->width + gap;
			else if (dir == -1) node2->x = node->x - gap - node2->width;
			else if (dir ==  2) node2->y = node->y - gap - node->height;
			else if (dir == -2) node2->y = node->y + node->height + gap;

			NoOverlap(node2, gap);

			num_adjusted++;
		}
	}

	return num_adjusted;
}

/*! Compute the bounding box containing all the sub nodes.
 */
void NodeGroup::BoundingBox(DoubleBBox &box)
{
	box.clear();
	for (int c=0; c<nodes.n; c++) {
		box.addtobounds(nodes.e[c]->x,nodes.e[c]->y);
		box.addtobounds(nodes.e[c]->x+nodes.e[c]->width, nodes.e[c]->y+nodes.e[c]->height);
	}
}

/*! Use when connecting forward to node via connection, to see if there are occurences of node.
 * It is assumed that there are NO cyclic loops involving any other nodes.
 * Also assumed that connection->from != node.
 *
 * If node == connection.to, then return 1. Next recurse through each property of
 * connection.to.
 *
 * Return 0 if not found, or 1 for found.
 */
int NodeGroup::CheckForward(NodeBase *node, NodeConnection *connection)
{
	NodeBase *check = connection->to;
	if (!check) return 0;

	NodeConnection *conn;
	NodeProperty *prop;

	if (check == node) return 1;
	for (int c=0; c<check->properties.n; c++) {
		prop = check->properties.e[c];
		if (prop->IsInput()) continue;

		for (int c2=0; c2<prop->connections.n; c2++) {
			conn = prop->connections.e[c2];
			if (CheckForward(node, conn)) return 1;
		}
	}

	return 0;
}

/*! Use when connecting backward to node via connection. Traverse backwards through connection,
 * and node should not be found. Return 0 if not found, or 1 for found.
 * The opposite of CheckForward.
 */
int NodeGroup::CheckBackward(NodeBase *node, NodeConnection *connection)
{
	NodeBase *check = connection->from;
	if (!check) return 0;

	NodeConnection *conn;
	NodeProperty *prop;

	if (check == node) return 1;
	for (int c=0; c<check->properties.n; c++) {
		prop = check->properties.e[c];
		if (prop->IsOutput()) continue;

		for (int c2=0; c2<prop->connections.n; c2++) {
			conn = prop->connections.e[c2];
			if (CheckBackward(node, conn)) return 1;
		}
	}

	return 0;
}

void NodeGroup::dump_out(FILE *f,int indent,int what,DumpContext *context)
{
    Attribute att;
    dump_out_atts(&att,what,context);
    att.dump_out(f,indent);
}

Attribute *NodeGroup::dump_out_atts(Attribute *att,int what,DumpContext *context)
{
   if (!att) att=new Attribute();

    if (what==-1) {
        att->push("id", "some_name");
        att->push("label", "Displayed label");
        att->push("matrix", "screen matrix to use");
        att->push("background", "rgb(.1,.2,.3) #color of the background for this group of nodes");
        att->push("output", "which_one #id of the node designated as non-deletable output for this group, if any");
        att->push("nodes", "#list of individual nodes in this group");
        att->push("connections", "#list of connections between the nodes");
        //att->push("", "");
        return att;
    }

	NodeBase::dump_out_atts(att,what,context);
//	att->push("id", Id()); 
//	att2->push("label", Label());
//
//	const double *matrix=m.m();
//	char s[200];
//	sprintf(s,"%.10g %.10g %.10g %.10g %.10g %.10g",
//            matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
//	att->push("matrix", s);


	if (output) att->push("output", output->Id());
	if (input)  att->push("input",  input ->Id());


	PtrStack<NodeConnection> connections;

	Attribute *att2, *att3;
	for (int c=0; c<nodes.n; c++) {
		NodeBase *node = nodes.e[c];
		
		att2 = att->pushSubAtt("node", node->Type());
		node->dump_out_atts(att2, what, context);

		for (int c2=0; c2<node->properties.n; c2++) {
			for (int c3=0; c3<node->properties.e[c2]->connections.n; c3++) {
				connections.pushnodup(node->properties.e[c2]->connections.e[c3], 0);
			}
		}
	}

	att2 = att->pushSubAtt("connections");
	for (int c=0; c<connections.n; c++) {
		 //"%s,%s -> %s,%s", 
		string str;
		str = str + connections.e[c]->from->Id() + "," + connections.e[c]->fromprop->name
					 + " -> " + connections.e[c]->to  ->Id() + "," + connections.e[c]->toprop  ->name;

		att2->push("connect", str.c_str());
	}

	att2 = att->pushSubAtt("frames");
	for (int c=0; c<frames.n; c++) {
		att3 = att2->pushSubAtt("frame", frames.e[c]->Label());
		if (frames.e[c]->comment) att3->push("comment", frames.e[c]->comment);
		for (int c2=0; c2<frames.e[c]->nodes.n; c2++) {
			att3->push("node", frames.e[c]->nodes.e[c2]->Id());
		}

	}

	return att;
}

void NodeGroup::dump_in_atts(Attribute *att,int flag,DumpContext *context)
{
	if (!att) return;

    char *name,*value;
	const char *out=NULL;
	const char *in =NULL;
	Attribute *conatt=NULL;
	Attribute *framesatt=NULL;

	NodeBase::dump_in_atts(att, flag, context);

    for (int c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

        if (!strcmp(name,"output")) {
			out = value;

        } else if (!strcmp(name,"input")) {
			in = value;

        } else if (!strcmp(name,"node")) {
			 //value is the node type
			if (isblank(value)) {
				if (context->log) context->log->AddMessage(_("Node missing a type, skipping!"), ERROR_Warning);
				continue;
			}

			NodeBase *newnode;
			if (!strcmp(value, "GroupInputs") || !strcmp(value, "GroupOutputs")) {
				newnode = new NodeBase();
				newnode->deletable = false;
				makestr(newnode->type, value);
				
			} else newnode = NewNode(value);

			if (!newnode) {
				char errormsg[200];
				sprintf(errormsg,_("Unknown node type: %s"), value);
				cerr << errormsg <<endl;
				context->log->AddMessage(object_id, Id(), NULL, errormsg, ERROR_Warning);
				continue;
			}

			newnode->dump_in_atts(att->attributes.e[c], flag, context);

			if (!newnode->colors && colors) newnode->InstallColors(colors, false);
			newnode->Wrap();
			nodes.push(newnode);
			newnode->dec_count();

        } else if (!strcmp(name,"connections")) {
			conatt = att->attributes.e[c];

        } else if (!strcmp(name,"frames")) {
			framesatt = att->attributes.e[c];
		}
	} 

	if (out) {
		//set designated output
		NodeBase *node = FindNode(out);
		if (node) DesignateOutput(node);
	}

	if (in) {
		//set designated output
		NodeBase *node = FindNode(in);
		if (node) DesignateInput(node);
	}


	if (conatt) {
		 //define connections
		for (int c=0; c<conatt->attributes.n; c++) {
			name= conatt->attributes.e[c]->name;
			value=conatt->attributes.e[c]->value;

        	if (!strcmp(name,"connect")) {
				if (!value) continue;

				const char *div = strstr(value, " -> ");
				if (!div) continue;
				const char *comma  = strchr(value, ',');
				const char *comma2 = strchr(div+4, ',');
				if (!comma || comma>div || !comma2) continue;

				char *fromstr = newnstr(value, comma-value);
				char *fpstr   = newnstr(comma+1, div-comma-1);
				char *tostr   = newnstr(div+4,comma2-(div+4));
				char *tpstr   = newnstr(comma2+1, value+strlen(value)-comma2);

				NodeBase *from = FindNode(fromstr),
						 *to   = FindNode(tostr);

				if (from && to) {
					NodeProperty *fromprop = from->FindProperty(fpstr),
								 *toprop   = to  ->FindProperty(tpstr);

					if (!from || !to || !fromprop || !toprop) {
						// *** warning!
						DBG cerr <<" *** bad node att input!"<<endl;
						//continue;

					} else {
						 //install the connection
						NodeConnection *newcon = new NodeConnection(from, to, fromprop, toprop);
						fromprop->AddConnection(newcon, 0);
						toprop  ->AddConnection(newcon, 1);

						from->Connected(newcon);
						to  ->Connected(newcon);
					}
					
				} else {
					DBG cerr <<" *** Warning! cannot connect "<<fromstr<<" to "<<tostr<<"!"<<endl;
				}

				delete[] fromstr;
				delete[] tostr;
				delete[] fpstr;
				delete[] tpstr;
			}
		}
	}

	if (framesatt) {
		 //build the frames

		for (int c=0; c<framesatt->attributes.n; c++) {
			name= framesatt->attributes.e[c]->name;
			value=framesatt->attributes.e[c]->value;

        	if (!strcmp(name,"frame")) {
				NodeFrame *frame = new NodeFrame(this, value);
				frames.push(frame);
				frame->dec_count();

				for (int c2=0; c2<framesatt->attributes.n; c2++) {
					name= framesatt->attributes.e[c]->attributes.e[c2]->name;
					value=framesatt->attributes.e[c]->attributes.e[c2]->value;

        			if (!strcmp(name,"comment")) {
						frame->Label(value);

					} else if (!strcmp(name,"node")) {
						NodeBase *node = FindNode(value);
						if (node) frame->AddNode(node);
					}
				}

				frame->Wrap();
			}
		}
	}

	for (int c=0; c<nodes.n; c++) {
		nodes.e[c]->Update();
	}
}

/*! Recursively install colors on any that doesn't have one.
 */
int NodeGroup::InstallColors(NodeColors *newcolors, bool absorb_count)
{
	NodeBase::InstallColors(newcolors, absorb_count);

	for (int c=0; c<nodes.n; c++) {
		if (!nodes.e[c]->colors) {
			nodes.e[c]->InstallColors(newcolors, false);
			if (nodes.e[c]->width <= 0) nodes.e[c]->Wrap();
			else nodes.e[c]->UpdateLayout();
			nodes.e[c]->UpdateLinkPositions();
		}
	}

	UpdateLinkPositions();

	return 0;
}

/*! Default is to just nodes.push(node).
 */
int NodeGroup::AddNode(NodeBase *node)
{
	return nodes.push(node);
}

/*! Create and return a new fresh node object, unconnected to anything.
 */
NodeBase *NodeGroup::NewNode(const char *type)
{
//	if (!strcmp(type, "group")) {
//		return new NodeGroup;
//	}

	ObjectFactory *factory = NodeFactory();
	ObjectFactoryNode *fnode;

	for (int c=0; c < factory->types.n; c++) {
		fnode = factory->types.e[c];

		if (!strcmp(fnode->name, type)) {
			anObject *obj = fnode->newfunc(fnode->parameter, NULL);
			NodeBase *newnode = dynamic_cast<NodeBase*>(obj);

			if (obj && !newnode) {
				DBG cerr << " *** uh oh! factory returned bad object in NodeGroup::NewNode()!"<<endl;
				return NULL;
			} 
			return newnode;
		}
	}

	return NULL;
}

/*! Return node at index in nodes, or NULL if doesn't exist.
 */
NodeBase *NodeGroup::GetNode(int index)
{
	if (index < 0 || index >= nodes.n) return NULL;
	return nodes.e[index];
}

NodeBase *NodeGroup::FindNode(const char *name)
{
	if (!name) return NULL;

	for (int c=0; c<nodes.n; c++) {
		if (!strcmp(name, nodes.e[c]->Id())) return nodes.e[c];
	}
	return NULL;
}

/*! Return frame at index in frame, or NULL if doesn't exist.
 */
NodeFrame *NodeGroup::GetFrame(int index)
{
	if (index < 0 || index >= frames.n) return NULL;
	return frames.e[index];
}




//-------------------------------------- NodeInterface --------------------------

/*! \class NodeInterface
 * \ingroup interfaces
 */


NodeInterface::NodeInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	node_interface_style=0;

	node_factory = NodeGroup::NodeFactory(true);
	node_factory->inc_count();


	nodes          = NULL;
	node_menu      = NULL;
	lasthover      = -1;
	lasthoverslot  = -1;
	lasthoverprop  = -1;
	lastconnection = -1;
	lastmenuindex  = -1;
	tempconnection = NULL;
	onconnection   = NULL;
	hover_action   = NODES_None;
	show_threads   = false;
	needtodraw     = 1;
	lastsave       = newstr("some.nodes");
	font           = app->defaultlaxfont;
	font->inc_count();
	defaultpreviewsize = 50; //pixels

	search_term   = NULL;
	last_search_index = -1;
	//search group contents
	//search: labels, var names
	//regex search

	pan_timer    = 0;
	pan_duration = 2;
	pan_current  = 0;
	pan_tick_ms  = 1./60 * 1000; //1/60th second ticks for panning

	 //play settings:
	playing      = 0;
	play_timer   = 0;
	play_fps     = 60;
	cur_fps      = 0;
	elapsed_time = 0;
	last_time    = 0;

	//color_controls.rgbf(.7,.5,.7,1.);
	color_controls.rgbf(.8,.8,.8,1.);
	color_background.rgbf(0,0,0,.5);
	color_grid.rgbf(0,0,0,.7);
	draw_grid = 50;

	viewport_bounds.setbounds(0.,1.,0.,1.); // viewport_bounds is always fraction of actual viewport bounds
	vp_dragpad = 40; //screen pixels

	sc = NULL; //shortcut list, define as needed in GetShortcuts()
}

NodeInterface::~NodeInterface()
{
	if (nodes) nodes->dec_count();
	if (node_menu) node_menu->dec_count();
	if (font) font->dec_count();
	if (node_factory) node_factory->dec_count();
	if (sc) sc->dec_count();
	if (tempconnection) tempconnection->dec_count();
	if (onconnection) onconnection->dec_count();
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


int NodeInterface::Idle(int tid, double delta)
{
	if (tid == pan_timer) {
		if (!nodes) return 1;

		DBG cerr <<"NodeInterface::Idle()... cur="<<pan_current<<endl;

		// advance pan
		// pan_current += delta/duration;
		//flatpoint oldpos = nodes->m.transformPointInverse(lastpos);

		double t = .5+.5*(-cos(pan_current/pan_duration*M_PI));
		flatpoint oldpos = bez_point(t, panpath[0], panpath[1], panpath[2], panpath[3]);
		pan_current += pan_tick_ms/1000.;
		t = .5+.5*(-cos(pan_current/pan_duration*M_PI));
		flatpoint newpos = bez_point(t, panpath[0], panpath[1], panpath[2], panpath[3]);

		newpos = nodes->m.transformPoint(newpos);
		oldpos = nodes->m.transformPoint(oldpos);
		flatpoint d = newpos - oldpos;
		nodes->m.Translate(-d);

		needtodraw=1;
		if (pan_current >= pan_duration) {
			pan_timer = 0;
			return 1;
		}

		return 0;

	} else if (tid == play_timer) {
		// advance threads by one node;
		ExecuteThreads();

		if (delta) cur_fps = 1/delta;

		if (!threads.n) {
			PostMessage(_("Threads done!"));
			play_timer = 0;
			playing = -1;
			//don't want to call stop, ...not sure if it will mess up timer stack. *** should find that out!!
			return 1;
		}
		return 0;
	}

	return 1;
}

/*! Do one iteration of thread execution.
 * Return 1 for threads done, or 0 for still things to do.
 */
int NodeInterface::ExecuteThreads()
{
	// advance threads by one node;
	NodeThread *thread;

	if (threads.n) needtodraw=1;

	for (int c=threads.n-1; c>=0; c--) {
		thread = threads.e[c];

		NodeBase *next = thread->next->Execute(thread);
		if (next) {
			//node has given us somewhere to go to
			thread->UpdateThread(next, NULL);

		} else {
			//done!
			if (thread->scopes.n) {
				thread->UpdateThread(thread->scopes.e[thread->scopes.n-1], NULL);
				thread->scopes.remove(-1);
			} else {
				threads.remove(c);
			}
		}
	}

	if (!threads.n) {
		return 1;
	}

	return 0;
}

/*! Return whether node is a current node in a thread.
 */
int NodeInterface::IsThread(NodeBase *node)
{
	for (int c=0; c<threads.n; c++) {
		if (threads.e[c]->next == node) return c+1;
	}
	return 0;
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
	needtodraw=1;
	return 0;
}

/*! Any cleanup when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int NodeInterface::InterfaceOff()
{ 
	Clear(NULL);
	needtodraw=1;
	return 0;
	
	// *** need to clear any unattached connections
}

void NodeInterface::Clear(SomeData *d)
{
	selected.flush();
	grouptree.flush();

	if (play_timer) {
		// just remove timer
		app->removetimer(this, play_timer);
		play_timer = 0;
	}
}

/*! To aid custom node actions from plugins, for instance.
 * *** put this somewhere responsible if used!!!
 */
class MenuAction
{
  public:
    unsigned int id;

    MenuAction();
	virtual ~MenuAction();
	virtual ObjectDef *Inputs() = 0;
	virtual const char *MenuText() = 0;
};

int NodeInterface::InitializeResources()
{
	InterfaceManager *imanager = InterfaceManager::GetDefault(true);

	ResourceManager *rmanager = imanager->GetResourceManager();
	rmanager->AddResourceType("Nodes", _("Nodes"), NULL, NULL);

	ObjectFactory *factory = imanager->GetObjectFactory();
	factory->DefineNewObject(getUniqueNumber(), "NodeGroup", newNodeGroup, NULL, 0);

	return 0;
}

Laxkit::MenuInfo *NodeInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	//if (no menu for x,y) return NULL;

	if (!menu) menu=new MenuInfo;
	if (!menu->n()) menu->AddSep(_("Nodes"));

	menu->AddItem(_("Add node..."), NODES_Add_Node);

	if (selected.n) {
		menu->AddItem(_("Show previews"), NODES_Show_Previews);
		menu->AddItem(_("Hide previews"), NODES_Hide_Previews);
	}


	menu->AddSep();
	menu->AddItem(_("Save nodes..."), NODES_Save_Nodes);
	menu->AddItem(_("Load nodes..."), NODES_Load_Nodes);

	if (NodeGroup::loaders.n) {
		menu->AddSep();
		char scratch[500];
		for (int c=0; c<NodeGroup::loaders.n; c++) {
			if (NodeGroup::loaders.e[c]->CanImport(NULL, NULL)) {
				sprintf(scratch, _("Load with %s..."), NodeGroup::loaders.e[c]->VersionName());
				menu->AddItem(scratch, NODES_Load_With_Loader, LAX_OFF, c);
			}
		}
		if (nodes) {
			for (int c=0; c<NodeGroup::loaders.n; c++) {
				if (NodeGroup::loaders.e[c]->CanExport(NULL)) {
					sprintf(scratch, _("Save with %s..."), NodeGroup::loaders.e[c]->VersionName());
					menu->AddItem(scratch, NODES_Save_With_Loader, LAX_OFF, c);
				}
			}
		}
	}

	//-------- get list of available node resources
	ResourceManager *manager = InterfaceManager::GetDefault(true)->GetResourceManager();
	if (manager->NumResources("Nodes")) {
		menu->AddSep();
		menu->AddItem(_("Resources"));
		menu->SubMenu(_("Saved Nodes"));
		manager->ResourceMenu("Nodes", true, menu);
		menu->EndSubMenu();
	}

	//menu->AddSep();
	//menu->AddItem(_("Save as resource..."), NODES_Save_As_Resource);
	//menu->AddItem(_("Load resource..."), NODES_Load_Resource);

	menu->AddSep();
	menu->AddItem(_("Show thread controls"), NODES_Thread_Controls);

	return menu;
}

int NodeInterface::Event(const Laxkit::EventData *data, const char *mes)
{
    if (!strcmp(mes,"menuevent")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
        int action = s->info2; //id of menu item
		int info   = s->info4;

        if (   action == NODES_Add_Node
			|| action == NODES_Save_Nodes
			|| action == NODES_Load_Nodes
			|| action == NODES_Show_Previews
			|| action == NODES_Hide_Previews
			|| action == NODES_Thread_Controls
			) {
			PerformAction(action);

		} else if (action == NODES_Load_With_Loader || action == NODES_Save_With_Loader) {
			lastmenuindex = info;
			PerformAction(action);

		} else {
			 //maybe a resource
			ResourceManager *manager = InterfaceManager::GetDefault(true)->GetResourceManager();
			NodeGroup *group = dynamic_cast<NodeGroup*>(manager->FindResource(s->str, "Nodes"));
			if (group) {
				if (nodes) nodes->dec_count();
				selected.flush();
				grouptree.flush();
				nodes = group;
				nodes->inc_count();
				if (!nodes->colors) {
					NodeColors *colors = new NodeColors;
					colors->Font(font, false);
					nodes->InstallColors(colors, true);
				}
				lasthover = lasthoverslot = lasthoverprop = lastconnection = -1;
				needtodraw=1;
			}
		}

		return 0;

	} else if (!strcmp(mes,"setpropdouble") || !strcmp(mes,"setpropint") || !strcmp(mes,"setpropstring")) {
		if (!nodes || lasthover<0 || lasthover>=nodes->nodes.n || lasthoverprop<0
				|| lasthoverprop>=nodes->nodes.e[lasthover]->properties.n) return 0;

        const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(data);

		NodeBase *node = nodes->nodes.e[lasthover];
		NodeProperty *prop = node->properties.e[lasthoverprop];

		const char *str = s->str;
		char *sstr = NULL;
		double d = 0;
		LineEdit *e = dynamic_cast<LineEdit*>(viewport->GetInputBox());
		
		if (s->info1 == -1) {
			 //was control key, either tab, up, or down.
			if (!e) return 0;
			int ch = s->info2;
			int state = s->info3;

			int pi = lasthoverprop;

			Value *v;
			if ((ch=='\t' && (state&ShiftMask)!=0) || ch==LAX_Up) {
				while((pi-1)%node->properties.n != lasthoverprop) {
					pi--;
					if (pi<0) pi = node->properties.n-1;
					if (node->properties.e[pi]->IsEditable()) {
						v = node->properties.e[pi]->GetData();
						if (v->type()!=VALUE_Real && v->type()!=VALUE_Int && v->type()!=VALUE_String) continue;
						break;
					}
				}
				
			} else {
				while((pi+1)%node->properties.n != lasthoverprop) {
					pi++;
					if (pi>=node->properties.n) pi = 0;
					if (node->properties.e[pi]->IsEditable()) {
						v = node->properties.e[pi]->GetData();
						if (v->type()!=VALUE_Real && v->type()!=VALUE_Int && v->type()!=VALUE_String) continue;
						break;
					}
				}
			}

			if (pi != lasthoverprop) {
				if (e) {
					sstr = e->GetText();
					str = sstr;
				}
				lasthoverprop = pi;
				viewport->ClearInputBox();
				EditProperty(lasthover, lasthoverprop);

			} else return 0;
		}

		 //parse the new data
		if (isblank(str) && strcmp(mes,"setpropstring")) { delete[] sstr; return  0; }
		char *endptr=NULL;
		d = strtod(str, &endptr);
		if (endptr==str && strcmp(mes,"setpropstring")) {
			PostMessage(_("Bad value."));
			delete[] sstr;
			return 0;
		}

		 //update node data
		if (!strcmp(mes,"setpropdouble")) {
			DoubleValue *v=dynamic_cast<DoubleValue*>(prop->data);
			v->d = d;
		} else if (!strcmp(mes,"setpropstring")) {
			StringValue *v=dynamic_cast<StringValue*>(prop->data);
			v->Set(str);
		} else {
			IntValue *v=dynamic_cast<IntValue*>(prop->data);
			v->i = d;
		}
		node->Update();
		needtodraw=1;

		delete[] sstr;
		return 0;

	} else if (!strcmp(mes,"newcolor")) {
		if (!nodes || lasthover<0 || lasthover>=nodes->nodes.n || lasthoverprop<0
				|| lasthoverprop>=nodes->nodes.e[lasthover]->properties.n) return 0;

		const SimpleColorEventData *ce=dynamic_cast<const SimpleColorEventData *>(data);
		if (!ce) return 0;
		if (ce->colorsystem != LAX_COLOR_RGB) {
			PostMessage(_("Color has to be rgb currently."));
			return 0;
		}
	
		double mx=ce->max;
        double cc[5];
        for (int c=0; c<5; c++) cc[c]=ce->channels[c]/mx;

		NodeBase *node = nodes->nodes.e[lasthover];
		NodeProperty *prop = node->properties.e[lasthoverprop];
		ColorValue *color = dynamic_cast<ColorValue*>(prop->GetData()); 
		color->color.Set(ce->colorsystem, cc[0],cc[1],cc[2],cc[3],cc[4]);

		node->Update();
		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"selectenum")) {
		if (!nodes || lasthover<0 || lasthoverprop<0) return 0;
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);

		const char *what = s->str;
		if (isblank(what)) return 0;

		NodeBase *node = nodes->nodes.e[lasthover];
		NodeProperty *prop = node->properties.e[lasthoverprop];
		if (prop->IsOutput()) return 0; //don't change outputs
		if (prop->IsInput() && prop->IsConnected()) return 0; //can't change if piped in from elsewhere

		EnumValue *ev = dynamic_cast<EnumValue*>(prop->data);
		ObjectDef *def=ev->GetObjectDef();

		const char *nm, *Nm;
		for (int c=0; c<def->getNumEnumFields(); c++) {
			def->getEnumInfo(c, &nm, &Nm); //grabs id == name in the def
			if (!Nm) Nm = nm;
			if (Nm && !strcmp(Nm, what)) {
				ev->value = c; //note: makes enum value the index of the enumval def, not the id
				break;
			}
		}

		node->Update();
		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"addnode")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);

		const char *what = s->str;
		if (isblank(what)) return 0;

		if (!nodes) FreshNodes(true);

		ObjectFactoryNode *type;
		for (int c=0; c<node_factory->types.n; c++) {
			type = node_factory->types.e[c];

			if (!strcmp(type->name, what)) {
				anObject *obj = type->newfunc(type->parameter, NULL);
				NodeBase *newnode = dynamic_cast<NodeBase*>(obj);
				flatpoint p = nodes->m.transformPointInverse(lastpos);
				newnode->x  = p.x;
				newnode->y  = p.y;
				newnode->InstallColors(nodes->colors, false);
				newnode->Wrap();

				if (!strcmp(newnode->whattype(), "NodeGroup")) {
					 //fresh group has to subnodes, ins and outs, need to install colors
					NodeGroup *g = dynamic_cast<NodeGroup*>(newnode);
					g->InitializeBlank();
				}

				nodes->nodes.push(newnode);
				newnode->dec_count();

				selected.flush();
				selected.push(newnode);

				needtodraw=1; 
				break;
			}
		}

		return 0;

	} else if (!strcmp(mes,"loadnodes")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		if (isblank(s->str)) return 0;

		if (LoadNodes(s->str, false) == 0) {
			 //success! select all new nodes
			makestr(lastsave, s->str);
		}
		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"savenodes")) {
		DBG cerr <<"save nodes..."<<endl;
		if (!nodes) return 0;

        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		if (isblank(s->str)) return 0;

		SaveNodes(s->str);
		makestr(lastsave, s->str);
		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"search")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		if (isblank(s->str)) return 0;

		Find(s->str);
		return 0;

	} else if (!strcmp(mes,"savewithloader")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		if (isblank(s->str) || !nodes) return 0;

		ObjectIO *loader = (lastmenuindex >=0 && lastmenuindex < NodeGroup::loaders.n ? NodeGroup::loaders.e[lastmenuindex] : NULL);
		if (!loader) {
			PostMessage(_("Could not find loader."));
			return 0;
		}

		ErrorLog log;
		NodeExportContext context(&selected);
		int status = loader->Export(s->str, nodes, &context, log); //ret # of failing errors
		if (status) {
			 //there were errors
			NotifyGeneralErrors(&log);
		} else PostMessage(_("Exported."));

		return 0;

	} else if (!strcmp(mes,"loadwithloader")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		if (isblank(s->str)) return 0;

		ObjectIO *loader = (lastmenuindex >=0 && lastmenuindex < NodeGroup::loaders.n ? NodeGroup::loaders.e[lastmenuindex] : NULL);
		if (!loader) {
			PostMessage(_("Could not find loader."));
			return 0;
		}

		if (!nodes) FreshNodes(true);

		int oldn = nodes->nodes.n;

		anObject *obj_ret = NULL;
		ErrorLog log;
		int status = loader->Import(s->str, &obj_ret, nodes, log);
		NodeGroup *group = dynamic_cast<NodeGroup*>(obj_ret);

		if (status) {
			 //there were errors
			NotifyGeneralErrors(&log);
			if (obj_ret && obj_ret != nodes) group->dec_count();

		} else {
			for (int c=oldn; c < nodes->nodes.n; c++) {
				if (!nodes->nodes.e[c]->colors) {
					nodes->nodes.e[c]->InstallColors(nodes->colors, false);
				}

				if (nodes->nodes.e[c]->width <= 0) nodes->nodes.e[c]->Wrap();
			}
			//else  should be all done!

			PostMessage(_("Loaded."));
		}

		makestr(lastsave, s->str);
		needtodraw=1;
		return 0;

	} else if (!strcmp(mes, "setNodeLabel")) {
		NodeBase *node = nodes->GetNode(lasthover);
		if (!node) return 0;

        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		if (isblank(s->str)) return 0;
		node->Label(s->str);

		needtodraw=1;
		return 0;

	} else if (!strcmp(mes, "setFrameLabel")) {
		NodeFrame *frame = nodes->GetFrame(lasthoverslot);
		if (!frame) return 0;

        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		if (isblank(s->str)) return 0;
		frame->Label(s->str);

		needtodraw=1;
		return 0;
	}

	return 1; //event not absorbed
}

int NodeInterface::Find(const char *what)
{
	if (isblank(what)) return -1;
	makestr(search_term, what);
	last_search_index = -1;
	return FindNext();
}

/*! Search from last_search_index + 1, wrap around at end if no match.
 * Centers on found node.
 * Return node index of found, or -1.
 */
int NodeInterface::FindNext()
{
	if (isblank(search_term)) return -1;

	if (last_search_index >= nodes->nodes.n) last_search_index = -1;

	for (int c=last_search_index+1; c<nodes->nodes.n; c++) {
		if (strcasestr(nodes->nodes.e[c]->Label(), search_term)) {
			//found!
			selected.flush();
			selected.push(nodes->nodes.e[c]);
			PerformAction(NODES_Center_Selected);
			last_search_index = c;
			return c;
		}
	}

	 //wrap around
	for (int c=0; c<last_search_index; c++) {
		if (strcasestr(nodes->nodes.e[c]->Label(), search_term)) {
			//found!
			selected.flush();
			selected.push(nodes->nodes.e[c]);
			PerformAction(NODES_Center_Selected);
			last_search_index = c;
			return c;
		}
	}

	return -1;
}

void NodeInterface::RebuildNodeMenu()
{
	if (node_menu) node_menu->dec_count();
	node_menu = new MenuInfo;

	ObjectFactoryNode *type;
	for (int c=0; c<node_factory->types.n; c++) {
		type = node_factory->types.e[c];
		node_menu->AddDelimited(type->name); //, '/', 0, c);
	}
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

	DBG cerr <<" NodeInterface::Refresh()"<<endl;

	int overnode=-1, overslot=-1, overprop=-1, overconn=-1; 
	if (buttondown.any(0,LEFTBUTTON)) {
		int device = buttondown.whichdown(0,LEFTBUTTON);
		int x,y;
		buttondown.getlast(device,LEFTBUTTON, &x,&y);
		overnode = scan(x,y, &overslot, &overprop, &overconn, 0);
	} else {
		overnode = lasthover;
		overprop = lasthoverprop;
		overslot = lasthoverslot;
		overconn = lastconnection;
	}

	dp->PushAxes();

	 //draw background overlay
	ScreenColor *bg = &color_background;
	if (nodes) bg = &nodes->background;
	if (bg->Alpha()>0) {
		dp->NewTransform(1,0,0,1,0,0);
		dp->NewFG(bg);

		double vpw = dp->Maxx-dp->Minx, vph = dp->Maxy-dp->Miny;
		dp->drawrectangle(viewport_bounds.minx*vpw, viewport_bounds.miny*vph,
					viewport_bounds.boxwidth()*vpw, viewport_bounds.boxheight()*vph, 1);

		//if (draw_grid) {
		//}

		//if (lasthover == NODES_VP_Top || lasthover == NODES_VP_Top_Left || lasthover == NODES_VP_Top_Right) {
		//} else if (lasthover == NODES_VP_Bottom || lasthover == NODES_VP_Bottom_Left || lasthover == NODES_VP_Bottom_Right) {
		//} else if (lasthover == NODES_VP_Left || lasthover == NODES_VP_Top_Left || lasthover == NODES_VP_Bottom_Left) {
		//} else if (lasthover == NODES_VP_Right || lasthover == NODES_VP_Top_Right || lasthover == NODES_VP_Bottom_Right) {
		//} else if (lasthover == NODES_VP_Move) {
		//} else if (lasthover == NODES_VP_Maximize) {
		//} else if (lasthover == NODES_VP_Close) {
		//}
	}


	if (!nodes) {
		dp->PopAxes();
		return 0;
	} 

	dp->font(nodes->colors->font);
	double th = dp->textheight();


	if (show_threads) {
		dp->NewFG(coloravg(nodes->colors->fg.Pixel(),nodes->colors->bg.Pixel(), .25));

		double width = 0;
		thread_controls.maxy = thread_controls.miny + thread_controls.miny + 2*th;

		if (playing<=0 && threads.n) {
			 //  "Next   Run   Reset"
			const char *msg = _("Next");
			double x = thread_controls.minx + th;
			dp->NewBG(coloravg(nodes->colors->fg.Pixel(),nodes->colors->bg.Pixel(), lasthoverprop == NODES_Thread_Next ? .65 : .75));
			width = dp->textextent(msg,-1, NULL,NULL);
			dp->drawrectangle(x-th, thread_controls.miny, 2*th + width, thread_controls.boxheight(), 2);
			dp->textout(x + width/2, (thread_controls.miny+thread_controls.maxy)/2, msg,-1, LAX_CENTER);

			msg = _("Run");
			x += width + 2*th;
			thread_run = x-th;
			dp->NewBG(coloravg(nodes->colors->fg.Pixel(),nodes->colors->bg.Pixel(), lasthoverprop == NODES_Thread_Run ? .65 : .75));
			width = dp->textextent(msg,-1, NULL,NULL);
			dp->drawrectangle(thread_run, thread_controls.miny, 2*th + width, thread_controls.boxheight(), 2);
			dp->textout(x + width/2, (thread_controls.miny+thread_controls.maxy)/2, msg,-1, LAX_CENTER);

			msg = _("Reset");
			x += width + 2*th;
			thread_reset = x-th;
			dp->NewBG(coloravg(nodes->colors->fg.Pixel(),nodes->colors->bg.Pixel(), lasthoverprop == NODES_Thread_Reset ? .65 : .75));
			width = dp->textextent(msg,-1, NULL,NULL);
			dp->drawrectangle(thread_reset, thread_controls.miny, 2*th + width, thread_controls.boxheight(), 2);
			dp->textout(x + width/2, (thread_controls.miny+thread_controls.maxy)/2, msg,-1, LAX_CENTER);

			thread_controls.maxx = x + width + th;

		} else {
			 //  "No threads" "Scan for threads"
			 //  "Pause"
			const char *msg = (playing==1) ? _("Pause")
								: playing == -1 ? _("Done!")
								  : lasthoverslot == NODES_Thread_Controls ? _("Scan for threads") : _("No threads");
			if (playing == -1) playing = 0;
			width = dp->textextent(msg,-1, NULL,NULL);
			thread_controls.maxx = thread_controls.minx + thread_controls.minx + width + 2*th;
			thread_controls.maxy = thread_controls.miny + thread_controls.miny + 2*th;

			dp->NewBG(coloravg(nodes->colors->fg.Pixel(),nodes->colors->bg.Pixel(), lasthoverslot == NODES_Thread_Controls ? .65 : .75));
			dp->drawrectangle(thread_controls.minx, thread_controls.miny, thread_controls.boxwidth(), thread_controls.boxheight(), 2);
			dp->textout(thread_controls.BBoxPoint(.5,.5), msg,-1, LAX_CENTER);

			thread_controls.maxx = thread_controls.minx + thread_controls.minx + width + 2*th;
		}


		 //show fps counter
		if (playing) {
			char str[50];
			sprintf(str, "fps: %.1f", cur_fps);
			dp->textout(thread_controls.BBoxPoint(.5,1), str,-1, LAX_TOP|LAX_HCENTER);
		}
	}


	 //draw node parent list
	double x = 0;

	if (grouptree.n) {
		dp->NewFG(coloravg(nodes->colors->fg.Pixel(),nodes->colors->bg.Pixel(), .25));
		dp->NewBG(coloravg(nodes->colors->fg.Pixel(),nodes->colors->bg.Pixel(), .75));

		double width = dp->textextent(_("Root"),-1, NULL,NULL);
		dp->drawRoundedRect(x,0, width+th,th*1.5, th/3, false, th/3, false, 2); 
		x = th/2;
		x += th + dp->textout(x,th/4, _("Root"),-1, LAX_TOP|LAX_LEFT);

		//c==0 is root
		const char *str;
		for (int c=1; c < grouptree.n; c++) {
			str = grouptree.e[c]->Label();
			if (!str) str = grouptree.e[c]->Id();
			width = dp->textextent(str,-1, NULL,NULL);
			dp->drawRoundedRect(x-th/2,0, width+th,th*1.5, th/3, false, th/3, false, 2); 

			x += th + dp->textout(x,th/4, str,-1, LAX_TOP|LAX_LEFT);
		}

		 //write out current
		str = nodes->Label();
		if (!str) str = nodes->Id();
		width = dp->textextent(str,-1, NULL,NULL);
		dp->drawRoundedRect(x-th/2,0, width+th,th*1.5, th/3, false, th/3, false, 2); 
		dp->textout(x,th/4, str,-1, LAX_TOP|LAX_LEFT);
	}


	dp->NewTransform(nodes->m.m());
	dp->font(font);

	dp->LineWidth(1);
	dp->NewFG(&nodes->colors->fg_frame);
	dp->NewBG(&nodes->colors->bg_frame);
	for (int c=0; c<nodes->frames.n; c++) {
		dp->drawRoundedRect(nodes->frames.e[c]->x, nodes->frames.e[c]->y, nodes->frames.e[c]->width, nodes->frames.e[c]->height,
							th/3, false, th/3, false, 2); 
		if (nodes->frames.e[c]->Label())
			dp->textout(th+nodes->frames.e[c]->x, th*.5 + nodes->frames.e[c]->y, nodes->frames.e[c]->Label(), -1, LAX_LEFT|LAX_TOP);
	}

	 //---draw connections
	dp->NewFG(&nodes->colors->connection);

	dp->LineWidth(3);
	NodeBase *node;
	for (int c=0; c<nodes->nodes.n; c++) {
		node = nodes->nodes.e[c];
		for (int c2=0; c2<node->properties.n; c2++) {
			if (node->properties.e[c2]->IsOutput() || node->properties.e[c2]->IsExecOut()) {
				for (int c3=0; c3 < node->properties.e[c2]->connections.n; c3++) {
					DrawConnection(node->properties.e[c2]->connections.e[c3]);
				}
			}
		}
	}


	 //---draw nodes:
	 //  box+border
	 //  label
	 //  expanded arrow
	 //  preview
	 //  ins/outs
	ScreenColor *border, *fg;
	ScreenColor tfg, tbg, tmid, hprop;
	NodeColors *colors=NULL;
	double borderwidth = 1;
	int status;

	for (int c=0; c<nodes->nodes.n; c++) {
		node = nodes->nodes.e[c];

		 //make current threads show up by drawing colored box behind
		if (show_threads && threads.n && IsThread(node)) {
			ScreenColor color_thread(.5,.5,1.,1.);
			dp->NewFG(&color_thread);
			dp->drawrectangle(node->x-th/2, node->y-th/2, node->width+th, node->height+th, 1);
		}


		DBG cerr <<"node "<<node->Id()<<" "<<node->x<<" "<<node->y<<" "<<node->width<<" "<<node->height<<endl;

		 //set up colors based on whether the node is selected or mouse overed
		if (node->colors) colors=node->colors;
		else colors = nodes->colors;

		borderwidth = 1;
		border = &colors->border;
		bg = &colors->bg;
		fg = &colors->fg;

		if (IsSelected(node))  { 
			border = &colors->selected_border;
			bg     = &colors->selected_bg;
			borderwidth = 3;
		}

		 //update for error state
		status = node->GetStatus();
		if (status < 0 || node->ErrorMessage()) {
			borderwidth = 3;
			border = &colors->error_border;
		} else if (status > 0) {
			border = &colors->update_border;
		}

		if (node->ErrorMessage()) {
			dp->textout(node->x+node->width/2, node->y, node->ErrorMessage(),-1, LAX_HCENTER|LAX_BOTTOM);
		}

		if (lasthover == c) { //mouse is hovering over this node
			tfg = *fg;
			tbg = *bg;
			tfg.AddDiff(colors->mo_diff, colors->mo_diff, colors->mo_diff);
			tbg.AddDiff(colors->mo_diff, colors->mo_diff, colors->mo_diff);
			fg = &tfg;
			bg = &tbg;
			hprop = *bg;
			hprop.AddDiff(colors->mo_diff, colors->mo_diff, colors->mo_diff);
		}
		flatpoint p;
		fg->Average(&tmid, *bg, .5); //middle color

		 //draw whole rect, bg
		dp->NewFG(node->collapsed ? &colors->label_bg : bg);
		dp->LineWidth(borderwidth);
		dp->drawRoundedRect(node->x, node->y, node->width, node->height,
							th/3, false, th/3, false, 1); 

		 //do something special for groups
		if (dynamic_cast<NodeGroup*>(node)) {
			dp->drawRoundedRect(node->x-th/4, node->y-th/4, node->width+th/2, node->height+th/2,
								th/3, false, th/3, false, 0); 
		}

		 //draw label area
		dp->NewFG(&colors->label_bg);
		dp->drawRoundedRect(node->x, node->y, node->width, th,
							th/3, false, th/3, false, 1, 8|4); 

		 //draw whole rect border
		dp->NewFG(border);
		dp->drawRoundedRect(node->x, node->y, node->width, node->height,
							th/3, false, th/3, false, 0); 

		 //draw label
		double labely = node->y;
		if (node->collapsed) {
			if (node->UsesPreview()) labely = node->y;
			else labely = node->y+node->height/2-th/2;
		}
		dp->NewFG(&colors->label_fg);
		dp->textout(node->x+node->width/2+th/4, labely, node->Name, -1, LAX_TOP|LAX_HCENTER);

		 //draw collapse arrow
		dp->LineWidth(1);
		if (node->collapsed) {
			dp->drawthing(node->x+th,labely+th/2, th/4,th/4, lasthover==c && lasthoverslot==NODES_Collapse ? 1 : 0, THING_Triangle_Right);
		} else {
			dp->NewFG(&tmid);
			dp->drawthing(node->x+th,labely+th/2, th/4,th/4, lasthover==c && lasthoverslot==NODES_Collapse ? 1 : 0, THING_Triangle_Down);
		}
	

		dp->NewFG(fg);
		dp->NewBG(bg);


		 //draw the properties (or not)
		p.set(node->x+node->width, node->y);

		double y = node->y+th*1.5;

		 //draw preview
		if (node->UsesPreview()) {
			 //assume we want to render the preview at actual pixel size
			DoubleRectangle box;
			dp->imageout_within(node->total_preview, node->x,node->y+th*1.15, node->width, node->preview_area_height, &box, 1);
			dp->LineWidthScreen(lasthover == c && lasthoverslot == NODES_PreviewResize ? 3 : 1);
			dp->NewFG(coloravg(fg->Pixel(),bg->Pixel(),.5));
			dp->drawrectangle(box.x,box.y,box.width,box.height, 0);
			dp->NewFG(fg);
			y += node->preview_area_height;
		}

		 //draw ins and outs
		NodeProperty *prop;

		for (int c2=0; c2<node->properties.n; c2++) {
			prop = node->properties.e[c2];
			if (lasthover == c && overslot == -1 && overprop == c2 && !node->collapsed) { //mouse is hovering over this property
				dp->NewFG(&hprop);
				dp->drawrectangle(node->x+node->properties.e[c2]->x,node->y+node->properties.e[c2]->y,
						node->properties.e[c2]->width, node->properties.e[c2]->height, 1);
				dp->NewFG(fg);
			}
			DrawProperty(node, prop, y, overnode == c && overprop == c2,
										overnode == c && overprop == c2 && overslot == c2);

			y += prop->height;
		}

		if (lasthover == c && (lasthoverslot == NODES_LeftEdge || lasthoverslot == NODES_RightEdge)) {	
			NodeBase *node = nodes->nodes.e[lasthover];
			flatpoint p1,p2;
			p1.x = p2.x = node->x + (lasthoverslot == NODES_LeftEdge ? 0 : node->width);
			p1.y = node->y;
			p2.y = node->y+node->height;;
			dp->LineWidthScreen(3);
			dp->NewFG(&color_controls);
			dp->drawline(p1,p2);
		}

	} //foreach node


	 //draw mouse action decorations
	if (hover_action==NODES_Cut_Connections || hover_action==NODES_Selection_Rect) {
		dp->LineWidthScreen(1);
		dp->NewFG(&color_controls);

		flatpoint p1 = nodes->m.transformPointInverse(flatpoint(selection_rect.minx,selection_rect.miny));
		flatpoint p2 = nodes->m.transformPointInverse(flatpoint(selection_rect.maxx,selection_rect.maxy));

		if (hover_action == NODES_Selection_Rect) {
			dp->drawrectangle(p1.x,p1.y, p2.x-p1.x,p2.y-p1.y, 0);
		} else {
			dp->drawline(p1, p2);
		}

	} else if (hover_action==NODES_Drag_Input
			|| hover_action==NODES_Drag_Output
			|| hover_action==NODES_Drag_Exec_In
			|| hover_action==NODES_Drag_Exec_Out) {
		if (tempconnection) DrawConnection(tempconnection);

	}

	dp->PopAxes();

	return 0;
}

void NodeInterface::DrawProperty(NodeBase *node, NodeProperty *prop, double y, int hoverprop, int hoverslot)
{
	// todo, defaults for:
	//   numbers
	//   vectors
	//   colors
	//   enums

	//DBG cerr <<"Draw property "<<prop->name<<endl;

	if (!strcmp(prop->name, "result")) {
		DBG cerr <<" ";
	}

	double th = dp->textheight();
	ScreenColor *propcolor = &node->colors->default_property;

	if (!node->collapsed) {
		if (prop->HasInterface()) {
			prop->Draw(dp, hoverprop);

		} else if (prop->type == NodeProperty::PROP_Button) {
			double w = th + dp->textextent(prop->Label(),-1, NULL,NULL);
			ScreenColor highlight(node->colors->bg), shadow(node->colors->bg);
			highlight.Average(&highlight, node->colors->fg, .2);
			shadow.   Average(&highlight, node->colors->fg, .8);
			int state = 0;
			dp->drawBevel(th*.05, &highlight, &shadow, state, node->x+prop->x+prop->width/2-w/2, node->y+prop->y+prop->height/2-th*.6, w, th*1.2);
			dp->textout(node->x+prop->x+prop->width/2, node->y+prop->y+prop->height/2, prop->Label(),-1, LAX_CENTER);

		} else {
			Value *v = prop->GetData();

			char extra[200];
			extra[0]='\0';
			dp->LineWidth(1);
			ScreenColor col;

			if (v && (v->type()==VALUE_Real || v->type()==VALUE_Int)) {
				dp->NewFG(coloravg(&col, &nodes->colors->bg_edit, &nodes->colors->fg_edit));
				dp->NewBG(&nodes->colors->bg_edit);
				if (prop->IsEditable()) 
					dp->drawRoundedRect(node->x+prop->x+th/2, node->y+prop->y+th/4, node->width-th, prop->height*.66,
									th/3, false, th/3, false, 2); 

				dp->NewFG(&nodes->colors->fg_edit);
				sprintf(extra, "%s:", prop->Label());
				dp->textout(node->x+prop->x+th, node->y+prop->y+prop->height/2, extra, -1, LAX_LEFT|LAX_VCENTER);
				v->getValueStr(extra, 199);
				dp->textout(node->x+node->width-th, node->y+prop->y+prop->height/2, extra, -1, LAX_RIGHT|LAX_VCENTER);

				propcolor = &node->colors->number;

			} else if (v && v->type()==VALUE_String) {
				dp->NewFG(&nodes->colors->fg);

				 //prop label
				double dx = th/2+dp->textout(node->x+th/2, y+prop->height/2, prop->Label(),-1, LAX_LEFT|LAX_VCENTER); 

				 //prop string
				dp->NewFG(coloravg(&col, &nodes->colors->bg_edit, &nodes->colors->fg_edit));
				dp->NewBG(&nodes->colors->bg_edit);
				if (prop->IsEditable()) 
					dp->drawRoundedRect(dx+node->x+prop->x+th/2, node->y+prop->y+th/4, node->width-th-dx, prop->height*.66,
									th/3, false, th/3, false, 2);
				dp->NewFG(&nodes->colors->fg_edit);
				dp->textout(dx+node->x+th, y+prop->height/2, dynamic_cast<StringValue*>(v)->str,-1, LAX_LEFT|LAX_VCENTER); 

			} else if (v && v->type()==VALUE_Enum) {

				 //draw name
				double x=th/2;
				double dx=dp->textout(node->x+x, node->y+prop->y+prop->height/2, prop->Label(),-1, LAX_LEFT|LAX_VCENTER);
				x+=dx+th/2;

				 //draw value
				dp->NewFG(coloravg(&col, &nodes->colors->bg_edit, &nodes->colors->fg_edit));
				dp->NewBG(&nodes->colors->bg_menu);
				dp->drawRoundedRect(node->x+x, node->y+prop->y+th/4, node->width-th/2-x, prop->height*.66,
									th/3, false, th/3, false, 2); 

				dp->NewFG(&nodes->colors->fg_edit);

				//v->getValueStr(extra, 199);
				//-----
				EnumValue *ev = dynamic_cast<EnumValue*>(v);
				const char *nm=NULL, *Nm=NULL; 
				ev->GetObjectDef()->getEnumInfo(ev->value, &nm, &Nm);
				if (!Nm) Nm = nm;
				dp->textout(node->x+th*1.5+dx, node->y+prop->y+prop->height/2, Nm,-1, LAX_LEFT|LAX_VCENTER);
				//dp->textout(node->x+th*1.5+dx, node->y+prop->y+prop->height/2, extra,-1, LAX_LEFT|LAX_VCENTER);
				dp->drawthing(node->x+node->width-th, node->y+prop->y+prop->height/2, th/4,th/4, 1, THING_Triangle_Down);

			} else if (v && v->type()==VALUE_Boolean) {
				coloravg(&col, &nodes->colors->bg_edit, &nodes->colors->fg_edit);
				dp->NewFG(&col);
				coloravg(&col, &nodes->colors->bg_edit, &nodes->colors->fg_edit, .15);
				dp->NewBG(hoverprop ? &col : &nodes->colors->bg_edit);

				BooleanValue *vv = dynamic_cast<BooleanValue*>(v);

				if (!prop->IsOutput()) {
					dp->drawrectangle(node->x+prop->x+th/2, y+prop->height/2-th/2, th, th, 2);

					dp->NewFG(&nodes->colors->fg_edit);
					if (vv->i) dp->drawthing(node->x+prop->x + th,y+prop->height/2, th/2,-th/2, 1, THING_Check);

					 //draw on left side
					dp->textout(node->x+2*th, y+prop->height/2, prop->Label(),-1, LAX_LEFT|LAX_VCENTER); 

				} else {
					 //draw on right side
					dp->drawrectangle(node->x+prop->x+prop->width - 3*th/2, y+prop->height/2-th/2, th, th, 2);

					dp->NewFG(&nodes->colors->fg_edit);
					if (vv->i) dp->drawthing(node->x+prop->x + prop->width - th,y+prop->height/2, th/2,-th/2, 1, THING_Check);

					dp->textout(node->x+node->width-2*th, y+prop->height/2, prop->Label(),-1, LAX_RIGHT|LAX_VCENTER);
				}

			} else if (v && v->type()==VALUE_Color) {
				ColorValue *color = dynamic_cast<ColorValue*>(v);
				double x = node->x+th/2;
				unsigned long oldfg = dp->FG();
				if (prop->IsEditable()) {
					 //draw color box
					dp->NewFG(color->color.Red(),color->color.Green(),color->color.Blue(),color->color.Alpha());
					dp->drawrectangle(x,y+prop->height/2-th/2, 2*th, th, 1);
					dp->NewFG(coloravg(&col, &nodes->colors->bg_edit, &nodes->colors->fg_edit));
					dp->drawrectangle(x,y+prop->height/2-th/2, 2*th, th, 0);
					x += 2*th + th/2;
				}
				dp->NewFG(oldfg);
				dp->textout(x,y+prop->height/2, prop->Label(),-1, LAX_LEFT|LAX_VCENTER);

				propcolor = &node->colors->color;

			} else {
				 //fallback, just write out the property name
				dp->NewFG(&nodes->colors->fg);
				if (!prop->IsOutput()) {
					 //draw on left side
					double dx = dp->textout(node->x+th/2, y+prop->height/2, prop->Label(),-1, LAX_LEFT|LAX_VCENTER); 
					if (!isblank(extra)) {
						dp->textout(node->x+th+dx, y+prop->height/2, extra,-1, LAX_LEFT|LAX_VCENTER);
						dp->drawrectangle(node->x+th/2+dx, y, node->width-(th+dx), th*1.25, 0);
					}

				} else {
					 //draw on right side
					double dx = dp->textout(node->x+node->width-th/2, y+prop->height/2, prop->Label(),-1, LAX_RIGHT|LAX_VCENTER);
					if (!isblank(extra)) {
						dp->textout(node->x+node->width-th-dx, y+prop->height/2, extra,-1, LAX_RIGHT|LAX_VCENTER);
						dp->drawrectangle(node->x+th/2, y-th*.25, node->width-(th*1.25+dx), th*1.25, 0);
					}
				}
			}

			if (v && (v->type()==VALUE_Flatvector || v->type()==VALUE_Spacevector || v->type()==VALUE_Quaternion))
				propcolor = &node->colors->vector;
		}
	} // !node->collapsed

	if (prop->IsExec()) propcolor = &node->colors->exec;

	 //draw connection spot
	if (prop->is_linkable) {
		//dp->NewBG(&prop->color);
		dp->NewBG(propcolor);

		if (prop->IsExec()) {
			//dp->drawthing(node->x+th,labely+th/2, th/4,th/4, lasthover==c && lasthoverslot==NODES_Collapse ? 1 : 0, THING_Triangle_Right);
			dp->drawthing(prop->pos+flatpoint(node->x,node->y),
				(hoverslot ? 2 : 1)*th*node->colors->slot_radius,
				(hoverslot ? 2 : 1)*th*node->colors->slot_radius, 2, THING_Triangle_Right);
		} else {
			dp->drawellipse(prop->pos+flatpoint(node->x,node->y),
				(hoverslot ? 2 : 1)*th*node->colors->slot_radius, (hoverslot ? 2 : 1)*th*node->colors->slot_radius, 0,0, 2);
		}
		if (node->collapsed && hoverslot) {
			 //draw tip of name next to pos
			flatpoint pp = prop->pos+flatpoint(node->x+th,node->y);
			double width = th + node->colors->font->extent(prop->Label(),-1);
			dp->drawrectangle(pp.x,pp.y-th*.75, width, 1.5*th, 2);
			dp->textout(pp.x+th/2,pp.y, prop->Label(),-1, LAX_LEFT|LAX_VCENTER);
		}
	} 

	//DBG cerr <<"end draw property "<<prop->name<<endl;
}

void NodeInterface::DrawConnection(NodeConnection *connection) 
{
	flatpoint p1,c1,c2,p2;
	flatpoint last = nodes->m.transformPointInverse(lastpos);
	if (connection->fromprop) p1=flatpoint(connection->from->x,connection->from->y)+connection->fromprop->pos; else p1=last;
	if (connection->toprop)   p2=flatpoint(connection->to->x,  connection->to->y)  +connection->toprop->pos;   else p2=last;

	dp->NewFG(0.,0.,0.);
	dp->moveto(p1);
	if (p2.x < p1.x) {
		c1 = p1-flatpoint((p2.x-p1.x), 0);
		c2 = p2+flatpoint((p2.x-p1.x), 0);
		dp->curveto(c1,c2,p2);
		//----
		//dp->curveto(p1-flatpoint((p2.x-p1.x), 0),
					//p2+flatpoint((p2.x-p1.x), 0),
					//p2);
	} else {
		c1 = p1+flatpoint((p2.x-p1.x)/3, 0);
		c2 = p2-flatpoint((p2.x-p1.x)/3, 0);
		dp->curveto(c1,c2,p2);
		//dp->curveto(p1+flatpoint((p2.x-p1.x)/3, 0),
					//p2-flatpoint((p2.x-p1.x)/3, 0),
					//p2);
	}
	dp->LineWidthScreen(3);
	dp->stroke(1);
	int isselected = (selected.findindex(connection->from)>=0 || selected.findindex(connection->to)>=0);
	dp->NewFG(isselected ? &nodes->colors->selected_border : &color_controls);
	dp->LineWidthScreen(2);
	dp->stroke(0);

	if ((connection->fromprop && connection->fromprop->IsExec())
		|| (connection->toprop && connection->toprop->IsExec())) {
		// draw little arrows to indicate flow directioon periodically on path
		double len = 2; //seconds
		double offset = 1;
		if (IsLive(connection)) {
			 //draw arrows offset
			len *= sysconf(_SC_CLK_TCK);
			offset = (times(NULL) % (int)len) / len;
		}

		len = bez_segment_length(p1,c1,c2,p2, 20);
		int n = 1 + len/(10*dp->textheight());
		if (n==1) n++;
		flatpoint p;
		double t;
		for (int c=0; c<n; c++) {
			t = (c + offset) / (float)n;
			p = bez_point(t, p1,c1,c2,p2);

			dp->drawcircle(p, 5, 1);
		}
	}
}

/*! Return 1 or -1 for connected to a property (via from or to) listed in any node threads.
 * Else 0.
 */
int NodeInterface::IsLive(NodeConnection *con)
{
	for (int c=0; c<threads.n; c++) {
		NodeThread *thread = threads.e[c];
		if (thread->next) return 1;
		//if (thread->current_property == con->fromprop) return -1;
		//if (thread->current_property == con->toprop) return 1;
	}
	return 0;
}

int NodeInterface::Play()
{
	if (playing == 1) return 0; //already playing

	 //start timer
	play_timer = app->addtimer(this, 1000/play_fps, 1000/play_fps, -1);
	playing = 1;
	return 0;
}

/*! Toggle the play timer on and off.
 * If !playing, then just return Play().
 */
int NodeInterface::TogglePause()
{
	if (play_timer) {
		// just remove timer
		app->removetimer(this, play_timer);
		play_timer = 0;
		return 0;
	}

	if (playing <= 0) return Play();

	//restart player
	play_timer = app->addtimer(this, 1000/play_fps, 1000/play_fps, -1);
	return 0;
}

/*! Flush threads and remove timers.
 */
int NodeInterface::Stop()
{
	threads.flush();
	if (play_timer) app->removetimer(this, play_timer);
	play_timer = 0;
	playing = 0;
	return 0;
}

/*! Flushes threads and repopulates. Returns the number of threads found.
 */
int NodeInterface::FindThreads(bool flush)
{
	if (flush) threads.flush();
	if (!nodes) return 0;

	NodeBase *node;
	NodeProperty *prop;
	//NodeConnection *con;
	int out = 0;

	for (int c=0; c<nodes->nodes.n; c++) {
		node = nodes->nodes.e[c];
		for (int c2=0; c2<node->properties.n; c2++) {
			prop = node->properties.e[c2];
			if (prop->IsExecIn()) break;
			if (!prop->IsExecOut()) continue;

			threads.push(new NodeThread(node, prop, NULL,0));

//			 //to count, out needs to be connected
//			for (int c3=0; c3<prop->connections.n; c3++) {
//				con = prop->connections.e[c3];
//				if (con->from == node) {
//					out++;
//					threads.push(new NodeThread(con, prop, NULL, 0));
//				}
//			}
		}
	}

	return out;
}

/*! Get the bezier path of the connection.
 * For now, keep it simple to single bezier segment.
 */
void NodeInterface::GetConnectionBez(NodeConnection *connection, flatpoint *pts)
{
	flatpoint p1,p2;
	flatpoint last = nodes->m.transformPointInverse(lastpos);
	if (connection->fromprop) p1=flatpoint(connection->from->x,connection->from->y)+connection->fromprop->pos; else p1=last;
	if (connection->toprop)   p2=flatpoint(connection->to->x,  connection->to->y)  +connection->toprop->pos;   else p2=last;

	pts[0] = p1;
	if (p2.x < p1.x) {
		pts[1] = p1-flatpoint((p2.x-p1.x), 0);
		pts[2] = p2+flatpoint((p2.x-p1.x), 0);
	} else {
		pts[1] = p1+flatpoint((p2.x-p1.x)/3, 0);
		pts[2] = p2-flatpoint((p2.x-p1.x)/3, 0);
	}
	pts[3] = p2;
}

/*! Return the node under x,y, or -1 if no node there.
 * This will scan within a buffer around edges to scan for hovering over node
 * in/out, or edges for resizing.
 *
 * overpropslot is the connecting port. overproperty is the index of the property.
 *
 * overpropslot and overproperty must NOT be null.
 */
int NodeInterface::scan(int x, int y, int *overpropslot, int *overproperty, int *overconnection, unsigned int state) 
{
	if (!nodes) return -1;


	 //scan for viewport bounds controls
	//double vpw = dp->Maxx-dp->Minx, vph = dp->Maxy-dp->Miny;
	//int t,b,l,r;
	//t = (x>=vpw*viewport_bounds.minx && x<=vpw*viewport_bounds.maxx && y>=vph*viewport_bounds.miny && y<=vph*viewport_bounds.miny+vp_dragpad);
	//b = (x>=vpw*viewport_bounds.minx && x<=vpw*viewport_bounds.maxx && y>=vph*viewport_bounds.maxy-vp_dragpad && y<=vph*viewport_bounds.maxy);
	//l = (x>=vpw*viewport_bounds.minx && x<=vpw*viewport_bounds.minx && y>=vph*viewport_bounds.miny && y<=vph*viewport_bounds.maxy);
	//r = (x>=vpw*viewport_bounds.minx && x<=vpw*viewport_bounds.maxx && y>=vph*viewport_bounds.miny && y<=vph*viewport_bounds.maxy);
	//if (t && l) return NODES_VP_Top_Left;
	//if (t && r) return NODES_VP_Top_Right;
	//if (b && l) return NODES_VP_Bottom_Left;
	//if (b && r) return NODES_VP_Bottom_Right;
	//if (t) return NODES_VP_Top_Top;
	//if (b) return NODES_VP_Top_Bottom;
	//if (l) return NODES_VP_Top_Left;
	//if (r) return NODES_VP_Top_Right;


	 //reminder: y increased downward
	flatpoint p = nodes->m.transformPointInverse(flatpoint(x,y));

	*overpropslot  =-1;
	*overproperty  =-1;
	*overconnection=-1;

	NodeBase *node;
	double th = font->textheight();
	double rr;

	 //check node related
	for (int c=nodes->nodes.n-1; c>=0; c--) {
		node = nodes->nodes.e[c];

		if (node->collapsed) rr = th*node->colors->slot_radius;
		else rr = th/2;

		 //check for bounds slightly bigger horizontally than actual bounds
		if (p.x >= node->x-th/2 &&  p.x <= node->x+node->width+th/2 &&  p.y >= node->y &&  p.y <= node->y+node->height) {
			 //found a node, now see if over a property's in/out

			NodeProperty *prop;
			for (int c2=0; c2<node->properties.n; c2++) {
				prop = node->properties.e[c2];

				//if (!(prop->IsInput() && !prop->is_linkable)) { //only if the input is not exclusively internal
				//if (prop->is_linkable && (prop->IsInput() || prop->IsOutput())) { //only if the input is not exclusively internal
				if (prop->is_linkable && !prop->IsBlock()) { //only if the input is not exclusively internal
				  if (  p.y >= node->y+prop->pos.y-rr && p.y <= node->y+prop->pos.y+rr) {
					if (p.x >= node->x+prop->pos.x-rr && p.x <= node->x+prop->pos.x+rr) {
						*overproperty=c2;
						*overpropslot = c2;
					}
				  }
				}

				if (p.y >= node->y+prop->y && p.y < node->y+prop->y+prop->height) {
					*overproperty=c2;
				}
			}

			if (*overpropslot == -1) {
				 //check if hovering over an edge
				if (!node->collapsed && p.x >= node->x-th/2 && p.x <= node->x+th/2) *overpropslot = NODES_LeftEdge;
				else if (!node->collapsed && p.x >= node->x+node->width-th/2 && p.x <= node->x+node->width+th/2) *overpropslot = NODES_RightEdge;

				else if (node->collapsed || (p.y >= node->y && p.y <= node->y+th)) { //on label area
					if (p.x >= node->x+th/2 && p.x <= node->x+3*th/2) *overpropslot = NODES_Collapse;
					//else if (p.x >= node->x+node->width-th/2 && p.x <= node->x+node->width-3*th/2) *overpropslot = NODES_TogglePreview;
					else *overpropslot = NODES_Label;
				}

				if (*overpropslot != -1) *overproperty = -1;
			}

			 //check for preview hover things
			if (*overpropslot == -1 && node->UsesPreview()) {
				double nw = node->total_preview->w() * node->preview_area_height/node->total_preview->h();
				if (nw > node->width) nw = node->width;

				//char str[100];
				//sprintf(str, "%f %f", nw, node->width);
				//PostMessage(str);

				double nh = node->total_preview->h();
				if (nh > node->preview_area_height) nh = node->preview_area_height;

				if (   p.x >= node->x + node->width/2 + nw/2 - th
					&& p.x <= node->x + node->width/2 + nw/2
					&& p.y >= node->y + th*1.15 + node->preview_area_height/2 + nh/2 - th
					&& p.y <= node->y + th*1.15 + node->preview_area_height/2 + nh/2) {
				  *overpropslot = NODES_PreviewResize;
				  if (*overpropslot != -1) *overproperty = -1;
				}
			}
			return c; 
		}
	}

	 //check frame related
	for (int c=0; c<nodes->frames.n; c++) {
		if (nodes->frames.e[c]->pointIsIn(p.x, p.y)) {
			*overproperty = NODES_Frame;
			*overpropslot = c;

			if (p.y < nodes->frames.e[c]->y + 1.5*th) {
				*overproperty = NODES_Frame_Label;
			}
			break;
		}
	}

	 //check over pipes
	if ((state & (ShiftMask|ControlMask)) != 0) {
		DoubleBBox box;
		NodeProperty *prop;
		NodeConnection *connection;
		double d;
		flatpoint bp;
		double threshhold = 200 / dp->Getmag();

		DBG cerr<<"scan connections "<<p.x<<','<<p.y<<"  threshhold: "<<threshhold<<endl;

		for (int c=nodes->nodes.n-1; c>=0; c--) {
			node = nodes->nodes.e[c];

			//shift click jumps to nearest pipe connection
			//control click to add anchor node

			for (int c2=0; c2<node->properties.n; c2++) {
				prop = node->properties.e[c2];
				if (!prop->IsInput()) continue;

				for (int c3=0; c3<prop->connections.n; c3++) {
					connection = prop->connections.e[c3];
					GetConnectionBez(connection, bezbuf);
					box.clear();
					box.addtobounds(bezbuf[0]);
					box.addtobounds(bezbuf[1]);
					box.addtobounds(bezbuf[2]);
					box.addtobounds(bezbuf[3]);
					if (!box.boxcontains(p.x, p.y)) continue;
					DBG cerr <<" ----almost "<<c<<','<<c2<<','<<c3<<endl;

					 //in box, now do more expensive scan for proximity
					bez_closest_point(p, bezbuf[0], bezbuf[1], bezbuf[2], bezbuf[3], 20, &d, NULL, &bp);
					if (d < threshhold) {
						*overconnection = c3;
						*overproperty = NODES_Connection;
						*overpropslot = c2;
						return c;
					}
				}
			}
		}
	}

	if (thread_controls.boxcontains(x,y)) {
		*overpropslot = NODES_Thread_Controls;
		if (playing<=0 && threads.n) {
			if (x<thread_run) *overproperty = NODES_Thread_Next;
			else if (x<thread_reset) *overproperty = NODES_Thread_Run;
			else *overproperty = NODES_Thread_Reset;
		}
		return -1;
	}

	return -1;
}

int NodeInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{

	int action = NODES_None;
	int overpropslot=-1, overproperty=-1, overconnection=-1; 
	int overnode = scan(x,y, &overpropslot, &overproperty, &overconnection, state);

	if (count == 2 && overnode>=0 && overproperty<0 && overpropslot == NODES_Label) {
		action = NODES_Label;

	} else if (count == 2 && overnode<0 && overproperty == NODES_Frame_Label) {
		action = NODES_Frame_Label;

	} else if (overnode<0 && (overproperty == NODES_Frame || overproperty == NODES_Frame_Label)) {
		action = NODES_Move_Frame;

	} else if (overproperty == NODES_Connection) {
		action = NODES_Jump_Nearest;

	} else if (overpropslot == NODES_Thread_Controls) {
		action = NODES_Thread_Controls;

	} else if (((state&LAX_STATE_MASK) == 0 || (state&ShiftMask)!=0) && overnode==-1 && overproperty==-1) {
		 //shift drag adds, shift-control drag removes, plain drag replaces selection 
		action = NODES_Selection_Rect;
		selection_rect.minx=selection_rect.maxx=x;
		selection_rect.miny=selection_rect.maxy=y;
		needtodraw=1;

	} else if ((state&LAX_STATE_MASK) == ControlMask && overnode==-1 && overproperty==-1) {
		 //control-drag make a cut line for connections
		action = NODES_Cut_Connections;
		selection_rect.minx=selection_rect.maxx=x;
		selection_rect.miny=selection_rect.maxy=y;
		needtodraw=1;

	} else if (overnode>=0 && overproperty==-1 && overpropslot == NODES_PreviewResize) {
		action = NODES_Resize_Preview;

	} else if (overnode>=0 && overproperty==-1) {
		 //in a node, but not clicking on a property, so add or remove this node to selection
		if ((state&LAX_STATE_MASK) == ShiftMask) {
			 //add to selection
			selected.pushnodup(nodes->nodes.e[overnode]);
			action = NODES_Move_Nodes;
			needtodraw=1;

		} else if ((state&LAX_STATE_MASK) == ControlMask) {
			selected.remove(selected.findindex(nodes->nodes.e[overnode]));
			action = NODES_Move_Nodes;
			needtodraw=1;

		} else {
			 //plain click, maybe move, but if no drag, then select on lbup
			action = NODES_Move_Or_Select;
			lasthover = overnode;
			needtodraw=1;
		}

	} else if (overnode>=0 && overproperty>=0 && overpropslot==-1) {
		 //click down on a property, but not on the slot...
		action = NODES_Property;

		NodeProperty *prop = nodes->nodes.e[overnode]->properties.e[overproperty];
		if (prop->HasInterface()) {
			//dp->PushAxes();
			//dp->NewTransform(nodes->m.m());
			anInterface *interface = prop->PropInterface(NULL, dp);
			flatpoint p = nodes->m.transformPointInverse(flatpoint(x,y));
			//dp->PopAxes();
			if (interface && interface->LBDown(p.x,p.y,state,count,d) == 0) {
				action = NODES_Property_Interface;
			}
		}

	} else if (overnode>=0 && overproperty>=0 && overpropslot>=0) {
		 //down on a slot, so drag out to connect to another node
		
		NodeProperty *prop = nodes->nodes.e[overnode]->properties.e[overpropslot];

		if ((state&LAX_STATE_MASK) == ShiftMask) {

			 //jump to opposite connection
			if (prop->connections.n > 0) {
				if (prop->IsInput()) action = NODES_Jump_Back;
				else if (prop->IsOutput()) action = NODES_Jump_Forward;
				
			} else {
				PostMessage(_("Nothing to jump to!"));
				action = NODES_None;
			}

		} else {

			if (prop->IsInput() || prop->IsExecIn()) {
				if (prop->IsInput()) action = NODES_Drag_Input;
				else action = NODES_Drag_Exec_In;

				if (prop->connections.n && prop->IsInput()) {
					// if dragging input that is already connected, then disconnect from current node,
					// and drag output at the other end
					action = NODES_Drag_Output;
					overnode = nodes->nodes.findindex(prop->connections.e[0]->from);
					overpropslot = nodes->nodes.e[overnode]->properties.findindex(prop->connections.e[0]->fromprop);
					if (!tempconnection) tempconnection = new NodeConnection();
					tempconnection->SetTo(NULL,NULL); //assumes only one input
					tempconnection->SetFrom(prop->connections.e[0]->from, prop->connections.e[0]->fromprop);

					nodes->Disconnect(prop->connections.e[0], true, false);

					lastconnection = 0;
					lasthover      = overnode;
					lasthoverslot  = overpropslot;

				} else {
					 //connection didn't exist, so install a half connection to current node
					if (tempconnection) tempconnection->dec_count();
					tempconnection = new NodeConnection(NULL, nodes->nodes.e[overnode], NULL,prop);

					lastconnection = prop->connections.n-1;
					lasthover      = overnode;
					lasthoverslot  = overpropslot;
				}

			} else if (prop->IsOutput() || prop->IsExecOut()) {
				 // if dragging output, create new connection
				if (prop->IsOutput()) action = NODES_Drag_Output;
				else action = NODES_Drag_Exec_Out;

				if (prop->connections.n && prop->IsExecOut()) {
					// if dragging input that is already connected, then disconnect from current node,
					// and drag output at the other end
					action = NODES_Drag_Exec_In;
					overnode = nodes->nodes.findindex(prop->connections.e[0]->to);
					overpropslot = nodes->nodes.e[overnode]->properties.findindex(prop->connections.e[0]->toprop);
					if (!tempconnection) tempconnection = new NodeConnection();
					tempconnection->SetFrom(NULL,NULL); //assumes only one input
					tempconnection->SetTo(prop->connections.e[0]->to, prop->connections.e[0]->toprop);

					nodes->Disconnect(prop->connections.e[0], true, false);

					lastconnection = 0;
					lasthover      = overnode;
					lasthoverslot  = overpropslot;

				} else {
					if (tempconnection) tempconnection->dec_count();
					tempconnection = new NodeConnection(nodes->nodes.e[overnode],NULL, prop,NULL);

					lastconnection = prop->connections.n-1;
					lasthoverslot  = overpropslot;
					lasthover      = overnode;
				}
			}

			if (action==NODES_Drag_Output) PostMessage(_("Drag output..."));
			else PostMessage(_("Drag input..."));
		}
	}

	if (action != NODES_None) {
		buttondown.down(d->id, LEFTBUTTON, x,y, action);
		hover_action = action;
	} else hover_action = NODES_None;


	return 0; //return 0 for absorbing event, or 1 for ignoring
}

/*! Return whether we know how to edit. 0 for success, nonzero for cannot.
 * Default here is just numbers and strings.
 */
int NodeInterface::EditProperty(int nodei, int propertyi)
{
	lasthover = nodei;
	lasthoverprop = propertyi;

	NodeBase *node = nodes->nodes.e[nodei];
	NodeProperty *prop = node->properties.e[propertyi];

	if (!prop->IsEditable()) return 1;

	Value *v=dynamic_cast<Value*>(prop->GetData());
	if (!v) return 2;

	if (v->type()!=VALUE_Real && v->type()!=VALUE_Int && v->type()!=VALUE_String) return 3;

	flatpoint ul= nodes->m.transformPoint(flatpoint(node->x, node->y+prop->y));
	flatpoint lr= nodes->m.transformPoint(flatpoint(node->x+node->width, node->y+prop->y+prop->height));

	DoubleBBox bounds;
	bounds.addtobounds(ul);
	bounds.addtobounds(lr);

	char valuestr[200];
	if (v->type()!=VALUE_String) v->getValueStr(valuestr, 199);

	viewport->SetupInputBox(object_id, NULL,
			v->type()==VALUE_String ? dynamic_cast<StringValue*>(v)->str : valuestr,
			v->type()==VALUE_String ? "setpropstring" : (v->type()==VALUE_Int ? "setpropint" : "setpropdouble"), bounds,
			NULL, true);

	return 10;
}

//! Finish a new freehand line by calling newData with it.
int NodeInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	int action=NODES_None;
	int property=-1;
	int dragged = buttondown.up(d->id, LEFTBUTTON, &action, &property);

	int overpropslot=-1, overproperty=-1, overconnection=-1; 
	int overnode = scan(x,y, &overpropslot, &overproperty, &overconnection, state);


	if (action == NODES_Property) {
		 //mouse up on a property, so do property specific actions...

		if (!nodes || overnode<0 || dragged>5) return 0;
		NodeBase *node = nodes->nodes.e[overnode];
		NodeProperty *prop = node->properties.e[overproperty];

		//if (prop->IsInput() && prop->IsConnected()) return 0; //can't change if piped in from elsewhere
		if (!prop->IsEditable()) return 0;

		Value *v=dynamic_cast<Value*>(prop->data);
		if (!v) return 0;

		if (v->type()==VALUE_Real || v->type()==VALUE_Int || v->type()==VALUE_String) {
			 //create input box..
			EditProperty(overnode, overproperty);

		} else if (v->type()==VALUE_Boolean) {
			BooleanValue *vv=dynamic_cast<BooleanValue*>(prop->data);
			vv->i = !vv->i;
			node->Update();
			needtodraw=1;

		} else if (v->type()==VALUE_Color) {
			ColorValue *color = dynamic_cast<ColorValue*>(v); 

			unsigned long extra=0;
			anXWindow *w = new ColorSliders(NULL,"New Color","New Color",ANXWIN_ESCAPABLE|ANXWIN_REMEMBER|ANXWIN_OUT_CLICK_DESTROYS|extra,
							0,0,200,400,0,
						   NULL,object_id,"newcolor",
						   LAX_COLOR_RGB, 1./255,
						   color->color.colors[0],
						   color->color.colors[1],
						   color->color.colors[2],
						   color->color.colors[3],
						   color->color.colors[4],
						   //cc[0],cc[1],cc[2],cc[3],cc[4],
						   x,y);
			if (!w) return 0;
			app->rundialog(w);
			return 0;
			

		} else if (v->type()==VALUE_Enum) {
			 //popup menu with enum values..
			EnumValue *ev = dynamic_cast<EnumValue*>(v);
			ObjectDef *def=ev->GetObjectDef();

			MenuInfo *menu = new MenuInfo();
			const char *nm, *Nm;
			for (int c=0; c<def->getNumEnumFields(); c++) {
				def->getEnumInfo(c, &nm, &Nm);
				if (!Nm) Nm = nm;
				menu->AddItem(Nm, c);
			}

//			PopupMenu *popup=new PopupMenu(NULL,_("Select value..."), 0,
//							0,0,0,0, 1,
//							object_id,"selectenum",
//							0, //mouse to position near?
//							menu,1, NULL,
//							MENUSEL_LEFT|MENUSEL_CHECK_ON_LEFT|MENUSEL_DESTROY_ON_LEAVE);
			PopupMenu *popup=new PopupMenu(NULL,_("Select value..."), 0,
							0,0,0,0, 1,
							object_id,"selectenum",
							0, //mouse to position near?
							menu,1, NULL,
							TREESEL_LEFT|TREESEL_SEND_PATH|TREESEL_LIVE_SEARCH);
			popup->pad=5;
			popup->WrapToMouse(0);
			app->rundialog(popup);
		}

		return 0; 

	} else if (action == NODES_Move_Or_Select) {
		//to have this, we clicked down, but didn't move, so select the node..

		if (overnode>=0) {
			if (selected.findindex(nodes->nodes.e[overnode]) < 0
				 || lasthoverslot != NODES_Collapse) {
				 //clicking on a node that's not in the selection
				selected.flush();
				selected.push(nodes->nodes.e[overnode]);
				needtodraw=1;
			}

			if (lasthoverslot == NODES_Collapse) {
				ToggleCollapsed();

			//} else if (lasthoverslot == NODES_TogglePreview) {

			//} else if (lasthoverslot == NODES_Label && count==2) {
			} else if (lasthoverslot == NODES_Label) {
				// *** use right click menu instead??
			}
		}
		return 0;

	} else if (action == NODES_Property_Interface) {
		NodeBase *node = nodes->nodes.e[lasthover];
		NodeProperty *prop = node->properties.e[lasthoverprop];

		if (prop->HasInterface()) {
			//dp->PushAxes();
			//dp->NewTransform(nodes->m.m());
			flatpoint p = nodes->m.transformPointInverse(flatpoint(x,y));
			anInterface *interface = prop->PropInterface(NULL, dp);
			interface->LBUp(p.x,p.y, state,d);
			//dp->PopAxes();
		}

		hover_action = NODES_None;
		needtodraw=1;
		return 0;

	} else if (action == NODES_Cut_Connections) {
		 //cut any connections that cross the line between selection_rect.min to max
		flatpoint p1 = nodes->m.transformPointInverse(flatpoint(selection_rect.minx,selection_rect.miny));
		flatpoint p2 = nodes->m.transformPointInverse(flatpoint(selection_rect.maxx,selection_rect.maxy));
		CutConnections(p1,p2);
		hover_action = NODES_None;
		needtodraw=1;
		return 0;

	} else if (action == NODES_Selection_Rect) {
		 //select or deselect any touching selection_rect
		if ((state&ShiftMask)==0) {
			selected.flush();
		}
		if (!nodes) return 0;

		NodeBase *node;
		flatpoint p;
		DoubleBBox box;

		if (selection_rect.maxx < selection_rect.minx) {
			double t=selection_rect.minx;
			selection_rect.minx=selection_rect.maxx;
			selection_rect.maxx=t;
		}
		if (selection_rect.maxy < selection_rect.miny) {
			double t=selection_rect.miny;
			selection_rect.miny=selection_rect.maxy;
			selection_rect.maxy=t;
		}

		for (int c=0; c<nodes->nodes.n; c++) {
			node = nodes->nodes.e[c];

			box.clear();
			box.addtobounds(nodes->m.transformPoint(flatpoint(node->x,            node->y)));
			box.addtobounds(nodes->m.transformPoint(flatpoint(node->x+node->width,node->y)));
			box.addtobounds(nodes->m.transformPoint(flatpoint(node->x+node->width,node->y+node->height)));
			box.addtobounds(nodes->m.transformPoint(flatpoint(node->x,            node->y+node->height)));

			if (selection_rect.intersect(&box, 0)) {
				if (state&ControlMask) {
					 //remove this node from selection
					selected.remove(selected.findindex(nodes->nodes.e[c]));
				} else {
					selected.pushnodup(nodes->nodes.e[c]);
				} 
			}
		}
		needtodraw=1;

	} else if (action == NODES_Drag_Input || action == NODES_Drag_Exec_In) {
		//check if hovering over the output of some other node
		// *** need to ensure there is no circular linking
		DBG cerr << " *** need to ensure there is no circular linking for nodes!!!"<<endl;

		int remove=0;

		if (overnode>=0 && overproperty>=0) {
			 //we had a temporary connection, need to fill in the blanks to finalize
			 //need to connect  last -> over
			NodeProperty *fromprop = nodes->nodes.e[overnode]->properties.e[overproperty];

			if ((fromprop->IsOutput() && action==NODES_Drag_Input)
				|| (fromprop->IsExecOut() && action==NODES_Drag_Exec_In)) {

				 //connect to fromprop
				tempconnection->from     = fromprop->owner;
				tempconnection->fromprop = fromprop;

				nodes->Connect(tempconnection, 0);

				tempconnection->to->Update();
				remove=1;

			} else {
				remove=1;
			}

		} else if (overnode<0) {
			 //hovered over nothing
			remove=1;

		} else { //lbup over something else
			remove=1;
		}

		if (remove) {   //note: currently always need to remove!
			 //remove the unconnected connection
			tempconnection->dec_count();
			tempconnection = NULL;
			lastconnection = -1;
		} 


	} else if (action == NODES_Drag_Output || action == NODES_Drag_Exec_Out) {
		//check if hovering over the input of some other node
		DBG cerr << " *** need to ensure there is no circular linking for nodes!!!"<<endl;

		int remove=0;

		if (overnode>=0 && overproperty>=0) {
			NodeProperty *toprop = nodes->nodes.e[overnode]->properties.e[overproperty];

			if ((toprop->IsInput() && action==NODES_Drag_Output)
				|| (toprop->IsExecIn() && action==NODES_Drag_Exec_Out)) {

				if (toprop->connections.n) {
					 //clobber any other connection going into the input. Only one input allowed.
					for (int c = toprop->connections.n-1; c >= 0; c--) {
						nodes->Disconnect(toprop->connections.e[c], false, true);
					}
				}

				 //connect lasthover.lasthoverslot.lastconnection to toprop
				tempconnection->to     = toprop->owner;
				tempconnection->toprop = toprop;

				nodes->Connect(tempconnection, 0);
				tempconnection->to->Update();
				remove=1;

			} else {
				remove=1;
			}

		} else if (overnode<0) {
			 //hovered over nothing
			remove=1;
		} else { //lbup over something else
			remove=1;
		}

		if (remove) {
			 //remove the unconnected connection from what it points to as well as from nodes->connections
			tempconnection->dec_count();
			tempconnection = NULL;
			lastconnection = -1;
		} 

	} else if (action == NODES_Resize_Preview) {
		NodeBase *node = nodes->nodes.e[lasthover];
		if (selected.findindex(node) < 0) {
			node->UpdatePreview();
			if (node->frame) node->frame->Wrap();
		} else {
			for (int c=0; c<selected.n; c++) {
				selected.e[c]->UpdatePreview();
				if (selected.e[c]->frame) selected.e[c]->frame->Wrap();
			}
		}
		needtodraw=1;

	} else if (action == NODES_Label) {
		NodeBase *node = nodes->nodes.e[lasthover];

		double th = font->textheight();
		flatpoint ul= nodes->m.transformPoint(flatpoint(node->x, node->y));
		flatpoint lr= nodes->m.transformPoint(flatpoint(node->x+node->width, node->y+1.5*th));

		DoubleBBox bounds;
		bounds.addtobounds(ul);
		bounds.addtobounds(lr);

		viewport->SetupInputBox(object_id, NULL, node->Label(), "setNodeLabel", bounds);

	//} else if (action == NODES_Frame_Label || (dragged==0 && action == NODES_Move_Frame)) {
	} else if (action == NODES_Frame_Label) {
		 //popup to change label name
		NodeFrame *frame = nodes->frames.e[lasthoverslot];

		double th = font->textheight();
		flatpoint ul= nodes->m.transformPoint(flatpoint(frame->x, frame->y));
		flatpoint lr= nodes->m.transformPoint(flatpoint(frame->x+frame->width, frame->y+1.5*th));

		DoubleBBox bounds;
		bounds.addtobounds(ul);
		bounds.addtobounds(lr);

		viewport->SetupInputBox(object_id, NULL, frame->Label(), "setFrameLabel", bounds);

	//} else if (action == NODES_Move_Frame) {
		//***

	} else if (action == NODES_Frame_Comment) {
		// ***

	} else if (action == NODES_Jump_Forward || action == NODES_Jump_Back || action == NODES_Jump_Nearest) {
		if (overnode>=0 && overpropslot>=0) {
			NodeProperty *prop = nodes->nodes.e[overnode]->properties.e[overpropslot];
			//NodeProperty *toprop = NULL;

			//int topropi = -1;
			NodeConnection *connection = prop->connections.e[0];
			GetConnectionBez(connection, panpath);
			double t=0;
			if (action == NODES_Jump_Nearest) {
				flatpoint p = nodes->m.transformPointInverse(flatpoint(x,y));
				t = bez_closest_point(p, panpath[0], panpath[1], panpath[2], panpath[3], 20, NULL, NULL, NULL);
				if (t<.5) prop = connection->fromprop; else prop = connection->toprop;
			}
			if (connection->fromprop != prop) {
				 //reverse path order
				if (action == NODES_Jump_Nearest) t=1-t;
				flatpoint pp = panpath[0];
				panpath[0] = panpath[3];
				panpath[3] = pp;
				pp = panpath[1];
				panpath[1] = panpath[2];
				panpath[2] = pp;
			}
			//flatpoint last = nodes->m.transformPointInverse(lastpos);
			//flatpoint offset = last - panpath[0];
			//for (int c=0; c<4; c++) panpath[c] += offset;

			//start timer
			pan_duration = pan_tick_ms/1000. + bez_length(panpath, 4, false, true, 10) / (nodes->colors->font->textheight()*100);
			pan_current = t*pan_duration;
			if (pan_duration > 1) pan_duration = sqrt(pan_duration);
			pan_timer = app->addtimer(this, pan_tick_ms, pan_tick_ms, pan_duration*1000);
			DBG cerr <<"Adding Idle timer for NodeInterface connection traverse"<<endl;
			DBG cerr <<"pan_duration = "<<pan_duration<<endl;
		}

	} else if (action == NODES_Thread_Controls) {
		if (overproperty == NODES_Thread_Run) {
			Play();

		} else if (overproperty == NODES_Thread_Reset) {
			FindThreads(true);

		} else if (overproperty == NODES_Thread_Next) {
			ExecuteThreads();

		} else {
			if (!threads.n) {
				FindThreads(false);
			} else ExecuteThreads();
		}

		needtodraw=1;
	}

	hover_action = NODES_None;
	needtodraw = 1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int NodeInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *mouse)
{
	DBG cerr <<"NodeInterface::MouseMove..."<<endl;

	if (!buttondown.any()) {
		// update any mouse over state
		// ...

		int newhoverslot=-1, newhoverprop=-1, newconnection=-1;
		int newhover = scan(x,y, &newhoverslot, &newhoverprop, &newconnection, state);
		lastpos.x=x; lastpos.y=y;
		//DBG cerr << "nodes lastpos: "<<lastpos.x<<','<<lastpos.y<<endl;
		DBG cerr <<"nodes scan, node,prop,slot: "<<newhover<<','<<newhoverprop<<','<<newhoverslot<<","<<newconnection<<endl;

		if (newhover >= 0 && newhoverprop >= 0 && nodes->nodes.e[newhover]->properties.e[newhoverprop]->HasInterface()) {
			NodeProperty *prop = nodes->nodes.e[newhover]->properties.e[newhoverprop];
			if (prop->HasInterface()) {
				//dp->PushAxes();
				//dp->NewTransform(nodes->m.m());
				anInterface *interface = prop->PropInterface(NULL, dp);
				flatpoint p = nodes->m.transformPointInverse(flatpoint(x,y));
				interface->MouseMove(p.x,p.y, state,mouse);
				//interface->MouseMove(x,y, state,mouse);
				//dp->PopAxes();
				needtodraw |= interface->Needtodraw();
			}
		}

		if (newhover!=lasthover || newhoverslot!=lasthoverslot || newhoverprop!=lasthoverprop || newconnection!=lastconnection) {
			needtodraw=1;
			lasthoverslot = newhoverslot;
			lasthoverprop = newhoverprop;
			lastconnection = newconnection;
			lasthover = newhover;

			//post hover message
			if (lasthover<0) PostMessage("");
			else {
				if (lasthoverprop >= 0 && nodes->nodes.e[lasthover]->properties.e[lasthoverprop]->tooltip) {
					PostMessage(nodes->nodes.e[lasthover]->properties.e[lasthoverprop]->tooltip);
				} else if ((state&LAX_STATE_MASK) == ShiftMask && lasthover>=0 && lasthoverprop>=0 && lasthoverslot>=0) {
					PostMessage(_("Click to traverse connection"));
				} else {
					
					char scratch[200];
					sprintf(scratch, "%s.%d.%d", nodes->nodes.e[lasthover]->Name, lasthoverprop, lasthoverslot);
					PostMessage(scratch);
				}
			}
		}

		return 1;
	}

	int lx,ly, action, property;
	buttondown.move(mouse->id, x,y, &lx,&ly);
	

	if (buttondown.isdown(mouse->id, MIDDLEBUTTON) || buttondown.isdown(mouse->id, RIGHTBUTTON)) {
		if ((state&LAX_STATE_MASK)==ControlMask && buttondown.isdown(mouse->id, RIGHTBUTTON)) {
			 //zoom
			double amount = 1 + (x-lx)*.1;
			if (amount < .7) amount = .7;
			nodes->m.Scale(flatpoint(x,y), amount);

		} else {
			 //move screen
			DBG cerr <<"node middle button move: "<<x-lx<<", "<<y-ly<<endl;
			nodes->m.origin(nodes->m.origin() + flatpoint(x-lx, y-ly));
		}
		needtodraw=1;
		return 0;
	}

	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &action, &property);


	 //special check to maybe change action
	if (action == NODES_Move_Or_Select) {
		if (lasthoverslot == NODES_LeftEdge) {
			action = NODES_Resize_Left;
		} else if (lasthoverslot == NODES_RightEdge) {
			action = NODES_Resize_Right;
		} else {
			action = NODES_Move_Nodes;
		}
		buttondown.moveinfo(mouse->id, LEFTBUTTON, action, property);

		int overnode = lasthover;

		if (selected.findindex(nodes->nodes.e[overnode]) < 0) {
			 //hovered node not already selected

			if ((state & ShiftMask) == 0) selected.flush();
			selected.push(nodes->nodes.e[overnode]);
		} //else node was already selected
	}

	if (action == NODES_Property) {
		action = NODES_Drag_Property;
		buttondown.moveinfo(mouse->id, LEFTBUTTON, action, property);
	}

	if (action == NODES_Drag_Property) {
		 //we are dragging on a property.. might have to get response from a custom interface
		NodeBase *node = nodes->nodes.e[lasthover];
		NodeProperty *prop = node->properties.e[lasthoverprop];
		Value *v = prop->GetData();
		if (v) {
			//if int, drag by 1s, int real, mult scale?
			// *** need better system to adhere to drag hints

			double drag = x-lx;
			int changed = 0;
			if (dynamic_cast<IntValue*>(v)) {
				double dragscale = 1;
				drag /= dragscale;
				dynamic_cast<IntValue*>(v)->i += drag;
				changed = 1;

			} else if (dynamic_cast<DoubleValue*>(v)) {
				double dragscale = 1;
				drag /= dragscale;
				double d = dynamic_cast<DoubleValue*>(v)->d + drag * ((state & ShiftMask) ? .1 : 1) * ((state & ControlMask) ? .1 : 1);
				if (fabs(d) < 1e-10) d=0;
				dynamic_cast<DoubleValue*>(v)->d = d;
				changed = 1;
			}

			if (changed) {
				node->Update();
				needtodraw=1;
			}
		}

		return 0;

	} else if (action == NODES_Cut_Connections) {
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
				if (selected.e[c]->frame) selected.e[c]->frame->Wrap();
				else {
					//if you move a node over an existing frame, then drop it into the frame
				}
			}
		}
		needtodraw=1;
		return 0;

	} else if (action == NODES_Move_Frame || action == NODES_Frame_Label) {
		if (action == NODES_Frame_Label) {
			action = NODES_Move_Frame;
			buttondown.moveinfo(mouse->id, LEFTBUTTON, action, property);
		}
		if (lasthoverslot>=0 && lasthoverslot<nodes->frames.n) {
			flatpoint d=nodes->m.transformPointInverse(flatpoint(x,y)) - nodes->m.transformPointInverse(flatpoint(lx,ly));

			for (int c=0; c<nodes->frames.e[lasthoverslot]->nodes.n; c++) {
				nodes->frames.e[lasthoverslot]->nodes.e[c]->x += d.x;
				nodes->frames.e[lasthoverslot]->nodes.e[c]->y += d.y;
			}
			nodes->frames.e[lasthoverslot]->Wrap();
		}
		needtodraw=1;
		return 0;

	} else if (action == NODES_Property_Interface) {
		NodeBase *node = nodes->nodes.e[lasthover];
		NodeProperty *prop = node->properties.e[lasthoverprop];

		if (prop->HasInterface()) {
			//dp->PushAxes();
			//dp->NewTransform(nodes->m.m());
			flatpoint p = nodes->m.transformPointInverse(flatpoint(x,y));
			anInterface *interface = prop->PropInterface(NULL, dp);
			interface->MouseMove(p.x,p.y, state,mouse);
			//interface->MouseMove(x,y,state,mouse);
			//dp->PopAxes();
		}

		//hover_action = NODES_None;
		needtodraw=1;
		return 0;

	} else if (action == NODES_Resize_Left || action == NODES_Resize_Right) {	
		if (selected.n && nodes) {
			flatpoint d=nodes->m.transformPointInverse(flatpoint(x,y)) - nodes->m.transformPointInverse(flatpoint(lx,ly));

			for (int c=0; c<selected.n; c++) {
				if (action == NODES_Resize_Left) {
					selected.e[c]->x += d.x;
					selected.e[c]->width -= d.x;
				} else {
					selected.e[c]->width += d.x;
				}

				double th = font->textheight();
				if (selected.e[c]->width < 2*th)  selected.e[c]->width = 2*th;

				selected.e[c]->UpdateLayout();
				selected.e[c]->UpdateLinkPositions();
			}
		}
		needtodraw=1;
		return 0;


	} else if (action == NODES_Drag_Input || action == NODES_Drag_Exec_In) {
		lastpos.x=x; lastpos.y=y;
		needtodraw=1;
		return 0;

	} else if (action == NODES_Drag_Output || action == NODES_Drag_Exec_Out) {
		lastpos.x=x; lastpos.y=y;
		needtodraw=1;
		return 0;

	} else if (action == NODES_Resize_Preview) {
		flatpoint d=nodes->m.transformPointInverse(flatpoint(x,y)) - nodes->m.transformPointInverse(flatpoint(lx,ly));

		NodeBase *node = nodes->nodes.e[lasthover];
		if (selected.findindex(node) < 0) {
			if (state&(ShiftMask|ControlMask)) {
				node->PreviewSample(d.x, d.y, true);
			} else {
				node->preview_area_height += d.y;
				if (node->preview_area_height < node->colors->font->textheight()) node->preview_area_height = node->colors->font->textheight();
			}

			node->UpdatePreview();
			node->Wrap();
		} else {
			for (int c=0; c<selected.n; c++) {
				node = selected.e[c];

				if (state&(ShiftMask|ControlMask)) {
					node->PreviewSample(d.x, d.y, true);
				} else {
					node->preview_area_height += d.y;
					if (node->preview_area_height < node->colors->font->textheight()) node->preview_area_height = node->colors->font->textheight();
				}
				node->UpdatePreview();
				node->Wrap();
			}
		}
		needtodraw=1;
		return 0;
	}


	//needtodraw=1;
	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}

int NodeInterface::MBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	if (!nodes) return 1;
	buttondown.down(d->id, MIDDLEBUTTON, x,y);
	return 0;
}

int NodeInterface::MBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	buttondown.up(d->id, MIDDLEBUTTON);
	if (!nodes) return 1;
	return 0;
}

/*! Intercept shift-right button to drag the scene around, if you are missing a middle button.
 */
int NodeInterface::RBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	if (!nodes || (state&LAX_STATE_MASK)==0) {
		int overnode=-1, overslot=-1, overprop=-1, overconn=-1; 
		overnode = scan(x,y, &overslot, &overprop, &overconn, 0);
		if (overnode >= 0) {
			if (selected.findindex(nodes->nodes.e[overnode]) < 0) {
				selected.flush();
				selected.push(nodes->nodes.e[overnode]);
				needtodraw=1;
			}
		}

		return anInterface::RBDown(x,y,state,count,d);
	}
	buttondown.down(d->id, RIGHTBUTTON, x,y);
	return 0;
}

int NodeInterface::RBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id, RIGHTBUTTON)) return anInterface::RBUp(x,y,state,d);
	buttondown.up(d->id, RIGHTBUTTON);
	return 0;
}

int NodeInterface::WheelUp(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	 //scroll nodes placement
	if (!nodes) return 1;

	nodes->m.Scale(flatpoint(x,y), 1.15);

	 //translate
	//if (state&ShiftMask) nodes->m.m(4, nodes->m.m(4) + .1*(dp->Maxx-dp->Minx));
	//else nodes->m.m(5, nodes->m.m(5) + .1*(dp->Maxy-dp->Miny));

	needtodraw=1;
	return 0;
}

int NodeInterface::WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	 //scroll nodes placement
	if (!nodes) return 1;

	nodes->m.Scale(flatpoint(x,y), .88);
	
	 //translate
	//if (state&ShiftMask) nodes->m.m(4, nodes->m.m(4) - .1*(dp->Maxx-dp->Minx));
	//else nodes->m.m(5, nodes->m.m(5) - .1*(dp->Maxy-dp->Miny));

	needtodraw=1;
	return 0;
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

	if (ch==LAX_Esc) {
		if (!nodes) return 1;
		if (selected.n == 0) {
			 //jump up to parent node
			if (grouptree.n == 0) return 1;
			LeaveGroup();
			return 0;
		}
		selected.flush();
		needtodraw=1;
		return 0;

	} else if (ch=='a') {
		if (!nodes) return 0;

		if (selected.n) {
			selected.flush();
		} else {
			for (int c=0; c<nodes->nodes.n; c++) {
				selected.push(nodes->nodes.e[c]);
			}
		}

		needtodraw=1;
	}

	 //default shortcut processing 
	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch, state&LAX_STATE_MASK, 0);
	if (action != -1) {
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
    sc->Add(NODES_Center,         ' ',0,          0, "Center"        , _("Center"         ),NULL,0);
    sc->Add(NODES_Center_Selected,' ',ShiftMask,  0, "CenterSelecetd", _("Center Selected"),NULL,0);
    sc->Add(NODES_Group_Nodes,    'g',ControlMask,0, "GroupNodes"    , _("Group Nodes"    ),NULL,0);
    sc->Add(NODES_Ungroup_Nodes,  'G',ShiftMask|ControlMask,0, "UngroupNodes" , _("Ungroup Nodes"),NULL,0);
    sc->Add(NODES_Add_Node,       'A',ShiftMask,  0, "AddNode"       , _("Add Node"       ),NULL,0);
    sc->Add(NODES_Delete_Nodes,   LAX_Bksp,0,     0, "DeleteNode"    , _("Delete Node"    ),NULL,0);
	sc->AddShortcut(LAX_Del,0,0,  NODES_Delete_Nodes);
    sc->Add(NODES_Toggle_Collapse,'c',0,          0, "ToggleCollapse", _("ToggleCollapse" ),NULL,0);
    sc->Add(NODES_TogglePreview,  'p',0,          0, "TogglePreview",  _("TogglePreview" ), NULL,0);
    sc->Add(NODES_Frame_Nodes,    'f',0,          0, "Frame",          _("Frame Selected" ),NULL,0);
    sc->Add(NODES_Unframe_Nodes,  'F',ShiftMask,  0, "Unframe",        _("Remove connected frame" ),NULL,0);
    sc->Add(NODES_Edit_Group,     LAX_Tab,0,      0, "EditGroup",      _("Toggle Edit Group"),NULL,0);
    sc->Add(NODES_Leave_Group,    LAX_Tab,ShiftMask,0,"LeaveGroup",    _("Leave Group")    ,NULL,0);
    sc->Add(NODES_Thread_Controls,'e',0,          0, "ThreadControls", _("Thread controls"),NULL,0);
    sc->Add(NODES_Find,           'f',ControlMask,0, "Find",           _("Find a node"),NULL,0);
    sc->Add(NODES_Find_Next,      'f',ShiftMask|ControlMask,0, "FindNext", _("Find next node"),NULL,0);

    sc->Add(NODES_Duplicate,      'D',ShiftMask,  0, "Duplicate"     , _("Duplicate")      ,NULL,0);

    sc->Add(NODES_No_Overlap,     'o',0,          0, "NoOverlap"     , _("NoOverlap"          ),NULL,0);
    sc->Add(NODES_Arrange_Grid,   'g',0,          0, "ArrangeGrid"   , _("Arrange in a grid"  ),NULL,0);
    sc->Add(NODES_Arrange_Row,    'h',0,          0, "ArrangeRow"    , _("Arrange in a row"   ),NULL,0);
    sc->Add(NODES_Arrange_Column, 'v',0,          0, "ArrangeCol"    , _("Arrange in a column"),NULL,0);

    sc->Add(NODES_Save_Nodes,      's',0,  0, "SaveNodes"      , _("Save Nodes"     ),NULL,0);
    sc->Add(NODES_Load_Nodes,      'l',0,  0, "LoadNodes"      , _("Load Nodes"     ),NULL,0);

    manager->AddArea(whattype(),sc);
    return sc;

}

int NodeInterface::PerformAction(int action)
{
	if (action==NODES_Group_Nodes) {
		if (selected.n) {
			NodeGroup *newgroup = nodes->GroupNodes(selected);
			selected.push(newgroup);
			PostMessage(_("Grouped."));
			needtodraw = 1;
		}
		return 0;

	} else if (action==NODES_Ungroup_Nodes) {
		if (selected.n) {
			int n = nodes->UngroupNodes(selected, true);
			if (n) PostMessage(_("Ungrouped."));
			else PostMessage(_("No groups selected!"));
			needtodraw=1;
		}
		return 0;

	} else if (action==NODES_Edit_Group) {
		if (selected.n == 1 && dynamic_cast<NodeGroup*>(selected.e[0]) != NULL) {
			EnterGroup(dynamic_cast<NodeGroup*>(selected.e[0]));
		} else LeaveGroup();
		return 0;

	} else if (action==NODES_Leave_Group) {
		LeaveGroup();
		return 0;

	} else if (action==NODES_Delete_Nodes) {
		DBG cerr << "delete nodes..."<<endl;
		if (!nodes || selected.n == 0) return 0;

		for (int c=0; c<selected.n; c++) {
			int tt = IsThread(selected.e[c])-1;
			if (tt >= 0) threads.remove(tt);
		}

		if (nodes->DeleteNodes(selected))
			PostMessage(_("Deleted."));
		else
			PostMessage(_("Can't delete."));
		needtodraw=1;
		return 0;

	} else if (action==NODES_Add_Node) {
		 //Pop up menu to select new node..
		if (lastpos.x == 0 && lastpos.y == 0) {
			int mx=-1,my=-1;
			int status=mouseposition(0, curwindow, &mx, &my, NULL, NULL, NULL);
			if (status!=0 || mx<0 || mx>curwindow->win_w || my<0 || my>curwindow->win_h) {
				mx=curwindow->win_w/2;
				my=curwindow->win_h/2;
			}
			lastpos.set(mx,my);
		}

		if (!node_menu) RebuildNodeMenu();
		//clean up from last time
		node_menu->SetRecursively(LAX_OFF, 1);
		node_menu->SetRecursively(LAX_ON, 0);
		node_menu->SetRecursively(MENU_SELECTED, 0);
		node_menu->ClearSearch();

        PopupMenu *popup=new PopupMenu(NULL,_("Add node..."), 0,
                        0,0,0,0, 1,
                        object_id,"addnode",
                        0, //mouse to position near?
                        node_menu,0, NULL,
                        TREESEL_LEFT|TREESEL_SEND_PATH|TREESEL_LIVE_SEARCH|TREESEL_SUB_ON_RIGHT);
        popup->pad=5;
        popup->WrapToMouse(0);
        app->rundialog(popup);

		return 0;

	} else if (action==NODES_Duplicate) {
		DuplicateNodes();
		return 0;

	} else if (action==NODES_Center || action==NODES_Center_Selected) {
		if (!nodes) return 0;
		SomeData box;
		NodeBase *node;

		Laxkit::RefPtrStack<NodeBase> *nn;
		if (action == NODES_Center_Selected && selected.n>0) nn = &selected;
		else nn = &nodes->nodes;

		for (int c=0; c<nn->n; c++) {
			node = nn->e[c];
			box.addtobounds(node->x, node->y);
			box.addtobounds(node->x + node->width, node->y + node->height);
		}

		double w = dp->Maxx-dp->Minx, h = dp->Maxy-dp->Miny;
		double margin = (w < h ? w*.05 : h*.05);

		DoubleBBox vp(dp->Minx+margin, dp->Maxx-margin, dp->Miny+margin, dp->Maxy-margin);
		box.fitto(NULL, &vp, 50, 50, 1);
		nodes->m.m(box.m());
		needtodraw=1;
		return 0;

	} else if (action==NODES_Find) {
		DoubleBBox bounds;
		double th = dp->textheight();
		double w = th * 20;
		bounds.addtobounds(dp->Maxx/2 - w/2, dp->Maxy/2 - th*.6);
		bounds.addtobounds(dp->Maxx/2 + w/2, dp->Maxy/2 + th*.6);

		viewport->SetupInputBox(object_id, _("Find node"),
				search_term,
				"search",
				bounds,
				NULL, false);
		return 0;

	} else if (action==NODES_Find_Next) {
		FindNext();
		return 0;

	} else if (action==NODES_Frame_Nodes) {
		if (!selected.n) {
			PostMessage(_("No nodes selected!"));
			return 0;
		}
		NodeFrame *frame = new NodeFrame(nodes, _("Frame"), NULL);
		for (int c=0; c<selected.n; c++) {
			frame->AddNode(selected.e[c]); //will remove any previous frame connection node has
		}
		frame->Wrap();
		nodes->frames.push(frame);
		frame->dec_count();
		needtodraw=1;
		return 0;

	} else if (action==NODES_Unframe_Nodes) {
		if (!selected.n) {
			PostMessage(_("No nodes selected!"));
			return 0;
		}
		for (int c=0; c<selected.n; c++) {
			if (selected.e[c]->frame) {
				nodes->frames.remove(selected.e[c]->frame);
			}
		}
		needtodraw=1;
		return 0;

	} else if (action==NODES_Toggle_Collapse) {
		ToggleCollapsed();
		needtodraw=1;
		return 0;

	} else if (action==NODES_TogglePreview) {
		if (!selected.n) return 0;
		bool show = selected.e[0]->show_preview;
		if (selected.e[0]->show_preview && !selected.e[0]->total_preview) {
			 //maybe somehow preview was not initially generated
			selected.e[0]->UpdatePreview();
			if (selected.e[0]->total_preview) {
				selected.e[0]->Wrap();
				show = true;
			}
		}

		if (show) return PerformAction(NODES_Hide_Previews);
		return PerformAction(NODES_Show_Previews);;

	} else if (action==NODES_Show_Previews) {
		for (int c=0; c<selected.n; c++) {
			selected.e[c]->show_preview = true;
			selected.e[c]->UpdatePreview();
			selected.e[c]->Wrap();
		}
		needtodraw=1;
		return 0;

	} else if (action==NODES_Hide_Previews) {
		for (int c=0; c<selected.n; c++) {
			selected.e[c]->show_preview = false;
			selected.e[c]->Wrap();
		}
		needtodraw=1;
		return 0;

	} else if (action==NODES_No_Overlap) {
		if (!nodes) return 0;
		for (int c=0; c<selected.n; c++) {
			nodes->NoOverlap(selected.e[c], 2*nodes->colors->font->textheight());
		}
		needtodraw=1;
		return 0;

	} else if (action==NODES_Arrange_Grid) {
		 //somewhat hamfistedly arrange in a grid
		if (!selected.n) return 0;
		int side = sqrt(selected.n);

		double w=0, h=0;
		for (int c=0; c<selected.n; c++) {
			if (selected.e[c]->width  > w) w = selected.e[c]->width;
			if (selected.e[c]->height > h) h = selected.e[c]->height;
		}
		w *= 1.1;
		h *= 1.1;
		double x0 = selected.e[0]->x;
		double y0 = selected.e[0]->y;
		double xx = x0, yy=y0;
		int i = 0;

		for (int y=0; y<side+2; y++) {
			xx = x0;
			for (int x=0; x<side; x++) {
				if (i >= selected.n) break;
				selected.e[i]->x = xx;
				selected.e[i]->y = yy;
				xx += w;
				i++;
			}
			yy += h;
		}
		
		needtodraw=1;
		return 0;

	} else if (action==NODES_Arrange_Row) {
		if (!selected.n) return 0;

		double x0 = selected.e[0]->x;
		double y0 = selected.e[0]->y;
		double th = nodes->colors->font->textheight();

		double xx = x0;
		for (int c=0; c<selected.n; c++) {
			selected.e[c]->x = xx;
			selected.e[c]->y = y0;
			xx += selected.e[c]->width + th;
		}

		needtodraw=1;
		return 0;

	} else if (action==NODES_Arrange_Column) {
		if (!selected.n) return 0;

		double x0 = selected.e[0]->x;
		double y0 = selected.e[0]->y;
		double th = nodes->colors->font->textheight();

		double yy = y0;
		for (int c=0; c<selected.n; c++) {
			selected.e[c]->x = x0;
			selected.e[c]->y = yy;
			yy += selected.e[c]->height + th;
		}

		needtodraw=1;
		return 0;

	} else if (action == NODES_Save_Nodes) {
		if (!nodes) return 0;
		app->rundialog(new FileDialog(NULL,"Save nodes",_("Save nodes"),ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
						                  object_id,"savenodes",FILES_SAVE, lastsave));
		return 0;

	} else if (action == NODES_Load_Nodes) {
		app->rundialog(new FileDialog(NULL,"Load nodes",_("Load Nodes"),ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
	                                          object_id,"loadnodes",FILES_OPEN_ONE, lastsave));
		return 0;

	} else if (action == NODES_Save_With_Loader || action == NODES_Load_With_Loader) {
		ObjectIO *loader = (lastmenuindex >=0 && lastmenuindex < NodeGroup::loaders.n ? NodeGroup::loaders.e[lastmenuindex] : NULL);
		if (!loader) {
			PostMessage(_("Could not find loader."));
			return 0;
		}

		char scratch[500];
		if (action == NODES_Save_With_Loader) {
			if (!nodes) return 0;
			sprintf(scratch, _("Save with %s..."), loader->VersionName());
			app->rundialog(new FileDialog(NULL,"Save nodes", scratch, ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
										  object_id, "savewithloader", FILES_SAVE, lastsave));
		} else {
			sprintf(scratch, _("Load with %s..."), loader->VersionName());
			app->rundialog(new FileDialog(NULL, "Load nodes", scratch, ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
										  object_id, "loadwithloader", FILES_OPEN_ONE, lastsave));
		}
		return 0;

	} else if (action == NODES_Thread_Controls) {
		show_threads = !show_threads;
		needtodraw=1;
		return 0;
	}

	return 1;
}

int NodeInterface::FreshNodes(bool asresource)
{
	selected.flush();
	grouptree.flush();
	lasthover = lasthoverslot = lasthoverprop = lastconnection = -1;

	if (nodes) nodes->dec_count();

	nodes = new Nodes;
	nodes->InstallColors(new NodeColors, true);
	nodes->colors->Font(font, false);

	if (asresource) {
		ResourceManager *manager = InterfaceManager::GetDefault(true)->GetResourceManager();
		manager->AddResource("Nodes", nodes, NULL, nodes->Id(), nodes->Label(), NULL, NULL, NULL);
	}

	return 0;
}

/*! Completely replaces exists nodes, unless append is true.
 */
int NodeInterface::LoadNodes(const char *file, bool append)
{
	if (!append) {
		if (nodes) {
			nodes->dec_count();
			nodes = NULL;
		}
	}

	if (!nodes) FreshNodes(true);

	FILE *f = fopen(file, "r");

	if (!f) {
		 //error!
		PostMessage(_("Could not open nodes file!"));
		DBG cerr <<(_("Could not open nodes file!")) << file<<endl;
		return 1;
	}

	int oldn = nodes->nodes.n;
	char first500[500];
	int num = fread(first500,1,499, f);
	first500[num] = '\0';
	rewind(f);

	bool success = false;
	ErrorLog log;
	if (strstr(first500, "#Laidout") != first500 || strstr(first500, "Nodes") == NULL) {
		 //does not appear to be a laidout node file.
		 //try with loaders
		fclose(f);
		f = NULL;

		if (NodeGroup::loaders.n) {
			for (int c=0; c<NodeGroup::loaders.n; c++) {
				if (NodeGroup::loaders.e[c]->CanImport(file, first500)) {
					anObject *obj_ret = NULL;
					ErrorLog log;
					int oldn = nodes->nodes.n;

					int status = NodeGroup::loaders.e[c]->Import(file, &obj_ret, nodes, log);
					if (status == 0) {
						for (int c=oldn; c < nodes->nodes.n; c++) {
							if (!nodes->nodes.e[c]->colors) {
								nodes->nodes.e[c]->InstallColors(nodes->colors, false);
							}

							if (nodes->nodes.e[c]->width <= 0) nodes->nodes.e[c]->Wrap();
						}

						success = true;
						break;
					}
				}
			}
		}

	} else {
		 //normal laidout nodes file
		DumpContext context(NULL,1, object_id);
		context.log = &log;

		nodes->dump_in(f, 0, 0, &context, NULL);
		success = true;
		if (f) fclose(f);
	}

	if (success) {
		 //replace current selection, and position 
		selected.flush();
		DoubleBBox box;
		for (int c=oldn; c<nodes->nodes.n; c++) {
			selected.pushnodup(nodes->nodes.e[c]);
			box.addtobounds(*nodes->nodes.e[c]);
		}
		flatpoint p = nodes->m.transformPointInverse(flatpoint((dp->Maxx+dp->Minx)/2, (dp->Maxy+dp->Miny)/2));
		p -= box.BBoxPoint(.5,.5);
		for (int c= oldn; c<nodes->nodes.n; c++) {
			nodes->nodes.e[c]->x += p.x;
			nodes->nodes.e[c]->y += p.y;
		}

		FindThreads(true);

		PostMessage(_("Loaded."));
		DBG cerr << _("Loaded.") <<endl;

	} else {
		PostMessage(_("Couldn't load nodes"));
	}

	NotifyGeneralErrors(&log);
	needtodraw=1;
	return success ? 0 : 1;
}

int NodeInterface::SaveNodes(const char *file)
{
	FILE *f = fopen(file, "w");
	if (f) {
		DumpContext context(NULL,1, object_id);
		ErrorLog log;
		context.log = &log;

		fprintf(f,"#Laidout %s Nodes\n",LAIDOUT_VERSION);
		nodes->dump_out(f, 0, 0, &context);
		fclose(f);

		PostMessage2(_("Nodes saved to %s."), file);
		DBG cerr << _("Nodes saved.") <<endl;

		NotifyGeneralErrors(&log);
		return 0;
	} 

	 //else error!
	PostMessage(_("Could not open nodes file!"));
	DBG cerr <<(_("Could not open nodes file!")) << file<< endl;
	return 1;
}

int NodeInterface::ToggleCollapsed()
{
	int collapsed = 1;
	for (int c=0; c<selected.n; c++) {
		if (c==0) collapsed = selected.e[0]->collapsed;
		selected.e[c]->Collapse(!collapsed);
	}

	needtodraw=1;
	return 0;
}

/*! Return 0 for success, or 1 for couldn't enter group.
 */
int NodeInterface::EnterGroup(NodeGroup *group)
{
	if (group == NULL && selected.n == 1 && dynamic_cast<NodeGroup*>(selected.e[0])) group = dynamic_cast<NodeGroup*>(selected.e[0]);
	if (!group) return 1;

	grouptree.push(nodes);
	nodes->dec_count();
	nodes = group;
	group->inc_count();

	selected.flush();
	lasthover = lasthoverslot = lasthoverprop = lastconnection = -1;
	needtodraw=1;
	return 0;
}

/*! Jump out of current group to parent group, if any.
 *
 * Return 0 for left, 1 for nothing to leave.
 */
int NodeInterface::LeaveGroup()
{
	if (grouptree.n == 0) return 1;
	selected.flush();
	selected.push(nodes);
	nodes->dec_count();
	nodes = grouptree.pop();
	lasthover = lasthoverslot = lasthoverprop = lastconnection = -1;
	needtodraw=1;
	return 0;
}

/*! Return number of connections cut.
 */
int NodeInterface::CutConnections(flatpoint p1,flatpoint p2)
{
	if (!nodes) return 0;

	int n = 0;

	NodeBase *node;
	NodeProperty *prop;
	NodeConnection *connection;
	flatpoint p;

	for (int c=0; c<nodes->nodes.n; c++) {
		node = nodes->nodes.e[c];

		for (int c2=0; c2<node->properties.n; c2++) {
			prop = node->properties.e[c2];
			if (!prop->IsInput() && !prop->IsExecOut()) continue;

			for (int c3=prop->connections.n-1; c3>=0; c3--) {
				connection = prop->connections.e[c3];
				GetConnectionBez(connection, bezbuf);

				if (bez_intersection(p1,p2, 0, bezbuf[0],bezbuf[1],bezbuf[2],bezbuf[3], 7, &p,NULL)) {
					nodes->Disconnect(connection, false, false);
				}
			}
		}
	}

	return n;
}

/*! Copy any links others in selected.
 *  deep copy groups.
 */
int NodeInterface::DuplicateNodes()
{
	if (!selected.n) { PostMessage(_("No nodes selected!")); return 0; }

	double offset = nodes->colors->font->textheight();

	int top = nodes->nodes.n;
	RefPtrStack<NodeBase> newnodes;
	for (int c=0; c<selected.n; c++) {
		NodeBase *newnode = selected.e[c]->Duplicate();
		if (!newnode) {
			cerr << " *** warning! Duplicate() not implemented for "<<selected.e[c]->Type()<<endl;
			continue;
		}

		if (!newnode->colors) newnode->InstallColors(selected.e[c]->colors, 0);
		newnode->Wrap();
		newnode->x += offset;
		newnode->y += offset;
		newnodes.push(newnode);
		nodes->AddNode(newnode);
		// *** make name unique
		newnode->dec_count();
	}

	// *** connect links

	selected.flush();
	for (int c=0; c<newnodes.n; c++) {
		for (int c2=0; c2<top; c2++) {
			if (!strcmp(newnodes.e[c]->Id(), nodes->nodes.e[c2]->Id())) {
				//name already exists
				char *s = increment_file(newnodes.e[c]->object_idstr);
				delete[] newnodes.e[c]->object_idstr;
				newnodes.e[c]->object_idstr = s;
				c2 = -1;
			}
		}
		top++;
		selected.push(newnodes.e[c]);
	}

	needtodraw=1;
	return 1;
}


//--------------------------NodeUndo --------------------------------
/*! \class NodeUndo
 * Undo for NodeBase related changes.
 */

/*! Takes mm and nstuff. WILL delete[] mm in destructor.
 * Does NOT increment count on nstuff, but WILL decrement in destructor.
 */
NodeUndo::NodeUndo(int ntype, int nisauto, double *mm, anObject *nstuff)
 : UndoData(nisauto)
{
	type = ntype;
	m = mm;
	stuff = nstuff;
}

NodeUndo::~NodeUndo()
{
	delete[] m;
	if (stuff) stuff->dec_count();
}

const char *NodeUndo::Description()
{
	if      (type == Moved)          return _("Moved"         );
	else if (type == Scaled)         return _("Scaled"        );
	else if (type == Added)          return _("Added"         );
	else if (type == Deleted)        return _("Deleted"       );
	else if (type == Grouped)        return _("Grouped"       );
	else if (type == Ungrouped)      return _("Ungrouped"     );
	else if (type == Resized)        return _("Resized"       );
	else if (type == Connected)      return _("Connected"     );
	else if (type == Disconnected)   return _("Disconnected"  );
	else if (type == Framed)         return _("Framed"        );
	else if (type == Unframed)       return _("Unframed"      );
	else if (type == PropertyChange) return _("PropertyChange");

	return NULL;
}

} // namespace Laidout

