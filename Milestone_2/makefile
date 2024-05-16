CC     = gcc
FLAGS  = -Wno-unused-command-line-argument -Wall -Wextra -g -std=c99 -D_XOPEN_SOURCE=600 -lpthread -D_REENTRANT

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	FLAGS += -lrt
endif

# Common header files
COMMON_HEADERS = structs.h utils.h

# Specific header files for each source file
HEADERS_1 = sysmanager.h monitorengine.h authmanager.h logs.h $(COMMON_HEADERS)
HEADERS_2 = $(COMMON_HEADERS)
HEADERS_3 = $(COMMON_HEADERS)
HEADERS_4 = monitorengine.h logs.h $(COMMON_HEADERS)
HEADERS_5 = authmanager.h authengine.h logs.h $(COMMON_HEADERS)
HEADERS_6 = authengine.h logs.h $(COMMON_HEADERS)

all: 5g_auth_platform mobile_user backoffice_user

clean:
	rm -f 5g_auth_platform mobile_user backoffice_user *.o *~

5g_auth_platform: sysmanager.o monitorengine.o authmanager.o authengine.o utils.o logs.o
	$(CC) $(FLAGS) $^ -o $@

mobile_user: mobile_user.o utils.o
	$(CC) $(FLAGS) $^ -o $@

backoffice_user: backoffice_user.o utils.o
	$(CC) $(FLAGS) $^ -o $@

sysmanager.o: sysmanager.c $(HEADERS_1)
	$(CC) $(FLAGS) -c $< -o $@

mobile_user.o: mobile_user.c $(HEADERS_2)
	$(CC) $(FLAGS) -c $< -o $@

backoffice_user.o: backoffice_user.c $(HEADERS_3)
	$(CC) $(FLAGS) -c $< -o $@

monitorengine.o: monitorengine.c $(HEADERS_4)
	$(CC) $(FLAGS) -c $< -o $@

authmanager.o: authmanager.c $(HEADERS_5)
	$(CC) $(FLAGS) -c $< -o $@

authengine.o: authengine.c $(HEADERS_6)
	$(CC) $(FLAGS) -c $< -o $@

utils.o: utils.c $(COMMON_HEADERS)
	$(CC) $(FLAGS) -c $< -o $@

logs.o: logs.c $(COMMON_HEADERS)
	$(CC) $(FLAGS) -c $< -o $@