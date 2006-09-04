#
#    $Id: Makefile 34 2006-04-29 22:33:20Z ajw $
#

all:
	mkdir -p Apps/Misc
	cp -r ../!hmail Apps/Misc
	perl CreatePackage
	zip -r -9 -I ^.hmail-pkg/zip Apps RiscPkg Sprites SysVars -x *svn*

.PHONY: clean all


clean:
	-rm -rf ../hmail-pkg.zip Apps RiscPkg/Control
