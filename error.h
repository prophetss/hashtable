#ifndef _ERROR_H_
#define _ERROR_H_

#include <stdio.h>
#include <errno.h>

#ifdef _DEBUG_


#define info_display(...)	fprintf(stdout, __VA_ARGS__)

/*一般错误打印*/
#define error_display(...)	fprintf(stderr, __VA_ARGS__)

/*致命错误打印*/
#define exit_throw(...)	do {                                             		\
    error_display("Error defined at %s, line %i : \n", __FILE__, __LINE__); 	\
    error_display(__VA_ARGS__);                                         		\
    exit(0);                                                           		    \
} while(0)

/*系统错误打印*/
#define sys_exit_throw(...)	do {                                            	\
    error_display("Errno : %d, msg : %s\n", errno, strerror(errno));            \
    exit_throw(__VA_ARGS__);                                                    \
} while(0)


#else


#define info_display(...)

#define error_display(...)

#define exit_throw(...)	do { exit(0); } while(0)

#define sys_exit_throw(...) do {                                                \
    fprintf(stderr, "Errno : %d, msg : %s\n", errno, strerror(errno));          \
    exit(errno);                                                                \
} while(0)


#endif /*_DEBUG_*/


#endif /*!_ERROR_H_*/