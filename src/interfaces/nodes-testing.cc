


//------------------------ NodePanel ------------------------

/*! \class NodePanel
 * Class to hold a definition for a panel allowing random access to
 * individual properties of nodes in one much simpler panel.
 */


class NodePanel : public NodeBase
{
  public:
	class NodePanelRef
	{
	  public:
		int uihint;
		const char *text;
		NodeProperty *prop;
		NodeBase *node;

		NodePanelRef();
		virtual ~NodePanelRef();
	};

	PtrStack<NodePanelRef> props;

	NodePanel();
	NodePanel(NodeGroup *nowner, const char *nname=NULL, const char *nlabel=NULL, const char *ncomment=NULL);
	virtual ~NodePanel();
	virtual void HideAddButton();
	virtual void ShowAddButton();

	virtual int LinkProperty(NodeProperty *prop);

	// *** just another node? only with no in or out, all blocks, and changes relayed to refd props

}

NodePanel::NodePanel()
{
}

NodePanel::~NodePanel()
{
}

void NodePanel::HideAddButton()
{
}

void NodePanel::ShowAddButton()
{
}




//------------------------ PathsDataNode ------------------------

/*! \class Node for paths.
 */

class PathsDataNode
{
  public:
	LPathsData *pathsdata;

	PathsDataNode(LPathsData *path);
	virtual ~PathsDataNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
};

PathsDataNode::PathsDataNode(LPathsData *path, int absorb)
{
	pathsdata = path;
	if (pathsdata && !absorb) pathsdata->inc_count();
}

PathsDataNode::~PathsDataNode()
{
	if (pathsdata) pathsdata->dec_count();
}

NodeBase *PathsDataNode::Duplicate()
{
	PathsDataNode *node = new PathsDataNode(pathsdata);
	node->DuplicateBase(this);
	return node;
}

int PathsDataNode::Update()
{
	***
	return NodeBase::Update();
}

int PathsDataNode::GetStatus()
{

	return NodeBase::GetStatus();
}



//---------------------
class PathGeneratorNode
{
  public:
	enum PathTypes {
		Square,
		Circle,     //num points, is bez, start, end
		Polygon, // vertices, winds
		Function, //y=f(x), x range, step
		FunctionT, //p=(x(t), y(t)), t range, step
		MAX
	};
	PathTypes pathtype;

	PathGeneratorNode(PathTypes ntype);
	virtual ~PathGeneratorNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
};

PathGeneratorNode::PathGeneratorNode(PathTypes ntype)
{
	pathtype = ntype;

	if (pathtype == Square) {
		makestr(type, "Path/Square");
		makestr(Name, _("Square");
		//no inputs! always square 0..1

	} else if (pathtype == Circle) {
		makestr(type, "Path/Circle");
		makestr(Name, _("Circle");
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "n",      new IntValue(4),1,     _("Points"), _("Number of points")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Start",  new DoubleValue(0),1,  _("Start"),  _("Start angle")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "End",    new DoubleValue(0),1,  _("End"),    _("End angle. Same as start means full circle.")));
		//AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Smooth", new BooleanValue(1),1, _("Smooth"), _("Is a bezier curve"));
		// if you don't want smooth, use polygon

	} else if (pathtype == Polygon) {
		makestr(type, "Path/Polygon");
		makestr(Name, _("Polygon");
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "n",      new IntValue(4),1,     _("Points"), _("Number of points")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Winding",new DoubleValue(1),1,  _("Winding"),_("Angle between points is winding*360/n")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Offset", new DoubleValue(0),1,  _("Offset"), _("Rotate all points by this many degrees")));

	} else if (pathtype == Function) {
		makestr(type, "Path/FunctionX");
		makestr(Name, _("Function of x");
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Y", new StringValue("x"),1, _("Y as a function of x")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Minx", new DoubleValue(0),1,  _("Min x"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Maxy", new DoubleValue(1),1,  _("Max x"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Step", new DoubleValue(.1),1, _("Step"), NULL));

	} else if (pathtype == FunctionT) {
		makestr(type, "Path/FunctionT");
		makestr(Name, _("Function of t");
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "X", new StringValue("t"),1, _("X as a function of t")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Y", new StringValue("t"),1, _("Y as a function of t")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Mint", new DoubleValue(0),1,  _("Min t"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Maxt", new DoubleValue(1),1,  _("Max t"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Step", new DoubleValue(.1),1, _("Step"), NULL));
	}

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", new LPathsData(),1, _("Out"), NULL,0, false));
}

PathGeneratorNode::~PathGeneratorNode()
{
}

NodeBase *PathGeneratorNode::Duplicate()
{
	PathGeneratorNode *newnode = new PathGeneratorNode(pathtype);

	for (int c=0; c<properties.n-1; c++) {
		Value *v = properties.e[c]->GetData();
		if (v) newnode->properties.e[c]->SetData(v->duplicate(), 1);
	}

	return newnode;
}

int PathGeneratorNode::GetStatus()
{
	***
	return NodeBase::GetStatus();
}

int PathGeneratorNode::Update()
{
	makestr(error_message, NULL);

	PathsData *path = dynamic_cast<PathsData>(properties.e[properties.n-1]->GetData());
	if (!path) {
		path = dynamic_cast<PathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
		properties.e[properties.n-1]->SetData(path, 1);
	}

	if (pathtype == Rectangle) {
		path->appendRect(0,0,1,1);

	} else if (pathtype == Circle) {
		path->clear();
		***
		for (int c=0; c<n; c++) {
			path.append();
		}

	} else if (pathtype == Polygon) {
		path->clear();
		***

		if (start == end) end = start + M_PI;
		double dtheta = 2*M_PI * winding / npoints;

	} else if (pathtype == Function) {
		if (start<end && step<=0 || start>end && step>=0) {
			makestr(error_message, _("Bad step value"));
			return -1;
		}
		if (start == end) {
			makestr(error_message, _("Start can't equal end"));
			return -1;
		}

		path->clear();

		Value *ret = NULL;
		DoubleValue *d;
		DoubleValue xx;
		ValueHash hash;
		hash.push("x", &xx);
		int status;
		int pointsadded = 0

		for (double x = start; (start < end && x <= end) || (start > end && x>=end); x += step) {
			xx.d = x;
			status = calculator->evaluate(expression, &hash, *** , &ret);
			d = dynamic_cast<DoubleValue*>(ret);
			if (d) {
				path.append(x, d->d);
				pointsadded++;
			}
		}
		hash.flush();

	} else if (pathtype == FunctionT) {
		if (start<end && step<=0 || start>end && step>=0) {
			makestr(error_message, _("Bad step value"));
			return -1;
		}
		if (start == end) {
			makestr(error_message, _("Start can't equal end"));
			return -1;
		}

		path->clear();

		ValueHash hash;
		Value *ret = NULL;
		DoubleValue *rx;
		DoubleValue tt;
		hash.push("t", &tt);
		int status;
		int pointsadded = 0

		for (double t = start; (start < end && t <= end) || (start > end && t>=end); t += step) {
			tt->d = t;
			status = calculator->evaluate(expressionX, &hash, *** , &ret);
			rx = dynamic_cast<DoubleValue*>(ret);
			status = calculator->evaluate(expressionY, &hash, *** , &ret);
			ry = dynamic_cast<DoubleValue*>(ret);

			if (rx && ry) {
				path.append(rx->d, ry->d);
				pointsadded++;
			}
		}
	}

	return NodeBase::Update();
}


Laxkit::anObject *newCircleNode(int p, Laxkit::anObject *ref)
{
	return new PathGeneratorNode(PathGeneratorNode::Circle);
}

Laxkit::anObject *newRectangleNode(int p, Laxkit::anObject *ref)
{
	return new PathGeneratorNode(PathGeneratorNode::Rectangle);
}

Laxkit::anObject *newPolygonNode(int p, Laxkit::anObject *ref)
{
	return new PathGeneratorNode(PathGeneratorNode::Polygon);
}

Laxkit::anObject *newPathFunctionNode(int p, Laxkit::anObject *ref)
{
	return new PathGeneratorNode(PathGeneratorNode::Function);
}

Laxkit::anObject *newPathFunctionTNode(int p, Laxkit::anObject *ref)
{
	return new PathGeneratorNode(PathGeneratorNode::FunctionT);
}


	factory->DefineNewObject(getUniqueNumber(), "Circle",newCircleNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "RectanglePath",newRectangleNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Polygon",newPolygonNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "PathFunctionX",newPathFunctionNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "PathFunctionT",newPathFunctionTNode, NULL, 0);


//------------------------ Path Effects ------------------------

// Input one or more paths. Do something to it. Output results


//------------------------ Generators for 3d ------------------------


// Hedron operators:
//   dup spin
//   extrude faces
//   inset faces
//   deformation

class Path3dGenerator
{
  public:
	PathGenerator();
	virtual ~PathGenerator();

	
	NumStack<spacevector> points;

};

int CircleGenerator(double radius, spacevector center, Polyhedron *poly)
{
}

int GridPoints(
		double xwidth, double xstep,
		double ywidth, double ystep,
		double zwidth, double zstep,
		spacevector center, Polyhedron *poly)
{
}

/*! Cube centered at center.
 */
int Cube( double xwidth, double ywidth, double zwidth, spacevector center, Polyhedron *poly)
{
}



//----------------------- ScriptNode ------------------------

/*! \class ScriptNode
 * A runnable script with optional parameters.
 *
 * If the final value is a hash, then output links are created corresponding to those.
 * Otherwise, a single out property is made for the final value in the expression.
 */
class ScriptNode : public NodeBase
{
  public:
	PlainText *script;
	Interpreter *interpreter;
	Value *output;

	ScriptNode();
	virtual ~ScriptNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
};

ScriptNode::ScriptNode(const char *expression, const char *runner)
{
	//script = new PlainText(expression);
	//script = FindResource("PlainText", textobject);
	//if (script) script->inc_count();

	interpreter = GetInterpreter(runner);

	output = NULL;
}

ScriptNode::~ScriptNode()
{
	if (script) script->dec_count();
	if (interpreter) interpreter->dec_count();
	if (output) output->dec_count();
}

NodeBase *ScriptNode::Duplicate()
{
	ScriptNode *node = new ScriptNode(script, interpreter);

	***
	
	return node;
}

int ScriptNode::Update()
{
}

int ScriptNode::GetStatus()
{
}

/*! Set up inputs. These can be accessed from the expression by name.
 */
int ScriptNode::AddParameters(const char *pname, const char *pName, const char *pTip, ...)
{
	***
}

/*! Create these as output properties. The script must return a hash with keys corresponding
 * to the pname.
 */
int ScriptNode::AddOutputs(const char *pname, const char *pName, const char *pTip, ...)
{
	***
}

int ScriptNode::Run()
{
}

//------------------------------ NumToStringNode --------------------------------------------

class NumToStringNode : public NodeBase
{
  public:
	NumToStringNode(double d, int isint);
	virtual ~NumToStringNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
};

NumToStringNode::NumToStringNode(double d, int isint)
{
	makestr(Name, _("NumToString"));
	makestr(type, "NumToString");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "N", isint ? new IntValue(d) : new DoubleValue(d),1, _("N"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Padding", new StringValue(""),1, _("Padding"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Decimals", new StringValue(""),1, _("Decimals"))); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL,1, _("Out"))); 
}

NumToStringNode::~NumToStringNode()
{
}

NodeBase *NumToStringNode::Duplicate()
{
	StringValue *s1 = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	StringValue *s2 = dynamic_cast<StringValue*>(properties.e[1]->GetData());

	NumToStringNode *newnode = new NumToStringNode(s1 ? s1->str : NULL, s2 ? s2->str : NULL);
	newnode->DuplicateBase(this);
	return newnode;
}

int NumToStringNode::GetStatus()
{ ***
	if (!isNumberType(properties.e[0]->GetData(), NULL) && !dynamic_cast<StringValue*>(properties.e[0]->GetData())) return -1;
	if (!isNumberType(properties.e[1]->GetData(), NULL) && !dynamic_cast<StringValue*>(properties.e[1]->GetData())) return -1;

	if (!properties.e[2]->data) return 1;

	return NodeBase::GetStatus(); //default checks mod times
}

int NumToStringNode::Update()
{ ***
	char *str=NULL;

	int isnum=0;
	double d = getNumberValue(properties.e[0]->GetData(), &isnum);
	if (isnum) {
		str = numtostr(d);
	} else {
		StringValue *s = dynamic_cast<StringValue*>(properties.e[0]->GetData());
		if (!s) return -1;
		
		str = newstr(s->str);
	}

	d = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (isnum) {
		char *ss = numtostr(d);
		appendstr(str, ss);
		delete[] ss;
	} else {
		StringValue *s = dynamic_cast<StringValue*>(properties.e[1]->GetData());
		if (!s) return -1;
		
		appendstr(str, s->str);
	}

	StringValue *out = dynamic_cast<StringValue*>(properties.e[2]->GetData());
	out->Set(str);

	delete[] str;

	return NodeBase::Update();
}


Laxkit::anObject *newNumToStringNode(int p, Laxkit::anObject *ref)
{ ***
	return new NumToStringNode(NULL,NULL);
}



//------------------------------ MathConstantsNode --------------------------------------------

class MathContstantNode : public NodeBase
{
	static SingletonKeeper defkeeper; //the def for the op enum
	static ObjectDef *GetDef() { return dynamic_cast<ObjectDef*>(defkeeper.GetObject()); }

  public:
	MathContstantNode(int which);
	virtual ~MathContstantNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus() { return 0; }
};

ObjectDef *DefineMathConstants()
{
	ObjectDef *def = new ObjectDef("MathConstants", _("Various constants"), NULL,NULL,"enum", 0);

	 //1 argument
	def->pushEnumValue("pi"   ,_("pi"),   _("3.1415"), CONST_Pi        );
	def->pushEnumValue("pi_2" ,_("pi/2"), _("1.5708"), CONST_Pi_Over_2 );
	def->pushEnumValue("pi2"  ,_("2*pi"), _("6.2832"), CONST_2_Pi      );
	def->pushEnumValue("e"    ,_("e"),    _("2.7182"), CONST_E         );
	def->pushEnumValue("tau"  ,_("tau"),  _("1.6180"), CONST_Tau       );
	def->pushEnumValue("taui" ,_("1/tau"),_("0.6180"), CONST_1_Over_Tau);

	return def;
}

SingletonKeeper MathConstantNode::defkeeper(DefineMathConstants(), true);

MathContstantNode::MathContstantNode(int which)
{
	ObjectDef *enumdef = GetDef();
	EnumValue *e = new EnumValue(enumdef, 0);
	node->AddProperty(new NodeProperty(NodeProperty::PROP_Block, false, "C", e, 1, "")); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", new DoubleValue(0),1, _("Out")));
}

MathContstantNode::~MathContstantNode()
{
}

NodeBase *MathContstantNode::Duplicate()
{
	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	MathContstantNode *newnode = new MathContstantNode(ev->value);
	newnode->DuplicateBase(this);
	return newnode;
}

int MathContstantNode::Update()
{
	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	ObjectDef *def = ev->GetObjectDef();

	DoubleValue *v = dynamic_cast<DoubleValue*>(properties.e[1]->GetData());
	int id = def->EnumId();

	if      (id == CONST_Pi        ) v->d = M_PI;
	else if (id == CONST_Pi_Over_2 ) v->d = M_PI/2;
	else if (id == CONST_2_Pi      ) v->d = 2*M_PI;
	else if (id == CONST_E         ) v->d = M_E;
	else if (id == CONST_Tau       ) v->d = (1+sqrt(5))/2;
	else if (id == CONST_1_Over_Tau) v->d = 2/(1+sqrt(5));

	return NodeBase::Update();
}


//------------------------------ ConvertNumberNode --------------------------------------------

class ConvertNumberNode : public NodeBase
{
  public:
	ConvertNumberNode();
	virtual ~ConvertNumberNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
};


ObjectDef *DefineConvertDef()
{
	ObjectDef *def = new ObjectDef("ConvertDef", _("Number Conversions"), NULL,NULL,"enum", 0);

	UnitManager *units = GetUnitManager();

	//units should be: singular plural abbreviation  localized_Name localized_Description
	char *shortname, *singular, *plural;
	double scale;
	int id;

	for (int c=0; c<units->NumberOfUnits(); c++) {
		UnitInfoIndex(c, &id, &scale, &shortname, &singular, &plural);
		def->pushEnumValue(singular, singular, singular, id);
	}


	return def;
}

ConvertNumberNode::ConvertNumberNode()
{
	makestr(Name, _("Convert number"));
	makestr(type, "ConvertNumber");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "In",    new DoubleValue(d),1,  _("In"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, "From", new EnumValue(from),1, _("From"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, "To",   new EnumValue(to),1,   _("To"))); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out",  new DoubleValue(ns),1, _("Out")), NULL, 0, false); 
}

ConvertNumberNode::~ConvertNumberNode()
{
}

NodeBase *ConvertNumberNode::Duplicate()
{
	double in = getNumberValue(properties.e[0]->GetData());
	EnumValue *f = dynamic_cast<EnumValue*>(properties.e[1]->GetData())->duplicate();
	EnumValue *t = dynamic_cast<ENumValue*>(properties.e[2]->GetData())->duplicate();

	ConvertNumberNode *newnode = new ConvertNumberNode(in, f->value, t->value);
	newnode->DuplicateBase(this);
	return newnode;
}

int ConvertNumberNode::GetStatus()
{ ***
	if (!isNumberType(properties.e[0]->GetData(), NULL) && !dynamic_cast<StringValue*>(properties.e[0]->GetData())) return -1;
	if (!isNumberType(properties.e[1]->GetData(), NULL) && !dynamic_cast<StringValue*>(properties.e[1]->GetData())) return -1;

	if (!properties.e[2]->data) return 1;

	return NodeBase::GetStatus(); //default checks mod times
}

int ConvertNumberNode::Update()
{ ***

	return NodeBase::Update();
}


Laxkit::anObject *newConvertNumberNode(int p, Laxkit::anObject *ref)
{
	return new ConvertNumberNode();
}


//------------------------------ HistogramNode --------------------------------------------


/*! If to==NULL, then return a new LaxImage. Else pack into that one.
 * Divide color range into resolution steps.
 */
LaxImage *Histogram(LaxImage *from, LaxImage *to, int resolution, bool ignorealpha)
{
	int channels = 4;
	if (ignorealpha) channels--;

	if (!to) to = create_new_image(200,200);
	unsigned char *buffer = from->getImageBuffer(); //bgra

    int width  = from->w();
    int height = from->h();

	int h[channels][resolution];

	 //sample source
	int i=0;
	int v;
	int max  = 256; //1 more than greatest value
	int max1 = max-1; //1 more than greatest value

	for (int y=0; y<height; y++) {
		for (int x=0; x<width; x++) {
			for (int c=0; c<channels; c++) {
				v = buffer[i+c];
				if (v>0) {
					h[c][v * resolution / max]++;
				}
			}
			i += 4;
		}
	}
	from->doneWithBuffer(buffer)

	 //build histogram image
	int cmax[channels];
	int chmax = 0;
	for (int c=0; c<channels; c++) {
		cmax[c] = 0;
		for (int r=0; r<resolution; r++) {
			if (h[c][r] > cmax[c]) cmax[c] = h[c][r];
		}
		if (cmax[c] > chmax) chmax = cmax[c];
	}

	buffer = to->getImageBuffer(); //bgra
    width  = to->w();
    height = to->h();
	memset(buffer, 0, width*height*4);

	int xx;
	int yh;
	int w4 = 4*width;
	for (int x=0; x<width; x++) {
		xx = x*resolution / width;
		for (int c=0; c<channels; c++) {
			i = xx;
			yh = h[channels][xx] *height / chmax;
			for (int y=0; y < yh; y++) {
				buffer[i][c] = max1;

				i += w4;
			}
		}
	}

	to->doneWithBuffer(buffer)
	return to;
}

class HistogramNode : public NodeBase
{
  public:
	HistogramNode(int w, int h, int r);
	virtual ~HistogramNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();
};

HistogramNode::HistogramNode(int w, int h, int r)
{
	if (w==0) w=h;
	if (w==0) w=100;
	if (h==0) h=w;
	if (h==0) h=100;
	if (r<=0) r=256;

	makestr(Name, _("Histogram"));
	makestr(type, "Histogram");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "In",    NULL,1,  _("Image in"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Width",      new IntValue(w),1,   _("Width"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Height",     new IntValue(h),1,   _("Height"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Resolution", new IntValue(res),1, _("Resolution"))); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, false, "Out",  NULL,1, _("Out"))); 
}

HistogramNode::~HistogramNode()
{
}

NodeBase *HistogramNode::Duplicate()
{
	int isnum;
	int w = getNumberValue(properties.e[1]->GetData(), &isnum);
	int h = getNumberValue(properties.e[2]->GetData(), &isnum);
	int r = getNumberValue(properties.e[3]->GetData(), &isnum);
	HistogramNode *newnode = new HistogramNode(w,h,r);
	newnode->DuplicateBase(this);
	return newnode;
}

int HistogramNode::GetStatus()
{
	int isnum;
	int w = getNumberValue(properties.e[1]->GetData(), &isnum);  if (!isnum || w<=0) return -1;
	int h = getNumberValue(properties.e[2]->GetData(), &isnum);  if (!isnum || h<=0) return -1;
	int r = getNumberValue(properties.e[3]->GetData(), &isnum);  if (!isnum || r<=0) return -1;
	return NodeBase::GetStatus();
}

int HistogramNode::Update()
{
	int isnum;
	int w = getNumberValue(properties.e[1]->GetData(), &isnum);  if (!isnum || w<=0) return -1;
	int h = getNumberValue(properties.e[2]->GetData(), &isnum);  if (!isnum || h<=0) return -1;
	int r = getNumberValue(properties.e[3]->GetData(), &isnum);  if (!isnum || r<=0) return -1;

	LaxImage *in   = dynamic_cast<LaxImage*>(properties.e[0]->GetData());
	if (!in) return -1;

	LaxImage *himg = dynamic_cast<LaxImage*>(properties.e[4]->GetData());

	LaxImage *himgr = Histogram(in, himg, r, false);
	if (himgr) properties.e[4]->SetData(himgr, (himgr == himg ? 0 : 1));
	else return -1;

	return 0;
}


//------------------------------ GradientStripProperty --------------------------------------------

SingletonKeeper GradientStripProperty::interfacekeeper;
class GradientStripProperty : public NodeProperty
{
	static SingletonKeeper interfacekeeper;

  public:
	static LaxInterfaces::GradientInterface *GetGradientInterface();


	GradientStrip *gradient;
	GradientData *gdata;

	GradientStripProperty(GradientStrip *ngradient, int absorb);
	virtual ~GradientStripProperty();

	//virtual bool AllowType(Value *v);

	virtual LaxInterfaces::anInterface *PropInterface(LaxInterfaces::anInterface *interface, Laxkit::Displayer *dp);
	virtual const char *PropInterfaceName() { return "GradientInterface"; }
	virtual bool HasInterface();
};

GradientStripProperty::GradientStripProperty(GradientStripValue *ngradient, int absorb)
{
	type = NodeProperty::PROP_Block;
	is_linkable = false;
	makestr(name, "Curve");
	makestr(label, _("Curve"));
	//makestr(tooltip, _(""));

	gradient = ngradient;
	if (gradient && !absorb) gradient->inc_count();
}

GradientStripProperty::~GradientStripProperty()
{
	if (gradient) gradient->dec_count();
}

bool GradientStripProperty::HasInterface()
{
	return true;
}

LaxInterfaces::GradientInterface *GradientStripProperty::GetGradientInterface()
{
	GradientInterface *interf = dynamic_cast<GradientInterface*>(interfacekeeper.GetObject());
	if (!interf) {
		interf = new GradientInterface(getUniqueNumber(), NULL);
		//interface->style |= CurveMapInterface::RealSpace;
		interfacekeeper.SetObject(interf, 1);
	}
	return interf;
}

/*! If interface!=NULL, then try to use it. If NULL, create and return a new appropriate interface.
 * Also set the interface to use *this.
 */
LaxInterfaces::anInterface *GradientStripProperty::PropInterface(LaxInterfaces::anInterface *interface, Laxkit::Displayer *dp)
{
	GradinteInterface *interf = NULL;
	if (interface) {
		if (strcmp(interface->whattype(), "GradinteInterface")) return NULL;
		interf = dynamic_cast<GradinteInterface*>(interface);
		if (!interf) return NULL; //wrong ref interface!!
	}

	if (!interf) interf = GetGradientInterface();

	interf->Dp(dp);
	interf->UseThis(gradient);
	interf->SetupRect(cowner->x + x, cowner->y + y, width, height);

	return interf;
}


//------------------------------ ColorMapNode --------------------------------------------

class ColorMapNode : public NodeBase
{
  public:
	ColorMapNode();
	virtual ~ColorMapNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
};

ColorMapNode::ColorMapNode()
{ ***
}

ColorMapNode::~ColorMapNode()
{ ***
}

NodeBase *ColorMapNode::Duplicate()
{ ***
	ColorMapNode *node = new ColorMapNode(start, end, step);
	node->DuplicateBase(this);
	return node;
}

int ColorMapNode::Update()
{ ***
}

int ColorMapNode::GetStatus()
{ ***
}


//------------------------------ CompileMath --------------------------------------------


/*! Take math nodes and create an equivalent expression.
 */
ScriptNode *CompileMath(RefPtrStack<NodeBase> &nodes, Interpreter *interpreter, RefPtrStack<NodeBase> &remainder)
{
}



//------------------------ ForeachNode ------------------------

/*! \class For loop node.
 */

class ForeachNode : public NodeBase
{
  public:
	int current;
	int running;

	ForeachNode(double nstart, double nend, double nstep);
	virtual ~ForeachNode();

	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();
	virtual NodeBase *Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks);
};


ForeachNode::ForeachNode(double nstart, double nend, double nstep)
{
	current = -1;
	running = 0;

	makestr(type,   "Threads/Foreach");
	makestr(Name, _("Foreach"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_In,  true, "In",    NULL,1, _("In")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_Out, true, "Done",  NULL,1, _("Done")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_Out, true, "Foreach",  NULL,1, _("Foreach")));
	 // *** while in loop, run to loop.
	 // when thread dead ends, return to this node and go out on Done

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Data", new DoubleValue(start),1,_("Start"), NULL));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Index",   new IntValue(current),1, _("Index"),   NULL, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Element", NULL,1, _("Element"), NULL, 0, false));
}

ForeachNode::~ForeachNode()
{
}

NodeBase *ForeachNode::Duplicate()
{
	ForeachNode *node = new ForeachNode();
	node->DuplicateBase(this);
	return node;
}

int ForeachNode::GetStatus()
{
	return 0;
}

NodeBase *ForeachNode::Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks)
{
	NodeProperty *done = properties.e[1];
	NodeProperty *loop = properties.e[2];

	NodeBase *next = NULL;

	int max = 0;
	Value *vin = properties.e[3]->GetData();

	if (vin->type() == VALUE_Hash) {
	} else if (vin->type() == VALUE_Set) {
	} //else vectors? each point in a path?


	if (!running && max>0) {
		 //initialize loop
		running = 1;
		current = 0;

		if      (loop->connections.n) next = loop->connections.e[0]->to;
		else if (done->connections.n) next = done->connections.e[0]->to;
		else next = this;

		dynamic_cast<IntValue*>(properties.e[4]->GetData())->i = current;
		properties.e[4]->Touch();
		PropagateUpdate();
		thread->scopes.push(this);
		return next;
	}

	 //update current
	if (current == max) {
		 //end loop
		running = 0;
		if (done->connections.n) next = done->connections.e[0]->to;
		else next = NULL;
		thread->scopes.remove(-1);

	} else {
		 //continue loop
		current++;
		if (loop->connections.n) next = loop->connections.e[0]->to;
		else next = this;

		dynamic_cast<IntValue*>(properties.e[4]->GetData())->i = current;
		properties.e[4]->Touch();
		PropagateUpdate();
	}

	return next;
}

int ForeachNode::Update()
{
	makestr(error_message, NULL);

	int isnum;
	start = getNumberValue(properties.e[3]->GetData(), &isnum);  if (!isnum) return -1;
	end   = getNumberValue(properties.e[4]->GetData(), &isnum);  if (!isnum) return -1;
	step  = getNumberValue(properties.e[5]->GetData(), &isnum);  if (!isnum) return -1;

	if ((start>end && step>=0) || (start<end && step<=0)) {
		makestr(error_message, _("Bad step value"));
		return -1;
	}
	return NodeBase::Update();
}


Laxkit::anObject *newForeachNode(int p, Laxkit::anObject *ref)
{
	return new ForeachNode(0,10,1);
}


//------------------------------ SwizzleNode --------------------------------------------

/*! \class SwizzleNode
 * Map arrays to other arrays using a special Swizzle interface.
 */

class SwizzleNode : public NodeBase
{
  public:
	SwizzleNode();
	virtual ~SwizzleNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
};

SwizzleNode::SwizzleNode()
{ ***
	makestr(type, "Swizzle");
	makestr(Name, NULL);
}

SwizzleNode::~SwizzleNode()
{ ***
}

NodeBase *SwizzleNode::Duplicate()
{ ***
	SwizzleNode *node = new SwizzleNode(start, end, step);
	node->DuplicateBase(this);
	return node;
}

int SwizzleNode::Update()
{ ***
}

int SwizzleNode::GetStatus()
{ ***
}

Laxkit::anObject *newSwizzle(int p, Laxkit::anObject *ref)
{
	return new ForkNode();
}


//------------------------------ ActionNode --------------------------------------------

/*! \class ActionNode
 * Thread class to perform some action.
 *
 * ----Action---
 * o  In
 *       Out o
 * [o] New through
 * [o] New In
 * str: Expression
 *     New Out [o]
 */

