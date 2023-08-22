# *********************************************************************
# Matricole : VR456187  -  VR422276  -  VR456008
# Nomi e Cognomi : Oualid Maftah  -  Valeria Stuani  -  Stefano Zanolli
# Data realizzazione : 02/05/2023
# *********************************************************************

.SILENT:

F4Server_obj:
	gcc -g3 -c -Wall -Wextra F4Server.c -o F4Server.obj
F4Server_functions_obj: F4Server_obj
	gcc -g3 -c -Wall -Wextra F4Server_functions.c -o F4Server_functions.obj
F4Server_exe: F4Server_functions_obj
	gcc -g3 -Wall -Wextra F4Server.obj F4Server_functions.obj -o F4Server
	echo "F4Server compilato"

F4Client_obj:
	gcc -g3 -c -Wall -Wextra F4Client.c -o F4Client.obj
F4Client_functions_obj: F4Client_obj
	gcc -g3 -c -Wall -Wextra F4Client_functions.c -o F4Client_functions.obj
F4Client_exe: F4Client_functions_obj
	gcc -g3 -Wall -Wextra F4Client.obj F4Client_functions.obj -o F4Client
	echo "F4Client compilato"

all: F4Server_exe F4Client_exe
	ipcrm -a
