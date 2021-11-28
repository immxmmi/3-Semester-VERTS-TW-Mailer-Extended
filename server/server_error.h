void server_error(int error){
    
    switch (error)
    {
    case 1:  perror("SERVER - SOCKET - CREATE ERROR"); break;
    case 2:  perror("SERVER - SOCKET - BIND ERROR"); break;
    case 3:  perror("SERVER - SOCKET - LISTEN ERROR");break;
    case 4:  perror("SERVER - SOCKET - ACCEPT ERROR");break;
    case 5:  perror("SERVER - SOCKET - ACCEPT AFTER ABORTED ERROR");break;
    default: perror("SERVER - UNKOWN - ERROR");break;
    }

}