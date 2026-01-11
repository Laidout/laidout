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
// # transform template file(s) into destination files
// template "Template pass name"  # one created per export
//   from  index-template.html
//   to    index.html
//   copy ...
//   vars
//     <!--SITE-NAME-->  "Default value"
//         tooltip "Description"
//     <!--NUM-DOC-PAGES-->
//         compute "context.doc.pages"
//         tooltip "Automatically add number of doc pages"
//     <!--TITLE-->
//     <!--DATE-->
//       uihint Date:now
//     <!--META-DESCRIPTION-->
//     <!--META-IMAGE-->
//     <!--META-URL-->
//     <!--META-IMAGE-->
//     <!--HANDLE-->
//     <!--BLURB-->
//     <!--IMAGE-LIST-->
// 
// # transform this template once for each rendered Laidout spread
// template_spread # one of these created per output file from normal filter
//   from  page-template.html
//   to    "page####.html"

namespace fs = std::filesystem;

namespace Laidout {


/*! Return number of files found.
 */
int ExpandPattern(const char *base_path, const char *pattern, NumStack<filesystem::path> &paths, bool follow_links)
{
	***
}

int CopyRecursive(const char *base_path, const char *pattern, const char *destination_path, bool follow_links)
{
	***

	fs::path pth(string(base_path));

	for (const fs::directory_entry& dir_entry : fs::recursive_directory_iterator(pth)) {
        std::cout << dir_entry << '\n';
    }

	filesystem::copy(from, to, filesystem::copy_options::overwrite_existing | filesystem::copy_options::recursive);
}

int TraverseRecursive(std::path at, std::set &visited_dirs, function<std::path> per_file)
{
	***
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
	if (!att) att = new Attribute();

	if (meta->attributes.n) {
		Attribute *att2 = att->pushSubAtt("meta");
		for (int c = 0; c < meta->attributes.n; c++) {
			att2->push(meta->attributes.e[c]->name, meta->attributes.e[c]->value);
		}
	}

	for (int c = 0; c < export_filters.n; c++) {
		DocumentExportConfig *config = export_filters.e[c];
		Attribute *att2 = att->pushSubAtt("filter_pass", config->filter->Format());
		config->dump_out_atts(att2, what, context);
	}

	if (copy.n) {
		Attribute *att2 = att->pushSubAtt("copy");
		for (int c = 0; c < copy.n; c++) {
			CopyPatter *cc = copy.e[c];
			Attribute *att3 = att2->pushSubAtt("pattern", cc->src_pattern.c_str());
			if (!cc->exclude_pattern.IsEmpty()) att3->push("exclude", cc->exclude_pattern.c_str());
			att3->push("to", dest_pattern.c_str());
		}
	}

	for (int c = 0; c < templates.n; c++) {
		Template *template = templates.[c];
		Attribute *att2;
		if (template->roll == Template::OneOff) att2 = att->pushSubAtt("template");
		else att2 = att->pushSubAtt("template_spread");
		att2->push("template_file", template->template_file.c_str());
		att2->push("dest_path", template->dest_path.c_str());
		if (template->vars.n) {
			Attribute *att3 = att2->pushSubAtt("vars");
			for (int c2 = 0; c2 < template->vars.n; c2++) {
				TemplateVar *v = template->vars.e[c2];
				att3->push(v->name, v->is_compute ? nullptr, v->default_value.c_str());
				if (v->is_compute) att3->Top()->push("compute", v->default_value.c_str());
				if (!v->tooltip.IsEmpty()) att3->Top()->push("tooltip", v->tooltip.c_str());
			}
		}
		if (template->copy.n) {
			Attribute *att3 = att->pushSubAtt("copy");
			for (int c = 0; c < template->copy.n; c++) {
				CopyPatter *cc = template->copy.e[c];
				Attribute *att4 = att3->pushSubAtt("pattern", cc->src_pattern.c_str());
				if (!cc->exclude_pattern.IsEmpty()) att4->push("exclude", cc->exclude_pattern.c_str());
				att4->push("to", dest_pattern.c_str());
			}
		}
	}

	return nullptr;
}


void ProjectTemplate::dump_in_atts(Laxkit::Attribute *att, int flag, Laxkit::DumpContext *context)
{
	for (int c = 0; c < att->attributes.n; c++) {
		const char *name  = att->attributes.e[c]->name;
		const char *value = att->attributes.e[c]->value;

		if (strEquals(name, "meta")) {
			for (int c = 0; c < att->attributes.n; c++) {
				const char *name  = att->attributes.e[c]->name;
				const char *value = att->attributes.e[c]->value;

				meta.push(name, value);
			}

		} else if (strEquals(name, "copy")) {	  	

			for (int c2 = 0; c2 < att->attributes.e[c]->attributes.n; c2++) {
				const char *name  = att->attributes.e[c]->attributes.e[c2]->name;
				const char *value = att->attributes.e[c]->attributes.e[c2]->value;

				if (strEquals(name, "pattern")) {
					CopyPattern pattern;
				  	pattern.src_pattern = value;

				  	// Utf8String src_pattern;
				  	// Utf8String exclude_pattern;
				  	// Utf8String dest_pattern;

				  	for (int c3 = 0; c3 < att->attributes.e[c]->attributes.e[c2]->attributes.n; c3++) {
						const char *name  = att->attributes.e[c]->attributes.e[c2]->attributes.e[c3]->name;
						const char *value = att->attributes.e[c]->attributes.e[c2]->attributes.e[c3]->value;

						if (strEquals(name, "exclude")) {
				  			pattern.exclude_pattern = value;

						} else if (strEquals(name, "to")) {
							pattern.dest_pattern = value;
						}
					}

				  	copy.push(pattern);
				}
			}

		} else if (strEquals(name, "filter_pass")) {
			// filter_pass "Html Gallery"
			//     transparent  true  # filter config
			//     tofiles "{temp}/####.html"
			ExportFilter *filter = laidout->FindExportFilter(value);
			
			if (!filter) {
				if (context->log) log->AddError(0,0,0,_("Unknown export filter: %s"), value ? value : _("null"));
			} else {
				DocumentExportConfig *config = filter->CreateConfig(nullptr);
				config->filter = filter;
				export_filters.push(config);
				config->dec_count();
			}

		} else if (strEquals(name, "template") || strEquals(name, "template_spread")) {
			bool for_spread = strEquals(name, "template_spread");
			ExportTemplate *template = new ExportTemplate();
			template->roll = for_spread ? ExportTemplate::PerSpread : ExportTemplate::OneOff;

			for (int c2 = 0; c2 < att->attributes.e[c]->attributes.n; c2++) {
				const char *name  = att->attributes.e[c]->attributes.e[c2]->name;
				const char *value = att->attributes.e[c]->attributes.e[c2]->value;

				if (strEquals(name, "pattern")) {
				} else if (strEquals(name, "template_file")) {
				} else if (strEquals(name, "dest_path")) {
				}
			}

			templates.push(template);
			template->dec_count();
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
