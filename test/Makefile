CFLAGS=-mclwb -mclflushopt -Wall -pthread #-DRAM_VER
CDEBBUG=-g
CRELEASE=-O3
BLDDIR=../bld

.PHONY: default
default: mkfs

mkfs: $(BLDDIR)/ctfs.a mkfs.o
	cd .. && make
	gcc $(CFLAGS) $(BLDDIR)/mkfs.o $(BLDDIR)/ctfs.a -o mkfs

parallel: $(BLDDIR)/libctfs.so parallel.o
	cd .. && make
	gcc $(CFLAGS) $(BLDDIR)/parallel.o -o parallel

pswap_test: $(BLDDIR)/ctfs.a pswap_test.o
	cd .. && make
	gcc $(CFLAGS) $(BLDDIR)/pswap_test.o $(BLDDIR)/ctfs.a -o pswap_test

qainit:
	rm testfile
	rm -rf testfolder
	rm -rf testfolder2
	rm -rf renamedfolder

qatest: $(BLDDIR)/ctfs.a qatest.o
	gcc -c $(CFLAGS) qatest.c -o $(BLDDIR)/qatest.o
	cd .. && make
	gcc $(CFLAGS) $(BLDDIR)/qatest.o $(BLDDIR)/ctfs.a -o qatest

mkfs.o: mkfs.c
	gcc -c $(CFLAGS) mkfs.c -o $(BLDDIR)/mkfs.o



parallel.o: parallel.c
	gcc -c $(CFLAGS) parallel.c -o $(BLDDIR)/parallel.o

pswap_test.o: pswap_test.c
	gcc -c $(CFLAGS) pswap_test.c -o $(BLDDIR)/pswap_test.o

# fstest: $(BLDDIR)/ctfs.a fstest.o
# 	cd .. && make
# 	gcc $(CFLAGS) $(BLDDIR)/fstest.o $(BLDDIR)/ctfs.a -o fstest

fstest.o: fstest.c
	gcc -c $(CFLAGS) fstest.c -o $(BLDDIR)/fstest.o

fstest_dbg: fstest.o
	gcc $(CFLAGS) $(CDEBBUG) $(BLDDIR)/fstest.o -o fstest

fstest: fstest.o
	gcc $(CFLAGS) $(CRELEASE) $(BLDDIR)/fstest.o -o fstest

# qatest.o: qatest.c
# 	gcc -c $(CFLAGS) qatest.c -o $(BLDDIR)/qatest.o