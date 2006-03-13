//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/************* Template Instances: laidout/laidouttempinst.cc ************/

// This file creates instances of the source of various template based classes.
// *** note that the compiler won't let me instantiate like this!! is there
// a flag to turn that off?
//
// this file is currently not used
  
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

