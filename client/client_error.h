void client_error(int error){
    
    switch (error)
    {
    case 1:  perror("CLIENT - SOCKET - CREATE ERROR"); break;
    case 2:  perror("CLIENT - CONNECT - NO SERVER ERROR"); break;
    case 3:  perror("CLIENT - RECV - ERROR"); break;
    case 4:  perror("CLIENT - REMOTE  - SERVER CLOSED"); break;



    default: perror("CLIENT - UNKOWN - ERROR");break;
    }

}