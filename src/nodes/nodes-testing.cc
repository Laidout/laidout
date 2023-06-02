
//*********** WORKS IN PROGRESS ****************

// TODO:
//   PathCutAt -> cut points, or cut out segments, return point list of new endpoints
//     CutPathNear:
//       at t value
//       +- s 
//       bool slice_only .. do not actually remove segment, just add points
//       bool keep_handles .. when !slice_only, preserve bez handles, else delete them too
//   PathBooleanNode
//   PathCorners
//   EditDrawable:
//       Transform relative to filter in
//       obj
//   TransformSpace, from object to object (must share an ancestor)
//   FitIn
//   AlignToBounds
//   AlignNode //using AlignInfo
//   TilingNode
//   set min/max/mean/mode
//   ObjectArrayNode // linear, radial, object target
//   FlipNode
//   MergePaths
//   Printf
//   MenuValue
//   NineSlice
// 
//   EulerToQuaternion
//   QuaternionToEuler
//   Swizzle


//----------------------- NineSliceNode ------------------------

/*! \class NineSliceNode
 *
 * Do stuff.
 */
class NineSliceNode : public NodeBase
{
  public:
	NineSliceNode();
	virtual ~NineSliceNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new NineSliceNode(); }
};

NineSliceNode::NineSliceNode()
{
	makestr(type, "Images/NineSlice");
	makestr(Name, _("Nine Slice"));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "base",   NULL,1,     _("Base"), _("Image to be cut")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "width",  new IntValue(100),1,   _("Width of final image")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "height", new IntValue(100),1,   _("Height of final image")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "scale",  new DoubleValue(1),1,  _("Extra scale for edges")));
	
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "hslice", new FlatvectorValue(.3,.3),1, _("H slice"),  _("Positions of horizontal slice, 0..1")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "vslice", new FlatvectorValue(.3,.3),1, _("V slice"),  _("Positions of vertical slice, 0..1")));
	
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", NULL,1, _("Out"), NULL,0, false));
}

NineSliceNode::~NineSliceNode()
{
}

NodeBase *NineSliceNode::Duplicate()
{
	NineSliceNode *newnode = new NineSliceNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int NineSliceNode::GetStatus()
{
	return NodeBase::GetStatus();
}

int NineSliceNode::Update() //bare bones
{
	ClearError();


	ImageValue *base = dynamic_cast<ImageValue*>(properties.e[0]->GetData());
	if (!base) {
		Error(_("In must be an image")).
		return -1;
	}

	// parse parameters
	int isnum;
	int width = getIntValue(properties.e[1]->GetData(), &isnum);;
	if (!isnum || width <= 0) {
		Error(_("Value must be a positive integer"));
		return -1;
	}

	int height = getIntValue(properties.e[2]->GetData(), &isnum);;
	if (!isnum || height <= 0) {
		Error(_("Value must be a positive integer"));
		return -1;
	}

	double scale = getNumberValue(properties.e[3]->GetData(), &isnum);;
	if (!isnum || scale <= 0) {
		Error(_("Value must be a positive number"));
		return -1;
	}

	flatvector hslice; ***
	flatvector vslice; ***

	int h1, h2, v1, v2;


	ImageValue *out = dynamic_cast<ImageValue*>(properties.e[properties.n-1]->GetData());
	if (out) {
		if (!out->image || out->image->w() <= 0 || out->image->h() <= 0) out = nullptr;
		else properties.e[properties.n-1]->Touch();
	}

	if (!out)
	{
		LaxImage *outimg = ImageLoader::NewImage(width, height);
		out = new ImageValue(outimg, true);
		properties.e[properties.n-1]->SetData(out, 1);
	}

	// render image
	*** 

	return NodeBase::Update();
}


//----------------------- PathCutNode ------------------------

/*! \class PathCutNode
 *
 * Do stuff.
 */
class PathCutNode : public NodeBase
{
  public:
	PathCutNode();
	virtual ~PathCutNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new PathCutNode(); }
};

PathCutNode::PathCutNode()
{
	makestr(type, "Paths/Cut");
	makestr(Name, _("Path Cut"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",     NULL,1,     _("In"), _("Path to cut")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "pathi", new IntValue(0),1,  _("Subpath"), _("Subpath indices of t values")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "t", new DoubleValue(.5),1,  _("t"), _("Position(s) to cut at")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "around", new DoubleValue(0),1,  _("Near"), _("Cut at t plus or minus this distance")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "remove", new BooleanValue(true),1, _("Remove"), _("Remove segments, rather than just slice")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "handles",new BooleanValue(true),1, _("Keep handles"), _("When Remove is true, do not also remove handles")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", NULL,1, _("Out"), NULL,0, false));
}

PathCutNode::~PathCutNode()
{
}

NodeBase *PathCutNode::Duplicate()
{
	PathCutNode *newnode = new PathCutNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int PathCutNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (!v || strcmp(v->whattype(), "PathsData")) return -1;

	return NodeBase::GetStatus();
}

int PathCutNode::Update() //bare bones
{
	ClearError();

	// in
	PathsData *in = dynamic_cast<PathsData*>(propertie.e[0]->GetData());
	if (!in) return -1;

	int isnum;

	// pathi
	Value *pi = properties.e[1]->GetData();
	int pathi = getIntValue(pi, &isnum);
	NumStack<int> pathis;
	if (isnum) {
		if (pathi < 0 || pathi >= in->paths.n) {
			Error(_("Bad subpath index"));
			return -1;
		}
	} else {
		SetValue *set = dynamic_cast<Setvalue*>(pi);
		if (!set) {
			Error(_("Subpath must be an int or set of numbers"));
			return -1;
		}
		for (int c=0; c<set->n(); c++) {
			pathi = getIntValue(set->e(c), &isnum);
			if (!isnum || pathi<0 || pathi >= in->paths.n) {
				Error(_("Bad subpath index"));
				return -1;
			}
			pathis.push(pathi);
		}
	}

	// t values
	double t = 0;
	Value *tv = properties.e[2]->GetData();
	if (!tv) return -1;
	t = getNumberValue(tv, &isnum);
	NumStack<double> tvals;
	if (isnum) {
		tvals.push(t);

	} else {
		SetValue *set = dynamic_cast<Setvalue*>(tv);
		if (!set) {
			Error(_("t must be a number or set of numbers"));
			return -1;
		}
		for (int c=0; c<set->n(); c++) {
			t = getNumberValue(set->e(c), &isnum);
			if (!isnum) {
				Error(_("t must be a number or set of numbers"));
				return -1;
			}
			tvals.push(t);
		}
	}
	if (!tvals.n) {
		Error(_("Missing t values"));
		return -1;
	}

	while (pathis.n < tvals.n) pathis.push(pathis.e[pathis.n-1]);

	// s values
	double s = 0;
	Value *sv = properties.e[3]->GetData();
	if (!sv) return -1;
	s = getNumberValue(sv, &isnum);
	NumStack<double> svals;
	if (isnum) {
		svals.push(s);

	} else {
		SetValue *set = dynamic_cast<Setvalue*>(sv);
		if (!set) {
			Error(_("Must be a number or set of numbers"));
			return -1;
		}
		for (int c=0; c<set->n(); c++) {
			s = getNumberValue(set->e(c), &isnum);
			if (!isnum) {
				Error(_("t must be a number or set of numbers"));
				return -1;
			}
			svals.push(s);
		}
	}

	// apply s span
	int i = 0;
	for (int c=0; c<svals.n; c++) {
		if (svals.e[c] == 0) { i++; continue; }
		if (c < pathis.n) pathi = pathis.e[c];
		if (i >= tvals.n) break;
		double s  = in->paths.e[pathi]->t_to_distance(tvals.e[i]);
		double t1 = in->paths.e[pathi]->distance_to_t(s-svals.e[c]);
		double t2 = in->paths.e[pathi]->distance_to_t(s+svals.e[c]);

		tvals.e[i] = t1;
		tvals.push(t2, i+1);

		pathis.push(pathis.e[i], i+1);

		i += 2;
	}

	// cut
	bool cut_segments = getBooleanValue(properties.e[4]->GetData(), &isnum);
	if (!isnum) return -1;

	// handles
	bool keep_handles = getBooleanValue(properties.e[5]->GetData(), &isnum);
	if (!isnum) return -1;


	//do stuff
	out = in->duplicate();
	properties.e[properties.n-1]->SetData(out, 1);

	if (cut_segments) {
		out->CutAt(tvals.n, pathis.e, tvals.e); // **** does NOT cut segments, will sever path at points sequentially

	} else {
		out->AddAt(tvals.n, pathis.e, tvals.e);
	}

	return NodeBase::Update();
}



//----------------------- PathBevelNode ------------------------

/*! \class PathBevelNode
 *
 * Bevel all corners of a path according to a bevel curve.
 */
class PathBevelNode : public ObjectFilter
{
  public:
	PathBevelNode();
	virtual ~PathBevelNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new PathBevelNode(); }
};

PathBevelNode::PathBevelNode()
{
	makestr(type, "Paths/Bevel");
	makestr(Name, _("Bevel Path Corners"));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "path",   NULL,1,     _("Path"),   _("Path to apply corners")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "corner", NULL,1,     _("Corner"), _("A path of a corner")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "dist",   new DoubleValue(.1),1, _("Width"), _("Corner size. Number, or list of numbers.")));
	
	//corner:
	//  path: first open path is considered the through path? should it have to be a through path?
	//  type:
	//    bevel  round  rev-round

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL,1, _("Out"), NULL,0, false));
}

PathBevelNode::~PathBevelNode()
{
}

NodeBase *PathBevelNode::Duplicate()
{
	PathBevelNode *newnode = new PathBevelNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int PathBevelNode::GetStatus()
{
	return NodeBase::GetStatus();
}

void PathBevelNode::FindCorners(PathsData *paths, double *inouts, int num_inouts, LPathsData *pathout)
{
	Coordinate *p, *pp, *p2, *end, *c1 = nullptr, *c2 = nullptr, *prev = nullptr;
	double prev_len = 0, len = 0;
	double in, out, out_prev, in_prev;
	double t_prev, t_next;
	flatpoint dirp, dirn;
	flatpoint pc1, pc2, pp2, nc1, nc2, np2;
	flatpoint in_axis, out_axis;

	int resolution = 50;

	if (!pathout) pathout = new LPathsData();
	pathout->Id(paths->Id());

	for (int c=0; c<paths->paths.n; c++) {
		p = paths->paths.e[c];
		if (!p) continue;

		p->resolveToControls(p1, c1, c2, p2, true);
		int i = 0;
		end = p2;

		if (!p2) continue;

		do {
			np2 = p2->fp;
			if (c1 == p1 && c2 == p2) {
				len = (p1->fp - p2->fp).norm();
				nc1 = p->fp + (np2 - p->fp)/3;
				nc2 = p->fp + (np2 - p->fp)*(2./3);
			} else {
				len = bez_segment_length(p1, c1, c2, p2, resolution);
				nc1 = c1->fp;
				nc2 = c2->fp;
			}

			if (prev) {
				// if corner:
				dirp = p->direction(false);
				dirn = p->direction(true);

				if (dirp.SmoothnessFlag(dirn) == LINE_Corner) {
					//   find in and out distances
					out = inouts[i % num_inouts];
					in = inouts[(i+1) % num_inouts];

					//   compute points + directions at the in/out points
					if (in > prev_len) in = prev_len;
					if (out > len - in) out = len - in;
					if (out > len) out = len;

					t_prev = bez_distance_to_t(in_prev, p->fp, nc1, nc2, p2->fp);
					t_next = bez_distance_to_t(out, p->fp, pc1, pc2, prev->fp);

					***

					//   compute axis for corner "square"
					***

					//   break path and insert corner transformed from def
					***
				}
			}

			prev = p;
			prev_len = len;
			prev_out = out;
			prev_in = in;
			pc1 = nc2;
			pc2 = nc1;
			pp1 = p->fp;
			p = p2;

			if (!p->resolveToControls(pp, c1, c2, p2, true)) break;

			i += 2;
		} while (p != end);
	}
}

int PathBevelNode::Update() //possible set ins
{
	//update with set parsing helpers
	
	ClearError();

	int num_ins = 3;
	ins[0] = properties.e[0]->GetData();
	ins[1] = properties.e[1]->GetData();
	ins[2] = properties.e[2]->GetData();

	SetValue *setins[3];
	SetValue *setouts[2];

	int num_outs = 2;
	int outprops[2];
	outprops[0] = 3;
	outprops[1] = 4;

	int max = 0;
	const char *err = nullptr;
	bool dosets = false;
	if (DetermineSetIns(num_ins, ins, setins, max, dosets) == -1) {; //does not check contents of sets.
		//had a null input
		return -1;
	}

	*** check for easy to spot errors with inputs

	//establish outprop: make it either type, or set. do prop->Touch(). clamp to max. makes setouts[*] null or the out set
	DoubleValue *out1 = UpdatePropType<DoubleValue>(properties.e[outprops[0]], dosets, max, setouts[0]);
	LPathsData  *out2 = UpdatePropType<LPathsData> (properties.e[outprops[1]], dosets, max, setouts[1]);

	DoubleValue *in1 = nullptr; //dynamic_cast<DoubleValue*>(ins[0]);
	IntValue    *in2 = nullptr; //dynamic_cast<IntValue*>(ins[1]);
	LPathsData  *in3 = nullptr; //dynamic_cast<LPathsData*>(ins[2]);

	for (int c=0; c<max; c++) {
		in1 = GetInValue<DoubleValue>(c, dosets, in1, ins[0], setins[0]);
		in2 = GetInValue<IntValue>   (c, dosets, in2, ins[1], setins[1]);
		in3 = GetInValue<LPathsData> (c, dosets, in3, ins[2], setins[2]);

		*** error check ins

		GetOutValue<DoubleValue>(c, dosets, out1, setouts[0]);
		GetOutValue<LPathsData> (c, dosets, out2, setouts[1]);
		

		*** based on ins, update outs
	}


	return NodeBase::Update();
}



//----------------------- FitInNode ------------------------

/*! \class FitInNode
 *
 * Scale and move an object to fit within bounds of parent space.
 */
class FitInNode : public NodeBase
{
  public:
	FitInNode();
	virtual ~FitInNode();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new FitInNode(); }
};

FitInNode::FitInNode()
{
	makestr(type, "Drawable/FitIn");
	makestr(Name, _("Fit In"));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in",     NULL,1,    _("Input"),  _("A DrawableObject")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "bounds", nullptr,1, _("Bounds"), nullptr));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "padding", new DoubleValue(0),1, _("Padding"), _("A number, or set of 4: [top, right, bottom, left]")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "space", new IntValue(1),1, _("Parent space"), _("Number of parents up to use as bounds space. -1 for top.")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "scaleup", new BooleanValue(false),1, _("If smaller that bounds, scale up"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "scaledown", new BooleanValue(true),1, _("If Larger than bounds, scale down"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "alignx", new DoubleValue(50),1, _("Align horizontally, 0..100"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "aligny", new DoubleValue(50),1, _("Align vertically, 0..100"), nullptr,0,true));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL,1, _("Out"), NULL,0, false));
}

FitInNode::~FitInNode()
{
}

int FitInNode::GetStatus()
{
	DrawableObject *dr = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!dr) return -1;
	BBoxValue *b = dynamic_cast<BBoxValue*>(properties.e[1]->GetData());
	if (!b) return -1;
	if (!isNumberType(properties.e[2]->GetData()), nullptr) {
		if (!dynamic_cast<SetValue*>(properties.e[2]->GetData())) return -1;
	}
	if (!isNumberType(properties.e[3]->GetData()), nullptr) return -1;
	if (!isNumberType(properties.e[4]->GetData()), nullptr) return -1;
	if (!isNumberType(properties.e[5]->GetData()), nullptr) return -1;
	if (!isNumberType(properties.e[6]->GetData()), nullptr) return -1;
	if (!isNumberType(properties.e[7]->GetData()), nullptr) return -1;
	
	return NodeBase::GetStatus();
}

int FitInNode::Update()
{
	DrawableObject *dr = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!dr) return -1;
	BBoxValue *bounds = dynamic_cast<BBoxValue*>(properties.e[1]->GetData());
	if (!bounds) return -1;
	double padding;
	SetValue *padset = nullptr;
	if (!isNumberType(properties.e[2]->GetData()), &padding) {
		padset = dynamic_cast<SetValue*>(properties.e[2]->GetData());
		if (!padset) return -1;
	}

	int parents = getNumberValue(properties.e[3]->GetData(), &isnum);
	if (!isnum) return -1;
	bool scaleup = getNumberValue(properties.e[4]->GetData(), &isnum);
	if (!isnum) return -1;
	bool scaledown = getNumberValue(properties.e[5]->GetData(), &isnum);
	if (!isnum) return -1;
	double alignx = getNumberValue(properties.e[6]->GetData(), &isnum);
	if (!isnum) return -1;
	double alignx = getNumberValue(properties.e[7]->GetData(), &isnum);
	if (!isnum) return -1;

	***

	return NodeBase::Update();
}


//----------------------- ObjectArrayNode ------------------------

/*! \class ObjectArrayNode
 *
 * Analagous to Blender's array modifier.
 */
class ObjectArrayNode : public NodeBase
{
  public:
	ObjectArrayNode();
	virtual ~ObjectArrayNode();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ObjectArrayNode(); }
};

ObjectArrayNode::ObjectArrayNode()
{
	makestr(type, "Drawable/Array");
	makestr(Name, _("Object Array"));

	//obj in, or set of objects (as from previous array) set uses bbox of whole set
	//shift:
	//  x,y as bbox multiple
	//  x,y as absolute distance
	//  x,y scale around clone origin
	//  angle around clone origin
	//count 
	//merge if path
	//start cap
	//end cap

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",     NULL,1,     _("Input"), _("A path or model")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "dir",   new FlatvectorValue(0,0,1),1,  _("Vector"),  _("Vector or path")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL,1, _("Out"), NULL,0, false));
}

ObjectArrayNode::~ObjectArrayNode()
{
}

int ObjectArrayNode::GetStatus()
{
	return NodeBase::GetStatus();
}

int ObjectArrayNode::Update()
{

}




//----------------------- SetToPointsetNode ------------------------

/*! \class SetToPointsetNode
 *
 * Turn a vector, affine, affine derived, or set thereof into a PointSet with Affine info.
 */
class SetToPointsetNode : public NodeBase
{
  public:
	SetToPointsetNode();
	virtual ~SetToPointsetNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new SetToPointsetNode(); }
};

SetToPointsetNode::SetToPointsetNode()
{
	makestr(type, "Points/ToPointSet");
	makestr(Name, _("To Pointset"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",     NULL,1,     _("In"), _("Something with position or transform information")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", new PointSetValue(),1, _("Out"), NULL,0, false));
}

SetToPointsetNode::~SetToPointsetNode()
{
}

NodeBase *SetToPointsetNode::Duplicate()
{
	SetToPointsetNode *newnode = new SetToPointsetNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int SetToPointsetNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (!v || v->type() != VALUE_Set) return -1;
	return NodeBase::GetStatus();
}

int SetToPointsetNode::Update()
{
	// in can be:
	//    vector2
	//    vector2[]
	//    Affine
	//    Affine[]

	SetValue *sv = dynamic_cast<SetValue*>(properties.e[0]->GetData());
	if (!sv) return -1;

	PointSetValue *out = dynamic_cast<PointSetValue*>(properties.e[properties.n-1]->GetData());

	for (int c=0; c<sv->n(); c++) {
		Value *v = s->e(c);
		if (!v) continue;

		switch (v->type()) {
			case VALUE_Flatvector: break;
			case VALUE_Spacevector: break;
			case AffineValue::TypeNumber(): break;
			case VALUE_Set: break;
		}
	}

	return NodeBase::Update();
}



//------------------------------ PrintfNode --------------------------------------------

class PrintfNode : public NodeBase
{
  public:
	PrintfNode(const char *fmt);
	virtual ~PrintfNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new PrintfNode(); }
};

PrintfNode::PrintfNode(const char *fmt)
{
	makestr(Name, _("Printf"));
	makestr(type, "Strings/Printf");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "format", new StringValue(fmt),1, ""));
	// will autopopulate with % fields	
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", new StringValue(),1, _("Out"), nullptr, 0, false));
}

PrintfNode::~PrintfNode()
{
}

NodeBase *PrintfNode::Duplicate()
{
	PrintfNode *newnode = new PrintfNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int PrintfNode::GetStatus()
{
	if (!dynamic_cast<StringValue*>(properties.e[0]->GetData())) return -1;
	return NodeBase::GetStatus(); //default checks mod times
}

int PrintfNode::SetProperties(const char *fmt)
{

}

int PrintfNode::Update()
{
	// NOT doing: %2$
	// NOT doing: length modifiers

	StringValue *fmtstr = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	if (!fmtstr) return -1;
	const char *fmt = fmtstr->str();

	if (modtime == 0 || modtime < properties.e[0]->modtime) SetProperties(fmt);

	Utf8String str, str2, str3;

	int cur_param = 0;
	int precision = 0;
	int fieldwidth = 0;
	int flags = 0;
	const char *ptr;
	const char *fstart = nullptr, *endptr;
	int i;
	char ch[2];
	ch[1] = '\0';

	#define ZEROPAD   (1<<0)
	#define LEFTJUST  (1<<1)
	#define BLANKSIGN (1<<2)
	#define PLUSMINUS (1<<3)

	//each input has: flags, field width, precision, type
	// flags: 
	//   0  zero padding
	//   -  make left justified within field
	//  ' ' insert a blank before positive number
	//   +  insert a + or - before numbers (never blank)
	//   #  NOT IMPLEMENTED -- force output with a decimal point, make first char a digit
	//   '  NOT IMPLEMENTED -- group by thounds
	//   I  NOT IMPLEMENTED -- locale alternate digits
	ptr = fmt;
	while (*ptr) {
		if (*ptr != '%') {
			ch[0] = *ptr;
			str.Append(ch);
			ptr++;
			continue;
		}

		if (ptr[1] == '%') {
			ch[0] = '%'
			str.Append(ch);
			ptr += 2;
			continue;
		}

		fstart = ptr;
		ptr++;
		precision = 1;
		fieldwidth = 1;
		flags = 0;

		while (strchr("0- +", *ptr)) {
			switch(*ptr) {
				case '0': flags |= ZEROPAD; break;
				case '-': flags |= LEFTJUST; break;
				case ' ': flags |= BLANKSIGN; break;
				case '+': flags |= PLUSMINUS; break;
			}
			ptr++;
		}

		if (isdigit(*ptr)) {
			// read in field width
			i = strtol(ptr, &endptr, 10);
			fieldwidth = i;
			ptr = endptr;
		}
		if (*ptr == '.') {
			// read in precision
			ptr++;
			if (isdigit(*ptr)) {
				i = strtol(ptr, &endptr, 10);
				precision = i;
				ptr = endptr;
			} else precision = 0;
		}

		if (!strchr("diouxXbeEfFgGcs", *ptr)) { //NOT doing aA (fractional hex), or p n m
			// add unformatted string, type was not recognized
			str.AppendN(fstart, ptr - fstart+1);
			ptr++;
		} else {
			//valid formatting, perform conversion
			char type = *ptr;
			ptr++;
			if (type == 'c') type = 's'; //we don't do chars per se
			if (type == 'b') {
				***; //our own special binary out

			} else if (type == 's') {
				//string/char
				str2.Sprintf("%%%s%s%ds", (flags&ZEROPAD) ? "0" : "", (flags&LEFTJUST) ? "-":"", fieldwidth);
				str3.Sprintf(str2.c_str(), sv->str);

			} else if (type == 'd' || type == 'i' || type == 'o' || type == 'u' || type == 'x' || type == 'X') {
				//integers
				str2.Sprintf("%%%s%s%d%c", (flags&ZEROPAD) ? "0" : "", (flags&LEFTJUST) ? "-":"", fieldwidth, type);
				str3.Sprintf(str2.c_str(), i);

			} else if (type == 'e' || type == 'E' || type == 'f' || type == 'F' || type == 'g' || type == 'G') {
				//floating point
				str2.Sprintf("%%%s%s%d", (flags&ZEROPAD) ? "0" : "", (flags&LEFTJUST) ? "-":"", fieldwidth);
				if (precision != 0) {
					str3.Sprintf(".%d", precision);
					str2.Append(str3);
				}
				ch[0] = type;
				str2.Append(ch);

				str3.Sprintf(str2.c_str(), d);
			}
		}
	}


	return NodeBase::Update();
}



//------------------------------ MenuValue ---------------------------------

/*! Value so that node properties can spawn custom selection windows, like popup menus or date selectors
 */
class MenuValue : public Value
{
  public:
  	Laxkit::MenuInfo *menu;
  	char *current;

  	MenuValue(MenuInfo *menu, bool absorb, const char *curvalue);
  	virtual ~MenuValue();

  	virtual void SetMenu(Laxkit::MenuInfo *nmenu, bool absorb);
  	virtual anXWindow *GetDialog();
};

MenuValue::MenuValue(MenuInfo *menu, bool absorb, const char *curvalue)
{
	this->menu = menu;
	if (!absorb && menu) menu->inc_count();
	current = nullptr;
	makestr(current, curvalue);
}

MenuValue::~MenuValue()
{
	if (menu) menu->dec_count();
	delete[] current;
}

void MenuValue::SetMenu(Laxkit::MenuInfo *nmenu, bool absorb)
{
	if (menu != nmenu) {
		if (menu) menu->dec_count();
		menu = nmenu;
	}
	if (!absorb && menu) menu->inc_count();
}

Laxkit::anXWindow *MenuValue::GetDialog(Laxkit::EventReceiver *proxy)
{
	if (!menu) return nullptr;

	PopupMenu *popup=new PopupMenu(NULL,menu->title, 0,
							0,0,0,0, 1,
							object_id,"prop_dialog_ret",
							0, //mouse to position near?
							menu,1, NULL,
							TREESEL_LEFT|TREESEL_SEND_PATH|TREESEL_LIVE_SEARCH);
	popup->WrapToMouse(0);
	return popup;
	// anXApp::app->rundialog(popup);
}


//------------------------------ FlipNode ---------------------------------

/*! Flip a point. If out is not a FlatVector, return a new one, else return out with updated contents. */
Value *MirrorPathNode::FlipVector2(Value *in, flatpoint p1, flatpoint p2, Value *out)
{

	FlatvectorValue *p = dynamic_cast<FlatvectorValue*>(in);

	FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(out);
	if (!fv) {
		fv = new FlatvectorValue();
	}

	flatvector v = p2 - p1;
	fv->v = p->v + 2*((p1 - p->v) |= v);
	// ----
	// Affine affine;
	// affine.Flip(p1,p2);
	// fv->v = affine.transformPoint(fv->v);

	return fv;
}



//------------------------ AlignToBoundsNode ------------------------

/*! \class Node to align one object to another based on bounds.
 */

class AlignToBoundsNode : public NodeBase
{
  public:
	AlignToBoundsNode();
	virtual ~AlignToBoundsNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref);

	// static SingletonKeeper keeper; //the def for domain enum
	// static ObjectDef *GetDef() { return dynamic_cast<ObjectDef*>(keeper.GetObject()); }
};

Laxkit::anObject *AlignToBoundsNode::NewNode(int p, Laxkit::anObject *ref)
{
	return new AlignToBoundsNode();
}

AlignToBoundsNode::AlignToBoundsNode()
{
	makestr(Name, _("Align To Bounds"));
	makestr(type, "Drawable/AlignToBounds");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",     NULL,1,  _("In"),     _("Object to align")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "bounds", NULL,1,  _("Bounds"), _("A bounding box to align in")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out",  nullptr,1, _("Out"), nullptr, 0, false));
}

AlignToBoundsNode::~AlignToBoundsNode()
{
}

NodeBase *AlignToBoundsNode::Duplicate()
{
	AlignToBoundsNode *node = new AlignToBoundsNode();
	node->DuplicateBase(this);
	return node;
}

/*! Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int AlignToBoundsNode::Update()
{
	DrawableObject *in = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!in) return -1;

	DoubleBBox *bbox = dynamic_cast<DoubleBBox*>(properties.e[1]->GetData());
	if (!bbox) return -1;

	DrawableObject *out = in->duplicate();
	out->fitto
	
	properties.e[4]->SetData(in, 0);
	return NodeBase::Update();
}

int AlignToBoundsNode::GetStatus()
{
	DrawableObject *dr = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!dr) return -1;
	StringValue *s = dynamic_cast<StringValue*>(properties.e[1]->GetData());
	if (!s) return -1;
	const char *pattern = s->str;
	if (isblank(pattern)) return -1;

	return NodeBase::GetStatus();
}



//------------------------ NodePanel ------------------------

/*! \class NodePanel
 * Class to hold a definition for a standalone panel allowing random access to
 * individual properties of other nodes, so users can adjust very complicated
 * node arrangements via this much simpler panel.
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

	NodeGroup *owner;
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




//------------------------ Path Effects ------------------------

// Input one or more paths. Do something to it. Output results


//------------------------ Model3D Hedron ------------------------

//Polyhedron class based:
//  name
//  vertices
//  edges
//  faces
//  uv(s)?
//  normals?
//
//Unwrap node:
//  click to pop up the unwrap window

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
	double pz, py;
	for (int zz = 0; zz < zdiv; zz++) {
		pz = zz / (double)zdiv * zwidth - zwidth/2;
		for (int yy = 0; yy < ydiv; yy++) {
			py = yy / (double)ydiv * ywidth - ywidth/2;
			for (int xx = 0; xx < xdiv; xx++) {
				poly->AddPoint(center + spacevector(xx / (double)xdiv * xwidth - xwidth/2, py, pz));
			}
		}
	}
}

/*! Cube centered at center.
 */
int Cube(double xwidth, double ywidth, double zwidth,
		 int xdiv, int ydiv, int zdiv,
		 spacevector center, Polyhedron *poly)
{
	double pz, py;
	for (int zz = 0; zz < zdiv; zz++) {
		pz = zz / (double)zdiv * zwidth - zwidth/2;
		for (int yy = 0; yy < ydiv; yy++) {
			py = yy / (double)ydiv * ywidth - ywidth/2;
			for (int xx = 0; xx < xdiv; xx++) {
				poly->AddPoint(center + spacevector(xx / (double)xdiv * xwidth - xwidth/2, py, pz));
			}
		}
	}
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


//------------------------------ ColorMapNode --------------------------------------------

/*! \class ColorMapNode
 * Map a number or a color through a gradient
 */
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
	return NodeBase::GetStatus();
}


//------------------------------ CompileMath --------------------------------------------


/*! Take math nodes and create an equivalent expression.
 */
ScriptNode *CompileMath(RefPtrStack<NodeBase> &nodes, Interpreter *interpreter, RefPtrStack<NodeBase> &remainder)
{
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
	return NodeBase::GetStatus();
}

Laxkit::anObject *newSwizzle(int p, Laxkit::anObject *ref)
{
	return new SwizzleNode();
}


//------------------------------ PathBooleanNode --------------------------------------------

/*! \class PathBooleanNode
 */

class PathBooleanNode : public NodeBase
{
  public:
	static SingletonKeeper defkeeper; //the def for the op enum
	static ObjectDef *GetDef();

	PathBooleanNode();
	virtual ~PathBooleanNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new PathBooleanNode(); }
};

static SingletonKeeper PathBooleanNode::defkeeper;

static ObjectDef *PathBooleanNode::GetDef()
{
	ObjectDef *def = dynamic_cast<ObjectDef*>(defkeeper.GetObject());
	if (def) return def;

	def = new ObjectDef("PathBooleanDef", _("Path Boolean Def"), NULL,NULL,"enum", 0);

	def->pushEnumValue("Union",        _("Union"),         _("Union"),                     Bezier::PathOp::Union       );
	def->pushEnumValue("Intersection", _("Intersection"),  _("Intersection"),              Bezier::PathOp::Intersection);
	def->pushEnumValue("AMinusB",      _("A - B"),         _("Remove second from first"),  Bezier::PathOp::AMinusB     );
	def->pushEnumValue("BMinusA",      _("B - A"),         _("Remove first from second"),  Bezier::PathOp::BMinusA     );
	def->pushEnumValue("Xor",          _("Xor"),           _("A or B but not both"),       Bezier::PathOp::Xor         );

	defkeeper.SetObject(def,1);
	return def;
}

PathBooleanNode::PathBooleanNode()
{
	makestr(type, "Filters/Path Boolean");
	makestr(Name, _("Path Boolean"));

	ObjectDef *enumdef = GetDef();

	EnumValue *e = new EnumValue(enumdef, 0);
	e->SetFromId(op);

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in",  NULL,1, _("In")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in2", NULL,1, _("In2")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Block, true, "op", e,1, _("Op"),NULL,0, false));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output,true, "out", NULL,0, NULL, 0, false)); 
}

PathBooleanNode::~PathBooleanNode()
{
}

NodeBase *PathBooleanNode::Duplicate()
{
	PathBooleanNode *node = new PathBooleanNode(op);
	node->DuplicateBase(this);
    node->DuplicateProperties(this);
	return node;
}

int PathBooleanNode::GetStatus()
{
	PathsData *path1 = dynamic_cast<PathsData*>(properties.e[0]->GetData());
	PathsData *path2 = dynamic_cast<PathsData*>(properties.e[1]->GetData());

	if (!path1 || !path2) return -1;

	return NodeBase::GetStatus();
}

int PathBooleanNode::Update()
{
	PathsData *path1 = dynamic_cast<PathsData*>(properties.e[0]->GetData());
	PathsData *path2 = dynamic_cast<PathsData*>(properties.e[1]->GetData());

	if (!path1 || !path2) return -1;

	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[2]->GetData());
	if (!ev) return -1;
	int op = ev->EnumId();

	Affine m2to1;
	SomeData *pnt = path1->FindCommonParent(path2);
	if (pnt) {
		m2to1 = path1->GetTransforms(path2, false);
	} else {
		m2to1.setIdentity();
	}
	PathsData *out = PathBoolean(path1, path2, op, m2to1.m());
	if (!out) {
		Error(_("Could not compute path op"));
		return -1;
	}

	out->m(path1->m());
	properties.e[properties.n-1]->SetData(out, 1);
	return NodeBase::Update();
}



//----------------------- CatenaryNode ------------------------

/*! \class CatenaryNode
 *
 * Create a catenary between two points.
 */
class CatenaryNode : public NodeBase
{
  public:
	CatenaryNode();
	virtual ~CatenaryNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new CatenaryNode(); }
};

CatenaryNode::CatenaryNode()
{
	makestr(type, "Paths/Catenary");
	makestr(Name, _("Catenary"));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "p2",     new FlatvectorValue(0,0),1,     _("P2"), _("P2")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "p1",     new FlatvectorValue(1,0),1,     _("P1"), _("P1")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "length", new DoubleValue(1.5),1,  _("Length"),  _("Length of the arc. If too short, just make a straight line.")));
	
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", NULL,1, _("Out"), NULL,0, false));
}

CatenaryNode::~CatenaryNode()
{
}

NodeBase *CatenaryNode::Duplicate()
{
	CatenaryNode *newnode = new CatenaryNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int CatenaryNode::GetStatus()
{
	return NodeBase::GetStatus();
}

/*! Compute a, p, and q for a catenary that hangs between p1 and p2, and that has the given length.
 * If length is less than the distance between p1 and p2, then the distance between p1 and p2 is used for the length.
 *
 * The function returned is as follows, and passes through p1 and p2:
 *     y = a * cosh((x-p)/2*a) + q
 */
int ComputeCatenary(flatpoint p1, flatpoint p2, double length, double *a_ret, double *p_ret, double *q_ret)
{
	***
}

int CatenaryNode::Update() //possible set ins
{
	//update with set parsing helpers
	
	ClearError();

	int num_ins = 3;
	Value *ins[3];
	ins[0] = properties.e[0]->GetData();
	ins[1] = properties.e[1]->GetData();
	ins[2] = properties.e[2]->GetData();

	SetValue *setins[3];
	SetValue *setouts[1];

	int num_outs = 1;
	int outprops[1];
	outprops[0] = 3;

	int max = 0;
	const char *err = nullptr;
	bool dosets = false;
	if (DetermineSetIns(num_ins, ins, setins, max, dosets) == -1) { //does not check contents of sets.
		//had a null input
		return -1;
	}

	//*** check for easy to spot errors with inputs

	//establish outprop: make it either type, or set. do prop->Touch(). clamp to max. makes setouts[*] null or the out set
	LPathsData  *out1 = UpdatePropType<LPathsData> (properties.e[outprops[0]], dosets, max, setouts[0]);

	FlatvectorValue *p1 = nullptr;
	FlatvectorValue *p2 = nullptr;
	DoubleValue *len    = nullptr;
	
	for (int c=0; c<max; c++) {
		p1 = GetInValue<DoubleValue>(c, dosets, in1, ins[0], setins[0]);
		p2 = GetInValue<IntValue>   (c, dosets, in2, ins[1], setins[1]);
		len = GetInValue<LPathsData> (c, dosets, in3, ins[2], setins[2]);

		*** error check ins

		GetOutValue<LPathsData>(c, dosets, out1, setouts[0]);
			

		*** based on ins, update outs
	}


	return NodeBase::Update();
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
 * str|TextObject: Expression
 *     New Out [o]
 */



//----------------------- BoilerPlateNode ------------------------

/*! \class BoilerPlateNode
 *
 * Do stuff.
 */
class BoilerPlateNode : public NodeBase
{
  public:
	BoilerPlateNode();
	virtual ~BoilerPlateNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new BoilerPlateNode(); }
};

BoilerPlateNode::BoilerPlateNode()
{
	makestr(type, "Paths/Extrude");
	makestr(Name, _("Extrude"));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",     NULL,1,     _("Input"), _("A path or model")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "dir", new FlatvectorValue(0,0,1),1,  _("Vector"),  _("Vector or path")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "b",   new BooleanValue(false),1,  _("Check this"),  _("Some boolean thing")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", NULL,1, _("Out"), NULL,0, false));
}

BoilerPlateNode::~BoilerPlateNode()
{
}

NodeBase *BoilerPlateNode::Duplicate()
{
	BoilerPlateNode *newnode = new BoilerPlateNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int BoilerPlateNode::GetStatus()
{
	return NodeBase::GetStatus();
}

int BoilerPlateNode::Update() //bare bones
{
	ClearError();

	int isnum;

	// cut
	bool cut_segments = getBooleanValue(properties.e[3]->GetData(), &isnum);
	if (!isnum) return -1;

	return NodeBase::Update();
}

int BoilerPlateNode::Update() //possible set ins
{
	//update with set parsing helpers
	
	ClearError();

	int num_ins = 3;
	Value *ins[num_ins];
	ins[0] = properties.e[0]->GetData();
	ins[1] = properties.e[1]->GetData();
	ins[2] = properties.e[2]->GetData();

	SetValue *setins[3];
	SetValue *setouts[2];

	int num_outs = 2;
	int outprops[2];
	outprops[0] = 3;
	outprops[1] = 4;

	int max = 0;
	const char *err = nullptr;
	bool dosets = false;
	if (DetermineSetIns(num_ins, ins, setins, max, dosets) == -1) {; //does not check contents of sets.
		//had a null input
		return -1;
	}

	*** check for easy to spot errors with inputs

	//establish outprop: make it either type, or set. do prop->Touch(). clamp to max. makes setouts[*] null or the out set
	DoubleValue *out1 = UpdatePropType<DoubleValue>(properties.e[outprops[0]], dosets, max, setouts[0]);
	LPathsData  *out2 = UpdatePropType<LPathsData> (properties.e[outprops[1]], dosets, max, setouts[1]);

	DoubleValue *in1 = nullptr; //dynamic_cast<DoubleValue*>(ins[0]);
	IntValue    *in2 = nullptr; //dynamic_cast<IntValue*>(ins[1]);
	LPathsData  *in3 = nullptr; //dynamic_cast<LPathsData*>(ins[2]);

	for (int c=0; c<max; c++) {
		in1 = GetInValue<DoubleValue>(c, dosets, in1, ins[0], setins[0]);
		in2 = GetInValue<IntValue>   (c, dosets, in2, ins[1], setins[1]);
		in3 = GetInValue<LPathsData> (c, dosets, in3, ins[2], setins[2]);

		*** error check ins

		GetOutValue<DoubleValue>(c, dosets, out1, setouts[0]);
		GetOutValue<LPathsData> (c, dosets, out2, setouts[1]);
			

		*** based on ins, update outs
	}


	return NodeBase::Update();
}