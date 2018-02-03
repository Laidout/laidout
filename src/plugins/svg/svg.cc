

#include <exception>


//Read in svg filter, and convert to LaidoutGeglNodes


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
	SvgFilterNode(const char *filterName, ValueHash *hash);
    //SvgFilterNode(const char *filterName, SvgFilterNode *in, SvgFilterNode *in2);
    virtual ~SvgFilterNode();

    //virtual int SetFilter(const char *filter);
    //virtual int UpdateProperties();
    //virtual int Update();
    //virtual int UpdatePreview();
    //virtual int Disconnected(NodeConnection *connection, int to_side);
    //virtual int Connected(NodeConnection *connection);
    //virtual int SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att);

	virtual const char *ResultName();
};


Laxkit::SingletonKeeper SvgFilterNode::svg_def_keeper;


///*! \class SvgFilterNode
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


SvgFilterNode::SvgFilterNode(const char *filterName, ValueHash *hash)
{
	makestr(Name, filterName);

	ObjectDef *svgdefs = GetSvgDefs();
	ObjectDef *fdef = svgdefs->FindDef(filterName);

	if (fdef) {
		InstallDef(fdef, 0);
		int issrc = (strcmp(fdef->name, "SvgSource") == 0);

		//add linkable image in
		if (issrc) AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "imageIn", NULL,1, _("In"),_("Input image"), 0, false));

		Value *v = NULL;

		for (int c=0; c<fdef->getNumFields(); c++) {
			ObjectDef *d = fdef->getField(c);
			if (!d) continue;

			if (d->format == VALUE_Number) v = new DoubleValue;
			else if (d->format == VALUE_String) v = new StringValue;
			else if (d->format == VALUE_Enum) {
				v = new EnumValue(d,0);
			}

			AddProperty(new NodeProperty(issrc ? NodeProperty::PROP_Output : NodeProperty::PROP_Input, true, d->name, v,1, d->Name, d->description, c));
		}

		//add linkable image in
//		if (fdef->FindDef("in"))
//			AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "inImg", NULL,1, _("Image in"),_("Image input, possibly modified by in property"), 0, false));
//
//		if (fdef->FindDef("in2"))
//			AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "inImg2", NULL,1, _("Image2 in"),_("Other Image input, possibly modified by in2 property"), 0, false));

		//add linkable image out
		if (!issrc) AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", NULL,1, _("Out"),_("The resulting image"), 0, false));
	}
}

SvgFilterNode::~SvgFilterNode()
{
}



/*! Convenience to get the result name, not the actual image output.
 */
const char *SvgFilterNode::ResultName()
{
	NodeProperty *p = FindProperty("result");
	return p ? p->name : NULL;
}


//---------------------- SvgFilter defs ----------------------------

ObjectDef *GetSvgDefs()
{
	ObjectDef *svgdefs = dynamic_cast<ObjectDef*>(SvgFilterNode::svg_def_keeper.GetObject());
	if (svgdefs) return svgdefs;

	svgdefs = new ObjectDef("SvgFilter", "SVG Filter", "SVG Filter", NULL, "class", 0);
	SvgFilterNode::svg_def_keeper.SetObject(svgdefs, 1);


	//in = "SourceGraphic | SourceAlpha | BackgroundImage | BackgroundAlpha | FillPaint | StrokePaint | <filter-primitive-reference>"
	ObjectDef *in = new ObjectDef("SvgFilterIn", _("In"), _("SVG Filter Container"), NULL, "class", 0);
	svgdefs->push(in,1);

	in->pushEnum("in", "In", NULL,  "SourceGraphic", NULL, NULL,
					"SourceGraphic",   _("Source"),           NULL,
					"SourceAlpha",     _("Source Alpha"),     NULL,
					"BackgroundImage", _("Background"),       NULL,
					"BackgroundAlpha", _("Background Alpha"), NULL,
					"FillPaint",       _("Fill Paint"),       NULL,
					"StrokePaint",     _("Stroke Paint"),     NULL,
					"Reference",       _("Reference"),        NULL,
					NULL);
	in->push("inRef", _("Ref"), _("Used when in is Reference"), "string", NULL, NULL, 0, NULL);


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
		"name SvgFilterStuff\n"
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

	// *** replace all "in" and "in2" with SvgFilterIn def

	// *** try to unify feFunc*


	return svgdefs;
}


//-------------------- SVG filter importing --------------------------


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
			SetValue *v = new SetValue();
			hash->push(".content", v, 1);

			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name  = att->attributes.e[c]->attributes.e[c2]->name;
				value = att->attributes.e[c]->attributes.e[c2]->value;
			}

		} else {
			ObjectDef *def = svgdefs->FindDef(name);
			if (def) {
				//try to convert value to a Value
				Value *val = NULL; // ***

				if (val) hash->push(name, value);
				else hash->push(name, value);

			} else {
				hash->push(name, value);
			}

			//if (!strcmp(name, "id")) {
			//} else if (!strcmp(name, "inkscape:label")) {
			//} else if (!strcmp(name, "inkscape:menu")) {
			//} else if (!strcmp(name, "inkscape:menu-tooltip")) {
			//} else if (!strcmp(name, "style")) {
			//}
		}
	}

	return hash;
}


/*! Load in filters extracted from an svg file.
 * If which_filter == NULL, then extract all. Else just that one.
 */
int LoadSVGFilters(const char *file, const char *which_filter)
{
	ObjectDef *svgdefs = GetSvgDefs();

	Attribute *att = XMLFileToAttribute(NULL, file, NULL);
	if (!att) return 1;

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

		//ValueHash *hash = new ValueHash;

		const char *name, *value;
		for (int c=0; c<att->attributes.n; c++) {
			name  = att->attributes.e[c]->name;
			value = att->attributes.e[c]->value;

			if (!strcmp(name, "filter")) {

				if (which_filter) {
					const char *aa = att->attributes.e[c]->findValue("id");
					if (aa && strcmp(aa, which_filter)) {
						aa = att->attributes.e[c]->findValue("inkscape:label");
						if (aa && strcmp(aa, which_filter)) {
							continue;
						}
					}

				}

				//ValueHash *h = XMLAttToHash(svgdefs, att->attributes.e[c], NULL, log);

				//---
			}
		}


	} catch (exception &e) {
		DBG cerr <<" *** error parsing svg file: "<<e.what()<<endl;
		//***
	}

	return 1;
}



//-------------------- Node installation --------------------------


Laxkit::anObject *newSvgFilterNode(int p, Laxkit::anObject *ref)
{
	ObjectDef *svgdefs = GetSvgDefs();
	ObjectDef *def = svgdefs->getField(p);
	if (!def) return NULL;

    SvgFilterNode *node = new SvgFilterNode(def->name, NULL);
    return node;
}

const char *filters[] = {
		"SvgSource",
		"feBlend",
		"feColorMatrix",
		"feComponentTransfer",
		"feComposite",
		"feConvolveMatrix",
		"feDiffuseLighting",
		"feDisplacementMap",
		"feFlood ",
		"feGaussianBlur",
		"feImage ",
		"feMerge ",
		"feMorphology",
		"feOffset",
		"feSpecularLighting",
		"feTile",
		"feTurbulence",
		NULL
	};

void RegisterSvgNodes(Laxkit::ObjectFactory *factory)
{
	ObjectDef *svgdefs = GetSvgDefs();

    char str[200];
    ObjectDef *def;

    for (int c=0; c < svgdefs->getNumFields(); c++) {
        def = svgdefs->getField(c);
		if (findInList(def->name, filters) < 0) continue;

		sprintf(str, "Svg Filter/%s", def->name);
		factory->DefineNewObject(getUniqueNumber(), str, newSvgFilterNode, NULL, c);
    }
}



} //namespace SvgFilterNS
} //namespace Laidout




