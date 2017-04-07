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



#include "nodeinterface.h"

#include <lax/laxutils.h>
#include <lax/bezutils.h>
#include <lax/popupmenu.h>
#include <lax/colorsliders.h>
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
	default_property(1.,1.,1.,1.),
	connection(.5,.5,.5,1.),
	sel_connection(1.,0.,1.,1.),

	label_fg(.2,.2,.2,1.),
	label_bg(.7,.7,.7,1.),
	fg(.2,.2,.2,1.),
	bg(.8,.8,.8,1.),
	text(0.,0.,0.,1.),
	border(.2,.2,.2,1.),
	error_border(.5,.0,.0,1.),

	fg_edit(.2,.2,.2,1.),
	bg_edit(.9,.9,.9,1.),

	fg_menu(.2,.2,.2,1.),
	bg_menu(.7,.7,.7,1.),

	selected_border(1.,.8,.1,1.),
	selected_bg(.9,.9,.9,1.),

	mo_diff(.05)
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
 *
 * Note connections are not reference counted.
 * This is ok since the connections only exist in relation to the nodes, and nodes don't get stranded
 * when a connection deletes itself, as they are in a list in a NodeGroup.
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

/*! Remove connection refs from any connected props.
 */
NodeConnection::~NodeConnection()
{
	if (fromprop) fromprop->connections.remove(fromprop->connections.findindex(this));
	if (toprop)   toprop  ->connections.remove(toprop  ->connections.findindex(this));
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
	modtime=0;
	width=height=0;

	type = PROP_Unknown;
	is_linkable=false; //default true for something that allows links in
}

/*! If !absorb_count, then ndata's count gets incremented.
 */
NodeProperty::NodeProperty(PropertyTypes input, bool linkable, const char *nname, Value *ndata, int absorb_count)
{
	color.rgbf(1.,1.,1.,1.);

	owner=NULL;
	width=height=0;
	data=ndata;
	if (data && !absorb_count) data->inc_count();

	type = input;
	is_linkable = linkable;

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

/*! 0=no, -1=prop is connected input, >0 == how many connected output
 */
int NodeProperty::IsConnected()
{
	if (IsInput()) return -connections.n;
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
 */
Value *NodeProperty::GetData()
{
	 //note: this assumes fromprop is a pure output, not a through
	if (IsInput() && connections.n && connections.e[0]->fromprop) return connections.e[0]->fromprop->data;
	return data;
}

/*! Returns 1 for successful setting, or 0 for not set (or absorbed).
 */
int NodeProperty::SetData(Value *newdata, bool absorb)
{
	if (newdata==data) return 1;
	if (data) data->dec_count();
	data = newdata;
	if (data && !absorb) data->inc_count();
	return 1;
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

	type = NULL;
	def = NULL;
}

NodeBase::~NodeBase()
{
	delete[] Name;
	delete[] type;
	if (def) def->dec_count();
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

enum NodePropValidEnum {
	NVALID_Min           = (1<<0),
	NVALID_Min_Exclusive = (1<<1),
	NVALID_Max           = (1<<2),
	NVALID_Max_Exclusive = (1<<3),
	NVALID_Types         = (1<<4),
	NVALID_MAX
};
/*! \class NodePropValid
 * Class to ease checking validity of property values for common situations.
 */
class NodePropValid
{
  public:
	int type; //[], (], [), (), ..], ..), [.., (.., particular type(s)
	double min, max;
	PtrStack<char> types;
	char *error;
	NodePropValid() { error=NULL; }
	virtual ~NodePropValid() { delete[] error; }
};

/*! Return whether the node has valid values, or the outputs are older than inputs.
 * Default is to return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 *
 * Default placeholder behavior is to return 1 if any output property has modtime less than
 * any input modtime. Else return 0. Thus subclasses need only redefine to catch error states.
 */
int NodeBase::GetStatus()
{ 
	 //find newest mod time of inputs
	std::time_t t=0;
	for (int c=0; c<properties.n; c++) {
		if (!properties.e[c]->IsOutput() && properties.e[c]->modtime > t) t = properties.e[c]->modtime;
	}
	if (t==0) return 0; //no inputs!

	 //if any outputs older than newest mod, then return 1
	for (int c=0; c<properties.n; c++) {
		if (properties.e[c]->IsOutput()) continue;
		if (properties.e[c]->modtime < t) return 1; 
	}
	return 0;
}

/*! Call whenever any of the inputs change, update outputs.
 * Default placeolder is to trigger update in connected outputs.
 * Subclasses should redefine to actually update the outputs based on the inputs
 * or any other internal state, as well as the overall preview (if any).
 *
 * Returns GetStatus().
 */
int NodeBase::Update()
{
	NodeProperty *prop;
	for (int c=0; c<properties.n; c++) {
		prop = properties.e[c];

		if (prop->IsInput()) continue;
		if (prop->connections.n==0) continue;

		for (int c2=0; c2<prop->connections.n; c2++) {
			if (prop->connections.e[c2]->to)
				prop->connections.e[c2]->to->Update();
		}
	}
	return GetStatus();
}

/*! Update the bounds to be just enough to encase everything.
 */
int NodeBase::Wrap()
{
	if (!colors) return -1;

	 //find overall width and height
	double th = colors->font->textheight();

	height = th*1.5;
	width = colors->font->extent(Name,-1);

	double w;
	Value *v;
	NodeProperty *prop;
	for (int c=0; c<properties.n; c++) {
		prop = properties.e[c];

		w=colors->font->extent(prop->Name(),-1);

		v=dynamic_cast<Value*>(prop->data);
		if (v) {
			if (v->type()==VALUE_Real || v->type()==VALUE_Int) {
				w+=3*th;

			} else if (v->type()==VALUE_Color) {
				w+=3*th;

			} else if (v->type()==VALUE_Enum) {
				EnumValue *ev = dynamic_cast<EnumValue*>(v);
				const char *nm=NULL;
				double ew=0, eww;
				for (int c=0; c<ev->GetObjectDef()->getNumEnumFields(); c++) {
					ev->GetObjectDef()->getEnumInfo(c, NULL, &nm);
					if (isblank(nm)) continue;
					eww = colors->font->extent(nm,-1);
					if (eww>ew) ew=eww;
				}
				w += ew;
			}

			prop->x=0;
			prop->width = w;
			prop->height = 1.5*th;

		} else {
			if (prop->height==0) prop->height = 1.5*th;
		}

		if (w>width) width=w;
	}

	width += 3*th;

	 //update link position
	double y=1.5*th;
	for (int c=0; c<properties.n; c++) {
		prop = properties.e[c];
		prop->y = y;
		prop->pos.y = y+prop->height/2;

		if (prop->IsInput()) prop->pos.x = 0;
		else prop->pos.x = width;

		y+=prop->height;
		height += prop->height;
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

/*! 0=no, -1=prop is connected input, 1=connected output
 */
int NodeBase::IsConnected(int propindex)
{
	if (propindex<0 || propindex>=properties.n) return -1;
	return properties.e[propindex]->IsConnected();
}

/*! Return the property index of the first property that has a connection to prop.
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

/*! This function aids dump_in_atts. Default will handle any builtin Value types, except enums.
 * Subclasses should redefine to catch these.
 *
 * Return 1 for property set, 0 for could not set.
 */
int NodeBase::SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att)
{
	if (att->attributes.n == 0) return 0;

	NodeProperty *prop = NULL;
	for (int c=0; c<properties.n; c++) {
		if (!strcmp(propname, properties.e[c]->name)) {
			prop = properties.e[c];
			break;
		}
	}
	if (!prop) return 0;

	Value *val = AttributeToValue(att->attributes.e[0]);
	if (val) {
		if (!prop->SetData(val, true)) {
			val->dec_count();
			return 0;
		}
	}

	return 1;
}

//-------------------------------------- NodeGroup --------------------------
/*! \class NodeGroup
 * Class to hold a collection of nodes, and optionally a designated output node.
 */

/*! Static keeper of default node factory.
 */
Laxkit::SingletonKeeper NodeGroup::nodekeeper;

/*! Return the current default node factory. If the default is null, then make a new default if create==true.
 */
Laxkit::ObjectFactory *NodeGroup::NodeFactory(bool create)
{
	ObjectFactory *node_factory = dynamic_cast<ObjectFactory*>(nodekeeper.GetObject());
	if (!node_factory && create) {
		node_factory = new ObjectFactory; 
		nodekeeper.SetObject(node_factory, true);

		SetupDefaultNodeTypes(node_factory);
	}

	return node_factory;
}

/*! Install newnodefactory. If it is null, then remove the default. Else inc count on it (and install as default).
 */
void NodeGroup::SetNodeFactory(Laxkit::ObjectFactory *newnodefactory)
{
	nodekeeper.SetObject(newnodefactory, false);
}

NodeGroup::NodeGroup()
{
	background.rgbf(0,0,0,.5);
	output=NULL;
}

NodeGroup::~NodeGroup()
{
	if (output) output->dec_count();
}

/*! Install noutput as the group's pinned output.
 * Basically just dec_count the old, inc_count the new.
 * It is assumed noutput is in the nodes stack already.
 *
 * If you pass in noutput, then it is assumed you don't want a designated output.
 */
int NodeGroup::DesignateOutput(NodeBase *noutput)
{
	if (output) output->dec_count();
	output=noutput;
	if (output) output->inc_count();
	return 0;
}

/*! Delete any nodes and related connections of any in selected.
 * Does not modify selected.
 */
int NodeGroup::DeleteNodes(Laxkit::RefPtrStack<NodeBase> &selected)
{
	int numdel = 0;
	NodeBase *node;

	for (int c=selected.n-1; c>=0; c--) {
		node = selected.e[c];
		if (!node->deletable) continue;

		for (int c2=connections.n-1; c2>=0; c2--) {
			if (connections.e[c2]->from == node || connections.e[c2]->to == node) {
				connections.remove(c2);
			}
		}

		nodes.remove(nodes.findindex(node));
		selected.remove(c);
		numdel++;
	}

	return numdel;
}

/*! Return 1 for success, or 0 for failure.
 * If usethis != NULL, then use that as the connection object, overwriting any incorrect settings.
 * Otherwise create a new one to install.
 */
int NodeGroup::Connect(NodeProperty *from, NodeProperty *to, NodeConnection *usethis)
{
	//***

	return 0;
}

/*! Use when connecting forward to node via connection. Traverse forwards through connection,
 * and node should not be found. Return 0 if not found, or 1 for found.
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
		if (prop->IsInput()) continue;

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
        att->push("matrix", "screen matrix to use");
        att->push("background", "rgb(.1,.2,.3) #color of the background for this group of nodes");
        att->push("output", "which_one #id of the node designated as non-deletable output for this group, if any");
        att->push("nodes", "#list of individual nodes in this group");
        att->push("connections", "#list of connections between the nodes");
        //att->push("", "");
        return att;
    }

    att->push("id", Id()); 

	const double *matrix=m.m();
    char s[100];
    sprintf(s,"%.10g %.10g %.10g %.10g %.10g %.10g",
            matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
    att->push("matrix", s);

	if (output) att->push("output", output->Id());


	Attribute *att2, *att3;
	for (int c=0; c<nodes.n; c++) {
		NodeBase *node = nodes.e[c];
		
		att2 = att->pushSubAtt("node", node->Type());
		att2->push("id", node->Id());

		sprintf(s,"%.10g %.10g %.10g %.10g", node->x,node->y,node->width,node->height);
		att2->push("xywh", s);

		if (node->collapsed) att2->push("collapsed");

		//NodeColors *colors;
	
		 //properties
		NodeProperty *prop;
		for (int c2=0; c2<node->properties.n; c2++) { 
			prop = node->properties.e[c2]; 
			att3 = NULL;

			if (prop->IsInput()) {
				//provide for reference if connected...
				//if (prop->IsConnected()) continue; //since it'll be recomputed after read in
				att3 = att2->pushSubAtt("in", prop->name);
			} else if (prop->IsOutput()) att3 = att2->pushSubAtt("out", prop->name); //just provide for reference
			else if (prop->IsBlock()) att3 = att2->pushSubAtt("block", prop->name);
			else continue;

			if (prop->GetData()) {
				prop->GetData()->dump_out_atts(att3, what, context);
			} else att3->push(prop->name, "arrrg! todo!");
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

	return att;
}

void NodeGroup::dump_in_atts(Attribute *att,int flag,DumpContext *context)
{
	if (!att) return;

    char *name,*value;
	const char *out=NULL;
	Attribute *conatt=NULL;

    for (int c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

        if (!strcmp(name,"id")) {
            if (!isblank(value)) Id(value);

        } else if (!strcmp(name,"matrix")) {
			double mm[6];
			DoubleListAttribute(value,mm,6);
			m.m(mm);

        } else if (!strcmp(name,"output")) {
			out = value;

        } else if (!strcmp(name,"node")) {
			 //value is the node type
			if (isblank(value)) continue;

			NodeBase *newnode = NewNode(value);
			if (!newnode) continue;

    		for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name= att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;

				if (!strcmp(name,"id")) {
					newnode->Id(value);

				} else if (!strcmp(name,"xywh")) {
					double xywh[4];
					int l = DoubleListAttribute(value,xywh,4);
					if (l==4) {
						newnode->x = xywh[0];
						newnode->y = xywh[1];
						newnode->width  = xywh[2];
						newnode->height = xywh[3];
					}

				} else if (!strcmp(name,"x")) {
					DoubleAttribute(value, &newnode->x);

				} else if (!strcmp(name,"y")) {
					DoubleAttribute(value, &newnode->y);

				} else if (!strcmp(name,"width")) {
					DoubleAttribute(value, &newnode->width);

				} else if (!strcmp(name,"height")) {
					DoubleAttribute(value, &newnode->height);

				} else if (!strcmp(name,"collapsed")) {
					newnode->collapsed = BooleanAttribute(value);

				} else if (!strcmp(name,"in") || !strcmp(name,"out")) {
					Attribute *v = att->attributes.e[c]->attributes.e[c2];
					if (!v) continue;
					newnode->SetPropertyFromAtt(value, v); 
					//Value *val = AttributeToValue(v);
					//if (val) {
					//	if (!newnode->SetProperty(value, val, true)) val->dec_count();
					//}
				}
			}

			if (!newnode->colors) newnode->InstallColors(colors, false);
			newnode->Wrap();
			nodes.push(newnode);
			newnode->dec_count();

        } else if (!strcmp(name,"connections")) {
			conatt = att->attributes.e[c];
		}
	} 

	if (out) {
		//set designated output
		NodeBase *node = FindNode(out);
		if (node) DesignateOutput(node);
	}

	if (conatt) {
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
				NodeProperty *fromprop = from->FindProperty(fpstr),
							 *toprop   = to  ->FindProperty(tpstr);

				delete[] fromstr;
				delete[] tostr;
				delete[] fpstr;
				delete[] tpstr;

				if (!from || !to || !fromprop || !toprop) {
					// *** warning!
					DBG cerr <<" *** bad node att input!"<<endl;
					continue;
				}


				NodeConnection *newcon = new NodeConnection(from, to, fromprop, toprop);
				fromprop->connections.push(newcon, 0);//prop list does NOT delete connection
				toprop  ->connections.push(newcon, 0);//prop list does NOT delete connection
				connections.push(newcon, 1);//node connection list DOES delete connection
			}
		}
	}

}

/*! Create and return a new fresh node object, unconnected to anything.
 */
NodeBase *NodeGroup::NewNode(const char *type)
{
	ObjectFactory *factory = NodeFactory();
	ObjectFactoryNode *fnode;

	for (int c=0; c < factory->types.n; c++) {
		fnode = factory->types.e[c];

		if (!strcmp(fnode->name, type)) {
			anObject *obj = fnode->newfunc(NULL);
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

NodeBase *NodeGroup::FindNode(const char *name)
{
	if (!name) return NULL;

	for (int c=0; c<nodes.n; c++) {
		if (!strcmp(name, nodes.e[c]->Id())) return nodes.e[c];
	}
	return NULL;
}


//-------------------------------------- Common Node Types --------------------------


//------------ DoubleNode

Laxkit::anObject *newDoubleNode(Laxkit::anObject *ref)
{
	NodeBase *node = new NodeBase;
	//node->Id("Value");
	makestr(node->Name, _("Value"));
	makestr(node->type, "Value");
	node->properties.push(new NodeProperty(NodeProperty::PROP_Output, true, _("V"), new DoubleValue(0), 1)); 
	return node;
}


//------------ ColorNode

Laxkit::anObject *newColorNode(Laxkit::anObject *ref)
{
	NodeBase *node = new NodeBase;
	makestr(node->Name, _("Color"));
	makestr(node->type, "Color");

	node->properties.push(new NodeProperty(NodeProperty::PROP_Output, true, _("Color"), new ColorValue("#ffffff"), 1)); 
	//----------
	//node->properties.push(new NodeProperty(NodeProperty::PROP_Intput, false, _("Red"), new DoubleValue(1), 1)); 
	//node->properties.push(new NodeProperty(NodeProperty::PROP_Intput, false, _("Green"), new DoubleValue(1), 1)); 
	//node->properties.push(new NodeProperty(NodeProperty::PROP_Intput, false, _("Blue"), new DoubleValue(1), 1)); 
	//node->properties.push(new NodeProperty(NodeProperty::PROP_Intput, false, _("Alpha"), new DoubleValue(1), 1)); 
	//node->properties.push(new NodeProperty(NodeProperty::PROP_Output, false, _("Out"), new ColorValue("#ffffffff"), 1)); 
	return node;
}


//------------ MathNode

class MathNode : public NodeBase
{
  public:
	static SingletonKeeper mathnodekeeper; //the def for the op enum
	static ObjectDef *GetMathNodeDef() { return dynamic_cast<ObjectDef*>(mathnodekeeper.GetObject()); }

	int operation; // + - * / % power
	double a,b,result;
	MathNode(int op=0, double aa=0, double bb=0);
	virtual ~MathNode();
	virtual int Update();
	virtual int GetStatus();
	virtual int SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att);
};


enum MathNodeOps {
	OP_None,
	OP_Add,
	OP_Subtract,
	OP_Multiply,
	OP_Divide,
	OP_Mod,
	OP_Power,
	OP_Greater_Than,
	OP_Less_Than,
	OP_Equals,
	OP_Not_Equal,
	OP_Minimum,
	OP_Maximum,
	OP_Average,

	OP_And,
	OP_Or,
	OP_Xor,
	OP_ShiftLeft,
	OP_ShiftRight,

	OP_MAX
};

/*! Create and return a fresh instance of the def for a MathNode op.
 */
ObjectDef *DefineMathNodeDef()
{ 
	ObjectDef *def = new ObjectDef("MathNodeDef", _("Math Node Def"), NULL,NULL,"enum", 0);

	def->pushEnumValue("Add",_("Add"),_("Add"), OP_Add);
	def->pushEnumValue("Subtract",_("Subtract"),_("Subtract"), OP_Subtract);
	def->pushEnumValue("Multiply",_("Multiply"),_("Multiply"), OP_Multiply);
	def->pushEnumValue("Divide",_("Divide"),_("Divide"), OP_Divide);
	def->pushEnumValue("Mod",_("Mod"),_("Mod"), OP_Mod);
	def->pushEnumValue("Power",_("Power"),_("Power"), OP_Power);
	def->pushEnumValue("GreaterThan",_("Greater than"),_("Greater than"), OP_Greater_Than);
	def->pushEnumValue("LessThan",_("Less than"),_("Less than"), OP_Less_Than);
	def->pushEnumValue("Equals",_("Equals"),_("Equals"), OP_Equals);
	def->pushEnumValue("NotEqual",_("Not Equal"),_("Not Equal"), OP_Not_Equal);
	def->pushEnumValue("Minimum",_("Minimum"),_("Minimum"), OP_Minimum);
	def->pushEnumValue("Maximum",_("Maximum"),_("Maximum"), OP_Maximum);
	def->pushEnumValue("Average",_("Average"),_("Average"), OP_Average);

	def->pushEnumValue("And"       ,_("And"       ),_("And"       ), OP_And      );
	def->pushEnumValue("Or"        ,_("Or"        ),_("Or"        ), OP_Or       );
	def->pushEnumValue("Xor"       ,_("Xor"       ),_("Xor"       ), OP_Xor      );
	def->pushEnumValue("ShiftLeft" ,_("ShiftLeft" ),_("ShiftLeft" ), OP_ShiftLeft);
	def->pushEnumValue("ShiftRight",_("ShiftRight"),_("ShiftRight"), OP_ShiftRight);

	return def;
}

SingletonKeeper MathNode::mathnodekeeper(DefineMathNodeDef(), true);


MathNode::MathNode(int op, double aa, double bb)
{
	type = newstr("Math");
	Name = newstr(_("Math"));

	a=aa;
	b=bb;
	operation = op;

	ObjectDef *enumdef = GetMathNodeDef();
	enumdef->inc_count();


	EnumValue *e = new EnumValue(enumdef, 0);
	enumdef->dec_count();

	properties.push(new NodeProperty(NodeProperty::PROP_Input, false, "Op", e, 1));
	properties.push(new NodeProperty(NodeProperty::PROP_Input,  true, "A", new DoubleValue(a), 1));
	properties.push(new NodeProperty(NodeProperty::PROP_Input,  true, "B", new DoubleValue(b), 1));
	properties.push(new NodeProperty(NodeProperty::PROP_Output, true, "Result", NULL, 0));

	Update();
}

MathNode::~MathNode()
{
//	if (mathnodedef) {
//		if (mathnodedef->dec_count()<=0) mathnodedef=NULL;
//	}
}

/*! Return 1 for property set, 0 for could not set.
 */
int MathNode::SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att)
{
	if (strcmp(propname, "Op")) return NodeBase::SetPropertyFromAtt(propname, att);
	if (isblank(att->value)) return 0;

	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	ObjectDef *def=ev->GetObjectDef();

	if (att->attributes.n==0) return 0;
	const char *nval = att->attributes.e[0]->value;
	if (isblank(nval)) return 0;

	const char *nm;
	for (int c=0; c<def->getNumEnumFields(); c++) {
		def->getEnumInfo(c, NULL, &nm); //grabs id == name in the def
		if (!strcmp(nm, nval)) {
			ev->value = c; //note: makes enum value the index of the enumval def, not the id
			break;
		}
	}
	return 1;
}

int MathNode::GetStatus()
{
	a = dynamic_cast<DoubleValue*>(properties.e[1]->data)->d;
	b = dynamic_cast<DoubleValue*>(properties.e[2]->data)->d;

	if ((operation==OP_Divide || operation==OP_Mod) && b==0) return -1;
	if (a==0 || (a<0 && fabs(b)-fabs(int(b))<1e-10)) return -1;
	if (!properties.e[3]->data) return 1;
	return 0;
}

int MathNode::Update()
{
	a = dynamic_cast<DoubleValue*>(properties.e[1]->GetData())->d;
	b = dynamic_cast<DoubleValue*>(properties.e[2]->GetData())->d;

	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	ObjectDef *def = ev->GetObjectDef();
	const char *nm = NULL;
	operation=OP_None;
	def->getEnumInfo(ev->value, &nm, NULL,NULL, &operation); 

	//DBG cerr <<"MathNode::Update op: "<<operation<<" vs enum id: "<<id<<endl;

	if      (operation==OP_Add) result = a+b;
	else if (operation==OP_Subtract) result = a-b;
	else if (operation==OP_Multiply) result = a*b;
	else if (operation==OP_Divide) {
		if (b!=0) result = a/b;
		else {
			result=0;
			return -1;
		}

	} else if (operation==OP_Mod) {
		if (b!=0) result = a-b*int(a/b);
		else {
			result=0;
			return -1;
		}
	} else if (operation==OP_Power) {
		if (a==0 || (a<0 && fabs(b)-fabs(int(b))<1e-10)) {
			 //0 to a power fails, as does negative numbers raised to non-integer powers
			result=0;
			return -1;
		}
		result = pow(a,b);

	} else if (operation==OP_Greater_Than) { result = (a>b);
	} else if (operation==OP_Less_Than)    { result = (a<b);
	} else if (operation==OP_Equals)       { result = (a==b);
	} else if (operation==OP_Not_Equal)    { result = (a!=b);
	} else if (operation==OP_Minimum)       { result = (a<b ? a : b);
	} else if (operation==OP_Maximum)       { result = (a>b ? a : b);
	} else if (operation==OP_Average)       { result = (a+b)/2;

	} else if (operation==OP_And       )       { result = (int(a) & int(b));
	} else if (operation==OP_Or        )       { result = (int(a) | int(b));
	} else if (operation==OP_Xor       )       { result = (int(a) ^ int(b));
	} else if (operation==OP_ShiftLeft )       { result = (int(a) << int(b));
	} else if (operation==OP_ShiftRight)       { result = (int(a) >> int(b));
	}

	if (!properties.e[3]->data) properties.e[3]->data=new DoubleValue(result);
	else dynamic_cast<DoubleValue*>(properties.e[3]->data)->d = result;
	properties.e[3]->modtime = time(NULL);

	return NodeBase::Update();
}

Laxkit::anObject *newMathNode(Laxkit::anObject *ref)
{
	return new MathNode();
}


//------------ FunctionNode

// sin cos tan asin acos atan sinh cosh tanh log ln abs sqrt int floor ceil factor random randomint
// fraction negative reciprocal clamp scale(old_range, new_range)
// pi tau e
//
//class FunctionNode : public NodeBase
//{
  //public:
//};


//------------ ImageNode

SingletonKeeper imageDepthKeeper;

ObjectDef *GetImageDepthDef()
{ 
	ObjectDef *edef = dynamic_cast<ObjectDef*>(imageDepthKeeper.GetObject());

	if (!edef) {
		//ObjectDef *def = new ObjectDef("ImageNode", _("Image Node"), NULL,NULL,"class", 0);

		edef = new ObjectDef("ColorDepth", _("Color depth"), NULL,NULL,"enum", 0);
		edef->pushEnumValue("d8",_("8"),_("8"));
		edef->pushEnumValue("d16",_("16"),_("16"));
		edef->pushEnumValue("d24",_("24"),_("24"));
		edef->pushEnumValue("d32",_("32"),_("32"));
		edef->pushEnumValue("d32f",_("32f"),_("32f"));
		edef->pushEnumValue("d64f",_("64f"),_("64f"));

		imageDepthKeeper.SetObject(edef, true);
	}

	return edef;
}

Laxkit::anObject *newImageNode(Laxkit::anObject *ref)
{
	NodeBase *node = new NodeBase;
	//node->Id("Image");
	
	makestr(node->type, "NewImage");
	makestr(node->Name, _("New Image"));
	//node->properties.push(new NodeProperty(true, true, _("Filename"), new FileValue("."), 1)); 
	node->properties.push(new NodeProperty(NodeProperty::PROP_Input, true, _("Width"), new DoubleValue(100), 1)); 
	node->properties.push(new NodeProperty(NodeProperty::PROP_Input, true, _("Height"), new DoubleValue(100), 1)); 
	node->properties.push(new NodeProperty(NodeProperty::PROP_Input, true, _("Channels"), new IntValue(4), 1)); 

	ObjectDef *enumdef = GetImageDepthDef();
	EnumValue *e = new EnumValue(enumdef, 0);
	node->properties.push(new NodeProperty(NodeProperty::PROP_Input, true, _("Depth"), e, 1)); 

	node->properties.push(new NodeProperty(NodeProperty::PROP_Input, true, _("Initial Color"), new ColorValue("#ffffff"), 1)); 
	node->properties.push(new NodeProperty(NodeProperty::PROP_Output, true, _("Color"), NULL, 1)); 
	//depth: 8, 16, 24, 32, 32f, 64f
	//format: gray, graya, rgb, rgba
	//backend: raw, default, gegl, gmic, gm, cairo
	return node;
}


//--------------------------- SetupDefaultNodeTypes()

/*! Install default built in node types to factory.
 */
int SetupDefaultNodeTypes(Laxkit::ObjectFactory *factory)
{
	 //--- ColorNode
	factory->DefineNewObject(getUniqueNumber(), "Color",  newColorNode,   NULL);

	 //--- ImageNode
	factory->DefineNewObject(getUniqueNumber(), "Image",  newImageNode,   NULL);

	 //--- MathNode
	factory->DefineNewObject(getUniqueNumber(), "Math",  newMathNode,   NULL);

	 //--- DoubleNode
	factory->DefineNewObject(getUniqueNumber(), "Value", newDoubleNode, NULL);

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

	node_factory = NodeGroup::NodeFactory(true);
	node_factory->inc_count();

	nodes=NULL;

	lasthover=-1;
	lasthoverslot=-1;
	lasthoverprop=-1;
	lastconnection=-1;
	hover_action=NODES_None;
	showdecs=1;
	needtodraw=1;
	font = app->defaultlaxfont;
	font->inc_count();
	slot_radius=.25;

	color_controls.rgbf(.7,.5,.7,1.);
	color_background.rgbf(0,0,0,.5);
	color_grid.rgbf(0,0,0,.7);
	draw_grid = 50;

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

	} else if (!strcmp(mes,"setpropdouble") || !strcmp(mes,"setpropint")) {
		if (!nodes || lasthover<0 || lasthover>=nodes->nodes.n || lasthoverprop<0
				|| lasthoverprop>=nodes->nodes.e[lasthover]->properties.n) return 0;

        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		if (isblank(s->str)) return  0;
        char *endptr=NULL;
        double d=strtod(s->str, &endptr);
		if (endptr==s->str) {
			PostMessage(_("Bad value."));
			return 0;
		}

		NodeBase *node = nodes->nodes.e[lasthover];
		NodeProperty *prop = node->properties.e[lasthoverprop];
		
		if (!strcmp(mes,"setpropdouble")) {
			DoubleValue *v=dynamic_cast<DoubleValue*>(prop->data);
			v->d = d;
		} else {
			IntValue *v=dynamic_cast<IntValue*>(prop->data);
			v->i = d;
		}
		node->Update();
		needtodraw=1;

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

		const char *nm;
		for (int c=0; c<def->getNumEnumFields(); c++) {
			def->getEnumInfo(c, NULL, &nm); //grabs id == name in the def
			if (!strcmp(nm, what)) {
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

		if (!nodes) {
			nodes = new Nodes;
			nodes->InstallColors(new NodeColors, true);
			nodes->colors->Font(font, false);
		}

		ObjectFactoryNode *type;
		for (int c=0; c<node_factory->types.n; c++) {
			type = node_factory->types.e[c];

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


	dp->PushAxes();

	ScreenColor *bg = &color_background;
	if (nodes) bg = &nodes->background;
	if (bg->Alpha()>0) {
		dp->NewTransform(1,0,0,1,0,0);
		dp->NewFG(bg);
		dp->drawrectangle(0,0, dp->Maxx,dp->Maxy, 1);

		if (draw_grid) {
		}
	}

	if (!nodes) {
		dp->PopAxes();
		return 0;
	} 

	dp->NewTransform(nodes->m.m());
	dp->font(font);

	 //---draw connections
	dp->NewFG(&nodes->colors->connection);

	dp->LineWidth(3);
	for (int c=0; c<nodes->connections.n; c++) {
		//dp->DrawColoredPath(nodes->nodes.e[c]->connections.e[c]->path.e,
		//					nodes->nodes.e[c]->connections.e[c]->path.n);
		//dp->drawlines(nodes->connections.e[c]->path.e,
		//			  nodes->connections.e[c]->path.n,
		//			  false, 0);
		DrawConnection(nodes->connections.e[c]);
	}

	 //---draw nodes:
	 //  box+border
	 //  label
	 //  expanded arrow
	 //  preview
	 //  ins/outs
	NodeBase *node;
	ScreenColor *border, *fg;
	ScreenColor tfg, tbg;
	NodeColors *colors=NULL;
	double th = dp->textheight();
	double borderwidth = 1;

	for (int c=0; c<nodes->nodes.n; c++) {
		node = nodes->nodes.e[c];

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
		if (lasthover == c) { //mouse is hovering over this node
			tfg = *fg;
			tbg = *bg;
			tfg.AddDiff(colors->mo_diff, colors->mo_diff, colors->mo_diff);
			tbg.AddDiff(colors->mo_diff, colors->mo_diff, colors->mo_diff);
			fg = &tfg;
			bg = &tbg;
		}
		flatpoint p;

		 //draw whole rect, bg
		dp->NewFG(bg);
		dp->LineWidth(borderwidth);
		dp->drawRoundedRect(node->x, node->y, node->width, node->height,
							th/3, false, th/3, false, 1); 

		 //draw label
		dp->NewFG(&colors->label_bg);
		dp->drawRoundedRect(node->x, node->y, node->width, th,
							th/3, false, th/3, false, 1, 8|4); 

		 //draw whole rect border
		dp->NewFG(border);
		dp->drawRoundedRect(node->x, node->y, node->width, node->height,
							th/3, false, th/3, false, 0); 

		dp->NewFG(&colors->label_fg);
		dp->textout(node->x+node->width/2-th/2, node->y, node->Name, -1, LAX_TOP|LAX_HCENTER);
		dp->NewFG(fg);
		dp->NewBG(bg);


		 //draw the properties (or not)
		p.set(node->x+node->width, node->y);
		if (node->collapsed) dp->drawthing(p.x-th/2, p.y+th/2, th/4,th/4, 1, THING_Triangle_Right);
		else {
			 //node is expanded, draw all the properties...
			// *** not implemented yet: *** dp->drawthing(p.x-th, p.y+th/2, th/4,th/4, 1, THING_Triangle_Down);

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
				DrawProperty(node, prop, y);

				y += prop->height;
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

		flatpoint p1 = nodes->m.transformPointInverse(flatpoint(selection_rect.minx,selection_rect.miny));
		flatpoint p2 = nodes->m.transformPointInverse(flatpoint(selection_rect.maxx,selection_rect.maxy));

		//dp->drawrectangle(selection_rect.minx,selection_rect.miny, selection_rect.maxx-selection_rect.minx,selection_rect.maxy-selection_rect.miny, 0);
		dp->drawrectangle(p1.x,p1.y, p2.x-p1.x,p2.y-p1.y, 0);

	} else if (hover_action==NODES_Drag_Input || hover_action==NODES_Drag_Output) {
		NodeBase *node = nodes->nodes.e[lasthover];
		NodeConnection *connection = node->properties.e[lasthoverslot]->connections.e[lastconnection];

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

void NodeInterface::DrawProperty(NodeBase *node, NodeProperty *prop, double y)
{
	// todo, defaults for:
	//   numbers
	//   vectors
	//   colors
	//   enums
	Value *v = prop->GetData();

	char extra[200];
	extra[0]='\0';
	double th = dp->textheight();
	dp->LineWidth(1);
	ScreenColor col;

	if (v && (v->type()==VALUE_Real || v->type()==VALUE_Int)) {
		dp->NewFG(coloravg(&col, &nodes->colors->bg_edit, &nodes->colors->fg_edit));
		dp->NewBG(&nodes->colors->bg_edit);
		dp->drawRoundedRect(node->x+prop->x+th/2, node->y+prop->y+th/4, node->width-th, prop->height*.66,
							th/3, false, th/3, false, 2); 

		dp->NewFG(&nodes->colors->fg_edit);
		sprintf(extra, "%s:", prop->Name());
		dp->textout(node->x+prop->x+th, node->y+prop->y+prop->height/2, extra, -1, LAX_LEFT|LAX_VCENTER);
		v->getValueStr(extra, 199);
		dp->textout(node->x+node->width-th, node->y+prop->y+prop->height/2, extra, -1, LAX_RIGHT|LAX_VCENTER);

	} else if (v && v->type()==VALUE_Enum) {

		 //draw name
		double x=th/2;
		double dx=dp->textout(node->x+x, node->y+prop->y+prop->height/2, prop->Name(),-1, LAX_LEFT|LAX_VCENTER);
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
		const char *nm; 
		ev->GetObjectDef()->getEnumInfo(ev->value, NULL, &nm);
		dp->textout(node->x+th*1.5+dx, node->y+prop->y+prop->height/2, nm,-1, LAX_LEFT|LAX_VCENTER);
		//dp->textout(node->x+th*1.5+dx, node->y+prop->y+prop->height/2, extra,-1, LAX_LEFT|LAX_VCENTER);
		dp->drawthing(node->x+node->width-th, node->y+prop->y+prop->height/2, th/4,th/4, 1, THING_Triangle_Down);

	} else if (v && v->type()==VALUE_Color) {
		ColorValue *color = dynamic_cast<ColorValue*>(v);
		double x = node->x+th/2;
		unsigned long oldfg = dp->FG();
		if (!(prop->IsInput() && prop->IsConnected())) {
			 //draw color box
			dp->NewFG(color->color.Red(),color->color.Green(),color->color.Blue(),color->color.Alpha());
			dp->drawrectangle(x,y+prop->height/2-th/2, 2*th, th, 1);
			dp->NewFG(coloravg(&col, &nodes->colors->bg_edit, &nodes->colors->fg_edit));
			dp->drawrectangle(x,y+prop->height/2-th/2, 2*th, th, 0);
			x += 2*th + th/2;
		}
		dp->NewFG(oldfg);
		dp->textout(x,y+prop->height/2, prop->name,-1, LAX_LEFT|LAX_VCENTER);

	} else {
		//strcpy(extra, prop->name);

		if (prop->IsInput()) {
			 //draw on left side
			double dx = dp->textout(node->x+th/2, y+prop->height/2, prop->Name(),-1, LAX_LEFT|LAX_VCENTER); 
			if (!isblank(extra)) {
				dp->textout(node->x+th+dx, y+prop->height/2, extra,-1, LAX_LEFT|LAX_VCENTER);
				dp->drawrectangle(node->x+th/2+dx, y, node->width-(th+dx), th*1.25, 0);
			}

		} else {
			 //draw on right side
			double dx = dp->textout(node->x+node->width-th/2, y+prop->height/2, prop->Name(),-1, LAX_RIGHT|LAX_VCENTER);
			if (!isblank(extra)) {
				dp->textout(node->x+node->width-th-dx, y+prop->height/2, extra,-1, LAX_RIGHT|LAX_VCENTER);
				dp->drawrectangle(node->x+th/2, y-th*.25, node->width-(th*1.25+dx), th*1.25, 0);
			}
		}
	}

	 //draw connection spot
	if (prop->is_linkable) {
		dp->NewBG(&prop->color);
		dp->drawellipse(prop->pos+flatpoint(node->x,node->y), th*slot_radius,th*slot_radius, 0,0, 2);
	} 
}

void NodeInterface::DrawConnection(NodeConnection *connection) 
{
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

/*! Return the node under x,y, or -1 if no node there.
 */
int NodeInterface::scan(int x, int y, int *overpropslot, int *overproperty) 
{
	if (!nodes) return -1;

	flatpoint p=nodes->m.transformPointInverse(flatpoint(x,y));
	if (overpropslot) *overpropslot=-1;

	NodeBase *node;
	double th = font->textheight();

	for (int c=nodes->nodes.n-1; c>=0; c--) {
		node = nodes->nodes.e[c];

		if (p.x >= node->x-th/2 &&  p.x <= node->x+node->width+th/2 &&  p.y >= node->y &&  p.y <= node->y+node->height) {
			 //found a node, now see if over a property's in/out

			NodeProperty *prop;
			for (int c2=0; c2<node->properties.n; c2++) {
				prop = node->properties.e[c2];

				if (!(prop->IsInput() && !prop->is_linkable)) { //only if the input is not exclusively internal
				  if (p.y >= node->y+prop->y && p.y < node->y+prop->y+prop->height) {
					if (p.x >= node->x+prop->pos.x-th/2 && p.x <= node->x+prop->pos.x+th/2) {
						if (overpropslot) *overpropslot = c2;
					}
				  }
				}

				if (p.y >= node->y+prop->y && p.y < node->y+prop->y+prop->height) {
					if (overproperty) *overproperty=c2;
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
	int overpropslot=-1, overproperty=-1; 
	int overnode = scan(x,y, &overpropslot, &overproperty);

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
		 //control-drag make a cut line for connections
		action = NODES_Cut_Connections;
		selection_rect.minx=selection_rect.maxx=x;
		selection_rect.miny=selection_rect.maxy=y;
		needtodraw=1;

	} else if (overnode>=0 && overproperty==-1 && overpropslot==-1) {
		 //in a node, but not clicking on a property, so add or remove this node to selection
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
			//selected.flush();
			//selected.push(nodes->nodes.e[overnode]);
			//action = NODES_Move_Nodes;
			action = NODES_Move_Or_Select;
			lasthover = overnode;
			needtodraw=1;
		}

	} else if (overnode>=0 && overproperty>=0 && overpropslot==-1) {
		 //click down on a property, but not on the slot...
		action = NODES_Property;

	} else if (overnode>=0 && overpropslot>=0) {
		 //drag out a property to connect to another node
		NodeProperty *prop = nodes->nodes.e[overnode]->properties.e[overpropslot];

		if (prop->IsInput()) {
			action = NODES_Drag_Input;

			if (prop->connections.n) {
				// if dragging input that is already connected, then disconnect from current node,
				// and drag output at the other end
				action = NODES_Drag_Output;
				overnode = nodes->nodes.findindex(prop->connections.e[0]->from);
				overpropslot = nodes->nodes.e[overnode]->properties.findindex(prop->connections.e[0]->fromprop);
				prop->connections.e[0]->to = NULL; // note: assumes only one input
				prop->connections.e[0]->toprop = NULL;
				prop->connections.remove(0);

				lastconnection = 0;
				lasthover      = overnode;
				lasthoverslot  = overpropslot;

			} else {
				 //connection didn't exist, so install a half connection to current node
				NodeConnection *newcon = new NodeConnection(NULL, nodes->nodes.e[overnode], NULL,prop);
				prop->connections.push(newcon, 0);//prop list does NOT delete connection
				nodes->connections.push(newcon, 1);//node connection list DOES delete connection
				lastconnection = prop->connections.n-1;
				lasthover      = overnode;
				lasthoverslot  = overpropslot;
			}

		} else if (prop->IsOutput()) {
			 // if dragging output, create new connection
			action = NODES_Drag_Output;
			prop->connections.push(new NodeConnection(nodes->nodes.e[overnode],NULL, prop,NULL), 0);//prop list does not delete connection
			nodes->connections.push(prop->connections.e[prop->connections.n-1], 1);//node connection list DOES delete connection
			lastconnection = prop->connections.n-1;
			lasthoverslot  = overpropslot;
			lasthover      = overnode;
		}

		if (action==NODES_Drag_Output) PostMessage(_("Drag output..."));
		else PostMessage(_("Drag input..."));
	}

	if (action != NODES_None) {
		buttondown.down(d->id, LEFTBUTTON, x,y, action);
		hover_action = action;
	} else hover_action = NODES_None;


	return 0; //return 0 for absorbing event, or 1 for ignoring
}

//! Finish a new freehand line by calling newData with it.
int NodeInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	int action=NODES_None;
	int property=-1;
	int dragged = buttondown.up(d->id, LEFTBUTTON, &action, &property);

	int overpropslot=-1, overproperty=-1; 
	int overnode = scan(x,y, &overpropslot, &overproperty);



	if (action == NODES_Property) {
		 //mouse up on a property, so do property specific actions...

		if (!nodes || overnode<0 || dragged>5) return 0;
		NodeBase *node = nodes->nodes.e[overnode];
		NodeProperty *prop = node->properties.e[overproperty];

		if (prop->IsInput() && prop->IsConnected()) return 0; //can't change if piped in from elsewhere

		Value *v=dynamic_cast<Value*>(prop->data);
		if (!v) return 0;

		char valuestr[200];
		if (v->type()==VALUE_Real || v->type()==VALUE_Int) {
			 //create input box..

			flatpoint ul= nodes->m.transformPoint(flatpoint(node->x+prop->x, node->y+prop->y));
			flatpoint lr= nodes->m.transformPoint(flatpoint(node->x+prop->x+prop->width, node->y+prop->y+prop->height));

			DoubleBBox bounds;
			bounds.addtobounds(ul);
			bounds.addtobounds(lr);

			v->getValueStr(valuestr, 199);

			viewport->SetupInputBox(object_id, NULL, valuestr, v->type()==VALUE_Real ? "setpropdouble" : "setpropint", bounds);
			lasthover = overnode;
			lasthoverprop = overproperty;

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
			const char *nm;
			for (int c=0; c<def->getNumEnumFields(); c++) {
				def->getEnumInfo(c, NULL, &nm);
				menu->AddItem(nm, c);
			}

			PopupMenu *popup=new PopupMenu(NULL,_("Add node..."), 0,
							0,0,0,0, 1,
							object_id,"selectenum",
							0, //mouse to position near?
							menu,1, NULL,
							MENUSEL_LEFT|MENUSEL_CHECK_ON_LEFT|MENUSEL_DESTROY_ON_LEAVE);
			popup->pad=5;
			popup->WrapToMouse(0);
			app->rundialog(popup);
		}

		return 0; 

	} else if (action == NODES_Move_Or_Select) {
		//to have this, we clicked down, but didn't move, so select the node..
		if (overnode>=0) {
			selected.flush();
			selected.push(nodes->nodes.e[overnode]);
			needtodraw=1;
		}
		return 0;

	} else if (action == NODES_Cut_Connections) {
		 //cut any connections that cross the line between selection_rect.min to max
		// ***
		PostMessage("Need to implement Cut Connections!!!");
		needtodraw=1;

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

	} else if (action == NODES_Drag_Input) {
		//check if hovering over the output of some other node
		// *** need to ensure there is no circular linking
		DBG cerr << " *** need to ensure there is no circular linking for nodes!!!"<<endl;

		int remove=0;

		if (overnode>=0 && overpropslot>=0) {
			 //need to connect
			NodeProperty *toprop = nodes->nodes.e[overnode]->properties.e[overpropslot];
			if (!toprop->IsInput()) {

				 //connect lasthover.lasthoverslot.lastconnection to toprop
				NodeConnection *connection = nodes->nodes.e[lasthover]->properties.e[lasthoverslot]->connections.e[lastconnection];
				toprop->connections.push(connection, 0);
				connection->from     = nodes->nodes.e[overnode];
				connection->fromprop = nodes->nodes.e[overnode]->properties.e[overpropslot];
				connection->to->Update();

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
			nodes->connections.remove(nodes->connections.findindex(nodes->nodes.e[lasthover]->properties.e[lasthoverslot]->connections.e[lastconnection]));
			nodes->nodes.e[lasthover]->properties.e[lasthoverslot]->connections.remove(lastconnection);
			lastconnection=-1;
		} 


	} else if (action == NODES_Drag_Output) {
		//check if hovering over the input of some other node
		DBG cerr << " *** need to ensure there is no circular linking for nodes!!!"<<endl;

		int remove=0;

		if (overnode>=0 && overpropslot>=0) {
			NodeProperty *toprop = nodes->nodes.e[overnode]->properties.e[overpropslot];

			if (toprop->IsInput()) {
				//nodes->Connect(
				//---------
				if (toprop->connections.n) {
					 //clobber any other connection going into the input. Only one input allowed.
					for (int c=toprop->connections.n-1; c>=0; c--) {
						//toprop->connections.remove();
						nodes->connections.remove(toprop->connections.e[c]);
					}
					toprop->connections.flush();
				}

				 //connect lasthover.lasthoverslot.lastconnection to toprop
				NodeConnection *connection = nodes->nodes.e[lasthover]->properties.e[lasthoverslot]->connections.e[lastconnection];
				toprop->connections.push(connection, 0);
				connection->to     = nodes->nodes.e[overnode];
				connection->toprop = nodes->nodes.e[overnode]->properties.e[overpropslot];
				connection->to->Update();

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
			nodes->connections.remove(nodes->connections.findindex(nodes->nodes.e[lasthover]->properties.e[lasthoverslot]->connections.e[lastconnection]));
			nodes->nodes.e[lasthover]->properties.e[lasthoverslot]->connections.remove(lastconnection);
			lastconnection=-1;
		} 

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

		int newhoverslot=-1, newhoverprop=-1;
		int newhover = scan(x,y, &newhoverslot, &newhoverprop);
		lastpos.x=x; lastpos.y=y;
		DBG cerr << "nodes lastpos: "<<lastpos.x<<','<<lastpos.y<<endl;

		if (newhover!=lasthover || newhoverslot!=lasthoverslot || newhoverprop!=lasthoverprop) {
			needtodraw=1;
			lasthoverslot = newhoverslot;
			lasthoverprop = newhoverprop;
			lasthover = newhover;

			if (lasthover<0) PostMessage("");
			else {
				char scratch[200];
				sprintf(scratch, "%s.%d.%d", nodes->nodes.e[lasthover]->Name, lasthoverprop, lasthoverslot);
				PostMessage(scratch);
			}
		}

		return 1;
	}

	int lx,ly, action, property;
	buttondown.move(mouse->id, x,y, &lx,&ly);
	

	if (buttondown.isdown(mouse->id, MIDDLEBUTTON)) {
		DBG cerr <<"node middle button move: "<<x-lx<<", "<<y-ly<<endl;
		nodes->m.origin(nodes->m.origin() + flatpoint(x-lx, y-ly));
		needtodraw=1;
		return 0;
	}

	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &action, &property);


	 //special check to maybe change action
	if (action == NODES_Move_Or_Select) {
		action = NODES_Move_Nodes;
		buttondown.moveinfo(mouse->id, LEFTBUTTON, action, property);

		int overnode = lasthover;

		if (selected.findindex(nodes->nodes.e[overnode]) < 0) {
			 //hovered node not already selected

			if ((state & ShiftMask) == 0) selected.flush();
			selected.push(nodes->nodes.e[overnode]);
		} //else node was already selected
	}

	if (action == NODES_Property) {
		 //we are dragging on a property.. might have to get response from a custom interface
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

int NodeInterface::MBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	buttondown.down(d->id, MIDDLEBUTTON, x,y);
	if (!nodes) return 1;
	return 0;
}

int NodeInterface::MBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	buttondown.up(d->id, MIDDLEBUTTON);
	if (!nodes) return 1;
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
    sc->Add(NODES_Delete_Nodes,  LAX_Bksp,0,     0, "DeleteNode"   , _("Delete Node"  ),NULL,0);
	sc->AddShortcut(LAX_Del,0,0, NODES_Delete_Nodes);


    sc->Add(NODES_Save_Nodes,      's',0,  0, "SaveNodes"      , _("Save Nodes"     ),NULL,0);
    sc->Add(NODES_Load_Nodes,      'l',0,  0, "LoadNodes"      , _("Load Nodes"     ),NULL,0);

    manager->AddArea(whattype(),sc);
    return sc;

}

int NodeInterface::PerformAction(int action)
{
	if (action==NODES_Group_Nodes) {
		//***

	} else if (action==NODES_Ungroup_Nodes) {
		//***

	} else if (action==NODES_Delete_Nodes) {
		DBG cerr << "delete nodes..."<<endl;
		if (!nodes || selected.n == 0) return 0;

		nodes->DeleteNodes(selected);
		PostMessage(_("Deleted."));
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

	} else if (action==NODES_Save_Nodes) {
		DBG cerr <<"save nodes..."<<endl;
		if (!nodes) return 0;

		const char *file = "nodes-TEMP.nodes";

		FILE *f = fopen(file, "w");
		if (f) {
			nodes->dump_out(f, 0, 0, NULL);
			fclose(f);

			PostMessage(_("Nodes saved to nodes-TEMP.nodes"));
			DBG cerr << _("Nodes saved to nodes-TEMP.nodes") <<endl;
		} else {
			PostMessage(_("Could not open nodes-TEMP.nodes!"));
			DBG cerr <<(_("Could not open nodes-TEMP.nodes!")) << endl;
		}
		return 0;

	} else if (action==NODES_Load_Nodes) {
		DBG cerr <<"load nodes..."<<endl;

		if (nodes) {
			nodes->dec_count();
			nodes = NULL;
		}

		nodes = new Nodes;
		nodes->InstallColors(new NodeColors, true);
		nodes->colors->Font(font, false);

		const char *file = "nodes-TEMP.nodes";

		FILE *f = fopen(file, "r");
		if (f) {
			nodes->dump_in(f, 0, 0, NULL, NULL);
			fclose(f);

			PostMessage(_("Nodes loaded from nodes-TEMP.nodes"));
			DBG cerr << _("Nodes loaded from nodes-TEMP.nodes") <<endl;
		} else {
			PostMessage(_("Could not open nodes-TEMP.nodes!"));
			DBG cerr <<(_("Could not open nodes-TEMP.nodes!")) << endl;
		}

		needtodraw=1;
		return 0;
	}

	return 1;
}

} // namespace Laidout

