//======================================================
// $Id: classRegistry.c,v 1.1 2004/05/12 22:04:51 plg Exp $
//======================================================
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


/**
 * @defgroup Class Toolbox Classes internals
 * gory details, useful for extending Toolbox base classes
 *
 * Every Toolbox class, including root class Object_t, get an internal identifier known as Cid, class id. This identifier is either staticaly defined (for Toolbox mandatory base classes, see Toolbox.h for macro constant definitions) or dynamically for user defined classes. Cid is a key in a complex structure known as class registry. This structure is global and hidden from userland action. 
 *
 * Every class method is also registered in a global and hidden 'method registry' and is given an unique identifier.
 * In class registry, every class identifier is associated with an array of function pointers, known as vtable. Vtables are indexed by methods identifiers. When a class method is called on an object (some class instance), the appropriate function pointer is located in the class vtable, or if not defined, in any of its ancestors's vtable.
 * 
 * For a better flexibility, a Class can also registers self as 'implementing' an interface (ie: a set of virtual methods not related to a special class), and by this it's vtable is extended by the interface's methods. Note that subclasses of a class implementing an interface inherits automatically this interface.
 *
 * example:
 * \code 
 * #include "tb_ClassBuilder.h"  // class building tools
 *
 * int MY_CLASS; // global holder of new class identifier
 * int COOL_METHOD; // global holder for a method identifier
 *
 * pthread_once_t __init_my_class_once = PTHREAD_ONCE_INIT; // to be sure to run this once at most
 *
 * void init_my_class_once() {
 *	pthread_once(&__class_registry_init_once, tb_classRegisterInit); // make sure base classes are ready (mandatory)
 *	MY_CLASS = tb_registerNewClass("MY_CLASS_T", TB_COMPOSITE, setup_my_class_once);
 * }
 *
 * void setup_my_class_once(int OID) {
 *   tb_registerMethod(OID, OM_NEW,         my_class_new);     // mandatory constructor
 *	 tb_registerMethod(OID, OM_FREE,        my_class_free);    // mandatory destructor
 *	 tb_registerMethod(OID, OM_CLONE,       my_class_clone);   // optional copy constructor
 *	 tb_registerMethod(OID, OM_DUMP,        my_class_dump);    // optional debug dumper
 *
 *   // to extend the new class with an interface :
 *   tb_implementsInterface(OID, "Iterable", 
 *                          &__iterable_build_once, build_iterable_once);
 *   // now assign some of th interface methods :
 *	 tb_registerMethod(OID, OM_GONEXT       my_class_next); // 
 *   // add specific class methods :
 *   COOL_METHOD = tb_registerNew_ClassMethod("my_cool_method", OID) 
 *   // and assign an implementation
 *	 tb_registerMethod(OID, COOL_METHOD,    my_class_cool_method); // whatever
 * }
 *
 * \endcode
 *
 *
 * See new_class.pl for an easy way to create new classes
 *
 * @see Object for base class virtual methods
 */


#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "Toolbox.h"
#include "classRegistry.h"
#include "tb_ClassBuilder.h"
#include "Ktypes.h"

pthread_once_t __class_registry_init_once = PTHREAD_ONCE_INIT;   


// Fixme: isDocked should be there !!
// Fixme: isAliased should be there !!

inline static int          __class_idOf          (char *name);
inline static vtable_t     __class_methods_of    (int Cid);
inline static vtable_t     __iface_methods_of    (int Cid, int Iid);
inline static ifaceReg_t   __iface_of            (int Iid);


// globos 
classRegister_t       __classRegister;
ifacesRegister_t      __ifacesRegister;
methodsRegister_t     __methodsRegister;

extern void __build_object_class_once(int OID);




/** 
 * Create a new class identifier
 * \ingroup Class
 *
 * Add a new identifier in global class registry. 
 *
 * A new vtable is allocated in classes registry, and is linked to it parent's one. Parent's interfaces vtables are also generated. Then the init callback is run, to assign class implementation specific methods to virtual slots (see registerMethod)
 *
 * @param name class name (should be unique)
 * @param parent parent's class id
 * @param init class builder function, called only once
 *
 * returns new class identifier or TB_ERR
 *
 * @see Object
*/

int tb_registerNewClass(char *name, int parent, void (*init)(int Cid)) {

	if( __class_idOf(name) == -1) {
		if( parent == -1 || (parent >-1 && parent < __classRegister->classes_nb)) {
			classReg_t Cr = (classReg_t)tb_xcalloc(1, sizeof(struct classReg));
			int i;
			Cr->Cid = __classRegister->classes_nb;
			Cr->name = tb_xstrdup(name); 
			Cr->class_methods  = tb_xcalloc(1, sizeof(struct vtable));
			Cr->ifaces_methods = tb_xcalloc(1, sizeof(struct ifaces));
			if( parent != -1) {
				// class inherits parent's methods
				Cr->class_methods->nb = __classRegister->Register[parent]->class_methods->nb;
				Cr->class_methods->methods = tb_xcalloc(1, sizeof(void *)*Cr->class_methods->nb);
				// class inherits parent's interfaces methods also !!!
				Cr->ifaces_methods->nb = __classRegister->Register[parent]->ifaces_methods->nb;
				if( Cr->ifaces_methods->nb >0) {
					Cr->ifaces_methods->interfaces = tb_xcalloc(1, sizeof(iface_t)*Cr->ifaces_methods->nb);
					for(i=0; i< Cr->ifaces_methods->nb; i++) {
						Cr->ifaces_methods->interfaces[i] =  tb_xcalloc(1, sizeof(struct iface));
						Cr->ifaces_methods->interfaces[i]->Iid = 
							__classRegister->Register[parent]->ifaces_methods->interfaces[i]->Iid;
						Cr->ifaces_methods->interfaces[i]->vtable = tb_xcalloc(1, sizeof(struct vtable));
						Cr->ifaces_methods->interfaces[i]->vtable->nb = 
							__classRegister->Register[parent]->ifaces_methods->interfaces[i]->vtable->nb;
						Cr->ifaces_methods->interfaces[i]->vtable->methods = 
							tb_xcalloc(1, sizeof(void *)*Cr->ifaces_methods->interfaces[i]->vtable->nb);
					}
				}
			}


			Cr->parent_Cid = parent;

			__classRegister->classes_nb++;
			__classRegister->Register = tb_xrealloc(__classRegister->Register, 
																					 sizeof(classReg_t)* __classRegister->classes_nb);
			__classRegister->Register[Cr->Cid] = Cr;

			tb_debug("tb_registerNewClass<%s::%s:%d>\n", __class_name_of(__parent_of(Cr->Cid)), name, Cr->Cid);
			init(Cr->Cid);

			return Cr->Cid;
		} else {
			tb_error("tb_registerNewClass<%s>: unkown parent %d\n", name, parent);
			return TB_ERR;
		}
	}
	tb_error("tb_registerNewClass<%s>: already registered\n", name);

	return TB_ERR;
}


/** 
 * Create a new interface identifier
 * \ingroup Class
 *
 * Add a new identifier in global interface registry. 
 *
 * A new vtable is allocated in interface registry. Much similar to class registering, but interface is a standalone set of virtual methods, and doesn't have any parent's nor children.
 *
 * @param name interface name (should be unique)
 *
 * returns new interface identifier or TB_ERR
 *
 * @see Object
*/

int tb_registerNewInterface(char *name) {
	if(__interface_id_of(name) == -1) {
		ifaceReg_t Ir = (ifaceReg_t)tb_xcalloc(1, sizeof(struct ifaceReg));
		Ir->Iid = __ifacesRegister->interfaces_nb;
		Ir->name = tb_xstrdup(name); 
		__ifacesRegister->interfaces_nb++;
		__ifacesRegister->Register = tb_xrealloc(__ifacesRegister->Register, 
																				 sizeof(ifaceReg_t)* __ifacesRegister->interfaces_nb);
		__ifacesRegister->Register[Ir->Iid] = Ir;

		tb_debug("tb_registerNewInterface<%s:%d>\n", name, Ir->Iid);
		
		return Ir->Iid;
	}
	tb_error("tb_registerNewInterface<%s>: already registered\n", name);
	return TB_ERR;
}

/** 
 * Create a new interface method identifier
 * \ingroup Class
 *
 * Add a new identifier in global method registry. 
 *
 * An interface method is a virtual function belongind to a specific interface. To be used upon an object, the object class (or any of his ancestors) must have been registered as 'implementing' this interface.
 *
 * @param name method name (should be unique in interface scope)
 * @param Iid owner's interface identifier
 *
 * returns new method identifier or TB_ERR
 *
 * @see Object
*/

int tb_registerNew_InterfaceMethod(char *name, int Iid) {
	ifaceReg_t iR = __iface_of(Iid);
	if( iR != NULL) {
		if(__method_id_of(name) == -1) { // fixme: check if no interfacename::name exists
			int last = __methodsRegister->methods_nb;
			methodReg_t Mr = (methodReg_t)tb_xcalloc(1, sizeof(struct methodReg));

			Mr->Mid           = last;
			Mr->Moffset       = iR->methods_nb++;

			Mr->method_type   = MT_INTERFACE;
			Mr->ownerId       = Iid;
			Mr->name          = tb_xstrdup(name);
			__methodsRegister->methods_nb++;
			__methodsRegister->Register = 
				tb_xrealloc(__methodsRegister->Register, 
								 sizeof(methodReg_t)*__methodsRegister->methods_nb); 
			__methodsRegister->Register[last] = Mr;	

			tb_debug("tb_registerNew_InterfaceMethod<%s::%s:%d>\n", __iface_name_of(Iid), name, last);
			return last;
		}
		tb_error("tb_registerNew_InterfaceMethod<%s>: already registered\n", name);
	}
	return TB_ERR;
}


/** 
 * Create a new class method identifier
 * \ingroup Class
 *
 * Add a new identifier in global method registry. 
 *
 * A class method is a virtual function belongind to a specific class. To be used upon an object, the object class (or any of his ancestors) must have registered this method.
 *
 * @param name method name (should be unique in interface scope)
 * @param Cid owner's class identifier
 *
 * returns new method identifier or TB_ERR
 *
 * @see Object
*/

int tb_registerNew_ClassMethod(char *name, int Cid) {
	if(__method_id_of(name) == -1) {
		int last = __methodsRegister->methods_nb;
		methodReg_t  Mr = (methodReg_t)tb_xcalloc(1, sizeof(struct methodReg));
		vtable_t     vT = __class_methods_of(Cid);
		Mr->Mid           = last;
		Mr->Moffset       = vT->nb++;
		// prepare storage
		vT->methods = tb_xrealloc(vT->methods, sizeof(void *)*vT->nb);
		// vT->methods[Mr->Moffset] = default_undefined_error;
		Mr->method_type   = MT_CLASS;
		Mr->ownerId       = Cid;
		Mr->name          = tb_xstrdup(name);
		__methodsRegister->methods_nb++;
		__methodsRegister->Register = 
			tb_xrealloc(__methodsRegister->Register, 
							 sizeof(methodReg_t)*__methodsRegister->methods_nb); 
		__methodsRegister->Register[last] = Mr;	

		tb_debug("registerNew_ClassMethod<%s::%s:%d>\n", __class_name_of(Cid), name, last);
		return last;
	}
	tb_error("registerNew_ClassMethod<%s>: already registered\n", name);

	return TB_ERR;
}


/** 
 * Assign a specific implementation to a class vtable method
 * \ingroup Class
 *
 * Assign a user function to a virtual method
 *
 * Binds method 'Mid' of class 'Cid' to function 'fnc'
 *
 * @param Cid owner's class identifier
 * @param Mid method identifier
 * @param fnc user function
 *
 *
 * @see Object
*/

void tb_registerMethod(int Cid, int Mid, void *fnc) {
	methodReg_t mR;
	
	if( Cid >-1 && Cid < __classRegister->classes_nb) {
		if( Mid >-1 &&  Mid < __methodsRegister->methods_nb) {
			mR = __methodsRegister->Register[Mid];
			
			tb_debug("registerMethod: <%s::%s> mR->mt = %d\n", 
							__class_name_of(Cid),
							mR->name,
							mR->method_type);
							

			switch( mR->method_type ) {
			case MT_CLASS:
				if(__VALID_CLASS(Cid, mR->ownerId)) {
					((void **)__class_methods_of(Cid)->methods)[mR->Moffset] = fnc;
				} else {
					tb_error("registerMethod: method <%s::%s> doesn't belongs to class <%s>\n", 
									 __method_name_of(mR->ownerId), 
									 mR->name, 
									 __class_name_of(Cid));
					abort();
				}
				break;
			case MT_INTERFACE:
				if(__VALID_IFACE(Cid, mR->ownerId)) {
					((void **)__iface_methods_of(Cid, mR->ownerId)->methods)[mR->Moffset] = fnc;
				} else {
					tb_error("registerMethod: interface <%s> not implemented by class <%s>\n", 
									 __iface_name_of(mR->ownerId), 
									 mR->name, 
									 __class_name_of(Cid));
					abort();
				}
				break;
			default:
				tb_error("bad");
				abort();
			}
		} else {
			tb_error("not registered method <%d>\n", Mid);
			abort();
		} 
	} else {
		tb_error("not registered class <%d>\n", Cid);
		abort();
	} 
}
	

/** 
 * Extends a class virtual methods table with an interface definition
 * \ingroup Class
 *
 *
 * @param Cid owner's class identifier
 * @param name interface name
 * @param interface_po pthread_once_t pointer, used to grant that requested interface has been registered, and only once
 * @param interface_builder builder function for the interface (registerNewInterface, registerNew_InterfaceMethod)
 *
 *
 * @see Object
*/
void tb_implementsInterface(int Cid, 
												 char *name, 
												 pthread_once_t *interface_po, 
												 void (*interface_builder)(void)) {
	if( Cid >-1 && Cid < __classRegister->classes_nb) {
		int Iid;

		pthread_once(interface_po, interface_builder);

		Iid = __interface_id_of(name);
		if(! __VALID_IFACE(Cid, Iid)) {
			ifaces_t iFs = __classRegister->Register[Cid]->ifaces_methods;
			iface_t  iF;
			iFs->nb++;
			iFs->interfaces = tb_xrealloc(iFs->interfaces, sizeof(iface_t)* iFs->nb);
			iF = tb_xcalloc(1, sizeof(struct iface));
			iFs->interfaces[iFs->nb-1] = iF;
			iF->vtable = tb_xcalloc(1, sizeof(struct vtable));
			iF->vtable->nb = __ifacesRegister->Register[Iid]->methods_nb;
			iF->vtable->methods = tb_xcalloc(1, sizeof(void *)*iF->vtable->nb);
			iF->Iid = Iid;
			return;
		} else {
			tb_error("implementsInterface: %s already implements %s (may be inherited)\n", 
							 __class_name_of(Cid),
							 name);
			abort();
		}
	}
	tb_error("implementsInterface: unkown class id %d\n", Cid); 
	abort();
}


void tb_classRegisterInit() {
	__classRegister     = (classRegister_t)     tb_xcalloc(1, sizeof(struct classRegister));
	__methodsRegister   = (methodsRegister_t)   tb_xcalloc(1, sizeof(struct methodsRegister));
	__ifacesRegister    = (ifacesRegister_t)    tb_xcalloc(1, sizeof(struct ifacesRegister));

	// setup basic key types for keyed containers
	register_basic_ktypes_once();

	// !!! don't change registering order !!!!

	tb_registerNewClass("TB_OBJECT",    -1,            __build_object_once);

	tb_registerNewClass("TB_SCALAR",    TB_OBJECT,     __build_scalar_once);
	tb_registerNewClass("TB_CONTAINER", TB_OBJECT,     __build_container_once);
	tb_registerNewClass("TB_COMPOSITE", TB_OBJECT,     __build_composite_once);


	tb_registerNewClass("TB_STRING",    TB_SCALAR,     __build_string_once);
	tb_registerNewClass("TB_RAW",       TB_SCALAR,     __build_raw_once);
	tb_registerNewClass("TB_NUM",       TB_SCALAR,     __build_num_once);
	tb_registerNewClass("TB_POINTER",   TB_SCALAR,     __build_pointer_once);

	tb_registerNewClass("TB_HASH",      TB_CONTAINER,  __build_hash_once);
	tb_registerNewClass("TB_VECTOR",    TB_CONTAINER,  __build_vector_once);
	tb_registerNewClass("TB_DICT",      TB_CONTAINER,  __build_dict_once);

	tb_registerNewClass("TB_SOCKET",    TB_COMPOSITE,  __build_socket_once);
	tb_registerNewClass("TB_ITERATOR",  TB_COMPOSITE,  __build_iterator_once);
	tb_registerNewClass("TB_XMLDOC",    TB_COMPOSITE,  __build_xmldoc_once);
	tb_registerNewClass("TB_XMLELT",    TB_COMPOSITE,  __build_xmlelt_once);
}



inline static ifaceReg_t __iface_of(int Iid) {
	if( Iid >=0 && Iid <__ifacesRegister->interfaces_nb) {
		return __ifacesRegister->Register[Iid];
	}
	tb_error("no such interface %d \n", Iid);
	return NULL;
}



retcode_t __VALID_CLASS(int Oid, int Cid) {
	if( Oid == Cid) return TB_OK;
	while(Oid >=0 && Oid < __classRegister->classes_nb) {
/* 		tb_debug("__VALID_CLASS:  checking if oid %d/%s is == %d/%s\n",  */
/* 						 Oid, __class_name_of(Oid), */
/* 						 Cid, __class_name_of(Cid)); */
		if( Oid == Cid ) return TB_OK;
		Oid = __classRegister->Register[Oid]->parent_Cid;
	}
	return TB_KO;
}

retcode_t __VALID_IFACE(int Oid, int Iid) {
	int i;
	while(Oid >=0 && Oid < __classRegister->classes_nb) {
/* 		tb_debug("__VALID_IFACE:  checking if oid %d/%s implements iface %d/%s\n",  */
/* 						 Oid, __class_name_of(Oid), */
/* 						 Iid, __iface_name_of(Iid)); */
		for(i=0; i< __classRegister->Register[Oid]->ifaces_methods->nb; i++) {
			if( __classRegister->Register[Oid]->ifaces_methods->interfaces[i]->Iid == Iid) {
				return TB_OK;
			}
		}
		Oid = __classRegister->Register[Oid]->parent_Cid;
	}
	return TB_KO;
}




inline const char * __class_name_of(int Oid) {
	if(Oid >=0 && Oid < __classRegister->classes_nb) {
		return __classRegister->Register[Oid]->name;
	}
	return "UNKNOWN";
}

inline const char * __iface_name_of(int Iid) {
	if(Iid >=0 && Iid < __ifacesRegister->interfaces_nb) {
		return __ifacesRegister->Register[Iid]->name;
	}
	return "UNKNOWN";
}

inline const char * __method_name_of(int Mid) {
	if(Mid >=0 && Mid < __methodsRegister->methods_nb) {
		return __methodsRegister->Register[Mid]->name;
	}
	return "UNKNOWN";
}


inline static int __class_idOf(char *name) {
	int i;
	for(i=0; i<__classRegister->classes_nb; i++) {
		if(strcmp(name, __classRegister->Register[i]->name) == 0) return i;
	}
	return -1;
}

inline int __interface_id_of(char *name) {
	int i;
	for(i=0; i<__ifacesRegister->interfaces_nb; i++) {
		if(strcmp(name, __ifacesRegister->Register[i]->name) == 0) return i;
	}
	return -1;
}



inline static vtable_t __class_methods_of(int Cid) {
	if(Cid >=0 && Cid < __classRegister->classes_nb) {
		return __classRegister->Register[Cid]->class_methods;
	}
	return NULL;
}

inline static vtable_t __iface_methods_of(int Cid, int Iid) {
	int i;
	if(Cid >=0 && Cid < __classRegister->classes_nb) {
		if(Iid >=0 && Iid < __ifacesRegister->interfaces_nb) {
			for(i=0; i< __classRegister->Register[Cid]->ifaces_methods->nb; i++) {
				if( __classRegister->Register[Cid]->ifaces_methods->interfaces[i]->Iid == Iid) {
					return __classRegister->Register[Cid]->ifaces_methods->interfaces[i]->vtable;
				}
			}
		}
	}
	return NULL;
}


inline int __method_id_of(char *name) {
	int i;
	for(i=0; i<__methodsRegister->methods_nb; i++) {
		if(strcmp(name, __methodsRegister->Register[i]->name) == 0) return i;
	}
	return -1;
}


inline int __parent_of(int Oid) {
	if(Oid >=0 && Oid < __classRegister->classes_nb) {
		return __classRegister->Register[Oid]->parent_Cid;
	}
	return -1;
}

void *__getMethod(int Cid, int Mid) {
	void *fnc = NULL;
	vtable_t vtable;
	if( Mid >=0 && Mid < __methodsRegister->methods_nb) {
		methodReg_t mR = __methodsRegister->Register[Mid];
		switch( mR->method_type ) {
		case MT_CLASS:
			if(__VALID_CLASS(Cid, mR->ownerId)) {
				do {
					if((vtable = __class_methods_of(Cid))) {
						if(vtable->nb < mR->Moffset) continue;
						if((fnc = ((void **)vtable->methods)[mR->Moffset])) break;
					}
				} while((Cid = __parent_of(Cid)) >=0);
			} else {
				tb_debug("__getMethod(%s::%s) : no such method\n", 
								 __class_name_of(Cid), __method_name_of(Mid));
			}
			break;
		case MT_INTERFACE:
			if(__VALID_IFACE(Cid, mR->ownerId)) {
				do {
					if((vtable = __iface_methods_of(Cid, mR->ownerId))) {
						if(vtable->nb < mR->Moffset) continue;
						if((fnc = ((void **)vtable->methods)[mR->Moffset])) break;
					}
				} while((Cid = __parent_of(Cid)) >=0);
			} else {
				tb_debug("__getMethod(%s::%s) : no such interface\n", 
								 __class_name_of(Cid), __method_name_of(Mid));
			}
			break;
		default:
			break;
		}
		tb_debug("__getMethod(%s::%s) : found method %x\n", 
						 __class_name_of(Cid), __method_name_of(Mid), fnc);
	}
	if(! fnc) {
		tb_debug("__getMethod(%s::%s) : no such method\n", 
						 __class_name_of(Cid), __method_name_of(Mid));
	}
	return fnc;
}

void *__getParentMethod(int Cid, int Mid) {
	tb_debug("getParentMethod[%s]: parent of %s is %s\n", 
					__method_name_of(Mid),
					__class_name_of(Cid),
					__class_name_of(__parent_of(Cid)));					
	return __getMethod(__parent_of(Cid), Mid);
}









