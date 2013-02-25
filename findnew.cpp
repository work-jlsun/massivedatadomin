// findv.cpp 
//find the most common
#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <functional>
#include <vector>
#include <string.h>
#include <ext/hash_map>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>
using namespace __gnu_cxx;
using namespace std;
#define SPLITES     100		//the number of files
#define RETURNOK    "ok"
typedef struct KeyValue {
	int key;
	int value;
}KeyValue;

//store each piece of data
typedef struct filev {		
	vector<int> datas;
	unsigned char  state; 
}filev;


//store the data ,in the reality  it may be the db files or  just filesystem file  in the disk 
typedef struct  StoreV {			
	filev  fv[SPLITES];
	int splits;
	volatile int cur;
	pthread_mutex_t  mutex;	//protect the currren
}StoreV;

//the information used for the threads
typedef struct TInfo {
	StoreV *db;
	vector<KeyValue> *results;
}TInfo;

//initlize the store for another test
bool InitStore(StoreV *db){
	int err;
	int i;
	db->splits = SPLITES;
	db->cur = 0;	
	if ( (err = pthread_mutex_init(&(db->mutex),NULL)) != 0 ) {
		return false;
	}
	for (i = 0;  i != db->splits; i++){
		db->fv[i].datas.clear();
	}
	return true;
}
/*
save  the key into the  storage , and  break the keys into servaral set 
in reality the number of key maybe very huge,the storage should be disk or other durable storage
*/
bool 
StoreValue(StoreV *db,int key){
	int n;
	if (db == NULL){
		return false;
	}
	n = key % SPLITES;	//file to store the value
	db->fv[n].datas.push_back(key);
	return true;
}

/*
the thread should fix the file that has not been dealed,
this oper shoud be serial
*/
inline filev*  
GetFile(StoreV *db){
	int cur;
	pthread_mutex_lock(&(db->mutex));
	cur = db->cur;
	if (cur >= SPLITES){
		pthread_mutex_unlock(&(db->mutex));
		return NULL;
	}
	//cout << "cur:" << cur << endl;
	db->cur++;
	pthread_mutex_unlock(&(db->mutex));
	return &(db->fv[cur]);
}
/*
record the data of every set
*/
inline void
ReturnOut(StoreV *db, KeyValue kv, vector<KeyValue> *results){
	pthread_mutex_lock(&(db->mutex));
	results->push_back(kv);
	pthread_mutex_unlock(&(db->mutex));
}
/*
*thread ot process the sets
*/
void * 
find_common(void *args ){
	int key = 0;
	int value = 0;
	int max_key = 0;
	int max_value = 0;
	KeyValue retkey = {0,0};
	hash_map<int,int> hs;		//key, and the value
	vector<int>::iterator  iter;
	hash_map<int,int>::iterator hiter;
	filev *fv;
	TInfo *info = (TInfo *)args;
	StoreV *db = info->db;
	vector<KeyValue> *results = info->results;
	
	while (true) {
		fv = GetFile(db);
		value = 0;
		if (fv == NULL ) { break;}
		for (iter = fv->datas.begin(); iter != fv->datas.end(); iter++){
			
			key = *iter;
			hiter = hs.find(key);
			if ( hiter  == hs.end()) {	//insert and fix
				value = 1;
				hs[key] = value;
			} else {
				value = hiter->second;   
				value++;
				hiter->second = value;	
			}
			if (value > max_value ){	 
				max_key = key;		//ok,this is global for every file
				max_value = value;
			}
			
		}
		hs.clear();		//release the  mem
	}
	//this thread's result
	retkey.key = max_key;
	retkey.value = max_value;
	ReturnOut(db, retkey, results);
	pthread_exit((void *)RETURNOK);
}


/*
*create process thread
*/
pthread_t
makethreads( void*(*fn)(void *), void *arg){
	int  err;
	pthread_t tid;
	//pthread_attr_t attr;
	err = pthread_create(&tid, NULL, fn, arg);
	if (err == 0){
		return tid;
	} else {
		return err;
	}
}

/*
*get the MostCommon data of the hole big set
*nthread: number of thread 
*mem:member per thread ,now it is not consider here
     if the data set to be process is very huge ,we should
    get the data part by part from the durable storage
*/

KeyValue 
GetMostCommon(StoreV *db,int nthreads, int mem){
	int i;
	int err;
	bool flag = true; 
	pthread_t *tids =  NULL;
	vector<KeyValue>  results;
	vector<KeyValue>::iterator iter;
	TInfo info ={db,&results};
	void*  thread_result;
	KeyValue Mostkv = {0,0};
	tids = (pthread_t *)malloc( nthreads * sizeof(pthread_t) );
	for(i = 0; i != nthreads; i++ ) {
		tids[i] =  makethreads(find_common, (void *)&info);	
	}
	
	for(i = 0; i != nthreads; i++ ) {
		if (tids[i] > 0 ){
			err  = pthread_join(tids[i],&thread_result);
			if (err != 0){
				cout << "thread join failed\n" << endl;
				assert(1);
			}
			if(strcmp((char *)thread_result,RETURNOK) != 0){
				flag = false;	
			}
			
		}
	}
	free(tids);	
	if( flag == false){		//some thread  not correctly exit, the result may be not right, discard it,user find it ,may retry 
		cout << "thread error\n" << endl; 
		Mostkv.value = 0;
	}
	else{				//all complete
	   	cout << "nice , all ok" << endl;
		for (iter = results.begin(); iter != results.end(); iter++){
			if( iter->value > Mostkv.value){
				Mostkv.key = iter->key;
				Mostkv.value = iter->value;	
			}
		}
	}
	cout << "result:" << Mostkv.key << ":" << Mostkv.value << endl;
	return Mostkv;
}

/*
*test here
*/
int  
main(int argc, char* argv[])
{
	int num;
	int data;
	long duration1;
	long duration2;
	struct timeval start, finish;
	KeyValue result;
	StoreV db;
	srand((unsigned int)time(0));
	while (true){
			if ( InitStore(&db) != true ) {
				return 0;		
			}
			cout << "insert how many datas:";
			cin  >> num;
			for(int i =0; i != num; i++){
				data = rand()%1000000;
				StoreValue(&db, data);	
			}
			cout << endl;
			cout << "insert how many threads:";
			cin  >> num;
			gettimeofday(&start,NULL);//singlethread process
			GetMostCommon(&db, 1, 0);
			gettimeofday(&finish,NULL);
			duration1 = finish.tv_sec - start.tv_sec;
			duration2 = finish.tv_usec - start.tv_usec;
			cout << "duration:" << duration1 << ":" << duration2 << endl;  
			db.cur = 0;		//use the same data, but multithread process 
			gettimeofday(&start,NULL);
			GetMostCommon(&db, num, 0);
			gettimeofday(&finish,NULL);
			duration1 = finish.tv_sec - start.tv_sec;
			duration2 = finish.tv_usec - start.tv_usec;
			cout << "duration:" << duration1 << ":" << duration2 << endl;  
			cout << endl;
	}
	system("pause");
	return 0;
}



