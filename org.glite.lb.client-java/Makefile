-include Makefile.inc

all compile:
	ant -Dno.deps=yes -Dreference.jobid-api-java.jar=${stagedir}/share/java/jobid-api-java.jar -DstageDir=${stagedir} 
	cd src_c && make PREFIX=${PREFIX}

check:
	@echo "No check"

install:
	mkdir -p ${PREFIX}/share/java
	cp dist/lb-client-java.jar ${PREFIX}/share/java
	cd src_c && make install PREFIX=${PREFIX}

clean:
	echo TODO


