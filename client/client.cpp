#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <iostream>
#include "client_error.h"
#include <cstring>
#include <string>
#include <stdio.h>
#include <ctype.h>
//#################################
#define BUF 1024
//#################################
using namespace std;

// SOCKET --> CLIENT
int client_socket;
char buffer[BUF];
char username[10];

void printBuffer(char *buff);
void sendToServer(int current_client_socket);
void LOGIN();
void SEND();
void LOGIN(char *buffer);
void LIST(char *buffer);
void DELETE(char *buffer);
void READ(char *buffer);
void QUIT();


//                [IP] [PORT]
int main(int argc, char **argv)
{
   int size;

// MELDUNG WENN ZU WENIGE ARGUMENTE ÜBERGEBEN WERDEN
   if (argc < 2 || atoi(argv[1]) == 0){fprintf(stderr, "\nUsage: ./client <ip> <port>\n");return EXIT_FAILURE;}
//PORT --> [PORT]
   int PORT = atoi(argv[2]);
  // int IP = atoi(argv[1]);
// VAR
   int isQuit;
   struct sockaddr_in address;
//##########################################



////////////////////////////////////////////////////////////////////////////
//--> Socket - #########################################################
////////////////////////////////////////////////////////////////////////////
// CREATE A SOCKET
//--> Socket - CREATE ################ (IPV4 - TCP) server_error(1)
   if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){client_error(1);return EXIT_FAILURE; }
//##########################################


////////////////////////////////////////////////////////////////////////////
// INIT ADDRESS
   memset(&address, 0, sizeof(address));  
   address.sin_family = AF_INET;
  // address.sin_addr.s_addr = INADDR_ANY; // IP EINBINDEN
  inet_aton(argv[1],&address.sin_addr);
  address.sin_port = htons(PORT);
//##########################################


////////////////////////////////////////////////////////////////////////////
// CREATE A CONNECTION
//--> SOCKET - CONNECT #####################
   if (connect(client_socket, (struct sockaddr *)&address, sizeof(address)) == -1){client_error(2);return EXIT_FAILURE;}
    // CONNECT - MSG
   printf("Connection with server (%s) established\n", inet_ntoa(address.sin_addr));
//##########################################


////////////////////////////////////////////////////////////////////////////
// RECEIVE DATA
   size = recv(client_socket, buffer, BUF - 1, 0);
   if (size == -1){client_error(3);}else if (size == 0){client_error(4);}
   else{buffer[size] = '\0';printf("%s", buffer);}
//##########################################



do{
     printf("\nCommand >>  ");

      if (fgets(buffer, BUF, stdin) != NULL){      
         int size = strlen(buffer);
         // remove new-line signs from string at the end
         if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n'){size -= 2;buffer[size] = 0;}
         else if (buffer[size - 1] == '\n'){--size;buffer[size] = 0;}
         isQuit = strcmp(buffer, "quit") == 0;
         if(strlen(buffer) <=0){

             std::strcpy(buffer,"*empty*;");
         }else{
             std::strcat(buffer,";");
         }


         //////////////////////////////////////////////////////////////////////
   // SEND DATA

          if(strcmp(buffer,"login;") == 0) {
             LOGIN(buffer);
          }else if(strcmp(buffer,"send;") == 0){
              SEND();
          }else if(strcmp(buffer,"list;") == 0){
              LIST(buffer);
          }else if(strcmp(buffer,"read;") == 0){
              READ(buffer);
          }else if(strcmp(buffer,"delete;") == 0){
              DELETE(buffer);
          }
          sendToServer(client_socket);

          //////////////////////////////////////////////////////////////////////
   // RECEIVE FEEDBACK
   size = recv(client_socket, buffer, BUF - 1, 0);if (size == -1){perror("recv error");break;}
         else if (size == 0)
         {
            printf("Server closed remote socket\n"); // ignore error
            break;
         }
         else{
            buffer[size] = '\0';
          //  printf("<< %s\n", buffer); // ignore error

              printBuffer(buffer);
          //  if (strcmp("OK", buffer) != 0){fprintf(stderr, "<< Server error occured, abort\n");break;}
         }
      }



}while(!isQuit);

   QUIT();
   return EXIT_SUCCESS;
}


void sendToServer(int current_client_socket){
    int size = strlen(buffer);
    if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n'){size -= 2;buffer[size] = 0;}
    else if (buffer[size - 1] == '\n'){--size;buffer[size] = 0;}

    if(size <=0){
        sprintf(buffer,"EMPTY;test");
    }

    if ((send(current_client_socket, buffer, BUF, 0)) == -1){perror("send error");}
    memset(buffer,0,BUF);
}
void printBuffer(char *buff){
    char *currentBuffer = (char *)malloc(strlen(buff) * sizeof(char));
    char *line;
    int line_counter = 0;
    strcpy(currentBuffer, buff);
    line = strtok(currentBuffer, ";");

    while(line != NULL){
        printf("%d: %s\n",line_counter,line);
        line = strtok(NULL, ";");
        line_counter++;
    }

}


string getUsername(){
    string text;
    bool check = true;
    while(true){
        std::getline (std::cin,text);
        if(text.length() > 9 || text.length() < 1) {std::cout<< "ONLY (a-z) (0-9) - TRY AGAIN \n:"; check = false; continue;}

        for(char c : text){
            if(!isalnum(c)) {std::cout<< "ONLY (a-z) (0-9) - TRY AGAIN \n:"; check = false; break;}
            if(isalpha(c)){if(isupper(c)) {std::cout<< "ONLY (a-z) (0-9) - TRY AGAIN \n:"; check = false; break;}}
        }

        if(check == true){
            break;
        }else{
            check = true;
        }
    };

    return text;
}

string getNumber(){
    string text;
    bool check = true;
    while(true){
        std::getline (std::cin,text);

        if(text.length() < 1) {std::cout<< "ONLY (0-9) - TRY AGAIN \n:"; check = false; continue;}
        for(char c : text){
            if(!isdigit(c)) {std::cout<< "ONLY (0-9) - TRY AGAIN \n:"; check = false; break;}
        }

        if(check == true){
            break;
        }else{
            check = true;
        }
    };

    return text;
}


void LOGIN(char *buffer){
    char username[10];
    char password[30];
    string text;
    std::cout << "Username: ";
    text = getUsername();
    std::strcpy(username,text.c_str());
    strcat(buffer,username);
    strcat(buffer,";");
    std::cout << "Password: ";
    std::cin.getline(password,30);
    strcat(buffer,password);

}
void SEND(){

    char temp_msg[BUF];
    string text;
    //std::strcat(buffer,"send");
    std::cout << "SENDER: ";
    text = getUsername();
    std::strcpy(temp_msg,text.c_str());

    std::strcat(buffer,temp_msg);
    std::strcat(buffer,";");
    std::cout << "Receiver: ";

    text = getUsername();
    std::strcpy(temp_msg,text.c_str());

    std::strcat(buffer,temp_msg);
    std::strcat(buffer,";");


        std::cout << "Subject: ";
    while(true){
        std::cin.getline(temp_msg,80);
        if(strlen(temp_msg)>0){break;};
    }
    std::strcat(buffer,temp_msg);
    std::strcat(buffer,";");

    std::cout << "MSG: ";
    while(true){
        std::cin.getline(temp_msg,BUF - 400);
        if(strlen(temp_msg)>0){break;};
    }

    std::strcat(buffer,temp_msg);

}
void LIST(char *buffer){
    char username[10];
    string text;
    std::cout << "Username: ";
    text = getUsername();
    std::strcpy(username,text.c_str());
    strcat(buffer,username);
}
void READ(char *buffer){
    char username[10];
    char number[4];
    string text;

    std::cout << "Username: ";
    text = getUsername();
    std::strcpy(username,text.c_str());
    strcat(buffer,username);
    strcat(buffer,";");

    std::cout << "MSG-Number: ";
    text = getNumber();
    std::strcpy(number,text.c_str());
    strcat(buffer,number);
}
void DELETE(char *buffer){
    char username[10];
    char number[4];
    string text;

    std::cout << "Username: ";
    text = getUsername();
    std::strcpy(username,text.c_str());
    strcat(buffer,username);

    strcat(buffer,";");
    std::cout << "MSG-Number: ";
    std::cin.getline(number,4);
    strcat(buffer,number);
}
void QUIT(){
////////////////////////////////////////////////////////////////////////////
    // CLOSES THE DESCRIPTOR
    if (client_socket != -1)
    {
        if (shutdown(client_socket, SHUT_RDWR) == -1)
        {
            // invalid in case the server is gone already
            perror("shutdown create_socket");
        }
        if (close(client_socket) == -1)
        {
            perror("close create_socket");
        }
        client_socket = -1;
    }

}