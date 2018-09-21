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

#define port 8002
#define otherTrackerIP 0
#define otherTrackerPort 0
#define bufSize 1024

using namespace std;

map < string, set<string> > seedersHash;  // stores the mapping of SHA hash to seeding clients

void handleRequest( struct sockaddr_in, char*, unsigned int, int );

string REPLY;
char* processRequest(char* buf, char* addr)
{
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
char logfile[] = "logfile.txt";
  if( size > 1 )
  {
    if( strcmp(tokens[0],"share") == 0 )
  	{
      set<string> s;
      string SHA(tokens[2]);
      string client(addr);
      string cliListenPort(tokens[3]);
      client = client + ":" + cliListenPort;

      if( seedersHash.find(SHA) == seedersHash.end() ) //SHA string not already present
      {
        s.insert(client);
        seedersHash[SHA] = s;
        ofstream log ( logfile, ios::ate | ios::app);
        log << tokens[1] <<endl;
        log << SHA <<endl;
        log << client <<endl;
        log.close();
      }
      else   // SHA string already present
      {
        if( seedersHash[SHA].find(client) == seedersHash[SHA].end() )  //client not already present
        {
          seedersHash[SHA].insert(client);
          vector<string> file;
          string line,logCli;
          ifstream log(logfile);
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
            ofstream log1 ( logfile, ios::ate );
            for(int i=0; i<file.size(); i++)
                log1 << file[i] <<endl;
            log1.close();
          }
        }
        REPLY = "Shared";
        buf = &REPLY[0];
        return buf;
      }

      else if( strcmp(tokens[0],"seederlist") == 0 )  //receive hash
    	{
          string SHA(tokens[1]);
          REPLY="";
          if( seedersHash.find(SHA) == seedersHash.end() ) //SHA string not present
          {
            REPLY = "NONE";
          }
          else
          {
            for( auto i = seedersHash[SHA].begin(); i != seedersHash[SHA].end() ; i++ )
              REPLY = *i + " " + REPLY;
          }
          buf = &REPLY[0];
          return buf;
      }

      else if( strcmp(tokens[0],"remove") == 0 && tokens[1]!=NULL && tokens[2]!=NULL )
    	{
        set<string> s;
        string SHA(tokens[1]);
        string client(addr);
        string cliListenPort(tokens[2]);
        client = client + ":" + cliListenPort;

        if( seedersHash[SHA].find(client) != seedersHash[SHA].end() )  //client not already present
        {
            seedersHash[SHA].erase(client);
            vector<string> file;
            string line,logCli;
            ifstream log(logfile);
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
                    cout<<strlen(&cl[0])<<endl;
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
              ofstream log1 ( logfile, ios::ate );
              for(int i=0; i<file.size(); i++)
                  log1 << file[i] <<endl;
              log1.close();
          }
          REPLY = "Removed";
          buf = &REPLY[0];
          return buf;
        }
    }
  return buf;
}

int main()
{
    char logfile[] = "logfile.txt";
    ifstream log ( logfile );    // read all hashes stored in the log file
    if (log.is_open())
    {
      string logSHA,logCli;
      string filename,cli1;
      set<string> s;
      char *token;
      while ( getline (log,filename) )
      {
        getline(log,logSHA);
        getline(log,logCli);   //tokenize clients and store it in a set
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
      log.close();
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
    servAdd.sin_addr.s_addr = INADDR_ANY;
    servAdd.sin_port = htons(port);

    if ( bind(sockfd, (const struct sockaddr *)&servAdd, sizeof(servAdd)) < 0 )
    {
        perror("bind function failed");
        exit(EXIT_FAILURE);
    }

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

void handleRequest( struct sockaddr_in cliAdd, char* buffer, unsigned int len, int sockfd )
{
  string CliIP( inet_ntoa(cliAdd.sin_addr) );
  //int CliPort = ntohs(cliAdd.sin_port);  //give the tracker port that the client will listen on
  //string clientSocket = CliIP + ":" + to_string(CliPort);
  cout<<"Client Address: "<<CliIP<<endl;
  printf("Client : %s\n", buffer);

  char reply[1024];
  strcpy( reply , processRequest( buffer, &CliIP[0] ) );
  //cout<<buffer<<endl;

  cout<<strlen(reply)<<endl;
  sendto(sockfd, (char *)reply, bufSize, MSG_CONFIRM, (const struct sockaddr *) &cliAdd, len);
  printf("Reply sent.\n");
  return;
}
