FLAGS = -O4 -Wall

LIBS = -lntl -lgmp

CXX = g++


objects = elemQuo.o resultantsumroots.o ZZ_pEJac2F.o ZZ_pXResultant.o elltorsion.o schoof.o ZZ_pJac2.o fastinterp.o vec_pair_long_long.o ZZ_pJac2F.o ZZ_pEJac2.o ZZ_pXCantorpoly.o

all: $(objects) main LMPMCT CRT InvMod

$(objects): %.o: %.c %.h
	$(CXX) -c $(FLAGS) $< -o $@

main: main.c $(objects)
	$(CXX) -o main main.c $(objects) $(FLAGS) $(LIBS) 

LMPMCT: LMPMCT.c $(objects)
	$(CXX) -o LMPMCT LMPMCT.c $(objects) $(FLAGS) $(LIBS)

CRT: CRT.c
	$(CXX) -o CRT CRT.c $(FLAGS) $(LIBS)

InvMod: InvMod.c
	$(CXX) -o InvMod InvMod.c $(FLAGS) $(LIBS)

clean:
	rm *.o main LMPMCT CRT InvMod

