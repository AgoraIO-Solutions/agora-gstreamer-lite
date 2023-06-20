
CC = g++  # C compiler
INCLUDE = -I${AGORA_SDK_DIR}/agora_sdk/include 
CFLAGS = -fPIC -g
CPPFLAGS       = -fPIC -std=c++11 -g $(INCLUDE)
LDFLAGS      = -shared


TARGET_LIB = libagora_rtsac.so
DESTDIR?=/usr/local/lib/lite/

OBJS = agoraio.o agorac.o agoralog.o syncbuffer.o utilities.o

all: ${TARGET_LIB}
	@echo ${AGORA_SDK_DIR}

$(TARGET_LIB): ${OBJS}
	@echo "what?"
	@echo ${AGORA_SDK_DIR}
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDE) ${LDFLAGS} -o $@ ${OBJS} 

clean:
	sudo rm $(DESTDIR)/$(TARGET_LIB)

install:
	mv $(TARGET_LIB) ${DESTDIR}


