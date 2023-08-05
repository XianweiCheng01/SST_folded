/* sstmac/common/config.h.  Generated from config.h.in by configure.  */
/* sstmac/common/config.h.in.  Generated from configure.ac by autoheader.  */

/* Track communcation synchronization stats */
#define COMM_DELAY_STATS 1

/* Track communcation synchronization stats */
/* #undef COMM_SYNC_STATS */

/* The include path for .ini configurations */
#define CONFIG_INSTALL_INCLUDE_PATH "/home/xianwei/local/sstmacro/include/sstmac/configurations"

/* The include path for .ini configurations */
#define CONFIG_SRC_INCLUDE_PATH "/home/xianwei/new_source/sst-macro/configurations"

/* Track communcation synchronization stats */
/* #undef CUSTOM_NEW */

/* Define to indicate default environment type (mpi/serial) */
#define DEFAULT_ENV_STRING "serial"

/* Define to indicate default event manager (event map/clock cycler) */
#define DEFAULT_EVENT_MANAGER_STRING "map"

/* Define to indicate default partitioning strategy */
#define DEFAULT_PARTITION_STRING "serial"

/* Define to indicate default runtime type (mpi/serial) */
#define DEFAULT_RUNTIME_STRING "serial"

/* Define to indicate distributed memory */
/* #undef DISTRIBUTED_MEMORY */

/* "Track/allow context switches for debugging purposes" */
/* #undef ENABLE_DEBUG_SWAP */

/* Contains -Werror flags */
/* #undef ERROR_CFLAGS */

/* Contains -Werror flags */
/* #undef ERROR_CXXFLAGS */

/* "Call graph utility is not available for use" */
#define HAVE_CALL_GRAPH 0

/* Define to use C++14 language features */
/* #undef HAVE_CXX14 */

/* Define to use C++17 language features */
/* #undef HAVE_CXX17 */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Whether to compile compatibility with event calendars */
#define HAVE_EVENT_CALENDAR 0

/* Define to 1 if you have the <execinfo.h> header file. */
#define HAVE_EXECINFO_H 1

/* Define to make pth available for threading */
/* #undef HAVE_GNU_PTH */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `dl' library (-ldl). */
#define HAVE_LIBDL 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <mpi.h> header file. */
/* #undef HAVE_MPI_H */

/* Define to 1 if you have the `MPI_Init' function. */
/* #undef HAVE_MPI_INIT */

/* Define to make pthreads available for threading */
#define HAVE_PTHREAD 1

/* Define if pthread_{,attr_}{g,s}etaffinity_np is supported. */
#define HAVE_PTHREAD_AFFINITY_NP 1

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
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to make ucontext available for threading */
#define HAVE_UCONTEXT 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* "working MPI not found" */
#define HAVE_VALID_MPI 0

/* Run on integrated SST core */
#define INTEGRATED_SST_CORE 0

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* "the definition of the offsetof macro" */
/* #undef OFFSET_OF_MACRO */

/* Define OTF2 support as enabled */
/* #undef OTF2_ENABLED */

/* Name of package */
#define PACKAGE "sstmacro"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "sst-macro-help@sandia.gov"

/* Define to the full name of this package. */
#define PACKAGE_NAME "sstmacro"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "sstmacro 12.1.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "sstmacro"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "12.1.0"

/* "Build from a repo checkout" */
#define REPO_BUILD 1

/* Whether safe mode should be run with sanity checks */
#define SANITY_CHECK 0

/* The size of `char', as computed by sizeof. */
#define SIZEOF_CHAR 1

/* The size of `double', as computed by sizeof. */
#define SIZEOF_DOUBLE 8

/* The size of `float', as computed by sizeof. */
#define SIZEOF_FLOAT 4

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 8

/* The size of `long double', as computed by sizeof. */
#define SIZEOF_LONG_DOUBLE 16

/* The size of `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 8

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* The size of `unsigned char', as computed by sizeof. */
#define SIZEOF_UNSIGNED_CHAR 1

/* The size of `unsigned int', as computed by sizeof. */
#define SIZEOF_UNSIGNED_INT 4

/* The size of `unsigned long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG 8

/* The size of `unsigned long long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG_LONG 8

/* The size of `unsigned short', as computed by sizeof. */
#define SIZEOF_UNSIGNED_SHORT 2

/* The size of `void *', as computed by sizeof. */
#define SIZEOF_VOID_P 8

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Major version number */
#define SUBSUBVERSION 0

/* Major version number */
#define SUBVERSION 1

/* "Whether to enable strict CPU affinity" */
#define USE_CPU_AFFINITY 0

/* "Whether to enable multithreading" */
/* #undef USE_MULTITHREAD */

/* "Whether to use spin locks for more efficient thread barriers" */
/* #undef USE_SPINLOCK */

/* Version number of package */
#define VERSION "12.1.0"

/* VTK is enabled and installed */
/* #undef VTK_ENABLED */

/* Contains -Werror flags */
/* #undef WARNING_CFLAGS */

/* Contains -Werror flags */
/* #undef WARNING_CXXFLAGS */
