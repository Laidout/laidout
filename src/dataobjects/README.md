Object types and their interfaces
=================================

This directory contains definitions for various drawable objects in Laidout,
tools that operate specifically on these objects, and directly related infrastructure.

If you make an interface that does not work on a specific object type, then those
interfaces should instead go in ../interfaces.

New objects must:

 - derive from DrawableObject (see drawableobject.cc/h).

 - Implement required functions from Value, and install its ObjectDef to stylemanager.
   See limagedata.cc for an example. To allow scripting access, you must implement
   assign(), dereference(), and Evaluate() for whatever you include in the ObjectDef.

 - Add a creator function in datafactory.cc, which makes general Laxkit functions
   able to use the new objects.
   
 - Add the object file name to the `objs` list in Makefile, and the `otherobjs` list in ../Makefile

 - If your new object also has an interface for it, you also must
   add creation code for the interface in ../interfaces.cc. If you do not have
   a specific interface, the object tool will be used instead.

