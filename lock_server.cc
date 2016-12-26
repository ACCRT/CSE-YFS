// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <map>

using namespace std;

pthread_mutex_t mutex_acquire;
pthread_mutex_t mutex_release;

map<lock_protocol::lockid_t,bool> lmap;

lock_server::lock_server():
  nacquire (0)
{
	lmap.clear();
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
	// Your lab4 code goes here
  pthread_mutex_lock(&mutex_acquire);
  if(lmap.find(lid) == lmap.end())
  	lmap[lid] = 1;
  else
  {
  	while(lmap[lid] == 1)
  		sleep(1);
  	lmap[lid] = 1;
  }
  	
  //printf("stat request from clt %d\n", clt);
  //r = nacquire;
  pthread_mutex_unlock(&mutex_acquire);
  return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
	// Your lab4 code goes here
  pthread_mutex_lock(&mutex_release);
  if(lmap.find(lid) != lmap.end())
  	lmap[lid] = 0;
  //printf("stat request from clt %d\n", clt);
  //r = nacquire;
  pthread_mutex_unlock(&mutex_release);
  return ret;
}
