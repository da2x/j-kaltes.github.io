#include <stdlib.h>

#include <signal.h>

#include "datbackup.h"
#include "nums/numdata.h"
#include "settings/settings.h"

//extern bool setfilesdir(const string_view filesdir,const char *country) ;
extern void startjuggluco(std::string_view dirfiles,const char *country) ;

extern pathconcat numbasedir;
extern vector<Numdata*> numdatas;

extern bool networkpresent;

const char intro[]=R"(Command line program to create a Juggluco backup on a desktop computer.
Juggluco is an android applet that connects with Freestyle libre 2 sensors
and allows one to add diabetes diary data, see:
https://play.google.com/store/apps/details?id=tk.glucodata
Within a certain directory (-d dir) backup and connection data is saved.
With this program, you can specify the connections the program
should receive data from and send data to.
)";
      #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>

template<typename P,typename  ... Ts>
bool  exportdata(const char *filename,P proc,Ts... args)  {
	int han=creat(filename, S_IRUSR| S_IWUSR);
	if(han<0) {
		cerr<<"Can't open "<<filename<<endl;
		return false;
		}
	if(!proc(han,args ...)) {
		cerr<<"Export failed"<<endl;
		return false;
		}
	
	cout<<"Exported to "<<filename<<endl;
	return true;
	}

extern bool allsavemeals(int handle) ;

bool exportnums(int handle) ;
bool exportnummer(const char *filename) {
	return exportdata(filename,exportnums);
	}
/*
bool exportnums(const char *filename) {
	int han=creat(filename, S_IRUSR| S_IWUSR);
	if(han<0) {
		cerr<<"Can't open "<<filename<<endl;
		return false;
		}
	if(!exportnums(han)) {
		cerr<<"Export failed"<<endl;
		return false;
		}
	
	cerr<<"Amounts exported to "<<filename<<endl;
	return true;
	}
bool exporthistory(int handle) {


	}
		case 3: return exporthistory(fd);
bool exportnums(int handle) {

		case 0: return exportnums(fd);	;
		case 1: return exportscans(fd, &SensorGlucoseData::getScandata);
		case 2: return exportscans(fd, &SensorGlucoseData::getPolldata);
		case 3: return exporthistory(fd);
*/

void help(const char *progname) {
const char *hostip=myip().data();
cout<<intro<<"Usages: "<<progname<<R"( -d dir : save data in directory dir)"<<endl
 <<"        "<<progname<<R"( [-d dir] -p port : listen on port port)"<<endl
 <<"        "<<progname<<R"( [-d dir] -l : list configuration data)"<<endl
 <<"        "<<progname<<R"( [-d dir] -c : clear configuration data)"<<endl
 <<"        "<<progname<<R"( -x[-+]: start xDrip watch app server or not (-x-)
        )"<<progname<<R"( [-d dir] -N filename: export nums 
	)"<<progname<<R"( [-d dir] -S filename: export scans 
	)"<<progname<<R"( [-d dir] -B filename: export stream 
	)" << progname<<R"( [-d dir] -H filename: export history 
	)"<<progname<<R"( [-d dir] -m filename: export meals 
	)" << progname<<R"( [-d dir] -M : mmol/L
	)" << progname<<R"( [-d dir] -G : mg/dL
	)" << progname<<R"( [-d dir] -R n : remove n-th connection
	)" << progname<<R"( -v  : version


	)"<< progname<<R"( [-d dir] OPTIONS IP1,IP2 ...  : Specify connection with IP(s).
OPTIONS:
	-r: receive from host
	-a: automatically detect IP the first time
	-A: Active only. Don't accept connections from this host. Only actively connect to this host.
	-P: Passive only. Never connect actively to this host. Only accepts connections.

	-n: send nums (amounts)
	-s: send scans (via NFC received data)
	-b: send stream (via bluetooth received data)

	-L label: give connection label 'label'. A connection is only established if both side specify 
		the same label.
	-i: don't test on ip
	-w password: encrypt communication with password
	-p port:  connect to port (default: 8795)

	-C n: overwrite n-th connection. If this program sends to this connection, data will not be resend. 

example:

)"<<progname<<R"( -d mydir -r  192.168.1.64

Receive data from host 192.168.1.64 (an ip shown in backup screen android app)
In the juggluco android applet you can in "backup, add connection"
specfiy send to amounts, scans and stream and specify port 8795 and
an ip of this computer (e.g )"<<hostip<<")\n\n"<<progname<<R"( -d mydir -nsb  192.168.1.69

Send the amounts, scans and stream data to 192.168.1.69 on the default port (8795)
In the android applet you can in the "add connection" dialog, specify 
an ip of this computer ()"<<hostip<<R"() check "Receive from" and specify as port 8795
(the port specified earlier for this computer)

)"<<progname<<R"( -d mydir

Starts the program with this configuration.
)";

	};
int showui;
bool getpassive(int pos);
bool getactive(int pos); 
int   listconnections() {
	const int hostnr=backup->gethostnr();
	cout<<"Listen at port "<< backup->getupdatedata()->port <<endl;
	cout<<"unit: "<<settings->getunitlabel()<<endl;
	cout<<"connection"<<(hostnr>1?"s:":":")<<endl;;
	for(int h=0;h<hostnr;h++) {
		cout<<h+1<<": ";
		const passhost_t &host=backup->getupdatedata()->allhosts[h];
		cout<< (host.hasname?host.getname():"")<<(host.detect?" detect,":" ")<< (host.receivefrom&2?" receiver":"")<<(getpassive(h)?" passiveonly ":" ")<<(getactive(h)?" active only ":"")<<(host.haspass()?backup->getpass(h).data():"no pass") <<", port="<< ntohs(host.ips[0].sin6_port);
		int sin=host.index;
		if(sin>=0) {
			const updateone &sto=backup->getupdatedata()->tosend[sin];
			const bool sen=sto.sendnums || sto.sendstream|| sto.sendscans;
			if(sen) 
				cout<<" send " <<(sto.sendnums?"nums ":"")<<(sto.sendscans?"scans ":"")<<(sto.sendstream?"stream ":"");
			}
		const int len=host.nr;
		for(int i=0;i<len;i++) {
			namehost name(host.ips+i);
			cout<<" "<<name.data();
			}
		cout<<endl;
		}
	return 1234;
	}
int clearhosts() {
		
	backup->resetall();	
	return 1234;
	}
//#if defined(_Windows) || __ANDROID__
const char dirconf[]=".jugglucorc";

bool exportscans(int handle, const std::span<const ScanData>  (SensorGlucoseData::*proc)(void) const) ;
bool exporthistory(int handle) ;
void showversion() {
#include "version.h"
	cout<<"Version "<<version<<endl;
	}
int readconfig(int argc, char **argv) {
	bool receive=false,detect=false,signal=false,nums=false,scans=false,stream=false;
	bool list=false,clear=false;
	const char *password="";
	char *dir=nullptr;
	string_view port;

	int rmindex=-1;
const char * numexport=nullptr;
const char *historyexport=nullptr;
const char *scanexport=nullptr;
const char *pollexport=nullptr;
const char *mealexport=nullptr;
const char *label=nullptr;
bool activeonly=false,passiveonly=false,testip=true

#ifndef DONT_USE_LIBREVIEW
,showlibreview=false
#endif
;
int xdripserver=-1;
int changer=-1;
int unit=0;
           for(int opt;(opt = getopt(argc, argv, "p:d:lcx::ransvzibAPw:hN:S:B:H:m:GMR:L:C:")) != -1;) {
               switch (opt) {
	       	   case 'v': showversion();return 1234;
	           case 'i': testip=false; break;
		   	case 'x': {
				if(optarg) {
					cerr<<"xdriparg:"<<optarg<<':'<<endl;
					if(!optarg[1]) {
						switch(optarg[0]) {
							case '+': xdripserver=1;goto XDRIPSUCCESS;break;
							case '-': xdripserver=0;goto XDRIPSUCCESS;break;
							};
						};
					cerr<<"Unknown arg: "<<optarg<<endl;;
					cerr<<"Argument to -x should be + or -\n";
					return 10;
					}
				else {
					cerr<<"xdriparg\n";
					xdripserver=1;
					}

				XDRIPSUCCESS:
				break;
				}
			case 'd':
				dir=optarg;
				break;
		       case 'r':
			   receive=true;
			   break;
		       case 'a':
			   detect=true;
			   break;
		       case 'G': unit=2;break;
		       case 'R': rmindex=atoi(optarg); 
			cerr<<"remove "<<rmindex--<<endl;
		       		break;
		       case 'C': changer=atoi(optarg)-1; 
		       		break;
			case 'L': label=optarg;break;
		       case 'M': unit=1;break;
		       case 'P': passiveonly=true;break;
		       case 'A': activeonly=true;break;
		       case 'n': nums=true;break;
		       case 's': scans=true;break;
		       case 'b': stream=true;break;
		       case 'w': password=optarg;break;
		       case 'p': port=optarg;
		       if(port.size()>5) {
		       		cerr<<"%s too large, port is maximally 5 digits\n";
				return 7;
		       		}
		       	
		       	break;
		       case 'l': list=true;break;;
		       case 'c': clear=true;break;
		       case 'N': numexport=optarg;break;
		       case 'S': scanexport=optarg;break;
		       case 'B': pollexport=optarg;break;
		       case 'H': historyexport=optarg;break;
		       case 'm': mealexport=optarg;break;
#ifndef DONT_USE_LIBREVIEW
		       case 'z': showlibreview=true;break;
#endif
		       default: help(argv[0]); return 4;
		       }
           }
	 Readall alldir;
	char defaultname[]="jugglucodata";
	if(!dir) {
		dir=alldir.fromfile(dirconf);
		if(!dir)
			dir=defaultname;
		}
	else {
		int dirlen=strlen(dir)-1;
		for(;dir[dirlen]=='/'||dir[dirlen]=='\\';dirlen--)
			dir[dirlen]='\0';	
		if(!writeall(dirconf,dir,dirlen+1)) {
			cerr<<"Write to "<<dirconf<<" failed\n";
			}
		}
	cout<<"Saving in directory "<<dir<<endl;
	startjuggluco(dir,nullptr);
	/*
	if(!setfilesdir(dir,nullptr)) {
		fprintf(stderr,"can't create files\n");
		return 3;
		}*/
	if(xdripserver>=0)
		settings->data()->usexdripwebserver=xdripserver;
	if(unit)
		settings->setunit(unit);
	else
		settings->setlinuxcountry();
//	constexpr size_t nummmaplen=77056;
	 if(Numdata* numdata=Numdata::getnumdata( pathconcat(numbasedir,"here"),0,nummmaplen))
		numdatas.push_back(numdata);
		
	 if(Numdata* numdata=Numdata::getnumdata( pathconcat(numbasedir,"watch"),-1,nummmaplen)) {
		numdatas.push_back(numdata);
		}
	if(!backup)  {
		fprintf(stderr,"My error: No Backup\n");
		return 2;
		}
	if(argc==1)
		return 0;
	bool sender=	nums||scans||stream;
	int hostnr=backup->gethostnr();
	if(!sender&&!receive) {
		bool did=false;
		if(rmindex>=0) {
			if(rmindex>hostnr) {
				cerr<<"Argument to -R should refer to an existing connection (1-"<<hostnr<<endl<<")"<<endl;
				return 9;
				}
			backup->deletehost(rmindex);
			return 1234;
			}
		if(port.size()) {
			memcpy(backup->getupdatedata()->port,port.data(),port.size());
			backup->getupdatedata()->port[port.size()]='\0';

			did=true;
//			return 1234;
			}
		if(unit) {
			did=true;
			}
		if(list)
			return 	listconnections();
		if(clear)
			return clearhosts();
		if(numexport)  {
			 if(!exportnummer(numexport)) 
				return 13;
			did=true;
			}
#ifndef DONT_USE_LIBREVIEW
		if(showlibreview) {
			void makelibreviewdata() ;
			makelibreviewdata();

			did=true;
			}
#endif
		if(historyexport)  {
			 if(!exportdata(historyexport,exporthistory))
				return 13;
			did=true;

			}
		if(scanexport) {
			if(!exportdata(scanexport,exportscans, &SensorGlucoseData::getScandata))
				return 13;
			did=true;
			}
		if(pollexport) {
			if(!exportdata(pollexport,exportscans, &SensorGlucoseData::getPolldata))
				return 13;
			did=true;
			}
		if(mealexport) {
			if(!exportdata(mealexport,allsavemeals))
				return 13;
			did=true;
			}
		if(detect||signal) {
			 help(argv[0]); 
			 return 0;
			}
		if(did)
			return 1234;
		return 0;
		}
	else {
           if (activeonly&&optind >= argc) {
               cerr<< "No IPS specified\n";
	       return 5;
           	}
 	uint32_t starttime=0;
	if(int er=backup->changehost(changer<0?hostnr:changer,nullptr,reinterpret_cast<jobjectArray>(argv+optind ),argc-optind, detect,port,nums,stream,scans,false,receive,activeonly,password,starttime,passiveonly,label,testip,false) <0) {
		cerr<<"changehost failed\n";
		return er;
		}
		}
	return 1234;
       }
#include "net/netstuff.h"
void sighandler(int sig) { }
static void wakeup() {
	backup->getupdatedata()->wakesender();
	backup->wakebackup(Backup::wakeall);
	//	backup->wakebackup();
	}
void exitproc() {
	cout<<"This is a normal exit"<<endl;
	}
 unsigned int alarm(unsigned int seconds);
static	void setalarm();
int main(int argc,char **argv) {
//	bool active=backup->getupdatedata()->port[0];
//	if(active) backup->stopreceiver();
//	backup->changehost(0,"192.168.1.69","8795", false,false,false,false,true,true,"1234567890123456");
	if(int ret=readconfig(argc,argv)) {
		if(ret==1234)
			return 0;
		return ret;
		}
	networkpresent=true;
	signal(SIGUSR1,sighandler);
	atexit(exitproc);
	setalarm();


	if(settings->data()->usexdripwebserver) {
		void startwatchthread() ;
		startwatchthread();
		}
	while(true) {
		wakeup();
		pause();
		perror("Got signal");
		}
	}

//int changehost(int index,string_view name,string_view port,const bool sendnums,const bool sendstream,const bool sendscans,const bool restore,const bool receive,const bool reconnect,string_view pass) 
void render() {
//  wakeup();
}
void     processglucosevalue(int sendindex) {
	constexpr int maxbluetoothage=3*60;
	if(!sensors)
		return;
	LOGGER("processglucosevalue %d ", sendindex);
	if(SensorGlucoseData *hist=sensors->gethist(sendindex)) {
		if(const ScanData *poll=hist->lastpoll()) {
			const time_t nutime=time(nullptr);
			const time_t tim=poll->t;
			const int dif=nutime-tim;
			if(dif<maxbluetoothage) {
				sensor *senso=sensors->getsensor(sendindex);
				logprint("finished=%d not finished %s ",senso->finished,ctime(&tim));
				senso->finished=0;

				}
			else {
				logprint(" too old %s ",ctime(&tim));
				logprint("dist=%d, dif= nu %s",maxbluetoothage,dif,ctime(&nutime));
				}
			}
		else {
			logprint("no stream data\n");
			}
		}
	else {
		logprint("no sensor\n");
		}
	}
#include "alarm.cpp"
/*
__asm__(".symver stat,stat@GLIBC_2.31");
__asm__(".symver fstat,fstat@GLIBC_2.31");
__asm__(".symver __libc_start_main,__libc_start_main@GLIBC_2.31");
__asm__(".symver __pthread_key_create,__pthread_key_create@GLIBC_2.31");
__asm__(".symver pthread_create,pthread_create@GLIBC_2.31");
__asm__(".symver _ZSt28__throw_bad_array_new_lengthv,_ZSt28__throw_bad_array_new_lengthv@GLIBCXX_3.4.28");

__asm__(".symver stat,stat@GLIBC_2.33");
__asm__(".symver fstat,fstat@GLIBC_2.33");
__asm__(".symver __libc_start_main,__libc_start_main@GLIBC_2.34");
__asm__(".symver __pthread_key_create,__pthread_key_create@GLIBC_2.34");
__asm__(".symver pthread_create,pthread_create@GLIBC_2.34");
__asm__(".symver _ZSt28__throw_bad_array_new_lengthv,_ZSt28__throw_bad_array_new_lengthv@GLIBCXX_3.4.29");

*/
bool updateDevices() {
	LOGGER("updateDevices() called\n");
	return true;
	}
