# To regenerate the icons, remove icons.stamp from this 
# directory and type 'make'.


LAXICONS=../laxkit/lax/icons/icons.svg
ifeq (,$(wildcard $(LAXICONS)))
LAXICONS=../../laxkit/lax/icons/icons.svg
endif


all:
	@if [ ! -e icons.stamp ] ; then \
			echo "Generating icons..."; \
			 ./makeimages.py 24 --file icons.svg 100 --file icons-tiling.svg && touch icons.stamp; \
		else echo "Icons already made! Remove icons.stamp and run make to redo."; fi
		
all-with-lax:
	@echo "Using Laxkit icons file: $(LAXICONS)"
	@if [ ! -f $(LAXICONS) ] ; then echo 'Missing Laxkit icons file!'; false; fi
	echo "Generating icons including Laxkit icons...";
	./makeimages.py 24 --file icons.svg --file $(LAXICONS) 100 --file icons-tiling.svg && touch icons.stamp; \

laidout-icons:
	if [ ! -d laidout-icons ] ; then mkdir laidout-icons; fi 
	cp -f *png *svg laidout-icons 
	zip -r laidout-icons.zip laidout-icons


.PHONY: laidout-icons

