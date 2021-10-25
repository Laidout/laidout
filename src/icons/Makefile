# To regenerate the icons, remove icons.stamp from this 
# directory and type 'make all' or if Laxkit is at ../../laxkit, then 'make all-with-lax'.

# pixel sizes of icons. The 100 is for the tiling icons that are extra wide.
DEFAULT_ICON_SIZE=24
DEFAULT_APP_ICON_SIZE=48
DEFAULT_WIDE_ICON=100

include ../../Makefile-toinclude

LAXICONS=$(LAXDIR)/icons/icons.svg

#LAXICONS=../laxkit/lax/icons/icons.svg
#ifeq (,$(wildcard $(LAXICONS)))
#LAXICONS=../../laxkit/lax/icons/icons.svg
#endif


all:
	@if [ ! -e icons.stamp ] ; then \
			echo "Generating icons..."; \
			 ./makeimages.py $(DEFAULT_ICON_SIZE) --file icons.svg $(DEFAULT_WIDE_ICON) --file icons-tiling.svg && touch icons.stamp; \
			 ../laidout -e "format=image width=$(DEFAULT_APP_ICON_SIZE) height=$(DEFAULT_APP_ICON_SIZE) tofile=laidout.png" laidout.svg; \
		else echo "Icons already made! Remove icons.stamp and run make to redo."; fi
		
all-with-lax:
	@echo "Using Laxkit icons file: $(LAXICONS)"
	@if [ ! -f $(LAXICONS) ] ; then echo 'Missing Laxkit icons file!'; false; fi
	echo "Generating icons including Laxkit icons...";
	./makeimages.py $(DEFAULT_ICON_SIZE) --file $(LAXICONS) --file icons.svg $(DEFAULT_WIDE_ICON) --file icons-tiling.svg && touch icons.stamp;
	../laidout -e "format=image width=$(DEFAULT_APP_ICON_SIZE) height=$(DEFAULT_APP_ICON_SIZE) tofile=laidout.png" laidout.svg;

laidout-icon:
	../laidout -e "format=image width=$(DEFAULT_APP_ICON_SIZE) height=$(DEFAULT_APP_ICON_SIZE) tofile=laidout.png" laidout.svg;

# make zip of the icons
laidout-icons:
	if [ ! -d laidout-icons ] ; then mkdir laidout-icons; fi 
	cp -f *png *svg laidout-icons 
	zip -r laidout-icons.zip laidout-icons


clean-icons:
	rm -f *png

.PHONY: laidout-icons clean-icons

