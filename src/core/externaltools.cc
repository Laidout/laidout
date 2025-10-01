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


#include <lax/strmanip.h>
#include <lax/fileutils.h>
#include <lax/utf8string.h>

#include "../laidout.h"
#include "../language.h"
#include "utils.h"

#include "externaltools.h"

#include <lax/debug.h>
using namespace std;

using namespace Laxkit;


namespace Laidout {

//----------------------------- ExternalToolCategory -----------------------------

ExternalToolCategory::ExternalToolCategory(int nid, const char *name, const char *Name, const char *desc, bool is_user)
{
	id = nid;
	Id(name);
	this->Name = newstr(Name);
	description = newstr(desc);
	is_user_category = is_user;
}

ExternalToolCategory::~ExternalToolCategory()
{
	delete[] Name;
	delete[] description;
}

/*! Static func to check laidout->prefs for new id. */
int ExternalToolCategory::GetNewUniqueId()
{
	int id = ExternalToolCategory::MAX;
	for (int c=0; c<laidout->prefs.external_tool_manager.external_categories.n; c++) {
		if (laidout->prefs.external_tool_manager.external_categories.e[c]->id >= id)
			id = laidout->prefs.external_tool_manager.external_categories.e[c]->id + 1;
	}
	return id;
}

//----------------------------- ExternalTool -----------------------------

ExternalTool::ExternalTool(const char *cmd_name, const char *name, int category)
{
	this->category = category;
	command_name = newstr(cmd_name);
	binary_path  = nullptr;  //"/usr/bin/lp"
	this->name   = newstr(name);
	description  = nullptr;  //_("Print")
	doc_website  = nullptr;
	//file_in_ext  = nullptr;  //"pdf"
	extra_config = nullptr;

	//TODO: ?  glob for expected output files (so that we can import generated images, for instance)
	
	parameters = nullptr; // command_name "{filename} --stuff {extra1} {extra2} {extra3}"

	verified = -1; //1 yes, 0 failed, -1 unknown
}

ExternalTool::~ExternalTool()
{
	delete[] command_name;
	delete[] binary_path;
	delete[] name;
	delete[] description;
	delete[] doc_website;
	//delete[] file_in_ext;
	delete[] parameters;
	if (extra_config) extra_config->dec_count();
}

anObject *ExternalTool::duplicate()
{
	// ExternalTool *rf = dynamic_cast<ExternalTool*>(ref);
	ExternalTool *tool = new ExternalTool(command_name, name, category);
	makestr(tool->binary_path, binary_path);
	makestr(tool->description, description);
	makestr(tool->parameters,  parameters);
	makestr(tool->doc_website, doc_website);
	//makestr(tool->file_in_ext, file_in_ext);
	return tool;
}

void ExternalTool::SetFrom(ExternalTool *tool)
{
	if (!tool) return;
	this->category = tool->category;
	makestr(command_name, tool->command_name);
	makestr(binary_path,  tool->binary_path);
	makestr(name,         tool->name);
	makestr(description,  tool->description);
	makestr(parameters,   tool->parameters);
	makestr(doc_website,  tool->doc_website);
	//makestr(file_in_ext,  tool->file_in_ext);
	// extra_config = nullptr;
}

void ExternalTool::Description(const char *desc)
{
	makestr(description, desc);
}

void ExternalTool::Name(const char *name)
{
	makestr(this->name, name);
}

/*! True when verified == 1. This should mean there is an executable at binary_path.
 */
bool ExternalTool::Valid()
{
	if (verified == -1) FindPath();
	return verified == 1;
}

/*! Names need to be non-blank, and binary_path needs to exist. Update verified.
 */
bool ExternalTool::Verify()
{
	verified = !isblank(name) && !isblank(command_name) && file_exists(binary_path, 1, nullptr);
	return verified;
}

/*! From command_name, find binary_path by searching for command_name on each in environment varible PATH.
 * Sets verified. 
 * Return 1 for verified, else 0.
 * If binary_path is already valid, use that and return 1.
 */
int ExternalTool::FindPath()
{
	if (binary_path && file_exists(binary_path, 1, nullptr)) {
		verified = 1;
		return 1;
	}
	
	const char *path = getenv("PATH");
	if (!path) {
		verified = 0;
		return 0;
	}

	int n = 0;
	char **paths = split(path, ':', &n);


	Utf8String fullpath;

	for (int c=0; c<n; c++) {
		fullpath.Sprintf("%s/%s", paths[c], command_name);
		if (!file_exists(fullpath.c_str(), 1, nullptr)) {
			if (c == n-1) {
				verified = 0;
				return 0;
			}
			continue;
		}

		//note: punting on legitimate test of executability until we actually run because of laziness.
		makestr(binary_path, fullpath.c_str());
		break;
	}

	deletestrs(paths, n);
	verified = 1;
	return 1;
}

/*! Return the command return. Note this will be -1 for misc error, else the return value of the command.
 */
int ExternalTool::RunCommand(Laxkit::PtrStack<char> &arguments, Laxkit::ErrorLog &log, bool in_background)
{
	if (!Valid()) {
		log.AddError(_("Command is not valid!"));
		return -1;
	}
	
	//build command:   command {arguments} {context.page_label}
	Utf8String cm;
	cm.Sprintf("%s", binary_path);

	// add arguments
	if (!isblank(parameters)) {
		Utf8String args = parameters;
		// {files} {file} {dir} {basename} %f
		if (args.Find("{files}",0,true) >= 0) {
			Utf8String all;
			for (int c=0; c<arguments.n; c++) {
				all.Append(" ");
				all.Append(arguments.e[c]);
			}
			args.Replace("{files}", all.c_str(), true);
		}

		args.Replace("%f", arguments[0], true);
		args.Replace("{file}", arguments[0], true);

		if (args.Find("{dirname}", 0, false) >= 0) {
			char *dname = lax_dirname(arguments[0], false);
			args.Replace("{dirname}", dname, true);
			delete[] dname;
		}
		args.Replace("{basename}", lax_basename(arguments[0]), true);

		cm.Append(" ");
		cm.Append(args);

	} else { // do default parameters, as list of arguments
		for (int c=0; c<arguments.n; c++) {
			cm.Append(" ");
			cm.Append(arguments.e[c]);
		}
	}

	if (isblank(cm.c_str())) {
		log.AddError(_("Missing command!"));
		return -1;
	}

	if (in_background) cm.Append(" &");

	DBGM("Running command: "<<cm.c_str());

	//run command in foreground:
	int status = system(cm.c_str()); //-1 for error, else the return value of the call

    if (status != 0) {
		log.AddError(_("Error running command!"));
		return status;
    }

	//done, no error!
    return status;
}



//---------------------- ExternalToolManager ------------------------------------

/*! \class ExternalToolManager
 * Class to handle list of categories and external executables to use for various purposes across Laidout.
 * 
 * This is meant to consolidate running system commands to a single pathway in the event that security becomes
 * a bigger issue some day.
 */

ExternalToolManager::ExternalToolManager()
{}
 
ExternalToolManager::~ExternalToolManager()
{

}

void ExternalToolManager::SetupDefaults()
{
	//int nid, const char *name, const char *Name, const char *desc, bool is_user
	ExternalToolCategory *cat = nullptr;
	cat = new ExternalToolCategory(ExternalToolCategory::PrintCommand, "Print", _("Print"), nullptr, false);
	AddExternalCategory(cat);

	cat = new ExternalToolCategory(ExternalToolCategory::TextEditor, "TextEditor", _("Text editor"), nullptr, false);
	AddExternalCategory(cat);

	cat = new ExternalToolCategory(ExternalToolCategory::ImageEditor, "ImageEditor", _("Image editor"), nullptr, false);
	AddExternalCategory(cat);

	cat = new ExternalToolCategory(ExternalToolCategory::ImageViewer, "ImageViewer", _("Image viewer"), nullptr, false);
	AddExternalCategory(cat);

	cat = new ExternalToolCategory(ExternalToolCategory::WebBrowser, "WebBrowser", _("Web browser"), nullptr, false);
	AddExternalCategory(cat);

	cat = new ExternalToolCategory(ExternalToolCategory::FileBrowser, "FileBrowser", _("File browser"), nullptr, false);
	AddExternalCategory(cat);

	cat = new ExternalToolCategory(ExternalToolCategory::Misc, "Misc", _("Misc"), nullptr, false);
	AddExternalCategory(cat);

	bool do_default = true;
	char *configfile = ConfigDir(nullptr);
	appendstr(configfile, "external_tools.conf");
	if (file_exists(configfile, 1, nullptr)) {
		FILE *f = fopen(configfile, "rb");
		if (f) {
			do_default = false;
			dump_in(f,0,0,nullptr,nullptr);
			fclose(f);
		} else {
			cerr << "Warning! Could not open "<<configfile<<" for reading! Using defaults instead."<<endl;
		}
	}

	if (do_default) {

		ExternalTool *cmd = new ExternalTool("lp", "lp", ExternalToolCategory::PrintCommand);
		cmd->Description(_("Send a PDF to a printer with the lp command"));
		cmd->FindPath();
		if (cmd->Valid()) AddExternalTool(cmd);
		else delete cmd;

		cmd = new ExternalTool("gvim", _("Gvim"), ExternalToolCategory::TextEditor);
		cmd->FindPath();
		if (cmd->Valid()) AddExternalTool(cmd);
		else delete cmd;

		cmd = new ExternalTool("gimp", _("Gimp"), ExternalToolCategory::ImageEditor);
		cmd->FindPath();
		if (cmd->Valid()) AddExternalTool(cmd);
		else delete cmd;

		cmd = new ExternalTool("feh", _("Feh"), ExternalToolCategory::ImageViewer);
		cmd->FindPath();
		if (cmd->Valid()) AddExternalTool(cmd);
		else {
			delete cmd;
			cmd = new ExternalTool("xdg-open", _("xdg-open"), ExternalToolCategory::ImageViewer);
			cmd->FindPath();
			if (cmd->Valid()) AddExternalTool(cmd);
			else delete cmd;
		}

		cmd = new ExternalTool("xdg-open", _("xdg-open"), ExternalToolCategory::WebBrowser);
		cmd->FindPath();
		if (cmd->Valid()) AddExternalTool(cmd);
		else delete cmd;

		cmd = new ExternalTool("xdg-open", _("xdg-open"), ExternalToolCategory::FileBrowser);
		cmd->FindPath();
		if (cmd->Valid()) AddExternalTool(cmd);
		else delete cmd;
	}

	delete[] configfile;
}

/*! Clobber existing file. Return 1 for saved, else 0. */
int ExternalToolManager::Save()
{
	char *configfile = ConfigDir(nullptr);
	appendstr(configfile, "external_tools.conf");

	FILE *f = fopen(configfile, "w");
	if (f) {
		dump_out(f,0,0,nullptr);
		fclose(f);
	} else {
		cerr << "Warning! Could not open "<<configfile<<" for writing! Using defaults instead."<<endl;
		delete[] configfile;	
		return 0;
	}

	delete[] configfile;
	return 1;
}


/*! tool->category must already be in external_categories, or else nothing is done and 1 is returned.
 * Return 0 if added successfully.
 *  Takes ownership of tool, so don't worry about dec_count UNLESS 1 IS RETURNED. 
 */
int ExternalToolManager::AddExternalTool(ExternalTool *tool)
{
	for (int c=0; c<external_categories.n; c++) {
		if (external_categories.e[c]->id == tool->category) {
			external_categories.e[c]->tools.push(tool);
			tool->dec_count();
			// Save();
			return 0;
		}
	}
	return 1;
}

/*! Takes ownership of category, so don't worry about dec_count. */
int ExternalToolManager::AddExternalCategory(ExternalToolCategory *category)
{
	external_categories.push(category);
	category->dec_count();
	// Save();
	return 0;
}

const char *ExternalToolManager::GetToolCategoryName(int id, const char **idstr)
{
	for (int c=0; c<external_categories.n; c++) {
		if (id == external_categories.e[c]->id) {
			if (idstr) *idstr = external_categories.e[c]->Id();
			return external_categories.e[c]->Name;
		}
	}
	if (idstr) *idstr = nullptr;
	return nullptr;
}

ExternalToolCategory *ExternalToolManager::GetToolCategory(const char *category_id)
{
	for (int c=0; c<external_categories.n; c++) {
		if (strEquals(category_id, external_categories.e[c]->Id())) {
			return external_categories.e[c];
		}
	}
	return nullptr;
}

ExternalToolCategory *ExternalToolManager::GetToolCategory(int category)
{
	for (int c=0; c<external_categories.n; c++) {
		if (category == external_categories.e[c]->id) {
			return external_categories.e[c];
		}
	}
	return nullptr;
}

/*! Find existing tool from str like "Print: lp" -> "category: command_name".
 */
ExternalTool *ExternalToolManager::FindExternalTool(const char *str)
{
	const char *ss = strstr(str, ":");
	if (!ss) return nullptr;
	for (int c=0; c<external_categories.n; c++) {
		if (!strncmp(external_categories.e[c]->Id(), str, ss-str)) {
			ss++;
			while (isspace(*ss)) ss++;
			for (int c2=0; c2<external_categories.e[c]->tools.n; c2++) {
				if (!strcmp(external_categories.e[c]->tools.e[c2]->command_name, ss))
					return external_categories.e[c]->tools.e[c2];
			}
		}
	}
	return nullptr;
}

/*! Return the first tool in category, or null. */
ExternalTool *ExternalToolManager::GetDefaultTool(int category)
{
	for (int c=0; c<external_categories.n; c++) {
		if (external_categories.e[c]->id == category) {
			if (external_categories.e[c]->tools.n > 0) return external_categories.e[c]->tools.e[0];
			return nullptr;
		}
	}
	return nullptr;
}

void ExternalToolManager::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute *att=dump_out_atts(NULL,0,context);
	att->dump_out(f,indent);
	delete att;
}

Laxkit::Attribute *ExternalToolManager::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (!att) att = new Attribute();

	for (int c=0; c<external_categories.n; c++) {
		Attribute *att2 = att->pushSubAtt("category");

		ExternalToolCategory *cat = external_categories.e[c];
		att2->push("name", cat->Id());
		att2->push("Name", cat->Name);
		att2->push("description", cat->description);

		//tools
		for (int c2=0; c2<cat->tools.n; c2++) {
			Attribute *att3 = att2->pushSubAtt("tool");

			ExternalTool *tool = cat->tools.e[c2];
			att3->push("name", tool->name);
			att3->push("command_name", tool->command_name); //"lp" non-translated name, not necessarily basename
			att3->push("binary_path", tool->binary_path);  //"/usr/bin/lp" (including executable)
			att3->push("parameters", tool->parameters); // command_name "{filename} --stuff {extra1} {extra2} {extra3}"
			att3->push("description", tool->description);  //_("Send pdf files to this command to print")
			if (tool->doc_website) att3->push("doc_website", tool->doc_website);
			//if (tool->file_in_ext) att3->push("file_in_ext", tool->file_in_ext);  //"pdf"

			// ValueHash *extra_config;
		}
	}

	return att;
}

void ExternalToolManager::dump_in_atts(Attribute *att,int flag, Laxkit::DumpContext *context)
{
	//we are assuming that built in categories exist already, but not tools
	
	const char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"category")) {
			Attribute *att2 = att->attributes.e[c];

			name = att2->findValue("name");
			value = att2->findValue("description");

			ExternalToolCategory *cat = GetToolCategory(name);
			if (!cat) {
				cat = new ExternalToolCategory(ExternalToolCategory::GetNewUniqueId(), name, att2->findValue("Name"), value, true);
				external_categories.push(cat);
				cat->dec_count();
			}

			for (int c2=0; c2<att2->attributes.n; c2++) {
				name= att2->attributes.e[c2]->name;
				value=att2->attributes.e[c2]->value;

				if (!strcmp(name,"tool")) {
					ExternalTool *tool = new ExternalTool(nullptr, nullptr, cat->id);
					cat->tools.push(tool);
					tool->dec_count();

					Attribute *att3 = att2->attributes.e[c2];
					for (int c3=0; c3<att3->attributes.n; c3++) {
						name= att3->attributes.e[c3]->name;
						value=att3->attributes.e[c3]->value;

						if (!strcmp(name,"name")) {
							makestr(tool->name, value);
						} else if (!strcmp(name,"command_name")) {
							makestr(tool->command_name, value);
						} else if (!strcmp(name,"binary_path")) {
							makestr(tool->binary_path, value);
						} else if (!strcmp(name,"parameters")) {
							makestr(tool->parameters, value);
						} else if (!strcmp(name,"description")) {
							makestr(tool->description, value);
						} else if (!strcmp(name,"doc_website")) {
							makestr(tool->doc_website, value);
						//} else if (!strcmp(name,"file_in_ext")) {
						//	makestr(tool->file_in_ext, value);
						}
					}

					tool->Verify();
				}
			}
		}
	}
}



} //namespace Laidout


