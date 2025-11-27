



namespace Laidout {


bool MatchIfIndex(Laxkit::anObject *obj, SearchPattern *pattern)
{
	const char *index_property = "index_item";

	DrawableObject *dobj = dynamic_cast<DrawableObject*>(obj);
	if (!dobj) return false;

	*** need special scan of stream caches

	return dobj->properties.HasKey(index_property);
}

void MatchIfExternalResources(Laxkit::anObject *obj, SearchPattern *pattern)
{
	// need to check for:
	// - fonts
	// - gradients
	// - external text
	// - images
	// - imposition from file
	// - polyhedron files?
	// - node values referencing random paths
}

int MakeIndex(const char *index_property, Document *doc)
{
	// objects or parts of streams marked with index property are collected
	// index_entry, index_category, special formatting hints
	ObjectIterator iterator;
	iterater.StartIn(doc);

	Laxkit::PtrStack<FindResult> results;
	iterator.FindAll(MatchIfExternalResources, results);

	std::map<std::string, std::vector<int> entries;

	for (int c = 0; c < results.n; c++) {
		std::string key = results.e[c]->obj->properties.findString(index_property);
		int page_num = ***;

		if (entries.count(key) == 0) {
			entries[key] = std::vector<int>();
		}
		entries[key].push_back(page_num);
		----
		if (entries.HasKey(key.c_str())) {
			SetValue
		} else {
			SetValue *s = new SetValue();
			s->Push(page_num);
			entries.push(key.c_str(), );
		}
	}

	for (auto const& [key, val] : entries)
	{
	    // *** val is a vector<int>.. needs to be sorted and remove doubles
	    
	    // *** construct:
	    //  key 1, 2, 3   <- convert number to page labels
	}
}

/*! For each object in a document, if it has an external resource like a font or a gradient,
 * then copy them to a new directory.
 *
 * Optionally relink everything to use the repositioned items.
 * 
 * If layout or spread is < 0, then do all.
 */
int CollectForOut(const char *output_dir, Document *doc, int layout, int spread, Attribute *map_ret)
{
	//resources
	//  nodes
	//project
	//  papergroups  <-- need to move to resources
	//  textobjects  <-- need to move to resources
	//  limbos
	//    objects
	//  docs
	//    pages
	//      layers
	//        objects
	
	ObjectIterator iterator;
	Laxkit::PtrStack<FindResult> results;
	iterator.FindAll(MatchIfExternalResources, results);

	for (int c = 0; c < results.n; c++) {
		***
	}
}



} // namespace Laidout

