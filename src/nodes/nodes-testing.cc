
//*********** WORKS IN PROGRESS ****************

TODO:
  FitIn
  CSV in: set of row hashes, or hash of row values
  AlignToBounds
  ListOf  [type][num] 0 1 2 ... n-1 out
  PathIntersections
  ObjectArrayNode
  MirrorPathNode
  MergePaths
  Printf
  MenuValue
  CutPathFromTo

  EulerToQuaternion
  QuaternionToEuler
  Swizzle





//----------------------- PathIntersectionsNode ------------------------

/*! \class PathIntersectionsNode
 *
 * 
 */
class PathIntersectionsNode : public NodeBase
{
  public:
	PathIntersectionsNode();
	virtual ~PathIntersectionsNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new PathIntersectionsNode(); }
};

PathIntersectionsNode::PathIntersectionsNode()
{
	makestr(type, "Paths/Intersections");
	makestr(Name, _("Path Intersections"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",     NULL,1,     _("Paths"), nullptr));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "self", new BooleanValue(true),1, _("Check self"), _("Whether to check for a path intersecting itself"),0,true));
	
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "points", NULL,1, _("Points"), NULL,0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "dir1", NULL,1, _("Direction 1"), NULL,0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "dir2", NULL,1, _("Direction 2"), NULL,0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "info", NULL,1, _("info"), NULL,0, false));
}

PathIntersectionsNode::~PathIntersectionsNode()
{
}

NodeBase *PathIntersectionsNode::Duplicate()
{
	PathIntersectionsNode *newnode = new PathIntersectionsNode();
	dynamic_cast<BooleanValue*>(newnode->properties.e[1]->GetData())->i
		= dynamic_cast<BooleanValue*>(properties.e[1]->GetData())->i;
	newnode->DuplicateBase(this);
	return newnode;
}

int PathIntersectionsNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (!v || (!dynamic_cast<LPathsData*>(v) && v->type() != VALUE_Set)) return -1;
	return NodeBase::GetStatus();
}

int PathIntersectionsNode::Update()
{
	Value *v = properties.e[0]->GetData();
	LPathsData *path1 = dynamic_cast<LPathsData*>(v);
	if (!path1 && v->type() != VALUE_Set) return -1;

	SetValue *setin = path1 ? nullptr : dynamic_cast<SetValue*>(v);

	int isnum;
	bool check_self = getIntValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;

	NumStack<flatpoint> points;
	NumStack<int> obj1, obj2, pathi1, pathi2;
	NumStack<double> t1, t2;
	
	int n = 0; //grand total
	LPathsData *path2;
	flatpoint pts1[4], pts2[4];
	flatpoint found[9];
	double foundt1[9], foundt2[9];
	Coordinate *start1, *start2, *p1, *p2, *p1next, *p2next;
	int num; //num per segment (up to 9 each check)
	int isline;

	for (int c=0; c< setin ? setin->n() : 1; c++) {
		if (setin) {
			path1 = dynamic_cast<LPathsData*>(setin->e(c));
			if (!pathin) {
				Error(_("In must be a path or set of paths"));
				return -1;
			}
		}

		path2 = path1;
		for (int c2 = c + (check_self ? 0 : 1); c2 < setin ? setin->n() : 1; c2++) {
			if (setin) {
				path2 = dynamic_cast<LPathsData*>(setin->e(c2));
				if (!path2) {
					Error(_("In must be a path or set of paths"));
					return -1;
				}
			}

			// check each bez segment in path1 vs each in path2
			if (path1 == path2) {
				// *** TODO !!!!! IMPLEMENT ME!!!!!!
				// bez_intersect_self(...);
				continue;
			}
			
			for (int c3 = 0; c3 < path1->paths.n; c3++) {
				if (!path1->paths.e[c3]->path) continue;
				start1 = p1 = path1->paths.e[c3]->path;

				pts1[0] = p1->p();
				if (!p1->getNext(pts1[1], pts1[2], p1next) != 0) break;
				pts1[3] = p1next->p();

				do {
					for (int c4 = 0; c4 < path2->paths.n; c4++) {
						if (!path2->paths.e[c4]->path) continue;
						start2 = p2 = path2->paths.e[c4]->path;

						do {
							pts2[0] = p2->p();
							if (!p2->getNext(pts2[1], pts2[2], p2next) != 0) break;
							pts2[3] = p2next->p();

							bez_intersect_bez(
									pts1[0], pts1[1], pts1[2], pts1[3],
									pts2[0], pts2[1], pts2[2], pts2[3],
									found, foundt1, foundt2, num,
									threshhold,
									0,0,1,
									1, maxdepth
								);

							for (int c5 = 0; c5 < num; c5++) {
								points.push(found[c5]);
								obj1.push(c);
								obj2.push(c2);
								pathi1.push(c3);
								pathi2.push(c4);
								t1.push(foundt1[c5]);
								t2.push(foundt2[c5]);
							}

							p2 = p2next;
						} while (p2 && p2 != start2);
					}

					p1 = p1next;
				} while (p1 && p1 != start1);
			}
		}
	}

	//only update things that are connected to other things
	if (properties.e[2]->IsConnected()) { //points
		SetValue *outpoints = dynamic_cast<SetValue*>(properties.e[2]->GetData());
		if (!outpoints) {
			outpoints = new SetValue();
			properties.e[2]->SetData(outpoints, 1);
		} else properties.e[2]->Touch();

		for (int c=0; c<points.n; c++) {
			if (c >= outpoints->n()) {
				outpoints->push(new FlatvectorValue(points.e[c]), 1);
			} else {
				FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(outpoints->e(c))
				fv->v = points.e[c];
			}
		}

		while (outpoints->n() != points.n) outpoints->Remove(outpoints->n()-1);
	}
	if (properties.e[3]->IsConnected()) { //dir1
		SetValue *out = dynamic_cast<SetValue*>(properties.e[3]->GetData());
		if (!out) {
			out = new SetValue();
			properties.e[3]->SetData(out, 1);
		} else properties.e[3]->Touch();

		flatpoint v;
		for (int c=0; c<points.n; c++) {
			int i = obj1[c];
			LPathsData *paths = setin ? dynamic_cast<LPathsData*>(setin->e(i)) : path1;
			paths->PointAlongPath(pathi1[c], foundt1[c], false, nullptr, &v);
			v.normalize();

			if (c >= out->n()) {
				out->push(new FlatvectorValue(v), 1);
			} else {
				FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(out->e(c))
				fv->v = v;
			}
		}

		while (out->n() != points.n) out->Remove(out->n()-1);
	}
	if (properties.e[4]->IsConnected()) { //dir2
		SetValue *out = dynamic_cast<SetValue*>(properties.e[4]->GetData());
		if (!out) {
			out = new SetValue();
			properties.e[4]->SetData(out, 1);
		} else properties.e[4]->Touch();

		flatpoint v;
		for (int c=0; c<points.n; c++) {
			int i = obj2[c];
			LPathsData *paths = setin ? dynamic_cast<LPathsData*>(setin->e(i)) : path1;
			paths->PointAlongPath(pathi2[c], foundt2[c], false, nullptr, &v);
			v.normalize();

			if (c >= out->n()) {
				out->push(new FlatvectorValue(v), 1);
			} else {
				FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(out->e(c))
				fv->v = v;
			}
		}

		while (out->n() != points.n) out->Remove(out->n()-1);
	}
	if (properties.e[5]->IsConnected()) { //info []
		// info [
		//   int    which path object1
		//   int    which path index1
		//   float  path1 t
		//   int    which path object2
		//   int    which path index2
		//   float  path2 t
		//  ]
		SetValue *out = dynamic_cast<SetValue*>(properties.e[5]->GetData());
		if (!out) {
			out = new SetValue();
			properties.e[5]->SetData(out, 1);
		} else properties.e[5]->Touch();

		flatpoint v;
		for (int c=0; c<points.n; c++) {
			if (c >= out->n()) {
				SetValue *set = new SetValue();
				set->push(new IntValue(obj1[c]), 1);
				set->push(new IntValue(pathi1[c]), 1);
				set->push(new DoubleValue(foundt1[c]), 1);
				set->push(new IntValue(obj2[c]), 1);
				set->push(new IntValue(pathi2[c]), 1);
				set->push(new DoubleValue(foundt2[c]), 1);
				out->Push(set, 1);
				
			} else {
				SetValue *set = dynamic_cast<SetValue*>(out->e(c))
				dynamic_cast<IntValue*>(set->e(0))->i    = obj1[c];
				dynamic_cast<IntValue*>(set->e(1))->i    = pathi1[c];
				dynamic_cast<DoubleValue*>(set->e(2))->i = foundt1[c];
				dynamic_cast<IntValue*>(set->e(3))->i    = obj2[c];
				dynamic_cast<IntValue*>(set->e(4))->i    = pathi2[c];
				dynamic_cast<DoubleValue*>(set->e(5))->i = foundt2[c];
			}
		}

		while (out->n() != points.n) out->Remove(out->n()-1);
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




//----------------------- GroupProxyNode ------------------------

 *** not sure if this class is worth it
/*! \class GroupProxyNode
 * Specific node to coordinate passing parameters across group nesting boundaries
 */
class GroupProxyNode : public NodeBase
{
  public:
	GroupProxyNode();
	virtual ~GroupProxyNode();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new GroupProxyNode(); }
};

GroupProxyNode::GroupProxyNode(bool ins)
{
	makestr(type, "GroupProxy");
	makestr(Name, ins ? _("Inputs") : _("Outputs"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",     NULL,1,     _("Input"), _("A path or model")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "dir",   new FlatvectorValue(0,0,1),1,  _("Vector"),  _("Vector or path")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL,1, _("Out"), NULL,0, false));
}

GroupProxyNode::~GroupProxyNode()
{
}

int GroupProxyNode::GetStatus()
{
	return NodeBase::GetStatus();
}

int GroupProxyNode::Update()
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
	return NodeBase::GetStatus();
}

int SetToPointsetNode::Update()
{
	// in can be:
	//    vector2
	//    vector2[]
	//    Affine
	//    Affine[]

	Value *v = properties.e[0]->GetData();
	PointSetValue *out = dynamic_cast<PointSetValue*>(properties.e[properties.n-1]->GetData());

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


//------------------------------ MirrorNode ---------------------------------

class MirrorPathNode : public ObjectFilterNode
{
	static SingletonKeeper keeper; //the def for the op enum

  public:
	static LaxInterfaces::PerspectiveInterface *GetPerspectiveInterface();

	bool render_preview;
	double render_dpi;
	LaxInterfaces::PerspectiveTransform *transform;
	Laxkit::Affine render_transform;

	PerspectiveNode();
	virtual ~PerspectiveNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int UpdateTransform();
	virtual int GetStatus();

	virtual LaxInterfaces::anInterface *ObjectFilterInterface();
	virtual DrawableObject *ObjectFilterData();
	// virtual int Mute(bool yes=true);
	//virtual int Connected(NodeConnection *connection);

};

SingletonKeeper MirrorPathNode::keeper;

/*! Static return for a PerspectiveInterface.
 */
LaxInterfaces::MirrorInterface *MirrorPathNode::GetPerspectiveInterface()
{
	PerspectiveInterface *interf = dynamic_cast<PerspectiveInterface*>(keeper.GetObject());
	if (!interf) {
		interf = new LPerspectiveInterface(NULL, getUniqueNumber(), NULL);
		interf->Id("keeperpersp");
		interf->interface_flags |= LaxInterfaces::PerspectiveInterface::PERSP_Dont_Change_Object;
		keeper.SetObject(interf, 1);
	}
	return interf;
}


MirrorPathNode::MirrorPathNode()
{
	makestr(Name, _("Mirror"));
	makestr(type, "Filters/Mirror");


	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",  NULL,1, _("In"),  NULL,0, false));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "p1",  new FlatvectorValue(0,0),1, _("p1")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "p2",  new FlatvectorValue(1,0),1, _("p2")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", NULL,1, _("Out"), NULL,0, false));
}

MirrorPathNode::~MirrorPathNode()
{
}

LaxInterfaces::anInterface *MirrorPathNode::ObjectFilterInterface()
{
	PerspectiveInterface *interf = GetPerspectiveInterface();
	return interf;
}

DrawableObject *MirrorPathNode::ObjectFilterData()
{
	NodeProperty *prop = FindProperty("out");
	return dynamic_cast<DrawableObject*>(prop->GetData());
}

NodeBase *MirrorPathNode::Duplicate()
{
	MirrorPathNode *node = new MirrorPathNode();
	node->DuplicateBase(this);
	node->DuplicateProperties(this);
	return node;
}

int MirrorPathNode::GetStatus()
{
	if (!dynamic_cast<PathsData*>(properties.e[0]->GetData())) return -1;
	return NodeBase::GetStatus();
}

int MirrorPathNode::UpdateMirror(flatpoint p1, flatpoint p2)
{
	NodeProperty *inprop = FindProperty("in");

	if (!properties.e[1]->IsConnected()) {
		FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[1]->GetData());
		if (!fv) {
			fv = new FlatvectorValue(p1);
			properties.e[1]->SetData(fv,1);
		} else fv->v = p1;
	}

	if (!properties.e[2]->IsConnected()) {
		FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[2]->GetData());
		if (!fv) {
			fv = new FlatvectorValue(p2);
			properties.e[2]->SetData(fv,1);
		} else fv->v = p2;
	}

	return 0;
}

int MirrorPathNode::Update()
{
	Error(nullptr);

	PathsData *orig = dynamic_cast<PathsData*>(properties.e[0]->GetData());
	if (!orig) { Error(_("Expected path")); return -1; }

	NodeProperty *outprop = FindProperty("out");
	PathsData *out = dynamic_cast<PathsData*>(outprop->GetData());

	if (IsMuted()) {
		outprop->SetData(orig, 0);
		return 0;
	} else if (out == orig) {
		//need to unset the mute
		outprop->SetData(nullptr, 0);
		out = nullptr;
	}

	FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[1]->GetData());
	if (!fv) { Error(_("Expected Vector2")); return -1; }

	FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[2]->GetData());
	if (!fv) { Error(_("Expected Vector2")); return -1; }

	if (out) out->set(*orig); //sets affine transform only


	if (dynamic_cast<PathsData*>(orig)) {
		 //go through point by point and transform.
		 //Try to preserve existing points to reduce allocations
		
		PathsData *pathin = dynamic_cast<PathsData*>(orig);

		LPathsData *pathout = dynamic_cast<LPathsData*>(out);
		if (!pathout) {
			pathout = dynamic_cast<LPathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
			pathout->Id("PerspFiltered");
			pathout->set(*pathin); //sets the affine transform
			pathout->InstallLineStyle(pathin->linestyle);
			pathout->InstallFillStyle(pathin->fillstyle);
			outprop->SetData(pathout,1);
			out = pathout;
		}

		DBG cerr << "pathout "<<pathout->Id()<<" previous bbox: "<<pathout->minx<<','<<pathout->maxx<<" "<<pathout->miny<<','<<pathout->maxy<<endl;

		Coordinate *start, *start2, *p, *p2;
		Path *path, *path2;
		flatpoint fp;

		 //make sure in and out have same number of paths
		while (pathout->NumPaths() > pathin->NumPaths()) pathout->RemovePath(pathout->NumPaths()-1, NULL);
		while (pathout->NumPaths() < pathin->NumPaths()) pathout->pushEmpty();

		for (int c=0; c<pathin->NumPaths(); c++) {
			path  = pathin ->paths.e[c];
			path2 = pathout->paths.e[c];
			path2->needtorecache = true;
		
			p  = path ->path;
			p2 = path2->path;
			start  = p;
			start2 = p2;

			if (p) {
				do {
					if (!p2) {
						p2 = p->duplicate();
						path2->append(p2);
						if (!start2) start2 = p2;
					} else {
						*p2 = *p;
					}

					fp = transform->transform(p->fp);
					p2->fp = fp;

					p  = p->next;
					p2 = p2->next;
					if (p2 == start2 && p != start) p2 = nullptr;
				} while (p && p != start);

				if (p == start && !p2) {
					path2->close();
				}

				if (p == start && p2 && p2 != start2) {
					 //too many points in path2! remove extra...
					 while (p2 && p2 != start2) {
					 	Coordinate *prev = p2->prev;
					 	path2->removePoint(p2, true);
					 	if (prev) p2 = prev->next;
					 	else p2 = nullptr;
					 }
				}

			} else {
				 //current path is an empty
				if (p2) path2->clear();
			}

			while (path2->pathweights.n > path->pathweights.n) path2->pathweights.remove(path2->pathweights.n-1);
			if (path2->pathweights.n < path->pathweights.n) {
				for (int c2 = 0; c2 < path->pathweights.n; c2++) {
					PathWeightNode *w1 = path->pathweights.e[c2];
					if (c2 >= path2->pathweights.n)
						path2->AddWeightNode(w1->t, w1->offset, w1->width, w1->angle);
					else *path2->pathweights.e[c2] = *w1;
				}
			}
			flatpoint tangent, point, wp1, wp2;
			for (int c2=0; c2<path->pathweights.n; c2++) {
				// virtual int PointAlongPath(double t, int tisdistance, flatpoint *point, flatpoint *tangent);
				PathWeightNode *w1 = path->pathweights.e[c2];
				PathWeightNode *w2 = path2->pathweights.e[c2];
				*w2 = *w1;
				path->PointAlongPath(w1->t, false, &point, &tangent);
				tangent.normalize();
				wp1 = transform->transform(point);
				wp2 = transform->transform(point + transpose(tangent) * .05);
				double scale = (wp1-wp2).norm() / .05;
				w2->offset *= scale;
				w2->width *= scale;
			}
		}


		pathout->FindBBox();
		DBG cerr << "pathout new bbox: "<<pathout->minx<<','<<pathout->maxx<<" "<<pathout->miny<<','<<pathout->maxy<<endl;
		outprop->Touch();
		pathout->touchContents();

		DBG cerr << "filter out:"<<endl;
		DBG pathout->dump_out(stderr, 10, 0, NULL);
		DBG cerr << "perspective transform:"<<endl;
		DBG transform->dump_out(stderr, 10, 0, NULL);

		return NodeBase::Update();

	// } else if (dynamic_cast<EngraverFillData*>(orig)) {
	// } else if (dynamic_cast<ColorPatchData*>(orig)) {
	// } else if (dynamic_cast<ImagePatchData*>(orig)) {
	// } else if (!strcmp(orig->whattype(), "PatchData")) {
	// } else if (dynamic_cast<VoronoiData*>(orig)) {
	// } else { // transform rasterized

	}

	Error(_("Cannot apply mirror to that type!"));
	return -1;


//	if (render_preview) {
//		*** //transform rasterized at much less than print resolution
//	} else {
//		//use render_scale
//		***
//	}

}



//------------------------ AlignToBoundsNode ------------------------

/*! \class Node to get a reference to other DrawableObjects in same page.
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
	makestr(Name, _("Find Drawable"));
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



//----------------------------- SimpleArrayValue ----------------------------------
template <class T>
class SimpleArrayValue : public Value
{
  public:
	NumStack<T> stack;

    SimpleArrayValue();
    virtual ~SimpleArrayValue();
    virtual const char *whattype() { return "SimpleArrayValue"; }
    virtual int getValueStr(char *buffer,int len);
    virtual Value *duplicate();
    virtual int type() { return VALUE_SimpleArray; }
    virtual ObjectDef *makeObjectDef();
};


typedef SimpleArrayValue<int> IntStackValue;
typedef SimpleArrayValue<double> DoubleStackValue;
typedef SimpleArrayValue<flatvector> Vector2StackValue;
typedef SimpleArrayValue<spacevector> Vector3StackValue;
typedef SimpleArrayValue<Quaternion> QuaternionStackValue;

typedef SimpleArrayValue<anObject> ObjectStackValue;


SimpleArrayValue<Int>::whattype() { return "IntStackValue"; }
SimpleArrayValue<Double>::whattype() { return "DoubleStackValue"; }
SimpleArrayValue<Vector2>::whattype() { return "Vector2StackValue"; }
SimpleArrayValue<Vector3>::whattype() { return "Vector3StackValue"; }
SimpleArrayValue<Quaternion>::whattype() { return "QuaternionStackValue"; }
SimpleArrayValue<String>::whattype() { return "StringStackValue"; }



class Vector2StackValue : public Value
{
  public:
	NumStack<flatvector> stack;

    Vector2StackValue();
    virtual ~SimpleArrayValue();
    virtual const char *whattype() { return "Vector2StackValue"; }
    virtual int getValueStr(char *buffer,int len);
    virtual Value *duplicate();
    virtual int type() { return VALUE_Vector2Stack; }
    virtual ObjectDef *makeObjectDef();
};





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
	return NodeBase::GetStatus();
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
 * Map arrays to other arrays using a special Boolean interface.
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
};

static SingletonKeeper PathBooleanNode::defkeeper;

static ObjectDef *PathBooleanNode::GetDef()
{
	ObjectDef *def = dynamic_cast<ObjectDef*>(defkeeper.GetObject());
	if (def) return def;

	def = new ObjectDef("PathBooleanDef", _("Path Boolean Def"), NULL,NULL,"enum", 0);

	def->pushEnumValue("Union",        _("Union"),         _("Union"),                     BOOL_Union         );
	def->pushEnumValue("Intersection", _("Intersection"),  _("Intersection"),              BOOL_Intersection  );
	def->pushEnumValue("OneMinusTwo",  _("A - B"),         _("Remove second from first"),  BOOL_AMinusB  );
	def->pushEnumValue("Intersection", _("B - A"),         _("Remove first from second"),  BOOL_BMinusA  );
	def->pushEnumValue("Xor",          _("Xor"),           _("A or B but not both"),       BOOL_Xor  );
	

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
{ ***
	PathBooleanNode *node = new PathBooleanNode(op);
	node->DuplicateBase(this);
    node->DuplicateProperties(this);
	return node;
}

int PathBooleanNode::Update()
{ ***
}

int PathBooleanNode::GetStatus()
{ ***
	return NodeBase::GetStatus();
}

Laxkit::anObject *newPathBoolean(int p, Laxkit::anObject *ref)
{
	return new PathBooleanNode();
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
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "dir",   new FlatvectorValue(0,0,1),1,  _("Vector"),  _("Vector or path")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL,1, _("Out"), NULL,0, false));
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

int BoilerPlateNode::Update()
{
	ClearError();
	return NodeBase::Update();
}

int BoilerPlateNode::Update()
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