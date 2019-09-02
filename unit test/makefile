SRC = main.c FlashMem.c EELS.c unity\unity.c app\communication\crc32.c
OBJ = main.o FlashMem.o EELS.o unity.o crc32.o
ODIR = .\build
Define = -D bool=_Bool -D FSIZE=0x40000
INC = -I"unity"


ARGS= $(Define) $(INC) -std=c99

compile: clean $(OBJ)
	gcc -o $(ODIR)\test.exe $(OBJ) $(ARGS)
	cls
	$(ODIR)\test.exe

buildonly: clean $(OBJ)
	gcc -o $(ODIR)\test.exe $(OBJ) $(ARGS)

testonly: $(ODIR)\test.exe
	$(ODIR)\test.exe


$(OBJ): $(SRC)
	gcc -c $^ $(ARGS)

.PHONY: cleanall cleanobj cleandiff clean

clean: cleanall

cleanall : cleanobj cleandiff
	del build\*.exe

cleanobj :
	del *.o

cleandiff :
	del *.diff