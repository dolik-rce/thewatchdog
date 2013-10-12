#ifndef _DynamicSql_Common_h_
#define _DynamicSql_Common_h_

#if __cplusplus < 201103L

#include <pthread.h>

pthread_key_t TLSessionKey = 0;

template <class T>
void* GetTLSession(){
	if (TLSessionKey==0)
		pthread_key_create(&TLSessionKey, NULL);
	void* data = pthread_getspecific(TLSessionKey);
	if (!data) {
		data = new T;
		pthread_setspecific(TLSessionKey, data);
	}
	return data;
}

#endif

#endif
