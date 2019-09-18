SRC = main.c FlashMem.c unity\unity.c
SRC += ..\src\EELS.c

OBJ = main.o FlashMem.o EELS.o unity.o
ODIR = .\build
Define = -D bool=_Bool -D FSIZE=0x40000 -D UNITY_OUTPUT_COLOR

INC = -I"unity" -I"..\src" -I"."


ARGS= $(Define) $(INC) -std=c99

run: clean $(OBJ)
	gcc -o $(ODIR)\test.exe $(OBJ) $(ARGS)
	cls
	$(ODIR)\test.exe

build: clean $(OBJ)
	gcc -o $(ODIR)\test.exe $(OBJ) $(ARGS)

test: $(ODIR)\test.exe
	cls
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
