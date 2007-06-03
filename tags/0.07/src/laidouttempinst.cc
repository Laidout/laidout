//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/************* Template Instances: laidout/laidouttempinst.cc ************/

// This file creates instances of the source of various template based classes.
// *** note that the compiler won't let me instantiate like this!! is there
// a flag to turn that off?
//

************* this file is currently not used
  
#include <lax/lists.h>

 // used by FieldMask
PtrStack<int>;

 // used by Spread
PtrStack<PageLocation>;

 // used by Style
PtrStack<StyleDef>;
PtrStack<FieldNode>;

 // used by LaidoutApp
PtrStack<Imposition>;

 // used by ???
PtrStack<PaperType>;


 // used by many
PtrStack<char>;
 //*** Attribute stack ought to be 
 //		class Att { char *name; char *value; }; 
 //		typedef PtrStack<Att> AttributeStack;
typedef PtrStack<char> AttributeStack;//******

