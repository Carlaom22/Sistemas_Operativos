CC     = gcc
FLAGS  = -g

PROG1  = exe/5g_auth_platform
PROG2  = exe/mobile_user
PROG3  = exe/backoffice_user
PROGS = ${PROG1} ${PROG2} ${PROG3}

OBJS1  = exe/sysmanager.o exe/authmanager.o exe/monitorengine.o exe/logs.o exe/signals.o
OBJS2 = exe/mobile_user.o exe/signals.o
OBJS3  = exe/backoffice_user.o exe/signals.o

OBJSSYS = exe/authmanager.o exe/monitorengine.o
OBJSOTHER = exe/logs.o exe/signals.o

HEAD_BASIC = src/main.h src/signals.h
HEAD_SYS = src/authmanager.h src/monitorengine.h
HEAD_OTHER  = src/sharedm.h src/sysmanager.h src/logs.h

all: ${PROG1} ${PROG2} ${PROG3}

clean:
	rm -f ${PROGS} exe/*.o *~

${PROG1}: ${OBJS1}
	${CC} ${FLAGS} ${OBJS1} -o $@

${PROG2}: ${OBJS2}
	${CC} ${FLAGS} ${OBJS2} -o $@

${PROG3}: ${OBJS3}
	${CC} ${FLAGS} ${OBJS3} -o $@

exe/sysmanager.o: src/sysmanager.c src/sysmanager.h ${OBJSSYS} ${OBJSOTHER} ${HEAD_BASIC} ${HEAD_SYS} ${HEAD_OTHER}
	${CC} ${FLAGS} -c src/sysmanager.c -o exe/sysmanager.o

exe/authmanager.o: src/authmanager.c src/authmanager.h ${OBJSOTHER} ${HEAD_BASIC} ${HEAD_SYS} ${HEAD_OTHER}
	${CC} ${FLAGS} -c src/authmanager.c -o exe/authmanager.o

exe/monitorengine.o: src/monitorengine.c src/monitorengine.h ${OBJSOTHER} ${HEAD_BASIC} ${HEAD_SYS} ${HEAD_OTHER}
	${CC} ${FLAGS} -c src/monitorengine.c -o exe/monitorengine.o
	
exe/%.o: src/%.c src/%.h ${HEAD_BASIC}
	${CC} ${FLAGS} -c $< -o $@

################


