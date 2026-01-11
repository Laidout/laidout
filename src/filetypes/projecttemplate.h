//
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
// Copyright (C) 2025 by Tom Lechner
//
#ifndef PROJECTEXPORT_H
#define PROJECTEXPORT_H

#include <lax/errorlog.h>
#include <lax/anxapp.h>
#include <lax/dump.h>
#include <lax/indexrange.h>

#include "../core/papersizes.h"
#include "../core/externaltools.h"
#include "../calculator/values.h"
#include "../plugins/plugin.h"


namespace Laidout {

class CopyPattern
{
  public:
  	Utf8String src_pattern;
  	Utf8String exclude_pattern;
  	Utf8String dest_pattern;
};

class TemplateVar
{
  public:
  	Utf8String name;
  	Utf8String tooltip;
  	Utf8String default_value;
  	bool is_compute = false; // if true, default_value is computed

  	Utf8String cached_value; // result if is_compute
};

class ExportTemplate
{
  public:
  	DocumentExportConfig *custom_base_export = nullptr;

  	enum Roll { OneOff, PerSpread };
  	Roll roll = OneOff;
  	Utf8String template_file;
  	Utf8String dest_path;
  	NumStack<Utf8String> vars;
  	NumStack<CopyPattern> copy;

  	~ExportTemplate() { if (custom_base_export) custom_base_export->dec_count(); }
};


//------------------------------ ProjectExportTemplate -------------------------------

class ProjectTemplate : public Value
{
  protected:
  	bool ProcessTemplate(const char *template_file, const char *dest_file, const NumStack<TemplateVar> &vars, Laxkit::Attribute *extra_vars, Laxkit::ErrorLog &log);

  public:
  	std::path project_template_path;

  	Laxkit::Attribute meta;
  	NumStack<CopyPattern> copy;
  	RefPtrStack<DocumentExportConfig> export_filters;
  	RefPtrStack<ExportTemplate> templates;

  	ProjectTemplate(const char *config_file);
  	virtual ~ProjectTemplate();

  	virtual void dump_out(FILE *f, int indent, int what, Laxkit::DumpContext *context);
  	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
  	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);

  	bool Export(Laxkit::ErrorLog &log);
};


} // namespace Laidout

#endif


