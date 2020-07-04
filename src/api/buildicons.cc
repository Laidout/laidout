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
// Copyright (C) 2020 by Tom Lechner
//

#include <lax/colors.h>
#include <lax/fileutils.h>

#include "buildicons.h"
#include "../language.h"
#include "../laidout.h"
#include "../filetypes/image.h"
#include "../filetypes/svg.h"

#include <unistd.h>


using namespace Laxkit;
using namespace LaxFiles;


namespace Laidout {


/*! \ingroup api */
ObjectDef *makeBuildIconsDef()
{
	 //define base
	ObjectDef *sd=new ObjectDef(NULL,"BuildIcons",
			_("Build Icons"),
			_("Build icons from source svg files"),
			"function",
			NULL,NULL,
			NULL,
			0, //new flags
			NULL,
			BuildIconsFunction);

	 //define parameters
	sd->push("files",
			_("Source files"),
			_("Set of source SVG files"),
			"set",
			NULL, //range
			nullptr,  //defvalue
			0,    //flags
			NULL);//newfunc
//	sd->push("pattern",
//			_("Pattern"),
//			_("Object id pattern"),
//			"any", //VALUE_DynamicEnum, ***
//			NULL, //range
//			NULL,  //defvalue
//			0,    //flags
//			NULL);//newfunc
	sd->push("grid_cell_size",
			_("Grid cell size"),
			_("Grid cell size in page units"),
			"real",
			NULL,
			".5",
			0,NULL);
	sd->push("output_px_size",
			_("Output pixel size"),
			_("Output pngs will have dimensions in multiples of output_px_size"),
			"int",
			NULL,
			"24",
			0,NULL);
	sd->push("output_dir",
			_("Output directory"),
			_("Where to put the generated image files."),
			"File",
			NULL,
			NULL,
			0,NULL);

	return sd;
}

/*! \ingroup api
 *
 * Build icons from source svg files.
 *
 * Return 0 for success, -1 for success with warnings, or 1 for unredeemable failure.
 */
int BuildIconsFunction(ValueHash *context, 
					 ValueHash *parameters,
					 Value **value_ret,
					 ErrorLog &log)
{
	Document *doc = nullptr;
	int        numhits = 0;
	clock_t time_start = times(nullptr);

	try {
		if (!parameters) throw _("BuildIcons needs parameters!");

		//    id pattern:
		//        0: [A-Z].*
		//          1: *-s*
		//            +
		//    padding: [by grid] [l r t b]
		//    unit size: .5 in
		//    output unit size: 24px  ....output will be in multiples of this, depending on bbox
		//    from files:
		//        - icons.svg
		//        - icons-tiling.svg
		//        - lax-icons.svg
		//        +
		//    output directory: _____ ...
			
		//BuildIcons( [ file1, file2, file3 ], 
		//			  pattern = "/*"  --OR--  regex = "spread/pagestack/0/.*/.*/[A-Z].*",
		//			  grid_cell_size = .5, //inches
		//			  output_px_size = 24,   //px
		//			  output_dir = "./icon-dump-test"
		//			 )

		 //----grid_cell_size
		int i;
		double grid_cell_size = .5; //inches
		double d = parameters->findIntOrDouble("grid_cell_size",-1,&i);
		if (i==2) throw _("Invalid format for grid_cell_size!");
		else if (i == 0) grid_cell_size = d;


		 //-----output_px_size
		int output_px_size = 24; //px
		int ii = parameters->findInt("output_px_size", -1, &i);
		if (i==2) throw _("Invalid format for output_px_size!");
		else if (i == 0) output_px_size = ii;


		 //-----files
		SetValue *files = nullptr;
		files = dynamic_cast<SetValue*>(parameters->find("files"));
		if (!files || !files->n()) throw _("Missing file list!");


		 //-----output_dir
		//Utf8String output_dir = "./icon-dump-test";
		Utf8String output_dir;
		Value *v = parameters->find("output_dir");
		if (!v) throw _("Missing output_dir!");
		if (dynamic_cast<StringValue*>(v)) {
			StringValue *sv = dynamic_cast<StringValue*>(v);
			output_dir = sv->str;
		} else if (dynamic_cast<FileValue*>(v)) {
			FileValue *fv = dynamic_cast<FileValue*>(v);
			output_dir = fv->filename;
		} else throw _("Wrong format for output_dir!");
		if (!output_dir.EndsWith("/")) output_dir.Append("/");


		Utf8String str;
		DoubleBBox box;
		DoubleBBox outbox;
		int        error = 0;
		ImageExportConfig config;

		config.target = DocumentExportConfig::TARGET_Single;
		config.start = 0;
		config.end = 0;
		config.use_transparent_bg = true;
		makestr(config.format, "png");
		PaperGroup *papergroup = new PaperGroup();
		papergroup->AddPaper(grid_cell_size, grid_cell_size, 0,0);
		config.papergroup = papergroup;
		ExportFilter *imagefilter = laidout->FindExportFilter(_("Image"), false); //<- searches by VersionName()
		if (!imagefilter) throw(_("Could not find image export filter!"));

		for (int c=0; !error && c<files->n(); c++) {
			StringValue *file = dynamic_cast<StringValue*>(files->e(c));
			if (!file) throw(_("Wrong value format for file!"));

			// import file
			if (!file_exists(file->str, 1, nullptr)) {
				str.Sprintf(_("Source file %s not found!"), file->str);
				throw str.c_str();
			}
			
			if (AddSvgDocument(file->str) != 0) {
				str.Sprintf(_("Problem reading svg file %s!"), file->str);
				throw str.c_str();
			}

			doc = laidout->project->docs.e[laidout->project->docs.n-1]->doc;
			Page *page = doc->pages.e[0];
			Group *pagelayer = dynamic_cast<DrawableObject*>(page->layers.e(0)); //the pagelayer object
			if (config.doc) config.doc->dec_count();
			config.doc = doc;
			doc->inc_count();
			int nx,ny;

			// walk through searching for pattern
			for (int c3=0; c3<pagelayer->n(); c3++) { //for each inkscape "layer"
				Group *layer = dynamic_cast<DrawableObject*>(pagelayer->e(c3));

				for (int c2=0; c2<layer->n(); c2++) {
					DrawableObject *obj = dynamic_cast<DrawableObject*>(layer->e(c2));
					if (obj->Id()[0] < 'A' || obj->Id()[0] > 'Z')
						continue;

					// else match!

					//   find bbox in page space
					box.clear();
					box.addtobounds(layer->transformPoint(obj->BBoxPoint(0,0,true)));
					box.addtobounds(layer->transformPoint(obj->BBoxPoint(1,0,true)));
					box.addtobounds(layer->transformPoint(obj->BBoxPoint(0,1,true)));
					box.addtobounds(layer->transformPoint(obj->BBoxPoint(1,1,true)));

					//   compute enclosing bounds
					outbox.minx = grid_cell_size * int(box.minx / grid_cell_size);
					outbox.maxx = grid_cell_size * int(box.maxx / grid_cell_size + 1);
					outbox.miny = grid_cell_size * int(box.miny / grid_cell_size);
					outbox.maxy = grid_cell_size * int(box.maxy / grid_cell_size + 1);
					nx = int((outbox.maxx - outbox.minx) / grid_cell_size + .5);
					ny = int((outbox.maxy - outbox.miny) / grid_cell_size + .5);

					//   compose export config for image with that crop
					papergroup->papers.e[0]->box->media.setbounds(0,0, outbox.boxwidth(), outbox.boxheight());
					papergroup->papers.e[0]->box->paperstyle->width = outbox.boxwidth();
					papergroup->papers.e[0]->box->paperstyle->height = outbox.boxheight();
					papergroup->papers.e[0]->origin(flatpoint(outbox.minx, outbox.miny));

					//   export
					Utf8String outfile(output_dir);
					outfile.Append(obj->Id());
					outfile.Append(".png");
					config.width  = nx * output_px_size;
					config.height = ny * output_px_size;
					makestr(config.filename, outfile.c_str());

					if (file_exists(output_dir.c_str(), 1, nullptr) != S_IFDIR) {
						//note: do this here so directory only gets created if we have an icon to actually make
						check_dirs(output_dir.c_str(), true, 0700);
					}
					cout << "Writing icon: "<<outfile.c_str()<<endl;
					if (imagefilter->Out(outfile.c_str(), &config, log) != 0) throw _("Error outputting image file!");

					numhits++;
				}
			}

			// remove file
			laidout->project->Pop(doc);
			doc = nullptr;
		}

	} catch (const char *error) {
		if (doc) laidout->project->Pop(doc);
		log.AddMessage(error,ERROR_Fail);
		if (value_ret) *value_ret=NULL;
		return 1;
	}
	
	time_start = times(nullptr) - time_start;
	cout << "Generated "<<numhits<<" icons in "<< (time_start / (float)sysconf(_SC_CLK_TCK)) <<" seconds."<<endl;
	if (value_ret) *value_ret=NULL;
	return 0;
}


} // namespace Laidout

