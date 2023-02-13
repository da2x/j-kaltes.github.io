#ifndef NUMDATA_H
#define NUMDATA_H
#include <algorithm>
#include <string_view>
#include <type_traits>
#include <stdint.h>
#include <memory>
#include <stdlib.h>

#include <string.h>
#include <filesystem>
#include "inout.h"
 #include <assert.h>
#include "settings/settings.h"
#include "net/backup.h"
#include "datbackup.h"
#include "meal/Meal.h"
extern Meal *meals;
extern Backup *backup;

typedef std::conditional<sizeof(long long) == sizeof(int64_t), long long, int64_t >::type identtype; //to get rid of %lld warning
constexpr size_t nummmaplen=77056;//  ((sizeof(size_t)==4)?64ul*1024ul*1024ul:(1024ul*1024*1024ul))/sizeof(Num);
//constexpr size_t nummmaplen= sizeof(size_t)==4?(1024ul*1024*1024ul):(256*1024ul*1024*1024ul);

namespace fs = std::filesystem;
#include "num.h"
struct showitem {
  uint32_t time;
  float value;
  const char *type;
  };
struct Daypos {
int start;
int end;
};
class Numdata {
//static constexpr const int weightstart=4;
static constexpr const int receivedbackuppos=16;
static constexpr const int newidentpos=32;
static constexpr const int lastpolledpos=120;
static constexpr const int lastpos=128;
static constexpr const int changedpos=136;
static constexpr const int firstpos=144;
static constexpr const int onechange=152;
static constexpr const int datastart=256;
std::unique_ptr<char[]> newnumsfile;
protected:

identtype ident;
Mmap<Num> nums;
//Mmap<Daypos> daypos;
//int lastday=0;
/*(
inline static char *_labelbase=nullptr;
//char **labels=nullptr;
inline static  const  string_view labelsex[]={"var1","var2","var3","var4","var5","var6","var7","var8"};
inline static uint32_t labelcount=std::size(labelsex);
inline static string_view *labels=const_cast<string_view*>(labelsex);
*/
public:

//friend void numdisplay::remake() ;
//Numdata(const std::string_view base,identtype id,size_t len=0):ident(id>=0L?id:readident(base)),nums(numfilename(base,ident).get(),len),daypos(identfilename(base,"/dayspos%ld.dat",ident).get(),365*10)
/*
Numdata(const std::string_view base,identtype id,size_t len=0):ident(id>=0L?id:readident(base)),nums(numfilename(base,ident).get(),len){
	LOGGER("ident=%lld\n",ident);
	if(getlastpos()*3/2> nums.size()) {
		auto newsize=nums.size()*10;
		nums.extend(numfilename(base,ident).get(),newsize);
		}
	else {
		if(len)
			addmagic();
		}
	} */


Numdata(const std::string_view base,identtype id,size_t len=0):newnumsfile(numfilename(base,id==0L?0L:-1L)), ident(renamefile(base,id)),nums(newnumsfile.get(),len){
	LOGGER("ident=%lld\n",ident);
	if(ident!=-1L)
		setnewident(ident);
	if(getlastpos()*3/2> nums.size()) {
		auto newsize=nums.size()*10;
		nums.extend(newnumsfile.get(),newsize);
		}
	else {
		if(len)
			addmagic();
		}
	}

//Numdata(const std::string_view base):Numdata(base,readident(base)) { }
Numdata(const std::string_view base):Numdata(base,-1L) { }


void renamefromident(std::string_view base,identtype ident) {
	auto from=numfilename(base,ident);
	if(!access(from.get(),F_OK))  {
		rename(from.get(),newnumsfile.get());
		}
	}
identtype renamefile(std::string_view base,identtype id) {
	if(id==0L)	
		return id;
	identtype ident=readident(base);
	if(ident==-1L) 
		return id;
	renamefromident(base,ident);
	if(id>0L)	
		return id;
	return ident;
	}

#define  devicenr "devicenr"
//static constexpr const char devicenr[]="devicenr";
static identtype readident(const std::string_view base) {
	pathconcat ident{base,devicenr};
	Readall	all(ident);
	if(all.data())
		return *reinterpret_cast<identtype*>(all.data());
	return -1L;
	}
static bool writeident(const std::string_view base,identtype ident) {
	const char *uit=reinterpret_cast<const char *>(&ident);
	pathconcat filename{base,devicenr};
	return writeall(filename,uit,sizeof(identtype));
	}
	

bool valid(const Num *num)const {
	uint32_t start=begin()->time;
	uint32_t endtime=(end()-1)->time;
	if(num->time&&num->time>=start&&num->time<=endtime&&num->type<settings->getlabelcount())
		return true;
	return false;
	}
	/*
uint32_t starttime= UINT32_MAX;
void firstime(void) {
	starttime= UINT32_MAX;
	Num *en=end();
	for(Num *it=begin();it<en;it++) {
		if(valid(it)) {
			if(auto tim=it->time;tim<starttime)  {
				time_t t=tim;
				LOGGER("tim %s",ctime(&t));
				t=starttime;
				LOGGER(" < starttime %s",ctime(&t));
				starttime=tim;
				}
			}
		}
	time_t t=starttime;
	LOGGER("starttime %s",ctime(&t));
	}	
	*/
static constexpr unsigned char magic[]={0xFE,0xD2,0xDE,0xAD};
unsigned char *raw() {
	return reinterpret_cast<unsigned char *>(nums.data());
	}
const unsigned char *raw() const {
	return reinterpret_cast<const unsigned char *>(nums.data());
	}
 void addmagic() {
	 memcpy(raw(),magic,sizeof(magic));
	 }
bool trymagic() const {
	const unsigned char *ra=raw();
	return !memcmp(magic,ra,sizeof(magic));
	}

/*
static bool writelabels(const char *name)  {
	Create file(name);
	if(file==-1) {
		LOGGER("writelabels Open ");
		lerror(name);
		return false;
		}
	for(int i=0;i<labelcount;i++) {
//		int len=strlen(labels[i]);
		int len=labels[i].size();
		const char nl[]="\n";
		if(write(file,labels[i].data(),len)!=len) {
			lerror(name);
			return false;
			}
		if(write(file,nl,sizeof(nl)-1)!=1) {
			lerror(name);
			return false;
			}
		}
	return true;
	}
*/
static bool mknumdata(const string_view base,identtype ident)   {
	 if(struct stat st;stat(base.data(),&st)||!S_ISDIR(st.st_mode)) {
//	if(!fs::is_directory(basepath))  
		fs::path basepath(base.cbegin(),base.cend());
		std::error_code err;
		if(!fs::create_directories( basepath,err)) {
			LOGGER("create_directories(%s) failed: %s\n",basepath.c_str(),err.message().data());
			return false;
			}
		}
	if(ident>=0L) {
		if(!writeident(base,ident)) {
			LOGGER("writeident failed\n");
			return false;
			}
		}
	return true;
	}
static Numdata* getnumdata(const string_view base,identtype ident,size_t len)   {
	if(mknumdata(base,ident))
		return new Numdata(base,ident,len);
	return nullptr;
	}
Numdata* createnew(const string_view base,identtype ident,size_t len)  const {
	fs::path basepath { base.data()};
	if(!fs::is_directory(basepath))  {
		std::error_code err;
		if(!fs::create_directories( basepath,err)) {
			LOGGER("create_directories(%s) failed: %s\n",basepath.c_str(),err.message().data());
			return nullptr;
			}
		}
		/*
	if(!writelabels(base,ident)) {
		LOGGER("Writelabels failed\n");
		return nullptr;
		}
		*/
	if(!writeident(base,ident)) {
		LOGGER("writeident failed\n");
		}
	Numdata *numdata=new Numdata(base,ident,len);
	numdata->addmagic();
	return numdata;
	}
static std::unique_ptr<char[]> numfilename(const std::string_view path ,identtype ident) {
	return identfilename(path,"/nums%lld",ident);
	}
static std::unique_ptr<char[]> identfilename(const std::string_view path ,const std::string_view format,identtype ident) {

	int pathlen=path.size();
	if(path.data()[pathlen-1]=='/')
		pathlen--;
	const int totlen= pathlen+20+format.size();
//	std::unique_ptr<char[]> name= make_unique_for_overwrite<char[]>(totlen);
	std::unique_ptr<char[]> name( new char[totlen]);

	memcpy(name.get(),path.data(),pathlen);
	snprintf(name.get()+pathlen,totlen-pathlen,format.data(),ident);
	return name;
	}
void setfirstpos(int first)  { 
	auto ra=raw();
	*reinterpret_cast<int *>(ra+firstpos)=first;
	}
void setlastpos(int last)  { 
	auto ra=raw();
	int *was=reinterpret_cast<int *>(ra+lastpos);
	if(getchangedpos()==*was)
		getchangedpos()=last;
		
	*was=last;
	LOGGER("setlastpos=%d\n",last);
	}
void setlastpolledpos(int last)  { 
	auto ra=raw();
	*reinterpret_cast<int *>(ra+lastpolledpos)=last;
	}
int getlastpolledpos() const {
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return *reinterpret_cast<const int *>(raw+lastpolledpos);
	}
void setnewident(identtype newident)  { 
	ident=newident;
	auto ra=raw();
	*reinterpret_cast<identtype *>(ra+newidentpos)=newident;
	}
identtype  getnewident() const {
	auto ra=raw();
	return *reinterpret_cast<const identtype *>(ra+newidentpos);
	}
bool &receivedbackup()  {
	char *raw=reinterpret_cast< char *>(nums.data());
	return *reinterpret_cast< bool *>(raw+receivedbackuppos);
	}

void updated(int pos) {
	setlastpolledpos(pos);
	
	if(pos>getlastpos()||ident!=0L) 
		setlastpos(pos);
	if(pos>getchangedpos()) {
		getchangedpos()=pos;
		}
	if(pos>=getonechange())
		getonechange()=0;
	}
void updated(int start,int end) {
	updated(end);
	updatepos(start,end);
	}
int getfirstpos() const { /*File functions: from which position in file starting at 256, is data present  each element is 16 bytes long*/
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return *reinterpret_cast<const int *>(raw+firstpos);
	}
int getlastpos() const {
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return *reinterpret_cast<const int *>(raw+lastpos);
	}
int &getchangedpos()  {
	char *raw=reinterpret_cast<char *>(nums.data());
	return *reinterpret_cast<int *>(raw+changedpos);
	}
const int getchangedpos() const  {
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return *reinterpret_cast<const int *>(raw+changedpos);
	}
int &getonechange()  {
	char *raw=reinterpret_cast<char *>(nums.data());
	return *reinterpret_cast<int *>(raw+onechange);
	}
const int getonechange() const  {
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return *reinterpret_cast<const int *>(raw+onechange);
	}

void setonechange(const Num *num) {
	const int pos=num-startdata();
	setonechange(pos);
}
void setonechange(int pos) {
	if(getonechange()>0) {
		setlastchange(getonechange());
		getonechange()=0;
		setlastchange(pos);
		}
	else {
		if(pos==0)  
			setlastchange(0); //I already set that getonechange()=0 means nothing set. So what can I do? It is only in the very beginning
		else
			getonechange()=pos;
		}
	}

void inclastpos() {
	++*reinterpret_cast<int *>(raw()+lastpos);
	}

void inclastpolledpos() {
	++*reinterpret_cast<int *>(raw()+lastpolledpos);
	}
private:
template <class Cl,typename T>
static T *begin(Cl *cl) {
	return cl->nums.data()+16+cl->getfirstpos();
	}
template <class Cl,typename T>
static T *end(Cl *cl) {
	return cl->nums.data()+16+cl->getlastpos();
	}
public:
const Num *startdata() const {
	return nums.data()+16;
	}
 Num *startdata()  {
	return nums.data()+16;
	}
Num *begin() { return begin<Numdata,Num>(this); }
const Num *begin() const { return begin<const Numdata,const Num>(this); }

Num *end() { return end<Numdata,Num>(this); }
const Num *end() const { return end<const Numdata,const Num>(this); }
	/*
Num *begin() {
	return nums.data()+16+getfirstpos();
	}
Num *end() {
	return nums.data()+16+getlastpos();
	}
	*/

showitem getitem(const int pos) const { /* the pos position, starting with 0 end ending with size()-1 */
	const Num *item =startdata()+pos;
	return {item->time,item->value,settings->getlabel(item->type).data()};
	}
Num &at(const int pos) {
	Num *item =startdata()+pos;
	return *item;
	}
Num & operator[](const int pos) {
	return at(pos);
	}
const Num &at(const int pos) const {
	const Num *item =startdata()+pos;
	return *item;
	}
const Num & operator[](const int pos) const {
	return at(pos);
	}
/*
float *getweights() {
	char *raw=reinterpret_cast<char *>(nums.data());
	return reinterpret_cast<float *>(raw+weightstart);
	}
const float *getweights() const {
	const char *raw=reinterpret_cast<const char *>(nums.data());
	return reinterpret_cast<const float *>(raw+weightstart);
	}
	*/
const Num *data() const {
	return begin();
	}
Num *data() {
	return begin();
	}
int size() const {
	return getlastpos()-getfirstpos();
	}
virtual ~Numdata() {
	}
Num* firstAfter(const uint32_t endtime)  {
	 Num zoek {.time=endtime};
	 Num *beg=  begin();
	 Num *en= end();
	auto comp=[](const Num &el,const Num &se ){return el.time<se.time;};
  	return std::upper_bound(beg,en, zoek,comp);
	}
Num* firstnotless(const uint32_t tim) {
	Num zoek;
	zoek.time=tim;
	Num *beg=  begin();
	Num *en= end();
	auto comp=[](const Num &el,const Num &se ){return el.time<se.time;};
  	return std::lower_bound(beg,en, zoek,comp);
	}
/*
struct Num {
  uint32_t time;
  uint32_t reserved;
  float32_t value;
  uint32_t type;
  };
  */


void numsave( uint32_t time, float32_t value, uint32_t type,uint32_t mealptr) {
	LOGGER("savenum %f %s mealptr=%d\n",value,settings->getlabel(type).data(),mealptr);
	Num *num=firstAfter(time);
	Num *endnum=end();
	if(num<endnum)  {
		memmove(num+1,num,(endnum-num)*sizeof(Num));
		}
	mealptr=meals->datameal()->endmeal(mealptr);
	*num={time,mealptr,value,type};
	inclastpos();
	inclastpolledpos();
	setlastchange(num);
	updatepos(num-startdata(),getlastpos());
	 }

/*
void updated(int pos) {
	setlastpolledpos(pos);
	
	if(pos>getlastpos()||ident!=0L) 
		setlastpos(pos);
	if(pos>getchangedpos())
		getchangedpos()=pos;
	if(pos>=getonechange())
		getonechange()=0;
	}
*/
void numsavepos(int pos, uint32_t time, float32_t value, uint32_t type,uint32_t mealptr) {
	if(pos>0&&time<at(pos-1).time)  {
		at(pos).time=at(pos-1).time;
		}
	else {
		LOGGER("numsavepos %d %d %f %s\n",pos,mealptr,value,settings->getlabel(type).data());
		if(mealptr&&!meals->datameal()->goodmeal(mealptr)) {
			mealptr=0;
			}
		at(pos)={time,mealptr,value,type};
		receivedbackup()=false;
		}
/*	int nextpos=pos+1;
	updatepos(pos,nextpos);
	if(pos>=getlastpos())
		setlastpos(nextpos); 
	if(pos>=getlastpolledpos())
		setlastpolledpos(nextpos);
	setonechange(pos);*/
	 }
static constexpr uint32_t removedtype=0xFFFFFFFF;

void numremove(int pos) {
	LOGGER("numremove(%d)\n",pos);
	at(pos).type=removedtype;
	if(pos>0)
		at(pos).time=at(pos-1).time; //Deleted items to be ordered in time, for binary search
	else
		at(pos).time=at(pos+1).time;
	updatepos(pos,pos+1);
		/*
	int vers=getlastpos()-pos;
	if(vers==1) {
		int lastpos=getdeclastpos();
		setlastpos(lastpos); 
		setlastpolledpos(lastpos); //was remove command from watch 
		}
		*/
	}
int index(const Num *num)const {
	return num-startdata();
	}
	/*
int numremove(Num *num) { //Ook effect in NumberView
	int pos=index(num);
	numremove(pos);
	return pos;
	}
	*/
int getdeclastpos() {
//	--*reinterpret_cast<int *>(raw()+lastpos);
	Num *last=end()-1;
	last->type=removedtype;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsequence-point"
	last->time=(--last)->time;
#pragma GCC diagnostic pop
	for(;last>=begin()&&last->type==removedtype;last--)
		;
	return last-startdata()+1;
	}
	
int numremove(Num *num) {
	const int ver=end()-num;
	int pos=index(num);
	LOGGER("numremove(NUM %d)\n",pos);
	if(ver>0) {	
		if(ver>1) {
			memmove(num,num+1,(ver-1)*sizeof(Num));
			}
		setlastpos(getdeclastpos());
		setlastchange(pos);
		}
	updatepos(pos,getlastpos());
	return pos;
	}


void setlastchange(int pos) {
	if(pos<getchangedpos())
		getchangedpos()=pos;
	}
void setlastchange(const Num *el) {
	setlastchange(el-startdata());
	}

void setmealptr(Num *hit,uint32_t mealptr) {
	LOGGER("setmealptr(%d,%d)\n",index(hit),mealptr);
	hit->mealptr= meals->datameal()->endmeal(mealptr);
	}
void numchange(const Num *hit, uint32_t time, float32_t value, uint32_t type,uint32_t mealptr) {
	if(mealptr==0)
		mealptr=hit->mealptr;

	Num *num;
	int pos,lastupdate;
	if(hit->time!=time) {
		num=firstAfter(time);
		if(num<hit) {
			memmove(num+1,num,(hit-num)*sizeof(Num));
			setlastchange(num);
			pos=num-startdata();
			lastupdate=getlastpos();
			}
		else {
			if(num>(hit+1)) {
				num--;
				Num *modhit=const_cast<Num *>(hit);
				memmove(modhit,modhit+1,(num-modhit)*sizeof(Num));
				setlastchange(modhit);
				pos=modhit-startdata();
				lastupdate=getlastpos();
				}
			else  {
				num=const_cast<Num *>(hit);
				setonechange(hit);
				pos=num-startdata();
				lastupdate=pos+1;
				}
			}
		}
	else {
		num=const_cast<Num *>(hit);
		setonechange(hit);
		pos=num-startdata();
		lastupdate=pos+1;
		}
	LOGGER("numchange %d %d\n",pos,mealptr);
	mealptr=meals->datameal()->endmeal(mealptr);
	*num={time,mealptr,value,type};
	updatepos(pos,lastupdate);
	}
std::pair<const Num*,const Num*> getInRange(const uint32_t start,const uint32_t endtime) const {
	Num zoek;
	zoek.time=start;

	const Num *beg=  begin();
	const Num *en= end();
	auto comp=[](const Num &el,const Num &se ){return el.time<se.time;};
  	const Num *low=std::lower_bound(beg,en, zoek,comp);
	if(low==en) {
		return {en,en};
		}
	zoek.time=endtime;
  	const Num *high=std::upper_bound(low,en, zoek,comp);
	return {low,high};
	}
uint32_t getlasttime() const {
	const Num *en=end();
	if(en>begin()) {
		return (--en)->time;	
		}
	return 0;
	}
uint32_t getfirsttime() const {
	const Num *first=begin();
	const Num *endnum=end();
	Num zoek;
	zoek.time=0;
	auto comp=[](const Num &el,const Num &se ){return el.time<se.time;};
  	const Num *nonnull=std::upper_bound(first,endnum, zoek,comp);
	time_t tim=(nonnull==endnum)?UINT32_MAX:nonnull->time;
	LOGGER("getfirsttime()=%ld %s",tim,ctime(&tim));
	return tim;
	}
	/*if(first<end()&&first->time)

		return first->time;
	return  UINT32_MAX; */

	/*
const identtype getident() const {
	return ident;
	} */
const bool needsync() const {
	return ident!=0L;
	}
const int getindex() const { //index in Java niet numdatas
	LOGGER("%p, ident=%lld\n",this,ident);
	if(ident==0L)
		return 1;
	return 0;
	}
/*
struct changednums {
	int len;
	struct numspan changed[100];
	};
void updatesizeon() {
	const int ind=getindex();
	const int hnr=backup->getsendhostnr();
	for(int i=0;i<hnr;i++) 
		backup->getnums(i,ind)->updatedsize=false;	
	}
	*/
	
void updatesize() {
//	updatesizeon();
//	backup->wakebackup(Backup::wakenums);
	}

void updateposnowake(int pos,int end) {
	int ind=getindex();
	const int hnr=backup->getsendhostnr();
	LOGGER("updateposnowake ind=%d hnr=%d) ",ind,hnr);
	for(int i=0;i<hnr;i++) {
		struct changednums *nu=backup->getnums(i,ind); 
		LOGGER("nu->len=%d\n",nu->len);
		int st=nu->changed[0].start;
		LOGGER("st=%d\n",st);
		if(pos<st) {
			if(pos==(st-1))
				nu->changed[0].start=pos;	
			else {
				if(nu->len>=maxchanged) {
					int mini=nu->changed[0].start;
					for(int i=1;i<maxchanged;i++) {
						if(nu->changed[i].start<mini)
							mini=nu->changed[i].start;
						}
					nu->changed[0].start=mini;
					nu->len=1;
					}
				else
					nu->changed[nu->len++]={pos,end};
				}
			}


//		nu->updatedsize=false;	
		}
	}
void updatepos(int pos,int end) {
	if(backup) {
		updateposnowake(pos,end);
		backup->wakebackup(Backup::wakenums);
		}
	}


#include "net/passhost.h"
//bool update(int sock,int &len, struct numspan *ch) 
static constexpr int sizeslen=onechange+4-lastpolledpos;
static_assert(sizeslen==36);

template <typename... Ts>
void remake(Ts... args) {
	nums.~Mmap();
	new(this) Numdata(args...);
	}

//std::unique_ptr<char[]> newnumsfile;
//static void renametoident(std::string_view base,identtype ident) {
/*
void remake() {
	if(ident==-1LL) {
		extern pathconcat numbasedir;
		pathconcat name(numbasedir,"watch");
		identtype id=readident(name);
		if(id>0) {
			LOGGER("NUM: remake(%s,%lld,%zu)\n",name.data(),id,nummmaplen);
			remake(name,id,nummmaplen);
			}
		}
	}
*/
bool happened(uint32_t stime,int type,float value) const {
	const Num *sta=begin();
	for(const Num *ptr=end()-1;ptr>=sta;ptr--) {
		const auto ti=ptr->gettime();
		if(ti<stime)
			return false;
		if(ptr->type==type&&ptr->value>value)
			return true;
		}
	return false;
	}



template <int nr,typename  T>
struct ardeleter { // deleter
    void operator() ( T ptr[]) {
        operator delete[] (ptr, std::align_val_t(nr));
    }
};

static bool	sendlastpos(crypt_t*pass,int sock,uint16_t dbase,uint32_t lastpos) {
	LOGGER("sendlastpos %hd %u\n",dbase,lastpos);
	lastpos_t data{snumnr,dbase,lastpos}; 
	return sendcommand(pass,sock,reinterpret_cast<uint8_t*>(&data),sizeof(data));
	}

//std::unique_ptr<char[]> newnumsfile;
//static void renamefromident(std::string_view base,identtype ident) {
bool numbackupinit(const numinit *nums) {
	LOGGER("numbackupinit\n");
	const identtype  newident=nums->ident;
	if(ident!=newident) {
		extern pathconcat numbasedir;
		pathconcat base(numbasedir,"watch");
		if(!writeident(base,newident)) //TODO needed?
			return false;
		}
	const uint32_t sendfirst=nums->first;
	setfirstpos(sendfirst);
	/*(
	int endpos=getlastpos();
	int begpos=getfirstpos();	
	if(endpos>begpos) {
		const uint32_t tim=at(endpos-1).time;
		for(int pos=endpos;pos<sendfirst;pos++)  {
			at(pos).type=removedtype;
			at(pos).time=tim;
			}
		if(sendfirst<begpos)
			setfirstpos(sendfirst);
		else
			setfirstpos(begpos);
		}
	else {
		setfirstpos(sendfirst);
		}
		*/
	return true;
	}

bool sendbackupinit(crypt_t*pass,int sock,struct changednums *nuall) {
	struct changednums *nu=nuall+getindex();	
	struct numspan *ch=nu->changed;
	ch[0].start=nu->lastlastpos=getfirstpos();
	nu->len=1;
	numinit gegs{.first=static_cast<uint32_t>(getfirstpos()),.ident=ident};
	 if(!sendcommand(pass, sock ,reinterpret_cast<uint8_t*>(&gegs),sizeof(gegs))) {
		LOGGER("NUM: sendbackupinit failure\n");
		return false;
		}
	LOGGER("NUM: sendbackupinit success\n");
//	nuall->init=false;
	return true;
	}
bool backupsendinit(crypt_t*pass,int sock,struct changednums *nuall,uint32_t starttime) {
	LOGGER("NUM: backupsendinit %u ",starttime);
	struct changednums *nu=nuall+getindex();	
	struct numspan *ch=nu->changed;
	if(starttime&&(getfirstpos()!=getlastpos()))  {
		asklastnum ask{.dbase=(bool)ident};
		 if(!noacksendcommand(pass,sock,reinterpret_cast<uint8_t*>(&ask),sizeof(ask))) {
		 	LOGGER("NUM: noacksendcommand asklastnum failed\n");
			return false;
			}
		auto ret=receivedata(sock, pass,sizeof(int));
		int *posptr=reinterpret_cast<int*>(ret.get());
		if(!posptr) {
			LOGGER("NUM: receivedata==null\n");
			return false;
			}
		int pos=*posptr;
	       LOGGER("NUM: get pos=%d ",pos);
		if(pos<=getfirstpos())
			goto STARTOVER;
		if(pos>getlastpos()) {
			pos=getlastpos();
			}
		const Num &num=at(pos-1);
		if(num.time>starttime) {
			pos=firstnotless(starttime)-startdata();
			LOGGER("NUM: startime pos=%d\n",pos);
			}
		ch[0].start=nu->lastlastpos=pos;
		nu->len=1;
		return true;
		}
	STARTOVER:
	LOGGER("NUM: zero start\n");
	return sendbackupinit(pass,sock,nuall);
	}

static inline constexpr const int intinnum=(sizeof(Num)/sizeof(uint32_t));
int update(crypt_t*pass,int sock,struct changednums *nuall) {
	struct changednums *nu=nuall+getindex();	
LOGGER("Num: update ");
	struct numspan *ch=nu->changed;
	int endpos=getlastpos();

	uint16_t dbase=(bool)ident;
//	const bool sendmagic=!ch->start&&!ch->end;

	int offoutnr=0;
	int totlen=0;
	for(int i=nu->len-1;i>=0;i--) {
		int chend=ch[i].end==0?endpos:ch[i].end;
		if(ch[i].start<chend) {	
			int len=sizeof(Num)*(chend-ch[i].start);
			totlen+=(len+8);
			offoutnr++;
			}
		}
	int ret;
	if(!offoutnr) {
		if(nu->lastlastpos==endpos)
			ret=2;
		else {
			if(!sendlastpos(pass,sock,dbase,endpos))
				return 0;
			ret=1;
			}
		}
	else {
		totlen+=sizeof(numsend);
		std::unique_ptr<uint8_t[],ardeleter<alignof(numsend),uint8_t>> destructptr(new(std::align_val_t(alignof(numsend))) uint8_t[totlen],ardeleter<alignof(numsend),uint8_t>());
		numsend *uitnums= reinterpret_cast<numsend *>(destructptr.get());
		uitnums->type=snums;
		uitnums->dbase=dbase;
		uitnums->nr=offoutnr;
		uitnums->totlen=totlen;
	//	uitnums->first=getfirstpos();
		uitnums->last=endpos;

		uint32_t *numsar=uitnums->nums;
		const Num *start=startdata();
		for(int i=nu->len-1;i>=0;i--) {
			const int chend=ch[i].end==0?endpos:ch[i].end;
			const int chstart=ch[i].start;
			if(chstart<chend) {	
				*numsar++=chstart;
				const int nr=chend-chstart;
				LOGGER("NUM SN%d: %d (%d)\n",dbase,chstart,nr);
				*numsar++=nr;
				const int datlen=nr*sizeof(Num);

				memcpy(numsar,start+chstart,datlen);

				numsar+=nr*intinnum;
				}
			}
		 if(!sendcommand(pass, sock ,destructptr.get(),totlen))
		 	return 0;
		ret=1;

		}
	nu->lastlastpos=endpos;
	ch[0].start=endpos;
	nu->len=1;
	return ret;
	}
	
//	int minpos=INT_MAX; if(off<minpos) minpos=off;
void resendtowatch() {
	receivedbackup()=true;
	setlastpolledpos(getlastpos());
	getchangedpos()=0;
	}
bool backupnums(const struct numsend* innums) { 
	const int nr=innums->nr;
	const uint32_t *numsar=innums->nums;
	Num *start=startdata();
	for(int i=0;i<nr;i++) {
		uint32_t off=*numsar++;
		uint32_t nr=*numsar++;
		LOGGER("NUM RN%d: %d (%d)\n",(bool)ident,off,nr);

		const Num *data=reinterpret_cast<const Num *>(numsar);
		const int datlen=nr*sizeof(Num);
		memcpy(start+off,data,datlen);
		numsar+=nr*intinnum;
		uint32_t eind=off+nr;
		if(off<getchangedpos()) 
			getchangedpos()=off;
		updateposnowake(off,eind);
		receivedbackup()=true;
		}
	setlastpos(innums->last);
	setlastpolledpos(innums->last); 
	
	if(backup)
		backup->wakebackup(Backup::wakenums);
	return true;
	}

//TODO send ident and send magic
//update sizes? startpos?
};
#undef  devicenr 

#endif
