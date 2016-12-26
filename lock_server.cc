// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <map>
#include <pthread.h>

using namespace std;

pthread_mutex_t mutex_acquire;
pthread_mutex_t mutex_release;

map<lock_protocol::lockid_t,bool> lock_status;
map<int,bool> client_status;
map<lock_protocol::lockid_t,int> lock_client;

map<int,pthread_t> client_thread;

void *client_detect(void* var)
{
	lock_server* ls = *(lock_server**)var;
	int clt = *(int*)(var+sizeof(lock_server*));
	int r = *(int*)(var+sizeof(lock_server*)+sizeof(int));
	
	while(1)
	{
		client_status[clt] = false;
		usleep(50000);
		if(client_status[clt] == false)
		{
			map<lock_protocol::lockid_t,int>::iterator ite;
			for(ite = lock_client.begin();ite!=lock_client.end();ite++)
				if(ite->second == clt)
					ls->release(clt,ite->first,r);
			client_thread[clt] = -1;
			printf("release:clt%d\n",clt);
			return NULL;
		}
	}
}

lock_server::lock_server():
  nacquire (0)
{
	lock_status.clear();
	client_status.clear();
	lock_client.clear();
	client_thread.clear();
}

lock_protocol::status
lock_server::client_ok(int clt, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  client_status[clt] = true;
  return ret;
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
  if(lock_status.find(lid) == lock_status.end())
  	lock_status[lid] = 1;
  else
  {
  	/*if(lock_status[lid] == 1 && lock_client[lid] == clt)
  	{
  		pthread_mutex_unlock(&mutex_acquire);
  		return ret;
  	}*/
  	while(lock_status[lid] == 1)
  		usleep(10000);
  	lock_status[lid] = 1;
  	lock_client[lid] = clt;
  }
  
  if(client_thread.find(clt) == client_thread.end() || client_thread[clt] == -1)
  {
  	printf("acquire:clt%d\n",clt);
  	void *var = malloc(sizeof(lock_server*)+2*sizeof(int));
  	*(lock_server**)var = this;
  	*(int*)(var+sizeof(lock_server*)) = clt;
  	*(int*)(var+2*sizeof(lock_server*)) = r;
  	
  	pthread_t pid;
  	pthread_create(&pid,NULL,client_detect,var);
  	client_thread[clt] = pid;
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
  if(lock_status.find(lid) != lock_status.end())
  {
  	lock_status[lid] = 0;
  	lock_client[lid] = -1;
  }
  //printf("stat request from clt %d\n", clt);
  //r = nacquire;
  pthread_mutex_unlock(&mutex_release);
  return ret;
}
