#ifndef _SSTMAC_COMMON_SSTMAC_CONFIG_H
#define _SSTMAC_COMMON_SSTMAC_CONFIG_H 1
 
/* sstmac/common/sstmac_config.h. Generated automatically at end of configure. */
/* sstmac/common/config.h.  Generated from config.h.in by configure.  */
/* sstmac/common/config.h.in.  Generated from configure.ac by autoheader.  */

/* Track communcation synchronization stats */
#ifndef SSTMAC_COMM_DELAY_STATS
#define SSTMAC_COMM_DELAY_STATS 1
#endif

/* Track communcation synchronization stats */
/* #undef COMM_SYNC_STATS */

/* The include path for .ini configurations */
#ifndef SSTMAC_CONFIG_INSTALL_INCLUDE_PATH
#define SSTMAC_CONFIG_INSTALL_INCLUDE_PATH "/home/xianwei/local/sstmacro/include/sstmac/configurations"
#endif

/* The include path for .ini configurations */
#ifndef SSTMAC_CONFIG_SRC_INCLUDE_PATH
#define SSTMAC_CONFIG_SRC_INCLUDE_PATH "/home/xianwei/new_source/sst-macro/configurations"
#endif

/* Track communcation synchronization stats */
/* #undef CUSTOM_NEW */

/* Define to indicate default environment type (mpi/serial) */
#ifndef SSTMAC_DEFAULT_ENV_STRING
#define SSTMAC_DEFAULT_ENV_STRING "serial"
#endif

/* Define to indicate default event manager (event map/clock cycler) */
#ifndef SSTMAC_DEFAULT_EVENT_MANAGER_STRING
#define SSTMAC_DEFAULT_EVENT_MANAGER_STRING "map"
#endif

/* Define to indicate default partitioning strategy */
#ifndef SSTMAC_DEFAULT_PARTITION_STRING
#define SSTMAC_DEFAULT_PARTITION_STRING "serial"
#endif

/* Define to indicate default runtime type (mpi/serial) */
#ifndef SSTMAC_DEFAULT_RUNTIME_STRING
#define SSTMAC_DEFAULT_RUNTIME_STRING "serial"
#endif

/* Define to indicate distributed memory */
/* #undef DISTRIBUTED_MEMORY */

/* "Track/allow context switches for debugging purposes" */
/* #undef ENABLE_DEBUG_SWAP */

/* Contains -Werror flags */
/* #undef ERROR_CFLAGS */

/* Contains -Werror flags */
/* #undef ERROR_CXXFLAGS */

/* "Call graph utility is not available for use" */
#ifndef SSTMAC_HAVE_CALL_GRAPH
#define SSTMAC_HAVE_CALL_GRAPH 0
#endif

/* Define to use C++14 language features */
/* #undef HAVE_CXX14 */

/* Define to use C++17 language features */
/* #undef HAVE_CXX17 */

/* Define to 1 if you have the <dlfcn.h> header file. */
#ifndef SSTMAC_HAVE_DLFCN_H
#define SSTMAC_HAVE_DLFCN_H 1
#endif

/* Whether to compile compatibility with event calendars */
#ifndef SSTMAC_HAVE_EVENT_CALENDAR
#define SSTMAC_HAVE_EVENT_CALENDAR 0
#endif

/* Define to 1 if you have the <execinfo.h> header file. */
#ifndef SSTMAC_HAVE_EXECINFO_H
#define SSTMAC_HAVE_EXECINFO_H 1
#endif

/* Define to make pth available for threading */
/* #undef HAVE_GNU_PTH */

/* Define to 1 if you have the <inttypes.h> header file. */
#ifndef SSTMAC_HAVE_INTTYPES_H
#define SSTMAC_HAVE_INTTYPES_H 1
#endif

/* Define to 1 if you have the `dl' library (-ldl). */
#ifndef SSTMAC_HAVE_LIBDL
#define SSTMAC_HAVE_LIBDL 1
#endif

/* Define to 1 if you have the <memory.h> header file. */
#ifndef SSTMAC_HAVE_MEMORY_H
#define SSTMAC_HAVE_MEMORY_H 1
#endif

/* Define to 1 if you have the <mpi.h> header file. */
/* #undef HAVE_MPI_H */

/* Define to 1 if you have the `MPI_Init' function. */
/* #undef HAVE_MPI_INIT */

/* Define to make pthreads available for threading */
#ifndef SSTMAC_HAVE_PTHREAD
#define SSTMAC_HAVE_PTHREAD 1
#endif

/* Define if pthread_{,attr_}{g,s}etaffinity_np is supported. */
#ifndef SSTMAC_HAVE_PTHREAD_AFFINITY_NP
#define SSTMAC_HAVE_PTHREAD_AFFINITY_NP 1
#endif

/* Define to 1 if you have the <pth.h> header file. */
/* #undef HAVE_PTH_H */

/* Define to 1 if you have the <Python.h> header file. */
/* #undef HAVE_PYTHON_H */

/* Define to 1 if you have the <sst/core/component.h> header file. */
/* #undef HAVE_SST_CORE_COMPONENT_H */

/* Build with sst-elements */
/* #undef HAVE_SST_ELEMENTS */

/* Define to 1 if you have the
   <sst/elements/memHierarchy/memHierarchyInterface.h> header file. */
/* #undef HAVE_SST_ELEMENTS_MEMHIERARCHY_MEMHIERARCHYINTERFACE_H */

/* Define to 1 if you have the <stdint.h> header file. */
#ifndef SSTMAC_HAVE_STDINT_H
#define SSTMAC_HAVE_STDINT_H 1
#endif

/* Define to 1 if you have the <stdlib.h> header file. */
#ifndef SSTMAC_HAVE_STDLIB_H
#define SSTMAC_HAVE_STDLIB_H 1
#endif

/* Define to 1 if you have the <strings.h> header file. */
#ifndef SSTMAC_HAVE_STRINGS_H
#define SSTMAC_HAVE_STRINGS_H 1
#endif

/* Define to 1 if you have the <string.h> header file. */
#ifndef SSTMAC_HAVE_STRING_H
#define SSTMAC_HAVE_STRING_H 1
#endif

/* Define to 1 if you have the <sys/stat.h> header file. */
#ifndef SSTMAC_HAVE_SYS_STAT_H
#define SSTMAC_HAVE_SYS_STAT_H 1
#endif

/* Define to 1 if you have the <sys/types.h> header file. */
#ifndef SSTMAC_HAVE_SYS_TYPES_H
#define SSTMAC_HAVE_SYS_TYPES_H 1
#endif

/* Define to make ucontext available for threading */
#ifndef SSTMAC_HAVE_UCONTEXT
#define SSTMAC_HAVE_UCONTEXT 1
#endif

/* Define to 1 if you have the <unistd.h> header file. */
#ifndef SSTMAC_HAVE_UNISTD_H
#define SSTMAC_HAVE_UNISTD_H 1
#endif

/* "working MPI not found" */
#ifndef SSTMAC_HAVE_VALID_MPI
#define SSTMAC_HAVE_VALID_MPI 0
#endif

/* Run on integrated SST core */
#ifndef SSTMAC_INTEGRATED_SST_CORE
#define SSTMAC_INTEGRATED_SST_CORE 0
#endif

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#ifndef SSTMAC_LT_OBJDIR
#define SSTMAC_LT_OBJDIR ".libs/"
#endif

/* "the definition of the offsetof macro" */
/* #undef OFFSET_OF_MACRO */

/* Define OTF2 support as enabled */
/* #undef OTF2_ENABLED */

/* Name of package */
#ifndef SSTMAC_PACKAGE
#define SSTMAC_PACKAGE "sstmacro"
#endif

/* Define to the address where bug reports for this package should be sent. */
#ifndef SSTMAC_PACKAGE_BUGREPORT
#define SSTMAC_PACKAGE_BUGREPORT "sst-macro-help@sandia.gov"
#endif

/* Define to the full name of this package. */
#ifndef SSTMAC_PACKAGE_NAME
#define SSTMAC_PACKAGE_NAME "sstmacro"
#endif

/* Define to the full name and version of this package. */
#ifndef SSTMAC_PACKAGE_STRING
#define SSTMAC_PACKAGE_STRING "sstmacro 12.1.0"
#endif

/* Define to the one symbol short name of this package. */
#ifndef SSTMAC_PACKAGE_TARNAME
#define SSTMAC_PACKAGE_TARNAME "sstmacro"
#endif

/* Define to the home page for this package. */
#ifndef SSTMAC_PACKAGE_URL
#define SSTMAC_PACKAGE_URL ""
#endif

/* Define to the version of this package. */
#ifndef SSTMAC_PACKAGE_VERSION
#define SSTMAC_PACKAGE_VERSION "12.1.0"
#endif

/* "Build from a repo checkout" */
#ifndef SSTMAC_REPO_BUILD
#define SSTMAC_REPO_BUILD 1
#endif

/* Whether safe mode should be run with sanity checks */
#ifndef SSTMAC_SANITY_CHECK
#define SSTMAC_SANITY_CHECK 0
#endif

/* The size of `char', as computed by sizeof. */
#ifndef SSTMAC_SIZEOF_CHAR
#define SSTMAC_SIZEOF_CHAR 1
#endif

/* The size of `double', as computed by sizeof. */
#ifndef SSTMAC_SIZEOF_DOUBLE
#define SSTMAC_SIZEOF_DOUBLE 8
#endif

/* The size of `float', as computed by sizeof. */
#ifndef SSTMAC_SIZEOF_FLOAT
#define SSTMAC_SIZEOF_FLOAT 4
#endif

/* The size of `int', as computed by sizeof. */
#ifndef SSTMAC_SIZEOF_INT
#define SSTMAC_SIZEOF_INT 4
#endif

/* The size of `long', as computed by sizeof. */
#ifndef SSTMAC_SIZEOF_LONG
#define SSTMAC_SIZEOF_LONG 8
#endif

/* The size of `long double', as computed by sizeof. */
#ifndef SSTMAC_SIZEOF_LONG_DOUBLE
#define SSTMAC_SIZEOF_LONG_DOUBLE 16
#endif

/* The size of `long long', as computed by sizeof. */
#ifndef SSTMAC_SIZEOF_LONG_LONG
#define SSTMAC_SIZEOF_LONG_LONG 8
#endif

/* The size of `short', as computed by sizeof. */
#ifndef SSTMAC_SIZEOF_SHORT
#define SSTMAC_SIZEOF_SHORT 2
#endif

/* The size of `unsigned char', as computed by sizeof. */
#ifndef SSTMAC_SIZEOF_UNSIGNED_CHAR
#define SSTMAC_SIZEOF_UNSIGNED_CHAR 1
#endif

/* The size of `unsigned int', as computed by sizeof. */
#ifndef SSTMAC_SIZEOF_UNSIGNED_INT
#define SSTMAC_SIZEOF_UNSIGNED_INT 4
#endif

/* The size of `unsigned long', as computed by sizeof. */
#ifndef SSTMAC_SIZEOF_UNSIGNED_LONG
#define SSTMAC_SIZEOF_UNSIGNED_LONG 8
#endif

/* The size of `unsigned long long', as computed by sizeof. */
#ifndef SSTMAC_SIZEOF_UNSIGNED_LONG_LONG
#define SSTMAC_SIZEOF_UNSIGNED_LONG_LONG 8
#endif

/* The size of `unsigned short', as computed by sizeof. */
#ifndef SSTMAC_SIZEOF_UNSIGNED_SHORT
#define SSTMAC_SIZEOF_UNSIGNED_SHORT 2
#endif

/* The size of `void *', as computed by sizeof. */
#ifndef SSTMAC_SIZEOF_VOID_P
#define SSTMAC_SIZEOF_VOID_P 8
#endif

/* Define to 1 if you have the ANSI C header files. */
#ifndef SSTMAC_STDC_HEADERS
#define SSTMAC_STDC_HEADERS 1
#endif

/* Major version number */
#ifndef SSTMAC_SUBSUBVERSION
#define SSTMAC_SUBSUBVERSION 0
#endif

/* Major version number */
#ifndef SSTMAC_SUBVERSION
#define SSTMAC_SUBVERSION 1
#endif

/* "Whether to enable strict CPU affinity" */
#ifndef SSTMAC_USE_CPU_AFFINITY
#define SSTMAC_USE_CPU_AFFINITY 0
#endif

/* "Whether to enable multithreading" */
/* #undef USE_MULTITHREAD */

/* "Whether to use spin locks for more efficient thread barriers" */
/* #undef USE_SPINLOCK */

/* Version number of package */
#ifndef SSTMAC_VERSION
#define SSTMAC_VERSION "12.1.0"
#endif

/* VTK is enabled and installed */
/* #undef VTK_ENABLED */

/* Contains -Werror flags */
/* #undef WARNING_CFLAGS */

/* Contains -Werror flags */
/* #undef WARNING_CXXFLAGS */
 
/* once: _SSTMAC_COMMON_SSTMAC_CONFIG_H */
#endif
