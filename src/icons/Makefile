# To regenerate the icons, remove icons.stamp from this 
# directory and type 'make'.

all:
	@if [ ! -e icons.stamp ] ; then \
			echo "Generating icons..."; \
			 ./makeimages.py; \
			 ./makeimages.py -icons-tiling.svg 100; \
			touch icons.stamp; \
		else echo "Icons already made! Remove icons.stamp and run make to redo."; fi
		

laidout-icons:
	if [ ! -d laidout-icons ] ; then mkdir laidout-icons; fi 
	cp -f *png *svg laidout-icons 
	zip -r laidout-icons.zip laidout-icons


.PHONY: laidout-icons

