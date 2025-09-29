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

#include "streaminterface.h"
#include "streams.h"
#include "../language.h"

#include <lax/interfaces/textonpathinterface.h>
#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/viewerwindow.h>

#include <lax/debug.h>

using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {


//--------------------------- StreamInterface -------------------------------------


StreamInterface::StreamInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp)
  : TextStreamInterface(nowner, nid, ndp)
{}

StreamInterface::~StreamInterface()
{}

const char *StreamInterface::Name()
{
	return _("Streams");
}

const char *StreamInterface::whatdatatype()
{
	return nullptr;
}

anInterface *StreamInterface::duplicate(anInterface *dup)
{
	if (dup == nullptr) dup = new StreamInterface(nullptr,-1,nullptr);
	else if (!dynamic_cast<StreamInterface *>(dup)) return nullptr;
	return TextStreamInterface::duplicate(dup);
}

void StreamInterface::SetupDefaultFont()
{
	if (default_font) return;
	default_font = anXApp::app->defaultlaxfont->duplicate(); //TODO: should be a laidout option? like laidout->last_font
}

bool StreamInterface::AttachStream()
{
	if (extra_hover == TXT_Hover_Stroke) {
		TextOnPath *tonpath = dynamic_cast<TextOnPath *>(somedatafactory()->NewObject(LAX_TEXTONPATH));
		if (!tonpath) tonpath = new TextOnPath();

		if (text_to_place_hint != TXT_NewText)
			tonpath->Text(text_to_place.c_str());

		if (!default_font) SetupDefaultFont();
		LaxFont *font = default_font->duplicate();
		double default_size = 20;
		font->Resize(default_size);
		tonpath->Font(font);
		//textonpath->color->screen.red=65535;
		font->dec_count();

		// *** set StreamAttachment and/or tonpath link to object
		// *** todo: need to clarify how textonpath links to other objects.. this extrahover is meaningless outside viewport context
		tonpath->UseThisPath(extrahover, outline_index);

		ObjectContext *oc = nullptr;
		viewport->NewData(tonpath, &oc);//viewport adds only its own counts
		// if (toc) { delete toc; toc = nullptr; }
		// if (oc) toc = oc->duplicate();
		dynamic_cast<ViewerWindow*>(viewport->win_parent)->SelectToolFor("TextOnPath", oc);
		tonpath->dec_count();

	
		return true;

	} else if (extra_hover == TXT_Hover_Area) {
		const char *txt = nullptr;

		if (text_to_place_hint == TXT_NewText)
		// vvvvvvvvvvvv Testing
			txt = "One\n"
				  "Two\n"
				  "Three! Three point five\n"
				  "four";
		// ^^^^^^^^^^^^^^^ Testing
		else txt = text_to_place.c_str();

		DBGM("====================================================== AttachStream");
		Stream *stream = new Stream();
		ErrorLog log;
		stream->ImportText(txt,-1, nullptr, false, &log);
		stream->dump_out(stderr, 0, 0, nullptr);
		dumperrorlog("Stream test error log", log);

		DrawableObject *dobj = dynamic_cast<DrawableObject*>(extrahover->obj);
		StreamAttachment *attachment = new StreamAttachment(dobj, stream);
		dobj->streams.push(attachment);
		attachment->dec_count();
		stream->dec_count();

		PostMessage("IMP ME!!");
	}

	return false;
}


} // namespace Laidout


