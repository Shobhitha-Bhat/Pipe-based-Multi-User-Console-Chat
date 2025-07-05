/*
=================
Required header files..
=================

=====================
a structure definition containing client info like - username , 
                                          pipe names it wants the server to create 
                                          any extra info
=====================

main{

ask user for client name

put that info the struct

open regpipe and notify itself
wait till it gets acknowledgement from server to proceed with chatting

once acknowledged,
loop(){
    client can have 2 possibilities:
                   -> write to server ie, chat with others
                    ->read from the server ie., read messages of others
                    
}

} 
*/