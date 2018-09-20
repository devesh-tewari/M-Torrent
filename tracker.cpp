#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <cstring>
#include <vector>
#include <bits/stdc++.h>
#include <iostream>
#include <thread>

using namespace std;

vector<string> decidePieces(vector<string> av, int c, int len)
{
  if( av.size() == 1 )
    return av;

  int i,j;

  pair<int,int> p;  // <number of pieces, client number>
  vector< pair<int,int> > no_of_pieces;  //stores number of pieces a clientNumber has

  string init = "";
  for(i=0; i<len; i++)
    init = init + "0";

  vector<string> selected(c,init);  //which pieces are decided currently for which client
  bool Sel[len] = {false};
  bool notDone = true;

  int pieces;
  for(i=0; i<c; i++)
  {
    pieces = 0;
    for(j=0; j<len; j++)  //count the number of pieces at client i
    {
      selected[i][j] = '0';
      if( av[i][j] == '1' )
        pieces++;
    }
    p = make_pair(pieces,i);
    no_of_pieces.push_back(p);
  }

  int min;
  sort( no_of_pieces.begin(), no_of_pieces.end() );//the client number will remain intact because of pair

  int count;
  int n;
  n = (float)len / (float)c  ;  //atmost pieces to take from each client(initially)

  while( notDone )
  {
      for(i=0; i<c; i++)
      {
          int min = no_of_pieces[i].second;
          count = 0;
          for(j=0; j<len; j++)
          {
              if( av[min][j] == '1' && Sel[j] == false )
              {
                  Sel[j] = true;
                  selected[min][j] = '1';
                  count++;
                  if( count >= n )  //select atmost n to divide equally
                  {
                      break;
                  }
              }
          }
      }

      notDone = false;
      for(i=0; i<len; i++)   //check if all pieces are selected
      {
          if( Sel[i] == false )
          {
            notDone = true;
            break;
          }
      }

      n++;  // if there are some pieces left, increase pieces per client by 1

  }

  return selected;

}

int main()
{
  vector<string> v;
  v.push_back("11111100000000001111");
  v.push_back("00000000100000011001");
  v.push_back("11111111111100000000");
  v.push_back("01111100001111110011");
  vector<string> s( decidePieces(v,4,20) );
  for(int i=0; i<s.size(); i++)
    cout<<s[i]<<endl;
  return 0;
}
