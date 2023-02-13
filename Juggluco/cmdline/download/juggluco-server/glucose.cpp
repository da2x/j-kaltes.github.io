#include "settings/settings.h"
#include "datbackup.h"

void SensorGlucoseData::backhistory(int pos) {
	int maxind=backup->getupdatedata()->sendnr;
	auto *updateptr=getinfo()->update;
	for(int i=0;i<maxind;i++) {
		if(pos<updateptr[i].histstart)
			updateptr[i].histstart=pos;
		}

	}

	
string_view getpreviousstate(string_view sbasedir ) ; // delete[] should be called on result
int SensorGlucoseData::updatescan(crypt_t *pass,int sock,int ind,bool restoreinfo)  {
	bool did=false;
	constexpr const int startinfolen=offsetof(Info, pollcount);
	alignas(alignof(Info)) uint8_t infoptr[startinfolen];
	
	int histend=getendhistory(); 
	int histstart=getinfo()->update[ind].histstart;
	if(histstart<histend) {
		memcpy(infoptr,meminfo.data(),startinfolen);
		if(histstart>0)
			histstart--;//Last in history contains only raw value, so should be overwritten
		if(!senddata(pass,sock,histstart*getelsize(),elstart(histstart),(histend+1-histstart)*getelsize(), histpath)) {
			LOGGER("GLU: senddata data.data failed\n");
			return 0;
			}
		getinfo()->update[ind].histstart=histend;
		did=true;
		}
	int scanend=getinfo()->scancount;
	int scanstart=getinfo()->update[ind].scanstart;
	if(scanend>scanstart) {
		if(!did)
			memcpy(infoptr,meminfo.data(),startinfolen);
		LOGGER("GLU: updatescan\n");
		if(const struct ScanData *startscans=scans.data()) {
			if(scanpath) {
				if(senddata(pass,sock,scanstart,startscans+scanstart,startscans+scanend,scanpath)) {
					if(const std::array<uint16_t,16> *starttrends=trends.data()) {
						if(trendspath) {
							if(!senddata(pass,sock,scanstart,starttrends+scanstart,starttrends+scanend,trendspath)) {
								LOGGER("GLU: senddata trends.dat failed");
								return 0;
								}
							getinfo()->update[ind].scanstart=scanend;
							}
						}
					}
				else  {
					LOGGER("GLU: senddata current.dat failed\n");
					return 0;
					}
				did=true;
				}
			}
		}
	if(!did) {
		if(getinfo()->update[ind].sendstreaming) {
			memcpy(infoptr,meminfo.data(),startinfolen);
			goto dosendinfo;
			}
		}
	else {
		dosendinfo:
		LOGGER("GLU updatescan %s hist: %d-%d scan: %d-%d ", shortsensorname()->data(),histstart,histend,scanstart,scanend);
		if(restoreinfo) {
			std::vector<subdata> vect;
			vect.reserve(2);
			vect.push_back({infoptr,0,startinfolen});
			Readall<senddata_t> updateinfo(pathconcat(globalbasedir,updateinfopath));
			if(updateinfo) {
				vect.push_back({updateinfo.data(),startinfolen,updateinfo.size()});
				}
			if(!senddata(pass,sock,vect,infopath) ) {
				LOGGER("GLU: senddata vect info.data failed\n");
				return 0;
				}
			}
		else {
		 {
			std::vector<subdata> vect;
			vect.reserve(2);
			vect.push_back({infoptr,0,startinfolen});
	//		constexpr const int startdev=offsetof(Info, deviceaddress);
			constexpr const int startdev=offsetof(Info, bluetoothOn);
			constexpr const int devlen=offsetof(Info,libreviewscan)-startdev;
			static_assert((4+deviceaddresslen)==devlen);
			vect.push_back({((uint8_t*)meminfo.data())+startdev,startdev,devlen});
			 if(!senddata(pass,sock,vect, infopath)) {
					LOGGER("GLU: senddata info.data failed\n");
					return 0;
				  }
				  	
		         LOGGER("send device address %p %p %s\n", ((char*)meminfo.data())+startdev+4,getinfo()->deviceaddress,getinfo()->deviceaddress);
			  }
		   if(!senddata(pass,sock,0,reinterpret_cast<const unsigned char*>(meminfo.data())+startinfolen,meminfo.size()*sizeof(meminfo.data()[0])-startinfolen, updateinfopath)) {
				LOGGER("GLU: senddata updateinfo.data failed\n");
				return 0;
				}
			}
		if(string_view state=getpreviousstate(sensordir);state.data()) {
			Readall<senddata_t> dat(state.data());
			if(dat) {
				LOGGER("GLU: %s\n",state.data());
				if(!senddata(pass,sock,0,dat.data(),dat.size(),std::string_view(state.data()+specstart,state.size()-specstart))) {
					LOGGER("GLU: senddata %s failed\n",state.data());
					return 0;
					}
				int sensdirlen= sensordir.size();
				pathconcat link(std::string_view(sensordir.data()+specstart,sensdirlen-specstart),"state.lnk");
				LOGGER("GLU: link=%s\n",link.data());
				sensdirlen++;
				if(!senddata(pass,sock,0,reinterpret_cast<const senddata_t *>(state.data())+sensdirlen,state.size()-sensdirlen,link)) {
					LOGGER("GLU: senddata %s failed\n",link.data());
					return 0;
					}
				}
			delete[] state.data();	
			}
		  if(getinfo()->update[ind].sendstreaming) {
			getinfo()->update[ind].sendstreaming=false;
			return 5;
			}
		return 1;
		  }
	return 2;
	}

template <typename It,typename T>
It	find_last(It beg, It en,T el) {
		for(It it=en-1;it>=beg;it--)	
			if(*it==el)
				return it;
	return en;
	}
string_view getpreviousstate(string_view sbasedir ) { // delete[] should be called on result
	LOGGER("get previous state %s\n",sbasedir.data());
	constexpr const char start[]="/state.lnk";
	int baselen= sbasedir.length();
	if(sbasedir[baselen-1]=='/')
		baselen--;
	const int buflen=baselen+sizeof(start);
	char filename[buflen]; 
	memcpy(filename,&sbasedir[0],baselen);
	char *ptr=filename+baselen;
	memcpy(ptr,start,sizeof(start));
	struct stat st;
	if(stat(filename,&st)==0&&st.st_size!=0) {
		const string_view::size_type filesize=st.st_size;
		auto alllen=baselen+filesize+1;
		char *all=new char[alllen+1]; //*****
		char *startfile=all+baselen+1;
		if(readfile(filename,startfile,filesize)==filesize) {
		/*
			if(*startfile=='/') { //TODO remove after first use
				char *end=startfile+filesize;
				char *name=find_last(startfile+1,end,'/');
				if(name==end) {
					LOGGER("getprevious NO more / in %s\n",startfile);
					name=startfile;
					}
				name++;
				int endlen= end-name;
				memmove(startfile,name,endlen);
				alllen=baselen+endlen+1;
				} */
			memcpy(all,sbasedir.data(),baselen);
			all[baselen]='/';
			all[alllen]='\0';
			return {all,alllen};
			}
		else
			delete[] all;
		}
	return {nullptr,0};
	}

void Sensoren::removeoldstates()  {
/*
	uint32_t nu=time(nullptr);
	if(nu<settings->data()->unlinkstatestime) {
		return;
		} */
	for(int i=0;i<=last();i++) {
		if(SensorGlucoseData *hist=gethist(i))
			hist->removeoldstates(); 
		}
//	constexpr const uint32_t period=60*60*24;
//	settings->data()->unlinkstatestime=nu+period;
	}


