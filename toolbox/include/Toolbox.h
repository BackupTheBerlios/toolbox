//============================================================
// 	$Id: Toolbox.h,v 1.1 2004/05/12 22:04:48 plg Exp $
//============================================================
/* Copyright (c) 1999-2004, Paul L. Gatille <paul.gatille@free.fr>
 *
 * This file is part of Toolbox, an object-oriented utility library
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the "Artistic License" which comes with this Kit.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the Artistic License for more
 * details.
 */

// ---------< div macros and defines >------------------------------

#ifndef __TOOLBOX_H
#define __TOOLBOX_H

#include <limits.h>

#include <tb_Errno.h>

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS
#endif
#ifndef _REENTRANT
#define _REENTRANT
#endif
#ifndef MULTITHREAD
#define MULTITHREAD
#endif

#ifndef TRUE
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif


/**
 * log levels enum
 * mimics syslog's levels
 * \ingroup Global
 */
enum tb_loglevels {
	TB_EMERG = 0,   /**< system is unusable */
	TB_ALERT,       /**< action must be taken immediately */
	TB_CRIT,        /**< critical conditions */
	TB_ERROR,       /**< error conditions */
	TB_WARN,        /**< warning conditions */
	TB_NOTICE,      /**< normal but signification condition */
	TB_INFO,        /**< informational */
	TB_DEBUG,       /**< debug-level messages */         
};

#define TB_LOGLEVELS enum tb_loglevels
#define TB_FATAL TB_EMERG // compatibility ...

#define DEFAULT_ERRLEVEL  TB_WARN


#ifndef RELEASE
#define tb_debug(x...)   tb_trace(TB_DEBUG, x)
#define TB_ASSERT(A)      assert(A)
#else
#define tb_debug(x...)   
#define tb_info(x...)   
#define tb_notice(x...)   
#define TB_ASSERT(A)      (A)
#endif

/** Info level logging
 * \ingroup Global
 * @see tb_trace  */
#define tb_info(x...)    tb_trace(TB_INFO, x)   

/** Notice level logging
 * \ingroup Global
 * @see tb_trace  */
#define tb_notice(x...)  tb_trace(TB_NOTICE, x)

/** Warn level logging
 * \ingroup Global
 * @see tb_trace  */
#define tb_warn(x...)    tb_trace(TB_WARN, x) 

/** Error level logging
 * \ingroup Global
 * @see tb_trace  */
#define tb_error(x...)   tb_trace(TB_ERROR, x) 

/** Crit level logging
 * \ingroup Global
 * @see tb_trace  */
#define tb_crit(x...)    tb_trace(TB_CRIT, x) 

/** Alert level logging
 * \ingroup Global
 * @see tb_trace  */
#define tb_alert(x...)   tb_trace(TB_ALERT, x)

/** Emerg/Fatal level logging
 * \ingroup Global
 * @see tb_trace  */
#define tb_fatal(x...)   tb_trace(TB_FATAL, x)




/**
 * Return code values for most Toolbox functions
 * \warning TB_ERR != -1 !
 * \ingroup Global
 */
enum Retcode 
{  
	//   !!!!!! warning: big change here !!!! can break many buggy assertion that TB_ERR == -1
	TB_ERR = INT_MIN,  ///< Error occured 
	//was:	TB_ERR = -1,  ///< Error occured 
	TB_KO  =  0,  /**< ko / false */
	TB_OK  =  1,  /**< ok / true */
};

/**
 * \def retcode_t
 * \ingroup Global
 */
typedef enum Retcode    retcode_t;




// ---------------- KTypes -----------------
union _container_key
{
	int    ndx;  /**< key is an vector's offset */
	char * key;  /**< key is a hash's or dict string key */
	void * user; /**< key is of 'user' type */
};
typedef union _container_key tb_Key_t;

#define  KINVAL ((tb_Key_t)INT_MIN)

typedef tb_Key_t (*kcp_t)   (tb_Key_t);
typedef void     (*kfree_t) (tb_Key_t);
typedef int      (*kcmp_t)  (tb_Key_t,tb_Key_t);
typedef char *   (*k2sz_t)  (tb_Key_t,char *);

int  registerNew_Ktype(char *name, kcp_t  kcp, kcmp_t  kcmp, kfree_t kfree, k2sz_t k2sz);

retcode_t     kt_exists      (int KT);
const char  * kt_getKname    (int KT);
kcmp_t        kt_getKcmp     (int KT);
kcp_t         kt_getKcp      (int KT);
kfree_t       kt_getKfree    (int KT);
k2sz_t        kt_getK2sz     (int KT);
int           K2i            (tb_Key_t K);
char *        K2s            (tb_Key_t K);
void *        K2p            (tb_Key_t K);
tb_Key_t      i2K            (int    k);
tb_Key_t      s2K            (char * k);
tb_Key_t      p2K            (void * k);



typedef enum { TLS1=1,
							 SSL2,
							 SSL3,
							 SSL23
} ssl_meth_t;


// macros

#define TB_MIN(A,B)  ((A) < (B)) ? (A) : (B)
#define TB_MAX(A,B)  ((A) > (B)) ? (A) : (B)
#define TB_ABS(A)    (((A) >  0) ? (A) : (-A))
// snap x to low or high if under or over limits
#define TB_CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))


#define streq(A,B)    strcmp((A),(B)) == 0
#define strneq(A,B,C) strncmp((A),(B),(C)) == 0

// limits
#define MAX_BUFFER        1024    
#define MAX_PATH          256      // full filename

#include <sys/types.h>
#include <stdarg.h>
// ---------< globos /use with care/ > -----------------------------

/** global current loglevel. 
 * Used with tb_trace to set lower bound of logging. (if set to TB_WARN, tb_debug(), tb_notice() and tb_info() will not be displayed)
 * \ingroup Global
 */
extern TB_LOGLEVELS tb_errorlevel; 

// ---------< internal types > -------------------------------------

#define TB_OBJECT         0
#define TB_SCALAR         1
#define TB_CONTAINER      2
#define TB_COMPOSITE      3
#define TB_STRING         4
#define TB_RAW            5
#define TB_NUM            6
#define TB_POINTER        7
#define TB_HASH           8
#define TB_VECTOR         9
#define TB_DICT          10
#define TB_SOCKET        11
#define TB_ITERATOR      12
#define TB_XMLDOC        13
#define TB_XMLELT        14

// ---------< Types >------------------------------------------

/**
 * \typedef struct tb_Object *    tb_Object_t.
 * virtual base (parent) class for all Toolbox types
 * @see struct tb_Object, tb_isA
 */
typedef struct tb_Object *    tb_Object_t;
typedef struct __members *    __members_t;

inline void *tb_getMembers  (tb_Object_t This, int tb_type_id);
void         tb_freeMembers (tb_Object_t This); //FIXME: better fit into tb_ClassBuilder.h
retcode_t    TB_VALID       (tb_Object_t O, int toolbox_class_id);


/**
 * Generic container class.
 * @see tb_Object_t, Hash_t, Dict_t, Vector_t
 */
typedef tb_Object_t           Container_t;

/** 
 * Generic scalar class.
 * @see tb_Object_t, String_t, Num_t, Raw_t, Pointer_t
 */
typedef tb_Object_t           Scalar_t;

/** 
 * Generic composite class.
 * @see tb_Object_t, Socket_t, Iterator_t, XmlDoc_t, XmlElt_t
 */
typedef tb_Object_t           Composite_t;

/**
 * Hash table type (key/value association, with char *key)
 * Hash_t derivate from Container (hashtable).
 * @see Container_t, tb_Hash
 */
typedef Container_t           Hash_t;

/**
 *  Dynamic vector type, accessible from extremities (stack-wise), or offset.
 * Vector derivate from Container (extensible array with stack like add-ons)
 * @see Container_t, tb_Vector
 */
typedef Container_t           Vector_t;

/**
 * B+ Tree key/value association. 
 * Good insert/lookup perf, average remove perf (to be enhanced).
 * Allow sorted access from any element
 * @see Container_t, tb_Dict
 */
typedef Container_t           Dict_t;

/**
 * All purpose string container.
 *
 * @see Scalar_t, tb_String
 */
typedef Scalar_t              String_t;


/**
 * Dedicated storage for numerical types.
 * \todo rewrite needed to deal with floats, double, long, and bytes
 * @see Scalar_t, tb_Num
 */
typedef Scalar_t              Num_t;

/**
 * Encapsulates unknown/untyped pointer.
 * Allows user defined struct to be stored and manipulated as any tb_type
 * @see Scalar_t, tb_Pointer
 */
typedef Scalar_t              Pointer_t;

/**
 * same as String_t, but allow raw data to be stored (including 0x00 bytes)
 * @see Scalar_t, tb_Raw
 */
typedef Scalar_t              Raw_t;

/**
 * TCP/UDP/IP/UNIX socket all-in-one.
 * @see Composite_t, tb_Socket
 */
typedef Composite_t           Socket_t;

/**
 * Xml document.
 * Dom-alike represention for an xml object. Not truely Dom compliant, but slimmer, faster and enought for simple parsing
 * @see Composite_t, tb_XmlDoc, XmlElt_t
 */
typedef Composite_t           XmlDoc_t;

/**
 * Xml element.
 * Dom-alike represention for an xml object. Not truely Dom compliant, but slimmer, faster and enought for simple parsing
 * @see Composite_t, tb_XmlDoc, XmlElt_t
 */
typedef Composite_t           XmlElt_t;

/**
 * Iterator for containers.
 * @see Composite_t, tb_Iterator
 */
typedef Composite_t           Iterator_t;


// -- here comes the funcs --

 
int tb_Rlock(void * O);
int tb_Wlock(void * O);
int tb_Unlock(void * O);


/** Retreive Toolbox version
 * \ingroup Gobal
 * @return current Toolbox version ID
 *
 * \warning dont free returned char * (static object)
 *
 */
const char *tb_getVersion(void) ; 

/** Retreive Toolbox build 
 * \ingroup Gobal
 * @return current Toolbox build date and host
 * \warning dont free returned char * (static object)
 *
 */
const char *tb_getBuild(void) ; 


/** Setup wrapper for tb_trace
 * \ingroup Gobal
 * \todo doc
*/
void tb_setTraceFnc   ( void * );

/** Setup callback for internal syslog initialiser
 * \ingroup Gobal
 * \todo doc
*/
void tb_setGetNameFnc ( char*(*fnc)(void) );



// sprintf family various add-ons (portability concerns)

int     tb_vnasprintf(char **s, 
											unsigned int max,
											const char *fmt, 
											va_list ap);  

int     tb_nasprintf (char **s,
											unsigned int max,
											const char *fmt,
											...); 

int     tb_vasprintf (char **s,  // pointer to buffer to be allocated
											const char *fmt, // format
 											va_list ap); // see stdargs.h

int     tb_asprintf    (char **s, // pointer to buffer to be allocated
												const char *fmt, // format
												...); // variadic arguments

int     tb_vsnprintf   (char *str, size_t size, const  char  *format, va_list ap);

int     tb_snprintf    (char *str, size_t size, const  char  *format,...);





// --------------Objects constructors---------------------

String_t  tb_String          (char *fmt, ...);
String_t  tb_string_new      (int len, char *data); 
String_t  tb_nString         (int len, char *fmt, ...);
Num_t     tb_Num             (int value);
Raw_t     tb_Raw             (int len, char *data);
Pointer_t tb_Pointer         (void *data,	void *destroy);
Hash_t    tb_Hash            (void);
Hash_t    tb_HashX           (int key_type, int allow_duplicates);

/* Dict_t constructor (balanced B-tree) */


enum BASIC_KTYPES {
	KT_STRING = 0,
	KT_STRING_I,
	KT_INT,
	KT_POINTER,
};

Dict_t        tb_Dict           (int keytype, int dupes_allowed);
Vector_t      tb_Vector         (void);
Socket_t      tb_Socket         (int mode, ...); 


// -----------Object_t Methods---------------

tb_Object_t   tb_Clone          (tb_Object_t obj); 
tb_Object_t   tb_Alias          (tb_Object_t obj); 
void          tb_Free           (tb_Object_t obj);
uint          tb_isA            (tb_Object_t obj);
int           tb_getSize        (tb_Object_t obj);
tb_Object_t   tb_Clear          (tb_Object_t to);
void          tb_Dump           (tb_Object_t obj);
char        * tb_toStr          (tb_Object_t obj, ...); 
int           tb_toInt          (tb_Object_t to, ...);
tb_Object_t   tb_unMarshall     (String_t xml_element);
tb_Object_t   tb_XmlunMarshall  (XmlElt_t X);
String_t      tb_Marshall       (tb_Object_t O);

int           tb_EncodeBase64   (void *data, int size, char **str);
int           tb_DecodeBase64   (char *str, void **data);

// ------------Containers Methods-----------------

Iterator_t      tb_Iterator         (Container_t Cn);
retcode_t       tb_First            (Iterator_t It);
retcode_t       tb_Last             (Iterator_t It);
tb_Object_t     tb_Prev             (Iterator_t It);
tb_Object_t     tb_Next             (Iterator_t It);
tb_Key_t        tb_Key              (Iterator_t It);
tb_Object_t     tb_Value            (Iterator_t It);

Iterator_t      tb_Dict_upperbound  (Dict_t D, tb_Key_t key);
Iterator_t      tb_Dict_lowerbound  (Dict_t D, tb_Key_t key);
int             tb_Dict_numKeys     (Dict_t D);

retcode_t       tb_Replace          (Container_t to, tb_Object_t from, ...);
retcode_t       tb_Insert           (Container_t to, tb_Object_t obj, ...);
retcode_t       tb_Remove           (Container_t obj, ...);
retcode_t       tb_Exists           (Container_t obj, ...);
tb_Object_t     tb_Take             (Container_t obj, ...); 
tb_Object_t     tb_Get              (Container_t obj, ...);
Iterator_t      tb_Find             (Container_t C, tb_Key_t K);

//------------Accessors Functions--------------------------------

// Casts a String_t object to string zero.
// No copy done. Overcome encapsulation for quicker access.
// Don't mess with this string ; S2sz allows only read access.
// returns a char * on String_t internal data
// todo: macro -> tb_toStr
inline char  *  S2sz   (String_t O); //FIXME: to be deprecated

// Casts a String_t object to int.
// returns an int on String_t internal data
inline int     S2int   (String_t O); //FIXME: to be deprecated
// Casts Pointer_p object to is void *data member
// No copy done. Overcome encapsulation for quicker access.
// Don't mess with this string ; P2p allows only read access.
// returns: a void * on Pointer_t internal data
void        * P2p      (Pointer_t P);

// Casts a Num_t object to C string.
// returns: an c string or "NaN" if error
char        * N2sz     (Num_t N);

// Casts a Num_t object to integer.
// returns: int value
int           N2int    (Num_t N);

Num_t tb_NumSet(Num_t N, int val);


// ------------String_t methods----------------


// not stabilized : don't use yet
int       tb_getStrCharset           (String_t S);
int       tb_getStrEncoding          (String_t S);
// here come the transcoders

retcode_t tb_Latin1_to_Url           (String_t from);
retcode_t tb_Url_to_Latin1           (String_t from);
retcode_t tb_Latin1_to_MimeQP        (String_t from);
retcode_t tb_MimeQP_to_latin1        (String_t from);
retcode_t tb_Latin1_to_MimeB64       (String_t from);
retcode_t tb_MimeB64_to_latin1       (String_t from);
retcode_t tb_UTF8_to_Latin1          (String_t from);
retcode_t tb_Latin1_to_UTF8          (String_t from);

retcode_t tb_Str_to_Base64           (String_t from);
retcode_t tb_Base64_to_Str           (String_t from);

String_t  tb_UrlDecode               (String_t S);
String_t  tb_UrlEncode               (String_t S);

// String_t specific manipulators
int       tb_StrCmp                  (String_t S, char *search, int case_sensitive);
retcode_t tb_StrEQ                   (String_t S, char *match);
retcode_t tb_StrEQi                  (String_t S, char *match);
String_t  tb_StrUcase                (String_t S);
String_t  tb_StrLcase                (String_t S);
String_t  tb_StrFill                 (String_t S, int len, int start, char filler);
String_t  tb_StrAdd                  (String_t S, int start, char *fmt, ...);
String_t  tb_StrnAdd                 (String_t S, int len, int start, char *fmt, ...);
String_t  tb_RawAdd                  (String_t S, int len, int start, char *raw);
String_t  tb_StrDel                  (String_t  S, int start, int len);
String_t  tb_StrSub                  (String_t  S, int start, int len); 
retcode_t tb_StrRepl                 (String_t S, char *search, char *replace);
String_t  tb_Join                    (Vector_t V, char *delim);

// tokenize flags
#define TK_KEEP_BLANKS       0x5001 // toolbox misc add : store empty tokens
#define TK_ESC_QUOTES        0x5002 // toolbox misc add : preserve quoted strings

int       tb_tokenize                (Vector_t V, char *s, char *delim, int flags);
Vector_t  tb_StrSplit                (char *str, char *regex, int options);
String_t  tb_ltrim                   (String_t S);
String_t  tb_rtrim                   (String_t S);
String_t  tb_trim                    (String_t S);
String_t  tb_chomp                   (String_t S);

char    * tb_str2hex                 (char *bin, int len); 
void      tb_hexdump                 (char *bin, int len);

int        strsz           (char *s); //FIXME: to be deprecated


// ---------Vector_t Methods------------------------------------------

int             tb_getIdByRef (Vector_t V, tb_Object_t O);
retcode_t       tb_Push       (Vector_t V, tb_Object_t data);
tb_Object_t     tb_Pop        (Vector_t V);
retcode_t       tb_Unshift    (Vector_t V, tb_Object_t data);
tb_Object_t     tb_Shift      (Vector_t V);
char         ** tb_toArgv     (Vector_t V);
void            tb_freeArgv   (char **Argv);
Vector_t   tb_Sort       (Vector_t V, 
													int (*cmp)(tb_Object_t, tb_Object_t)); 
Vector_t   tb_Reverse    (Vector_t V);

#define TB_MRG_MOVE      1
#define TB_MRG_COPY      2
#define TB_MRG_ALIAS     3

Vector_t tb_Merge(Vector_t dst, // destination Vector_t
									Vector_t src, // source Vector_t
									int start,  // destination offset
									int flag); // move / copy / alias opération

Vector_t tb_Splice(Vector_t V, int start, int len, int flag);

// Hash tables ----------------------------------------------------------------

retcode_t   tb_HashFreeze   (Hash_t H);
retcode_t   tb_HashThaw     (Hash_t H);
tb_Key_t    tb_getKeybyRef  (Hash_t H, tb_Object_t O);
Vector_t    tb_HashKeys     (Hash_t H);

// ---------< Sockets >---------------------------------------------

/// Socket_t status alternatives
enum tb_sockstatus { 
	TB_BROKEN = 0,    //< error occured
	TB_UNSET,         //< not yet initialised
	TB_DISCONNECTED,  //< connection broken
	TB_CONNECTED,     //< in connected state
	TB_LISTENING,     //< server listening
	TB_TIMEDOUT,      //< timeout occured on last operation
	TB_SHUTDOWN,      //< shutdown requested on server
};

#define TB_SOCKSTATUS  enum tb_sockstatus

#define MAX_SERV_THREADS  100

enum tb_sock_family 
{ 
	TB_IP     = 1,
	TB_UNIX   = 2,
	TB_X25    = 4
};

enum tb_sock_proto 
{ 
	TB_TCP    = 8,
	TB_UDP    = 16,
};

#define TB_FAMILY   enum tb_sock_family
#define TB_PROTO    enum tb_sock_proto
#define TB_TCP_IP   (TB_IP   | TB_TCP)
#define TB_UDP_IP   (TB_IP   | TB_UDP)
#define TB_TCP_UX   (TB_UNIX | TB_TCP)
#define TB_UDP_UX   (TB_UNIX | TB_UDP)

// set_nonblock_flag: internal use (voir BLOCKING/NON_BLOCKING)
int       set_nonblock_flag   (int fd, int value); // FIXME: to be replaced
retcode_t tb_Connect(Socket_t So, int TO, int nb_tries);

#ifdef WITH_SSL

enum ssl_mode {
	SSL_CLIENT,
	SSL_SERVER
};

retcode_t tb_initSSL(Socket_t        S,          // target Socket_t
									 enum ssl_mode   mode,       // SSL_CLIENT  | SSL_SERVER
									 ssl_meth_t      method,     // SSL1 | SSL2 | SSL3 | TLS1
									 char          * CA_path,
									 char          * CA_file,
									 char          * cert,
									 char          * pwd,
									 char          * cipher);

typedef void (*ssl_validate_cb_t)(int, void *);
void tb_SSL_validate(Socket_t S, int bool, ssl_validate_cb_t validate);

#endif

retcode_t tb_setSockNoDelay(Socket_t S);
retcode_t tb_initServer(Socket_t O, // Socket_t server
											void *callback, // of type int (*)(Socket_t)
											void *cb_args); // args to be passed to callback

/* setup TCP/IP server ACL */
// fixme: where is the doc !!

enum acl_list {
	ACL_DENY=0,
	ACL_ALLOW
};
typedef enum acl_list acl_list_t;

int      tb_sockACL                         (Socket_t S, int use_acl);
int      tb_sockACL_ADD                     (Socket_t S, acl_list_t list, char *pattern); 
int      tb_sockACL_DEL                     (Socket_t S, acl_list_t list, char *pattern); 
int      tb_sockACL_CLEAR                   (Socket_t S, acl_list_t list); 
Vector_t tb_sockACL_LIST                    (Socket_t S, acl_list_t list);
int      tb_sockACL_set_global_max_hps      (Socket_t S, int max_hps);
int      tb_sockACL_get_global_max_hps      (Socket_t S);
int      tb_sockACL_set_global_max_simult   (Socket_t S, int max_simult);
int      tb_sockACL_get_global_max_simult   (Socket_t S);
int      tb_sockACL_set_host_max_hps        (Socket_t S, int max_hps);
int      tb_sockACL_get_host_max_hps        (Socket_t S);
int      tb_sockACL_set_host_max_simult     (Socket_t S, int max_simult);
int      tb_sockACL_get_host_max_simult     (Socket_t S);

retcode_t tb_stopServer     (Socket_t O);
void   *tb_getServArgs     (Socket_t O);
int     tb_getSockFD       (Socket_t O);
retcode_t tb_setServMAXTHR   (Socket_t O, int max);
int     tb_getServMAXTHR   (Socket_t O);
retcode_t tb_setSockTO       (Socket_t S, long int sec, long int usec);
retcode_t tb_getSockTO       (Socket_t S, long int *sec, long int *usec);
int     tb_getSockStatus   (Socket_t S);
void *tb_Accept              (void * S); // arg must be a tb_Socket
int       tb_readSock     (Socket_t So, String_t msg, int len);
int       tb_readSockLine (Socket_t O, String_t msg);
int       tb_writeSock    (Socket_t So, char *msg);

#define CLOSE_END          1
#define DONT_CLOSE         0

//-------< Xml (lite) >---------------------------------------------------------

#define XELT_TYPE_TEXT      1
#define XELT_TYPE_CDATA     2
#define XELT_TYPE_NODE      3

XmlDoc_t tb_XmlDoc                  (char *xmltext);
String_t XDOC_to_xml                (XmlDoc_t Doc);
XmlElt_t XDOC_getRoot               (XmlDoc_t X);
retcode_t  XDOC_setRoot               (XmlDoc_t Doc, XmlElt_t newRoot);
XmlElt_t tb_XmlElt                  (int ElmType, XmlElt_t parent,
																		 char *name, char **attr);

Vector_t XELT_getChildren           (XmlElt_t X);
XmlElt_t XELT_getParent             (XmlElt_t X);
String_t XELT_getName               (XmlElt_t X);
int      XELT_getType               (XmlElt_t X);
String_t XELT_getText               (XmlElt_t X);
tb_Object_t  XELT_getAttribute         (XmlElt_t X, char *attributeName);
Hash_t   XELT_getAttributes         (XmlElt_t X);
XmlElt_t tb_XmlTextElt              (XmlElt_t parent, char *text);
XmlElt_t tb_XmlNodeElt              (XmlElt_t parent, char *name, Hash_t Attributes);
retcode_t  XELT_addAttributes         (XmlElt_t X, Hash_t newAttr);
int      XELT_setName               (XmlElt_t X, char *name);
int      XELT_setText               (XmlElt_t X, char *text);

//-------< Regex >-------------------------------------------------------

// for extensive infos look at pcre's own doc (man 7 pcre)
#define PCRE_CASELESS        0x0001 //< case insensitive
#define PCRE_MULTILINE       0x0002 //< pattern run over lines breaks (^/$ applies on every lines)
#define PCRE_DOTALL          0x0004 //< match even LF
#define PCRE_EXTENDED        0x0008 //< not used
#define PCRE_ANCHORED        0x0010 //< ^ match only start of buffer
#define PCRE_DOLLAR_ENDONLY  0x0020 //< $ match only end of buffer
#define PCRE_EXTRA           0x0040 //< unused
#define PCRE_NOTBOL          0x0080 //< unused
#define PCRE_NOTEOL          0x0100 //< unused
#define PCRE_UNGREEDY        0x0200 //< toggle greediness to off (default is greedy)

#define PCRE_NOBLANKS        0x1000 //< toolbox misc add : skip empty backrefs
#define PCRE_MATCHMULTI      0x2000 //< toolbox misc add : apply pattern more than once if possible
#define PCRE_MULTI           0x2000 //< toolbox misc add : igual

int     tb_getRegex         (Vector_t V,      // already created Vector to store substrings
														 String_t string, // subject string
														 char *regex,     // pattern
														 int options);    // See PCRE options

retcode_t tb_matchRegex       (String_t string, // subject string
														 char *regex,     // pattern
														 int options);    // See PCRE options

int     tb_Sed              (char *search,      // search pattern string
														 char *replace,     // replace pattern
														 String_t text,     // subject string
														 int options);      // See PCRE options

Hash_t tb_readConfig(char *file);
void      tb_trace            (int level, char *format, ...);
void tb_profile(char *format, ...);

#ifndef __C2MAN__



// ---------< Memory    >------------------------------------------
///*
// Initialise internal memory tracking
// if environment var TB_MEM_DEBUG is set to a valid file name, will
// try to open this file, and all further memory mess will be written in.
// Need TB_MEM_DEBUG to be defined in .c file to operate.
// if NDEBUG is also defined, nothing is done.

// Better used under TB_MEM_DEBUG define.
// don't mess with it unless you know what you're doin'.

void *xMalloc    (char*, char*, int, size_t);
void *xCalloc    (char*, char*, int, size_t, size_t);
void *xRealloc   (char*, char*, int, void*, size_t);
char *xStrdup    (char*, char*, int, char*);
void  xFree      (char*, char*, int, void*);


extern void * (*tb_xmalloc)  (size_t);
extern void * (*tb_xcalloc)  (size_t, size_t);
extern void * (*tb_xrealloc) (void *, size_t);
extern char * (*tb_xstrdup)  (char *);
extern void   (*tb_xfree)    (void *);


#if defined TB_MEM_DEBUG && (! defined NDEBUG)

#define tb_xmalloc(A)      xMalloc   (__FUNCTION__, __FILE__, __LINE__, A)
#define tb_xcalloc(A,B)    xCalloc   (__FUNCTION__, __FILE__, __LINE__, A, B)
#define tb_xrealloc(A,B)   xRealloc  (__FUNCTION__, __FILE__, __LINE__, A, B)
#define tb_xstrdup(A)      xStrdup   (__FUNCTION__, __FILE__, __LINE__, A)
#define tb_xfree(A)        xFree     (__FUNCTION__, __FILE__, __LINE__, A)


#ifndef __BUILD // when building toolbox, we de-activate this (to avoid preproc loops)

// wrappers for vanilla libc allocators
void *dbg_malloc  (char *func, char *file, int line, size_t size);
void *dbg_calloc  (char *func, char *file, int line, size_t n, size_t size);
void *dbg_realloc (char *func, char *file, int line, void *chunk, size_t size);
void *dbg_strdup  (char *func, char *file, int line, const void *m);
void  dbg_free    (char *func, char *file, int line, void *m);

#define free(A)      dbg_free(__FUNCTION__, __FILE__,__LINE__,A)
#define malloc(A)    dbg_malloc(__FUNCTION__, __FILE__,__LINE__,A)
#define calloc(A,B)  dbg_calloc(__FUNCTION__, __FILE__,__LINE__,A,B)
#define realloc(A,B) dbg_realloc(__FUNCTION__, __FILE__,__LINE__,A,B)
#define strdup(A)    dbg_strdup(__FUNCTION__, __FILE__,__LINE__,A)


void dbg_tb_free              (char *func, char *file, int line, void *v);
#define tb_Free(x)            dbg_tb_free(__FUNCTION__,__FILE__, __LINE__, x)

// logging constructors & factories to keep trace on objects creation
Pointer_t  dbg_tb_pointer     (char *func, char *file, int line, void *v, void *free_fnc);
#define tb_Pointer(x...)      dbg_tb_pointer (__FUNCTION__,__FILE__, __LINE__, x)
Socket_t   dbg_tb_socket      (char *func, char *file, int line, int mode, char *name, int port);
#define tb_Socket(x...)       dbg_tb_socket  (__FUNCTION__,__FILE__, __LINE__, x)
String_t   dbg_tb_string      (char *func, char *file, int line, char *fmt, ...);
#define tb_String(x...)       dbg_tb_string  (__FUNCTION__,__FILE__, __LINE__, x)
String_t   dbg_tb_nstring     (char *func, char *file, int line, int len, char *fmt, ...);
#define tb_nString(x...)      dbg_tb_nstring (__FUNCTION__,__FILE__, __LINE__, x)
Raw_t      dbg_tb_raw         (char *func, char *file, int line, int len, char *data);
#define tb_Raw(x...)          dbg_tb_raw     (__FUNCTION__,__FILE__, __LINE__, x)
Num_t      dbg_tb_num         (char *func, char *file, int line, int val);
#define tb_Num(x...)          dbg_tb_num     (__FUNCTION__,__FILE__, __LINE__, x)
Hash_t     dbg_tb_hash        (char *func, char *file, int line);
#define tb_Hash()             dbg_tb_hash    (__FUNCTION__,__FILE__, __LINE__)
Vector_t   dbg_tb_vector      (char *func, char *file, int line);
#define tb_Vector()           dbg_tb_vector  (__FUNCTION__,__FILE__, __LINE__)
Dict_t     dbg_tb_dict        (char *func, char *file, int line, int type, int dupes_allowed);
#define tb_Dict(x...)         dbg_tb_dict    (__FUNCTION__,__FILE__, __LINE__, x)
Iterator_t dbg_tb_iterator    (char *func, char *file, int line, Container_t Cn);
#define tb_Iterator(x...)     dbg_tb_iterator(__FUNCTION__,__FILE__, __LINE__, x)

XmlDoc_t   dbg_tb_xmldoc      (char *func, char *file, int line, char *xmltext);
#define tb_XmlDoc(x...)       dbg_tb_xmldoc  (__FUNCTION__,__FILE__, __LINE__, x)
XmlElt_t   dbg_tb_xmlelt      (char *func, char *file, int line,
															 int type, XmlElt_t parent, char *name, char **atts);
#define tb_XmlElt(x...)       dbg_tb_xmlelt  (__FUNCTION__,__FILE__, __LINE__, x)
XmlElt_t   dbg_tb_xmlnodeelt  (char *func, char *file, int line,
															 XmlElt_t parent, char *name, Hash_t Attr);
#define tb_XmlNodeElt(x...)   dbg_tb_xmlnodeelt  (__FUNCTION__,__FILE__, __LINE__, x)
XmlElt_t   dbg_tb_xmltextelt  (char *func, char *file, int line, XmlElt_t parent, char *text);
#define tb_XmlTextElt(x...)   dbg_tb_xmltextelt  (__FUNCTION__,__FILE__, __LINE__, x)
// logging factories
Vector_t   dbg_tb_strsplit    (char *func, char *file, int line,
															 char *str, char *regex, int options);
#define tb_StrSplit(x...)     dbg_tb_strsplit(__FUNCTION__,__FILE__, __LINE__, x)
int        dbg_tb_tokenize    (char *func, char *file, int line,
															 Vector_t V, char *s, char *delim, int flags);
#define tb_tokenize(x...)     dbg_tb_tokenize(__FUNCTION__,__FILE__, __LINE__, x)
String_t   dbg_tb_join        (char *func, char *file, int line, Vector_t V, char *delim);
#define tb_Join(x...)         dbg_tb_join    (__FUNCTION__,__FILE__, __LINE__, x)
String_t   dbg_tb_substr      (char *func, char *file, int line, String_t S, int start, int len);
#define tb_StrSub(x...)       dbg_tb_substr  (__FUNCTION__,__FILE__, __LINE__, x)
int        dbg_tb_getregex    (char *func, char *file, int line,
															 Vector_t  V, String_t  Str, char *regex,
															 int options);
#define tb_getRegex(x...)     dbg_tb_getregex(__FUNCTION__,__FILE__, __LINE__, x)
 

#endif  // __BUILD

#endif  // TB_MEM_DEBUG

void *fm_malloc        (size_t sz);
void *fm_realloc       (void *mem, size_t new_size);
void *fm_calloc        (size_t sz, size_t nb);
void fm_free           (void *mem);

void fm_dumpChunks     (void);
void fm_Dump           (void);



// read one line on a Socket_t (using timeout and retries values as set in Socket object)
int       tb_readSockLine (Socket_t O, String_t msg);

#endif //__C2MAN__




/* alpha */

typedef void (*worker_cb_t)(void *);
typedef struct job *job_t;;
typedef struct WPool *WPool_t;


enum job_mode {
	DETACHED=0,
	JOINABLE
};
typedef enum job_mode job_mode_t;


int tb_submitJob(WPool_t WP, job_t Job);
WPool_t tb_WPool(int start, int max, int min, int timeout);
job_t newJob(worker_cb_t work, void *data, job_mode_t mode);

#endif






















