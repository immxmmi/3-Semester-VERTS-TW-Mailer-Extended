#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <ldap.h>
#include <lber.h>
#include "server_error.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>
#include <sstream>
#include <time.h>


using namespace std;
namespace fs = filesystem;
using std::filesystem::directory_iterator;
using std::cout; using std::cerr;
using std::endl; using std::string;
using std::ifstream; using std::vector;

/// SERVER///////////////////////////
#define BUF 1024 // Speicher
#define CLIENT 5 // anzahl der Clients die sich verbinden können im backlog

/// GLOBAL- SERVER - VAR ///////////
int server_socket = -1;
int new_socket = -1;
int abortRequested = 0;
int loginCounter = 0;
char buffer[BUF] = "";
string delimiter (";");
////////////////////////////////////

/// DOCUMENT ZEILEN/////////////////
#define sender_line 1
#define subject_line 6
#define msg_line 10
////////////////////////////////////

/// DEFAULT PATH + PORT ///////////
int PORT = 4444;
char path[20] = "./../mail/spool/";
string currentIP="";
///////////////////////////////////

/// BLACK LIST ////////////////////
pthread_mutex_t mutexLockBlacklist;
char blacklistPath[] = "./../data/blacklist/";
string blackListFileName = "../data/blacklist/blacklist.txt";
int addToBlacklist();
int checkBlacklist();
///////////////////////////////////

/// CLIENT COMMUNICATION //////////
void *clientCommunication(void *client_data);
void sendToClient(int* current_socket,char *buffer);
string checkLOGIN(char* ldap_username, char* ldap_password);
///////////////////////////////////

///Handler/////////////////////////
void signalHandler(int sig);
void commandHandler(char* command, char* buffer, int* current_socket);
char* getCommand(char* buffer);
///////////////////////////////////

/// TOOLS /////////////////////////
void createMSG(char *path, char *sender, char *receiver, char *subject, std::string msg);
void createFolder(char *path);
vector<string> getFileInput(std::string path, std::string file);
void sendStatus(int status,int *current_socket);
string getUserPath(char* root, char* username);
string getUserFileName(char* username, char* fileNumber);
///////////////////////////////////


///COMMANDS ////////////////////////
    string LOGIN(char *buffer);
    int SEND(char *buffer);
    int LIST(char* path,char* buffer, int* current_socket);
    int READ(char* path,char* buffer, int* current_socket);
    int DELETE(char* path,char* buffer,int* current_socket);
    int HELP(int *current_socket);
    int QUIT(int *current_socket);
///////////////////////////////////





//        [PORT][MAIL-SPOOL-NAME]
int main(int argc, char **argv)
{

////////////////////////////////////////////////////////////////////////////
///ARGUMENT ///////////////////////////////////////////////////////////////
   if (argc < 3 || atoi(argv[1]) == 0){
      fprintf(stderr, "\nUsage: ./server <PORT> <mail-spool-name>\n");
      return EXIT_FAILURE;
   }
////////////////////////////////////////////////////////////////////////////


/// MUTEX /////////////////////////////////////////////////////////////////
    if (pthread_mutex_init(&mutexLockBlacklist, NULL) != 0 )
    {
        printf("\n mutex init has failed\n");
        return 1;
    }
////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////
///SET ARGUMENTS PORT AND PATH ////////////////////////////////////////////
PORT = atoi(argv[1]);
/// Speicher Adresse//////////////////////////////////////////////////////
   char newPath[20];
   strcat(newPath,"./../mail/"); // Speicher adresse
   strcat(newPath,argv[2]); // fügt den Parameter --> als namen hinzu
   strcat(newPath,"/");
   strcpy(path,newPath);   // übergibt die neu erstellte Variable der globalen Variable
   createFolder(newPath); //erstellt neuen Ordner


   ////////////////////////////////////////////////////////////////////////////
   ///ADRESS Size////////////////////
   socklen_t addrlen;
   struct sockaddr_in address, clientaddress;
   int reuseValue = 1;
   /////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////
   /// SIGNAL HANDLER
   if (signal(SIGINT, signalHandler) == SIG_ERR){
      perror("signal can not be registered");
      return EXIT_FAILURE;
   }
   ////////////////////////////////////////////////////////////////////////////


   ////////////////////////////////////////////////////////////////////////////
   //////////////////////////////  -  SOCKET  -  //////////////////////////////
   ////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////
   /// CREATE SOCKET OPTIONS///////////////////////////////////////////////////
   ///--> Socket - CREATE ################ (IPV4 - TCP) server_error(1)
   if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      server_error(1);
      return EXIT_FAILURE;
   }
   ////////////////////////////////////////////////////////////////////////////


   ////////////////////////////////////////////////////////////////////////////
   /// SET SOCKET OPTIONS
   /// dieser createSocket wird hier bearbeitet
   /// socket, level, optname, optvalue, optlen

   /// set address
   if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuseValue, sizeof(reuseValue)) == -1)
   {
      perror("set socket options - reuseAddr");
      return EXIT_FAILURE;
   }

   /// set port
   if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &reuseValue, sizeof(reuseValue)) == -1)
   {
      perror("set socket options - reusePort");
      return EXIT_FAILURE;
   }
   ////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////
   /// INIT ADDRESS
   memset(&address, 0, sizeof(address));
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY; /// IP - ADRESSE
   address.sin_port = htons(PORT);  /// PORT WIRD HIER ÜBERGBEN
   ////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////
   /// ASSIGN AN ADDRESS WITH PORT TO SOCKET
   if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) == -1)
   {
      server_error(2);
      return EXIT_FAILURE;
   }
   ////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////
   /// ALLOW CONNECTION ESTABLISHING
   ///--> Socket - LISTEN ////////////////////////////////////////////////////
   if (listen(server_socket, CLIENT) == -1){ // wartet auf die Verbindung
      server_error(3);
      return EXIT_FAILURE;
   }
   ////////////////////////////////////////////////////////////////////////////

   /// FORKEN -- VAR
    pid_t childPid; // KIND PROZESS
    static int childPidCounter = 0; // KIND PROZESS ZÄHLER
    ////////////////////////////////////////////////////////////////////////////

    while (!abortRequested) {
       ///CONNECTION////////////////////////////////////////////////////////////////
       printf("Waiting for connections...\n");
       ////////////////////////////////////////////////////////////////////////////

       /////////////////////////////////////////////////////////////////////////
       /// ACCEPTS CONNECTION SETUP
       /// blocking, might have an accept-error on ctrl+c
       ///--> Socket - ACCEPT ######################
       addrlen = sizeof(struct sockaddr_in);

       /// wird eine Verbindung gefunden wird eine neue Socket erstellt
       /// damit der server_socket weiter listen kann
       /// Adresse wird in clientaddress gespeichert

       if ((new_socket = accept(server_socket, (struct sockaddr *) &clientaddress, &addrlen)) == -1) {
           /// DATEN BEKOMMEN wird überprüft ob abgebrochen wird
           if (abortRequested) {
               server_error(5);
           } else {
               server_error(4);
           }
           break;
       }


       /////////////////////////////////////////////////////////////////////////
       /// START CLIENT
       /// Client connect

       ///    [Socket]          [Socket]
       /// zwei socket  Quelladresse <--> Zieladress = verbindung
       /// Datei Kommunikation nur die DATA <--> Programm


       /// gibt die Client adresse aus [IP] + [Port]
       printf("Client connected from %s:%d...\n",
              inet_ntoa(clientaddress.sin_addr), // Liest die Client adresse aus
              ntohs(clientaddress.sin_port));    // Liest den Port vom Client aus

              currentIP =  inet_ntoa(clientaddress.sin_addr);
       ////////////////////////////////////////////////////////////////////////
       /// FORKEN

       if ((childPid = fork()) == 0) {  // WENN DAS FORKEN ERFOLGREICH WAR


           childPidCounter++; // Zählt die Kind-Prozesse
           clientCommunication(&new_socket); /// und startet die Kommunikation mit dem Client
           new_socket = -1;
           exit(EXIT_SUCCESS);

       } else if (childPid == -1)  // FORK ERROR
       {
           perror("ERROR - FORK");
           exit(EXIT_FAILURE);
       }


       if (childPid != 0 && childPid != -1) {
           for (int i = 0; i < childPidCounter; i++) // BEENDET ALLE PROZESSE
           {
               wait(NULL);
           }

           //  QUIT(current_socket);  // SHUTDOWN SOCKET
       }

   }


   return EXIT_SUCCESS;
}




void *clientCommunication(void *client_data)
{
   int size;
   memset(buffer, 0, BUF); // BUFFER LEEREN
   int *current_socket = (int *)client_data; // SOCKET SPEICHERN

   ////////////////////////////////////////////////////////////////////////////
   /// SEND welcome message
   strcpy(buffer, "Welcome to my First TW-Mailer!\r\nPlease enter your commands.... \r\n");
   sendToClient(current_socket,buffer); // sendet Buffer an Client
   ////////////////////////////////////////////////////////////////////////////



   do
   {
      /////////////////////////////////////////////////////////////////////////
      /// RECEIVE
      size = recv(*current_socket, buffer, BUF - 1, 0);
      if (size == 1) {continue;}
      if (size == -1){
          if (abortRequested){perror("recv error after aborted");}
         else{perror("recv error");}
         break;
      }

      /// VERBINDUNG MIT CLIENT BEENDET///////////////
      if (size == 0){
         printf("Client closed remote socket\n");
         break;
      }
      ////////////////////////////////////////////////


      /// ENTFERNT \n \r /////////////////////////////
      if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n'){size -= 2;}
      else if (buffer[size - 1] == '\n'){--size;}
      buffer[size] = '\0';
      ////////////////////////////////////////////////

      /// GIBT DEN BUFFER AUS --> DIE ANTWORT VOM CLIENT///////////////
      printf("Message received: %s\n", buffer); // ignore error
      /////////////////////////////////////////////////////////////////


       ////////////////////////////////////////////////////////////////////////////
       ///                            CLIENT-SESSION                           ////
       ////////////////////////////////////////////////////////////////////////////


       // Handler
      commandHandler(getCommand(buffer),buffer, current_socket);


       ////////////////////////////////////////////////////////////////////////////


   } while (strcmp(buffer, "quit") != 0 && !abortRequested);


   //QUIT(current_socket);
   return NULL;
}


//COMMANDOS
// -> LOGIN
string LOGIN(char *buffer){
    /// buffer --> COMMAND;USERNAME;PASSWORD;
    char *msgCopy = (char *)malloc(strlen(buffer) * sizeof(char));
    strcpy(msgCopy, buffer);
    strtok(msgCopy, delimiter.c_str());
    char* username1 = strtok(NULL, delimiter.c_str());
    char* password1 = strtok(NULL, delimiter.c_str());
    char username[10];
    char password[30];
    sprintf(username,username1);
    sprintf(password,password1);
    free(msgCopy);
    memset(buffer,0,BUF);
    return checkLOGIN(username, password);
}
// -> SEND
int SEND(char *buffer){
    /// buffer --> COMMAND;SENDER;RECEIVER;SUBJECT;MESSAGE
    char *msgCopy = (char *)malloc(strlen(buffer) * sizeof(char));
    strcpy(msgCopy, buffer);

    char *line;

    line = strtok(msgCopy, delimiter.c_str());
    line = strtok(NULL, delimiter.c_str());
    char *sender = line;
    line = strtok(NULL, delimiter.c_str());
    char *receiver = line;
    line = strtok(NULL, delimiter.c_str());
    char *subject = line;

    string msg;
        line = strtok(NULL, delimiter.c_str());
    while(line != NULL) {
        msg.append(line);
        msg.append("\n");
        line = strtok(NULL, delimiter.c_str());
    }

    createMSG(path, sender, receiver, subject, msg);
    free(msgCopy);
    memset(buffer,0,BUF);
    return 0;
}

// -> LIST
int LIST(char* path,char* buffer,int* current_socket){
    /// buffer --> COMMAND;USERNAME;
    char *msgCopy = (char *)malloc(strlen(buffer) * sizeof(char));
    strcpy(msgCopy, buffer);
    strtok(msgCopy, delimiter.c_str());
    char* username = strtok(NULL, delimiter.c_str());
    fs::path p(getUserPath(path, username));
    char buff[BUF] = "";
    strcat(buff,"LIST;");
    int size =0;
    bool is_empty = true;


if(fs::exists(p) == true) {
    string currentFilename = "";
    string fileNumber = "";
    char* fileNameToChar = NULL;
    for (const auto &file: directory_iterator(getUserPath(path, username))) {
        is_empty = false;
        currentFilename = file.path().filename();
        string filename = currentFilename;
        fileNameToChar = strdup(filename.c_str());
        strtok(fileNameToChar, "#");
        fileNumber = strtok(NULL, "#");

        string path = file.path().parent_path();
        path.append("/");

        strcat(buff,fileNumber.c_str());
        strcat(buff,": ");
        strcat(buff, getFileInput(path, currentFilename).at(subject_line).c_str());
        strcat(buff, delimiter.c_str());
        size = strlen(buff);
        if (buff[size - 2] == '\r' && buff[size - 1] == '\n') {
            size -= 2;
            buff[size] = 0;
        }
        else if (buff[size - 1] == '\n') {
            --size;
            buff[size] = 0;
        }
    }
}


    if(is_empty == true){
        free(msgCopy);
        sprintf(buff,"ERROR - EMPTY FOLDER;");
        sendToClient(current_socket,buff);
        memset(buffer,0,BUF);
        return -1;
    }

    sendToClient(current_socket,buff);
    free(msgCopy);
    memset(buffer,0,BUF);
    return 0;
}
// -> READ
int READ(char* path,char* buffer, int* current_socket){
    /// buffer --> COMMAND;USERNAME;NUMBER
    char *msgCopy = (char *)malloc(strlen(buffer) * sizeof(char));
    strcpy(msgCopy, buffer);
    strtok(msgCopy, delimiter.c_str());
    char* username = strtok(NULL, delimiter.c_str());
    char* fileNumber = strtok(NULL, delimiter.c_str());
    char buff[BUF] = "";

    std::vector<string> lines = getFileInput(getUserPath(path,username),getUserFileName(username,fileNumber));

    if(lines.empty()){
        strcat(buff,"FILE - NOT FOUND");
        sendToClient(current_socket,buff);
        free(msgCopy);
        memset(buff,0,BUF);
        return -1;
    }else{
      //  strcat(buff,"READ;");
        for (const auto &i : lines){
            strcat(buff, delimiter.c_str());
            strcat(buff, i.c_str());
        }
    }
    sendToClient(current_socket,buff);
    free(msgCopy);
    memset(buff,0,BUF);
    return 0;
}
// -> DELETE
int DELETE(char* path,char* buffer, int* current_socket){
    /// buffer --> COMMAND;USERNAME;NUMBER
    char *msgCopy = (char *)malloc(strlen(buffer) * sizeof(char));
    strcpy(msgCopy, buffer);
    strtok(msgCopy, delimiter.c_str());
    char* username = strtok(NULL, delimiter.c_str());
    char* fileNumber = strtok(NULL, delimiter.c_str());

    string filename = getUserPath(path,username);
    filename.append(getUserFileName(username,fileNumber));

    char buff[BUF];
    if(remove (filename.c_str()) == -1){
        sprintf(buff,"ERROR - FILE NOT");
        sendToClient(current_socket,buff);
        return -1;
    }else{
        sprintf(buff,"SUCCSESS - DELETE FILE;");
    };
        sendToClient(current_socket,buff);


    free(msgCopy);
    return 0;
}
// -> HELP
int HELP(int *current_socket){

    char buff[BUF];
    strcpy(buff,"|-----------------------------------------------------|;");
    strcat(buff,"|-----------------        HELP       -----------------|;");
    strcat(buff,"|-----------------------------------------------------|;");
    strcat(buff,"| SEND: client sends a message to the server.         |;");
    strcat(buff,"| LIST: lists all messages of a specific user.        |;");
    strcat(buff,"| READ: display a specific message of a specific user.|;");
    strcat(buff,"| DELETE: removes a specific message.                 |;");
    strcat(buff,"| QUIT: logout the client.                            |;");
    strcat(buff,"|-----------------------------------------------------|\n");
    sendToClient(current_socket,buff);
    memset(buff,0,BUF);
    return 0;
}
// -> QUIT
int QUIT(int *current_socket)
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

    return 0;
}

//GETTER
char* getCommand(char* buffer){
    char *currentBuffer = (char *)malloc(strlen(buffer) * sizeof(char));
    strcpy(currentBuffer, buffer);
    return strtok(currentBuffer, delimiter.c_str());
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

    input_file.close();

    return lines;
}
string getUserPath(char* root, char* username){
    string userPath = "";
    userPath.append(root);
    userPath.append(username);
    userPath.append("/");
    return userPath;
}
string getUserFileName(char* username, char* fileNumber){
    string fileName="";
    fileName.append(username);
    fileName.append("_message#");
    fileName.append(fileNumber);
    fileName.append("#.txt");
    return fileName;
}

//LDAP
string checkLOGIN(char* ldap_username, char* ldap_password){

    if(checkBlacklist() == -1){
       return "false";
    }

    if(loginCounter > 2){
        addToBlacklist();
        loginCounter = 0;
        return "false";
    }

    const char *ldapUri = "ldap://ldap.technikum-wien.at:389";
    const int ldapVersion = LDAP_VERSION3;


    //// USER////////////////////////////////////////////////
    // read username (bash: export ldapuser=<yourUsername>)
    char ldapBindUser[256];
    char rawLdapUser[128];

        strcpy(rawLdapUser, ldap_username);
        sprintf(ldapBindUser, "uid=%s,ou=people,dc=technikum-wien,dc=at", rawLdapUser);
        printf("user set to: %s\n", ldapBindUser);

    /////////////////////////////////////////////////////////

    //// PW ////////////////////////////////////////////////
// read password (bash: export ldappw=<yourPW>)
    char ldapBindPassword[256];
     strcpy(ldapBindPassword, ldap_password);
     printf("\nSET PW: ... \n");

/////////////////////////////////////////////////////////

int rc = 0; // return code




    ////////////////////////////////////////////////////////////////////////////
    /// setup LDAP connection
    ///
    LDAP *ldapHandle;
    rc = ldap_initialize(&ldapHandle, ldapUri);
    if (rc != LDAP_SUCCESS)
    {
        fprintf(stderr, "ldap_init failed\n");
        return "false";
    }
    printf("connected to LDAP server %s\n", ldapUri);

    ////////////////////////////////////////////////////////////////////////////
    /// set verison options
    rc = ldap_set_option( ldapHandle,
                          LDAP_OPT_PROTOCOL_VERSION, // OPTION
                          &ldapVersion);             // IN-Value
    if (rc != LDAP_OPT_SUCCESS)
    { fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        return "false";
    }

    ////////////////////////////////////////////////////////////////////////////
    /// start CONNECTION

    rc = ldap_start_tls_s(ldapHandle,NULL,NULL);
    if (rc != LDAP_SUCCESS)
    {
        fprintf(stderr, "ldap_start_tls_s(): %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        return "false";
    }

    ////////////////////////////////////////////////////////////////////////////
    /// bind credentials
    BerValue bindCredentials;
    bindCredentials.bv_val = (char *)ldapBindPassword;
    bindCredentials.bv_len = strlen(ldapBindPassword);
    BerValue *servercredp; // server's credentials
    rc = ldap_sasl_bind_s(
            ldapHandle,
            ldapBindUser,
            LDAP_SASL_SIMPLE,
            &bindCredentials,
            NULL,
            NULL,
            &servercredp);

    // LOGIN FAILD
    if (rc != LDAP_SUCCESS)
    {
        fprintf(stderr, "LDAP bind error: %s\n", ldap_err2string(rc));
        loginCounter++;
        return "false";
    }else{
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        string username = "";
        username.append(ldap_username);
        return username;
    }
}

//BLACKLIST

int addToBlacklist(){
    printf("\nSTART BUFFER:\n");
    char Path[] = "./../data/";
    createFolder(Path);
    //char blacklistPath[] = "./../data/blacklist/";
    createFolder(blacklistPath);
    strcat(blacklistPath,"blacklist.txt");
    // ZEIT
    time_t currentTime = time(NULL);

    pthread_mutex_lock(&mutexLockBlacklist);

    std::ofstream out;
    out.open(blacklistPath, std::ofstream::app);
    out << "" << currentTime;
    out << "\n";
    out << "" << currentIP;
    out << "\n\n";
    out.close();

    pthread_mutex_unlock(&mutexLockBlacklist);
    return 0;
}


int checkBlacklist(){
    std::ifstream in("../data/blacklist/blacklist.txt");
   // std::ifstream in("../data/blacklist/blacklist.txt");
    if (in.is_open()){
        std::string line;
        while(std::getline(in,line)){
            if(strcmp(currentIP.c_str(),line.c_str())==0){
                std:: cout << "\nIP : " << line << " BLOCKED\n";
                return -1;
            }
        }
        in.close();
    }
        return 0;
}

/**
int checkBlacklist(){
    char blacklistPath[] = "./data/blacklist.txt";
    char buffer[BUF];

    FILE *filePtr1 = NULL;


    pthread_mutex_lock(&mutexLockBlacklist);

    filePtr1 = fopen(blacklistPath, "r");


    if (filePtr1 == NULL)
    {
        filePtr1 = fopen(blacklistPath, "w");
    }

    else
    {
        while (!feof(filePtr1))
        {
            strcpy(buffer, "\0");
            fgets(buffer, BUF, filePtr1);
            if (!feof(filePtr1))
            {
                strcpy(buffer, "\0");
                fgets(buffer, BUF, filePtr1);

                if (strncmp(ipAddress, buffer, strlen(ipAddress)) == 0)
                {
                    fclose(filePtr1);
                    pthread_mutex_unlock(&mutexLockBlacklist);
                    return 1;
                }


            }
        }
    }


    fclose(filePtr1);
    pthread_mutex_unlock(&mutexLockBlacklist);
    return 0;
}


**/

 // TOOLS
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
        strcat(fileName, "#.txt"); // Data name
        strcat(current_path, fileName); // Data name
        fs::path p(current_path);
        if(!(fs::exists(p))){break;}
        else{counter ++;}
    }while(true);


    strcat(full_path, fileName);
   // std::cout << full_path;
    std::ofstream out;
    out.open(full_path, std::ofstream::app);
    out << "\n\nSENDER:\n" << sender;
    out << "\n\nBETREFF:\n" << subject;
    out << "\n\n\nNACHRICHT:\n";
    out <<  msg;
    out.close();
}
void createFolder(char *path){
    mkdir(path, 0777);
    strcat(path, "/");
}

// HANDLER + COMMUNICATION
void commandHandler(char* command, char* buffer, int* current_socket){
    char MSG [BUF] = "";

    if(strcmp(command,"login") == 0) {
        printf("LOGIN: ");
        string loginData = LOGIN(buffer);
        sprintf(MSG,"AUTH;");
        strcat(MSG,loginData.c_str());
        strcat(MSG,delimiter.c_str());

        //int status = 0;

        if(loginData == "false"){
            //status = -1;
            strcat(MSG,"ERROR - LOGIN");
        }else{
            strcat(MSG,"SUCCESS - LOGIN");
        }


        sendToClient(current_socket,MSG);
        //sendStatus(status,current_socket);
        memset(buffer,0,BUF);
        memset(MSG,0,BUF);
    }

    else if(strcmp(command,"send") == 0) {
        sendStatus(SEND(buffer),current_socket);
        //sprintf(MSG,"FINISH - SEND");
        //sendToClient(current_socket,MSG);
        memset(buffer,0,BUF);
        //memset(MSG,0,BUF);
    }

    else if(strcmp(command,"list") == 0){
        printf("LIST: ");
        LIST(path,buffer,current_socket);
        memset(buffer,0,BUF);
    }

    else if(strcmp(command,"read") == 0){
            printf("READ: ");
            READ(path,buffer, current_socket);
            memset(buffer,0,BUF);
        }

    else if(strcmp(command,"delete") == 0){
        printf("DELETE: ");
        DELETE(path,buffer,current_socket);
        memset(buffer,0,BUF);
    }

    else if(strcmp(command,"help") == 0){
        HELP(current_socket);
        memset(buffer,0,BUF);
    }

    else if(strcmp(command,"quit") == 0){
        char MSG [BUF];
        sprintf(MSG,"\n\nBye!");
        sendToClient(current_socket,MSG);
        memset(MSG,0,BUF);
        memset(buffer,0,BUF);
    }

    else {
        strcpy(MSG,"|ERROR - COMMAND NOT FOUND!;");
        sendToClient(current_socket,MSG);
        //HELP(current_socket);
       // sendStatus(-1,current_socket);
        memset(buffer,0,BUF);
       memset(MSG,0,BUF);

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
     //ENTFERNEN \r und \n
    if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n'){size -= 2;buffer[size] = 0;}
    else if (buffer[size - 1] == '\n'){--size;buffer[size] = 0;}

    if ((send(*current_socket, buffer, strlen(buffer), 0)) == -1){perror("send error");} // SENDET AN CLIENT
    memset(buffer,0,BUF); // LEERT DEN BUFFER
}
void sendStatus(int status, int *current_socket){
    char MSG [BUF];
    if(status == 0){
        strcpy(MSG,"\n<<OK");
    }else{
        strcpy(MSG,"\n<<Err");
    }
    sendToClient(current_socket,MSG);
    memset(MSG,0,BUF);
}





