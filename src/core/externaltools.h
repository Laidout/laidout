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
// Copyright (C) 2021 by Tom Lechner
//
#ifndef EXTERNALTOOLS_H
#define EXTERNALTOOLS_H


#include <lax/anobject.h>

#include "../calculator/values.h"


namespace Laidout {


//-------------------------------------- ExternalTool ------------------------------

class ExternalTool : public Laxkit::anObject
{
  public:
  	int category;

  	char *command_name; //"lp" non-translated name, not necessarily basename
	char *binary_path;  //"/usr/bin/lp" (including executable)
	char *parameters; // command_name "{filename} --stuff {extra1} {extra2} {extra3}"
	int verified; //1 yes binary_path exists, 0 failed, -1 unknown

	char *name;         //_("Print")
	char *description;  //_("Send pdf files to this command to print")
	char *doc_website;
	char *file_in_ext;  //"pdf"

	ValueHash *extra_config;

	// function to verify file
	// glob for expected output files (so that we can import generated images, for instance)
	

	ExternalTool(const char *cmd_name, const char *name, int category);
	virtual ~ExternalTool();
	virtual anObject *duplicate(anObject *ref);
	virtual void SetFrom(ExternalTool *tool);

	virtual void Description(const char *desc);
	virtual void Name(const char *name);
	virtual bool Valid();
	virtual bool Verify();
	virtual int FindPath(); //from command_name and env PATH
	virtual int RunCommand(Laxkit::PtrStack<char> &files, Laxkit::ErrorLog &log, bool in_background);
};


//-------------------------------------- ExternalToolCategory ------------------------------

class ExternalToolCategory : public Laxkit::anObject
{
  public:
  	enum Category {
  		PrintCommand = 1,
  		WebBrowser,
  		TextEditor,
  		ImageEditor,
  		ImageViewer,
  		MAX
  	};
  	
  	int id;
    char *Name;
    char *description;
    bool is_user_category;

    Laxkit::RefPtrStack<ExternalTool> tools;
	
    ExternalToolCategory(int nid, const char *name, const char *Name, const char *desc, bool is_user);
    ~ExternalToolCategory();

    static int GetNewUniqueId();
};


//-------------------------------------- ExternalToolManager ------------------------------

class ExternalToolManager : public Laxkit::anObject, public LaxFiles::DumpUtility
{
  public:
  	Laxkit::RefPtrStack<ExternalToolCategory> external_categories;

  	ExternalToolManager();
  	virtual ~ExternalToolManager();

	virtual void SetupDefaults();
	virtual int Save();

  	virtual int AddExternalTool(ExternalTool *tool);
	virtual int AddExternalCategory(ExternalToolCategory *category);
	virtual ExternalToolCategory *GetToolCategory(int category);
	virtual ExternalToolCategory *GetToolCategory(const char *category_id);
	virtual const char *GetToolCategoryName(int id, const char **idstr = nullptr);
	virtual ExternalTool *FindExternalTool(const char *str);
	virtual ExternalTool *GetDefaultTool(int category);

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
  	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
  	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag, LaxFiles::DumpContext *context);
};


} //namespace Laidout


#endif
