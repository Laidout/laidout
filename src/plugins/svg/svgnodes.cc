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
// Copyright (C) 2018 by Tom Lechner
//


#include "../../language.h"
#include "svgnodes.h"
#include "../../interfaces/nodeinterface.h"
#include <exception>

//template implementation:
#include <lax/lists.cc>


#include <iostream>
#define DBG


using namespace std;


//---- svg blend to gegl:
//feBlend
//   mode:
//     "normal",   svg:dst-over,  same as "over" for feComposite
//     "multiply"  gegl:svg-multiply
//     "screen"    svg:screen
//     "darken"    svg:darken
//     "lighten"   svg:lighten
//feColorMatrix
//  type:
//	   "matrix"             gegl:matrix, values is double[5,4]
//	   "saturate"			gegl:svg-saturate   is 1 real, [0..1]
//	   "hueRotate"          gegl:svg-huerotate, is 1 real degrees
//	   "luminanceToAlpha"   gegl:svg-luminancetoalpha, no values needed
//feComponentTransfer
//  feFunc[RGBA]
//feComposite
//feConvolveMatrix
//feDiffuseLighting
//feDisplacementMap
//feFlood
//feGaussianBlur
//feImage
//feMerge
//  feMergeNode*
//feMorphology
//feOffset
//feSpecularLighting
//feTile
//feTurbulence


namespace Laidout {
namespace SvgFilterNS {


//---------------------- SvgFilterNode ----------------------------

const char *svgprimitives[] = {
		 //primitives
		"feBlend",
		"feColorMatrix",
		"feComponentTransfer",
		"feComposite",
		"feConvolveMatrix",
		"feDiffuseLighting",
		"feDisplacementMap",
		"feFlood",
		"feGaussianBlur",
		"feImage",
		"feMerge",
		"feMorphology",
		"feOffset",
		"feSpecularLighting",
		"feTile",
		"feTurbulence",

		NULL
	};

const char *svgmisclist[] = {
		 //input source node
		"SvgSource",
		 //misc child elements
		"feMergeNode",
		"feFuncR",
		"feFuncG",
		"feFuncB",
		"feFuncA",
		"feDistantLight",
		"fePointLight",
		"feSpotLight",
		NULL
	};


ObjectDef *GetSvgDefs();

class SvgFilterNode : public Laidout::NodeBase
{
  protected:
    //int AutoProcess();

  public:
	static Laxkit::SingletonKeeper svg_def_keeper;

    //char *filter;
    //GeglNode *gegl;

    //SvgFilterNode(const char *filterName);
	SvgFilterNode(const char *filterName);
    //SvgFilterNode(const char *filterName, SvgFilterNode *in, SvgFilterNode *in2);
    virtual ~SvgFilterNode();
	virtual NodeBase *Duplicate();

    //virtual int SetFilter(const char *filter);
    //virtual int UpdateProperties();
    //virtual int Update();
    //virtual int UpdatePreview();
    virtual int Disconnected(NodeConnection *connection, bool from_will_be_replaced, bool to_will_be_replaced);
    virtual int Connected(NodeConnection *connection);
    //virtual int SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att);

	virtual const char *ResultName();
	virtual int dump_in_atts(Attribute *att, NodeGroup *filter, SvgFilterNode *last, SvgFilterNode *srcnode, Laxkit::ErrorLog &log);
	//virtual int dump_out_to_svg(Attribute *defs);
	virtual NodeProperty *FindRef(const char *name, NodeGroup *filter);

};


Laxkit::SingletonKeeper SvgFilterNode::svg_def_keeper;


///*! \class SvgFilterParentNode
// * Hold a collection of SvgFilterNode and SvgFilterNodes
// */
//class SvgFilterNode : public Laidout::NodeGroup
//{
//  public:
//	//<filter id="f027"
//	//   inkscape:label="Barbed Wire"
//	//   inkscape:menu="Overlays"
//	//   inkscape:menu-tooltip="Gray bevelled wires with drop shadows"
//	//   style="color-interpolation-filters:sRGB;"
//	// >
//};


SvgFilterNode::SvgFilterNode(const char *filterName)
{
	makestr(Name, filterName);

	makestr  (type, "Svg Filter/");
	appendstr(type, filterName);

	ObjectDef *svgdefs = GetSvgDefs();
	ObjectDef *fdef = svgdefs->FindDef(filterName);

	if (fdef) {
		InstallDef(fdef, 0);
		int issrc = (strcmp(fdef->name, "SvgSource") == 0);

		//add linkable image in for SvgSource node
		if (issrc) AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "imageIn", NULL,1, _("In"),_("Input image"), 0, false));


		ObjectDef *result = NULL;
		//int resulti = -1;
		//Value *resultv = NULL;

		int isprimitive = false;
		if (fdef->extendsdefs.n && !strcmp(fdef->extendsdefs.e[0]->name, "FilterPrimitive")) {
			isprimitive = true;
			//add a rect
			AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "bounds", new BBoxValue,1, _("Bounds"), _("Rectangle the filter acts in"), -1));
		}

		for (int c=0; c<fdef->getNumFieldsOfThis(); c++) {
			ObjectDef *d = fdef->getFieldOfThis(c);
			if (!d) continue;

			int isenum = false;
			Value *v = NULL;
			if (d->format == VALUE_Number) v = new DoubleValue;
			else if (d->format == VALUE_Int) v = new IntValue;
			else if (d->format == VALUE_String) v = new StringValue;
			else if (d->format == VALUE_Enum) {
				v = new EnumValue(d,0);
				isenum = true;
			}

			//if (!strcmp(d->name, "result")) { result = d; resulti = c; resultv = v; }
			//else
				AddProperty(new NodeProperty(issrc ? NodeProperty::PROP_Output : isenum ? NodeProperty::PROP_Block : NodeProperty::PROP_Input,
											 isenum ? false : true, d->name, v,1, d->Name, d->description, c));
		}

		 //add child button for certain nodes
		if (fdef->uihint && strstr(fdef->uihint, "kids") == fdef->uihint) {
			 //there are possible children, so add slot to insert new children
			const char *kids = fdef->uihint + 5;
			char *ttip = newstr(_("Possible kids: "));
			while (*kids != ')' && *kids != 0) {
				const char *k = kids;
				while (isalnum(*k)) k++;
				if (k == kids) break;
				appendnstr(ttip, kids, k-kids);
				appendstr(ttip, " ");
				kids = k;
				while (*kids==',' || *kids==' ') kids++;
			}
			AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "NewChild", NULL,1, _("(add child)"), ttip, 0, false));
			delete[] ttip;
		}

		if (isprimitive) {
			 //add result, part of FilterPrimitive, which we skipped before
			result = fdef->FindDef("result");
			//if (result) AddProperty(new NodeProperty(NodeProperty::PROP_Block, false, result->name, resultv,1, result->Name, result->description, resulti));
			if (result) AddProperty(new NodeProperty(NodeProperty::PROP_Block, false, result->name, new StringValue(),1, result->Name, result->description, -2));
		}

		//add linkable image out
		if (!issrc) AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", NULL,1, _("Out"),_("The resulting image"), 0, false));

	} else {
		DBG if (filterName) cerr << " *** warning! Could not find Svg node def for "<<filterName<<endl;
	}
}

SvgFilterNode::~SvgFilterNode()
{
}

NodeBase *SvgFilterNode::Duplicate()
{
	SvgFilterNode *newnode = new SvgFilterNode(strrchr(type, '/'));

	 //copy the properties' data
	for (int c=0; c<properties.n; c++) {
		NodeProperty *property = properties.e[c];
		if (!(property->type == NodeProperty::PROP_Input || property->type == NodeProperty::PROP_Block)) continue;

		Value *v = property->GetData();
		if (v) {
			v = v->duplicate();
			NodeProperty *newprop = newnode->FindProperty(property->name);
			newprop->SetData(v, 1);
		}
	}

	newnode->DuplicateBase(this);
	return newnode;
}



/*! Convenience to get the result name, not the actual image output.
 */
const char *SvgFilterNode::ResultName()
{
	NodeProperty *p = FindProperty("result");
	return p ? p->name : NULL;
}

/*! Dump in an XML based Attribute presumably from an svg file.
 */
int SvgFilterNode::dump_in_atts(Attribute *att, NodeGroup *filter, SvgFilterNode *last, SvgFilterNode *srcnode, Laxkit::ErrorLog &log)
{
	//ObjectDef *svgdefs = dynamic_cast<ObjectDef*>(SvgFilterNode::svg_def_keeper.GetObject());
	//ObjectDef *sourcedef = svgdefs->FindDef("SvgSource");

	const char *name, *value;
	NodeProperty *srcprop = NULL, *src2prop = NULL;

	const char *xx=NULL, *yy=NULL, *ww=NULL, *hh=NULL;

	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

		if (!strcmp(name, "in")) {
			srcprop = srcnode->FindProperty(value);

			if (!srcprop) {
				 //is reference
				srcprop = FindRef(value, filter); //find "out" prop in node with "result" property equal to value
			}

		} else if (!strcmp(name, "in2")) {
			src2prop = srcnode->FindProperty(value);

			if (!src2prop) {
				 //is reference
				src2prop = FindRef(value, filter); //find "out" prop in node with "result" property equal to value
			}

		} else if (!strcmp(name, "content:")) {
			//has sub elements, like feFunc*, feMergeNode, etc
			// ***

			NodeProperty *newchild = FindProperty("NewChild");
			if (newchild) {
				ObjectDef *svgdefs = dynamic_cast<ObjectDef*>(SvgFilterNode::svg_def_keeper.GetObject());

				Attribute *contents = att->attributes.e[c];
				for (int c3=0; c3 < contents->attributes.n; c3++) {
					name  = contents->attributes.e[c3]->name;
					value = contents->attributes.e[c3]->value;

					ObjectDef *def = svgdefs->FindDef(name);
					if (def) {
						SvgFilterNode *pp = new SvgFilterNode(name);
						//pp->def should == def.. assume it is so for now

						pp->dump_in_atts(contents->attributes.e[c3], filter, last, srcnode, log);
						filter->AddNode(pp);
						pp->dec_count();

						newchild = FindProperty("NewChild");
						NodeProperty *childout = pp->FindProperty("out");

						filter->Connect(childout, newchild);


					} else {
						DBG cerr << " warning! could not find ObjectDef for "<<name<<endl;
					}

				}
			} else {
				DBG cerr <<" Warning! filter "<<(Label()?Label():"?")<<" has contents but doesn't seem to want it!"<<endl;
			}

		} else {
			//should just be a plain old value
			ObjectDef *fdef = def->FindDef(name);
			NodeProperty *prop = FindProperty(name);

			Value *v = NULL;

			if (prop && fdef) {
				 //intercept these optional values, and construct a RectangleNode later
				if (!strcmp(name, "x")) {
					xx = value;
				} else if (!strcmp(name, "y")) {
					yy = value;
				} else if (!strcmp(name, "width")) {
					ww = value;
				} else if (!strcmp(name, "height")) {
					hh = value;

				} else {

					if (fdef->format == VALUE_Number || fdef->format == VALUE_Real) {
						DoubleValue *vv = new DoubleValue(); //clunkiness due to compiler nag workaround
						vv->Set(value);
						v = vv;

					} else if (fdef->format == VALUE_Int) {
						v = new IntValue(value, 10);

					} else if (fdef->format == VALUE_Boolean) {
						v = new BooleanValue(value);

					} else if (fdef->format == VALUE_Enum) {
						int i = fdef->findfield(value, NULL);
						if (i>=0) {
							v = new EnumValue(fdef, i);
						}

					} else {
						//shove everything else to string...
						v = new StringValue(value);
					}
				}
			}

			if (v && prop) prop->SetData(v, 1);
		}
	}

	if (xx || yy || ww || hh) {
		 //install a rectangle with the filter bounds
		NodeProperty *bounds = FindProperty("bounds");
		if (bounds) {
			double xxx = xx ? strtof(value,NULL) : 0;
			double yyy = yy ? strtof(value,NULL) : 0;
			double www = ww ? strtof(value,NULL) : 0;
			double hhh = hh ? strtof(value,NULL) : 0;
			//bounds->SetData(new BBoxValue(xxx,xxx+www, yyy,yyy+hhh),1);

			NodeBase *rect = filter->NewNode("Rectangle");
			rect->FindProperty("x")     ->SetData(new DoubleValue(xxx), 1);
			rect->FindProperty("y")     ->SetData(new DoubleValue(yyy), 1);
			rect->FindProperty("width") ->SetData(new DoubleValue(www), 1);
			rect->FindProperty("height")->SetData(new DoubleValue(hhh), 1);

			filter->AddNode(rect);
			rect->dec_count();
			filter->Connect(rect->FindProperty("Rect"), FindProperty("bounds"));

			//DBG cerr << " ****** need to install a new node for bounds"<<endl;
		}
	}

	 //connect in, if necessary
	if (!srcprop && last) srcprop = last->FindProperty("out");
	if (!srcprop && !last) srcprop = srcnode->FindProperty("SourceGraphic");
	if (srcprop) {
		NodeProperty *inprop = FindProperty("in");
		if (inprop) filter->Connect(srcprop, inprop);
	}

	 //connect in2, if necessary
	if (src2prop) {
		NodeProperty *in2prop = FindProperty("in2");
		if (in2prop) filter->Connect(src2prop, in2prop);
	}

	return log.Errors();
}

bool IsSvgFilterPrimitive(SvgFilterNode *node)
{
	const char *fname = strrchr(node->Type(), '/');
	if (!fname) return false;
	return node && findInList(fname+1, svgprimitives) >= 0;
}

//-----------------------dump out filter

/*! If boundsnode, also write out a "laidout:bounsdXYWH" hint for that bounds node placement.
 */
void DumpSvgBounds(DoubleBBox *bounds, Attribute *att, NodeBase *boundsnode)
{
	att->push("x", bounds->minx);
	att->push("y", bounds->miny);
	att->push("width", bounds->maxx - bounds->minx);
	att->push("height", bounds->maxy - bounds->miny);

	if (boundsnode) {
		char scratch[200];
		sprintf(scratch, "%f,%f,%f,%f", boundsnode->x,boundsnode->y,boundsnode->width,boundsnode->height);
		att->push("laidout:boundsXYWH", scratch);
	}
}

/*! From an "in" prop, grab either the "result" string from a connected node,
 * or one of the SvgSource enums.
 */
const char *GetInString(NodeProperty *prop, int *issrc_ret)
{
	if (!prop || !prop->IsInput() || !prop->connections.n) return NULL;
	*issrc_ret = 0;

	NodeBase *from = prop->connections.e[0]->from;
	SvgFilterNode *snode = dynamic_cast<SvgFilterNode*>(from);

	if (!snode) return NULL;
	if (!strcmp(snode->Type(), "SvgSource")) {
		*issrc_ret = 1;
		return prop->connections.e[0]->fromprop->name;
	}

	NodeProperty *result = snode->FindProperty("result");
	if (!result) return NULL;
	StringValue *s = dynamic_cast<StringValue*>(result->GetData());
	if (s) return s->str;
	return NULL;
}

/*! For each filter primitive SvgFilterNode...
 */
int DumpSvgNodesBackwards(Attribute *att, SvgFilterNode *node, NodeGroup *filter, Laxkit::ErrorLog &log)
{
	const char *nodetype = strrchr(node->Type(), '/') + 1;
	Attribute *nodeatt = new Attribute(nodetype, NULL);

	try {
		Attribute *contents = NULL;

		PtrStack<SvgFilterNode> uppernodes;

		for (int c=0; c<node->properties.n; c++) {
			NodeProperty *prop = node->properties.e[c];
			if (!prop->IsInput() && !prop->IsBlock()) continue;

			if (!strcmp(prop->Name(), "NewChild")) {
					continue;

			} else if (!strcmp(prop->Name(), "bounds") && prop->IsConnected()) {
				 //write out filter bounds
				BBoxValue *bounds = dynamic_cast<BBoxValue*>(prop->GetData());
				if (!bounds || !bounds->validbounds()) throw _("Bad bounds property");
				DumpSvgBounds(bounds, nodeatt, NULL);

			} else if (!strcmp(prop->name, "in") || !strcmp(prop->name, "in2")) {
				//grab either "result" string, or SvgSource name
				// if connected....
				int issrc = 0;
				const char *in = GetInString(prop, &issrc);
				nodeatt->push(prop->name, in);

				uppernodes.pushnodup(node, 0);

			} else if (!strncmp(prop->name, "Child", 5)) {
				if (!contents) contents = nodeatt->pushSubAtt("content:");

				//only ones that have kids are feMerge, feComponentTransfer, feDiffuseLighting, feSpecularLighting

				 //svg filter nodes can only have basically one node deep. Exception is feMergeNode, but for that we
				 //just want the "result" attribute

				//NodeBase *cnode = NULL;
				SvgFilterNode *childnode = NULL;
				if (prop->connections.n) childnode = dynamic_cast<SvgFilterNode*>(prop->connections.e[0]->from);
				if (!childnode) throw _("Expected an svg child node!");

				Attribute *ch = contents->pushSubAtt(strrchr(childnode->Type(),'/') + 1);

				for (int c2=0; c2<childnode->properties.n; c2++) {
					NodeProperty *prop2 = childnode->properties.e[c2];
					if (!prop2->IsInput() && !prop2->IsBlock()) {
						// *** make special exception for SvgSource outputs
						continue;
					}

					Value *d = prop2->GetData();
					if (!d) throw 2;

					//special catch for "in"
					const char *cstr = NULL;
					char *str = NULL;
					if (!strcmp(prop2->name, "in")) {
						int issrc = 0;
						cstr = GetInString(prop2, &issrc);
						if (cstr) uppernodes.pushnodup(childnode, 0);

					} else {
						// ***
						int n = d->getValueStr(NULL,0);
						str = new char[n+1];
						d->getValueStr(str, n+1);
					}

					ch->push(childnode->properties.e[c2]->name, cstr ? cstr : str);
					delete[] str;
				}


			} else {
				 //simple dump out property
				int n=0;
				Value *d = prop->GetData();
				n = d->getValueStr(NULL,0);
				char *str = new char[n+1];
				d->getValueStr(str, n+1);
				nodeatt->push(prop->name, str);
				delete[] str;
			}

		}

	} catch (int e) {
		// ***clean up
		delete nodeatt;
		return 1;

	} catch (const char *error_mes) {
		// ***clean up
		log.AddMessage(error_mes, ERROR_Fail);
		delete nodeatt;
		return 1;
	}

	att->push(nodeatt, 0);


	return 0;
}

/*! Return 0 for success, or nonzero for error.
 */
int DumpOutSvgFilter(Attribute *defs, NodeGroup *filter, Laxkit::ErrorLog &log)
{

	//first check filter properties:
	//  needs to have one input "in" and one output "out"
	NodeProperty *prop = filter->FindProperty("in");
	if (!prop || (prop && !prop->IsInput())) {
		log.AddMessage(_("Filter needs an in!"), ERROR_Fail);
		return 1;
	}

	 //find SvgSource node, which needs to be connected to the filter in, which is prop->topropproxy
	prop = prop->topropproxy;
	if (!prop->connections.n) {
		log.AddMessage(_("Unconnected filter"), ERROR_Fail);
		return 1;
	}

	SvgFilterNode *sourcein = dynamic_cast<SvgFilterNode*>(prop->connections.e[0]->to);
	if (!sourcein || strcmp(sourcein->Type(), "Svg Filter/SvgSource")) {
		log.AddMessage(_("Filter in needs to connect to an SvgSource node"), ERROR_Fail);
		return 1;
	}

	prop = filter->FindProperty("out");
	if (!prop || (prop && !prop->IsOutput())) {
		log.AddMessage(_("Filter needs an out!"), ERROR_Fail);
		return 1;
	}

	prop = prop->frompropproxy;
	if (!prop->connections.n) {
		log.AddMessage(_("Unconnected filter"), ERROR_Fail);
		return 1;
	}
	SvgFilterNode *finalout = dynamic_cast<SvgFilterNode*>(prop->connections.e[0]->from);
	if (!IsSvgFilterPrimitive(finalout)) {
		log.AddMessage(_("Final out needs to be an Svg Filter Node"), ERROR_Fail);
		return 1;
	}


	 //first go through all nodes and make sure that there are no repeat "result" strings.
	 //this makes it easier to code later on
	NumStack<const char *> results;
	for (int c=0; c<filter->nodes.n; c++) {
		SvgFilterNode *node = dynamic_cast<SvgFilterNode*>(filter->nodes.e[c]);
		if (!node) continue;
		NodeProperty *prop = node->FindProperty("result");
		if (!prop) continue;
		StringValue *v = dynamic_cast<StringValue*>(prop->GetData());
		if (!v) continue;

		int c2 = findInList(v->str, results.e, results.n);
		if (c2 == -1) {
			 //was unique
			results.push(v->str);
		} else {
			 //already taken, so make a new unique result name
			char *str = newstr(v->str);
			while(1) {
				char *str2 = increment_file(str);
				c2 = findInList(str2, results.e);
				if (c2==-1) {
					 //ok to use
					v->Set(str2);
					results.push(v->str);
					delete[] str2;
					break;
				}
				delete[] str;
				str = str2;
			}
			delete[] str;
		}
	}


	// the finalout connects to the final node. Work backwards from there

	Attribute *filteratt = new Attribute();

	int status = DumpSvgNodesBackwards(filteratt, finalout, filter, log);
	if (status == 1) delete filteratt;
	else defs->push(filteratt, -1);

	return status;
}
//----------------------- end dump out filter


/*! Find the "out" property of the node that has name == "result".
 */
NodeProperty *SvgFilterNode::FindRef(const char *name, NodeGroup *filter)
{
	//dup names for result is just fine, just refers to most recent
	for (int c=filter->nodes.n-1; c>=0; c--) {
		NodeProperty *prop = filter->nodes.e[c]->FindProperty("result");
		if (!prop) continue;
		StringValue *s = dynamic_cast<StringValue*>(prop->GetData());
		if (s && s->str && !strcmp(name, s->str)) return filter->nodes.e[c]->FindProperty("out");
	}
	return NULL;
}

/*! If connecting a child, then add a new slot for another child.
 */
int SvgFilterNode::Connected(NodeConnection *connection)
{
	NodeProperty *prop = (connection->from == this ? connection->fromprop : connection->toprop);
	if (!strcmp(prop->name, "NewChild")) {
		//something was put into the slot for "new child".
		//rename that to "Child#" and add a new "new childe" property

		int where = properties.findindex(prop) + 1;
		//int numkids = where-1;
		//while (numkids > 0 && !strncmp(properties.e[numkids-1]->name, "Child", 5)) numkids--;
		//numkids = where - numkids;

		char str[50];
		sprintf(str, _("Child%ld"), getUniqueNumber());
		prop->Name(str);
		prop->Label(_("Child"));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "NewChild", NULL,1, _("(add child)"), prop->tooltip, 0, false), where);

		UpdateLayout();
		return 1;
	}

	return 0;
}

/*! If disconnecting a child, then remove that property.
 */
int SvgFilterNode::Disconnected(NodeConnection *connection, bool from_will_be_replaced, bool to_will_be_replaced)
{
	if (connection->to == this && !to_will_be_replaced && !strncmp(connection->toprop->name, "Child", 5)) {
		 //remove empty child link
		RemoveProperty(connection->toprop);
		UpdateLayout();
		connection->toprop = NULL;
		connection->to = NULL;
	}

	return 0;
}


//---------------------- SvgFilter defs ----------------------------

ObjectDef *GetSvgDefs()
{
	ObjectDef *svgdefs = dynamic_cast<ObjectDef*>(SvgFilterNode::svg_def_keeper.GetObject());
	if (svgdefs) return svgdefs;

	svgdefs = new ObjectDef("SvgFilter", "SVG Filter", "SVG Filter", NULL, "class", 0);
	SvgFilterNode::svg_def_keeper.SetObject(svgdefs, 1);


	//in = "SourceGraphic | SourceAlpha | BackgroundImage | BackgroundAlpha | FillPaint | StrokePaint | <filter-primitive-reference>"
//	ObjectDef *in = new ObjectDef("SvgFilterIn", _("In"), _("SVG Filter Container"), NULL, "class", 0);
//	svgdefs->push(in,1);
//
//	in->pushEnum("in", "In", NULL,  "SourceGraphic", NULL, NULL,
//					"SourceGraphic",   _("Source"),           NULL,
//					"SourceAlpha",     _("Source Alpha"),     NULL,
//					"BackgroundImage", _("Background"),       NULL,
//					"BackgroundAlpha", _("Background Alpha"), NULL,
//					"FillPaint",       _("Fill Paint"),       NULL,
//					"StrokePaint",     _("Stroke Paint"),     NULL,
//					"Reference",       _("Reference"),        NULL,
//					NULL);
//	in->push("inRef", _("Ref"), _("Used when in is Reference"), "string", NULL, NULL, 0, NULL);


	ObjectDef *sourcenode = new ObjectDef("SvgSource", _("Svg Source"), _("SVG Source"), NULL, "class", 0);
	svgdefs->push(sourcenode,1);

	sourcenode->push("SourceGraphic",   _("Source"),           NULL, NULL, NULL, NULL, 0, NULL);
	sourcenode->push("SourceAlpha",     _("Source Alpha"),     NULL, NULL, NULL, NULL, 0, NULL);
	sourcenode->push("BackgroundImage", _("Background"),       NULL, NULL, NULL, NULL, 0, NULL);
	sourcenode->push("BackgroundAlpha", _("Background Alpha"), NULL, NULL, NULL, NULL, 0, NULL);
	sourcenode->push("FillPaint",       _("Fill Paint"),       NULL, NULL, NULL, NULL, 0, NULL);
	sourcenode->push("StrokePaint",     _("Stroke Paint"),     NULL, NULL, NULL, NULL, 0, NULL);
	//sourcenode->push("Reference",       _("Reference"),        NULL, "string", NULL, NULL, 0, NULL);


	 //read in rest from parsed svg spec
	const char *svgdefstr =
		"#Laidout namespace\n"
		"name SvgFilters\n"
		"format class\n"
		"class FilterPrimitive\n"
		"  number x\n"
		"  number y\n"
		"  number width\n"
		"  number height\n"
		"  string result\n"
		"class feFuncR\n"
		"  enum type\n"
		"    enumval \"identity\"\n"
		"    enumval \"table\"\n"
		"    enumval \"discrete\"\n"
		"    enumval \"linear\"\n"
		"    enumval \"gamma\"\n"
		"  string tableValues\n"
		"  number slope\n"
		"  number intercept\n"
		"  number amplitude\n"
		"  number exponent\n"
		"  number offset\n"
		"class feFuncG\n"
		"  enum type\n"
		"    enumval \"identity\"\n"
		"    enumval \"table\"\n"
		"    enumval \"discrete\"\n"
		"    enumval \"linear\"\n"
		"    enumval \"gamma\"\n"
		"  string tableValues\n"
		"  number slope\n"
		"  number intercept\n"
		"  number amplitude\n"
		"  number exponent\n"
		"  number offset\n"
		"class feFuncB\n"
		"  enum type\n"
		"    enumval \"identity\"\n"
		"    enumval \"table\"\n"
		"    enumval \"discrete\"\n"
		"    enumval \"linear\"\n"
		"    enumval \"gamma\"\n"
		"  string tableValues\n"
		"  number slope\n"
		"  number intercept\n"
		"  number amplitude\n"
		"  number exponent\n"
		"  number offset\n"
		"class feFuncA\n"
		"  enum type\n"
		"    enumval \"identity\"\n"
		"    enumval \"table\"\n"
		"    enumval \"discrete\"\n"
		"    enumval \"linear\"\n"
		"    enumval \"gamma\"\n"
		"  string tableValues\n"
		"  number slope\n"
		"  number intercept\n"
		"  number amplitude\n"
		"  number exponent\n"
		"  number offset\n"
		"class feDistantLight\n"
		"  number azimuth\n"
		"  number elevation\n"
		"class fePointLight\n"
		"  number x\n"
		"  number y\n"
		"  number z\n"
		"class feSpotLight\n"
		"  number x\n"
		"  number y\n"
		"  number z\n"
		"  number pointsAtX\n"
		"  number pointsAtY\n"
		"  number pointsAtZ\n"
		"  number specularExponent\n"
		"  number limitingConeAngle\n"
		"class feMergeNode\n"
		"  string in\n"
		"class feBlend\n"
		"  extends FilterPrimitive\n"
		"  string in\n"
		"  string in2\n"
		"  enum mode\n"
		"    defaultValue normal\n"
		"    enumval \"normal\"\n"
		"    enumval \"multiply\"\n"
		"    enumval \"screen\"\n"
		"    enumval \"darken\"\n"
		"    enumval \"lighten\"\n"
		"class feColorMatrix\n"
		"  extends FilterPrimitive\n"
		"  string in\n"
		"  enum type\n"
		"    defaultValue matrix\n"
		"    enumval \"matrix\"\n"
		"    enumval \"saturate\"\n"
		"    enumval \"hueRotate\"\n"
		"    enumval \"luminanceToAlpha\"\n"
		"  string values\n"
		"class feComponentTransfer\n"
		"  extends FilterPrimitive\n"
		"  uihint kids(feFuncR, feFuncG, feFuncB, feFuncA)\n"
		"  string in\n"
		"class feComposite\n"
		"  extends FilterPrimitive\n"
		"  string in\n"
		"  string in2\n"
		"  enum operator\n"
		"    defaultValue over\n"
		"    enumval \"over\"\n"
		"    enumval \"in\"\n"
		"    enumval \"out\"\n"
		"    enumval \"atop\"\n"
		"    enumval \"xor\"\n"
		"    enumval \"arithmetic\"\n"
		"  number k1\n"
		"  number k2\n"
		"  number k3\n"
		"  number k4\n"
		"class feConvolveMatrix\n"
		"  extends FilterPrimitive\n"
		"  string in\n"
		"  number order\n"
		"  string kernelMatrix\n"
		"  number divisor\n"
		"  number bias\n"
		"  int targetX\n"
		"  int targetY\n"
		"  enum edgeMode\n"
		"    defaultValue duplicate\n"
		"    enumval \"duplicate\"\n"
		"    enumval \"wrap\"\n"
		"    enumval \"none\"\n"
		"  number kernelUnitLength\n"
		"  boolean preserveAlpha\n"
		"class feDiffuseLighting\n"
		"  extends FilterPrimitive\n"
		"  uihint kids(feDistantLight, fePointLight, feSpotLight)\n"
		"  string in\n"
		"  number surfaceScale\n"
		"  number diffuseConstant\n"
		"  number kernelUnitLength\n"
		"class feDisplacementMap\n"
		"  extends FilterPrimitive\n"
		"  string in\n"
		"  string in2\n"
		"  number scale\n"
		"  enum xChannelSelector\n"
		"    defaultValue A\n"
		"    enumval \"R\"\n"
		"    enumval \"G\"\n"
		"    enumval \"B\"\n"
		"    enumval \"A\"\n"
		"  enum yChannelSelector\n"
		"    defaultValue A\n"
		"    enumval \"R\"\n"
		"    enumval \"G\"\n"
		"    enumval \"B\"\n"
		"    enumval \"A\"\n"
		"class feFlood\n"
		"  string flood-color\n"
		"  number flood-opacity\n"
		"  extends FilterPrimitive\n"
		"class feGaussianBlur\n"
		"  extends FilterPrimitive\n"
		"  string in\n"
		"  number stdDeviation\n"
		"class feImage\n"
		"  extends FilterPrimitive\n"
		"  string preserveAspectRatio\n"
		"    defaultValue xMidYMid meet\n"
		"class feMerge\n"
		"  extends FilterPrimitive\n"
		"  uihint kids(feMergeNode)\n"
		"class feMorphology\n"
		"  extends FilterPrimitive\n"
		"  string in\n"
		"  enum operator\n"
		"    defaultValue erode\n"
		"    enumval \"erode\"\n"
		"    enumval \"dilate\"\n"
		"  number radius\n"
		"class feOffset\n"
		"  extends FilterPrimitive\n"
		"  string in\n"
		"  number dx\n"
		"  number dy\n"
		"class feSpecularLighting\n"
		"  extends FilterPrimitive\n"
		"  uihint kids(feDistantLight, fePointLight, feSpotLight)\n"
		"  string in\n"
		"  number surfaceScale\n"
		"  number specularConstant\n"
		"  number specularExponent\n"
		"  number kernelUnitLength\n"
		"class feTile\n"
		"  extends FilterPrimitive\n"
		"  string in\n"
		"class feTurbulence\n"
		"  extends FilterPrimitive\n"
		"  number baseFrequency\n"
		"  int numOctaves\n"
		"  number seed\n"
		"  enum stitchTiles\n"
		"    defaultValue noStitch\n"
		"    enumval \"stitch\"\n"
		"    enumval \"noStitch\"\n"
		"  enum type\n"
		"    defaultValue turbulence\n"
		"    enumval \"fractalNoise\"\n"
		"    enumval \"turbulence\"\n"
	  ;

	svgdefs->dump_in_str(svgdefstr, 0, NULL, NULL);


	return svgdefs;
}


//-------------------- SVG filter importing --------------------------


// *********** DON'T USE, NOT FUNCTIONAL
ValueHash *XMLAttToHash(ObjectDef *types, LaxFiles::Attribute *att, ValueHash *append_to_this, Laxkit::ErrorLog *log)
{
	ObjectDef *svgdefs = GetSvgDefs();

	ValueHash *hash = append_to_this;
	if (!hash) hash = new ValueHash();

	const char *name, *value;

	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

		if (!strcmp(name, "content:")) {
			if (att->attributes.e[c]->attributes.n || value) {
				SetValue *content = new SetValue();
				hash->push(".content", content, 1);

				//if (value) content->Push("cdata", value);

				for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {

					//if (!XMLAttToHash(types, att->attributes.e[c]->attributes.e[c2], content, log)) break;
				}
			}

		} else {
			ObjectDef *def = svgdefs->FindDef(name);
			if (def) {
				//try to convert value/attribute to a Value
				Value *val = NULL; // ***


				//val = AttributeToValue(value);

				if (val) hash->push(name, value);
				else hash->push(name, value);

			} else {
				hash->push(name, value);
			}

		}
	}

	return hash;
}


/*! Load in filters extracted from an svg file. One group node per filter
 * If which_filter == NULL, then extract all. Else just that one.
 */
int LoadSVGFilters(const char *file, const char *which_filter, NodeGroup *put_here, Laxkit::ErrorLog &log, Attribute **att_ret)
{
	DBG cerr <<"loading svg filters..."<<endl;

	ObjectDef *svgdefs = GetSvgDefs();

	Attribute *att = XMLFileToAttribute(NULL, file, NULL);
	if (!att) {
		if (att_ret) *att_ret = NULL;
		return 1;
	}

	try {
		Attribute *att2 = att->find("svg");
		if (!att2) throw 1;

		//if (att2) {
		//  att2 = att2->find("content:);
		//  if (att2) {
		//    att2 = att2->find("defs");
		//    ...
		//  }
		//

		att2 = att2->find("content:");
		if (!att2) throw 2;

		att2 = att2->find("defs");
		if (!att2) throw 3;

		Attribute *defs = att2->find("content:");
		if (!defs) throw 4;


		const char *name, *value;
		for (int c=0; c<defs->attributes.n; c++) {
			name  = defs->attributes.e[c]->name;
			value = defs->attributes.e[c]->value;

			if (!strcmp(name, "filter")) {
				if (which_filter) {
					const char *aa = defs->attributes.e[c]->findValue("id");
					if (aa && strcmp(aa, which_filter)) {
						aa = defs->attributes.e[c]->findValue("inkscape:label");
						if (aa && strcmp(aa, which_filter)) {
							continue;
						}
					}
				}

				NodeGroup *filter = new NodeGroup();
				filter->InitializeBlank();
				put_here->AddNode(filter);
				filter->dec_count();
				SvgFilterNode *last = NULL;

				SvgFilterNode *srcnode = new SvgFilterNode("SvgSource");
				filter->AddNode(srcnode);
				srcnode->dec_count();

				 //add the primitives
				Attribute *filteratt = defs->attributes.e[c];

				for (int c2=0; c2<filteratt->attributes.n; c2++) {
					name  = filteratt->attributes.e[c2]->name;
					value = filteratt->attributes.e[c2]->value;

					if (!strcmp(name, "id")) {
						filter->Id(value);

					} else if (!strcmp(name, "inkscape:label")) {
						filter->Label(value);

					//} else if (!strcmp(name, "inkscape:menu")) {
						//***
					//} else if (!strcmp(name, "inkscape:menu-tooltip")) {
						//***
					//} else if (!strcmp(name, "style")) {
						//***

					} else if (!strcmp(name, "content:")) {

						//these should only be filter primitives

						Attribute *primitives = filteratt->attributes.e[c2];
						for (int c3=0; c3<primitives->attributes.n; c3++) {
							name  = primitives->attributes.e[c3]->name;
							value = primitives->attributes.e[c3]->value;

							ObjectDef *def = svgdefs->FindDef(name);
							if (def) {
								SvgFilterNode *pp = new SvgFilterNode(name);
								//pp->def should == def.. assume it is so for now

								pp->dump_in_atts(primitives->attributes.e[c3], filter, last, srcnode, log);
								filter->AddNode(pp);
								pp->dec_count();
								last = pp;


							} else {
								DBG cerr << " warning! could not find ObjectDef for "<<name<<endl;
							}

						}
					}
				}

				 //connect in to srcnode->in
				NodeProperty *nodeprop = filter->AddGroupInput ("in",  _("In"),  NULL);
				filter->Connect(nodeprop->topropproxy, srcnode->FindProperty("imageIn"));

				 //connect final result to out
				nodeprop = filter->AddGroupOutput("out", _("Out"), NULL);

				NodeProperty *maybeout = NULL;

				for (int c2=0; c2<filter->nodes.n; c2++) {
					if (!filter->nodes.e[c2]->type || !strcmp(filter->nodes.e[c2]->type, "SvgSource")) continue;
					NodeProperty *mprop = filter->nodes.e[c2]->FindProperty("out");
					if (mprop && !mprop->IsConnected()) maybeout = mprop;
				}

				if (maybeout) {
					filter->Connect(maybeout, nodeprop->frompropproxy);
				}

				 //remove from att, in case we export again later
				if (att_ret) {
					defs->attributes.remove(c);
					c--;
				}


			} //if filter
		}


	} catch (exception &e) {
		DBG cerr <<" *** error parsing svg file: "<<e.what()<<endl;
		log.AddMessage(_("Could not load svg filters!"), ERROR_Fail);
		delete att;
		if (att_ret) *att_ret = NULL;
		return 1;

	} catch (int e) {
		DBG cerr <<" *** error parsing svg file: "<<e<<endl;
		log.AddMessage(_("Could not load svg filters!"), ERROR_Fail);
		delete att;
		if (att_ret) *att_ret = NULL;
		return 1;
	}

	if (att_ret) *att_ret = att;
	return 0;
}

//-------------------------------- SvgFilterLoader --------------------------


int SvgFilterLoader::CanImport(const char *file, const char *first500)
{
	if (!file && !first500) return true;
	if (first500) {
		if (strstr(first500, "<svg")) return true;
		return false;
	}
	return true;
}

/*! if null, then return if in theory it can export
 */
int SvgFilterLoader::CanExport(anObject *object)
{
	return true;
}

/*! context needs to be a NodeGroup.
 */
int SvgFilterLoader::Import(const char *file, anObject **object_ret, anObject *context, Laxkit::ErrorLog &log)
{
	NodeGroup *parent = dynamic_cast<NodeGroup*>(context); //maybe null
	NodeGroup *group = NULL;

	if (parent) group = parent;
	else {
		group = new NodeGroup;
		group->Id("svgimport");
		group->Label(_("Svg Import"));

		//log.AddMessage(_("Parent needs to be a node group"), ERROR_Fail);
		//return 1;
	}

    LoadSVGFilters(file, NULL, group, log, NULL);

	if (object_ret) *object_ret = group;

    return log.Errors();
}

int SvgFilterLoader::Export(const char *file, anObject *object, anObject *context, Laxkit::ErrorLog &log)
{
	NodeGroup *group = dynamic_cast<NodeGroup*>(object);
	if (!group) {
		log.AddMessage(_("Object not a NodeGroup in Export"), ERROR_Fail);
		return 1;
	}

	NodeExportContext *ncontent = dynamic_cast<NodeExportContext*>(context);
	if (!ncontent) {
		log.AddError(_("Bad context!"));
		return 1;
	}


	Attribute defs;
	NodeGroup *g;
	int status = 0;
	for (int c=0; c<group->nodes.n; c++) {
		g = dynamic_cast<NodeGroup*>(group->nodes.e[c]);
		if (!g) continue;
		status = DumpOutSvgFilter(&defs, g, log);
		if (status != 0) break;
	}

	if (status != 0) return 1;
	if (!defs.attributes.n) {
		log.AddError(_("Didn't find any filters to export!"));
		return 1;
	}

	FILE *f = fopen(file, "w");
	if (!f) {
		log.AddError(_("Could not open file"));
		return 1;
	}

	defs.dump_out(f, 0);

	fclose(f);

    DBG cerr << " done with SvgFilterLoader::Export()!"<<endl;
    return 0;
}


//-------------------- Node installation --------------------------

Laxkit::anObject *newSvgFilterNode(int p, Laxkit::anObject *ref)
{
	ObjectDef *svgdefs = GetSvgDefs();
	ObjectDef *def = svgdefs->getField(p);
	if (!def) return NULL;

    SvgFilterNode *node = new SvgFilterNode(def->name);
    return node;
}

void RegisterSvgNodes(Laxkit::ObjectFactory *factory)
{
	ObjectDef *svgdefs = GetSvgDefs();

    char str[200];
    ObjectDef *def;

    for (int c=0; c < svgdefs->getNumFields(); c++) {
        def = svgdefs->getField(c);
		if (findInList(def->name, svgprimitives) < 0 && findInList(def->name, svgmisclist) < 0) continue;

		sprintf(str, "Svg Filter/%s", def->name);
		factory->DefineNewObject(getUniqueNumber(), str, newSvgFilterNode, NULL, c);
    }
}




} //namespace SvgFilterNS
} //namespace Laidout




