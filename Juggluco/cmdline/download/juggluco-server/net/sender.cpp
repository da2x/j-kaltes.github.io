//#include <arpa/inet.h>
       #include <sys/types.h>
       #include <sys/socket.h>
       #include <netdb.h>
#include <arpa/inet.h>
       #include <sys/socket.h>
              #include <unistd.h>

#include <iostream>
#include <string.h>
#include <atomic>
#include <thread>
#include <algorithm>
//#include <latch>
#include <poll.h>

       #include <sys/prctl.h>

#include "destruct.h"

#include "logs.h"
#include "netstuff.h"
#include "passhost.h"
#include "crypt.h"
#include "makerandom.h"
using namespace std;

void	sendpassinit(int sock,passhost_t *host,crypt_t *ctx) {
	constexpr int makelen=8;
	uint8_t nonce[ASCON_AEAD_NONCE_LEN];
	constexpr int takelen=ASCON_AEAD_NONCE_LEN-makelen;
	uint8_t *takestart=nonce+makelen;
       makerandom(nonce, makelen);
	if(sendni(sock,nonce,makelen)!=makelen) {
		lerror("sendpassinit send");
		return;
		}
	int len=recvni(sock,takestart,takelen);
	if(len!=takelen) {
		lerror("recv");
		return;
		}
        ascon_aead128a_init(ctx, host->pass.data(),nonce);	LOGGER("end sendpassinit\n");
	}
bool unblock(int sock) {
  if( int val = fcntl(sock, F_GETFL, NULL);val >=0) {
	  val|=O_NONBLOCK;
	  if( fcntl(sock, F_SETFL,val) < 0) {
     		flerror("fcntl(%d, F_SETFL,%d)",sock,val);
     		return false;
		} 
	return true;
	} 
  else {
	     flerror("fcntl(%d, F_GETFL)",sock);
	     return false;
	     }
     }
bool block(int sock) {
  if( int val = fcntl(sock, F_GETFL, NULL);val >=0) {
	  val&=(~O_NONBLOCK);
	  if( fcntl(sock, F_SETFL,val) < 0) {
     		flerror("fcntl(%d, F_SETFL,%d)",sock,val);
     		return false;
		} 
	return true;
	} 
  else {
	     flerror("fcntl(%d, F_GETFL)",sock);
	     return false;
	     }
     }


#include "sendmagic.h"
auto getsendmagic() {
	std::array<unsigned char,sizeof(sendmagic)> back=sendmagicinit;
	makerandom(end(back)-4,4);
	return back;
	}
std::array<unsigned char,sizeof(sendmagic)>  sendmagicspec=getsendmagic();


bool testsendmagic(int sock) {
	#include "receivemagic.h"

	if(sendni(sock,sendmagicspec.data(),sendmagicspec.size())!=sendmagicspec.size()) {
		lerror("send magic failed\n");
		return false;
		}
	constexpr int buflen=1024;
	char buf[buflen];
	LOGGER("before recv magic ");
	int gotlen;
	if((gotlen=recvni(sock,buf,buflen))!=sizeof(receivemagic)) {
		flerror("recv()=%d!=%d\n",gotlen,sizeof(receivemagic));
		return false;
		}
	LOGGER("after recv magic ");
	if(memcmp(buf,receivemagic,sizeof(receivemagic)-4)) {//4 less for version info
		LOGGER("Wrong magic\n");
		return false;
		}
	extern void	setreceiverversion(uint8_t version) ;
	LOGGER("testsendmagic %d success\n",sock);
	setreceiverversion(buf[sizeof(receivemagic)-1]);
	return true;
	}

void receivetimeout(int sock,int secs) {
	struct timeval tv;
	tv.tv_usec = 0;
	tv.tv_sec = secs;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	}
void sendtimeout(int sock,int secs) {
	struct timeval tv;
	tv.tv_usec = 0;
	tv.tv_sec = secs;
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO , (const char*)&tv, sizeof tv);
	}
//extern void getmyname(int sock) ;


bool sendtype(int sock,char type) {
	char ant=type;
	if(sendni(sock,&ant,1)!=1) {
		return false;
		}
	return true;
        }

int shakehands(passhost_t *pass,int &sock,crypt_t*ctx,char stype) {
	LOGGER("opensender connection %d\n",sock);
	//getmyname(sock);
	
	destruct closer([&sock]()->void{
		int so=sock;
		sock=-1;
		close(so);; });
	
/*SO_RCVTIMEO and SO_SNDTIMEO
Specify the receiving or sending timeouts until reporting an error. The argument is a struct timeval. If an input or output function blocks for this period of time, and data has been sent or received, the return value of that function will be the amount of data transferred; if no data has been transferred and the timeout has been reached then -1 is returned with errno set to EAGAIN or EWOULDBLOCK, or EINPROGRESS (for connect(2)) just as if the socket was specified to be nonblocking. If the timeout is set to zero (the default) then the operation will never timeout. Timeouts only have effect for system calls that perform socket I/O (e.g., read(2), recvmsg(2), send(2), sendmsg(2)); timeouts have no effect for select(2), poll(2), epoll_wait(2), and so on.*/
/*
#ifndef NOTIMEOUT
	struct timeval tv;
	tv.tv_usec = 0;
	tv.tv_sec = 60*4;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	tv.tv_sec = 60*20; 
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO , (const char*)&tv, sizeof tv);
#endif */
/*	const int minimumsize = 1;
	setsockopt(sock, SOL_SOCKET, SO_RCVLOWAT, &minimumsize, sizeof minimumsize);
	setsockopt(sock, SOL_SOCKET, SO_SNDLOWAT, &minimumsize, sizeof minimumsize); */
	if(pass->hasname) {
		const char *name= pass->getname();
		if(sendni(sock,name,pass->maxnamelen)!=pass->maxnamelen) {
			lerror("send name");
			return -1;
			}
		
		}
	if(!testsendmagic(sock)) 
		return -1;
	if(stype) {
		sendtype(sock,stype);
		}
		
	if(ctx)
		sendpassinit(sock,pass,ctx);

	closer.active=false;
	LOGGER("sock=%d\n",sock);
	return sock;
	}
/*
sockaddr myname;
bool nameset=false;

void getmyname(int sock) {
	 socklen_t  namelen= sizeof(myname);
	if(getsockname(sock,  &myname,&namelen )==-1) {
		lerror("getsockname");
		}
	else {
		nameset=true;
		namehost mname(&myname);
		LOGGER("getsockname returned %s\n",mname.data());
		}
	} 
	*/
int opensender(passhost_t *pass,int &sock,crypt_t*ctx,char stype) {

	sock=-1;
	const int nr=pass->nr;
	LOGGER("opensender nr=%d\n",nr);
	if(nr<=0) {
		return -1;
		}
//	struct sockaddr_in6  *sin=&pass->connect;
	struct pollfd	 cons[nr];
	int use=0;
	for(int i=0;i<nr;i++) {
		struct sockaddr_in6  *sin=&pass->ips[i];
		int so;
		namehost name(sin);
		LOGGER("%s family=%d\n", name.data(),sin->sin6_family);
		if((so=socket(sin->sin6_family,SOCK_STREAM,0))==-1) {
			flerror("openone %d: socket",i);
			continue;
			}

		if(!unblock(so)) {
			close(so);
			continue;
			}
		LOGGER("try  %s sock=%d ", name.data(),so);
		if(connect(so,(const struct sockaddr* )sin,sizeof(*sin))==-1) {
			if(errno == EINPROGRESS) {
				LOGGER("%d progress\n",so);
				cons[use++]={so,POLLOUT,0};
				}
			else {
				LOGGER("close\n");
				close(so);
				continue;
				}
			}
		else {
			block(so);
			sock=so;
			if(int ret=shakehands(pass,sock,ctx,stype);ret>0) {
				LOGGER("before poll %d\n",sock);
				for(int w=0;w<use;w++) {
					close(cons[w].fd);
					}
				return ret;
				}
			continue;		
			}
		}
	while(use) {	
		int errcode=poll(cons, use, 60000);
		switch(errcode) {
			case -1: {
				int er=errno;
				lerror("poll");
				if(er== EINTR)
					continue;
				return -1;
				};
			case 0: {LOGGER("poll timeout\n");
			/* Already closed?
				for(int i=0;i<use;i++)
					if(cons[i].fd>=0)
						close(cons[i].fd);  */
				return -1;
				}
			};
		int newuse=0;
		for(int i=0;i<use;i++) {
			if(cons[i].revents & POLLRDHUP){
				LOGGER(" %d: POLLRDHUP\n",cons[i].fd);
				close(cons[i].fd); 
				continue;
				}
			if(cons[i].revents &POLLERR){
				int error = 0;
				socklen_t errlen = sizeof(error);
				if(getsockopt(cons[i].fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen)==-1)
					lerror("getsockopt");
//s/^[ 	]*\(E[A-Z]*\)[ 	]*\(.*\)$/case \1: errstr=\"\2\";break;
				const char *errstr="";
				switch(error) {
					case EINTR: errstr="The system call was interrupted by a signal that was caught; see signal(7).";break;

					case EISCONN: errstr="The socket is already connected.";break;
					case EHOSTUNREACH: errstr="Host is unreachable.";break;

					case ENETUNREACH: errstr="Network is unreachable.";break;

					case ENOTSOCK: errstr="The file descriptor sockfd does not refer to a socket.";break;

					case EPROTOTYPE: errstr="The  socket type does not support the requested communications protocol.  This error can occur, for example, on an attempt to connect a UNIX domain datagram socket to a stream socket.";break;

					case ETIMEDOUT: errstr="Timeout while attempting connection.  The server may be too busy to accept new connections.  Note that for IP sockets the timeout may be very long when syncookies are enabled on the server.";break;

					};
				LOGGER(" %d: POLLERR %s (%d)\n",cons[i].fd,errstr,error);
				close(cons[i].fd); 
				continue;
				}
			if(cons[i].revents &POLLHUP){
				LOGGER(" %d: POLLHUP\n",cons[i].fd);
				close(cons[i].fd);
				continue;
				}
			if(cons[i].revents &POLLNVAL){
				LOGGER(" %d: POLLNVAL\n",cons[i].fd);
				close(cons[i].fd);
				continue;
				}

			if (cons[i].revents & POLLOUT){
				sock=cons[i].fd;
				block(sock);
				if(int ret=shakehands(pass,sock,ctx,stype);ret>0) {
					for(int w=0;w<newuse;w++) {
						close(cons[w].fd);
						}
					for(int w=i+1;w<use;w++) {
						close(cons[w].fd);
						}
					LOGGER(" via poll %d\n",sock);
					return ret;	
					}
				continue;
				}
			else  {
				LOGGER("poll again %d\n",cons[i].fd);
				cons[newuse++]={cons[i].fd,POLLOUT,0};
				}
			}
		use=newuse;
		}
	LOGGER("no one\n");
	return -1;
	}


#ifdef MAIN

int main(int argc, char **argv) {
//	int sock=opensender("192.168.1.69","12345");
const char *name=argv[1],*port=argv[2];
struct in6_addr  addr= getaddr(name, port) ;
for(int i=0;i<16;i++) 
#ifdef __ANDROID__
	printf("%d ",addr.in6_u.u6_addr8[i]);
#else	
	printf("%d ",addr.__in6_u.__u6_addr8[i]);
	#endif

printf("/n");
int sock;
int res=opensender(addr,port,sock);
	}
#endif
