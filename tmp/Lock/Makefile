DIR_INC = inc
DIR_SRC = usr
DIR_OBJ = obj
TARGET  = SystemOp

SRC = $(wildcard ${DIR_SRC}/*.c)
OBJ = $(patsubst %.c,${DIR_OBJ}/%.o,$(notdir ${SRC}))

#CC      = arm-none-linux-gnueabi-gcc
CC      = gcc
CFLAGS  = -g -Wall -I${DIR_INC}

${TARGET}:${OBJ}
	$(CC) $(OBJ) -o $@
     
${DIR_OBJ}/%.o:${DIR_SRC}/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf ${TARGET}
	rm -rf ${DIR_OBJ}/*
	find -name *~ | xargs rm -rf
