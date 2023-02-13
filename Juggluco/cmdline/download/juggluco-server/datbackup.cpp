#include "datbackup.h"
#include "nums/numdata.h"
void receivetimeout(int sock,int secs) ;
void sendtimeout(int sock,int secs) ;

extern void stopreceiver() ;
constexpr const char orsettings[]="orsettings.dat";

extern std::vector<Numdata*> numdatas;
 void uptodate(int ind) {
 	LOGGER("uptodate %d\n",ind);
	lastuptodate[ind]=time(nullptr);
	}

void uptodate(passhost_t *host) {
	if(backup)
		uptodate(host-backup->getupdatedata()->allhosts);
	}
int updateone::update() {
	if(getsock()<0)
		return 0;
	crypt_t *pass=getcrypt();
	bool sendsensors=(sendstream||sendscans) ;
	int ret =0;
	if(starttime) {
		if(sendnums) {
			if(nums[0].lastlastpos==0&&nums[1].lastlastpos==0) {
				if(starttime==1)  {
					for(auto el:numdatas)
						if(! el->sendbackupinit(pass,getsock(),nums) )
							return 0;
					}
				else   {
					bool numsbackupsendinit(crypt_t*pass,int sock,struct changednums *nuall,uint32_t starttime) ;
					if(!numsbackupsendinit(pass,getsock(),nums, starttime) )
						return 0;
					}
				}
			}

		if(starttime!=1)  {
			if(sendsensors) {
				if( !sensors->setbackuptime(pass, getsock(),ind,starttime)) 
					return 0;
				}
			}
		starttime=0;
		ret=1;
	   }

	int subdid=updatenums();
	if(!subdid)
		return 0;
	ret|=subdid;
	if(sendsensors) {
		subdid=sensors->update(pass,getsock(),ind,startsensors,firstsensor,sendstream,sendscans,restore);
		if(subdid&4) {
			resetdevices=true;
			subdid&=3;
			}
		if(!subdid)
			return 0; 
		ret|=subdid;
		}
	auto update= settings->getupdate();
	static bool init=true;
	if(update>updatesettings||init) {
		init=false;
		std::vector<subdata> vect;
		vect.reserve(2+(restore?1:0));
		if(sendnums) {
			bool nochangenum =true;
			vect.push_back({reinterpret_cast<const senddata_t *>(&nochangenum),offsetof(Tings,nochangenum),sizeof(nochangenum)});
			}
		constexpr const int  sharedstart=offsetof(Tings, update);
		constexpr const int len=offsetof(Tings, reserved4)-sharedstart;
		vect.push_back({reinterpret_cast<const senddata_t *>( settings->data())+sharedstart,sharedstart,len});
		if(restore) {
			Readall<senddata_t> orset(pathconcat(globalbasedir,orsettings));
			vect.push_back({orset.data(),0,orset.length()});
			if(!senddata(pass,getsock(),vect,settingsdat) ) 
				return 0;
			}
		else {
			if(!senddata(pass,getsock(),vect,settingsdat) ) 
				return 0;
			if(!senddata(pass,getsock(),0, reinterpret_cast<const senddata_t *>( settings->data()),	sharedstart,orsettings))
				return 0;
			}
		ret=1;
		}
	updatesettings=update;

	if(restore) {
		Readall<senddata_t> back(pathconcat(globalbasedir,orbackup));
		if(back) {
			if(!senddata(pass,getsock(),0, back.data(),back.size(),backupdat))
				return 0;
			}
		ret=1;
		}
	else
	if(!backupupdated) {
		if(!senddata(pass,getsock(),0, backup->mapdata.data(),	backup->mapdata.size()*sizeof(backup->mapdata.data()[0]),orbackup))
			return 0;
		backupupdated=1;
		ret=1;
		}

  	uptodate(allindex) ;
	if(!noacksendone(pass,getsock(), suptodate))
		return 0;
	if(resetdevices) {
		if(!askresetdevices(pass,getsock()) ) {
			LOGGER("askresetdevices(%p,%d) failed\n",pass,getsock() );
			return 0;
			}
		ret=1;
		resetdevices=false;
		}
	return ret;
//	return sendrender(getsock());

	}
int 	updateone::updatenums() {
	if(!sendnums)
		return 2;
	int soc=getsock();
	if(soc<0)
		return 0;
	return ::updatenums(getcrypt(),soc,nums);
	}

int  updateone::updatestream() {
	if(!sendstream)
		return 2;
	if(getsock()<0)
		return 0;
	return sensors->updatestream(getcrypt(),getsock(),ind,firstsensor);
 	} 
int updateone::updatescans() {
	if(!sendscans)
		return 2;
	if(getsock()<0)
		return 0;
	return sensors->updatescans(getcrypt(),getsock(),ind,firstsensor);
 	} 
bool netwakeup(int sock,passhost_t *pass,crypt_t *ctx){
	if(backup) {
		LOGGER("netwakeup %d\n",sock);
#ifdef REVERSECONNECT
		int sin=pass->index;
		if(sin>=0) {
		       updateone &host=backup->getupdatedata()->tosend[sin];
		       if(host.getsock()<0) {
				LOGGER("use sock\n");
				if(ctx) {
					crypt_t *crypt=host.getcrypt();
					if(!crypt)
						return false;
					memcpy(crypt,ctx,sizeof(crypt_t));;
					}
				host.setsock(sock);
//				backup->wakebackuponly(Backup::wakeall);
				backup->wakebackup(Backup::wakeall);
				return true;
				}
			else
				LOGGER("old sock=%d\n",host.getsock());
			}
#endif
		//backup->wakebackuponly(Backup::wakeall|Backup::wakereconnect);
		backup->wakebackup(Backup::wakeall|Backup::wakereconnect);
		}
	return false;
	}
bool netwakeupstream(int sock,passhost_t *pass,crypt_t *ctx){
	if(backup) {
		LOGGER("netwakeupstream %d\n",sock);
		backup->wakebackup(Backup::wakestream|Backup::wakereconnect);
		}
	return false;
	}
bool networkpresent=false;


int hostsocks[maxallhosts];
uint32_t lastuptodate[maxallhosts]={};
//int *sendsocks=nullptr;
//crypt_t **crypts=nullptr;
std::vector<int> sendsocks;
//std::vector<std::vector<int>> tmpsocks;
//int tmpsocks[maxallhosts][maxip];
std::vector<crypt_t *> crypts;
Backup *backup=nullptr;
#define SENDPASSIVE 1
#define RECEIVEFROM 2
/*
bool sendtype(int sock,char type) {
	if(sock!=-1) {
		char ant=type;
		if(sendni(sock,&ant,1)!=1) {
		//	return false;
			
			}
		}
	return true;
	}
	*/
int saysender(const passhost_t *host) {
	if(host->activereceive) 
		return RECEIVEFROM;
	return 0;
	}
int sayactivereceive(const passhost_t *host) {
	if(host->index>=0) {
		return SENDPASSIVE;
		}
	return 0;
	}
void	updateone::open() {
	     LOGGER("updateone::open");
		auto *host=backup->getupdatedata()->allhosts+allindex;
		if(!host->sendpassive) {
			opensender(host,getsock(),getcrypt(),saysender(host));
			receivetimeout(getsock(),60);

			}
		}


//extern int opensender(passhost_t *pass,int &sock,crypt_t*ctx) ;

bool turnreceiver(int sock,passhost_t *hostptr,crypt_t *ctxptr) ;

void sendup(passhost_t *hostptr) {
	const bool haspas= hostptr->haspass();
	LOGGER("wake sender %spass\n",(haspas?"":"no" ));
	crypt_t ctx;
	crypt_t *ctxptr=haspas?&ctx:nullptr;

        prctl(PR_SET_NAME, "wake sender", 0, 0, 0);
	if(int sock;opensender(hostptr,sock,ctxptr,saysender(hostptr))>=0) {
		sendtimeout(sock,60*5);
		if(sendbackup(ctxptr,sock)) {

			LOGGER("sendup success %d\n",sock);	
#ifdef REVERSECONNECT
//			int ind=hostptr-backup->getupdatedata()->allhosts;
			if(hostsocks[ind]==-1) {
				hostsocks[ind]=sock;
				LOGGER("turnreceiver\n");
				if(turnreceiver(sock,hostptr,ctxptr))
					return;
				}
			else {
				LOGGER("sendup hostsocks[ind]=%d\n",hostsocks[ind]);
				}
#endif
			}
		else
			LOGGER("%d: failure %d\n",agettid(),sock);	

		close(sock);
		}
	}

extern std::vector<Backup::condvar_t*> active_receive;
std::vector<Backup::condvar_t*> active_receive;
#include <chrono>
using namespace std::chrono_literals;
void activereceivethread(int allindex,passhost_t *pass) {
	constexpr const int maxbuf=50;
	char buf[maxbuf];
	const int h=pass->activereceive-1;
	if(h<0)
		return;
	if(!active_receive[h])
		return;
#ifndef NOLOG
	int slen=
#endif
	snprintf(buf,maxbuf, "Ractive%d",h);
       prctl(PR_SET_NAME, buf, 0, 0, 0);
	LOGGERN(buf,slen);
	const bool haspas= pass->haspass();
	crypt_t ctx,*ctxptr=haspas?&ctx:nullptr;
	decltype(active_receive[h]->dobackup) current{};
	while(true) {
		  active_receive[h]->dobackup=active_receive[h]->dobackup&(~current);
		  if(!active_receive[h]->dobackup) {
		    std::unique_lock<std::mutex> lck(active_receive[h]->backupmutex);
			LOGGER("R-active before lock\n");
	constexpr const int waitsec=
#if defined(JUGGLUCO_APP) && !defined(WEAROS)
	70
#else
	30
#endif
;
			auto status=active_receive[h]->backupcond.wait_for(lck,std::chrono::seconds(waitsec));    //Inreality much longer if phone is in doze mode.
			LOGGER("R-active after lock %stimeout\n",(status==std::cv_status::no_timeout)?"no-":"");
			}
		current=active_receive[h]->dobackup;
		if(current&Backup::wakeend) {
			int sockwas=hostsocks[allindex];
			hostsocks[allindex]=-1;
			close(sockwas);
		       delete active_receive[h];
			active_receive[h]=nullptr;
			LOGGER("end activereceivethread");
			return;
			}
		int &sock=hostsocks[allindex];
		if(opensender(pass,sock,ctxptr,sayactivereceive(pass))<0)
			continue;
		void	receiversockopt(int sock) ;
		receiversockopt(sock) ;
		bool	activegetcommands(int sock,passhost_t *host,crypt_t *ctx) ;
		LOGGER("before activegetcommands\n");
		activegetcommands(sock,pass,ctxptr); 
		LOGGER("after activegetcommands\n");
		close(sock);
		sock=-1;
		if(ctxptr) 
			ascon_aead_cleanup(ctxptr);

		}
	}
void updatedata::wakesender() {
    LOGGER("wakesender\n");
	for(int i=0;i<hostnr;i++) {
		passhost_t &host=allhosts[i];
		if(host.activereceive) {
			auto ind=host.activereceive-1;
			LOGGER("active %d: ",ind);
			active_receive[ind]->wakebackup(Backup::wakeall);
			}
		else 
		{
			if(host.receivefrom==3&&host.index<0) {
				std::thread wake(sendup,&host);
				wake.detach();
				}
			else {
				if(host.index>=0&&backup->con_vars[host.index])
					  backup->con_vars[host.index]->wakebackup(Backup::wakesend);
					  
				}
			}
		}
	}
void updatedata::wakestreamsender() {
    LOGGER("wakestreamsender\n");
	for(int i=0;i<hostnr;i++) {
		passhost_t &host=allhosts[i];
		if(host.activereceive) {
			auto ind=host.activereceive-1;
			LOGGER("active %d: ",ind);
			active_receive[ind]->wakebackup(Backup::wakestream);
			}
		else {
			if(host.receivefrom==3&&host.index<0) {
				std::thread wake(sendup,&host);
				wake.detach();
				}
			else {
				if(host.index>=0&&backup->con_vars[host.index])
					  backup->con_vars[host.index]->wakebackup(Backup::wakestreamsend);
					  
				}
			}
		}
	}


void passivesender(int sock,passhost_t *pass)  {
	LOGGER("passivesender %d\n",sock);
	 if(!networkpresent) {
		close(sock);
		return;
		}
	int h=pass->index;
	updateone &host=backup->getupdatedata()->tosend[h];
	LOGGER("passivesender got host\n");
	if(h>=0&&backup->con_vars[h]) {
		int oldsock=host.getsock();
		if(oldsock>=0) {
			LOGGER("passivesender shutdown oldsock %d\n",oldsock);
			host.setsock(-1);
			::shutdown(oldsock,SHUT_RDWR);
			close(oldsock);
			}
		const bool haspas= pass->haspass();
		if(haspas) {
			LOGGER("passivesender  haspas true\n");
			bool	receivepassinit(int ,passhost_t *,crypt_t *);
			if(!receivepassinit(sock,pass,host.getcrypt()))  {
				close(sock);
				return ;
				}
			}
		else
			LOGGER("passivesender  haspas false\n");

		receivetimeout(sock,60) ;
		host.setsock(sock); 
		LOGGER("wakebackup\n");
		 backup->con_vars[h]->wakebackup(Backup::wakeall);
		 }
	}

/*
	int h=pass->index;
	updateone &host=backup->getupdatedata()->tosend[h];
	host.close();
	host.setsock(sock); */
	


bool getpassive(int pos) {
	if(pos<backup->getupdatedata()->hostnr)  {
		const auto &host=backup->getupdatedata()->allhosts[pos];
		if(host.index>=0)
			return host.sendpassive;
		return host.receivefrom==2;
		}
	return false;
	}
bool getactive(int pos) {
	if(pos<backup->getupdatedata()->hostnr)  {
		const auto &host=backup->getupdatedata()->allhosts[pos];
		LOGGER("receivefrom=%d sendpassive=%d\n",host.receivefrom,host.sendpassive);
		if(host.index>=0) {
			if(host.sendpassive) 
				return false;
			if(host.activereceive)
				return true;
			return !host.receivefrom;
			}
			
		return host.activereceive;
		}
	return false;
	}
	/*
updateone &getsendto(int index) {
         int tohost=backup->getupdatedata()->allhosts[index].index;
      	return backup->getupdatedata()->tosend[tohost];
	}*/

updateone &getsendto(const passhost_t *host) {
        const int tohost=host->index;
      	return backup->getupdatedata()->tosend[tohost];
	}
updateone &getsendto(int index) {
	return getsendto(backup->getupdatedata()->allhosts + index);
	}

bool sendall(const passhost_t *host) {
 const updateone &sender=getsendto(host);
  return (sender.sendnums&&sender.sendstream&&sender.sendscans);
  }

