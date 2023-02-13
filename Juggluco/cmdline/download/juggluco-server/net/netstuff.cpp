
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "destruct.h"
#include "logs.h"
#include "netstuff.h"
#include <ifaddrs.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <dlfcn.h>
#include <linux/if.h>



bool  getaddr(const struct sockaddr *sa, struct sockaddr_in6  *uit) {
	switch(sa->sa_family) {
		case AF_INET: 
			{
			*uit={.sin6_family=AF_INET6, .sin6_port=((struct sockaddr_in *)sa)->sin_port, .sin6_addr=v426(sa)};	
			return true;
			}
		case AF_INET6: *uit=*((struct sockaddr_in6*)sa);return true;
		default: return false; 
		}
	}
bool   getaddr(const char *host, const char *port, struct sockaddr_in6  *uit) {
	struct addrinfo hints{.ai_flags=AI_ADDRCONFIG|AI_NUMERICHOST,.ai_family=AF_UNSPEC,.ai_socktype=SOCK_STREAM};
	struct addrinfo *servinfo=nullptr;
	destruct serv([&servinfo]{ if(servinfo)freeaddrinfo(servinfo);});
	if(int status=getaddrinfo(host,port,&hints,&servinfo)) {
		LOGGER("getaddrinfo: %s:%s %s\n",host,port,gai_strerror(status));
		return false; 
		}
	return getaddr(servinfo->ai_addr,uit);
	}
#if 0
        struct sockaddr_in6 {
               sa_family_t     sin6_family;   /* AF_INET6 */
               in_port_t       sin6_port;     /* port number */
               uint32_t        sin6_flowinfo; /* IPv6 flow information */
               struct in6_addr sin6_addr;     /* IPv6 address */
               uint32_t        sin6_scope_id; /* Scope ID (new in 2.4) */
           };

           struct in6_addr {
               unsigned char   s6_addr[16];   /* IPv6 address */
           };
#endif 
/*

struct in6_addr {
  union {
    __u8 u6_addr8[16];

    __be16 u6_addr16[8];
    __be32 u6_addr32[4];

  } in6_u;

};

struct in6_addr
  {
    union
      {
 uint8_t __u6_addr8[16];
 uint16_t __u6_addr16[8];
 uint32_t __u6_addr32[4];
      } __in6_u;





  };

*/
/* IPv6 Wildcard Address (::) and Loopback Address (::1) defined in RFC2553
 * NOTE: Be aware the IN6ADDR_* constants and in6addr_* externals are defined
 * in network byte order, not in host byte order as are the IPv4 equivalents
 */
#define logstring(x) LOGGERN(x,sizeof(x)-1)
void showflags(int flags) {
	if(flags&IFF_UP) logstring("IFF_UP            Interface is running.");
	if(flags&IFF_BROADCAST) logstring("IFF_BROADCAST     Valid broadcast address set.");
	if(flags&IFF_DEBUG) logstring("IFF_DEBUG         Internal debugging flag.");
	if(flags&IFF_LOOPBACK) logstring("IFF_LOOPBACK      Interface is a loopback interface.");
	if(flags&IFF_POINTOPOINT) logstring("IFF_POINTOPOINT   Interface is a point-to-point link.");
	if(flags&IFF_RUNNING) logstring("IFF_RUNNING       Resources allocated.");
	if(flags&IFF_NOARP) logstring("IFF_NOARP         No arp protocol, L2 destination address not set.");
	if(flags&IFF_PROMISC) logstring("IFF_PROMISC       Interface is in promiscuous mode.");
	if(flags&IFF_NOTRAILERS) logstring("IFF_NOTRAILERS    Avoid use of trailers.");
	if(flags&IFF_ALLMULTI) logstring("IFF_ALLMULTI      Receive all multicast packets.");
	if(flags&IFF_MASTER) logstring("IFF_MASTER        Master of a load balancing bundle.");
	if(flags&IFF_SLAVE) logstring("IFF_SLAVE         Slave of a load balancing bundle.");
	if(flags&IFF_MULTICAST) logstring("IFF_MULTICAST     Supports multicast");
	if(flags&IFF_PORTSEL) logstring("IFF_PORTSEL       Is able to select media type via ifmap.");

	if(flags&IFF_AUTOMEDIA) logstring("IFF_AUTOMEDIA     Auto media selection active.");
	if(flags&IFF_DYNAMIC) logstring("IFF_DYNAMIC       The addresses are lost when the interface goes down.");
	if(flags&IFF_LOWER_UP) logstring("IFF_LOWER_UP      Driver signals L1 up (since Linux 2.6.17)");
	if(flags&IFF_DORMANT) logstring("IFF_DORMANT       Driver signals dormant (since Linux 2.6.17)");
	if(flags&IFF_ECHO) logstring("IFF_ECHO          Echo sent packets (since Linux 2.6.25)");
	}


extern bool getownip(struct sockaddr_in6 *outip);
//extern "C"   int getifaddrs(struct ifaddrs **ifap);

#ifndef _SIZEOF_ADDR_IFREQ
#define _SIZEOF_ADDR_IFREQ sizeof
#endif
bool oldgetownip(struct sockaddr_in6 *outip) { 
  int  socketfd = socket(AF_INET, SOCK_DGRAM, 0);
  if(socketfd<0) {
  	LOGGER(" socket(AF_INET, SOCK_DGRAM, 0) failed\n");
 	return false; 	
	}
  destruct _des([socketfd] {close(socketfd);});
  struct ifreq ifrbuf[20];
  struct ifconf conf{.ifc_len = sizeof(ifrbuf), .ifc_ifcu{.ifcu_req = ifrbuf}};
    if (ioctl(socketfd,SIOCGIFCONF,&conf) < 0) {
      lerror("ioctl");
      return false;
    }
    const char *endifr=(char *)&ifrbuf[10];
   const struct ifreq *wlanip6=nullptr;
    for(struct ifreq *ifr=ifrbuf;(char*)ifr < endifr; ifr = (struct ifreq*)((char*)ifr +_SIZEOF_ADDR_IFREQ(*ifr))) {
	constexpr const char wlan[]="wlan";
	if(!memcmp(ifr->ifr_name,wlan,sizeof(wlan)-1)) {
	      switch (ifr->ifr_addr.sa_family) {
				case AF_INET: return  putip(&ifr->ifr_addr,outip);;
				case AF_INET6: wlanip6=ifr; break;
				}
		      }

      	}

    if(!wlanip6)  {
	LOGGER("No wlan\n");
	return false;
	}
   return putip(&wlanip6->ifr_addr,outip);
   }

bool getownip(struct sockaddr_in6 *outip) {
   LOGGER("getownip\n");
    struct ifaddrs *ifaddr;
   typedef int (*getifaddrs_t)(struct ifaddrs **ifap);
   static int (*getifaddrs)(struct ifaddrs **ifap)=(getifaddrs_t)dlsym( RTLD_DEFAULT, "getifaddrs");
   if(!getifaddrs) {
      lerror("getifaddrs==null");
      return false;
   	}
   if(getifaddrs(&ifaddr) == -1) {
      lerror("getifaddrs");
      return false;
      }
  typedef void (*freeifaddrs_t)(struct ifaddrs *ifa);
  static freeifaddrs_t freeifaddrs=(freeifaddrs_t)dlsym( RTLD_DEFAULT, "freeifaddrs");

    destruct _dest([ifaddr]{freeifaddrs(ifaddr);});

   const struct ifaddrs *wlanip6=nullptr;
   for(const struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;
       const int family = ifa->ifa_addr->sa_family;
	namehost name(ifa->ifa_addr);
	LOGGER("%s %s fam=%d\n",ifa->ifa_name,name.data(),family);
	const int flags= ifa->ifa_flags;
	showflags(flags);
	constexpr const char wlan[]="wlan";
	if(!memcmp(ifa->ifa_name,wlan,sizeof(wlan)-1)) {
		switch(family) {
	       		case AF_INET: return  putip(ifa->ifa_addr,outip);;
			case AF_INET6: wlanip6=ifa; break;

			}
	      }
	  }
  if(!wlanip6)  {
	LOGGER("No wlan\n");
	return false;
	}
   return putip(wlanip6->ifa_addr,outip);
   }


bool usefullflags(int flags) {
 constexpr const int should=IFF_UP  | IFF_RUNNING;
 constexpr const int shouldnot= IFF_LOOPBACK ;
 return (flags&should)==should && !(flags&shouldnot);
 }

/*
struct ifconf  {
        int     ifc_len;                        / size of buffer       
        union {
                char *ifcu_buf;
                struct ifreq *ifcu_req;
        } ifc_ifcu;
};
*/

int oldgetownips(struct sockaddr_in6 *outips,int max) { 
LOGGER("oldgetownips\n");
  int  socketfd = socket(AF_INET, SOCK_DGRAM, 0);
  if(socketfd<0) {
  	LOGGER(" socket(AF_INET, SOCK_DGRAM, 0) failed\n");
 	return 0; 	
	}
  destruct _des([socketfd] {close(socketfd);});
  struct ifreq ifrbuf[20];
  struct ifconf conf{.ifc_len = sizeof(ifrbuf), .ifc_ifcu{.ifcu_req = ifrbuf}};
    if (ioctl(socketfd,SIOCGIFCONF,&conf) < 0) {
      lerror("ioctl");
      return 0;
    }
    const char *endifr=(char *)&ifrbuf[10];
    int iter = 0;
    for(struct ifreq *ifr=ifrbuf;(char*)ifr < endifr; ifr = (struct ifreq*)((char*)ifr +_SIZEOF_ADDR_IFREQ(*ifr))) {
      switch (ifr->ifr_addr.sa_family) {
        case AF_INET:
	    if(!strcmp("lo",ifr->ifr_name)||!strncmp("dummy",ifr->ifr_name,5))
		continue;
	    putip(&ifr->ifr_addr,outips+iter++);
            break;
      	}
    }
  return iter;
}
//#define OLDOWNIPS 1
#ifdef OLDOWNIPS
int getownips(struct sockaddr_in6 *outips,int max) {
      return oldgetownips(outips,max);
      }
#else

int getownips(struct sockaddr_in6 *outips,int max) {
   LOGGER("getownips\n");
    struct ifaddrs *ifaddr;
   typedef int (*getifaddrs_t)(struct ifaddrs **ifap);
   static int (*getifaddrs)(struct ifaddrs **ifap)=(getifaddrs_t)dlsym( RTLD_DEFAULT, "getifaddrs");
   if(!getifaddrs) {
      lerror("getifaddrs==null");
      return oldgetownips(outips,max);
   	}
   if(getifaddrs(&ifaddr) == -1) {
      lerror("getifaddrs");
      return oldgetownips(outips,max);
      }
  typedef void (*freeifaddrs_t)(struct ifaddrs *ifa);
  static freeifaddrs_t freeifaddrs=(freeifaddrs_t)dlsym( RTLD_DEFAULT, "freeifaddrs");

    destruct _dest([ifaddr]{freeifaddrs(ifaddr);});

   const struct ifaddrs *wlanip6=nullptr;
   const struct ifaddrs *wlanip=nullptr;
   max--;
   int iter=0;
   for(const struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
#ifndef NOLOG
			if(ifa->ifa_name)
				LOGGER("skip %s\n",ifa->ifa_name);
#endif
            continue;
		}
	const int flags= ifa->ifa_flags;
       const int family = ifa->ifa_addr->sa_family;
	namehost name(ifa->ifa_addr);
	LOGGER("%s %s fam=%d\n",ifa->ifa_name,name.data(),family);
	showflags(flags);
	if(usefullflags(flags)) {
		 const char wlan[]="wlan";
		if(!memcmp(ifa->ifa_name,wlan,sizeof(wlan)-1)) {
			showflags(flags);
			switch(family) {
				case AF_INET: wlanip=ifa;break;
				case AF_INET6: wlanip6=ifa; break;

				}

		      }
		     else  {
		       if(family==AF_INET) {
		       		if(iter<max)  {
				     LOGGER("take\n");
				     putip(ifa->ifa_addr,outips+iter++);
				     }
		       		}
			}
		  }
	}
    if(!wlanip) {
    	if(wlanip6)  {
		LOGGER("using ip6 \n");
		wlanip=wlanip6;
		}
	}
 if(wlanip)
	   putip(wlanip->ifa_addr,outips+iter++);
   if(iter)
	   return iter;

    return oldgetownips(outips,max);
   }
#endif

#ifdef MAIN2
int main(int argc,char **argv) {

struct sockaddr_in6  uit;
getaddr(argv[1],argv[2],&uit);
char name[INET6_ADDRSTRLEN];
inet_ntop( AF_INET6,&uit.sin6_addr ,name, sizeof name);
printf("inet_ntop=%s\n",name);
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
//   if (getnameinfo((sockaddr *)&addr, addrlen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV) == 0)
   if (getnameinfo((sockaddr *)&uit, sizeof(uit), hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),0) == 0)
               printf("host=%s, serv=%s\n", hbuf, sbuf);
namehost name2(&uit);
printf("host2=%s\n",name2.data());
}

	#endif
