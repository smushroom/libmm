#ifndef     _DEBUG_H_
#define     _DEBUG_H_ 

#ifndef     _DEBUG_
#define     _DEBUG_
#endif 

#ifdef  _DEBUG_

#define RED     "\033[0;31m"        /*  0 -> normal ;  31 -> red */
#define CYAN    "\033[1;36m"        /*  1 -> bold ;  36 -> cyan */
#define GREEN   "\033[4;32m"        /*  4 -> underline ;  32 -> green */
#define BLUE    "\033[9;34m"        /*  9 -> strike ;  34 -> blue */

#define BLACK   "\033[0;30m"
#define BROWN   "\033[0;33m"
#define MAGENTA "\033[0;35m"
#define GRARY   "\033[0;37m"

#define NONE    "\033[0m"        /*  to flush the previous property */

#define DE(fmt, args...)  do{fprintf(stderr, fmt "\t%s[ File :%s, %sFunc :%s, Line :%d ]\n", ##args, RED, __FILE__,BLUE, __func__,__LINE__);}while(0)
#define DD(fmt, args...)  do{fprintf(stdout, fmt "\t%s[ File :%s, %sFunc :%s, %s, Line :%d ]\n", ##args, RED, __FILE__, GREEN,__func__,BROWN,__LINE__);}while(0)
#define     AE(expression)      do{assert(expression);}while(0)
#else
#define  DE(fmt, args...)
#define  DD(fmt, args...)
#define  AE(expression) 
#endif

#endif
