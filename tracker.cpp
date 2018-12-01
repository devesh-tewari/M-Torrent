#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <cstring>
#include <map>
#include <set>
#include <vector>
#include <netinet/in.h>
#include <fstream>
#include <thread>
#include <mutex>

#define bufSize 1024

using namespace std;

map < string, set<string> > seedersHash;  // stores the mapping of SHA hash to seeding clients

char seeder_list[200];   // this file stores the active seeders

char OTHER_TRACKER_IP[50];
char OTHER_TRACKER_PORT[20];

void handleRequest( struct sockaddr_in, char*, unsigned int, int );

//string REPLY;

mutex file_lock;
mutex map_lock;

string processRequest(char* buf, char* addr, int sockfd, bool fromTracker)
{
  string REPLY;

  char buffer[bufSize];
  strcpy(buffer,buf);
  char *token;
  token = strtok (buffer," ");
  vector<char*> tokens;
  tokens.push_back(token);
  while (token != NULL)
  {
    //cout<<tokens[i++];
    token = strtok(NULL," ");
    tokens.push_back( token );
  }
  int size = tokens.size();
  if( size > 1 )
  {
    if( strcmp(tokens[0],"share") == 0 )
  	{
      set<string> s;
      string SHA(tokens[2]);
      string client(tokens[3]);
      //string cliListenPort(tokens[3]);
      //client = client + ":" + cliListenPort;

      map_lock.lock();

      if( seedersHash.find(SHA) == seedersHash.end() ) //SHA string not already present
      {
        s.insert(client);
        seedersHash[SHA] = s;

        file_lock.lock();

        ofstream log ( seeder_list, ios::ate | ios::app);
        log << tokens[1] <<endl;
        log << SHA <<endl;
        log << client <<endl;
        log.close();

        file_lock.unlock();
      }
      else   // SHA string already present
      {
        if( seedersHash[SHA].find(client) == seedersHash[SHA].end() )  //client not already present
        {
          seedersHash[SHA].insert(client);
          vector<string> file;
          string line,logCli;

          file_lock.lock();

          ifstream log(seeder_list);
          while ( getline (log,line) )
          {
              if( SHA == line )
              {
                  file.push_back(line);
                  getline(log,logCli);   //tokenize clients and store it in a set
                  token = strtok (&logCli[0]," ");
                  string cl(token);
                  while (token != NULL)
                  {
                    token = strtok(NULL," ");
                    if(token!=NULL)
                    {
                      string cl1(token);
                      cl = cl + " " + cl1;
                    }
                  }
                  cl = cl + " " + client;
                  file.push_back(cl);
              }
              else
                  file.push_back(line);
            }
            log.close();
            ofstream log1 ( seeder_list, ios::ate );
            for(int i=0; i<file.size(); i++)
                log1 << file[i] <<endl;
            log1.close();

            file_lock.unlock();
          }
        }

        map_lock.unlock();

        REPLY = "Shared";

        if( !fromTracker )
        {
          struct sockaddr_in otherServAdd;
          memset(&otherServAdd, 0, sizeof(otherServAdd));

          otherServAdd.sin_family = AF_INET;  // internet address
          otherServAdd.sin_port = htons( atoi(OTHER_TRACKER_PORT) );
          otherServAdd.sin_addr.s_addr = inet_addr(OTHER_TRACKER_IP);

          char toOtherTracker[1024];
          strcpy(toOtherTracker,"tracker:");
          strcat(toOtherTracker,buf);

          sendto(sockfd, (const char *)toOtherTracker, strlen(toOtherTracker), MSG_CONFIRM, (const struct sockaddr *) &otherServAdd, sizeof(otherServAdd));
        }

        return REPLY;
      }

      else if( strcmp(tokens[0],"seederlist") == 0 )  //receive hash
    	{
          string SHA(tokens[1]);
          REPLY="";
          map_lock.lock();
          if( seedersHash.find(SHA) == seedersHash.end() ) //SHA string not present
          {
            REPLY = "NONE";
          }
          else
          {
            for( auto i = seedersHash[SHA].begin(); i != seedersHash[SHA].end() ; i++ )
              REPLY = *i + " " + REPLY;
          }
          map_lock.unlock();
          return REPLY;
      }

      else if( strcmp(tokens[0],"remove") == 0 && tokens[1]!=NULL && tokens[2]!=NULL )
    	{
        set<string> s;
        string SHA(tokens[1]);
        string client(tokens[2]);
        //string cliListenPort(tokens[2]);
        //client = client + ":" + cliListenPort;
        map_lock.lock();

        if( seedersHash[SHA].find(client) != seedersHash[SHA].end() )  //client not already present
        {
            seedersHash[SHA].erase(client);
            vector<string> file;
            string line,logCli;

            file_lock.lock();

            ifstream log(seeder_list);
            while ( getline (log,line) )
            {
                if( SHA == line )
                {
                    file.push_back(line);
                    getline(log,logCli);   //tokenize clients and store it in a set
                    token = strtok (&logCli[0]," ");
                    string cl(token);
                    if( cl == client )
                      cl="";
                    while (token != NULL)
                    {
                      token = strtok(NULL," ");
                      if(token!=NULL)
                      {
                        string cl1(token);
                        if( cl1 != client )  //remove client from seederlist file
                          cl = cl + " " + cl1;
                      }
                    }
                    file.push_back(cl);
                    //cout<<strlen(&cl[0])<<endl;
                    if( strlen(&cl[0]) < 9 ) //if no clients left
                    {
                        file.pop_back();
                        file.pop_back();
                        file.pop_back();
                        seedersHash.erase(SHA);
                    }
                }
                else
                    file.push_back(line);
              }
              log.close();
              ofstream log1 ( seeder_list, ios::ate );
              for(int i=0; i<file.size(); i++)
                  log1 << file[i] <<endl;
              log1.close();

              file_lock.unlock();
          }
          map_lock.unlock();
          REPLY = "Removed";

          if( !fromTracker )
          {
            struct sockaddr_in otherServAdd;
            memset(&otherServAdd, 0, sizeof(otherServAdd));

            otherServAdd.sin_family = AF_INET;  // internet address
            otherServAdd.sin_port = htons( atoi(OTHER_TRACKER_PORT) );
            otherServAdd.sin_addr.s_addr = inet_addr(OTHER_TRACKER_IP);

            char toOtherTracker[1024];
            strcpy(toOtherTracker,"tracker:");
            strcat(toOtherTracker,buf);

            sendto(sockfd, (const char *)toOtherTracker, strlen(toOtherTracker), MSG_CONFIRM, (const struct sockaddr *) &otherServAdd, sizeof(otherServAdd));
          }
          return REPLY;
        }

        else if( strcmp(tokens[0],"alive?") == 0 )  //by other tracker
      	{
            REPLY="alive";

            return REPLY;
        }

        else if( strcmp(tokens[0],"give_seederlist_file") == 0 )
        {
            ifstream file(seeder_list);
            if (file.is_open())
            {
              string str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
              REPLY = str;
            }
            else
            {
              REPLY = "EMPTY";
            }
            return REPLY;
        }
    }
  return REPLY;
}

void restore_seederlist(int sockfd)
{
    bool tracker_alive = false;
    char buffer [1024];

    struct sockaddr_in otherServAdd;
    memset(&otherServAdd, 0, sizeof(otherServAdd));

    otherServAdd.sin_family = AF_INET;  // internet address
    otherServAdd.sin_port = htons( atoi(OTHER_TRACKER_PORT) );
    otherServAdd.sin_addr.s_addr = inet_addr(OTHER_TRACKER_IP);

    char reqTracker[] = "alive?";
    unsigned int len;

    struct timeval tv;

    sendto(sockfd, (const char*)reqTracker, strlen(reqTracker), MSG_CONFIRM, (const struct sockaddr*)&otherServAdd, sizeof(otherServAdd));

    tv.tv_sec = 0;
    tv.tv_usec = 800000;  //0.8 seconds
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
    {
        perror("setsockopt Error");
    }

    recvfrom(sockfd, (char *)buffer, bufSize, MSG_WAITALL, (struct sockaddr *) &otherServAdd, &len);
    if( strcmp(buffer,"alive") == 0 )
    {
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
        {
            perror("setsockopt Error");
        }
        tracker_alive = true;
    }
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
    {
        perror("setsockopt Error");
    }

    if (tracker_alive)
    {
      buffer[0] = '\0';
      char reqFile[] = "give_seederlist_file";
      sendto(sockfd, (const char*)reqFile, strlen(reqFile), MSG_CONFIRM, (const struct sockaddr*)&otherServAdd, sizeof(otherServAdd));
      recvfrom(sockfd, (char *)buffer, bufSize, MSG_WAITALL, (struct sockaddr *) &otherServAdd, &len);
      if (strcmp(buffer, "EMPTY") != 0 && buffer != NULL)
      {
        ofstream file(seeder_list, ios::trunc | ios::out);
        string data(buffer);
        file << data;
        file.close();
      }
    }

    cout<<"is other tracker alive?: "<<tracker_alive<<endl;
    return;
}

int main( int argc, char** argv )
{
    if( argc < 4 )
    {
        cout<<"Insuffecient arguments"<<endl;
        exit(1);
    }

    string myIP( argv[1] );
    int colon = myIP.find(':');
    myIP.erase( colon, strlen(argv[1]) - colon );

    string myPORT( argv[1] );
    myPORT.erase( 0, colon+1 );

    char* MY_IP = &myIP[0];
    char* MY_PORT = &myPORT[0];


    string otherIP( argv[2] );
    colon = otherIP.find(':');
    otherIP.erase( colon, strlen(argv[2]) - colon );

    string otherPORT( argv[2] );
    otherPORT.erase( 0, colon+1 );

    strcpy( OTHER_TRACKER_IP, &otherIP[0]);
    strcpy(OTHER_TRACKER_PORT , &otherPORT[0]);

    strcpy( seeder_list, argv[3] );

    ifstream SL ( seeder_list );    // read all hashes stored in the log file
    if (SL.is_open())
    {
      string logSHA,logCli;
      string filename,cli1;
      set<string> s;
      char *token;
      while ( getline (SL,filename) )
      {
        getline(SL,logSHA);
        getline(SL,logCli);   //tokenize clients and store it in a set
        token = strtok (&logCli[0]," ");
        if(token!=NULL)
        {
          string client(token);
          s.clear();
          s.insert(client);
          while (token != NULL)
          {
            token = strtok(NULL," ");
            if(token!=NULL)
            {
              cli1 = string(token);
              s.insert(string(token) );
            }
          }
          seedersHash[logSHA] = s;
        }
      }
      SL.close();
    }

    int sockfd;
    char buffer[bufSize];

    struct sockaddr_in servAdd, cliAdd;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        perror("socket failed to create");
        exit(EXIT_FAILURE);
    }

    memset(&servAdd, 0, sizeof(servAdd));
    memset(&cliAdd, 0, sizeof(cliAdd));

    servAdd.sin_family = AF_INET; // internet address
    servAdd.sin_addr.s_addr = inet_addr( MY_IP );
    servAdd.sin_port = htons( atoi(MY_PORT) );

    if ( bind(sockfd, (const struct sockaddr *)&servAdd, sizeof(servAdd)) < 0 )
    {
        perror("bind function failed");
        exit(EXIT_FAILURE);
    }

    thread restore_seederlist_thread( restore_seederlist, sockfd );
    restore_seederlist_thread.detach();
    sleep(1);

    int n,pid;
    unsigned int len;
    while(1)
    {
      n = recvfrom(sockfd, (char *)buffer, bufSize, MSG_WAITALL, ( struct sockaddr *) &cliAdd, &len);
      buffer[n] = '\0';

      thread t( handleRequest, cliAdd, buffer, len, sockfd );
      t.detach();
    }

    return 0;
}

void handleRequest( struct sockaddr_in cliAdd, char* buf, unsigned int len, int sockfd )
{
  char buffer[1024];
  strcpy(buffer,buf);
  string CliIP( inet_ntoa(cliAdd.sin_addr) );
  bool fromTracker = false;
  if( buffer!=NULL )
  {
      string tr(buffer);
      if( tr.substr(0,8) == "tracker:" )
      {
          tr.erase(0,8);
          fromTracker = true;
          strcpy(buffer,&tr[0]);
      }
  }
  //int CliPort = ntohs(cliAdd.sin_port);  //give the tracker port that the client will listen on
  //string clientSocket = CliIP + ":" + to_string(CliPort);
  //cout<<"Client Address: "<<CliIP<<endl;
  printf("%s: %s\n", &CliIP[0], buffer);

  char reply[1024];
  string REPLY = processRequest( buffer, &CliIP[0], sockfd, fromTracker );
  strcpy( reply ,  &REPLY[0]);
  //cout<<buffer<<endl;

  //cout<< "bool    "<<fromTracker;
  if( !fromTracker )
  {
    //cout<<"sending..."<<endl;
    sendto(sockfd, (char *)reply, bufSize, MSG_CONFIRM, (const struct sockaddr *) &cliAdd, len);
  }
  //printf("Reply sent.\n");
  return;
}
