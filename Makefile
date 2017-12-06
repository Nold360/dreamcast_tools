# build all

DIRS = 	cdirip \
		scramble \
		nerorip \
		raw2wav \
		makeip \
		isofix \
		binhack \
		img4dc/cdi4dc \
		img4dc/lbacalc \
		img4dc/libedc/src \
		img4dc/mds4dc \
		gdiopt \
		ciso

all:
	for i in $(DIRS); do cd $$i; make ; cd .. || true; done

clean:
	for i in $(DIRS); do make -C $$i clean || true; done

install:
	for i in $(DIRS); do make -C $$i install || true; done
