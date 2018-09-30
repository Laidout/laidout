
include ../../../Makefile-toinclude

#LAXDIR=/home/tom/other/p/github/laxkit/lax


CPPFLAGS = -g -I.  -I.. -I$(LAXDIR)/.. -L$(LAXDIR)
#LDFLAGS = `imlib2-config --libs`

dtpcompare: withlax
	g++ -o dtpcompare $(CPPFLAGS) dtpcompare.cc -L/usr/X11R6/lib -llaxkit -lcairo `imlib2-config --libs` -lsqlite3 -lX11 -lXext -lXft -lXi -lcrypto -lfontconfig -lfreetype


withlax: 
	cd $(LAXDIR) && $(MAKE) 
	cd $(LAXDIR)/interfaces && $(MAKE) 




.PHONY: clean withlax
clean:
	rm calc $(objs)
