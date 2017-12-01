//
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2017 by Tom Lechner
//


//#include <glib-2.0/glib.h>
#include <gegl-0.3/gegl.h>

#include "../language.h"
#include "../interfaces/nodeinterface.h"
#include "geglnodes.h"


#include <iostream>
#define DBG


using namespace std;


/*! dl entry point for laidout
 */
Laidout::PluginBase *GetPlugin()
{
	return new Laidout::GeglNodesPluginNS::GeglNodesPlugin();
}




namespace Laidout {
namespace GeglNodesPluginNS {


//-------------------------------- Helper funcs --------------------------

Laxkit::MenuInfo *GetGeglOps();


void XMLOut(GeglNode *gegl_node, const char *nodename)
{ 
    gchar *str = gegl_node_to_xml(gegl_node, ".");
    cout <<"\nXML for "<<nodename<<":\n"<<str<<endl;
    g_free(str);
}


int GValueToValue(GValue *gv, Value **v_Ret)
{
	// ***
	return 0;
}

/*! gvtype is the desired type as reported by gegl.
 * gv_ret should be at default blank GValue, with no specific type.
 *
 * Return 0 for success, or nonzero error.
 */
int ValueToGValue(Value *v, const char *gvtype, GValue *gv_ret, GeglNode *node, const char *property)
{
	if (!v) return 1;

	int vtype = v->type();

	if (!strcmp(gvtype, "gboolean")) {
		int isnum=0;
		int i = getNumberValue(v, &isnum);
		if (i) i=1;
		if (isnum) {
			g_value_init (gv_ret, G_TYPE_BOOLEAN);
			g_value_set_boolean(gv_ret, i);
			return 0;
		}
	} else if (!strcmp(gvtype, "gdouble")) {
		int isnum=0;
		double i = getNumberValue(v, &isnum);
		if (isnum) {
			g_value_init (gv_ret, G_TYPE_DOUBLE);
			g_value_set_double(gv_ret, i);
			return 0;
		}
	} else if (!strcmp(gvtype, "gint")) {
		int isnum=0;
		int i = getNumberValue(v, &isnum);
		if (isnum) {
			g_value_init (gv_ret, G_TYPE_INT);
			g_value_set_int(gv_ret, i);
			return 0;
		}
	} else if (!strcmp(gvtype, "gchararray")) {
		if (vtype == VALUE_Int || vtype == VALUE_Real || vtype == VALUE_Boolean) {
			 //convert numbers to strings
			char str[20]; str[0]='\0';
			v->getValueStr(str,20);
			g_value_init (gv_ret, G_TYPE_STRING);
			g_value_set_string(gv_ret, str);
			return 0;

		} else {
			if (vtype == VALUE_String) {
				StringValue *s = dynamic_cast<StringValue*>(v);
				if (s->str) {
					g_value_init (gv_ret, G_TYPE_STRING);
					g_value_set_string(gv_ret, s->str);
					return 0;
				} else return 2;
			}
			return 3;
		}

//	} else if (!strcmp(gvtype, "GeglColor")) {
//		if (vtype == VALUE_Color) {
//			ColorValue *col = dynamic_cast<ColorValue*>(v);
//			GeglColor  *color = gegl_color_new ("rgba(1.0,1.0,1.0,1.0)");
//			gegl_color_set_rgba(color, col->color.Red(), col->color.Green(), col->color.Blue(), col->color.Alpha());
//
//			GParamSpec *spec = gegl_node_find_property(node, property);
//			if (!spec) {
//				DBG cerr <<" *** missing spec for gegl property: "<<property<<endl;
//				return 5;
//			}
//
//			g_value_init (gv_ret, spec->value_type);
//			g_value_take_object (gv_ret, color);
//
//			g_object_unref (color);
//			return 0;
//		}
		return 4;

	//} else if (!strcmp(gvtype, "")) {
	//	***
	}

	return 100;
}

/*! Return 0 for able to set, else nonzero error.
 */
int ValueToProperty(Value *v, const char *gvtype, GeglNode *node, const char *property)
{
	int vtype = v->type();

	if (!strcmp(gvtype, "GeglColor")) {
		if (vtype == VALUE_Color) {
			ColorValue *col = dynamic_cast<ColorValue*>(v);
			GeglColor  *color = gegl_color_new (NULL);
			gegl_color_set_rgba(color, col->color.Red(), col->color.Green(), col->color.Blue(), col->color.Alpha());
			gegl_node_set(node, property, color, NULL);
			g_object_unref (color);
			return 0;
		}
	}

	return 100;
}

int XMLToGeglNodes(const char *str, NodeGroup *parent, Laxkit::ErrorLog *log)
{
	// ***
	return 1;
}

/*! Read in a file, and pass to XMLToGeglNodes().
 */
int XMLFileToGeglNodes(const char *file, NodeGroup *parent, Laxkit::ErrorLog *log)
{
//	char *contents = read_in_whole_file(file, NULL, 0);
//	if (!contents) return 1;
//
//	int status = XMLToGeglNodes(contents, parent, log);
//	delete[] contents;
//	return status;
//	----
//	Attribute att;
//	Attribute *ret = XMLFileToAttribute(&att, file, NULL);
//	if (!ret) return 1;
//
//	// *** parse the att
//
	return 1;
}


//-------------------------------- GeglLaidoutNode --------------------------


GeglNode *GeglLaidoutNode::masternode = NULL;

Laxkit::SingletonKeeper GeglLaidoutNode::op_menu;


GeglLaidoutNode::GeglLaidoutNode(const char *oper)
{
	gegl      = NULL;
	operation = NULL;
	op        = NULL; //don't delete this! it lives within op_menu
	SetOperation(oper);

	//gegl = gegl_node_new();
}

GeglLaidoutNode::~GeglLaidoutNode()
{
	delete[] operation;
	if (gegl) g_object_unref (gegl);
}

int GeglLaidoutNode::UpdateProperties()
{
	//*** gegl_node_process(gegl);
	return 1;
}

/*! Return 1 for saving nodes, display nodes.
 * 0 otherwise.
 */
int GeglLaidoutNode::OkToProcess()
{
	const char *savenodes[] = {
		"gegl:exr-save",
		"gegl:ff-save",
		"gegl:gegl-buffer-save",
		"gegl:jpg-save",
		"gegl:npy-save",
		"gegl:png-save",
		"gegl:ppm-save",
		"gegl:rgbe-save",
		"gegl:save",
		"gegl:save-pixbuf",
		"gegl:tiff-save",
		"gegl:webp-save",
		NULL
	};

	for (int c=0; savenodes[c]!=NULL; c++) {
		if (!strcmp(operation, savenodes[c])) return 1;
	}

	return 0;
}

/*! Called whenever input properties are changed, and outputs need to be updated.
 */
int GeglLaidoutNode::Update()
{
	DBG cerr << "GeglLaidoutNode::Update()..."<<endl;

	int num_updated = 0;
	int errors = 0;

	NodeProperty *prop;
	NodeConnection *connection;

	for (int c=0; c<properties.n; c++) {
		if (!properties.e[c]->IsInput()) continue;
		prop = properties.e[c];
		connection = prop->connections.n ? prop->connections.e[0] : NULL;

		 //turn Value back into GValue
		 //set it in the node

		const char *ptype = NULL;
		MenuItem *propspec = op->e(c);
		if (!propspec) {
			if (connection) {
				 //is an input pad, we need to make sure it's connected
				GeglNode *prevnode = NULL;
				int pindex=-1;
				GeglLaidoutNode *prev = dynamic_cast<GeglLaidoutNode*>(prop->GetConnection(0, &pindex));
				if (prev) {
					prevnode = prev->gegl;
					gegl_node_connect_to(prevnode, connection->fromprop->Name(),
										 gegl, prop->Name());
				} else {
					DBG cerr <<" *** Warning! error connected gegl input pad for "<<operation<<endl;
					errors++;
				}
			}

		} else {
			 //we are updating a from a non-gegl node property
			Value *v = properties.e[c]->GetData(); //<-not inc counted
			if (v) {
				ptype = propspec->GetString(1);

				GValue gv = G_VALUE_INIT;

				if (ValueToGValue(v, ptype, &gv, gegl, prop->name) == 0) {
					gegl_node_set_property(gegl, prop->name, &gv);
					num_updated++;

				} else if (ValueToProperty(v, ptype, gegl, propspec->name)==0) {
					//was possible to set directly to gegl without GValue setup

				} else {
					 //check for some thing
					errors++;
					DBG cerr <<" *** Warning! error setting properties on gegl node "<<operation<<endl;
				}

				g_value_unset(&gv);
			}
		}

	}

//	if (errors == 0) {
		 //should do this ONLY if the node is a sync
		if (OkToProcess()) {
			DBG cerr <<"..........Attempting to process "<<operation<<endl;
			gegl_node_process(gegl);
			XMLOut(gegl, operation);
		}

//	} else {
//		DBG cerr << " *** warning! "<<errors<<" encountered in GeglLaidoutNode::Update()!"<<endl;
//	}

	UpdatePreview();

	return NodeBase::Update();
}

/*! Return box.nonzerobounds().
 */
int GeglLaidoutNode::GetRect(Laxkit::DoubleBBox &box)
{
	GeglRectangle rect = gegl_node_get_bounding_box (gegl);
	box.minx = rect.x;
	box.maxx = rect.x + rect.width;
	box.miny = rect.y;
	box.maxy = rect.y + rect.height;
	return box.nonzerobounds();
}

/*! Return 0 for nothing done, or 1 for preview updated.
 */
int GeglLaidoutNode::UpdatePreview()
{
	GeglRectangle rect = gegl_node_get_bounding_box (gegl);
	if (rect.width <=0 || rect.height <= 0) return 0;

	double aspect = (double)rect.width / rect.height;
	int bufw = rect.width;
	int bufh = rect.height;
	int maxdim = 150;

	 //first determine a smallish size for the preview image, adjust total_preview if necessary.
	int ibufw = bufw, ibufh = bufh;
//	if (total_preview) {
//		ibufw = total_preview->w();
//		ibufh = total_preview->h();
//
//		if (ibufw != bufw 
//	}
//
	if (ibufw > maxdim || ibufh > maxdim) {
		if (ibufh > ibufw) {
			double scale = (double)maxdim / ibufh;
			ibufh = maxdim;
			ibufw = ibufw * maxdim;
			if (ibufw==0) ibufw = 1;
		} else {
			double scale = (double)maxdim / ibufw;
			ibufw = maxdim;
			ibufh = ibufh * maxdim;
			if (ibufh==0) ibufh = 1;
		}
	} 

	if (total_preview && (ibufw != total_preview->w() || ibufh != total_preview->h())) {
		total_preview->dec_count();
		total_preview = NULL;
	}

//	if (!total_preview) { 
//		total_preview = create_new_image(ibufw, ibufh);
//	}
//
//	unsigned char *ibuf = total_preview->getImageBuffer(); //bgra
//
//
//	GeglRectangle  roi;
//	unsigned char *buffer;
//
//	buffer = malloc (roi.w*roi.h*4);
//	gegl_node_blit (gegl,
//					1.0,
//					&roi,
//					babl_format("R'G'B'A u8"),
//					buffer,
//					GEGL_AUTO_ROWSTRIDE,
//					GEGL_BLIT_DEFAULT);
//
//	total_preview->doneWithBuffer(ibuf);

	return 1;
}

/*! Sever gegl connection if the connection is to an input pad of this.
 */
int GeglLaidoutNode::Disconnected(NodeConnection *connection, int to_side)
{
	if (connection->to == this) {
		 //remove something connected to an input pad
		int propindex = properties.findindex(connection->toprop);
		if (propindex < 0) return 0; // not found!
		if (propindex >= op->n()) {
			//is not a simpleproperty, but an input or output pad
			GeglLaidoutNode *othernode = dynamic_cast<GeglLaidoutNode*>(connection->from);
			if (!othernode) return 0; // *** for now assume it has to be a gegl connection
			gegl_node_disconnect(gegl, connection->toprop->Name());
		}
	} else {
		 //never mind about outgoing, should be taken care of elsewhere
	}

	return 0;
}

int GeglLaidoutNode::Connected(NodeConnection *connection)
{
	if (connection->to == this) {
		int propindex = properties.findindex(connection->toprop);
		if (propindex < 0) return 0; // not found!
		if (propindex >= op->n()) {
			//is not a simple property, but an input or output pad
			GeglLaidoutNode *othernode = dynamic_cast<GeglLaidoutNode*>(connection->from);
			if (!othernode) return 0; // *** for now assume it has to be a gegl connection
			gegl_node_connect_to(othernode->gegl, connection->fromprop->Name(), gegl, connection->toprop->Name());
		}
	}
	return 0;
}

int GeglLaidoutNode::SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att)
{
	int status = NodeBase::SetPropertyFromAtt(propname, att);
	//*** update the gegl node
	return status;
}

#define GEGLNODE_INPUT   (-1)
#define GEGLNODE_OUTPUT  (-2)
#define GEGLNODE_PREVIEW (-3)

/*! Set to op, and grab the spec for the properties.
 *
 * Return 0 for success, or nonzero for error.
 */
int GeglLaidoutNode::SetOperation(const char *oper)
{
	if (oper==NULL) return 1;
	if (oper!=NULL && operation!=NULL && !strcmp(oper, operation)) return 0;

	MenuInfo *menu = GetGeglOps();

	int i = menu->findIndex(oper);
	if (i<0) return 2; //unknown op!

	MenuItem *opitem = menu->e(i);

	if (opitem->info == -1) { 
		guint n_props = 0;

		GParamSpec **specs = gegl_operation_list_properties (oper, &n_props);

		opitem->info = n_props;

		menu->SubMenu("", i);

		 //add properties to the op list: name, type, nickname, blurb
		for (unsigned int c2=0; c2<n_props; c2++) {
			const gchar *nick  = g_param_spec_get_nick  (specs[c2]);
			const gchar *blurb = g_param_spec_get_blurb (specs[c2]);
			const gchar *ptype = g_type_name(specs[c2]->value_type);

			DBG cerr << "    prop: "<<specs[c2]->name <<','<< ptype <<','<< (nick?nick:"(no nick)")<<','<< (blurb?blurb:"(no blurb)")<<endl;

			//if (!strcmp(ptype, "gboolean")) ptype = "boolean";
			//else if (!strcmp(ptype, "gint")) ptype = "int";
			//else if (!strcmp(ptype, "gdouble")) ptype = "real";
			//else if (!strcmp(ptype, "gchararray")) ptype = "string";
			//else if (!strcmp(ptype, "GeglCurve")) ptype = "CurveInfo";
			//else if (!strcmp(ptype, "GeglColor")) ptype = "";
			//else if (!strcmp(ptype, "")) ptype = "";
			//else if (!strcmp(ptype, "")) ptype = "";
			//else if (!strcmp(ptype, "")) ptype = "";
			//else {
			//	DBG cerr <<" Warning! unknown gegl property type "<<ptype<<endl;
			//}

			menu->AddItem(specs[c2]->name, 0, -1);
			menu->AddDetail(ptype, NULL);
			menu->AddDetail(nick ? nick : "", NULL);
			menu->AddDetail(blurb ? blurb : "", NULL);

			//gchar **keys = gegl_operation_list_property_keys (operations[i], specs[c2]->name, &nn);
			//
			//for (int c3=0; c3<nn; c3++) {
			//	const gchar *val = gegl_param_spec_get_property_key(specs[c2], keys[c3]);
			//	printf("      key: %s, val: %s\n", keys[c3], val);
			//}
			//
			//g_free(keys);
		}

		if (specs) g_free(specs);

		menu->EndSubMenu();
	}

	makestr(operation, oper);
	makestr(Name, operation);
	makestr  (type, "Gegl/");
	appendstr(type, operation);

	if (gegl) g_object_unref (gegl);
	if (GeglLaidoutNode::masternode == NULL) {
		GeglLaidoutNode::masternode = gegl_node_new();
	}

	gegl = gegl_node_new_child(GeglLaidoutNode::masternode,
								"operation", operation,
								NULL);

	 //set up non input/output properties
	op = opitem->GetSubmenu();
	MenuItem *prop;
	const char *type;
	Value *v;
	for (int c=0; c<op->n(); c++) {
		prop = op->e(c);
		type = prop->GetString(1);
		v = NULL;

		if (type) {
			 //set default values
			GValue gv = G_VALUE_INIT;
			gegl_node_get_property(gegl, prop->name, &gv);

            if (!strcmp(type, "gboolean")) {
                bool boolean=0;
                if (G_VALUE_HOLDS_BOOLEAN(&gv)) { boolean = g_value_get_boolean(&gv); }
				v = new BooleanValue(boolean); 

            } else if (!strcmp(type, "gint")) {
                int vv=0;
                if (G_VALUE_HOLDS_INT(&gv)) { vv = g_value_get_int(&gv); }
				v = new IntValue(vv);

            } else if (!strcmp(type, "gdouble")) {
                double vv=0;
                if (G_VALUE_HOLDS_DOUBLE(&gv)) { vv = g_value_get_double(&gv); }
				v = new DoubleValue(vv);

            } else if (!strcmp(type, "gchararray")) {
                const gchar *vv = NULL;
                if (G_VALUE_HOLDS_STRING(&gv)) { vv = g_value_get_string(&gv); }
				new StringValue(vv);

			} else if (!strcmp(type, "GeglColor" )) {
				v = new ColorValue("#000");

			//} else if (!strcmp(type, "GdkPixbuf"               )) {
			//} else if (!strcmp(type, "GeglAbyssPolicy"         )) {
			//} else if (!strcmp(type, "GeglAlienMapColorModel"  )) {
			//} else if (!strcmp(type, "GeglAudioFragment"       )) {
			//} else if (!strcmp(type, "GeglBuffer"              )) {
			//} else if (!strcmp(type, "GeglColorRotateGrayMode" )) {
			//} else if (!strcmp(type, "GeglComponentExtract"    )) {
			//} else if (!strcmp(type, "GeglCurve"               )) {
			//} else if (!strcmp(type, "GeglDTMetric"            )) {
			//} else if (!strcmp(type, "GeglDitherMethod"        )) {
			//} else if (!strcmp(type, "GeglGaussianBlurFilter2" )) {
			//} else if (!strcmp(type, "GeglGaussianBlurPolicy"  )) {
			//} else if (!strcmp(type, "GeglGblur1dFilter"       )) {
			//} else if (!strcmp(type, "GeglGblur1dPolicy"       )) {
			//} else if (!strcmp(type, "GeglImageGradientOutput" )) {
			//} else if (!strcmp(type, "GeglNewsprintColorModel" )) {
			//} else if (!strcmp(type, "GeglNewsprintPattern"    )) {
			//} else if (!strcmp(type, "GeglNode"                )) {
			//} else if (!strcmp(type, "GeglOrientation"         )) {
			//} else if (!strcmp(type, "GeglPath"                )) {
			//} else if (!strcmp(type, "GeglPixelizeNorm"        )) {
			//} else if (!strcmp(type, "GeglRenderingIntent"     )) {
			//} else if (!strcmp(type, "GeglSamplerType"         )) {
			//} else if (!strcmp(type, "GeglVignetteShape"       )) {
			//} else if (!strcmp(type, "GeglWarpBehavior"        )) {
			//} else if (!strcmp(type, "GeglWaterpixelsFill"     )) {
			//} else if (!strcmp(type, "gpointer"                )) {

			}
		}

		AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, prop->name, v,1, prop->GetString(2),prop->GetString(3), c));
	}

	 //set up input pads
	gchar **pads = gegl_node_list_input_pads(gegl);
	if (pads) {
		for (int c=0; pads[c]!=NULL; c++) {
			AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, pads[c],  NULL,1, pads[c] ,NULL, GEGLNODE_INPUT));
		}
		g_strfreev(pads);
	}

	 //set up output pads
	pads = gegl_node_list_output_pads(gegl);
	if (pads) {
		for (int c=0; pads[c]!=NULL; c++) {
			AddProperty(new NodeProperty(NodeProperty::PROP_Output,  true, pads[c],  NULL,1, pads[c] ,NULL, GEGLNODE_OUTPUT));
		}
		g_strfreev(pads);
	}

	return 0;
}



//------------------------------- Gegl funcs ----------------------------

/*! Query what gegl ops are available. Properties are not scanned here, only operations.
 */
Laxkit::MenuInfo *GetGeglOps()
{
	MenuInfo *menu = dynamic_cast<MenuInfo*>(GeglLaidoutNode::op_menu.GetObject());
	if (menu) return menu;

	menu = new MenuInfo;
	GeglLaidoutNode::op_menu.SetObject(menu, 1);


    guint   n_operations;
    gchar **operations = gegl_list_operations (&n_operations);
	MenuItem *item;

	DBG cerr <<"gegl operations:"<<endl;

    for (unsigned int i=0; i < n_operations; i++) { 
        DBG cerr << "  " << (operations[i]?operations[i]:"?") << endl;
		menu->AddItem(operations[i], i, -1); 
		item = menu->e(menu->n()-1);
		item->info = -1;
    }

    g_free (operations); 

	return menu;
}

Laxkit::anObject *newGeglLaidoutNode(int p, Laxkit::anObject *ref)
{
	MenuInfo *menu = GetGeglOps();
	MenuItem *op   = menu->e(p);
	if (!op) return NULL;

	GeglLaidoutNode *node = new GeglLaidoutNode(op->name);
	return node;
}

void RegisterNodes(Laxkit::ObjectFactory *factory)
{
	MenuInfo *menu = GetGeglOps();

	char str[200];
	for (int c=0; c<menu->n(); c++) {
		sprintf(str, "Gegl/%s", menu->e(c)->name);
		factory->DefineNewObject(getUniqueNumber(), str, newGeglLaidoutNode, NULL, c);
	}
}



//-------------------------------- GeglNodesPlugin --------------------------

/*! class GeglNodesPlugin
 *
 */

GeglNodesPlugin::GeglNodesPlugin()
{
	DBG cerr <<"GeglNodesPlugin constructor"<<endl;
}

GeglNodesPlugin::~GeglNodesPlugin()
{
	if (GeglLaidoutNode::masternode != NULL) {
		g_object_unref (GeglLaidoutNode::masternode);
		GeglLaidoutNode::masternode = NULL;
	}
    gegl_exit ();
}

unsigned long GeglNodesPlugin::WhatYouGot()
{
	 //or'd list of PluginBase::PluginBaseContents
	return PluginBase::PLUGIN_Nodes;
		//| PLUGIN_Panes
		//| PLUGIN_ImageImporters
		//| PLUGIN_TextImporters
		//| PLUGIN_ImportFilters
		//| PLUGIN_ExportFilters
		//| PLUGIN_DrawableObjects
		//| PLUGIN_Tools
		//| PLUGIN_Configs
		//| PLUGIN_Resources
		//| PLUGIN_Impositions
		//| PLUGIN_Actions
		//| PLUGIN_Interpreters
		//| PLUGIN_CalcModules
	  ;
}

const char *GeglNodesPlugin::PluginName()
{
	return _("Gegl Nodes");
}

const char *GeglNodesPlugin::Version()
{
	return "0.1";
}

/*! Return localized description.
 */
const char *GeglNodesPlugin::Description()
{
	return _("Provides gegl image functionality to nodes.");
}

const char *GeglNodesPlugin::Author()
{
	return "Laidout";
}

const char *GeglNodesPlugin::ReleaseDate()
{
	return "2017 12 01";
}

const char *GeglNodesPlugin::License()
{
	return "GPL3";
}


/*! Install stuff.
 */
int GeglNodesPlugin::Initialize()
{
	DBG cerr << "GeglNodesPlugin initializing..."<<endl;

	if (initialized) return 0;

	// do stuff

	 //initialize gegl
	gegl_init (NULL,NULL); //(&argc, &argv);


	ObjectFactory *node_factory = Laidout::NodeGroup::NodeFactory(true);

	RegisterNodes(node_factory);

	DBG cerr << "GeglNodesPlugin initialized!"<<endl;
	initialized = 1;

	return 0;
}



} //namespace GeglNodesPluginNS
} //namespace Laidout


