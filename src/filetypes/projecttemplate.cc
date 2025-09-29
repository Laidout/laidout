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

#include "projecttemplate.h"

#include <filesystem>


//------ sample export.config:
//
// # Project Export Example
//
// # One or more of these to create base exports from Laidout pages
// filter_pass "Pass name"
//   filter "Html Gallery"
//     transparent  true  # filter config
//     tofiles "{temp}/####.html"
//
// copy  # direct copy of files in project template directory
//   pattern "scrim"
//   pattern "**/*.png"
//     exclude something/specific.png
//   pattern "here.txt" to "to/there.txt"
//   pattern "*"
//     exclude "*template*"
//
// templates  # transform template file(s) into destination files
//   template "Template pass name"  # one created per export
//     from  index-template.html
//     to    index.html
//     copy ...
//     vars
//       <!--SITE-NAME-->  "Default value"
//           tooltip "Description"
//       <!--NUM-DOC-PAGES-->
//           compute "context.doc.pages"
//           tooltip "Automatically add number of doc pages"
//       <!--TITLE-->
//       <!--DATE-->
//         uihint Date:now
//       <!--META-DESCRIPTION-->
//       <!--META-IMAGE-->
//       <!--META-URL-->
//       <!--META-IMAGE-->
//       <!--HANDLE-->
//       <!--BLURB-->
//       <!--IMAGE-LIST-->
//   template_spread # one of these created per output file from normal filter
//     from  page-template.html
//     to    "page####.html"

namespace fs = std::filesystem;

namespace Laidout {


/*! Return number of files found.
 */
int ExpandPattern(const char *base_path, const char *pattern, NumStack<filesystem::path> &paths, bool follow_links)
{
}

int CopyRecursive(const char *base_path, const char *pattern, const char *destination_path, bool follow_links)
{
	fs::path pth(string(base_path));

	for (const fs::directory_entry& dir_entry : fs::recursive_directory_iterator(pth)) {
        std::cout << dir_entry << '\n';
    }


	filesystem::copy(from, to, filesystem::copy_options::overwrite_existing | filesystem::copy_options::recursive);
}

int TraverseRecursive(std::path at, std::set &visited_dirs, function per_file)
{

}


//------------------------------ ProjectTemplate -------------------------------

ProjectTemplate::ProjectTemplate(const char *config_file)
{
	if (config_file) InitFromFile(config_file);
}

ProjectTemplate::~ProjectTemplate()
{}


bool ProjectTemplate::InitFromFile(const char *config_file)
{
	FILE *f = fopen(config_file, "r");
	if (!f) return false;

	project_template_path = config_file;
	copy.flush();
	export_filters.flush();
	templates.flush();

	setlocale(LC_ALL,"C");
	dump_in(f, 0, 0, nullptr, nullptr);

	fclose(f);
	setlocale(LC_ALL,"");
	return true;
}

void ProjectTemplate::dump_out(FILE *f, int indent, int what, Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att, what, context);
	att.dump_out(f, indent);
}

Laxkit::Attribute *ProjectTemplate::dump_out_atts(Laxkit::Attribute *att, int what, Laxkit::DumpContext *context)
{
	DBGE("IMPLEMENT ME!!");
	return nullptr;
}


void ProjectTemplate::dump_in_atts(Laxkit::Attribute *att, int flag, Laxkit::DumpContext *context)
{
	for (int c = 0; c < att->attributes.n; c++) {
		const char *name  = att->attributes.e[c]->name;
		const char *value = att->attributes.e[c]->value;

		if (strEquals(name, "copy")) {
		} else if (strEquals(name, "meta")) {
		} else if (strEquals(name, "filter_pass")) {
		} else if (strEquals(name, "templates")) {
		}
	}
}

/*! Return true for successful export, else false.
 */
bool ProjectTemplate::Export(Laxkit::ErrorLog &log)
{
	bool status = true;

	// do any direct copying
	for (int c = 0; c < copy.n; c++) {
		CopyPattern *cc = copy.e[c];
		RecursiveCopy(project_template_path, cc->src_pattern, cc->exclude_pattern, cc->dest_pattern);
	}

	// generate files from document pages
	for (int c = 0; c < export_filters.n; c++) {
		***
	}

	// process templates
	for (int c = 0; status && c < templates.n; c++) {
		Template *template = templates.e[c];
		if (template->roll == Template::OneOff) {
			for (int c2 = 0; status && c2 < template->vars.n; c2++) {
				*** set up context
				TemplateVar *var = template->vars.e[c2];
				if (var->is_compute) {
					Value *result = nullptr;
					laidout->calculator.evaluate(var->default_value.c_str(),-1, &result, &log);
				}
			}
			if (!status) break;

			status = ProcessTemplate(template->template_file.c_str(), dest_file, template->vars, extra_vars, log);
			if (!status) break;

		} else {
			// Template::PerSpread
			***
			for (c2 = 0; c2 < *** each filter output file; c2++) {
				*** set up context
				for (int c3 = 0; status && c3 < template->vars.n; c3++) {
					*** set up context
					TemplateVar *var = template->vars.e[c3];
					if (var->is_compute) {
						Value *result = nullptr;
						laidout->calculator.evaluate(var->default_value.c_str(),-1, &result, &log);
					}
				}
				if (!status) break;

				status = ProcessTemplate(template->template_file.c_str(), dest_file, template->vars, extra_vars, log);
				if (!status) break;
			}
		}
	}
	
	return status;
}

bool ProjectTemplate::ProcessTemplate(const char *template_file, const char *dest_file, const NumStack<TemplateVar> &vars, Laxkit::Attribute *extra_vars, Laxkit::ErrorLog &log)
{
	Utf8String content;
	int n = 0;
	char *templ = read_in_whole_file(template_file, &n, -1);
	if (!templ) {
		log.AddError("Could not read template file for reading!");
		return false;
	}
	content.InsertBytes(templ,-1,-1);

	for (int c = 0; c < template->vars.n; c++) {
		TemplateVar *var = template->vars.e[c];
		content.Replace(var->name.c_str(), var->cached_value.c_str(), true);
	}

	if (extra_vars) for (int c = 0; c < extra_vars.attributes.n; c++) {
		content.Replace(extra_vars->attributes.e[tc]->name, extra_vars->attributes.e[tc]->value, true);
	}

	bool save_error = save_string_to_file(content.c_str(), content.Bytes(), dest_file);
	return !save_error;
}


} // namespace Laidout
