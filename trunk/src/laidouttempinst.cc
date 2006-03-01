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
PtrStack<Disposition>;

 // used by ???
PtrStack<PaperType>;


 // used by many
PtrStack<char>;
 //*** Attribute stack ought to be 
 //		class Att { char *name; char *value; }; 
 //		typedef PtrStack<Att> AttributeStack;
typedef PtrStack<char> AttributeStack;//******

