#ifndef GLUCOSEHISTORY_H
#define GLUCOSEHISTORY_H
//#include <filesystem>

#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <fstream>
#include <stdlib.h>
#include <span>
#include <math.h>
#include <time.h>

#include <stdint.h>
#include <assert.h>
#include <limits>
#include <mutex>

#include "inout.h"

#include "net/backup.h"
#include "net/passhost.h"
#include "nfcdata.h"
//namespace fs = std::filesystem;
#include "gltype.h"
//#include "datbackup.h"
#include "maxsendtohost.h"
inline	constexpr const char rawstream[]="rawstream.dat";
#include <string_view>
extern std::string_view globalbasedir;
//string basedir(FILEDIR);
constexpr int maxminutes=22000;
struct ScanData {uint32_t t;int32_t id;int32_t g;int32_t tr;float ch;
const uint16_t getmgdL() const { return g;};
const uint16_t getsputnik() const { return g*10;};
uint32_t gettime() const {
	return t;
	};
const uint32_t getid() const {return id;};
bool valid() const {
	return t&&g&&g>38&&g<501&&id>=0&&id<maxminutes&&t>1598911200u&&t<2145909600u;
	}
} ;
struct Glucose { 
uint32_t time;
uint16_t id;
uint16_t glu[];
const uint16_t getraw() const { return glu[0];};
const uint16_t getsputnik() const { return glu[1];};
const uint32_t gettime() const {return time;};
const uint32_t getid() const {return id;};
};
class SensorGlucoseData {
string sensordir;
//inline static  const string basedir{FILEDIR};
inline static const int blocksize=getpagesize();

static constexpr uint16_t defaultinterval=15*60;

struct updatestate {
	uint32_t scanstart;uint32_t histstart;uint32_t streamstart; uint32_t rawstreamstart;
	bool sendbluetoothOn:1;
	bool sendstreaming:1;
	uint8_t rest:6;
	};
static inline constexpr const int deviceaddresslen=18;
struct Info {
uint32_t starttime;
uint32_t lastscantime;
uint32_t starthistory;
uint32_t endhistory;  //In reality one past last pos. Previously said wrong.
uint32_t scancount;
uint16_t startid;
uint16_t interval;
uint16_t reserved;
uint8_t dupl;
uint8_t days;
//uint32_t reserved2[2];
uint32_t pollcountwas;
uint32_t lockcountwas;
struct { int len;
	signed char data[8];
	} ident;
struct { int len;
	signed char data[6];
	} info;
uint32_t bluestart;	
union {
	struct { int len;
		signed char data[6];
		uint8_t sensorgen;
		bool reserved2;
		} blueinfo;
	uint8_t streamingAuthenticationData[10];
	};
uint32_t pollcount;
double pollinterval; 
uint32_t lockcount;
//int bluetoothOn;
int8_t bluetoothOn;
uint8_t reserved4[3];
char deviceaddress[deviceaddresslen];
uint16_t libreviewscan;
uint16_t libreviewid;
uint16_t libreviewnotsend:15;
bool putsensor:1;
updatestate update[maxsendtohost];

bool infowrong() const {
	if(days<10||days>16)
		return true;	
	if(starttime<1583013600)
		return true;
	if(!dupl)
		return true;
	return false;
	}


void setauth(uint8_t *authin) {
	memcpy(streamingAuthenticationData,authin,10);
	}
const uint8_t *getauth() const {
	return streamingAuthenticationData;
	}
} ;

//pathconcat sensordir;
//pathconcat scanfile;
Mmap<unsigned char>  meminfo;
int getelsize() const {
	return sizeof(uint32_t)+(getinfo()->dupl+1)*sizeof(uint16_t);
	}
Mmap<uint8_t> historydata;

const size_t scansize;
Mmap<ScanData> scans,polls;
Mmap<std::array<uint16_t,16>> trends;
//static constexpr int getinfo()->dupl=3,days=15;
bool haserror=false;
const int nrunits(int perhour=4)  {
//	if(error()) return 0;
	const auto elsize=getelsize();
	const auto days=getinfo()->days;
	if(elsize<10||elsize>20||days<14||days>20) {
		haserror=true;
		return 0;
		}
	return elsize*days*24*perhour;
	}
	/*
inline int getstartpos() {
	return getinfo()->startpos;
	}
inline void setstartpos(int pos) {
	getinfo()->startpos=pos;
	}
	*/
public:
bool resetdevice=false;

std::mutex mutex;
int getsensorgen() const {
	return getinfo()->blueinfo.sensorgen;
	}
void setsensorgen() {
	extern void setlastGen(int gen);
	const data_t *info=getpatchinfo();
	getinfo()->blueinfo.sensorgen=getgeneration((const char *)info->data());
	}
char *deviceaddress() {
	return getinfo()->deviceaddress;
	}
const char *deviceaddress() const {
	return getinfo()->deviceaddress;
	}
const int glucosebytes()const {
	return getelsize();
	}
bool waiting=true;
const uint32_t maxpos() const {
	return getinfo()->days*24*perhour();
	}
int bluetoothOn() const{
	return getinfo()->bluetoothOn;
	}
void setbluetoothOn(int val) {
	getinfo()->bluetoothOn=val;
	}
uint32_t getfirsttime() const {
	uint32_t locfirstpos=getstarthistory()+1;
	for(int pos=locfirstpos,end=std::min(getendhistory(),maxpos());pos<end;pos++) {
		int16_t id =getid(pos);
		uint32_t tim=timeatpos(pos);
		if(id&&tim)
			return tim;
		}

	return UINT32_MAX;
	}
void checkhistory(std::ostream &os) {
	int interval=getinterval();
	int minhistinterval=interval/60;
	uint32_t locfirstpos=getstarthistory()+1;
	int som=0;
	for(int pos=locfirstpos,end=std::min(getendhistory(),maxpos()),loclastpos=locfirstpos;pos<end;pos++) {
		int16_t id =getid(pos);
		uint32_t nu=timeatpos(pos);
		if(id&&nu) {
			auto prevtime=timeatpos(loclastpos);
			if(prevtime) {
				if(long times=pos-loclastpos;times!=0) {
					uint32_t totdif=nu-(long)prevtime- times*interval;
					int dif= totdif/times;
					som+=totdif;
					if(abs(dif)>60) {
						os<<dif<<" difference at pos "<<pos;
						if(times>1)
							os<<times<<" times"<<std::endl;
						else
							os<<std::endl;
						}
					}
				}
			if(id!=pos*minhistinterval)
				os<<"pos="<<pos<<"id="<<id<<"!="<<pos*minhistinterval<<std::endl;
			loclastpos	=pos;
			}
		}

	os<<"total difference: "<<som<<" seconds "<<som/60.0<<" minutes"<<std::endl;

	}
int getinterval() const {
	if(getinfo()->interval)
		return getinfo()->interval;
	return defaultinterval;
	}
int getmininterval() const {
	return  getinterval()/60;
	}

const int perhour() const {
	return 60/getmininterval();
	}
uint32_t getmaxtime() const {
	return getinfo()->days*24*60*60+getstarttime();
	}
uint32_t getstarttime() const {
	return getinfo()->starttime;
	}
uint32_t getstarthistory() const {
	return getinfo()->starthistory;
	}
inline void setstarthistory(int pos)  {
	getinfo()->starthistory=pos;
	}
uint32_t getendhistory() const {
	return getinfo()->endhistory;
	}
uint32_t getlasttime() const {
	return timeatpos(getendhistory()-1);
	}
inline int getlastpos(int pos) {
	if(pos<getinfo()->starthistory)
		getinfo()->starthistory=pos;
	return getinfo()->endhistory;
	}
inline void setendhistory(int pos)  {
	getinfo()->endhistory=pos;
	}
inline const uint8_t *elstart(int pos) const {
	return &historydata[pos*getelsize()];
	}
inline uint8_t *elstart(int pos)  {
	return &historydata[pos*getelsize()];
	}


const Glucose * getglucose(int pos) const {
	return reinterpret_cast<const Glucose *>( &historydata[pos*getelsize()]);
	};
/*
Wrong Glucose has variable size
const std::span<const Glucose> getHistoryData() const {
	const auto first=getstarthistory();
	const size_t count=getendhistory()-first+1;
	return std::span<const Glucose>(getglucose(first),count);
	}
	*/
template<int N>
void saveel(int pos,time_t tin,uint16_t id, uint16_t const (&glu)[N]) {
	assert(N<=getinfo()->dupl);
	int idpos=int(round(id/(double)getmininterval()));
	if(pos!=idpos) {
		LOGGER("GLU: saveel %d!=%d\n",pos,idpos);
		return;
		}
	uint32_t time=static_cast<uint32_t>(tin);
	uint8_t *memstart=elstart(pos);
	memcpy(memstart,&time,sizeof(time));
	memstart+=sizeof(time);
	memcpy(memstart,&id,sizeof(id));
	memstart+=sizeof(id);
	memcpy(memstart,glu,N*sizeof(*glu));
	}
uint32_t timeatpos(int pos)  const {
	return *((const uint32_t *)elstart(pos));
	}
uint32_t rawglucose(int pos) const {
	return *((const uint16_t *)(elstart(pos)+sizeof(uint32_t)+sizeof(uint16_t)));
	}
uint32_t sputnikglucose(int pos)const  {
	return *((const uint16_t *)(elstart(pos)+sizeof(uint32_t)+2*sizeof(uint16_t)));
	}
//	uint8_t *memstart=&historydata[pos*getelsize()];
	
int gettimepos(uint32_t time)const {
	return ( int)round(((double)time-getinfo()->starttime)/getinfo()->interval);
	}
uint16_t getatpos(int pos,int field) const {
	const uint16_t *d=(const uint16_t *)(historydata+pos*getelsize());
	return d[field];
	}
uint16_t getid(int pos) const {
	if(pos<=0) {
		return 0;
		}
	return getatpos(pos,2);	
	}
static constexpr int glinel=6;
uint16_t &glucose(int pos,int kind) {
	uint16_t *glus=(uint16_t *)(historydata+pos*getelsize()+glinel);
	return glus[kind];
	}
float tommolL(uint16_t raw) {
	return (float)raw/180.0;
	}
void dumpdata(std::ostream &os,uint32_t locfirstpos,uint32_t loclastpos) {
   	os<< std::fixed<< std::setprecision(1);
	for(auto pos=locfirstpos;pos<loclastpos;pos++) {
		os<<timeatpos(pos)<<"\t"<<getid(pos)<<"\t"<<sputnikglucose(pos)<<"\t"<<rawglucose(pos)<<std::endl;
		}
	}
void exporttsv(const char * file) {
	std::ofstream uit(file);
	dumpdata(uit,getstarthistory(),getendhistory());
	uit.close();
	}

	bool infowrong() const {
			return getinfo()->infowrong();
		}

public:
static inline constexpr uint32_t bluestartunknown= 97812u;
void setnobluetooth() {
	getinfo()->bluestart=bluestartunknown;
	}
bool hasbluetooth() const {
	return getinfo()->bluestart!=bluestartunknown;
	}
//typedef array<char,11>  sensorname_t;
const sensorname_t * shortsensorname() const {
	return reinterpret_cast<const sensorname_t *>(sensordir.data()+sensordir.length()-11);
	}
typedef std::array<char,16>  longsensorname_t;
const longsensorname_t * sensorname() const {
	return reinterpret_cast<const longsensorname_t *>(sensordir.data()+sensordir.length()-16);
	}
static int getgeneration(const char *info) {
    int i = info[2] >> 4;
    int i2 = info[2] & 0xF;
    if(i == 3) {
	if (i2 < 9) {
	    return 1;
	}
	return 2;
    } else if (i == 7) {
	if (i2 < 4) {
	    return 1;
	}
	return 2;
    }
    return 0;
    }




static bool mkdatabase(string_view sensordir,time_t start,const  char *uid,const  char *infodat,uint8_t days=15,uint8_t dupl=3,uint32_t bluestart=0,const char *blueinfo=nullptr,uint16_t secinterval=defaultinterval) {
     LOGGER("mkdatabase(%s,%s)",sensordir.data(),ctime(&start));
       int gen=getgeneration(infodat);
//	pathconcat  sensordir{basedir,sensorid};
	mkdir(sensordir.data(),0700);
//	string_view prob{sensordir};
	pathconcat infoname(sensordir,infopdat);
	if(access(infoname,F_OK)!=-1)  {
		Readall<uint8_t> inf(infoname);
		if(inf.data()&&inf.size()>=sizeof(Info)) {
			const Info *in=reinterpret_cast<const Info*>(inf.data());
			if(in->starttime>1590000000&&in->dupl>0&&in->info.len==6)
				return false;
			}
		}
       Info inf{.starttime=(uint32_t)start,.lastscantime=inf.starttime,.starthistory=0,.endhistory=0,.scancount=0,.startid=0,.interval=secinterval,.reserved=0,.dupl=dupl,.days=days ,
	.pollcount=0, .lockcount=0};
	inf.ident.len=8;
//		memcpy(&inf.ident.data,uid,8);
	memcpy(inf.ident.data,uid,8);
	inf.info.len=6;
	memcpy(inf.info.data,infodat,6);


	inf.bluestart=bluestart?bluestart:start;	
	inf.blueinfo.sensorgen=gen;
	if(gen!=2) {
		inf.blueinfo.len=6;
		if(blueinfo) 
			memcpy(inf.blueinfo.data,blueinfo,6);
		else {
			memcpy(inf.blueinfo.data,infodat,4);
			inf.blueinfo.data[4]=0;
			inf.blueinfo.data[5]=0;
			}
		}
	writeall(infoname,&inf,sizeof(inf));
	return true;
	}
	/*
static pathconcat namelibre3(const std::string_view sensorid) {
	char sendir[17]="E07A-XXX";              
	const int totsens=16;
	const int len=sensorid.length();
	memcpy(sendir+totsens-len,sensordir.data(),len);
	sendir[totsens]='\0';
	return pathconcat(basedir,std::string_view(sendir,totsens));
	} */
/*
E007-0M0063KNUJ0
E07A-XX068ZMRF18              
E07A-000T3YL1R50
*/
static bool mkdatabase3(string_view sensordir,time_t start) {
     LOGGER("mkdatabase3(%s,%s)",sensordir.data(),ctime(&start));
	mkdir(sensordir.data(),0700);
	pathconcat infoname(sensordir,infopdat);
	constexpr uint16_t interval5=5*60;
	if(access(infoname,F_OK)!=-1)  {
		Readall<uint8_t> inf(infoname);
		if(inf.data()&&inf.size()>=sizeof(Info)) {
			const Info *in=reinterpret_cast<const Info*>(inf.data());
			if(in->starttime>1590000000&&in->dupl>0&&in->interval==interval5)
				return false;
			}
		}
       Info inf{.starttime=(uint32_t)start,.lastscantime=inf.starttime,.starthistory=0,.endhistory=0,.scancount=0,.startid=0,.interval=interval5,.reserved=0,.dupl=3,.days=15 ,
	.pollcount=0, .lockcount=0};
	writeall(infoname,&inf,sizeof(inf));
	return true;
	}

	/*
bool bluetoothfirst() const {
	return !getinfo()->blueinfo.data[4]&&!getinfo()->blueinfo.data[5];
	}*/
const std::pair<uint32_t, const data_t *> getbluedata() const {
	if(getinfo()->bluestart==0)
		return {getinfo()->starttime, reinterpret_cast<const data_t *>(&getinfo()->info)};

	return {getinfo()->bluestart, reinterpret_cast<const data_t *>(&getinfo()->blueinfo)};
	}
static constexpr char blueback[]= "bluetooth.bak";
bool setbluetooth(uint32_t start,const signed char *infoin) {
	pathconcat backup(sensordir,blueback);
      	Create file(backup.data());
	if(file<0)
		return false;
	if(write(file,&getinfo()->bluestart,4)!=4|| write(file,getinfo()->blueinfo.data,6)!=6)
		return false;
	getinfo()->bluestart=start;
	getinfo()->blueinfo.len=6;
	memcpy(getinfo()->blueinfo.data,infoin,6);
	return true;
	}
bool bluetoothback() {
	Readall all(blueback);
	const char *data=all.data();	
	if(!data)
		return false;
	unlink(blueback);
	memcpy(&getinfo()->bluestart,data,4);
	memcpy(getinfo()->blueinfo.data,data+4,6);
	return true;
	}



private:

size_t maxscansize()  {
	if(haserror)
		return 0;
//	if(error()) return 0;
	const int days=	std::max((int)getinfo()->days,15);
	const int scanblocks=ceil((40*days*sizeof(ScanData))/blocksize);
        int used=getinfo()->scancount*sizeof(ScanData);
        int past= getinfo()->endhistory-getinfo()->starthistory;
	if(past<0||used<0) {
		haserror=true;
		return 0;
		}
	int take;
	int maxp=maxpos();
	if(maxp<past)
		maxp+=24*perhour();
        if(used<=0||past<=0||(take =ceil((maxp+(maxp-past)*.2)*used/((double)past*blocksize)))<scanblocks)
		take=scanblocks;
	LOGGER("blocksize=%d take=%d used=%d maxp=%d\n",blocksize,take,used,maxp);
	return take*blocksize/sizeof(ScanData);
        }



public:
Info *getinfo() {
	return reinterpret_cast<Info *>(meminfo.data());
	}
const Info *getinfo() const {
	return reinterpret_cast<const Info *>(meminfo.data());
	}
private:
 int specstart;
static constexpr const char infopdat[]="info.dat";
pathconcat polluit;
 pathconcat infopath;
 pathconcat updateinfopath;
 pathconcat histpath;
 pathconcat scanpath;
 pathconcat trendspath;
 static constexpr const char trendsdat[]="trends.dat";
SensorGlucoseData(string_view sensordir,int spec,string_view baseuit): sensordir(sensordir),meminfo(sensordir,infopdat,sizeof(struct Info)),historydata(sensordir,"data.dat",nrunits()),
scansize(maxscansize()),
scans(sensordir,"current.dat",scansize),
polls(sensordir,"polls.dat",21600),
trends(sensordir,trendsdat,scansize),
specstart(spec),
 polluit(baseuit, "polls.dat"),
 infopath(baseuit, infopdat),
 updateinfopath(baseuit, "updateinfo.dat"),
 histpath(baseuit, "data.dat"),
 scanpath(baseuit, "current.dat"),
 trendspath(baseuit, trendsdat)
{
LOGGER("SensorGlucoseData %s %s scansize=%zu\n",sensordir.data(),scanpath.data(),scansize);
if(error()) {
	LOGGER("SensorGlucoseData Error\n");
	return;
	}

    if(getinfo()->pollinterval<58.6||getinfo()->pollinterval>62.6)
		getinfo()->pollinterval=60.5752;
  // mkabspath();
	}
SensorGlucoseData(string_view sensin,int specin): SensorGlucoseData(sensin, specin, std::string_view(sensin.data()+specin,sensin.size()-specin)) {
	}
	public:
SensorGlucoseData(string_view sensin): SensorGlucoseData(sensin,globalbasedir.size()+1) {
	}
//bool haserror=false;
bool error() const {
	if(!haserror&&meminfo.data()&& historydata.data()&& scans.data()&&polls.data()&& trends.data())
		return false;	
	return true;
	}
void setlastscantime(uint32_t tim) {
	getinfo()->lastscantime=tim;
	}
uint32_t getlastscantime() const {
	return getinfo()->lastscantime;
	}
	/*
uint32_t getlastpolltime() const {
	uint32_t last=(pollcount()>0)?lastpoll()->t:0;
	return last;
	}*/
uint32_t getlastpolltime() const {
	const auto stream=lastpoll();
	if(!stream)
		return 0;
	return stream->t;
	}
uint32_t firstpolltime() const {
	const ScanData* start= polls.data();
	for(int i=0;i<pollcount();i++)
		if(start[i].valid())
			return start[i].t;	
	return UINT32_MAX;
	}
int scancount() const {
	return getinfo()->scancount;
	}
int pollcount() const {
	return getinfo()->pollcount;
	}
std::array<uint16_t,16> &gettrendsbuf(int index) {
	return trends[index];
	}
const	std::array<uint16_t,16> &gettrendsbuf(int index) const {
	return trends[index];
	}
void savetrend(const nfcdata *scan,std::array<uint16_t,16> &trendbuf){
	for(int i=0;i<trend::num;i++) 
		trendbuf[i]=scan->getglucose<trend>(i);
	
	}

void saveglucose(const nfcdata*nfc,time_t tim,int id,int glu,int trend,float change) {
	savetrend(nfc,gettrendsbuf(scancount()));
	saveglucosedata(scans,getinfo()->scancount,tim, id, glu, trend, change);
	setlastscantime(tim);
	}

bool savepoll(time_t tim,int id,int glu,int trend,float change) {
	if(getinfo()->pollcount) {
		int count=getinfo()->pollcount-1;
		int previd=polls[count].id;
		if(previd>=id)  {
			LOGGER("GLU: duplicate id: previd=%d id=%d\n",previd,id);
			return false;
			}
		uint32_t prevt=polls[count].t;		
		uint32_t predict =prevt+(id-previd)*getinfo()->pollinterval;

		if((tim-predict)>3*60)   {
			LOGGER("GLU: oldvalue: pollinterval=%f %d prev=%d savepoll %d nu=%lu,  %d %s",getinfo()->pollinterval,previd,prevt,id,tim,glu,ctime(&tim));
			}
		else {
			constexpr const double weight=0.9;
			double af=(double)(tim-prevt)/(id-previd);
			getinfo()->pollinterval= weight*getinfo()->pollinterval+(1.0-weight)*af;
			}
		}
	saveglucosedata(polls,getinfo()->pollcount,tim, id, glu, trend, change);
	return true;
	}

void saveglucosedata(Mmap<ScanData> &scans,uint32_t &count,time_t tim,int id,int glu,int trend,float change) {
 	scans[count++]={static_cast<uint32_t>(tim),id,glu,trend,change};
	}

bool savepollallIDs(time_t tim,const int id,int glu,int trend,float change) {
	int count=getinfo()->pollcount;
	if(count<id) {
		uint32_t timiter=tim-(id-count)*60;
		for(;count<id;++count)  {
			scans[count]={timiter,count,0,0,0.0};
			timiter+=60;
			}
		}
	scans[id]={static_cast<uint32_t>(tim),id,glu,trend,change};
	if(id==count)
		getinfo()->pollcount=id+1;
	return true;
	}



const std::span<const ScanData> getScandata() const {
	return std::span<const ScanData>(scans.data(),scancount());
	}
const ScanData* beginscans() const {
	return scans.data();
	}
const std::span<const ScanData> getPolldata() const {
	return std::span<const ScanData>(polls.data(),pollcount());
	}
const ScanData *lastscan() const {
	const ScanData *start=scans.data();
	for(int i=scancount()-1;i>=0;i--) {
		const ScanData *sc=start+i;
		if(sc->g)
			return sc;
		}
	return nullptr;
	}
const ScanData *lastpoll() const {
	const auto polc=pollcount();
	if(polc>0)
		return polls.data()+polc-1;
	return nullptr;
	}
const ScanData *getscan(int ind) const {
	return scans.data()+ind;
	}
static void exportscans(const char *file,int count,const ScanData *scans)  {
	std::ofstream uit(file);
	for(int i=0;i<count;i++) {
		const ScanData &scan=scans[i];	
		uit<<scan.t<<"\t"<<scan.id<<'\t'<<scan.g<<'\t'<<scan.tr<<'\t'<<scan.ch<<std::endl;
		}
	uit.close();
	}

void exportscans(const char *file) const {
	exportscans(file,getinfo()->scancount,scans.data());
	}
void exportpolls(const char *file) const {
	exportscans(file,getinfo()->pollcount,polls.data());
	}
int nextlock()  {
	return getinfo()->lockcount++;
	}
void setlock(uint32_t lock) {
	getinfo()->lockcount=lock;
	}
const string &getsensordir() const {
	return sensordir;
	}
void removeoldstates()  {
extern void removeoldstates(const std::string_view dirin) ;
	removeoldstates(sensordir);
	}
const data_t *getpatchinfo() const {
	return reinterpret_cast<const data_t *>(&getinfo()->info);
	}
const data_t *getsensorident() const {
	return reinterpret_cast<const data_t *>(&getinfo()->ident);
	}
uint32_t lastused() const {
//	uint32_t last=(pollcount()>0)?lastpoll()->t:0;
	uint32_t last=getlastpolltime();
	if(last<getlastscantime())
		return getlastscantime();
	return last;
	}
/*
struct updatestate {
	uint32_t scansstart;uint32_t histsttart;uint32_t streamstart;
	bool infoupdated;
	};
	*/

void updateinit(const int ind) {
//	const int pos=offsetof(struct Info,update[ind+1]);
	const int pos=offsetof(struct Info,update[0])+sizeof(updatestate)*(ind+1);
//	const int pos= getinfo()->update[ind]-reinterpret_cast<uint8_t *>(getinfo());	

	if(meminfo.size()<pos) {
		meminfo.extend(pos);
//		getinfo()=reinterpret_cast<Info *>(meminfo.data());
		}
	getinfo()->update[ind]={};	
//	if(ind>=getinfo()->updatenr) getinfo()->updatenr=ind+1;
	}
static constexpr auto datelen=sizeof("2021-04-21-13:38:34")-1;

inline static constexpr const auto norawstream=std::numeric_limits<decltype(updatestate::rawstreamstart)>::max();


const ScanData *firstnotless(std::span<const ScanData> dat,const uint32_t start) {
	const ScanData *scan=&dat[0];
        const ScanData *endscan= &dat.end()[0];
//        const ScanData *endscan= scan+len;
        const ScanData scanst{.t=start};
        auto comp=[](const ScanData &el,const ScanData &se ){return el.t<se.t;};
        return std::lower_bound(scan,endscan, scanst,comp);
	}



dataonlyptr  getfromfile(crypt_t *pass,int sock, std::string_view filename,int offset,int asklen) {
	LOGGER("GLU: getfromfile ");
	const int pathlen=filename.size()+1;
	constexpr const int ali=alignof(struct askfile);
//	const int comlen=((sizeof(askfile)+(pathlen+ali-1))/ali)*ali;
	const int comlen=((sizeof(askfile)+(pathlen+ali-1))/ali)*ali;
//	const int totlen=sizeof(askfile)+(pathlen);
	alignas(ali) uint8_t buf[comlen];
	struct askfile *ask=reinterpret_cast<askfile *>(buf); 
	ask->com=saskfile;
	ask->off=offset;
	ask->len= asklen;

	ask->namelen=pathlen;
	memcpy(ask->name,filename.data(),pathlen);
//	ask->name[filename.size()]='\0';
	if(!noacksendcommand(pass,sock,buf,comlen) ) {
		LOGGER("GLU:  !noacksendcommand\n");
		return dataonlyptr(nullptr); 
		}
	return receivedataonly(sock,pass, asklen);

	};

int posearlier(int pos,uint32_t starttime) {
	for(int	i=pos-1;i>=0;i--) { //CHANGED
		if(uint32_t tim=timeatpos(i)) {
			if(tim<=starttime)
				return i+1; 	
			else
				return -1;
			}
		}
	return 0;
	}

int getbackuptimestream(uint32_t starttime)  {
	decltype(auto) poldat=getPolldata();
	int pos=firstnotless(poldat,starttime)-&poldat[0];
	LOGGER("GLU: setbackuptimestream pos=%d\n",pos);
	return pos;
	}
int  getbackuptimescan(uint32_t starttime)  {
	decltype(auto) scandat=getScandata();
	int pos=firstnotless(scandat,starttime)-&scandat[0];
	LOGGER("GLU: setbackuptimescan pos=%d\n",pos);
	return pos;
	}
	/*
bool setbackuptimes(crypt_t *pass,int sock,int ind,uint32_t starttime) {
	const int pathlen=infopath.size()+1;
	constexpr const int ali=alignof(struct askfile);
	const int comlen=((sizeof(askfile)+(pathlen+ali-1))/ali)*ali;
	const int totlen=sizeof(askfile)+(pathlen);
	const int comlen2=(ali-totlen%ali)+totlen;
	assert(comlen==comlen2);
	alignas(ali) uint8_t buf[comlen];
	struct askfile *ask=reinterpret_cast<askfile *>(buf); 
	const int asklen= offsetof(Info,pollinterval);
	ask->com=saskfile;ask->off=0;ask->len=asklen;
	memcpy(ask->name,infopath.data(),pathlen);
	if(!noacksendcommand(pass,sock,buf,comlen) ) {
		return false;
		}
       dataonlyptr res=receivedata(sock,pass, asklen) ;

	} */
uint32_t getbackuptimehistory(uint32_t starttime)  {
	int pos=gettimepos(starttime);
	const int endpos=getendhistory();
	const int first=getstarthistory();
	if(pos<first)
		pos=first;
	else {
		if(pos>endpos)
			pos=endpos;
		else {
			while(pos>first&&timeatpos(pos)>starttime) {
				LOGGER("GLU:  > ");
				pos--;
				}
			while(pos<endpos&&timeatpos(pos)<starttime) {
				LOGGER("GLU:  < ");
				pos++;
				}
			}
		}
	LOGGER("GLU: setbackuptimehistory pos=%d\n",pos);
	return pos;
	}
	/*
void setbackuptime(int ind,uint32_t starttime) {
	 setbackuptimestream(ind,starttime)  ;
	 setbackuptimescan(ind,starttime)  ;
	 setbackuptimehistory(ind,starttime)  ;
	 }
	 */
void backhistory(int pos) ;
bool setbackuptime(crypt_t *pass,int sock,int ind,uint32_t starttime) {
	
	const int asklen= offsetof(Info,pollinterval);
	const int minlen= offsetof(Info,pollcount);
//	const int asklen= sizeof(Info);
	LOGGER("GLU: %s setbackuptime %u asklen=%d\n",shortsensorname()->data(),starttime,asklen);
	auto dontdestroy=getfromfile(pass,sock,infopath, 0,asklen);
       dataonly *dat=dontdestroy.get();
	if(dat==nullptr) {
		LOGGER("GLU: ==nullptr\n");
		return false;
		}

	LOGGER("GLU: len=%d\n",dat->len);

	if(dat->len>=minlen) {
		const Info *infothere=reinterpret_cast<const Info*>(dat->data);

		if(getinfo()->update[ind].scanstart==0&&getinfo()->update[ind].histstart==0) {
			uint32_t histend=infothere->endhistory;
			uint32_t scantime= infothere->lastscantime;
			int histpos;
			uint32_t histstart=getinfo()->update[ind].histstart= (scantime<starttime&&(histpos=posearlier(histend,starttime))>=0)?histpos:getbackuptimehistory(starttime);
			uint32_t scanend=infothere->scancount;
			uint32_t scanstart=getinfo()->update[ind].scanstart=(scantime<starttime)?scanend:getbackuptimescan(starttime);
			LOGGER("GLU: hist start=%d endhistory=%d, scanstart=%d scancount=%d\n",histstart,histend,scanstart,scanend);
			}
		if(getinfo()->update[ind].streamstart==0) {
			uint32_t streamend=(dat->len<asklen)?0:infothere->pollcount;
			uint32_t streamstart=getinfo()->update[ind].streamstart=getbackuptimestream(starttime);
			LOGGER("GLU: streamstart=%d streamend=%d\n", streamstart,streamend);
			}
		}
		/*
	else {
		getinfo()->update[ind].histstart=0;
		getinfo()->update[ind].scanstart=0;
		getinfo()->update[ind].streamstart=0;
		} */
	return true;
	}
	
static uint16_t streamupdatecmd(int sensindex) {
	return sensindex<0?0:((1<<15)|sensindex);
	}
void setsendstreaming(int maxind) {
	updatestate  *up= getinfo()->update;
	for(int i=0;i<maxind;i++) {
		up[i].sendstreaming=true;
		}
	}

void sendbluetoothOn(int maxind) {
	updatestate  *up= getinfo()->update;
	for(int i=0;i<maxind;i++) {
		up[i].sendbluetoothOn=true;
		}
	}
int updatestream(crypt_t *pass,int sock,int ind,int sensindex)  {
	LOGGER("updatestream sock=%d ind=%d sensindex=%d\n",sock,ind,sensindex);
	int streamstart=getinfo()->update[ind].streamstart;
	struct {
		uint32_t pollcount;
		double pollinterval; 
		uint32_t lockcount;
		uint8_t bluetoothOn;
		} pollinfo;
	constexpr const int off=offsetof(Info,pollcount); 
	constexpr const int len=offsetof(Info,bluetoothOn)+sizeof(Info::bluetoothOn)-off;
	memcpy(&pollinfo,meminfo.data()+off,len);
	const int streamend=pollinfo.pollcount;	 //TODO test earlier?
	if(streamstart<streamend) {
		const struct ScanData *startstreams= polls.data();
		const uint16_t cmd=streamupdatecmd(sensindex);
#ifndef NDEBUG
		const struct ScanData *fn=startstreams+streamstart;
		time_t tim=fn->t;
		LOGGER("GLU: streamstart=%d streamend=%d %s %.1f (%d) dowith=%d %s",streamstart,streamend,polluit.data(),fn->g/18.0,fn->g,cmd,ctime(&tim));
#endif

		if(!senddata(pass,sock,streamstart,startstreams+streamstart,startstreams+streamend,polluit)) {
			LOGGER("GLU: senddata polls.dat failed\n");
			return 0;
			}
		if(!senddata(pass,sock,off,reinterpret_cast<uint8_t*>(&pollinfo),len, infopath,cmd)) {
			LOGGER("GLU: senddata info.data failed\n");
			return 0;
			}
		getinfo()->update[ind].streamstart=streamend;
		return 1;
		}
	else {
		if(getinfo()->update[ind].sendbluetoothOn) {
		      constexpr	const int offset=offsetof(Info,bluetoothOn);
		      if(!senddata(pass,sock,offset,meminfo.data()+offset,1, infopath)) {
				LOGGER("GLU: senddata bluetoothON info.data failed\n");
		      		return 0;
		   		}
			getinfo()->update[ind].sendbluetoothOn=false;
			return 1;
			}
		}
	return 2;
	}
	
private:
public:
int updatescan(crypt_t *pass,int sock,int  ind,int _unused)  { 
	return  updatescan( pass,sock, ind,false)&3; 
	}
int updatescan(crypt_t *pass,int ,int ,bool) ;

int lastlifecount=0;
time_t timelastcurrent=0;
time_t lifeCount2time(int lifecount) {
	return timelastcurrent-60*(lastlifecount-lifecount);
	}

};

struct lastscan_t {
	int sensorindex;
	const ScanData *scan;
	};
#endif

