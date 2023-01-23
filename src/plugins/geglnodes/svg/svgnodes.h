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
#ifndef PLUGINS_SVG_NODES_H
#define PLUGINS_SVG_NODES_H


#include <lax/objectfactory.h>
#include "../../../filetypes/objectio.h"
#include "../../plugin.h"


//extern "C" Laidout::PluginBase *GetPlugin();


namespace Laidout {
namespace SvgFilterNS {


//---------------------- SvgFilterNode ----------------------------

void RegisterSvgNodes(Laxkit::ObjectFactory *factory);

class SvgFilterLoader : public Laidout::ObjectIO
{
  public:
    SvgFilterLoader(PluginBase *fromplugin) { plugin = fromplugin; }
    virtual ~SvgFilterLoader() {}

    virtual const char *Author() { return "Laidout"; }
    virtual const char *FilterVersion() { return "0.1"; }

    virtual const char *Format() { return "svg"; }
    virtual const char *DefaultExtension()  { return "svg"; }
    virtual const char *Version() { return "0.1"; }
    virtual const char *VersionName() { return "Svg Filter"; }
    virtual ObjectDef *GetObjectDef() { return NULL; }

    virtual int Serializable(int what) { return 0; }
    virtual int CanImport(const char *file, const char *first500);
    virtual int CanExport(anObject *object);
    virtual int Import(const char *file, int file_is_data, Laxkit::anObject **object_ret, Laxkit::anObject *context, Laxkit::ErrorLog &log);
    virtual int Export(const char *file, anObject *object, Laxkit::anObject *context, Laxkit::ErrorLog &log);
};



} //namespace SvgFilterNS
} //namespace Laidout



#endif


