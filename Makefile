default_target: all

all: server client

server: Server/main.c
	@echo "Building ServerApp"
	gcc -o ServerApp Server/main.c Server/users.c -pthread -I.
	@echo "Building ServerApp Done"
.PHONY : server		# Pri rovnakom názve súboru ako volaná funkcia make zabráni použitiu názvu súboru => nevykonaného make

client: Client/main.c
	@echo "Building ClientApp"
	gcc -o ClientApp Client/main.c Client/operations.c
	@echo "Building ClientApp Done"
.PHONY : client

clean:
	rm -f ServerApp, ClientApp
.PHONY : clean

clean-server:
	rm -f ServerApp
.PHONY : clean-server

clean-client:
	rm -f ClientApp
.PHONY : clean-client

help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... server"
	@echo "... client"
	@echo "... clean"
	@echo "... clean-server"
	@echo "... clean-client"
.PHONY : help


