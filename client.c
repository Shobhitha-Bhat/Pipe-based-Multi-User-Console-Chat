/*
=================
Required header files..
=================

=====================
a structure definition containing client info like - username , 
                                          pipe names it wants the server to create 
                                          any extra info
=====================



function1(){
            while(true){
                read( fd2,  " the content that server broadcasted")

                // sleep(1)
            }
}



main{

i) ask user for client name

ii) put that info the struct

iii) open regpipe and notify itself to the server
iv) wait till it gets acknowledgement from server to proceed with chatting

once acknowledged,

    int fd1 = open( client to server pipe )
    int fd2 = open(server to client pipe)

v)  pthread( read from server for other client messages , function1())

vi) loop(){
       
        fgets( get input from stdin )

        if( strcmp(msg == "/exit") ){
        write( fd1, " client is leaving ")
        close(client to server pipe )
        close(server to client pipe)
        unlink(client to server pipe )
        unlink(server to client pipe)

        exit(0)
        }

        write(fd1, " the text that user entered")                
                    
}

} 


*/