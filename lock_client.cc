// RPC stubs for clients to talk to lock_server

#include "lock_client.h"
#include "rpc.h"
#include <arpa/inet.h>

#include <sstream>
#include <iostream>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void* client_status(void* client);

lock_client::lock_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() < 0) {
    printf("lock_client: call bind\n");
  }
  
  pthread_t pid;
  pthread_create(&pid,NULL,client_status,(void*)this);
}

int
lock_client::stat(lock_protocol::lockid_t lid)
{
  int r;
  lock_protocol::status ret = cl->call(lock_protocol::stat, cl->id(), lid, r);
  VERIFY (ret == lock_protocol::OK);
  return r;
}

lock_protocol::status
lock_client::acquire(lock_protocol::lockid_t lid)
{
	// Your lab4 code goes here
	int r;
	lock_protocol::status ret = cl->call(lock_protocol::acquire, cl->id(), lid, r);
	VERIFY (ret == lock_protocol::OK);
	return r;
}

lock_protocol::status
lock_client::release(lock_protocol::lockid_t lid)
{
	// Your lab4 code goes here
	int r;
	lock_protocol::status ret = cl->call(lock_protocol::release, cl->id(), lid, r);
	VERIFY (ret == lock_protocol::OK);
	return r;
}

lock_protocol::status
lock_client::client_ok()
{
	// Your lab4 code goes here
	int r;
	lock_protocol::status ret = cl->call(lock_protocol::client_ok, cl->id(), r);
	VERIFY (ret == lock_protocol::OK);
	return r;
}

void* client_status(void* client)
{
	lock_client* lc = (lock_client*) client;
	while(1)
	{
		usleep(10000);
		lc->client_ok();
	}
}
