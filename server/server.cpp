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
#include "server_error.h"
#include <fstream>
#include<string>
#include <filesystem>
#include <iostream>
#include <vector>

using namespace std;
namespace fs = filesystem;
using std::filesystem::directory_iterator;
using std::cout; using std::cerr;
using std::endl; using std::string;
using std::ifstream; using std::vector;

//#################################
#define BUF 1024
#define CLIENT 5 // anzahl der Clients die sich verbinden können im backlog
// DOCUMENT ZEILEN
#define sender_line 1
#define subject_line 6
#define msg_line 10

int server_socket = -1;
int new_socket = -1;
int abortRequested = 0;
char *FOLDER = NULL;
char *spoolName = NULL;
char buffer[BUF];
char path[20] = "./../mail/spool/";
//#################################

///////////////////////////////////////////////////////////////////////////////

// neue Verbindung mit dem Client
void *clientCommunication(void *client_data);
void signalHandler(int sig);
void createMSG(char *path, char *sender, char *receiver, char *subject, std::string msg);
void QUIT(int *current_socket);
void SEND(char *buffer);
void commandHandler(char* command, char* buffer, int* current_socket);
char* getCommand(char* buffer);
void sendToClient(int* current_socket,char *buffer);
vector<string> getFileInput(std::string path, std::string file);
void LOGIN();
int LIST(char* path,char* buffer, int* current_socket);
int READ(char* path,char* buffer, int* current_socket);
int DELETE(char* path,char* buffer,int* current_socket);
void createFolder(char *path);

///////////////////////////////////////////////////////////////////////////////

//        [PORT][MAIL-SPOOL-NAME]
int main(int argc, char **argv)
{

   // Benachrichtigung  - ########################
   if (argc < 3 || atoi(argv[1]) == 0)
   {
      fprintf(stderr, "\nUsage: ./server [PORT] [mail-spool-name]\n");
      return EXIT_FAILURE;
   }
   //##########################################

   //PORT durch eingabe [PORT]  - #############
   int PORT = atoi(argv[1]);
   spoolName = argv[2];
   char pa[20];
   strcat(pa,"./../mail/");
   strcat(pa,spoolName);
   strcat(pa,"/");
   strcpy(path,pa);



    createFolder(pa);
   //##########################################

   //ADRESS Size
   socklen_t addrlen;
   struct sockaddr_in address, clientaddress;
   int reuseValue = 1;
   //##########################################

   ////////////////////////////////////////////////////////////////////////////
   // SIGNAL HANDLER
   if (signal(SIGINT, signalHandler) == SIG_ERR)
   {
      perror("signal can not be registered");
      return EXIT_FAILURE;
   }
   ////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////
   // SOCKET
   ////////////////////////////////////////////////////////////////////////////
   // CREATE SOCKET OPTIONS
   //--> Socket - CREATE ################ (IPV4 - TCP) server_error(1)
   if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      server_error(1);
      return EXIT_FAILURE;
   }
   ////////////////////////////////////////////////////////////////////////////

   //### socket --> dieser Socket := createSocket wurde erstellt

   ////////////////////////////////////////////////////////////////////////////
   // SET SOCKET OPTIONS
   // dieser createSocket wird hier bearbeitet
   // socket, level, optname, optvalue, optlen

   // set address
   if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuseValue, sizeof(reuseValue)) == -1)
   {
      perror("set socket options - reuseAddr");
      return EXIT_FAILURE;
   }

   // set port
   if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &reuseValue, sizeof(reuseValue)) == -1)
   {
      perror("set socket options - reusePort");
      return EXIT_FAILURE;
   }
   ////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////
   // INIT ADDRESS
   memset(&address, 0, sizeof(address));
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons(PORT);
   //##########################################

   ////////////////////////////////////////////////////////////////////////////
   // ASSIGN AN ADDRESS WITH PORT TO SOCKET
   if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) == -1)
   {
      server_error(2);
      return EXIT_FAILURE;
   }
   //##########################################

   ////////////////////////////////////////////////////////////////////////////
   // ALLOW CONNECTION ESTABLISHING
   //--> Socket - LISTEN #####################
   if (listen(server_socket, CLIENT) == -1)
   {
      server_error(3);
      return EXIT_FAILURE;
   }
   //##########################################

   while (!abortRequested)
   {

      // CONNECTION
      printf("Waiting for connections...\n");

      /////////////////////////////////////////////////////////////////////////
      // ACCEPTS CONNECTION SETUP
      // blocking, might have an accept-error on ctrl+c
      //--> Socket - ACCEPT ######################
      // WENN ER WARTET --> SITZEN AUF EINEM PORT UND WARTEN

      addrlen = sizeof(struct sockaddr_in);

      // wird eine Verbindung gefunden wird eine neue Socket erstellt
      // damit der server_socket weiter listen kann
      // Adresse wird in clientaddress gespeichert
      if ((new_socket = accept(server_socket, (struct sockaddr *)&clientaddress, &addrlen)) == -1)
      {
         // DATEN BEKOMMEN wird überprüft ob abgebrochen wird
         if (abortRequested)
         {
            server_error(5);
         }
         else
         {
            server_error(4);
         }
         break;
      }

      /////////////////////////////////////////////////////////////////////////
      // START CLIENT
      // Client connect

      //    [Socket]          [Socket]
      // zwei socket  Quelladresse <--> Zieladress = verbindung
      // Datei Kommuikation nur die DATA <--> Programm

      printf("Client connected from %s:%d...\n",
             // Liest die Client adresse aus
             inet_ntoa(clientaddress.sin_addr),
             // Liest den Port vom Client aus
             ntohs(clientaddress.sin_port));
      // gibt die Client adresse aus [IP] + [Port]
      clientCommunication(&new_socket); // returnValue can be ignored
      new_socket = -1;
   }

   //######################################################################

   return EXIT_SUCCESS;
}

void *clientCommunication(void *client_data)
{
   char buffer[BUF];
   memset(buffer, 0, BUF);
   int size;
   int *current_socket = (int *)client_data;

   ////////////////////////////////////////////////////////////////////////////
   // SEND welcome message
   strcpy(buffer, "Welcome to my First TW-Mailer!\r\nPlease enter your commands... (help)\r\n");
   if (send(*current_socket, buffer, strlen(buffer), 0) == -1)
   {
      perror("send failed");
      return NULL;
   }



   do
   {
      /////////////////////////////////////////////////////////////////////////
      // RECEIVE

      size = recv(*current_socket, buffer, BUF - 1, 0);
      if (size == 1)
      {
         continue;
      }
      if (size == -1)
      {
         if (abortRequested)
         {
            perror("recv error after aborted");
         }
         else
         {
            perror("recv error");
         }
         break;
      }
      if (size == 0)
      {
         printf("Client closed remote socket\n");
         break;
      }

      // remove ugly debug message, because of the sent newline of client
      if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n')
      {
         size -= 2;
      }
      else if (buffer[size - 1] == '\n')
      {
         --size;
      }

      buffer[size] = '\0';

      // GIBT DEN BUFFER AUS
      printf("Message received: %s\n", buffer); // ignore error


      commandHandler(getCommand(buffer),buffer, current_socket);


     //  if (send(*current_socket, "OK", 3, 0) == -1){perror("send answer failed");return NULL;}

   } while (strcmp(buffer, "quit") != 0 && !abortRequested);


   QUIT(current_socket);
   return NULL;
}


//GETTER
char* getCommand(char* buffer){
    char *currentBuffer = (char *)malloc(strlen(buffer) * sizeof(char));
    strcpy(currentBuffer, buffer);
    return strtok(currentBuffer, ";");

}

//COMMANDOS
// -> SEND
void SEND(char *buffer){

    // path

    // buffer --> SENDER;REC;SUB;MSG
    char *msgCopy = (char *)malloc(strlen(buffer) * sizeof(char));
    strcpy(msgCopy, buffer);
    strtok(msgCopy, ";");
    char *sender = strtok(NULL, ";");
    char *receiver = strtok(NULL, ";");
    char *subject = strtok(NULL, ";");
    char *msg = strtok(NULL, ";");

    createMSG(path, sender, receiver, subject, msg);
    free(msgCopy);
}
void createMSG(char* path, char *sender, char *receiver, char *subject, std::string msg)
{
    int counter = 0;
    char number [4];
    char full_path[BUF] = "";
    char current_path[BUF] = "";
    char fileName[50] = "";
    //path
    strcat(full_path, path);
    //Foldername - RECIVER
    strcat(full_path, receiver);
    mkdir(full_path, 0777);
    strcat(full_path, "/");


    do{
        strcpy(current_path,full_path);
        strcpy(fileName,"");
        strcat(fileName, receiver);
        strcat(fileName, "_message#");
        sprintf(number,"%d",counter);
        strcat(fileName, number); // Counter
        strcat(fileName, ".txt"); // Data name
        strcat(current_path, fileName); // Data name
        fs::path p(current_path);
        if(!(fs::exists(p))){break;}
        else{counter ++;}
    }while(true);


    strcat(full_path, fileName);
    std::cout << full_path;
    std::ofstream out;
    out.open(full_path, std::ofstream::app);
    out << "\n\nSENDER:\n" << sender;
    out << "\n\nBETREFF:\n" << subject;
    out << "\n\n\nNACHRICHT:\n" << msg;
    out.close();
}


void createFolder(char *path){
    mkdir(path, 0777);
    strcat(path, "/");
}



// -> LOGIN
void LOGIN(){
}

// -> LIST
int LIST(char* path,char* buffer,int* current_socket){
    char *msgCopy = (char *)malloc(strlen(buffer) * sizeof(char));
    strcpy(msgCopy, buffer);
    strtok(msgCopy, ";");
    char* username = strtok(NULL, ";");

    char currentPath[BUF];
    strcpy(currentPath,path);
    strcat(currentPath,username);
    string currentfilename = "";
    char buff[BUF];
    int size =0;
    bool is_empty = true;


    for (const auto & file : directory_iterator(currentPath)){
        is_empty = false;
        currentfilename =file.path().filename();

        string filename = currentfilename;
        string path = file.path().parent_path();
        path.append("/");
        strcat(buff, getFileInput(path, currentfilename).at(subject_line).c_str());
        strcat(buff, ";");

        size = strlen(buff);
        if (buff[size - 2] == '\r' && buff[size - 1] == '\n'){size -= 2;buff[size] = 0;}
        else if (buff[size - 1] == '\n'){--size;buff[size] = 0;}
    }

    if(is_empty == true){
        strcat(buff,"0");
        strcat(buff, ";");
    }
    sendToClient(current_socket,buff);
    free(msgCopy);
    return 0;
}

// -> READ
int READ(char* path,char* buffer, int* current_socket){
    char *msgCopy = (char *)malloc(strlen(buffer) * sizeof(char));
    strcpy(msgCopy, buffer);
    strtok(msgCopy, ";");
    char* username = strtok(NULL, ";");
    char* fileNumber = strtok(NULL, ";");

    char buff[BUF];

    string currentPath = path;
    currentPath.append(username);
    string filename = username;
    filename.append("/");
    filename.append(username);
    filename.append("_message#");
    filename.append(fileNumber);
    filename.append(".txt");


    std::vector<string> lines = getFileInput(path,filename);
    if(lines.empty()){
        return -1;
    }else{
        for (const auto &i : lines){
            strcat(buff, i.c_str());
            strcat(buff, ";");
        }
    }

    sendToClient(current_socket,buff);
    free(msgCopy);
    return 0;
}
vector<string> getFileInput(std::string path, std::string file){
    std::string filename = "";
    filename.append(path);
    filename.append(file);
    std::vector<string> lines;
    std::string line;

    ifstream input_file(filename);
    if (!input_file.is_open()) {
        cerr << "Could not open the file - '"
             << filename << "'" << endl;
        return lines;
    }

    while (getline(input_file, line)){
        lines.push_back(line);
    }

 //   for (const auto &i : lines)
  //      cout << i << endl;

    input_file.close();

    return lines;
}

// -> DELETE
int DELETE(char* path,char* buffer, int* current_socket){
    char *msgCopy = (char *)malloc(strlen(buffer) * sizeof(char));
    strcpy(msgCopy, buffer);
    strtok(msgCopy, ";");
    char* username = strtok(NULL, ";");
    char* fileNumber = strtok(NULL, ";");

    string filename = path;
    filename.append(username);
    filename.append("/");
    filename.append(username);
    filename.append("_message#");
    filename.append(fileNumber);
    filename.append(".txt");

    char buff[BUF];
    if(remove (filename.c_str()) == -1){
        sprintf(buff,"ERROR - FILE NOT FOUND");
        sendToClient(current_socket,buff);
        return -1;
    }else{
        sprintf(buff,"SUCCSESS - DELETE FILE");
    };
        sendToClient(current_socket,buff);


    free(msgCopy);
    return 0;
}

// -> QUIT
void QUIT(int *current_socket)
{
    ////////////////////////////////////////////////////////////////////////////
    // CLOSES THE DESCRIPTOR
    // closes/frees the descriptor if not already
    if (*current_socket != -1)
    {
        if (shutdown(*current_socket, SHUT_RDWR) == -1)
        {
            perror("shutdown new_socket");
        }
        if (close(*current_socket) == -1)
        {
            perror("close new_socket");
        }
        *current_socket = -1;
    }
}

// HANDLER
void commandHandler(char* command, char* buffer, int* current_socket){
    if(strcmp(command,"send") == 0) {
        printf("SEND: %d\n",strcmp(command,"send"));
        SEND(buffer);
        char MSG [BUF];
        sprintf(MSG,"FINISH - SEND");
        sendToClient(current_socket,MSG);
    }

    else if(strcmp(command,"list") == 0){
        printf("LIST: %d\n",strcmp(command,"list"));
        LIST(path,buffer,current_socket);
        char MSG [BUF];
        sprintf(MSG,"FINISH - LIST");
        sendToClient(current_socket,MSG);
    }

    else if(strcmp(command,"read") == 0){
        printf("READ: %d\n",strcmp(command,"read"));
        char MSG [BUF];
        if(READ(path,buffer, current_socket) == -1){
            sprintf(MSG,"FILE - NOT FOUND");
        }else{
            sprintf(MSG,"FINISH - READ");
        }
        sendToClient(current_socket,MSG);
    }

    else if(strcmp(command,"delete") == 0){
        printf("DELETE: %d\n",strcmp(command,"delete"));
        DELETE(path,buffer,current_socket);
    }
    else if(strcmp(command,"help") == 0){
        char MSG [BUF];
        sprintf(MSG," --- HELP ---");

        strcat(MSG,"SEND: client sends a message to the server. ;");
        strcat(MSG,"LIST: lists all messages of a specific user.;");
        strcat(MSG,"READ: display a specific message of a specific user.;");
        strcat(MSG,"DEL: removes a specific message.;");
        strcat(MSG,"QUIT: logout the client.;");

        sendToClient(current_socket,MSG);
    }

    else if(strcmp(command,"quit") == 0){
        char MSG [BUF];
        sprintf(MSG,"Bye!");
        sendToClient(current_socket,MSG);
    }

    else {
        char MSG [BUF];
        sprintf(MSG,"ERROR - COMMAND NOT FOUND");

        sprintf(MSG,"\n --- HELP ---\n");
        strcat(MSG,"SEND: client sends a message to the server. ;");
        strcat(MSG,"LIST: lists all messages of a specific user.;");
        strcat(MSG,"READ: display a specific message of a specific user.;");
        strcat(MSG,"DEL: removes a specific message.;");
        strcat(MSG,"QUIT: logout the client.;");

        sendToClient(current_socket,MSG);
    }
    memset(buffer, 0, BUF);
}
void signalHandler(int sig)
{
   if (sig == SIGINT)
   {
      printf("abort Requested... "); // ignore error
      abortRequested = 1;
      /////////////////////////////////////////////////////////////////////////
      // With shutdown() one can initiate normal TCP close sequence ignoring
      // the reference count.
      if (new_socket != -1)
      {
         if (shutdown(new_socket, SHUT_RDWR) == -1)
         {
            perror("shutdown new_socket");
         }
         if (close(new_socket) == -1)
         {
            perror("close new_socket");
         }
         new_socket = -1;
      }

      if (server_socket != -1)
      {
         if (shutdown(server_socket, SHUT_RDWR) == -1)
         {
            perror("shutdown server_socket");
         }
         if (close(server_socket) == -1)
         {
            perror("close server_socket");
         }
         server_socket = -1;
      }
   }
   else
   {
      exit(sig);
   }
}
void sendToClient(int* current_socket,char *buffer){
    int size = strlen(buffer);
    // remove new-line signs from string at the end
    if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n'){size -= 2;buffer[size] = 0;}
    else if (buffer[size - 1] == '\n'){--size;buffer[size] = 0;}

    if ((send(*current_socket, buffer, strlen(buffer), 0)) == -1)
    {
        perror("send error");
    }
    memset(buffer,0,BUF);
}






