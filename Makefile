CXX = gcc
SHARE_FLAG = -shared -fPIC
EXE_NAME = logger
MAIN_FILE = hw2.c
SO_NAME = logger.so

all: so
	$(CXX) -o $(EXE_NAME) -g -Wall $(MAIN_FILE)

so: logger.c
	$(CXX) -o $(SO_NAME) $(SHARE_FLAG) logger.c -ldl

clean:
	rm -rf $(EXE_NAME)
	rm -rf $(SO_NAME)