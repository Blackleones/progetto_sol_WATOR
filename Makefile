CC = gcc
CFLAG = -Wall -pedantic -g
CFLAGPTHREAD = -pthread

#librerie ../lib
LIBDIR = lib

#operazione linking
LIBS= -L $(LIBDIR)

#nome librerie
LIBNAME1 = libWator.a
LIBNAME2 = libQueue.a
LIBNAME3 = libThreadPool.a
LIBNAME4 = libFinal.a

#sorgenti delle librerie
object1 = wator_planet.o wator_wator.o wator_animals.o wator_util.o
object2 = queue.o
object3 = threadPool.o threadPool_init.o

#creazione librerie
lib: $(object1)
	-mkdir -p lib
	-rm -f $(LIBNAME1)
	-rm -f $(LIBDIR)/$(LIBNAME1)
	ar -r $(LIBNAME1) $(object1)
	cp $(LIBNAME1) $(LIBDIR)
	-rm -f $(LIBNAME1)
	make lib2

lib2: $(object2)
	-mkdir -p lib
	-rm -f $(LIBNAME2)
	-rm -f $(LIBDIR)/$(LIBNAME2)
	ar -r $(LIBNAME2) $(object2)
	cp $(LIBNAME2) $(LIBDIR)
	-rm -f $(LIBNAME2)
	make lib3

lib3: $(object3)
	-mkdir -p lib
	-rm -f $(LIBNAME3)
	-rm -f $(LIBDIR)/$(LIBNAME3)
	ar -r $(LIBNAME3) $(object3)
	cp $(LIBNAME3) $(LIBDIR)
	-rm -f $(LIBNAME3)
	-rm -f *.o
	ar -x $(LIBDIR)/$(LIBNAME1)
	ar -x $(LIBDIR)/$(LIBNAME2)
	ar -x $(LIBDIR)/$(LIBNAME3)
	ar -qc $(LIBDIR)/$(LIBNAME4) *.o

#comandi
build:
	make lib
	make wator
	make clean
	
clean:
	-rm -f *.o

#generazione eseguibili
wator: watorprocess.o
	$(CC) -o $@ $^ $(LIBS) -lFinal -lpthread

wator.o: watorprocess.c
	$(CC) $(CFLAG) $(CFLAGPTHREAD) -c $<

threadPool.o: threadPool.c
	$(CC) $(CFLAG) $(CFLAGPTHREAD) -c $<

threadPool_init.o: threadPool_init.c
	$(CC) $(CFLAG) $(CFLAGPTHREAD) -c $<