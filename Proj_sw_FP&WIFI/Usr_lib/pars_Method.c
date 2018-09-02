#include "pars_Method.h"
#include "string.h"

void *memmem(void *start, unsigned char s_len, void *find, unsigned char f_len){

	unsigned char len	= 0;
			char *p		= start, 
				 *q		= find;
	
	while((p - (char *)start + f_len) <= s_len){
	
		while(*p ++ == *q ++){
		
			len ++;
			if(len == f_len)return (p - f_len);
		}
		
		q 	= find;
		len = 0;
	}
	
	return NULL;
}