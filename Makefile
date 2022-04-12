
# Author: Pythagore Guekam
ALL:=testBluetooth \
		testBluetooth_dbg \
		testBluetooth_dbg_msan \
		testBluetooth_dbg_asan \
		testBluetooth_dbg_ub \
		highLevel_ble_func

CC=${CROSS_COMPILE}gcc
CL=${CROSS_COMPILE}clang

CFLAGS= -O2 -UDEBUG -Wall
CFLAGS_DBG= -g -ggdb -gdwarf-4 -O0 -Wall -Wextra -DDEBUG
CFLAGS_DBG_ASAN= ${CFLAGS_DBG} -fsanitize=address
CFLAGS_DBG_MSAN= ${CFLAGS_DBG} -fsanitize=memory 
CFLAGS_DBG_UB= ${CFLAGS_DBG} -fsanitize=undefined
LINKIN = -lbluetooth -lcap
all: ${ALL}
CB_FILES:= *.[ch]


highLevel_ble_func.o: highLevel_ble_func.c highLevel_ble_func.h
	${CC} ${CFLAGS} -c highLevel_ble_func.c -o highLevel_ble_func.o	
highLevel_ble_func_dbg.o: highLevel_ble_func.c highLevel_ble_func.h
	${CC} ${CFLAGS_DBG} -c highLevel_ble_func.c -o highLevel_ble_func_dbg.o	
	
testBluetooth: testBluetooth.o highLevel_ble_func.o
	${CC} ${CFLAGS} -o testBluetooth testBluetooth.o highLevel_ble_func.o ${LINKIN}
testBluetooth.o: testBluetooth.c testBluetooth.h highLevel_ble_func.h
	${CC} ${CFLAGS} -c testBluetooth.c -o testBluetooth.o
	
testBluetooth_dbg: testBluetooth_dbg.o highLevel_ble_func_dbg.o
	${CC} ${CFLAGS_DBG} -o testBluetooth_dbg testBluetooth_dbg.o highLevel_ble_func_dbg.o ${LINKIN}
testBluetooth_dbg.o: testBluetooth.c testBluetooth.h highLevel_ble_func.h
	${CC} ${CFLAGS_DBG} -c testBluetooth.c -o testBluetooth_dbg.o


highLevel_ble_func_dbg_asan.o: highLevel_ble_func.c highLevel_ble_func.h
	${CL} ${CFLAGS_DBG_ASAN} -c highLevel_ble_func.c -o highLevel_ble_func_dbg_asan.o
highLevel_ble_func_dbg_msan.o: highLevel_ble_func.c highLevel_ble_func.h
	${CL} ${CFLAGS_DBG_MSAN} -c highLevel_ble_func.c -o highLevel_ble_func_dbg_msan.o
highLevel_ble_func_dbg_ub.o: highLevel_ble_func.c highLevel_ble_func.h
	${CL} ${CFLAGS_DBG_UB} -c highLevel_ble_func.c -o highLevel_ble_func_dbg_ub.o
	
testBluetooth_dbg_asan: testBluetooth_dbg_asan.o highLevel_ble_func_dbg_asan.o
	${CL} ${CFLAGS_DBG_ASAN} -o testBluetooth_dbg_asan testBluetooth_dbg_asan.o ${LINKIN}
testBluetooth_dbg_asan.o: testBluetooth.c testBluetooth.h
	${CL} ${CFLAGS_DBG_ASAN} -c testBluetooth.c -o testBluetooth_dbg_asan.o
	
testBluetooth_dbg_msan: testBluetooth_dbg_msan.o highLevel_ble_func_dbg_msan.o
	${CL} ${CFLAGS_DBG_MSAN} -o testBluetooth_dbg_msan testBluetooth_dbg_msan.o ${LINKIN}
testBluetooth_dbg_msan.o: testBluetooth.c testBluetooth.h highLevel_ble_func.h
	${CL} ${CFLAGS_DBG_MSAN} -c testBluetooth.c -o testBluetooth_dbg_msan.o
		
testBluetooth_dbg_ub: testBluetooth_dbg_ub.o highLevel_ble_func_dbg_ub.o
	${CL} ${CFLAGS_DBG_UB} -o testBluetooth_dbg_ub testBluetooth_dbg_ub.o ${LINKIN}
testBluetooth_dbg_ub.o: testBluetooth.c testBluetooth.h highLevel_ble_func.h
	${CL} ${CFLAGS_DBG_UB} -c testBluetooth.c -o testBluetooth_dbg_ub.o
	
	
cb: ${CB_FILES}
	mkdir bkp 2> /dev/null; cp -f ${CB_FILES} bkp/
	indent -linux ${CB_FILES}
	
clean: 
	rm -vf ${ALL} *.o *~
