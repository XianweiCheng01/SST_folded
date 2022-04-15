
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <mpi.h>
#include <pthread.h>

#include <stdint.h>
#include <inttypes.h>
#include <stddef.h>
#include <limits.h>
#include <assert.h>

//////////////////////////////
//void func_barrier();
//////////////////////////////

//////////aml.c/////////
//#define SOATTR __attribute__((visibility("default")))
#define SENDSOURCE(node) ( sendbuf+(AGGR*nbuf[node]))
#define SENDSOURCE_intra(node) ( sendbuf_intra+(AGGR_intra*nbuf_intra[node]) )
#define ushort unsigned short

struct __attribute__((__packed__)) hdr { //header of internode message
	ushort sz;
	char hndl;
	char routing;
};

struct __attribute__((__packed__)) hdri { //header of internode message
	ushort routing;
	ushort sz;
	char hndl;
};

#ifndef PROCS_PER_NODE_NOT_POWER_OF_TWO
//int loggroup,groupmask;
#define PROC_FROM_GROUPLOCAL(g,l) ((l)+((g)<<loggroup))
#define GROUP_FROM_PROC(p) ((p) >> loggroup)
#define LOCAL_FROM_PROC(p) ((p) & groupmask)
#else
#define PROC_FROM_GROUPLOCAL(g,l) ((g)*group_size+(l))
#define GROUP_FROM_PROC(p) ((p)/group_size)
#define LOCAL_FROM_PROC(p) ((p)%group_size)
#endif
/**/
//////////aml.c/////////

//////////user_settings.h/////////
#define GENERATOR_USE_PACKED_EDGE_TYPE

#define FAST_64BIT_ARITHMETIC
//////////user_settings.h/////////

//////////graph_generator.h/////////

#ifdef GENERATOR_USE_PACKED_EDGE_TYPE
typedef struct packed_edge {
  uint32_t v0_low;
  uint32_t v1_low;
  uint32_t high; /* v1 in high half, v0 in low half */
} packed_edge;
#else
typedef struct packed_edge {
  int64_t v0;
  int64_t v1;
} packed_edge;
#endif

//////////graph_generator.h/////////

//////////graph_generator.c/////////

//////////graph_generator.c/////////

/////////splittable_mrg.h////////

typedef struct mrg_state {
  uint_fast32_t z1, z2, z3, z4, z5;
} mrg_state;

/////////splittable_mrg.h////////

/////////splittable_mrg.c////////

typedef struct mrg_transition_matrix {
  uint_fast32_t s, t, u, v, w;
  /* Cache for other parts of matrix (see mrg_update_cache function)     */
  uint_fast32_t a, b, c, d;
} mrg_transition_matrix;

/////////splittable_mrg.c////////

//////////common.h/////////

#ifndef PROCS_PER_NODE_NOT_POWER_OF_TWO
#define SIZE_MUST_BE_A_POWER_OF_TWO
#endif

#ifdef SIZE_MUST_BE_A_POWER_OF_TWO
#define MOD_SIZE(v) ((v) & ((1 << lgsize) - 1))
#define DIV_SIZE(v) ((v) >> lgsize)
#define MUL_SIZE(x) ((x) << lgsize)
#else
#define MOD_SIZE(v) ((v) % size)
#define DIV_SIZE(v) ((v) / size)
#define MUL_SIZE(x) ((x) * size)
#endif

#define VERTEX_OWNER(v) ((int)(MOD_SIZE(v)))
#define VERTEX_LOCAL(v) ((size_t)(DIV_SIZE(v)))
#define VERTEX_TO_GLOBAL(r, i) ((int64_t)(MUL_SIZE((uint64_t)((i))) + (int)((r))))

typedef struct tuple_graph {
	int data_in_file; /* 1 for file, 0 for memory */
	int write_file; /* 1 if the file needs written, 0 if re-used and read */
	packed_edge* restrict edgememory; /* NULL if edges are in file */
	int64_t edgememory_size;
	int64_t max_edgememory_size;
	MPI_File edgefile; /* Or MPI_FILE_NULL if edges are in memory */
	int64_t nglobaledges; /* Number of edges in graph, in both cases */
#ifdef SSSP
	float* restrict weightmemory;
	MPI_File weightfile;
#endif
} tuple_graph;

#define ITERATE_TUPLE_GRAPH_BLOCK_COUNT(tg) \
	((tg)->data_in_file ? \
	 (DIV_SIZE((MPI_Offset)(((tg)->nglobaledges + FILE_CHUNKSIZE - 1) / FILE_CHUNKSIZE) \
		   + size - 1)) : \
	 (((tg)->max_edgememory_size + FILE_CHUNKSIZE - 1) / FILE_CHUNKSIZE))
#ifdef SSSP
#define ITERATE_TUPLE_GRAPH_BEGIN(tg, user_buf, user_buf_count,wbuf) \
	do { \
		MPI_Offset block_limit = ITERATE_TUPLE_GRAPH_BLOCK_COUNT(tg); \
		if ((tg)->data_in_file) { \
			assert ((tg)->edgefile != MPI_FILE_NULL); \
			assert ((tg)->weightfile != MPI_FILE_NULL); \
		} \
		/* fprintf(stderr, "%d has block_limit = %td\n", rank, (ptrdiff_t)block_limit); */ \
		MPI_Offset block_idx; \
		packed_edge* edge_data_from_file = (packed_edge*)((tg)->data_in_file ? xmalloc(FILE_CHUNKSIZE * sizeof(packed_edge)) : NULL); \
		float* weight_data_from_file = (float*)((tg)->data_in_file ? xmalloc(FILE_CHUNKSIZE * sizeof(float)) : NULL); \
		int64_t edge_count_i = (int64_t)(-1); \
		if ((tg)->data_in_file && block_limit > 0) { \
			MPI_Offset start_edge_index = FILE_CHUNKSIZE * rank; \
			if (start_edge_index > (tg)->nglobaledges) start_edge_index = (tg)->nglobaledges; \
			edge_count_i = (tg)->nglobaledges - start_edge_index; \
			if (edge_count_i > FILE_CHUNKSIZE) edge_count_i = FILE_CHUNKSIZE; \
		} \
		int break_from_block_loop = 0; \
		for (block_idx = 0; block_idx < block_limit; ++block_idx) { \
			MPI_Offset start_edge_index, end_edge_index; \
			if ((tg)->data_in_file) { \
				start_edge_index = FILE_CHUNKSIZE * (MUL_SIZE(block_idx) + rank); \
				if (start_edge_index > (tg)->nglobaledges) start_edge_index = (tg)->nglobaledges; \
				end_edge_index = start_edge_index + FILE_CHUNKSIZE; \
				if (end_edge_index > (tg)->nglobaledges) end_edge_index = (tg)->nglobaledges; \
				/* fprintf(stderr, "%d trying to read offset = %" PRId64 ", count = %" PRId64 "\n", rank, start_edge_index, edge_count_i); */ \
			} else { \
				start_edge_index = int64_min(FILE_CHUNKSIZE * block_idx, (tg)->edgememory_size); \
				end_edge_index = int64_min(start_edge_index + FILE_CHUNKSIZE, (tg)->edgememory_size); \
			} \
			edge_count_i = end_edge_index - start_edge_index; \
			const packed_edge* restrict const user_buf = ((tg)->data_in_file ? edge_data_from_file : (tg)->edgememory + start_edge_index); \
			const float* restrict const wbuf = ((tg)->data_in_file ? weight_data_from_file : (tg)->weightmemory + start_edge_index); \
			ptrdiff_t const user_buf_count = edge_count_i; \
			assert (user_buf != NULL); \
			assert (wbuf != NULL); \
			assert (user_buf_count >= 0); \
			assert (tuple_graph_max_bufsize((tg)) >= user_buf_count); \
			int iteration_count = 0; (void)iteration_count; \
			int buffer_released_this_iter = 0; /* To allow explicit buffer release to be optional */ \
			while (1) { \
				/* Prevent continue */ assert (iteration_count == 0); \
				buffer_released_this_iter = 0; \
				{
#else
#define ITERATE_TUPLE_GRAPH_BEGIN(tg, user_buf, user_buf_count,unused) \
					do { \
						MPI_Offset block_limit = ITERATE_TUPLE_GRAPH_BLOCK_COUNT(tg); \
						if ((tg)->data_in_file) { \
							assert ((tg)->edgefile != MPI_FILE_NULL); \
						} \
						/* fprintf(stderr, "%d has block_limit = %td\n", rank, (ptrdiff_t)block_limit); */ \
						MPI_Offset block_idx; \
						packed_edge* edge_data_from_file = (packed_edge*)((tg)->data_in_file ? xmalloc(FILE_CHUNKSIZE * sizeof(packed_edge)) : NULL); \
						int64_t edge_count_i = (int64_t)(-1); \
						if ((tg)->data_in_file && block_limit > 0) { \
							MPI_Offset start_edge_index = FILE_CHUNKSIZE * rank; \
							if (start_edge_index > (tg)->nglobaledges) start_edge_index = (tg)->nglobaledges; \
							edge_count_i = (tg)->nglobaledges - start_edge_index; \
							if (edge_count_i > FILE_CHUNKSIZE) edge_count_i = FILE_CHUNKSIZE; \
						} \
						int break_from_block_loop = 0; \
						for (block_idx = 0; block_idx < block_limit; ++block_idx) { \
							MPI_Offset start_edge_index, end_edge_index; \
							if ((tg)->data_in_file) { \
								start_edge_index = FILE_CHUNKSIZE * (MUL_SIZE(block_idx) + rank); \
								if (start_edge_index > (tg)->nglobaledges) start_edge_index = (tg)->nglobaledges; \
								end_edge_index = start_edge_index + FILE_CHUNKSIZE; \
								if (end_edge_index > (tg)->nglobaledges) end_edge_index = (tg)->nglobaledges; \
								/* fprintf(stderr, "%d trying to read offset = %" PRId64 ", count = %" PRId64 "\n", rank, start_edge_index, edge_count_i); */ \
							} else { \
								start_edge_index = int64_min(FILE_CHUNKSIZE * block_idx, (tg)->edgememory_size); \
								end_edge_index = int64_min(start_edge_index + FILE_CHUNKSIZE, (tg)->edgememory_size); \
							} \
							edge_count_i = end_edge_index - start_edge_index; \
							const packed_edge* restrict const user_buf = ((tg)->data_in_file ? edge_data_from_file : (tg)->edgememory + start_edge_index); \
							ptrdiff_t const user_buf_count = edge_count_i; \
							assert (user_buf != NULL); \
							assert (user_buf_count >= 0); \
							assert (tuple_graph_max_bufsize((tg)) >= user_buf_count); \
							int iteration_count = 0; (void)iteration_count; \
							int buffer_released_this_iter = 0; /* To allow explicit buffer release to be optional */ \
							while (1) { \
								/* Prevent continue */ assert (iteration_count == 0); \
								buffer_released_this_iter = 0; \
								{
#endif

#define ITERATE_TUPLE_GRAPH_BLOCK_NUMBER (block_idx)
#define ITERATE_TUPLE_GRAPH_BREAK /* Must be done collectively and before ITERATE_TUPLE_GRAPH_RELEASE_BUFFER */ \
									break_from_block_loop = 1; \
									break
#define ITERATE_TUPLE_GRAPH_RELEASE_BUFFER \
									do { \
										if ((tg)->data_in_file && block_idx + 1 < block_limit) { \
											MPI_Offset start_edge_index = FILE_CHUNKSIZE * (MUL_SIZE((block_idx) + 1) + rank); \
											if (start_edge_index > (tg)->nglobaledges) start_edge_index = (tg)->nglobaledges; \
											edge_count_i = (tg)->nglobaledges - start_edge_index; \
											if (edge_count_i > FILE_CHUNKSIZE) edge_count_i = FILE_CHUNKSIZE; \
											buffer_released_this_iter = 1; \
										} \
									} while (0)
#define ITERATE_TUPLE_GRAPH_END \
									if (!buffer_released_this_iter) ITERATE_TUPLE_GRAPH_RELEASE_BUFFER; \
								} \
								if (break_from_block_loop) ITERATE_TUPLE_GRAPH_RELEASE_BUFFER; \
								iteration_count = 1; \
								break; \
							} \
							/* Prevent user break */ assert (iteration_count == 1); \
							if (break_from_block_loop) break; \
						} \
						if (edge_data_from_file) free(edge_data_from_file); \
					} while (0)

//////////common.h/////////

//////////csr_reference.h/////////

typedef struct oned_csr_graph {
	size_t nlocalverts;
	int64_t max_nlocalverts;
	size_t nlocaledges;
	int lg_nglobalverts;
	int64_t nglobalverts,notisolated;
	int *rowstarts;
	int64_t *column;
#ifdef SSSP 
	float *weights;
#endif
	const tuple_graph* tg;
} oned_csr_graph;

#define SETCOLUMN(a,b) memcpy(((char*)column)+(BYTES_PER_VERTEX*a),&b,BYTES_PER_VERTEX)
#define COLUMN(i) (*(int64_t*)(((char*)column)+(BYTES_PER_VERTEX*i)) & (int64_t)(0xffffffffffffffffULL>>(64-8*BYTES_PER_VERTEX)))

//////////csr_reference.h/////////

//////////bfs_reference.c/////////
typedef struct visitmsg {
	//both vertexes are VERTEX_LOCAL components as we know src and dest PEs to reconstruct VERTEX_GLOBAL
	int vloc;
	int vfrom;
} visitmsg;
//////////bfs_reference.c/////////

//////////bitmap_reference.h/////////

#define ulong_bits 64
#define ulong_mask &63
#define ulong_shift >>6
#define SET_VISITED(v) do {visited[VERTEX_LOCAL((v)) ulong_shift] |= (1UL << (VERTEX_LOCAL((v)) ulong_mask));} while (0)
#define SET_VISITEDLOC(v) do {visited[(v) ulong_shift] |= (1ULL << ((v) ulong_mask));} while (0)
#define TEST_VISITED(v) ((visited[VERTEX_LOCAL((v)) ulong_shift] & (1UL << (VERTEX_LOCAL((v)) ulong_mask))) != 0)
#define TEST_VISITEDLOC(v) ((visited[(v) ulong_shift] & (1ULL << ((v) ulong_mask))) != 0)
#define CLEAN_VISITED()  memset(visited,0,visited_size*sizeof(unsigned long));

//////////bitmap_reference.h/////////

//////////sssp_reference.c/////////
typedef struct  __attribute__((__packed__)) relaxmsg {
	float w; //weight of an edge
	int dest_vloc; //local index of destination vertex
	int src_vloc; //local index of source vertex
} relaxmsg;
//////////sssp_reference.c/////////

//////////utils.c/////////
#if defined(_OPENMP)
#define OMP(x_) _Pragma(x_)
#else
#define OMP(x_)
#endif
//////////utils.c/////////

//////////validate.c/////////

typedef struct edgedist {
	unsigned int vfrom;
	unsigned int vloc;
	int64_t predfrom;
	float distfrom;
#ifdef SSSP
	float w;
#endif
} edgedist;

#define DUMPERROR(text) { printf("Validation Error: %s, edge %llu %llu weight %f pred0 %llu pred1 %llu dist0 %f dist1 %f\n",text,v0,v1,w,predv0,predv1,distv0,distv1); val_errors++; return; }
//////////validate.c/////////

//////////make_graph.h/////////
//////////make_graph.h/////////

//////////main.c/////////
//////////main.c/////////

class graph_500_class {
    public:
    const static int MAXGROUPS=65536;
    const static int AGGR=(1024*32);
    const static int AGGR_intra=(1024*32);
    const static int NRECV=4;
    const static int NRECV_intra=4;
    const static int NSEND=4;
    const static int NSEND_intra=4;

    enum {s_minimum, s_firstquartile, s_median, s_thirdquartile, s_maximum, s_mean, s_std, s_LAST};
    long long FILE_CHUNKSIZE=((MPI_Offset)(1) << 23); /* Size of one file I/O block or memory block to be processed in one step, in edges */
    long long CHUNKSIZE = (1 << 22);
    long long HALF_CHUNKSIZE = ((CHUNKSIZE) / 2);
    long long BITMAPSIZE = (1UL << 29);
    const int INITIATOR_A_NUMERATOR=5700;
    const int INITIATOR_BC_NUMERATOR=1900;
    const int INITIATOR_DENOMINATOR=10000;
    const int SPK_NOISE_LEVEL=0;
//////////common.h/////////
    int rank, size;
    #ifdef SIZE_MUST_BE_A_POWER_OF_TWO
    int lgsize;
    #endif
    MPI_Datatype packed_edge_mpi_type; /* MPI datatype for packed_edge struct */
    int64_t* column;
    float* weights;
    //const int ulong_bits = sizeof(unsigned long) * CHAR_BIT;
//////////common.h/////////

//////////aml.c/////////
    int myproc, num_procs;
    int mygroup, num_groups;
    int mylocal, group_size;
    #ifndef PROCS_PER_NODE_NOT_POWER_OF_TWO
    int loggroup,groupmask;
    //#define PROC_FROM_GROUPLOCAL(g,l) ((l)+((g)<<loggroup))
    //#define GROUP_FROM_PROC(p) ((p) >> loggroup)
    //#define LOCAL_FROM_PROC(p) ((p) & groupmask)
    //#else
    //#define PROC_FROM_GROUPLOCAL(g,l) ((g)*group_size+(l))
    //#define GROUP_FROM_PROC(p) ((p)/group_size)
    //#define LOCAL_FROM_PROC(p) ((p)%group_size)
    #endif

    volatile int ack=0;
    volatile int inbarrier=0;
    MPI_Comm comm, comm_intra;
    char *sendbuf;
    int *sendsize;
    ushort *acks;
    ushort *nbuf;
    ushort activebuf[NSEND];
    MPI_Request rqsend[NSEND];
    char recvbuf[AGGR*NRECV];
    MPI_Request rqrecv[NRECV];

    unsigned long long nbytes_sent, nbytes_rcvd;

    char *sendbuf_intra;
    int *sendsize_intra;
    ushort *acks_intra;
    ushort *nbuf_intra;
    ushort activebuf_intra[NSEND_intra];
    MPI_Request rqsend_intra[NSEND_intra];
    char recvbuf_intra[AGGR_intra*NRECV_intra];
    MPI_Request rqrecv_intra[NRECV_intra];
    volatile int ack_intra=0;
//////////aml.c/////////

//////////graph_generator.c/////////
//////////graph_generator.c/////////

//////////csr_reference.h/////////
    int BYTES_PER_VERTEX=6;
//////////csr_reference.h/////////

//////////csr_reference.c/////////
    int64_t nverts_known = 0;
    int *degrees;
    //int64_t *column;
    //float *weights;
    oned_csr_graph g;
//////////csr_reference.c/////////

//////////bfs_reference.c/////////
    int *q1, *q2;
    int qc,q2c;
    unsigned long *visited;
    int64_t visited_size;
    int64_t *pred_glob;
    int * rowstarts;
    //oned_csr_graph g;
//////////bfs_reference.c/////////

//////////bitmap_reference.h/////////
//int ulong_bits = 64;
//////////bitmap_reference.h/////////

//////////sssp_reference.c/////////
    float *glob_dist;
    float glob_maxdelta, glob_mindelta; //range for current bucket
    volatile int lightphase;
//////////sssp_reference.c/////////

//////////validate.c/////////
    int firstvalidationrun=1;
    int validatingbfs=0;
    unsigned int *vdegrees, *vrowstarts;
    int64_t *vcolumn;
    #ifdef SSSP
    float* vweights;
    #endif
    int64_t *globpred,nedges_traversed;
    float *globdist,prevlevel;
    int64_t val_errors=0;
    int failedtovalidate=0;
    int64_t newvisits;
    int * confirmed=NULL;
    int64_t maxvertex;
//////////validate.c/////////

//////////utils.c/////////
    #if defined(HAVE_LIBNUMA)
    int numa_inited=0;
    int numa_avail= -1;
    #endif
//////////utils.c/////////
    public:
//////////graph_generator.h/////////
    //inline int64_t get_v0_from_edge(const packed_edge* p);
    //inline int64_t get_v1_from_edge(const packed_edge* p);
    //inline void write_edge(packed_edge* p, int64_t v0, int64_t v1);
#ifdef GENERATOR_USE_PACKED_EDGE_TYPE

#if 0
typedef struct packed_edge {
  uint32_t v0_low;
  uint32_t v1_low;
  uint32_t high; /* v1 in high half, v0 in low half */
} packed_edge;
#endif

inline int64_t get_v0_from_edge(const packed_edge* p) {
  return (p->v0_low | ((int64_t)((int16_t)(p->high & 0xFFFF)) << 32));
}

inline int64_t get_v1_from_edge(const packed_edge* p) {
  return (p->v1_low | ((int64_t)((int16_t)(p->high >> 16)) << 32));
}

inline void write_edge(packed_edge* p, int64_t v0, int64_t v1) {
  p->v0_low = (uint32_t)v0;
  p->v1_low = (uint32_t)v1;
  p->high = (uint32_t)(((v0 >> 32) & 0xFFFF) | (((v1 >> 32) & 0xFFFF) << 16));
}

#else

#if 0
typedef struct packed_edge {
  int64_t v0;
  int64_t v1;
} packed_edge;
#endif

inline int64_t get_v0_from_edge(const packed_edge* p) {
  return p->v0;
}

inline int64_t get_v1_from_edge(const packed_edge* p) {
  return p->v1;
}

inline void write_edge(packed_edge* p, int64_t v0, int64_t v1) {
  p->v0 = v0;
  p->v1 = v1;
}

#endif

    void generate_kronecker_range(
       const uint_fast32_t seed[5] /* All values in [0, 2^31 - 1) */,
       int logN /* In base 2 */,
       int64_t start_edge, int64_t end_edge /* Indices (in [0, M)) for the edges to generate */,
       packed_edge* edges /* Size >= end_edge - start_edge */
    #ifdef SSSP
       ,float* weights
    #endif
    );
//////////graph_generator.h/////////

//////////graph_generator.c/////////
    int generate_4way_bernoulli(mrg_state* st, int level, int nlevels);
    inline uint64_t bitreverse(uint64_t x);
    inline int64_t scramble(int64_t v0, int lgN, uint64_t val0, uint64_t val1);
    void make_one_edge(int64_t nverts, int level, int lgN, mrg_state* st, packed_edge* result, uint64_t val0, uint64_t val1);
    /*void generate_kronecker_range(const uint_fast32_t seed[5], int logN, int64_t start_edge, int64_t end_edge,
				packed_edge* edges
				#ifdef SSSP
				, float* weights
				#endif
				);*/
//////////graph_generator.c/////////

/////////splittable_mrg.h////////
//uint_fast32_t mrg_get_uint_orig(mrg_state* state);
//double mrg_get_double_orig(mrg_state* state);
//float mrg_get_float_orig(mrg_state* state);
//void mrg_seed(mrg_state* st, const uint_fast32_t seed[5]);
/*void mrg_skip(mrg_state* state,
              uint_least64_t exponent_high,
              uint_least64_t exponent_middle,
              uint_least64_t exponent_low);*/
/////////splittable_mrg.h////////

/////////splittable_mrg.c////////
#ifdef DUMP_TRANSITION_TABLE
void mrg_update_cache(mrg_transition_matrix* restrict p) { /* Set a, b, c, and d */
  p->a = mod_add(mod_mul_x(p->s), p->t);
  p->b = mod_add(mod_mul_x(p->a), p->u);
  p->c = mod_add(mod_mul_x(p->b), p->v);
  p->d = mod_add(mod_mul_x(p->c), p->w);
}

void mrg_make_identity(mrg_transition_matrix* result) {
  result->s = result->t = result->u = result->v = 0;
  result->w = 1;
  mrg_update_cache(result);
}

void mrg_make_A(mrg_transition_matrix* result) { /* Initial RNG transition matrix */
  result->s = result->t = result->u = result->w = 0;
  result->v = 1;
  mrg_update_cache(result);
}

void mrg_multiply(const mrg_transition_matrix* restrict m, const mrg_transition_matrix* restrict n, mrg_transition_matrix* result) {
  uint_least32_t rs = mod_mac(mod_mac(mod_mac(mod_mac(mod_mul(m->s, n->d), m->t, n->c), m->u, n->b), m->v, n->a), m->w, n->s);
  uint_least32_t rt = mod_mac(mod_mac(mod_mac(mod_mac(mod_mul_y(mod_mul(m->s, n->s)), m->t, n->w), m->u, n->v), m->v, n->u), m->w, n->t);
  uint_least32_t ru = mod_mac(mod_mac(mod_mac(mod_mul_y(mod_mac(mod_mul(m->s, n->a), m->t, n->s)), m->u, n->w), m->v, n->v), m->w, n->u);
  uint_least32_t rv = mod_mac(mod_mac(mod_mul_y(mod_mac(mod_mac(mod_mul(m->s, n->b), m->t, n->a), m->u, n->s)), m->v, n->w), m->w, n->v);
  uint_least32_t rw = mod_mac(mod_mul_y(mod_mac(mod_mac(mod_mac(mod_mul(m->s, n->c), m->t, n->b), m->u, n->a), m->v, n->s)), m->w, n->w);
  result->s = rs;
  result->t = rt;
  result->u = ru;
  result->v = rv;
  result->w = rw;
  mrg_update_cache(result);
}

void mrg_power(const mrg_transition_matrix* restrict m, unsigned int exponent, mrg_transition_matrix* restrict result) {
  mrg_transition_matrix current_power_of_2 = *m;
  mrg_make_identity(result);
  while (exponent > 0) {
    if (exponent % 2 == 1) mrg_multiply(result, &current_power_of_2, result);
    mrg_multiply(&current_power_of_2, &current_power_of_2, &current_power_of_2);
    exponent /= 2;
  }
}
#endif

#ifdef __MTA__
#pragma mta inline
#endif
void mrg_apply_transition(const mrg_transition_matrix* restrict mat, const mrg_state* restrict st, mrg_state* r) {
#ifdef __MTA__
  uint_fast64_t s = mat->s;
  uint_fast64_t t = mat->t;
  uint_fast64_t u = mat->u;
  uint_fast64_t v = mat->v;
  uint_fast64_t w = mat->w;
  uint_fast64_t z1 = st->z1;
  uint_fast64_t z2 = st->z2;
  uint_fast64_t z3 = st->z3;
  uint_fast64_t z4 = st->z4;
  uint_fast64_t z5 = st->z5;
  uint_fast64_t temp = s * z1 + t * z2 + u * z3 + v * z4;
  r->z5 = mod_down(mod_down_fast(temp) + w * z5);
  uint_fast64_t a = mod_down(107374182 * s + t);
  uint_fast64_t sy = mod_down(104480 * s);
  r->z4 = mod_down(mod_down_fast(a * z1 + u * z2 + v * z3) + w * z4 + sy * z5);
  uint_fast64_t b = mod_down(107374182 * a + u);
  uint_fast64_t ay = mod_down(104480 * a);
  r->z3 = mod_down(mod_down_fast(b * z1 + v * z2 + w * z3) + sy * z4 + ay * z5);
  uint_fast64_t c = mod_down(107374182 * b + v);
  uint_fast64_t by = mod_down(104480 * b);
  r->z2 = mod_down(mod_down_fast(c * z1 + w * z2 + sy * z3) + ay * z4 + by * z5);
  uint_fast64_t d = mod_down(107374182 * c + w);
  uint_fast64_t cy = mod_down(104480 * c);
  r->z1 = mod_down(mod_down_fast(d * z1 + sy * z2 + ay * z3) + by * z4 + cy * z5);
/* A^n = [d   s*y a*y b*y c*y]                                           */
/*       [c   w   s*y a*y b*y]                                           */
/*       [b   v   w   s*y a*y]                                           */
/*       [a   u   v   w   s*y]                                           */
/*       [s   t   u   v   w  ]                                           */
#else
  uint_fast32_t o1 = mod_mac_y(mod_mul(mat->d, st->z1), mod_mac4(0, mat->s, st->z2, mat->a, st->z3, mat->b, st->z4, mat->c, st->z5));
  uint_fast32_t o2 = mod_mac_y(mod_mac2(0, mat->c, st->z1, mat->w, st->z2), mod_mac3(0, mat->s, st->z3, mat->a, st->z4, mat->b, st->z5));
  uint_fast32_t o3 = mod_mac_y(mod_mac3(0, mat->b, st->z1, mat->v, st->z2, mat->w, st->z3), mod_mac2(0, mat->s, st->z4, mat->a, st->z5));
  uint_fast32_t o4 = mod_mac_y(mod_mac4(0, mat->a, st->z1, mat->u, st->z2, mat->v, st->z3, mat->w, st->z4), mod_mul(mat->s, st->z5));
  uint_fast32_t o5 = mod_mac2(mod_mac3(0, mat->s, st->z1, mat->t, st->z2, mat->u, st->z3), mat->v, st->z4, mat->w, st->z5);
  r->z1 = o1;
  r->z2 = o2;
  r->z3 = o3;
  r->z4 = o4;
  r->z5 = o5;
#endif
}

#ifdef __MTA__
#pragma mta inline
#endif
void mrg_step(const mrg_transition_matrix* mat, mrg_state* state) {
  mrg_apply_transition(mat, state, state);
}

#ifdef __MTA__
#pragma mta inline
#endif
void mrg_orig_step(mrg_state* state) { /* Use original A, not fully optimized yet */
  uint_fast32_t new_elt = mod_mac_y(mod_mul_x(state->z1), state->z5);
  state->z5 = state->z4;
  state->z4 = state->z3;
  state->z3 = state->z2;
  state->z2 = state->z1;
  state->z1 = new_elt;
}

void mrg_skip(mrg_state* state, uint_least64_t exponent_high, uint_least64_t exponent_middle, uint_least64_t exponent_low) {
  /* fprintf(stderr, "skip(%016" PRIXLEAST64 "%016" PRIXLEAST64 "%016" PRIXLEAST64 ")\n", exponent_high, exponent_middle, exponent_low); */
  int byte_index;
  for (byte_index = 0; exponent_low; ++byte_index, exponent_low >>= 8) {
    uint_least8_t val = (uint_least8_t)(exponent_low & 0xFF);
    if (val != 0) mrg_step(&mrg_skip_matrices[byte_index][val], state);
  }
  for (byte_index = 8; exponent_middle; ++byte_index, exponent_middle >>= 8) {
    uint_least8_t val = (uint_least8_t)(exponent_middle & 0xFF);
    if (val != 0) mrg_step(&mrg_skip_matrices[byte_index][val], state);
  }
  for (byte_index = 16; exponent_high; ++byte_index, exponent_high >>= 8) {
    uint_least8_t val = (uint_least8_t)(exponent_high & 0xFF);
    if (val != 0) mrg_step(&mrg_skip_matrices[byte_index][val], state);
  }
}

#ifdef DUMP_TRANSITION_TABLE
const mrg_transition_matrix mrg_skip_matrices[][256] = {}; /* Dummy version */

void dump_mrg(FILE* out, const mrg_transition_matrix* m) {
  /* This is used as an initializer for the mrg_transition_matrix struct, so
   * the order of the fields here needs to match the struct
   * mrg_transition_matrix definition in splittable_mrg.h */
  fprintf(out, "{%" PRIuFAST32 ", %" PRIuFAST32 ", %" PRIuFAST32 ", %" PRIuFAST32 ", %" PRIuFAST32 ", %" PRIuFAST32 ", %" PRIuFAST32 ", %" PRIuFAST32 ", %" PRIuFAST32 "}\n", m->s, m->t, m->u, m->v, m->w, m->a, m->b, m->c, m->d);
}

void dump_mrg_powers(void) {
  /* transitions contains A^(256^n) for n in 0 .. 192/8 */
  int i, j;
  mrg_transition_matrix transitions[192 / 8];
  FILE* out = fopen("mrg_transitions.c", "w");
  if (!out) {
    fprintf(stderr, "dump_mrg_powers: could not open mrg_transitions.c for output\n");
    exit (1);
  }
  fprintf(out, "/* Copyright (C) 2010 The Trustees of Indiana University.                  */\n");
  fprintf(out, "/*                                                                         */\n");
  fprintf(out, "/* Use, modification and distribution is subject to the Boost Software     */\n");
  fprintf(out, "/* License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at */\n");
  fprintf(out, "/* http://www.boost.org/LICENSE_1_0.txt)                                   */\n");
  fprintf(out, "/*                                                                         */\n");
  fprintf(out, "/*  Authors: Jeremiah Willcock                                             */\n");
  fprintf(out, "/*           Andrew Lumsdaine                                              */\n");
  fprintf(out, "\n");
  fprintf(out, "/* This code was generated by dump_mrg_powers() in splittable_mrg.c;\n * look there for how to rebuild the table. */\n\n");
  fprintf(out, "#include \"splittable_mrg.h\"\n");
  fprintf(out, "const mrg_transition_matrix mrg_skip_matrices[][256] = {\n");
  for (i = 0; i < 192 / 8; ++i) {
    if (i != 0) fprintf(out, ",");
    fprintf(out, "/* Byte %d */ {\n", i);
    mrg_transition_matrix m;
    mrg_make_identity(&m);
    dump_mrg(out, &m);
    if (i == 0) {
      mrg_make_A(&transitions[i]);
    } else {
      mrg_power(&transitions[i - 1], 256, &transitions[i]);
    }
    fprintf(out, ",");
    dump_mrg(out, &transitions[i]);
    for (j = 2; j < 256; ++j) {
      fprintf(out, ",");
      mrg_power(&transitions[i], j, &m);
      dump_mrg(out, &m);
    }
    fprintf(out, "} /* End of byte %d */\n", i);
  }
  fprintf(out, "};\n");
  fclose(out);
}

/* Build this file with -DDUMP_TRANSITION_TABLE on the host system, then build
 * the output mrg_transitions.c */
int main(int argc, char** argv) {
  dump_mrg_powers();
  return 0;
}
#endif

uint_fast32_t mrg_get_uint_orig(mrg_state* state) {
  mrg_orig_step(state);
  return state->z1;
}

double mrg_get_double_orig(mrg_state* state) {
  return (double)mrg_get_uint_orig(state) * .000000000465661287524579692 /* (2^31 - 1)^(-1) */ +
         (double)mrg_get_uint_orig(state) * .0000000000000000002168404346990492787 /* (2^31 - 1)^(-2) */
    ;
}

float mrg_get_float_orig(mrg_state* state) {
  return (float)mrg_get_uint_orig(state) * .000000000465661287524579692; /* (2^31 - 1)^(-1) */
}


void mrg_seed(mrg_state* st, const uint_fast32_t seed[5]) {
  st->z1 = seed[0];
  st->z2 = seed[1];
  st->z3 = seed[2];
  st->z4 = seed[3];
  st->z5 = seed[4];
}

/////////splittable_mrg.c////////

/////////mod_arith.h////////

#ifdef __MTA__
#else
#ifdef FAST_64BIT_ARITHMETIC

inline uint_fast32_t mod_add(uint_fast32_t a, uint_fast32_t b) {
  assert (a <= 0x7FFFFFFE);
  assert (b <= 0x7FFFFFFE);
  return (a + b) % 0x7FFFFFFF;
}

inline uint_fast32_t mod_mul(uint_fast32_t a, uint_fast32_t b) {
  assert (a <= 0x7FFFFFFE);
  assert (b <= 0x7FFFFFFE);
  return (uint_fast32_t)((uint_fast64_t)a * b % 0x7FFFFFFF);
}

inline uint_fast32_t mod_mac(uint_fast32_t sum, uint_fast32_t a, uint_fast32_t b) {
  assert (sum <= 0x7FFFFFFE);
  assert (a <= 0x7FFFFFFE);
  assert (b <= 0x7FFFFFFE);
  return (uint_fast32_t)(((uint_fast64_t)a * b + sum) % 0x7FFFFFFF);
}

inline uint_fast32_t mod_mac2(uint_fast32_t sum, uint_fast32_t a, uint_fast32_t b, uint_fast32_t c, uint_fast32_t d) {
  assert (sum <= 0x7FFFFFFE);
  assert (a <= 0x7FFFFFFE);
  assert (b <= 0x7FFFFFFE);
  assert (c <= 0x7FFFFFFE);
  assert (d <= 0x7FFFFFFE);
  return (uint_fast32_t)(((uint_fast64_t)a * b + (uint_fast64_t)c * d + sum) % 0x7FFFFFFF);
}

inline uint_fast32_t mod_mac3(uint_fast32_t sum, uint_fast32_t a, uint_fast32_t b, uint_fast32_t c, uint_fast32_t d, uint_fast32_t e, uint_fast32_t f) {
  assert (sum <= 0x7FFFFFFE);
  assert (a <= 0x7FFFFFFE);
  assert (b <= 0x7FFFFFFE);
  assert (c <= 0x7FFFFFFE);
  assert (d <= 0x7FFFFFFE);
  assert (e <= 0x7FFFFFFE);
  assert (f <= 0x7FFFFFFE);
  return (uint_fast32_t)(((uint_fast64_t)a * b + (uint_fast64_t)c * d + (uint_fast64_t)e * f + sum) % 0x7FFFFFFF);
}

inline uint_fast32_t mod_mac4(uint_fast32_t sum, uint_fast32_t a, uint_fast32_t b, uint_fast32_t c, uint_fast32_t d, uint_fast32_t e, uint_fast32_t f, uint_fast32_t g, uint_fast32_t h) {
  assert (sum <= 0x7FFFFFFE);
  assert (a <= 0x7FFFFFFE);
  assert (b <= 0x7FFFFFFE);
  assert (c <= 0x7FFFFFFE);
  assert (d <= 0x7FFFFFFE);
  assert (e <= 0x7FFFFFFE);
  assert (f <= 0x7FFFFFFE);
  assert (g <= 0x7FFFFFFE);
  assert (h <= 0x7FFFFFFE);
  return (uint_fast32_t)(((uint_fast64_t)a * b + (uint_fast64_t)c * d + (uint_fast64_t)e * f + (uint_fast64_t)g * h + sum) % 0x7FFFFFFF);
}

inline uint_fast32_t mod_mul_x(uint_fast32_t a) {
  return mod_mul(a, 107374182);
}

inline uint_fast32_t mod_mul_y(uint_fast32_t a) {
  return mod_mul(a, 104480);
}

inline uint_fast32_t mod_mac_y(uint_fast32_t sum, uint_fast32_t a) {
  return mod_mac(sum, a, 104480);
}

#else

inline uint_fast32_t mod_add(uint_fast32_t a, uint_fast32_t b) {
  uint_fast32_t x;
  assert (a <= 0x7FFFFFFE);
  assert (b <= 0x7FFFFFFE);
#if 0
  return (a + b) % 0x7FFFFFFF;
#else
  x = a + b; /* x <= 0xFFFFFFFC */
  x = (x >= 0x7FFFFFFF) ? (x - 0x7FFFFFFF) : x;
  return x;
#endif
}

inline uint_fast32_t mod_mul(uint_fast32_t a, uint_fast32_t b) {
  uint_fast64_t temp;
  uint_fast32_t temp2;
  assert (a <= 0x7FFFFFFE);
  assert (b <= 0x7FFFFFFE);
#if 0
  return (uint_fast32_t)((uint_fast64_t)a * b % 0x7FFFFFFF);
#else
  temp = (uint_fast64_t)a * b; /* temp <= 0x3FFFFFFE00000004 */
  temp2 = (uint_fast32_t)(temp & 0x7FFFFFFF) + (uint_fast32_t)(temp >> 31); /* temp2 <= 0xFFFFFFFB */
  return (temp2 >= 0x7FFFFFFF) ? (temp2 - 0x7FFFFFFF) : temp2;
#endif
}

inline uint_fast32_t mod_mac(uint_fast32_t sum, uint_fast32_t a, uint_fast32_t b) {
  uint_fast64_t temp;
  uint_fast32_t temp2;
  assert (sum <= 0x7FFFFFFE);
  assert (a <= 0x7FFFFFFE);
  assert (b <= 0x7FFFFFFE);
#if 0
  return (uint_fast32_t)(((uint_fast64_t)a * b + sum) % 0x7FFFFFFF);
#else
  temp = (uint_fast64_t)a * b + sum; /* temp <= 0x3FFFFFFE80000002 */
  temp2 = (uint_fast32_t)(temp & 0x7FFFFFFF) + (uint_fast32_t)(temp >> 31); /* temp2 <= 0xFFFFFFFC */
  return (temp2 >= 0x7FFFFFFF) ? (temp2 - 0x7FFFFFFF) : temp2;
#endif
}

inline uint_fast32_t mod_mac2(uint_fast32_t sum, uint_fast32_t a, uint_fast32_t b, uint_fast32_t c, uint_fast32_t d) {
  assert (sum <= 0x7FFFFFFE);
  assert (a <= 0x7FFFFFFE);
  assert (b <= 0x7FFFFFFE);
  assert (c <= 0x7FFFFFFE);
  assert (d <= 0x7FFFFFFE);
  return mod_mac(mod_mac(sum, a, b), c, d);
}

inline uint_fast32_t mod_mac3(uint_fast32_t sum, uint_fast32_t a, uint_fast32_t b, uint_fast32_t c, uint_fast32_t d, uint_fast32_t e, uint_fast32_t f) {
  assert (sum <= 0x7FFFFFFE);
  assert (a <= 0x7FFFFFFE);
  assert (b <= 0x7FFFFFFE);
  assert (c <= 0x7FFFFFFE);
  assert (d <= 0x7FFFFFFE);
  assert (e <= 0x7FFFFFFE);
  assert (f <= 0x7FFFFFFE);
  return mod_mac2(mod_mac(sum, a, b), c, d, e, f);
}

inline uint_fast32_t mod_mac4(uint_fast32_t sum, uint_fast32_t a, uint_fast32_t b, uint_fast32_t c, uint_fast32_t d, uint_fast32_t e, uint_fast32_t f, uint_fast32_t g, uint_fast32_t h) {
  assert (sum <= 0x7FFFFFFE);
  assert (a <= 0x7FFFFFFE);
  assert (b <= 0x7FFFFFFE);
  assert (c <= 0x7FFFFFFE);
  assert (d <= 0x7FFFFFFE);
  assert (e <= 0x7FFFFFFE);
  assert (f <= 0x7FFFFFFE);
  assert (g <= 0x7FFFFFFE);
  assert (h <= 0x7FFFFFFE);
  return mod_mac2(mod_mac2(sum, a, b, c, d), e, f, g, h);
}

inline uint_fast32_t mod_mul_x(uint_fast32_t a) {
  static const int32_t q = 20 /* UINT32_C(0x7FFFFFFF) / 107374182 */;
  static const int32_t r = 7  /* UINT32_C(0x7FFFFFFF) % 107374182 */;
  int_fast32_t result = (int_fast32_t)(a) / q;
  result = 107374182 * ((int_fast32_t)(a) - result * q) - result * r;
  result += (result < 0 ? 0x7FFFFFFF : 0);
  assert ((uint_fast32_t)(result) == mod_mul(a, 107374182));
  return (uint_fast32_t)result;
}

inline uint_fast32_t mod_mul_y(uint_fast32_t a) {
  static const int32_t q = 20554 /* UINT32_C(0x7FFFFFFF) / 104480 */;
  static const int32_t r = 1727  /* UINT32_C(0x7FFFFFFF) % 104480 */;
  int_fast32_t result = (int_fast32_t)(a) / q;
  result = 104480 * ((int_fast32_t)(a) - result * q) - result * r;
  result += (result < 0 ? 0x7FFFFFFF : 0);
  assert ((uint_fast32_t)(result) == mod_mul(a, 104480));
  return (uint_fast32_t)result;
}

inline uint_fast32_t mod_mac_y(uint_fast32_t sum, uint_fast32_t a) {
  uint_fast32_t result = mod_add(sum, mod_mul_y(a));
  assert (result == mod_mac(sum, a, 104480));
  return result;
}

#endif
#endif

/////////mod_arith.h////////

//////////common.h/////////
/*
#ifdef SIZE_MUST_BE_A_POWER_OF_TWO
int MOD_SIZE(int64_t v) {return (int)((v) & ((1 << lgsize) - 1));}
size_t DIV_SIZE(int64_t v) {return (size_t)((v) >> lgsize);}
int64_t MUL_SIZE(int x) {return (int64_t)((x) << lgsize);}
int64_t MUL_SIZE(long x) {return (int64_t)((x) << lgsize);}
#else
int MOD_SIZE(int64_t v) {return (int)((v) % size);}
size_t DIV_SIZE(int64_t v) {return (size_t)((v) / size);}
int64_t MUL_SIZE(int x) {return (int64_t)((x) * size);}
int64_t MUL_SIZE(long x) {return (int64_t)((x) * size);}
#endif

int VERTEX_OWNER(int64_t v) {return MOD_SIZE(v);}
size_t VERTEX_LOCAL(int64_t v) {return DIV_SIZE(v);}
int64_t VERTEX_TO_GLOBAL(int r, long i) {return (int64_t)(MUL_SIZE(i) + (int)(r));}
*/
int isisolated(int64_t v);

void setup_globals(void); /* In utils.c */
void cleanup_globals(void); /* In utils.c */
int lg_int64_t(int64_t x); /* In utils.c */
void* xMPI_Alloc_mem(size_t nbytes); /* In utils.c */
void* xmalloc(size_t nbytes); /* In utils.c */
void* xcalloc(size_t n, size_t unit); /* In utils.c */

//int validate_result(int isbfs, const tuple_graph* const tg, const size_t nlocalverts, const int64_t root, int64_t* const pred, float * dist, int64_t* const edge_visit_count_ptr); /* In validate.c */

//void make_graph_data_structure(const tuple_graph* const tg);
//void free_graph_data_structure(void);
//void run_bfs(int64_t root, int64_t* pred);
//void get_edge_count_for_teps(int64_t* edge_visit_count);
//void clean_pred(int64_t* pred);
//size_t get_nlocalverts_for_pred(void);
//#ifdef SSSP
//void run_sssp(int64_t root, int64_t* pred, float * dist_shortest);
//void clean_shortest(float * dist);
//#endif

inline size_t size_min(size_t a, size_t b) {
    return a < b ? a : b;
}

inline ptrdiff_t ptrdiff_min(ptrdiff_t a, ptrdiff_t b) {
    return a < b ? a : b;
}

inline int64_t int64_min(int64_t a, int64_t b) {
    return a < b ? a : b;
}

inline int64_t tuple_graph_max_bufsize(const tuple_graph* tg) {
    return FILE_CHUNKSIZE;
}

//////////common.h/////////

//////////csr_reference.h/////////

void convert_graph_to_oned_csr(const tuple_graph* const tg, oned_csr_graph* const g);
void free_oned_csr_graph(oned_csr_graph* const g);

//////////csr_reference.h/////////

//////////csr_reference.c/////////

void halfedgehndl(int from, void* data, int sz);
void fulledgehndl(int frompe, void* data, int sz);
void send_half_edge(int64_t src, int64_t tgt);
#ifdef SSSP
void send_full_edge(int64_t src, int64_t tgt, float w);
#else
void send_full_edge(int64_t src, int64_t tgt);
#endif
//////////csr_reference.c/////////

//////////bfs_reference.c/////////
void visithndl(int from, void* data, int sz);
void send_visit(int64_t glob, int from);
void make_graph_data_structure(const tuple_graph* const tg);
void run_bfs(int64_t root, int64_t* pred);
void get_edge_count_for_teps(int64_t* edge_visit_count);
void clean_pred(int64_t* pred);
void free_graph_data_structure(void);
size_t get_nlocalverts_for_pred(void);
//////////bfs_reference.c/////////

//////////aml.c/////////
    void (graph_500_class::*aml_handlers[256]) (int,void *,int);
    //inline void aml_send_intra(void* srcaddr, int type, int length, int local, int from);
    void aml_finalize(void);
    void aml_barrier(void);
    void aml_register_handler(void(graph_500_class::*f)(int,void*,int), int n);
    void process(int fromgroup, int length, char* message);
    void process_intra(int fromlocal, int length, char* message);
    inline void aml_poll_intra(void);
    void aml_poll(void);
    inline void flush_buffer(int node);
    inline void flush_buffer_intra(int node);
    inline void aml_send_intra(void* src, int type, int length, int local, int from);
    void aml_send(void* src, int type, int length, int node);
    //int stringCmp(const void* a, const void* b);
    int aml_init(int *argc, char ***argv);
//////////aml.c/////////
//////////aml.h/////////
    //int aml_init(int *, char***);
    //void aml_finalize(void);
    //void aml_barrier(void);
    //void aml_register_handler(void(*f)(int, void*, int),int n);
    //void aml_send(void *srcaddr, int type, int length, int node);
    int aml_my_pe(void);
    int aml_n_pes(void);

//////////////be careful!!!!!////////////
    int my_pe(void) {return aml_my_pe();}
    int num_pes(void) {return aml_n_pes();}
    double aml_time() {return MPI_Wtime();}
    int aml_long_allsum(long* p) {return MPI_Allreduce(MPI_IN_PLACE, p, 1, MPI_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);}
    int aml_long_allmin(long* p) {return MPI_Allreduce(MPI_IN_PLACE, p, 1, MPI_LONG_LONG, MPI_MIN, MPI_COMM_WORLD);}
    int aml_long_allmax(long* p) {return MPI_Allreduce(MPI_IN_PLACE, p, 1, MPI_LONG_LONG, MPI_MAX, MPI_COMM_WORLD);}
////////////aml.h/////////////////////
//////////sssp_reference.c/////////
void relaxhndl(int form, void* dat, int sz);
void send_relax(int64_t glob, float weight, int fromloc);
void run_sssp(int64_t root, int64_t* pred, float *dist);
void clean_shortest(float* dist);
//////////sssp_reference.c/////////

//////////utils.h/////////
//void* xmalloc(size_t n);
void* xrealloc(void* p, size_t nbytes);
uint_fast64_t random_up_to(mrg_state* st, uint_fast64_t n);
void make_mrg_seed(uint64_t userseed1, uint64_t userseed2, uint_fast32_t* seed);
//////////utils.h/////////

//////////main_utils.c/////////
//////////main_utils.c/////////

//////////utils.c/////////
void xfree(void* p, size_t sz);
//////////utils.c/////////

//////////make_graph.h/////////
void make_graph(int log_numverts, int64_t desired_nedges,
		uint64_t userseed1, uint64_t userseed2,
		int64_t* nedges, packed_edge** result);

void make_random_numbers( int64_t nvalues, uint64_t userseed1,
			uint64_t userseed2, int64_t position,
			double* result);
//////////make_graph.h/////////

//////////validate.c/////////
void frompredhndl(int from, void* data, int sz);
void send_frompred(int vfrom, int64_t src);
void vhalfedgehndl(int from, void* data, int sz);
void send_half(int64_t src);
void vfulledgehndl(int frompe, void* data, int sz);
void vsend_full_edge(int64_t src, int64_t tgt, float w);
void sendedgepreddist(unsigned vloc, unsigned int vedge);
void edgepreddisthndl(int frompe, void* data, int sz);
void makedepthmapforbfs(const size_t nlocalverts, const int64_t root, int64_t* const pred, float* dist);
int validate_result(int isbfs, const tuple_graph* const tg, const size_t nlocalverts, const int64_t root, int64_t* const pred, float* dist, int64_t* nedges_in);
//////////validate.c/////////

//////////main.c/////////
//int compare_doubles(const void* a, const void* b);
void get_statistics(const double x[], int n, volatile double r[s_LAST]);
//int ori_main(int argc, char** argv);
//////////main.c/////////
//private:
public:
#ifndef DUMP_TRANSITION_TABLE
//#include "mrg_transitions.c"
const mrg_transition_matrix mrg_skip_matrices[24][256] = {
/* Byte 0 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{0, 0, 0, 1, 0, 0, 0, 1, 107374182}
,{0, 0, 1, 0, 0, 0, 1, 107374182, 177167401}
,{0, 1, 0, 0, 0, 1, 107374182, 177167401, 45365592}
,{1, 0, 0, 0, 0, 107374182, 177167401, 45365592, 1272612231}
,{107374182, 0, 0, 0, 104480, 177167401, 45365592, 1272612231, 735806205}
,{177167401, 0, 0, 104480, 2147447079, 45365592, 1272612231, 735806205, 279302172}
,{45365592, 0, 104480, 2147447079, 1288502987, 1272612231, 735806205, 279302172, 331753768}
,{1272612231, 104480, 2147447079, 1288502987, 300643231, 735806205, 279302172, 331753768, 1043522871}
,{735806205, 2147447079, 1288502987, 300643231, 1075890875, 279302172, 331753768, 1043522871, 1891773876}
,{279302172, 1288502987, 300643231, 1075890875, 1412703094, 331753768, 1043522871, 1891773876, 321085508}
,{331753768, 300643231, 1075890875, 1412703094, 1483135124, 1043522871, 1891773876, 321085508, 82265008}
,{1043522871, 1075890875, 1412703094, 1483135124, 1247618060, 1891773876, 321085508, 82265008, 2077818766}
,{1891773876, 1412703094, 1483135124, 1247618060, 1672287537, 321085508, 82265008, 2077818766, 1589296063}
,{321085508, 1483135124, 1247618060, 1672287537, 287178247, 82265008, 2077818766, 1589296063, 53047172}
,{82265008, 1247618060, 1672287537, 287178247, 1171826053, 2077818766, 1589296063, 53047172, 294266084}
,{2077818766, 1672287537, 287178247, 1171826053, 818480546, 1589296063, 53047172, 294266084, 1144984146}
,{1589296063, 287178247, 1171826053, 818480546, 1382796450, 53047172, 294266084, 1144984146, 1626297093}
,{53047172, 1171826053, 818480546, 1382796450, 1922108906, 294266084, 1144984146, 1626297093, 601285647}
,{294266084, 818480546, 1382796450, 1922108906, 1860721300, 1144984146, 1626297093, 601285647, 254406953}
,{1144984146, 1382796450, 1922108906, 1860721300, 1544565868, 1626297093, 601285647, 254406953, 703904158}
,{1626297093, 1922108906, 1860721300, 1544565868, 219534298, 601285647, 254406953, 703904158, 1905903125}
,{601285647, 1860721300, 1544565868, 219534298, 171675059, 254406953, 703904158, 1905903125, 41479877}
,{254406953, 1544565868, 219534298, 171675059, 1985272869, 703904158, 1905903125, 41479877, 1648632365}
,{703904158, 219534298, 171675059, 1985272869, 1033350521, 1905903125, 41479877, 1648632365, 993200105}
,{1905903125, 171675059, 1985272869, 1033350521, 1181452678, 41479877, 1648632365, 993200105, 1370703553}
,{41479877, 1985272869, 1033350521, 1181452678, 1189848278, 1648632365, 993200105, 1370703553, 2105966405}
,{1648632365, 1033350521, 1181452678, 1189848278, 195549314, 993200105, 1370703553, 2105966405, 2142815631}
,{993200105, 1181452678, 1189848278, 195549314, 1593652977, 1370703553, 2105966405, 2142815631, 2024783512}
,{1370703553, 1189848278, 195549314, 1593652977, 989663713, 2105966405, 2142815631, 2024783512, 1569479672}
,{2105966405, 195549314, 1593652977, 989663713, 1865249951, 2142815631, 2024783512, 1569479672, 456938607}
,{2142815631, 1593652977, 989663713, 1865249951, 195522780, 2024783512, 1569479672, 456938607, 787213544}
,{2024783512, 989663713, 1865249951, 195522780, 1911959836, 1569479672, 456938607, 787213544, 2065931825}
,{1569479672, 1865249951, 195522780, 1911959836, 767267790, 456938607, 787213544, 2065931825, 581062563}
,{456938607, 195522780, 1911959836, 767267790, 1679812934, 787213544, 2065931825, 581062563, 1798563584}
,{787213544, 1911959836, 767267790, 1679812934, 236702903, 2065931825, 581062563, 1798563584, 36702378}
,{2065931825, 767267790, 1679812934, 236702903, 1594880667, 581062563, 1798563584, 36702378, 1367286470}
,{581062563, 1679812934, 236702903, 1594880667, 680748736, 1798563584, 36702378, 1367286470, 1275940295}
,{1798563584, 236702903, 1594880667, 680748736, 53881550, 36702378, 1367286470, 1275940295, 1217915182}
,{36702378, 1594880667, 680748736, 53881550, 514209232, 1367286470, 1275940295, 1217915182, 302687283}
,{1367286470, 680748736, 53881550, 514209232, 1406143545, 1275940295, 1217915182, 302687283, 1622325543}
,{1275940295, 53881550, 514209232, 1406143545, 1330703513, 1217915182, 302687283, 1622325543, 1085012120}
,{1217915182, 514209232, 1406143545, 1330703513, 899666781, 302687283, 1622325543, 1085012120, 519912539}
,{302687283, 1406143545, 1330703513, 899666781, 782196022, 1622325543, 1085012120, 519912539, 492852451}
,{1622325543, 1330703513, 899666781, 782196022, 923142118, 1085012120, 519912539, 492852451, 1931759766}
,{1085012120, 899666781, 782196022, 923142118, 1835958577, 519912539, 492852451, 1931759766, 1804087753}
,{519912539, 782196022, 923142118, 1835958577, 699539764, 492852451, 1931759766, 1804087753, 1463973421}
,{492852451, 923142118, 1835958577, 699539764, 2010707502, 1931759766, 1804087753, 1463973421, 1605690987}
,{1931759766, 1835958577, 699539764, 2010707502, 861192714, 1804087753, 1463973421, 1605690987, 1050820145}
,{1804087753, 699539764, 2010707502, 861192714, 1157272032, 1463973421, 1605690987, 1050820145, 1326355893}
,{1463973421, 2010707502, 861192714, 1157272032, 6285309, 1605690987, 1050820145, 1326355893, 937925117}
,{1605690987, 861192714, 1157272032, 6285309, 1420268505, 1050820145, 1326355893, 937925117, 769872167}
,{1050820145, 1157272032, 6285309, 1420268505, 1171818120, 1326355893, 937925117, 769872167, 1653982138}
,{1326355893, 6285309, 1420268505, 1171818120, 1734780372, 937925117, 769872167, 1653982138, 941138259}
,{937925117, 1420268505, 1171818120, 1734780372, 543959730, 769872167, 1653982138, 941138259, 107187157}
,{769872167, 1171818120, 1734780372, 543959730, 442444256, 1653982138, 941138259, 107187157, 82806204}
,{1653982138, 1734780372, 543959730, 442444256, 96526128, 941138259, 107187157, 82806204, 497040686}
,{941138259, 543959730, 442444256, 96526128, 44704150, 107187157, 82806204, 497040686, 514985004}
,{107187157, 442444256, 96526128, 44704150, 1144071484, 82806204, 497040686, 514985004, 1393323462}
,{82806204, 96526128, 44704150, 1144071484, 1934427902, 497040686, 514985004, 1393323462, 1661513055}
,{497040686, 44704150, 1144071484, 1934427902, 1528063804, 514985004, 1393323462, 1661513055, 409663323}
,{514985004, 1144071484, 1934427902, 1528063804, 361321526, 1393323462, 1661513055, 409663323, 540061910}
,{1393323462, 1934427902, 1528063804, 361321526, 430442335, 1661513055, 409663323, 540061910, 1315162490}
,{1661513055, 1528063804, 361321526, 430442335, 813846924, 409663323, 540061910, 1315162490, 1427281876}
,{409663323, 361321526, 430442335, 813846924, 895897508, 540061910, 1315162490, 1427281876, 2114335769}
,{540061910, 430442335, 813846924, 895897508, 127418683, 1315162490, 1427281876, 2114335769, 353768805}
,{1315162490, 813846924, 895897508, 127418683, 535531875, 1427281876, 2114335769, 353768805, 948583705}
,{1427281876, 895897508, 127418683, 535531875, 1435801905, 2114335769, 353768805, 948583705, 1640668520}
,{2114335769, 127418683, 535531875, 1435801905, 1145956800, 353768805, 948583705, 1640668520, 571722818}
,{353768805, 535531875, 1435801905, 1145956800, 600829171, 948583705, 1640668520, 571722818, 185977820}
,{948583705, 1435801905, 1145956800, 600829171, 1423697883, 1640668520, 571722818, 185977820, 1358605646}
,{1640668520, 1145956800, 600829171, 1423697883, 1655189350, 571722818, 185977820, 1358605646, 1823922468}
,{571722818, 600829171, 1423697883, 1655189350, 607298766, 185977820, 1358605646, 1823922468, 827919361}
,{185977820, 1423697883, 1655189350, 607298766, 1342383335, 1358605646, 1823922468, 827919361, 1159985741}
,{1358605646, 1655189350, 607298766, 1342383335, 530595544, 1823922468, 827919361, 1159985741, 231974717}
,{1823922468, 607298766, 1342383335, 530595544, 596311027, 827919361, 1159985741, 231974717, 192997329}
,{827919361, 1342383335, 530595544, 596311027, 15589154, 1159985741, 231974717, 192997329, 914407730}
,{1159985741, 530595544, 596311027, 15589154, 373536120, 231974717, 192997329, 914407730, 1127235238}
,{231974717, 596311027, 15589154, 373536120, 2070601235, 192997329, 914407730, 1127235238, 1461320537}
,{192997329, 15589154, 373536120, 2070601235, 217992118, 914407730, 1127235238, 1461320537, 1531891030}
,{914407730, 373536120, 2070601235, 217992118, 1636972237, 1127235238, 1461320537, 1531891030, 27068553}
,{1127235238, 2070601235, 217992118, 1636972237, 67142664, 1461320537, 1531891030, 27068553, 1453533041}
,{1461320537, 217992118, 1636972237, 67142664, 1239497466, 1531891030, 27068553, 1453533041, 838135084}
,{1531891030, 1636972237, 67142664, 1239497466, 1272338648, 27068553, 1453533041, 838135084, 1408488098}
,{27068553, 67142664, 1239497466, 1272338648, 18603490, 1453533041, 838135084, 1408488098, 1458367938}
,{1453533041, 1239497466, 1272338648, 18603490, 2033937988, 838135084, 1408488098, 1458367938, 1308760845}
,{838135084, 1272338648, 18603490, 2033937988, 1531058781, 1408488098, 1458367938, 1308760845, 1609863397}
,{1408488098, 18603490, 2033937988, 1531058781, 412902601, 1458367938, 1308760845, 1609863397, 1674811512}
,{1458367938, 2033937988, 1531058781, 412902601, 372084718, 1308760845, 1609863397, 1674811512, 1074390877}
,{1308760845, 1531058781, 412902601, 372084718, 2022440296, 1609863397, 1674811512, 1074390877, 1324280942}
,{1609863397, 412902601, 372084718, 2022440296, 459346522, 1674811512, 1074390877, 1324280942, 210596557}
,{1674811512, 372084718, 2022440296, 459346522, 1166034579, 1074390877, 1324280942, 210596557, 770203237}
,{1074390877, 2022440296, 459346522, 1166034579, 896765259, 1324280942, 210596557, 770203237, 305071579}
,{1324280942, 459346522, 1166034579, 896765259, 1241116623, 210596557, 770203237, 305071579, 1026967388}
,{210596557, 1166034579, 896765259, 1241116623, 648927597, 770203237, 305071579, 1026967388, 1148482470}
,{770203237, 896765259, 1241116623, 648927597, 10828198, 305071579, 1026967388, 1148482470, 682601157}
,{305071579, 1241116623, 648927597, 10828198, 326981376, 1026967388, 1148482470, 682601157, 1913432071}
,{1026967388, 648927597, 10828198, 326981376, 926285146, 1148482470, 682601157, 1913432071, 1437699927}
,{1148482470, 10828198, 326981376, 926285146, 679759532, 682601157, 1913432071, 1437699927, 928183834}
,{682601157, 326981376, 926285146, 679759532, 652205828, 1913432071, 1437699927, 928183834, 1830580039}
,{1913432071, 926285146, 679759532, 652205828, 236966490, 1437699927, 928183834, 1830580039, 1636372941}
,{1437699927, 679759532, 652205828, 236966490, 1835111556, 928183834, 1830580039, 1636372941, 1369755209}
,{928183834, 652205828, 236966490, 1835111556, 849716251, 1830580039, 1636372941, 1369755209, 1336669569}
,{1830580039, 236966490, 1835111556, 849716251, 580445094, 1636372941, 1369755209, 1336669569, 1078978386}
,{1636372941, 1835111556, 849716251, 580445094, 1961389253, 1369755209, 1336669569, 1078978386, 80508265}
,{1369755209, 849716251, 580445094, 1961389253, 629287069, 1336669569, 1078978386, 80508265, 1137980088}
,{1336669569, 580445094, 1961389253, 629287069, 1566516593, 1078978386, 80508265, 1137980088, 2027217021}
,{1078978386, 1961389253, 629287069, 1566516593, 80037416, 80508265, 1137980088, 2027217021, 1625369288}
,{80508265, 629287069, 1566516593, 80037416, 1655203662, 1137980088, 2027217021, 1625369288, 1945317870}
,{1137980088, 1566516593, 80037416, 1655203662, 1957565548, 2027217021, 1625369288, 1945317870, 202962470}
,{2027217021, 80037416, 1655203662, 1957565548, 727478085, 1625369288, 1945317870, 202962470, 1730183044}
,{1625369288, 1655203662, 1957565548, 727478085, 1617217764, 1945317870, 202962470, 1730183044, 1441150428}
,{1945317870, 1957565548, 727478085, 1617217764, 2018856421, 202962470, 1730183044, 1441150428, 225963583}
,{202962470, 727478085, 1617217764, 2018856421, 368770932, 1730183044, 1441150428, 225963583, 611806225}
,{1730183044, 1617217764, 2018856421, 368770932, 1265335122, 1441150428, 225963583, 611806225, 1588073855}
,{1441150428, 2018856421, 368770932, 1265335122, 793483601, 225963583, 611806225, 1588073855, 1848270487}
,{225963583, 368770932, 1265335122, 793483601, 580808035, 611806225, 1588073855, 1848270487, 685532641}
,{611806225, 1265335122, 793483601, 580808035, 1387420369, 1588073855, 1848270487, 685532641, 1254858127}
,{1588073855, 793483601, 580808035, 1387420369, 1663635045, 1848270487, 685532641, 1254858127, 1976053977}
,{1848270487, 580808035, 1387420369, 1663635045, 927352239, 685532641, 1254858127, 1976053977, 2061094447}
,{685532641, 1387420369, 1663635045, 927352239, 1275976226, 1254858127, 1976053977, 2061094447, 1306212446}
,{1254858127, 1663635045, 927352239, 1275976226, 1575736936, 1976053977, 2061094447, 1306212446, 1762807674}
,{1976053977, 927352239, 1275976226, 1575736936, 1552975963, 2061094447, 1306212446, 1762807674, 291748183}
,{2061094447, 1275976226, 1575736936, 1552975963, 1189178027, 1306212446, 1762807674, 291748183, 1409188710}
,{1306212446, 1575736936, 1552975963, 1189178027, 2077635988, 1762807674, 291748183, 1409188710, 510678116}
,{1762807674, 1552975963, 1189178027, 2077635988, 490591230, 291748183, 1409188710, 510678116, 2029840807}
,{291748183, 1189178027, 2077635988, 490591230, 1358278212, 1409188710, 510678116, 2029840807, 1399453206}
,{1409188710, 2077635988, 490591230, 1358278212, 467274322, 510678116, 2029840807, 1399453206, 621710794}
,{510678116, 490591230, 1358278212, 467274322, 557582480, 2029840807, 1399453206, 621710794, 1843222255}
,{2029840807, 1358278212, 467274322, 557582480, 1418349965, 1399453206, 621710794, 1843222255, 236351264}
,{1399453206, 467274322, 557582480, 1418349965, 872472228, 621710794, 1843222255, 236351264, 1219246015}
,{621710794, 557582480, 1418349965, 872472228, 1299373238, 1843222255, 236351264, 1219246015, 335766221}
,{1843222255, 1418349965, 872472228, 1299373238, 1405886311, 236351264, 1219246015, 335766221, 1395742316}
,{236351264, 872472228, 1299373238, 1405886311, 2117674028, 1219246015, 335766221, 1395742316, 1199667488}
,{1219246015, 1299373238, 1405886311, 2117674028, 65605867, 335766221, 1395742316, 1199667488, 504715705}
,{335766221, 1405886311, 2117674028, 65605867, 241190807, 1395742316, 1199667488, 504715705, 601411222}
,{1395742316, 2117674028, 65605867, 241190807, 1709396335, 1199667488, 504715705, 601411222, 1713650772}
,{1199667488, 65605867, 241190807, 1709396335, 132642498, 504715705, 601411222, 1713650772, 821354916}
,{504715705, 241190807, 1709396335, 132642498, 1228605438, 601411222, 1713650772, 821354916, 511634488}
,{601411222, 1709396335, 132642498, 1228605438, 1235906315, 1713650772, 821354916, 511634488, 1915827703}
,{1713650772, 132642498, 1228605438, 1235906315, 72963340, 821354916, 511634488, 1915827703, 1872029838}
,{821354916, 1228605438, 1235906315, 72963340, 78557229, 511634488, 1915827703, 1872029838, 1356082068}
,{511634488, 1235906315, 72963340, 78557229, 1715089560, 1915827703, 1872029838, 1356082068, 2099454295}
,{1915827703, 72963340, 78557229, 1715089560, 408365116, 1872029838, 1356082068, 2099454295, 1284168848}
,{1872029838, 78557229, 1715089560, 408365116, 875156217, 1356082068, 2099454295, 1284168848, 1284690579}
,{1356082068, 1715089560, 408365116, 875156217, 1161872774, 2099454295, 1284168848, 1284690579, 604856889}
,{2099454295, 408365116, 875156217, 1161872774, 1073370168, 1284168848, 1284690579, 604856889, 1828037898}
,{1284168848, 875156217, 1161872774, 1073370168, 562586079, 1284690579, 604856889, 1828037898, 1855508097}
,{1284690579, 1161872774, 1073370168, 562586079, 1625425421, 604856889, 1828037898, 1855508097, 653875040}
,{604856889, 1073370168, 562586079, 1625425421, 301305479, 1828037898, 1855508097, 653875040, 72449215}
,{1828037898, 562586079, 1625425421, 301305479, 1446482451, 1855508097, 653875040, 72449215, 884254314}
,{1855508097, 1625425421, 301305479, 1446482451, 498986154, 653875040, 72449215, 884254314, 1692735697}
,{653875040, 301305479, 1446482451, 498986154, 1547225282, 72449215, 884254314, 1692735697, 632645241}
,{72449215, 1446482451, 498986154, 1547225282, 1114400836, 884254314, 1692735697, 632645241, 1000349184}
,{884254314, 498986154, 1547225282, 1114400836, 1761611172, 1692735697, 632645241, 1000349184, 1840985687}
,{1692735697, 1547225282, 1114400836, 1761611172, 2144232780, 632645241, 1000349184, 1840985687, 104023419}
,{632645241, 1114400836, 1761611172, 2144232780, 1009873875, 1000349184, 1840985687, 104023419, 866091496}
,{1000349184, 1761611172, 2144232780, 1009873875, 1375608667, 1840985687, 104023419, 866091496, 642979914}
,{1840985687, 2144232780, 1009873875, 1375608667, 601128477, 104023419, 866091496, 642979914, 1879324060}
,{104023419, 1009873875, 1375608667, 601128477, 369283264, 866091496, 642979914, 1879324060, 1859003490}
,{866091496, 1375608667, 601128477, 369283264, 2099563300, 642979914, 1879324060, 1859003490, 375170255}
,{642979914, 601128477, 369283264, 2099563300, 721068441, 1879324060, 1859003490, 375170255, 52887940}
,{1879324060, 369283264, 2099563300, 721068441, 957969266, 1859003490, 375170255, 52887940, 939458487}
,{1859003490, 2099563300, 721068441, 957969266, 905492649, 375170255, 52887940, 939458487, 1328301455}
,{375170255, 721068441, 957969266, 905492649, 1673665932, 52887940, 939458487, 1328301455, 671889511}
,{52887940, 957969266, 905492649, 1673665932, 1916717356, 939458487, 1328301455, 671889511, 715188386}
,{939458487, 905492649, 1673665932, 1916717356, 256547469, 1328301455, 671889511, 715188386, 650476628}
,{1328301455, 1673665932, 1916717356, 256547469, 1735151978, 671889511, 715188386, 650476628, 218994970}
,{671889511, 1916717356, 256547469, 1735151978, 1952814672, 715188386, 650476628, 218994970, 802424609}
,{715188386, 256547469, 1735151978, 1952814672, 2070656144, 650476628, 218994970, 802424609, 608691525}
,{650476628, 1735151978, 1952814672, 2070656144, 1189071915, 218994970, 802424609, 608691525, 1512900793}
,{218994970, 1952814672, 2070656144, 1189071915, 383116831, 802424609, 608691525, 1512900793, 1249465924}
,{802424609, 2070656144, 1189071915, 383116831, 1303690462, 608691525, 1512900793, 1249465924, 1295874118}
,{608691525, 1189071915, 383116831, 1303690462, 1709053087, 1512900793, 1249465924, 1295874118, 1040748781}
,{1512900793, 383116831, 1303690462, 1709053087, 509809742, 1249465924, 1295874118, 1040748781, 252921851}
,{1249465924, 1303690462, 1709053087, 509809742, 193531558, 1295874118, 1040748781, 252921851, 1286124916}
,{1295874118, 1709053087, 509809742, 193531558, 816322037, 1040748781, 252921851, 1286124916, 2084165234}
,{1040748781, 509809742, 193531558, 816322037, 526356231, 252921851, 1286124916, 2084165234, 1300136952}
,{252921851, 193531558, 816322037, 526356231, 1745656682, 1286124916, 2084165234, 1300136952, 431615290}
,{1286124916, 816322037, 526356231, 1745656682, 488716145, 2084165234, 1300136952, 431615290, 1411392617}
,{2084165234, 526356231, 1745656682, 488716145, 1984463596, 1300136952, 431615290, 1411392617, 1168353633}
,{1300136952, 1745656682, 488716145, 1984463596, 889326167, 431615290, 1411392617, 1168353633, 1876266766}
,{431615290, 488716145, 1984463596, 889326167, 1378137622, 1411392617, 1168353633, 1876266766, 1365689348}
,{1411392617, 1984463596, 889326167, 1378137622, 156395847, 1168353633, 1876266766, 1365689348, 537398034}
,{1168353633, 889326167, 1378137622, 156395847, 1041035611, 1876266766, 1365689348, 537398034, 208701205}
,{1876266766, 1378137622, 156395847, 1041035611, 174629419, 1365689348, 537398034, 208701205, 638454909}
,{1365689348, 156395847, 1041035611, 174629419, 1454478932, 537398034, 208701205, 638454909, 49903708}
,{537398034, 1041035611, 174629419, 1454478932, 1967121419, 208701205, 638454909, 49903708, 661164933}
,{208701205, 174629419, 1454478932, 1967121419, 1386641505, 638454909, 49903708, 661164933, 403614502}
,{638454909, 1454478932, 1967121419, 1386641505, 1700430409, 49903708, 661164933, 403614502, 1773913698}
,{49903708, 1967121419, 1386641505, 1700430409, 631849206, 661164933, 403614502, 1773913698, 1943714694}
,{661164933, 1386641505, 1700430409, 631849206, 1996600571, 403614502, 1773913698, 1943714694, 672055334}
,{403614502, 1700430409, 631849206, 1996600571, 405726791, 1773913698, 1943714694, 672055334, 1673745977}
,{1773913698, 631849206, 1996600571, 405726791, 1654276468, 1943714694, 672055334, 1673745977, 746342829}
,{1943714694, 1996600571, 405726791, 1654276468, 2074496352, 672055334, 1673745977, 746342829, 632160356}
,{672055334, 405726791, 1654276468, 2074496352, 372666918, 1673745977, 746342829, 632160356, 1869397711}
,{1673745977, 1654276468, 2074496352, 372666918, 68490361, 746342829, 632160356, 1869397711, 595317168}
,{746342829, 2074496352, 372666918, 68490361, 1238818103, 632160356, 1869397711, 595317168, 1889450553}
,{632160356, 372666918, 68490361, 1238818103, 620067703, 1869397711, 595317168, 1889450553, 1354624380}
,{1869397711, 68490361, 1238818103, 620067703, 106947748, 595317168, 1889450553, 1354624380, 1780312862}
,{595317168, 1238818103, 620067703, 106947748, 1035150630, 1889450553, 1354624380, 1780312862, 626789493}
,{1889450553, 620067703, 106947748, 1035150630, 1168844579, 1354624380, 1780312862, 626789493, 197848980}
,{1354624380, 106947748, 1035150630, 1168844579, 212043318, 1780312862, 626789493, 197848980, 142796175}
,{1780312862, 1035150630, 1168844579, 212043318, 1245466865, 626789493, 197848980, 142796175, 658617292}
,{626789493, 1168844579, 212043318, 1245466865, 644253208, 197848980, 142796175, 658617292, 1702227344}
,{197848980, 212043318, 1245466865, 644253208, 1599897022, 142796175, 658617292, 1702227344, 1433614181}
,{142796175, 1245466865, 644253208, 1599897022, 1731328025, 658617292, 1702227344, 1433614181, 1336937244}
,{658617292, 644253208, 1599897022, 1731328025, 775468291, 1702227344, 1433614181, 1336937244, 737036985}
,{1702227344, 1599897022, 1731328025, 775468291, 516167339, 1433614181, 1336937244, 737036985, 795075306}
,{1433614181, 1731328025, 775468291, 516167339, 559707521, 1336937244, 737036985, 795075306, 925676258}
,{1336937244, 775468291, 516167339, 559707521, 1320219924, 737036985, 795075306, 925676258, 781484869}
,{737036985, 516167339, 559707521, 1320219924, 129434005, 795075306, 925676258, 781484869, 822281942}
,{795075306, 559707521, 1320219924, 129434005, 1155578674, 925676258, 781484869, 822281942, 1082528359}
,{925676258, 1320219924, 129434005, 1155578674, 505537626, 781484869, 822281942, 1082528359, 19278518}
,{781484869, 129434005, 1155578674, 505537626, 581909548, 822281942, 1082528359, 19278518, 360413702}
,{822281942, 1155578674, 505537626, 581909548, 63370533, 1082528359, 19278518, 360413702, 151974102}
,{1082528359, 505537626, 581909548, 63370533, 1934001925, 19278518, 360413702, 151974102, 2095559354}
,{19278518, 581909548, 63370533, 1934001925, 1041711771, 360413702, 151974102, 2095559354, 1811504550}
,{360413702, 63370533, 1934001925, 1041711771, 2027383401, 151974102, 2095559354, 1811504550, 319614985}
,{151974102, 1934001925, 1041711771, 2027383401, 2045318462, 2095559354, 1811504550, 319614985, 322840482}
,{2095559354, 1041711771, 2027383401, 2045318462, 1907574689, 1811504550, 319614985, 322840482, 2009328885}
,{1811504550, 2027383401, 2045318462, 1907574689, 1641043329, 319614985, 322840482, 2009328885, 1474649131}
,{319614985, 2045318462, 1907574689, 1641043329, 1819122949, 322840482, 2009328885, 1474649131, 336628112}
,{322840482, 1907574689, 1641043329, 1819122949, 2921950, 2009328885, 1474649131, 336628112, 1173592299}
,{2009328885, 1641043329, 1819122949, 2921950, 1995399578, 1474649131, 336628112, 1173592299, 1477268091}
,{1474649131, 1819122949, 2921950, 1995399578, 975541374, 336628112, 1173592299, 1477268091, 1639613548}
,{336628112, 2921950, 1995399578, 975541374, 126952865, 1173592299, 1477268091, 1639613548, 412081582}
,{1173592299, 1995399578, 975541374, 126952865, 1565454841, 1477268091, 1639613548, 412081582, 1635974652}
,{1477268091, 975541374, 126952865, 1565454841, 2049606761, 1639613548, 412081582, 1635974652, 618022174}
,{1639613548, 126952865, 1565454841, 2049606761, 1025470496, 412081582, 1635974652, 618022174, 164917641}
,{412081582, 1565454841, 2049606761, 1025470496, 2052973850, 1635974652, 618022174, 164917641, 2102626858}
,{1635974652, 2049606761, 1025470496, 2052973850, 1531532304, 618022174, 164917641, 2102626858, 580864539}
,{618022174, 1025470496, 2052973850, 1531532304, 1965725289, 164917641, 2102626858, 580864539, 1655048518}
,{164917641, 2052973850, 1531532304, 1965725289, 418441524, 2102626858, 580864539, 1655048518, 1771909825}
,{2102626858, 1531532304, 1965725289, 418441524, 1333831799, 580864539, 1655048518, 1771909825, 1250534272}
,{580864539, 1965725289, 418441524, 1333831799, 1319486681, 1655048518, 1771909825, 1250534272, 22806227}
,{1655048518, 418441524, 1333831799, 1319486681, 839170500, 1771909825, 1250534272, 22806227, 1582807597}
,{1771909825, 1333831799, 1319486681, 839170500, 1938420553, 1250534272, 22806227, 1582807597, 1062315347}
,{1250534272, 1319486681, 839170500, 1938420553, 1015759071, 22806227, 1582807597, 1062315347, 1395567976}
,{22806227, 839170500, 1938420553, 1015759071, 768171433, 1582807597, 1062315347, 1395567976, 1997709559}
,{1582807597, 1938420553, 1015759071, 768171433, 1235232437, 1062315347, 1395567976, 1997709559, 428659909}
,{1062315347, 1015759071, 768171433, 1235232437, 464530031, 1395567976, 1997709559, 428659909, 1280866704}
,{1395567976, 768171433, 1235232437, 464530031, 162643012, 1997709559, 428659909, 1280866704, 143836395}
,{1997709559, 1235232437, 464530031, 162643012, 1244952121, 428659909, 1280866704, 143836395, 657738471}
,{428659909, 464530031, 162643012, 1244952121, 316621449, 1280866704, 143836395, 657738471, 1267528990}
,{1280866704, 162643012, 1244952121, 316621449, 615834135, 143836395, 657738471, 1267528990, 1245940812}
,{143836395, 1244952121, 316621449, 615834135, 214803821, 657738471, 1267528990, 1245940812, 1067214725}
,{657738471, 316621449, 615834135, 214803821, 2083471541, 1267528990, 1245940812, 1067214725, 99333652}
,{1267528990, 615834135, 214803821, 2083471541, 1038746080, 1245940812, 1067214725, 99333652, 144985843}
,{1245940812, 214803821, 2083471541, 1038746080, 407332004, 1067214725, 99333652, 144985843, 678709506}
,{1067214725, 2083471541, 1038746080, 407332004, 1879807561, 99333652, 144985843, 678709506, 139020681}
,{99333652, 1038746080, 407332004, 1879807561, 948548466, 144985843, 678709506, 139020681, 1007265410}
,{144985843, 407332004, 1879807561, 948548466, 1738978656, 678709506, 139020681, 1007265410, 312693939}
,{678709506, 1879807561, 948548466, 1738978656, 1918714349, 139020681, 1007265410, 312693939, 1701897288}
,{139020681, 948548466, 1738978656, 1918714349, 1659162940, 1007265410, 312693939, 1701897288, 1922492348}
,{1007265410, 1738978656, 1918714349, 1659162940, 1448846219, 312693939, 1701897288, 1922492348, 1634967356}
} /* End of byte 0 */
,/* Byte 1 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{312693939, 1918714349, 1659162940, 1448846219, 1653915565, 1701897288, 1922492348, 1634967356, 652180261}
,{1490540692, 1016212937, 1288169097, 1445404775, 1735087838, 1783013883, 986236785, 1637092812, 303111895}
,{2000977377, 1001943231, 1311130587, 2040446836, 1344782180, 2126962249, 1533061441, 1611249514, 136599756}
,{351698113, 1701095545, 1429876927, 176189899, 972386072, 826381929, 2107010893, 834600457, 358153365}
,{858255002, 1164423131, 2047535417, 789854044, 1911175580, 1078782245, 59348896, 339585201, 1899694942}
,{1108948050, 1703136668, 1797164158, 423624305, 751050497, 241263027, 316857728, 1171717559, 233575169}
,{78744670, 595706641, 1402720996, 1244172727, 179208595, 1641887830, 1901802079, 471167817, 1839660959}
,{1921475429, 1682175673, 511721276, 803426084, 843045387, 1976026914, 1323350409, 1306621082, 600476373}
,{1560719607, 555888348, 1089888607, 2105962025, 953112022, 761255762, 1038197455, 1205722004, 960606050}
,{2066909054, 1409919306, 256485846, 68185987, 315761891, 42256043, 563818778, 1803584697, 1509868347}
,{1667955123, 548863247, 928966273, 1854233440, 109245176, 287201501, 935819930, 452954641, 58085234}
,{1958192493, 28265551, 1102144206, 653596019, 669122304, 738762549, 1809944955, 1630728020, 98367497}
,{928248412, 335515911, 467999208, 844704075, 1943855929, 1299119155, 1623920239, 168957809, 703604690}
,{1612892272, 114555911, 1021719716, 160242693, 1438155293, 838533804, 1157729614, 1258275881, 1105132917}
,{719229692, 163875986, 1599070253, 1413505520, 103522844, 1200635782, 1393596094, 281501793, 1400861587}
,{1893946831, 905799957, 1645941880, 823563257, 1128134089, 1424034572, 288536321, 829949727, 1589270961}
,{1427756481, 1952124018, 1604365958, 422382890, 360730196, 1559783432, 199448298, 137827621, 419864711}
,{580930584, 1072420559, 1312381756, 775303010, 1936734135, 1298591584, 1287371431, 1505839015, 872819568}
,{1629722972, 419909105, 1238524829, 319314954, 2048002667, 1137996253, 88606864, 717799281, 1904147101}
,{2048441213, 2091969478, 332303260, 332789117, 1254585379, 623395777, 1939475838, 1586707856, 269740900}
,{1395265528, 1952533090, 1358904250, 1329497098, 1668725802, 175699967, 2049028538, 397588745, 2066440653}
,{1255606735, 525291061, 394601392, 685447314, 2015074721, 1696441439, 1840956353, 1436976961, 1619506967}
,{88380627, 460711077, 1534451506, 967994117, 1590409373, 1181397134, 476717415, 264272110, 424172311}
,{1775365434, 645037836, 1252994209, 1040286208, 139759725, 1526898487, 1470199015, 2136329288, 251037933}
,{1996081361, 1750819522, 532901925, 60937824, 324274482, 1159565228, 986047554, 1219059733, 1293467946}
,{2075639444, 1985456167, 64413007, 258351591, 929109183, 1688479091, 654561331, 1210371131, 1686595293}
,{1123798223, 654091347, 409721012, 63880789, 1216396668, 582884516, 1923698349, 356954008, 1950456224}
,{609643813, 1681257991, 1218560561, 551973857, 375742993, 716263380, 967868378, 2145955207, 376277947}
,{696675109, 1400346484, 897755710, 1265413298, 934000773, 2122877837, 1980109567, 1323994226, 1114847888}
,{1908138516, 1131042087, 380074813, 1268850074, 63179262, 33696877, 46158359, 1145320466, 306562193}
,{1832630869, 2139557784, 1820227562, 104868816, 555670797, 317020974, 1065025127, 483729298, 171617178}
,{262388536, 1964109240, 223560501, 1326432190, 1952044537, 1442776523, 40711265, 1849054159, 1197501399}
,{940308882, 482531245, 474202125, 864968428, 611251194, 368171501, 452716282, 921266094, 1792046614}
,{2145065540, 1312128493, 823648467, 2062051006, 152779774, 561355554, 2130412576, 886909875, 1452974053}
,{876997513, 570144107, 1948465114, 1735738748, 1928390330, 1659059348, 79304154, 1063737200, 1556082310}
,{377290071, 110400282, 786541597, 165730193, 1723845622, 1159464763, 702851477, 1745093276, 683566246}
,{1130632317, 1219481583, 1675052276, 2107846701, 1651725478, 501637725, 2036349984, 1824620936, 583611421}
,{1709270644, 382607852, 1551576329, 291562165, 119540335, 213859856, 1047228650, 998773961, 2024827278}
,{1130276897, 1006505571, 53045463, 1736217076, 1744205243, 288786110, 1025712148, 88727636, 1283653841}
,{1074854969, 191545506, 482976500, 1945051261, 98799751, 781713908, 1068370091, 604754088, 746129279}
,{836262229, 1979075486, 1534821121, 1945119835, 1462621252, 505267700, 1357977426, 2114072830, 1796437585}
,{1152236488, 2079349374, 1912330975, 1029956825, 489893955, 387576415, 1239808318, 381275549, 1322815154}
,{337360508, 1950200542, 770376895, 137163297, 939274159, 543634176, 150608204, 513947155, 222521743}
,{742258146, 59680736, 1001366041, 211054277, 1526669618, 444135479, 738544441, 59937905, 2042562263}
,{1387290610, 1204378697, 11633966, 43137784, 1159618202, 1792568807, 135854160, 2143072475, 2020155571}
,{992527729, 1683269963, 65914427, 336589020, 1585349762, 154769252, 1300235377, 1706867738, 773197689}
,{1886133274, 982075485, 948582810, 148074131, 470024557, 1825167392, 1598264411, 769797593, 1596459770}
,{95964553, 1769517349, 2070677133, 527208819, 33250257, 984310479, 1618794283, 282753367, 685905855}
,{773199615, 412138333, 2066066849, 566385851, 1735919232, 1752131203, 1774943475, 1555768370, 117658479}
,{495003552, 1014813534, 413248271, 1911261048, 467113202, 2130052479, 1707839368, 25027081, 565727906}
,{1647541329, 1039630555, 1434824947, 1039660811, 308504423, 1429358731, 2115665397, 2124539022, 1927147777}
,{1028824985, 1562152217, 225932211, 686320315, 455672065, 1738934384, 46801906, 1314184742, 210455770}
,{239583419, 1584543509, 2124006269, 1248258715, 694111164, 1393315130, 562604150, 2125089086, 594575078}
,{1699479898, 1410386868, 1248673807, 108412326, 1026309001, 600820539, 931012436, 1500544891, 1682234295}
,{403880611, 2059238209, 983142497, 177958951, 1976423837, 951512354, 5868079, 68530941, 2059812190}
,{550989945, 881637524, 858449133, 936020052, 1817611943, 1225661955, 2040080184, 651488717, 1267468345}
,{221354924, 1575546824, 1924999433, 1140317556, 1917207470, 1927569330, 176608344, 1508001365, 1926277904}
,{1458300975, 1265387810, 701351464, 1110984424, 1620711167, 218111557, 302889872, 145979510, 495876515}
,{1358077121, 1742041707, 1426391412, 666380672, 1402386436, 1374088897, 623337751, 1629328465, 1368992385}
,{1460983968, 1664050967, 769570418, 2056801789, 1153237441, 2011700037, 1890836505, 1931879924, 906576197}
,{1672876350, 109035968, 1588955367, 177076390, 776105674, 597271069, 198794487, 859117596, 45917786}
,{1356943266, 1937936793, 1537774779, 247805435, 1440145299, 2107251744, 1229733398, 1750134028, 1686591848}
,{1134213795, 608214076, 1266884020, 398654246, 1487400489, 1821851983, 951358373, 1461543186, 1620105468}
,{671147444, 401027466, 946034606, 803370771, 1119823610, 595622590, 1811308523, 491535335, 410915331}
,{930148275, 183165135, 1657257514, 1608004959, 1387771271, 1468225974, 499133329, 252192288, 11013782}
,{1030641991, 948983858, 881949496, 1514386093, 673478871, 1769375167, 1014287464, 1588882210, 1191111921}
,{514493809, 1787307606, 1617857528, 1491228406, 695722998, 426118767, 72851589, 284614344, 1025604707}
,{711580541, 194844871, 1707225951, 2143421818, 1116141830, 53165864, 2118114628, 113591510, 2642978}
,{1532855080, 369863741, 1388369654, 53615650, 1214673023, 1980848110, 1768814639, 1474639991, 1879665032}
,{274025079, 795098418, 989471546, 1025787262, 1582267737, 591815458, 567587771, 2008247548, 1738374554}
,{2060526370, 513281218, 2101533691, 220146973, 928428082, 865838812, 939496648, 750316605, 1202688182}
,{1898947129, 712745136, 1975109045, 155874042, 2041089309, 1014481282, 1834788961, 1768555735, 885223890}
,{196702740, 740332729, 1840726461, 402625160, 80771333, 671486770, 531964268, 1075431125, 241241351}
,{661533327, 294154760, 2020570571, 1917719932, 1460759763, 814237372, 876594032, 751918562, 1412336631}
,{524499350, 945062465, 227464427, 788307628, 546698725, 1835229516, 1303121014, 1835453826, 548534980}
,{1629598130, 854082649, 1145040055, 707546695, 2084133060, 1357465127, 1421546537, 2035366507, 2123374059}
,{1343279220, 1284929790, 28931231, 2142686446, 860497593, 814782063, 65880056, 1690131697, 2094312599}
,{402484807, 1456581759, 260990005, 586582470, 1190833971, 2067331353, 933288402, 474679894, 380450914}
,{253355173, 1383273585, 791411776, 1926284647, 1293151823, 542979998, 386620412, 931974044, 1396457637}
,{51001, 1178578801, 2056838356, 1989641596, 1157710648, 1285935133, 855141783, 2012464519, 345973884}
,{1535005288, 588271937, 552939128, 1327371100, 1951201118, 910013545, 771305299, 950040063, 1940809643}
,{1537483069, 1252260005, 23376490, 189891780, 2011698199, 1680508572, 723688678, 1869336025, 1894301502}
,{1419818886, 354791592, 1647360014, 371273744, 184451241, 502100076, 1042128258, 1939264136, 1223695711}
,{1878618447, 786553337, 1837327085, 41866207, 1960723521, 880656157, 1206974883, 2089031192, 370569145}
,{905292614, 1342542487, 482661775, 456292632, 1685042686, 381444978, 134407668, 1268243407, 1992776770}
,{2002254530, 145802132, 269592759, 1924641782, 1997304700, 518754870, 1161770378, 1303273785, 2078029787}
,{2057315234, 1162041245, 1845101087, 248790476, 1537739959, 1945219466, 1808519368, 474802156, 942062475}
,{1316832064, 1464112652, 153525729, 696472602, 30271024, 1432718159, 1692183838, 2036943541, 1572198614}
,{1499678535, 678863065, 84125397, 1157675314, 1434710829, 1764588313, 862383858, 641092599, 1102954237}
,{1576355733, 1677528850, 1303851517, 41342994, 594716829, 374185067, 1924506020, 1515249534, 1567618045}
,{500361084, 2115928600, 710623780, 1407182306, 1002645185, 222815303, 954760971, 106648325, 1502189183}
,{554574380, 689477521, 511862719, 1370811853, 369812270, 495376488, 1197474407, 1703315087, 525271266}
,{680058010, 77427193, 1170398184, 1553378582, 28387068, 913148713, 99176858, 1303918317, 1397376757}
,{1578801438, 418136807, 1363796831, 743530455, 1421904699, 1798291586, 1378639870, 1334748324, 1384239515}
,{1548711230, 1763191646, 1134013060, 273912643, 611728566, 147400892, 223429289, 1162080033, 1600864925}
,{595469324, 568551835, 1702363589, 1665919013, 1324923467, 789634301, 1533365766, 1773486089, 1670570977}
,{459293309, 1216962610, 1567386836, 818805576, 710131202, 2022577593, 107865402, 995801050, 1435342658}
,{1050467969, 2037323329, 1559251236, 1919458385, 1015680602, 488543534, 744015905, 48440083, 1320849120}
,{1582697253, 1296792921, 1582631153, 387873051, 2017661340, 2138713253, 82462238, 144262903, 141808224}
,{608975647, 1818941310, 84291694, 1296133838, 471909377, 209935463, 332936829, 2145973589, 687186262}
,{454141115, 822279448, 1068794301, 1206867695, 126907893, 126459146, 1668778694, 2126033705, 2067150655}
,{2102660197, 1189202215, 825451022, 1809039965, 703859457, 131148599, 672174830, 500036951, 1709962530}
,{2076085680, 159946747, 1975622410, 2124517694, 703790261, 1580800406, 2066587362, 1615960482, 352952457}
,{515733287, 1811911077, 1896465084, 324617601, 218344167, 235540056, 1384529335, 1450645069, 676986034}
,{524792024, 1054724607, 986662722, 1378014102, 1269288891, 1300544128, 1390465736, 461854365, 1644510775}
,{1613844544, 706989387, 1498082765, 608483708, 622885217, 571640526, 1942253675, 1539307657, 1909488637}
,{1932716273, 1100765401, 420745730, 195054538, 1280416522, 1820179076, 1501669971, 850586054, 338466309}
,{1420232715, 1950849869, 1599004806, 2047259074, 756754638, 916897507, 2029709955, 799989678, 262009886}
,{1625813766, 48122134, 727136944, 1744869703, 2066738031, 123332410, 1757712424, 1559167084, 1950526281}
,{1408965261, 72427661, 571859098, 750199078, 1279463630, 1834147649, 896275062, 651251171, 85158079}
,{1953890346, 1534325413, 1068284177, 506014694, 1110077440, 1494708886, 1189381161, 197105470, 2114832349}
,{1556580339, 1394178781, 1318846118, 1802699011, 643114279, 742001480, 1059145600, 1431998051, 1323030967}
,{95065998, 1864271677, 1885954880, 31378801, 983624314, 1616250213, 568648029, 798719632, 1992562631}
,{1767511345, 256457456, 516406163, 647955004, 1183609041, 174699397, 133138827, 1352975691, 1891183555}
,{1826282846, 1226882812, 1932562034, 715452833, 608997079, 1231928910, 427645092, 1854267239, 2000113010}
,{715094734, 51250840, 998921628, 442889990, 933231540, 1304206236, 112952716, 2121343457, 2016122430}
,{1583281283, 2004290796, 2030968974, 594139231, 1316110854, 1772264894, 766431167, 1077507599, 831609012}
,{186242185, 831489454, 231180584, 511507729, 463569452, 1303175601, 2029926953, 1196897666, 688900363}
,{232910, 481244912, 1843531872, 637374459, 1000842954, 1554905217, 977192499, 187982902, 1149797303}
,{1209135230, 1378555283, 1310829061, 19637814, 356256120, 2029099776, 171147410, 1033478044, 424035534}
,{425066314, 1223944921, 1467767169, 1207640135, 551217215, 430926617, 994820306, 1503698122, 239671237}
,{1894745532, 98514332, 1921950691, 1378411817, 615508098, 723843584, 2098102166, 1288321153, 1560460065}
,{1314430366, 1499301458, 1583808938, 845519359, 84181640, 1683495924, 1424082094, 1850329179, 1476675892}
,{855469330, 1539515635, 548355650, 2046577385, 2112615144, 166359546, 1134374903, 1971668716, 993034364}
,{1052507536, 920441642, 2084309885, 1640687860, 842274460, 122567275, 1504540427, 1865717987, 940892441}
,{876242430, 1790705070, 683693034, 918507775, 1438495216, 410278396, 110598866, 1524043266, 1549325167}
,{425734971, 653635638, 885650677, 196956304, 2111915301, 1685744404, 725136865, 480029313, 1192285765}
,{941446626, 764417580, 1019861092, 2146303402, 1389650141, 1079156355, 105285456, 1679956763, 1123787821}
,{546655753, 439701231, 1473411677, 1115002163, 185129032, 1644236088, 1756922505, 1036950198, 1754931745}
,{1682345727, 1323396471, 1617331350, 267434695, 311281296, 1486194743, 1419285737, 1596045787, 504284547}
,{1130103543, 265348005, 995097999, 233924448, 1183085432, 191934312, 68927531, 1390915818, 481516531}
,{309784146, 218738568, 1544255781, 1427870415, 1485918726, 754559211, 313792416, 888546340, 1174927507}
,{1251963111, 1993608809, 1776088951, 1658927159, 149566068, 589054079, 1462545841, 1254410297, 1535883564}
,{601203539, 1501932956, 460692137, 385485330, 1271649986, 1184137535, 1656856735, 1416198208, 1634974072}
,{560138621, 195180069, 1712480444, 599611846, 1783373799, 106505734, 1030958343, 560898973, 835439882}
,{146904381, 419009549, 1533394219, 1433749044, 1550350457, 474967198, 1152407335, 493535565, 1914483921}
,{356823776, 2000967606, 1127239285, 981359937, 1248634026, 1446582555, 84064479, 844563187, 1704656187}
,{754131443, 1067756091, 513495673, 894325019, 1279393566, 1125932633, 1515283622, 578724116, 647343396}
,{1161349442, 1036686413, 1828673623, 519768625, 1778710959, 844962473, 781317481, 353681689, 473806362}
,{1683923445, 1612788585, 834083809, 440791568, 1621558475, 1560286291, 1469099613, 1322471074, 514448505}
,{187785570, 346018255, 623763138, 1807869008, 527129693, 1354035129, 1116218484, 1846689268, 739781908}
,{641463442, 635644009, 1122554399, 1639705172, 1664775554, 625880169, 1869863981, 1092626961, 1389730300}
,{1633581327, 187930031, 998165356, 1277772739, 743430462, 367795843, 1191559358, 645978599, 409963770}
,{2072649482, 795917967, 911185786, 1301307402, 1146766782, 285239013, 59732855, 743529991, 2067647291}
,{1097221525, 1821517360, 1277987257, 1568995859, 882661632, 1974360738, 372212634, 794476343, 926717459}
,{2019448501, 1976646003, 1226972522, 1002303269, 2009983317, 1377213210, 1818689722, 580510231, 840437095}
,{248579610, 1413415727, 1259661761, 836492051, 1378898473, 252671040, 1171226897, 104440090, 268602618}
,{897030869, 131747595, 1774308605, 399094650, 1392137342, 784154432, 640861095, 1785406002, 981993606}
,{2075679591, 2099014574, 955975504, 213688007, 1677343613, 406159076, 384323098, 2011910205, 1510045953}
,{1961225979, 576569876, 1835862313, 1868983862, 489441017, 1930250248, 2019268185, 1699110909, 861119840}
,{1780107625, 1309894392, 1394507387, 1499975875, 1538805884, 1223727635, 429331803, 1671832291, 2134780588}
,{1879022346, 1529028241, 2078045806, 1859823492, 1552505234, 1515615514, 903335282, 1758404508, 1796057115}
,{1009274784, 1460222641, 1647151780, 1891766649, 1685509748, 1536473196, 679889432, 794811889, 226209581}
,{671807548, 748677521, 1840864770, 1987535397, 277205192, 1372538338, 1145727987, 190666231, 1391588017}
,{1026373216, 1076206496, 1866237363, 1658481065, 1565537885, 287479141, 1872993846, 1647178313, 237406199}
,{1797514595, 924373710, 1320281115, 151630056, 1618131451, 1905856337, 331108850, 1109483782, 1444560492}
,{926093270, 724775786, 823507753, 805406870, 1976194267, 1474384965, 844343927, 1261505772, 675673788}
,{1712489973, 369543503, 841112737, 537437655, 1624232950, 1166036383, 755122550, 1346886586, 1797067739}
,{2123168138, 1753706609, 1293216114, 1403149197, 320859278, 795849396, 585172096, 768842234, 1555003049}
,{469019000, 1583455415, 1013248623, 732358762, 1098297022, 1419298765, 1053364967, 1115300300, 707941917}
,{1724988571, 259295416, 982366943, 821766987, 1209960647, 836665422, 904282410, 1579009967, 1408926435}
,{491768362, 708122139, 2096995799, 168385697, 1604313766, 750751577, 1512110200, 1786630774, 334747901}
,{2138102227, 1820669156, 577427028, 1060752054, 80825747, 1823952653, 1334907970, 1667276088, 356272575}
,{18434956, 1412510318, 353355676, 1765115143, 1474389860, 976561354, 1514797755, 698065017, 907944557}
,{1485500093, 65198820, 900551025, 1232190274, 216988986, 941138158, 356404305, 1644319679, 1681586563}
,{749933075, 64453639, 2142497891, 827621988, 233271894, 1412589798, 1433343097, 3829357, 2057292719}
,{938425462, 811326751, 513002227, 2107102498, 598766701, 697626204, 698329785, 252074338, 295792318}
,{881643107, 1927684630, 1276852272, 1556357086, 1377025659, 223245172, 339723003, 1759576582, 975922220}
,{350397182, 1305663945, 1793073041, 1809483488, 1139835994, 1397773296, 874355658, 1288710643, 1010909816}
,{1903658225, 29395794, 1090036730, 126092634, 727206787, 2047469974, 1876660792, 757751545, 998864658}
,{285913622, 778839807, 1754459990, 1675849889, 1205894621, 893518404, 1871225278, 806172677, 601611637}
,{119380511, 1197452534, 1522035490, 549969138, 871131852, 189301714, 811534796, 1983918877, 2002121345}
,{1578345651, 678077965, 2044622844, 2057412934, 1552072462, 1306772993, 835633020, 1764941377, 612220433}
,{2114030128, 2014596192, 918274653, 655730609, 306183418, 2133679106, 815732060, 370224388, 1035598341}
,{77780503, 836624126, 832731568, 1899868393, 175889748, 1131523497, 114575797, 1537644317, 1463075337}
,{250411113, 836919135, 1198769713, 763446614, 1258347275, 2145139616, 18474118, 542232308, 1927559426}
,{1145481733, 671629055, 1555928102, 1393609174, 1954510991, 1666574819, 865252733, 339151441, 1943182169}
,{2078909763, 896220392, 663807074, 590782226, 1773867169, 490724522, 706801856, 2061388494, 408136102}
,{1280515433, 936032095, 726726163, 348546661, 627054786, 1883716064, 496922270, 1248365690, 1263868618}
,{1219392870, 1874529349, 80965756, 194569366, 653540568, 374000021, 57439931, 1355581396, 1897073997}
,{556387109, 1489964626, 936034307, 582542358, 136535128, 114113132, 37101252, 1858047108, 345212099}
,{1278475410, 73197859, 892784666, 931195654, 8963113, 699473289, 1614336656, 2084164742, 1641737465}
,{252991860, 579282841, 547847420, 1190294297, 701813306, 490735690, 1449831752, 1971343372, 1300333314}
,{237806756, 119127330, 1022559994, 538266023, 1750701242, 1753881883, 730823882, 497226029, 395556126}
,{1541032249, 2005375005, 340450955, 1723664456, 233670443, 284897712, 1529226944, 1617931755, 1278007064}
,{491127597, 247645527, 742399393, 1472127805, 898667456, 1901111968, 936003663, 1466649070, 1459082105}
,{1895153230, 576925191, 790708849, 724986580, 1936658671, 987363384, 874628394, 1922105195, 727050941}
,{1942588252, 801624442, 1417476499, 108173759, 1116379769, 1410208742, 1138651804, 139142357, 745557397}
,{1148007632, 2055170127, 1996286251, 1924574048, 1226775713, 794373997, 1396132805, 1972798478, 321547881}
,{798508685, 655449803, 1407449460, 124540662, 263368978, 912842675, 551083612, 1220151586, 480561017}
,{1323189424, 718135641, 1999363857, 2066595110, 1865058350, 684516072, 900789773, 999699413, 763544279}
,{936024010, 136387117, 370706377, 1799190012, 517899554, 882520537, 1887185289, 2105042802, 2143366585}
,{128363447, 933155033, 423047013, 1664212100, 1090798439, 1639847103, 171223074, 960038930, 1828526637}
,{823172988, 1957447619, 1434223383, 936694720, 343410092, 380846885, 1837797885, 830336372, 1341282550}
,{2131161113, 310921412, 458949405, 1310640153, 1944983135, 960879393, 1518505988, 1638156516, 942131625}
,{1946817707, 634516752, 1445408899, 1231151996, 1250640067, 704749831, 232378817, 827696863, 1283068712}
,{681052549, 171433214, 2075742841, 1506844588, 1728501639, 899432463, 2083064026, 1422017273, 479176317}
,{2078895116, 484684663, 2007287530, 1019892736, 1610259715, 1475058290, 417275305, 1410717291, 150141022}
,{574885214, 556213525, 1912194687, 62731099, 1082849632, 1858242253, 510190622, 98912746, 1692475265}
,{715817941, 302550299, 2063742387, 770317850, 2095644482, 159388202, 75221234, 99745324, 342746701}
,{1147089970, 2021495608, 282770556, 292446768, 2022577141, 546272295, 1702187988, 555674431, 861723449}
,{371161603, 1496973592, 128162583, 411153437, 1816326751, 1689189578, 1469681513, 1292629278, 1149158139}
,{863880651, 1408342094, 589239118, 684025361, 1530641413, 139616225, 1077244351, 1488105844, 1439301097}
,{374705027, 261365181, 56783564, 1856991354, 177649954, 881837698, 1680875652, 409691417, 1859619058}
,{564060272, 1004771536, 1536280075, 945393681, 2002371182, 2095840629, 1769103496, 2044194375, 750032239}
,{2023412844, 1701548188, 292764532, 118919087, 2064141960, 1422850422, 9515249, 1081956391, 719089582}
,{1005033287, 1697265731, 1234585841, 985613245, 135240568, 2097123357, 178470119, 815774521, 2104577315}
,{508350056, 146311113, 530707854, 903832735, 1190217424, 1686375511, 1121592431, 1692391390, 1671622261}
,{201837623, 2019367897, 759691813, 354740523, 630753048, 123363629, 1682882184, 195228488, 1421416536}
,{53466201, 1379537234, 149404493, 1277445041, 1463518148, 1468198246, 279780201, 1286896153, 261485218}
,{1797388533, 459057386, 953803746, 1024238778, 413936309, 1225835770, 1598503050, 1538504534, 1378698275}
,{977653318, 1981021395, 1864225647, 27906252, 133500217, 1424094369, 184676612, 1251759626, 339629442}
,{1021605017, 1563269396, 381073616, 42617072, 1399306170, 883585093, 1467683204, 2105908327, 1413857532}
,{803705661, 1715574018, 936307134, 970836790, 200375145, 1541651219, 289355025, 1406433443, 30245987}
,{1988271820, 67429518, 1761493527, 254648368, 1417499769, 1519018028, 2088830676, 1241544549, 1949326818}
,{2078190531, 1087139080, 1753973826, 473345296, 836702915, 1540888400, 1214662886, 692458380, 594342482}
,{1614617948, 1717692804, 548535033, 1986727678, 1452851408, 2011569981, 2099343369, 70841493, 676437609}
,{913143544, 1186365326, 13891550, 63623220, 1651854358, 1296261815, 1170812650, 727580616, 967704413}
,{1608321929, 1998927234, 1877629788, 1669121864, 1383925143, 254898553, 1036796018, 1091494893, 250282654}
,{972687533, 328316780, 836940557, 427329099, 1522900111, 1383740514, 1855869930, 851516447, 1976488631}
,{1656558384, 1655938582, 957516312, 730320389, 1464840090, 1505639877, 108419808, 1551366915, 384990758}
,{729946080, 1117553488, 59998674, 213171452, 1327385012, 862072360, 1905756995, 1156769239, 815141596}
,{472746710, 39987169, 1936201832, 1608853023, 805829423, 948267644, 2033804886, 1541266407, 1018005457}
,{1367733300, 441772064, 500040521, 884817994, 203157290, 2110549056, 1479335269, 1333418291, 917576894}
,{555674102, 458054739, 1876728100, 436052410, 1138713024, 478317168, 420826903, 610885541, 1032277267}
,{1106270735, 1957197264, 1013382828, 327385743, 1726145331, 1033131595, 114915858, 72416828, 412309253}
,{205426304, 1802551393, 295611946, 1643158357, 29907339, 12665269, 1257546743, 1525139544, 2073088875}
,{86118563, 1756613235, 1163077075, 1853126634, 489161415, 2048594285, 982939987, 113233268, 1308523230}
,{1525217444, 1799277390, 115668246, 1200982500, 835873731, 1694948014, 1025674994, 197751158, 551912461}
,{400787236, 231066659, 1474385013, 1315185431, 2048800950, 1808778044, 1270809427, 1622021408, 192603269}
,{1656018907, 20444189, 1014974017, 2004538483, 701880557, 192456848, 1806607579, 1264851648, 1118175939}
,{47740694, 2131914178, 1671343220, 1316423662, 395631557, 1470959841, 1263881458, 659316787, 916489958}
,{203274205, 1873604315, 2117222029, 1901592453, 1056376261, 191845608, 761585878, 1420289031, 1740391106}
,{446223657, 70145202, 1346143033, 145889738, 1469473743, 1739328022, 952126590, 886387255, 622367292}
,{985915762, 1477948858, 2043544189, 1742194409, 1698482616, 1347626706, 68636289, 537055702, 1725261485}
,{1884351556, 301644298, 1108428368, 1499186247, 1300626254, 1360108171, 1813506514, 220213873, 471932122}
,{1691079806, 84974232, 804378893, 28985994, 2141454419, 137341394, 112064311, 1170879491, 765278956}
,{833970317, 1656414714, 1881788398, 276657115, 869798074, 1042402556, 1087450774, 1399287897, 57924763}
,{1681702018, 1501192648, 1894979791, 232693144, 644838785, 697848577, 1328610242, 2129911571, 1080485741}
,{627698545, 1283607364, 1733685630, 1911855924, 637920446, 1600783785, 1710282217, 991134601, 398397518}
,{2121356651, 1398674318, 619321425, 467452537, 489160863, 1837315496, 1694247919, 1914575230, 892801356}
,{969943396, 1830370542, 583573841, 1517182490, 666663683, 1061393624, 641582802, 1507376874, 1642320330}
,{1620932285, 1332573943, 396595440, 932059820, 1687446792, 1302118555, 1551466681, 496420664, 1943196289}
,{1209613320, 1044387226, 1984468763, 171547595, 152552431, 621022564, 49123948, 1013347672, 1086370934}
,{1157843205, 1388739227, 1203597480, 1199989920, 1898031004, 1520365017, 349347177, 755595861, 1740946635}
,{2098111121, 1466863394, 1871036769, 338004646, 1431877302, 839898684, 2006568959, 1675814975, 308471149}
,{1098612978, 995897770, 118213194, 1321261782, 6748148, 396634863, 301513539, 1108357861, 1873680726}
,{1918039191, 1042136620, 1004623393, 1362332275, 1486958283, 1551938909, 1427812416, 433101200, 1335372863}
,{1706736313, 606834521, 576157191, 1683375779, 135133728, 1405341182, 299036142, 1793461494, 1010660758}
,{1115429275, 1709702319, 1056158010, 950153845, 1956033284, 782431161, 889681286, 1283010489, 325863607}
,{1416716424, 1546822756, 676541530, 1890273073, 90931706, 1480468737, 1983738572, 336971114, 1476230369}
,{775757276, 962600597, 1886998271, 104997002, 1457665031, 261588821, 1902816366, 83256368, 140035114}
,{314253602, 1953163559, 1879073620, 692225200, 2022506351, 2057923163, 1480923060, 173902129, 780524600}
,{971452087, 2119529524, 479718582, 628383116, 1402372015, 383656923, 667561206, 1038981788, 1897721848}
,{833832024, 961803108, 754816731, 1597341125, 412031720, 1099458629, 1336373852, 270616818, 102567469}
} /* End of byte 1 */
,/* Byte 2 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{1841195089, 1488605044, 70958207, 42299766, 358764160, 1810554404, 2014244542, 1699546188, 622916453}
,{1492876117, 632513910, 622638443, 758371028, 1744353458, 1935368369, 911627155, 2049914259, 919509285}
,{746814505, 1549606791, 367684326, 417383095, 1855988473, 1825092626, 373147001, 394155827, 322169563}
,{324543768, 349086733, 1476201052, 991873218, 2059583029, 1094489873, 341510320, 872344606, 251023864}
,{1442047988, 1610722385, 529817400, 1118058889, 911667037, 1964999048, 701061192, 13694013, 155254856}
,{645176773, 1438037227, 1569378510, 1588035248, 1915118326, 460606080, 1408166382, 1309925379, 1349270261}
,{539845751, 1636173371, 1659721947, 474005036, 1213620331, 480859717, 1169298499, 2104860026, 1121164416}
,{1305529819, 874978240, 592466250, 2026365239, 1890475564, 310668621, 591106415, 1282607082, 1656311450}
,{1834769364, 995490617, 1272115049, 1805509723, 458070657, 782818069, 1964496366, 1762181089, 807674917}
,{1073525991, 439750824, 339986086, 1769424051, 1506475496, 1245132733, 1300054000, 1314405151, 80066052}
,{485553444, 638335632, 76219088, 1842662032, 1618638231, 897888656, 1479944976, 895184561, 1412697817}
,{740858403, 224058730, 415653557, 847981676, 395051016, 286880836, 2033232182, 351098777, 2097527544}
,{632040916, 359858413, 1117674985, 573608425, 240752102, 1856631010, 1541595955, 1644662576, 1383107118}
,{1975673105, 798581198, 1558633908, 859185964, 607092008, 643966523, 1655368172, 1568297292, 1346678144}
,{525760283, 353253881, 454151010, 233517642, 369269384, 491360329, 1248542536, 1514514672, 1127679437}
,{1389695502, 563722919, 488978747, 186097697, 1446458569, 292077858, 172003132, 1414386789, 1917790834}
,{1903581582, 1143657322, 194107071, 961839963, 558339732, 692152133, 1347718195, 2100751330, 896818590}
,{451363390, 1446595021, 1891400395, 1566938630, 199138278, 214876011, 849826150, 195757654, 1633861652}
,{1977696358, 1939041108, 2008019528, 166446940, 1832229020, 1032099018, 1432036507, 416853439, 1578956134}
,{1879262282, 151808584, 73624403, 174928238, 368627535, 1856298797, 1249280924, 167176644, 739612439}
,{1563781965, 168922865, 1976224973, 1324664547, 1626193075, 158470089, 739644436, 636292265, 1940361694}
,{1424280874, 142479859, 959931056, 304178388, 837158781, 1147220106, 1202649113, 1279115569, 1355835973}
,{1562538882, 693321316, 513353543, 1514756137, 1406313158, 361181072, 1675430356, 498858783, 1553835131}
,{582076310, 1065498298, 525362734, 1883042636, 1169812722, 1935513413, 1243797410, 373971719, 931548438}
,{751733178, 1101374736, 365007970, 79098941, 1280219555, 623519759, 39401872, 1353798474, 162144995}
,{877785889, 1823359403, 1431882674, 222696380, 2030557112, 335018336, 885129527, 664520322, 2012723364}
,{455199638, 977719931, 172415300, 671505125, 1099944846, 603651693, 1357001578, 2129289855, 1965306132}
,{392538003, 877479765, 445581704, 846465215, 314338622, 1062214011, 1254922806, 1051487327, 697937334}
,{500317088, 2144337988, 110031159, 777954021, 1045498034, 680736819, 1911882737, 1934156163, 690665924}
,{549414891, 1670622007, 2027850200, 1021293350, 1291119561, 511959154, 1204419402, 814494924, 1435543067}
,{1605693737, 1006417876, 1738651896, 79390692, 1262596208, 122302521, 1803220196, 1166250541, 961782701}
,{1812517847, 754062902, 2135988828, 331784467, 1737239903, 871300932, 972040043, 313692999, 1520073171}
,{1448007831, 1131261544, 133175049, 1524734127, 1632517556, 1805574809, 467591507, 2112696376, 463577095}
,{782524778, 812448509, 285448085, 1557059652, 304216489, 323816472, 1460602508, 1904842233, 1033386078}
,{1588441027, 923462523, 1631113353, 819228749, 2012617470, 1119127440, 1239418749, 1351799828, 250997342}
,{342480107, 1634867284, 531893750, 1631111684, 1292843532, 119134876, 60699814, 965621655, 418005041}
,{1220821423, 1590211822, 452934149, 1486950818, 1104703395, 1485046871, 1114283750, 23209682, 1311328371}
,{1740471691, 1221993589, 568939489, 1577707101, 424060623, 1793944503, 263181460, 1485593590, 977844690}
,{293866215, 1117248586, 179366407, 1584662553, 1832817430, 477524499, 2052342297, 544220202, 1857088724}
,{1284138581, 1667559303, 876329549, 22190656, 1554370381, 1325484982, 627158170, 876427120, 1247620889}
,{1686630382, 1873988732, 1110640557, 1175496248, 774470078, 1498416463, 908317342, 1072333543, 721275885}
,{2076042331, 990839792, 1249625533, 1426217254, 480932128, 1445340982, 958504554, 446495566, 968903774}
,{1686878895, 1110833275, 656168947, 151618172, 754837880, 2131038397, 1735666608, 403128318, 398994604}
,{1320790320, 2003025039, 1921313831, 922170587, 481985270, 1540748427, 2133671158, 2108120964, 173639662}
,{805783706, 729520412, 116351572, 621287264, 322156780, 1091741209, 700609790, 1449815661, 2069579128}
,{93285413, 1128783683, 259506273, 1080900733, 706702247, 344514512, 1427416382, 796053364, 857580299}
,{979303749, 126874296, 773222332, 1348489216, 1602833254, 750485625, 1047423275, 445020158, 1232327834}
,{1739892849, 1799552983, 2128356499, 1463078657, 594453625, 9474480, 2125040431, 1900430512, 1217793134}
,{1872599771, 236021484, 337679156, 568258554, 464769298, 761727570, 1144816330, 1241314662, 245057531}
,{1476987404, 1721461484, 652234785, 228105294, 1297130961, 1634012622, 295078732, 1413317926, 1446714781}
,{342858906, 284881741, 1365430574, 1211297317, 1375247513, 809126218, 867488033, 156057229, 139511477}
,{1240025165, 1245287783, 1897423078, 1197289782, 139102074, 1348149887, 29706247, 1938511872, 749113107}
,{1179435316, 355217794, 1627640064, 1755202373, 1853625195, 1660402351, 80131600, 1727156313, 497501209}
,{983197421, 249953478, 1660132631, 313299437, 1962637872, 13208563, 1977632181, 1875986003, 1628165318}
,{906865151, 604062766, 112919691, 220615884, 422856470, 1467775969, 565565743, 344790421, 409554005}
,{266060520, 394834030, 1624867746, 532800402, 1290474441, 301712848, 230778061, 559402263, 1416806196}
,{557963095, 2136316551, 1952273170, 764093179, 1476578086, 1404158556, 1031320946, 1047375942, 1324744871}
,{1216951903, 2082575172, 1560974073, 656296158, 1264168173, 1978764553, 116787203, 937543184, 1365524788}
,{1719177888, 1076011395, 747260717, 776444301, 307920850, 1333292593, 1676472680, 189678863, 563655795}
,{1887247074, 1650133450, 1424944414, 1876460642, 1115786199, 345351880, 1304071256, 990538973, 17478282}
,{1730135031, 1980738826, 1944901245, 656702812, 414750729, 408823924, 83825954, 2130602281, 1923897760}
,{668916699, 478536060, 1258895922, 1489660770, 2139620432, 137041033, 459312284, 1758398200, 1524181062}
,{1011527388, 1895678486, 1389246755, 1960133989, 1746955757, 253153712, 441649497, 1483434118, 1013005451}
,{867081930, 1775641679, 824132775, 1432974174, 743051653, 398421180, 684685362, 1408082662, 464971086}
,{147733520, 1973160383, 362923842, 1234315349, 1583098090, 1921453651, 871531070, 2003021298, 667292271}
,{1203921189, 147835735, 1899496731, 1364878550, 1746348857, 692830960, 1657005895, 248055575, 1122658494}
,{2082649937, 1972830532, 1348580990, 52169871, 1117985211, 921780507, 1777577089, 396385531, 12882634}
,{751905443, 36484442, 817135492, 927447636, 1164455887, 95440084, 1213228192, 1791307957, 215375555}
,{1178878744, 1818062452, 151784350, 1012998094, 515964936, 1834951621, 1764409112, 1683945093, 1322448524}
,{1322183253, 243744077, 1338085685, 2133527251, 42601299, 1176844309, 1892557818, 1256383650, 676608845}
,{138632274, 1956766080, 1594663912, 1979238278, 2037499459, 1263999690, 78522197, 1629632962, 1681876287}
,{556870760, 638893680, 1655244131, 933184387, 1969439507, 443988914, 855602917, 311600819, 1753005038}
,{1468461893, 711632964, 929757561, 833768594, 41945989, 1593535672, 1660510264, 682086731, 984331639}
,{1581852863, 803431249, 1339746509, 1678641562, 1606383197, 571905294, 495334562, 1720022830, 2078117030}
,{676175681, 1081948722, 2047283343, 1970041717, 1786268618, 952661416, 1284355118, 1305769061, 1436623629}
,{152029652, 1827655445, 1364390301, 2005111179, 1789037635, 915451608, 1902975697, 1016947138, 1218357772}
,{856345469, 1805764693, 1779780645, 860782167, 1861920983, 324927773, 914436648, 1399722799, 1264643821}
,{1927279076, 1929341655, 137735671, 691365220, 2021585694, 825297249, 815249275, 2016640709, 134645440}
,{30716072, 1007026531, 1810865950, 482474258, 999861455, 137282447, 366952723, 676163352, 2051694470}
,{454239280, 1481001298, 251909000, 763795810, 1304138521, 1322017550, 862944681, 569139354, 460694653}
,{914712528, 1688824623, 2122622574, 1021216209, 603355638, 80185050, 1020815983, 986053162, 472985396}
,{37970339, 1314625165, 978362774, 489821368, 26775807, 1193961364, 989973026, 787575903, 73246788}
,{970461646, 1595274922, 701361307, 1875634658, 603242810, 1899858440, 36410853, 1111271583, 536420303}
,{135401189, 1718820340, 2000965898, 279971586, 1930979424, 490313918, 1614607662, 2077090916, 774500874}
,{160801739, 197390763, 155435185, 993584385, 1417856956, 33735972, 1432117783, 814465708, 1991787417}
,{1461338992, 1194482472, 134276916, 1932435944, 1459571335, 1971504013, 840114882, 1853144100, 810970900}
,{2069130977, 904710628, 605018748, 1335019134, 1957906491, 2005875886, 547207282, 1358244950, 408778935}
,{1268432390, 1567439653, 253608082, 986690816, 142140895, 49746493, 1632061180, 415469403, 318849151}
,{1509332824, 996726384, 76881964, 355427235, 1460638433, 897956625, 299468057, 2075974515, 197176441}
,{917188435, 265008294, 20518140, 1704710779, 407835830, 1554605077, 1301767463, 1571214714, 1361149233}
,{1511846443, 1826918069, 2102036298, 1954709944, 559966670, 1619894361, 1642447454, 735608241, 409877968}
,{23807830, 1142103193, 953440212, 49207096, 335209239, 60028629, 1898797833, 780492225, 598907872}
,{940931994, 1579924795, 1734939035, 1453985671, 216452538, 606353503, 1844837856, 378795692, 1372364234}
,{787049764, 533949095, 253356031, 296070825, 361375779, 687978407, 764182865, 565477734, 1666697125}
,{306179896, 250555060, 2145215424, 2087832416, 2097398993, 1861379014, 849487675, 1253640818, 1443876342}
,{1328518626, 552285828, 2067258785, 1460331229, 1306741140, 731549403, 2133339041, 821036747, 1770997555}
,{1823247615, 496645825, 387909514, 2000323671, 1056023761, 1469121895, 1484329586, 2125053410, 1385996891}
,{876359590, 174350937, 1161546871, 1074198513, 125747627, 941366904, 1261565184, 1062147428, 612989486}
,{1635961441, 1670355854, 985019829, 507510802, 2103853521, 1205143532, 1851709781, 2114270208, 75368760}
,{1519270373, 1836270955, 1399997774, 514951442, 128948400, 552907048, 2065473766, 436280718, 1908985431}
,{1993338614, 800033158, 518941063, 1388978512, 1340116023, 1605603196, 1674966862, 1017488475, 447124145}
,{828521457, 668951900, 561730311, 713378444, 2004660142, 56846843, 863956463, 733116229, 566953456}
,{2103148147, 1424471835, 875243471, 1083834052, 2130587624, 1439989260, 371247230, 2027639345, 1957784765}
,{1162401808, 107935631, 449283122, 1026979813, 1910153835, 560088457, 2078613262, 514213536, 1300682368}
,{980153351, 203399679, 258102351, 1648075689, 1363984, 1041462012, 1182080835, 697476485, 294118126}
,{1711888320, 1476282766, 2102565422, 217541558, 2077835353, 877121854, 1151327679, 1854686335, 891824224}
,{853743333, 866374259, 1929662594, 1268668187, 1788171707, 1963428463, 1564585179, 613689192, 714387031}
,{446589943, 1410431868, 1817633832, 1798024153, 1312877773, 1576247935, 729076143, 1864970050, 1733880079}
,{2060169803, 1309161064, 1123339029, 694960459, 1776409314, 910224180, 804760566, 1057539355, 869399628}
,{529820140, 1986814834, 868211731, 364376587, 2077256966, 1801377785, 774600418, 2026001723, 1690278910}
,{1930207021, 1137894750, 1850583184, 1376020870, 105358096, 569696475, 1114318506, 1630254487, 286388302}
,{1473503410, 1383491993, 1533168560, 382468184, 2093436240, 1941507623, 1175763439, 2011060445, 1926435996}
,{718332808, 1413880585, 1751143915, 1606998801, 539115183, 2021457561, 1151007951, 237778377, 133770204}
,{751978999, 208233834, 1707126069, 84335917, 1964603588, 1985150649, 1978690983, 1861400267, 2064732771}
,{1495236380, 359595642, 1859802499, 185954440, 164732207, 1983746556, 735994475, 1538969109, 592460660}
,{158370092, 525727336, 1203485279, 1192554958, 2133256718, 1758787992, 1876399670, 1609556897, 1247789257}
,{1560865692, 1405699866, 545478900, 1560629947, 904417784, 403415, 8466793, 806047293, 2018165602}
,{563789743, 338837688, 659793634, 2036661662, 908748424, 463633825, 1034392707, 278759844, 1240679208}
,{2008495944, 330036915, 196883614, 226661074, 1523010446, 56560064, 606584321, 121730744, 1909901415}
,{1510754894, 1202744135, 585447182, 1491239263, 350224335, 29734828, 1434033451, 22959914, 1845426918}
,{1884689066, 297271460, 476874964, 1547143234, 1981804740, 281875381, 485592763, 1699308314, 742801736}
,{238589965, 265350022, 1852171214, 1794589718, 329267702, 718714446, 97382605, 149893071, 1457921133}
,{1776894547, 755091066, 908562437, 683060349, 91502324, 884797251, 1779999405, 596931469, 848943951}
,{739282285, 452032584, 122625147, 15610449, 1911753770, 730154696, 1585057921, 1715698006, 1955504562}
,{1546322520, 1141225323, 1962092997, 292375657, 1780237179, 600012441, 1859462825, 178434580, 1717785076}
,{2100580412, 984283576, 1302446131, 699204888, 1894328464, 1537570620, 764296414, 1934939696, 787602841}
,{153447303, 1986647467, 247535385, 954426617, 1339513336, 107579811, 1390998457, 145454610, 214862399}
,{563065758, 759975544, 611258691, 1741373803, 429994382, 348154164, 918901463, 1741880838, 1753071371}
,{502349238, 1098612146, 305115496, 731843443, 121641219, 708041548, 916294413, 1807004769, 455557191}
,{1062437260, 1380696098, 884917196, 121968139, 1159455889, 1008843057, 209699579, 2088682751, 1609532932}
,{1534052441, 221620131, 1189554162, 1529267231, 1476887031, 1939559606, 1154953394, 480788449, 127495068}
,{1779532773, 381764752, 1727837803, 1925537472, 134916088, 1154792652, 464666916, 1333407322, 2030455537}
,{161396139, 25249205, 615995692, 2124728390, 1320591329, 2008870021, 20265367, 721771141, 1175345612}
,{1442292008, 1650580482, 644020039, 1530496257, 386999483, 2004771738, 1875085213, 122597156, 2062077396}
,{703880074, 931390865, 1145843531, 1473918377, 2001388672, 40787745, 1668438732, 30971362, 57813413}
,{1429580271, 1820947110, 1168518302, 792026407, 1019543206, 354226374, 400293977, 329800968, 1763106326}
,{924318276, 783007922, 1100352505, 1349364251, 1109844353, 29999796, 660355847, 1869858981, 562767892}
,{56986627, 191193126, 443172431, 634627920, 11215130, 922867083, 442291499, 372451713, 1276721401}
,{839623712, 1572782749, 1483623098, 2024557832, 1305865176, 419920991, 370283110, 821216920, 1018439254}
,{772746857, 2004699612, 1619471940, 1918460883, 1342091509, 1412115665, 1662102369, 155609048, 2146621801}
,{478534481, 1065797931, 33433980, 535970305, 322925324, 1005685045, 218315126, 1103805105, 473464449}
,{553400443, 1389710483, 1881928365, 2000229417, 503131984, 1518142875, 813707447, 319567440, 391283380}
,{1026385271, 827347753, 1909917345, 1796988189, 747006415, 1649228914, 688442131, 589665802, 755371749}
,{1635453806, 1845266000, 1354112253, 72016742, 640580788, 1917102262, 897874826, 402005647, 1251498088}
,{495379617, 2100398979, 697518525, 1046208605, 282288728, 1604893566, 780050871, 1954306806, 242526440}
,{1847866473, 715464133, 1780210397, 871638771, 1195188203, 1464575238, 1052860699, 395763344, 1486167762}
,{221160328, 1578636281, 7111192, 730176523, 1179389498, 212739978, 1865387482, 292039269, 2043543395}
,{2054880983, 1223236109, 1665795018, 234123767, 1463750824, 826150312, 517648950, 1126688458, 854661499}
,{799159333, 1189019838, 534771341, 2141246748, 917029486, 157694795, 2090190898, 1194931569, 1465171078}
,{804290582, 51904006, 1138156647, 694072575, 1816224782, 2132634314, 1894973190, 1104573782, 1644372323}
,{777581920, 1906992840, 622861937, 808530898, 1698549172, 1634839168, 909661687, 1241768584, 1693426897}
,{534547955, 743116108, 121498013, 1218370981, 643050580, 19153412, 1403284507, 1478840680, 125456342}
,{2126250901, 881058343, 145579477, 718078036, 709066802, 244244710, 1133835652, 1609725746, 789907885}
,{660960487, 1733744867, 1455664722, 1451993915, 1601249940, 106544326, 2062619302, 944825524, 1700057736}
,{238804718, 1092810976, 94446494, 135322235, 2119351350, 794480960, 1963861805, 2132325162, 1587785908}
,{587542815, 1884460516, 941310619, 311645860, 1939387927, 1141949619, 434254070, 1233398759, 1400324179}
,{170942760, 1250046262, 465600353, 778353873, 1302694058, 1190216296, 1767011567, 911519101, 1091036555}
,{1446526800, 748704866, 1211121267, 1918792343, 269471641, 242420486, 1770519191, 332742985, 689882508}
,{1856195390, 904397949, 268446889, 894450941, 593834102, 1328471386, 447726998, 522998127, 1162404034}
,{1053705993, 374811456, 122108528, 744484116, 2080862601, 1401878729, 597818614, 2038486154, 723147353}
,{1779319604, 1933882239, 524952913, 1900084851, 1837803166, 1740617107, 667356202, 1881258545, 1716233587}
,{166846334, 495011515, 2035263688, 2074589765, 849524506, 1939853851, 389947199, 1830734063, 530890131}
,{957381893, 307330183, 1911492522, 1643575850, 1260600266, 1368110891, 466286069, 299259720, 1155859364}
,{237933605, 1532327678, 1454353559, 2085223733, 480560506, 1985921828, 1618274378, 1304079336, 1742119656}
,{1622939996, 970136319, 491075762, 1447528308, 941997311, 2120094238, 1681778061, 966280169, 1570166893}
,{1091937897, 1962544578, 1076663563, 1756819218, 54767773, 1258243767, 1387897521, 1378429268, 431310988}
,{1285142711, 894764556, 9189720, 797384461, 960511603, 1626080613, 835925876, 75313675, 397280905}
,{1509225026, 338174255, 294150781, 2040369534, 2126673574, 454190590, 1208925898, 1402497105, 25186852}
,{170689542, 115472479, 1076745825, 264629872, 560075346, 270479504, 1411574728, 629572176, 2057712002}
,{1319175593, 173782795, 1976747188, 1555151851, 106001477, 1107935708, 300479502, 1664732390, 597086964}
,{983674610, 1836797113, 1067661676, 1983569788, 1951388645, 418769176, 491595735, 1274640369, 324148510}
,{1685327376, 1024196651, 736006173, 754083683, 844266509, 4835340, 734313804, 926570581, 627340988}
,{1567023633, 1922188627, 1458449358, 292171537, 300543453, 622111079, 1133336298, 1828239115, 1271272498}
,{1775617307, 280890643, 2088229886, 252405253, 9068178, 411043862, 11629252, 1536825203, 1940785551}
,{1032954543, 1542764846, 1986907277, 2097922718, 282400785, 1503353303, 1782856168, 185432871, 1398615286}
,{1505670470, 2136946339, 1681275606, 1421604988, 511446568, 536219851, 527231017, 914951585, 728084425}
,{1194766024, 703550080, 174584973, 41035303, 117080296, 714878701, 31751610, 1103664063, 52920421}
,{1338677424, 1816126915, 766169402, 866572872, 1406421157, 1777086546, 788434205, 1127491812, 152805564}
,{1582417159, 1221168182, 391271617, 722842207, 2115615974, 559947994, 1698528372, 1416847465, 9106626}
,{1062220436, 145155895, 113613836, 2136392059, 1074402763, 1491365660, 1739119502, 1742448598, 249797389}
,{1144232945, 66935242, 2122366861, 1125317187, 1919769631, 203324623, 225842143, 1368394984, 1870328116}
,{980675056, 1589488519, 155502110, 475170953, 1530179907, 816755520, 2017121325, 306049401, 1530436799}
,{1889184277, 1305008335, 286581012, 285517757, 326920054, 321671291, 1355112066, 455473628, 1026497743}
,{1935279877, 1084973944, 1619212658, 1514992165, 1301576301, 85503440, 1589286454, 314496812, 332508958}
,{74167153, 82830705, 1171666650, 1152308050, 1956634626, 1452736572, 1951699038, 254465022, 2082320233}
,{1142498431, 155945186, 2090109209, 2099606270, 2116360213, 937186741, 1869468032, 586299000, 1911155563}
,{1022273152, 432678325, 1077313921, 1424920362, 47944993, 1363372910, 1673875226, 1483309127, 280406075}
,{355021005, 2030254560, 1834387765, 1678289267, 1832880130, 295384473, 979383923, 1657627441, 1360084708}
,{347408671, 621218114, 1745996542, 2064460428, 1567014174, 1680741085, 1694608074, 827102508, 2136521755}
,{1257016779, 1854245595, 2107220657, 1904312837, 1195712187, 1306915540, 1649800218, 1112134396, 376968419}
,{1768785775, 1092532705, 1769087158, 112964778, 1556421611, 2084070419, 932288329, 753031504, 1722357314}
,{658161389, 1503837147, 2050509639, 1317970985, 1633643402, 92364655, 1481311098, 584763736, 999479365}
,{1269434329, 1711869176, 1858920728, 1271355682, 677201235, 86451155, 1291791912, 2107718701, 46873872}
,{1704274889, 568991656, 1308684190, 274966581, 1016683436, 938863086, 1624327204, 135948789, 1935469001}
,{1277875188, 534012130, 1689848462, 286022244, 891309647, 945749273, 607216940, 73496315, 328715025}
,{2011874447, 207409355, 2060574340, 498795029, 1088366910, 254872575, 1434498027, 748339996, 396951182}
,{1643050204, 2124664152, 570342723, 1760418584, 2019141619, 1979093310, 951401888, 138937735, 1433642500}
,{548422770, 1138816154, 1893239673, 1436416811, 763631205, 2020610008, 2045019629, 1687027582, 387919916}
,{153105827, 1449639829, 2069872333, 1729124786, 954526160, 188419, 1962432204, 1471770244, 868903304}
,{591577190, 1779678838, 557937569, 862595645, 227417900, 498884998, 168579455, 266721924, 563561956}
,{132021276, 2069726937, 1181282741, 1663346862, 57590357, 1594022761, 730748957, 1085462180, 1825162241}
,{549795487, 122242455, 209279321, 1281499185, 1267365894, 681433311, 1151893668, 1737329860, 659300443}
,{292769924, 373995874, 951810436, 1169424274, 664303011, 701023130, 1780194164, 975853046, 966999539}
,{2035757612, 1547413162, 1826121020, 1806236848, 288627897, 2123388186, 1727180249, 20607755, 1892027918}
,{268524954, 523829708, 972400910, 1955056584, 960788122, 1933084527, 1047440602, 1803200738, 114919499}
,{1745464768, 816395960, 1236767844, 1258291356, 1982354365, 1064476750, 1937942805, 1116882286, 88207012}
,{426146367, 2112614410, 1137255932, 1818453051, 536403522, 567598811, 2119712354, 432308633, 1780959871}
,{130439740, 352375589, 354446776, 393878257, 586466118, 306721680, 247094188, 1166388750, 1251971879}
,{1513710315, 1207655689, 68166486, 1274847043, 1308761000, 140986167, 770440604, 1434689561, 913993836}
,{1184303845, 1205126870, 1352385373, 1683783166, 1740111402, 1327491436, 458266641, 1630764024, 1598840723}
,{2115094671, 729585700, 2030832568, 1350103189, 1097449856, 1170418571, 654818427, 1872536016, 12565521}
,{1991654077, 607155134, 913119183, 1805809782, 90501714, 1735437307, 1057335402, 1650490756, 1230816867}
,{636645626, 203895418, 1429610531, 539084314, 1787396119, 625314543, 1532872988, 861572227, 89981469}
,{656406810, 1301096965, 1859175605, 612034218, 1544223338, 2145096405, 1645262775, 1646804982, 1182589959}
,{1074062055, 68558680, 484813967, 1932330853, 1698689305, 1303249696, 1746663491, 354630990, 500826635}
,{67704871, 1866851541, 1731958483, 1221308147, 356789052, 876787195, 888212053, 158814652, 1589694112}
,{2089975909, 70686675, 322053773, 1963312759, 1684049776, 305562748, 1074100270, 513635841, 1611651414}
,{586413818, 669005964, 274737799, 1902235547, 1702160706, 249012763, 509705879, 1616464307, 1888017475}
,{815845038, 1669050013, 284356522, 835328645, 1118304782, 1168755885, 412162874, 46826545, 1638786403}
,{1081047557, 1185722470, 122018601, 1004005526, 1997469531, 485233278, 1884922236, 2062269661, 1383049332}
,{752924987, 1456426753, 1320500395, 64674997, 1479535882, 1944522284, 1069414325, 227250895, 863127157}
,{1947995851, 1482658762, 313304726, 1564051543, 1936521888, 1981976220, 1767096696, 516070970, 682155225}
,{1656478333, 938367637, 963468552, 579355364, 1545939228, 1754464591, 1530521951, 1224788687, 1868882464}
,{854309253, 1016981493, 1397679075, 1860509229, 1709149955, 2113837625, 1194706818, 1227613478, 1064736873}
,{403502657, 571592508, 1660265130, 916773041, 781964530, 108244031, 656012078, 472420449, 1582985014}
,{1786587529, 1969262123, 76232485, 411195143, 743324637, 162840482, 233986681, 436673987, 1342108018}
,{257593948, 296358305, 2138313523, 47167270, 1067411648, 1065193882, 1980244029, 320449501, 1062628505}
,{1162619597, 1392769274, 1685134533, 1210180229, 736897977, 663729868, 164338891, 186293976, 242198356}
,{1242943523, 1643322426, 2126792827, 781892554, 1095945640, 1530414740, 1591147668, 1083984329, 1682918766}
,{1984571918, 965085189, 1052495235, 436208962, 41632156, 55736653, 281368130, 1411471940, 1695100624}
,{1644349700, 766931040, 475492806, 884232419, 1083784872, 191408645, 945370692, 1841842865, 976010781}
,{206936455, 2140092514, 1539024672, 1358156901, 498646637, 1530793843, 1325369374, 250032526, 1055380347}
,{980875179, 814382915, 1450045674, 822743461, 1703689877, 363702420, 1322749827, 1111400298, 1099951408}
,{1808601811, 974357842, 2132812671, 839229540, 1202809978, 1522463214, 955705452, 1793222820, 575181991}
,{1923396953, 2015177087, 1726477436, 872091206, 2103387400, 590368877, 1197725782, 667635547, 473850588}
,{1424122035, 1738656968, 1694703567, 565424182, 129617386, 703343344, 1878030126, 552358732, 1224782018}
,{1208788459, 1634298789, 1366067376, 1139729807, 635332550, 1103848646, 1623965444, 1000838631, 1466155035}
,{2142176669, 1730570839, 24090258, 68783503, 1409144156, 1947176646, 2134307173, 717640363, 1480092576}
,{1272516130, 265166903, 1548004215, 1821210346, 1577465430, 893528081, 1342643569, 170169091, 551538607}
,{962730392, 532463400, 605113504, 1679747543, 241821752, 1483997951, 1266830227, 1987976240, 1693513715}
,{273527163, 1333082763, 1388888427, 1142494874, 962255772, 1559470803, 1165196193, 2130540577, 2041927670}
,{606954392, 136640759, 1813320939, 1842501640, 1581437742, 1212696910, 315135197, 1410081774, 443664027}
,{1351910748, 1838322085, 1261381362, 448513022, 447944140, 76663135, 697678353, 1600189969, 854245292}
,{806008417, 842939634, 1134622161, 334753166, 217837324, 238714141, 1158446394, 1432535481, 1971307735}
,{1124641897, 1423632689, 1276669320, 449118283, 258274760, 707885478, 814161038, 2096897202, 1886592751}
,{1500808344, 57401737, 483002986, 1061671257, 273799836, 2109099193, 1140682639, 555058151, 1260645489}
,{1233251419, 1859138784, 987402314, 2124395307, 974583289, 1320126605, 1062228914, 1108370093, 1982518127}
,{2085761726, 298150413, 500689843, 1583578945, 335253507, 212378903, 748479774, 677365930, 1171917255}
,{1520101139, 128925241, 1285468319, 1138507044, 1764629093, 1636999307, 1464137838, 411310436, 1191173711}
,{1181727236, 320402870, 1998412667, 901930693, 851105791, 1624785255, 892866916, 159930543, 1117252648}
,{612437270, 813852641, 1674160834, 607340654, 106179468, 1673241420, 1088526337, 2051717536, 1106065248}
,{259299661, 396225387, 1383674226, 431194328, 1660563502, 412844688, 2098172044, 126330842, 1831096072}
,{140667663, 1981982315, 411913397, 1198119490, 502805728, 107387533, 1770192131, 1759668250, 960663664}
,{1914060477, 2079098059, 1046429582, 1868127660, 1328382868, 1087054345, 1202831473, 695517368, 1943945248}
,{1878991998, 1289747012, 216805625, 2094455703, 1271904144, 417351448, 929726077, 1446929029, 1731846625}
} /* End of byte 2 */
,/* Byte 3 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{1508012513, 2017431394, 364496407, 468402026, 1594355142, 738007738, 2038928981, 2009634712, 31989534}
,{598133793, 1796514457, 1581669433, 335560125, 1089786967, 835548353, 537608233, 1543261614, 2052883955}
,{553642099, 555313442, 1833568133, 2030403321, 365626859, 254164525, 133997814, 1339258992, 1185376400}
,{1186955238, 719854390, 2131409688, 206683826, 178478510, 89671692, 1241031137, 1597684028, 478282559}
,{1929548276, 1508588234, 1917917787, 949267285, 1522736265, 403749608, 488115236, 348930223, 1722733234}
,{2040820581, 306610133, 248429574, 1885326886, 1801077327, 1847180759, 1642025773, 558998589, 424311815}
,{1104561090, 1302420874, 1876321703, 1951393463, 935145465, 1989566316, 750476763, 2010849143, 553470812}
,{1799823648, 126849880, 580491968, 1825467117, 362792278, 355905062, 670673561, 1698105553, 1164319705}
,{1623871533, 1010078678, 1228793193, 613051372, 281165259, 1837588012, 1874127577, 1782467820, 1804785169}
,{834976593, 596587738, 1398503430, 1136946059, 101805234, 1700210301, 910804007, 1569783933, 948245228}
,{296465561, 458030599, 994586473, 303722641, 916851446, 461641835, 296140919, 92699137, 562284201}
,{140393748, 255646822, 25850420, 1683382737, 1456409330, 1065502469, 619292197, 1144507921, 1163205740}
,{732729554, 1012892054, 735339772, 490003985, 1092976204, 112191616, 266575977, 74579846, 1711118352}
,{1410834007, 278969045, 1587020442, 542501577, 1355323411, 536796419, 1291767513, 1486247318, 620388485}
,{1889406890, 652996057, 1118800996, 2051195787, 370301760, 1065445469, 1712262723, 1774026381, 2004250356}
,{423681515, 206237420, 28633995, 670066325, 1317505971, 1668561625, 2128991985, 461790042, 1370627821}
,{1126603873, 1677269140, 1162425614, 656931474, 1776025067, 531338508, 1835450595, 1625136501, 1314601474}
,{284857799, 1206025074, 1055398737, 716918519, 712087226, 998950662, 920514370, 1468480313, 1593983487}
,{293748584, 1683693168, 1215223307, 111163627, 49661888, 2010377893, 1907455415, 1054166967, 432322726}
,{338789418, 288746733, 1774405266, 964196526, 425994225, 2102905719, 931014082, 853089962, 342161103}
,{42615987, 1469476379, 21554181, 357747255, 767108514, 58696413, 1396874807, 620460349, 1516315033}
,{2006543520, 544758069, 773887134, 326882602, 2053622741, 1989951484, 506900844, 578964036, 1421488599}
,{953216565, 129440452, 1933276544, 289819843, 1872965183, 332685566, 313598043, 502183075, 1160330195}
,{770498223, 1605057535, 518964655, 2046355876, 940264637, 1657505704, 368334388, 628948652, 2008622797}
,{1026052687, 1032660109, 967854055, 585582437, 433338563, 1425160945, 1005918636, 1951497832, 1038804510}
,{445121449, 53504038, 144389967, 775421534, 1725205494, 864079172, 1130452445, 916634090, 330641739}
,{1959964493, 1062537791, 230587153, 14141235, 2135732873, 1772414589, 576609688, 671321303, 75409317}
,{471161438, 1078191166, 1569848781, 1944042241, 1519822739, 698536298, 1110612712, 696334333, 524486446}
,{1557262229, 1239288581, 509619013, 1169055489, 190546694, 1660614442, 143152323, 1441074723, 8293088}
,{733522654, 563940157, 1586180799, 1421658749, 1248294110, 1810445781, 1059898958, 835945749, 1922080739}
,{2042333112, 1121522760, 894104384, 475607264, 825377670, 1695196359, 193411476, 2125900165, 618183524}
,{857461129, 1288056933, 347872062, 1253270629, 922086808, 1954313179, 1703971914, 12635365, 1454535342}
,{187131489, 192762230, 712390497, 1755556693, 715080663, 1093633850, 1403360473, 512761251, 1716730231}
,{2123850889, 1072400251, 1241410826, 1603037952, 2037432735, 1295420081, 895387980, 1289652159, 1478680297}
,{880126030, 252146281, 143165443, 218194251, 1073273876, 1017843994, 1290158598, 1699374024, 907989697}
,{625313978, 1095727766, 1358507687, 1824663138, 1189571897, 662119509, 2093133500, 1092066413, 55729376}
,{982415943, 1020675533, 512606198, 1157974139, 606064527, 998952500, 162972823, 1423056198, 2040730140}
,{2116377503, 1497129330, 1759107941, 636700496, 1978619864, 1078519751, 415258387, 1242979337, 1221454549}
,{1355380784, 538107060, 29344912, 1746714846, 384865425, 493220515, 1467330467, 1984768459, 1730305929}
,{488728680, 1297885483, 69444926, 1025843808, 819610319, 1126830445, 211925182, 1166418359, 303989711}
,{708725502, 2068823443, 490137380, 25260631, 989634317, 2035517882, 2139938133, 672146655, 217512076}
,{1179181232, 643541765, 1503711004, 191429836, 1880157623, 1519318522, 1186697886, 420330670, 659300065}
,{1653445860, 1322409571, 1458865960, 1084703557, 875413955, 743703520, 1198569728, 1524197611, 1523060797}
,{1186818631, 54990040, 236855204, 1640277521, 1350249017, 820719525, 486474282, 1684759887, 1512202333}
,{68413368, 1208000715, 1926315293, 1267171806, 2130513509, 2043049495, 674377058, 816391471, 878408853}
,{1763045311, 208106143, 480288451, 61747738, 1740861350, 772156290, 1283775573, 1008290658, 1173211255}
,{1167081324, 821033012, 1273588105, 2005698546, 497788225, 842051278, 764121793, 986636642, 367213765}
,{1050916699, 979722033, 923946137, 1581220652, 829510151, 504527006, 1391606779, 986784097, 162013170}
,{2047357415, 484062844, 1603755456, 467008733, 520192708, 1378100484, 1550917016, 1642174695, 1556044300}
,{669855080, 1012699297, 644309784, 1680920569, 1472886925, 778250019, 264548095, 1051457824, 1534373416}
,{2073246366, 733742807, 1028224097, 1803878572, 540698844, 652351673, 48281735, 1250109053, 1499025046}
,{1241215680, 877973808, 1249644245, 1442885226, 438250115, 443548320, 1094402333, 308225133, 1726235689}
,{1279784079, 877861792, 713246437, 517791822, 706467772, 322563182, 815097688, 1091501090, 1398184214}
,{1305695356, 987385976, 1925127739, 685388205, 461672137, 100895872, 1030820725, 861471863, 482279532}
,{1854936624, 696131437, 1282781060, 247960455, 655027726, 476400348, 1975034397, 1382059516, 1889293813}
,{1949393078, 2042737963, 1347705107, 1365396267, 1256182872, 1145702021, 1054083582, 1211215378, 617509125}
,{1816941325, 225062648, 487759523, 379977651, 1453637747, 126004096, 14161360, 375021175, 785509424}
,{1501956370, 1989708259, 1964408679, 1815047216, 310004659, 390281706, 324571529, 520331175, 1738501483}
,{1396612712, 152893057, 461862991, 1159606488, 565285496, 952568796, 1846450830, 1587090521, 117177996}
,{1386200424, 124770259, 436744253, 381616129, 936554301, 69096840, 412560359, 129845821, 998482446}
,{1084989172, 1950179577, 1828723708, 107219927, 93702804, 711439908, 291229552, 1293779772, 929370072}
,{40833562, 1597957745, 1034472750, 673816629, 893202668, 1798414363, 727150270, 1493055858, 155884753}
,{995938568, 870294468, 1292320087, 760302943, 308571874, 1380709428, 1668065246, 820725201, 128692236}
,{1556417358, 1130429024, 958921915, 1080036356, 1516479239, 370934584, 1258591540, 639529317, 970521431}
,{1901767270, 439334930, 382902460, 576154466, 1914928151, 847458209, 1052659728, 1066717020, 1541577194}
,{222741713, 1774704466, 1719611005, 1936684748, 1647477707, 945125590, 315075225, 215795684, 2001445947}
,{238503438, 582970284, 319062699, 1737262353, 1940428241, 284745716, 1937388616, 629679608, 431550190}
,{909904009, 654478934, 2098781166, 1213981788, 1948069550, 1302380172, 783954647, 1691216938, 1141395257}
,{79652426, 742918734, 2066714035, 2097665234, 1055475194, 1359285479, 1483589935, 1041537845, 1227807860}
,{250877579, 1242999246, 44825127, 1610612474, 711356473, 1047817911, 859204864, 1739387501, 209945030}
,{818860503, 578086143, 837394010, 1751274560, 1970648634, 613607514, 2125869933, 255600807, 485323981}
,{2014339809, 514736920, 1309116625, 1055197657, 1244419, 776085628, 1896480114, 1894668170, 411852383}
,{1553791375, 1765313574, 588546148, 735827793, 859680417, 684615681, 456304842, 790869463, 904998652}
,{1764814146, 1091596015, 2013889650, 1429743861, 1734788119, 1118156158, 1407786630, 2010760364, 1460518721}
,{1974064705, 1788625484, 186719417, 230191784, 1769905321, 1634573749, 580986246, 671091692, 676029770}
,{832022621, 367164518, 2054894210, 2016759397, 1478704765, 183330783, 165367336, 1529384100, 943420330}
,{69893121, 1163842894, 39048632, 1832964604, 1902137207, 1246754484, 32181292, 962707693, 813570238}
,{1403482538, 432458233, 1629249677, 1467035925, 1528164584, 1873974627, 1724977834, 219048589, 270381572}
,{1031637859, 1419504601, 373786862, 133506481, 146429337, 951057168, 899910312, 1107028060, 1906453163}
,{983613807, 1296758057, 1595794782, 1209644490, 609506090, 1704112501, 1106729589, 1788656775, 1594088954}
,{1465832961, 543606247, 1781973916, 166629437, 3225815, 137938893, 982076027, 574522104, 231639808}
,{1504201775, 101379373, 733884340, 2076386514, 1505474502, 1185521487, 1070571096, 1272189901, 1167582219}
,{1528987338, 1956501387, 329312909, 1023066267, 1743790892, 1206607454, 1410238853, 1925347039, 962545246}
,{1116563350, 1249104498, 1596278866, 1699929146, 721323881, 1932049149, 1886429305, 1576549801, 276905633}
,{555104553, 578897528, 1266695731, 2072297537, 690157276, 1780475305, 1180400286, 155918884, 1065082396}
,{245088049, 402967616, 1889416457, 746097022, 113859496, 1283554440, 1440172403, 564159228, 775397225}
,{452378721, 1011967142, 1178694102, 8165194, 748931462, 961008772, 2130831220, 1409857914, 1758719745}
,{1360189021, 155847922, 1250088436, 1527391930, 323563952, 1934639594, 2076203131, 1981836840, 1777404705}
,{1121244026, 1658173336, 1304812492, 567959803, 1303875490, 1909983021, 743692617, 2133028487, 1308934796}
,{1257748109, 493422611, 309341189, 1122451738, 1632948835, 1019578414, 1455727297, 290824637, 1209037665}
,{959440536, 1247234430, 1772923732, 1870040742, 1122614486, 481933513, 852627726, 68382485, 1635551528}
,{2019193531, 647333922, 1510267480, 350411833, 231263990, 1121732192, 258667754, 1763116672, 902663343}
,{1985231014, 47449135, 1290457419, 966300248, 1999953508, 855856833, 239288251, 2063665366, 1921915724}
,{49104666, 925933005, 1267853788, 1079223113, 1860454490, 1552991466, 1368551869, 1566597600, 1312145330}
,{679677099, 1923206612, 1803112678, 2004128986, 1907407075, 1577945445, 1787702684, 1807929776, 845134924}
,{2035713883, 1193330332, 2035387356, 261707423, 14051361, 802953020, 1754353799, 1687793058, 1356059073}
,{2146009395, 2003639128, 1938516805, 1437818919, 760793052, 715664928, 399543892, 438985098, 392399903}
,{1982879095, 295436112, 1416465543, 691522414, 648583599, 1212041164, 1421747865, 730781573, 1788674419}
,{2023584684, 1781976636, 2000158466, 852281829, 1991601011, 1503218726, 2118277006, 755129971, 760937880}
,{447623595, 1487319019, 2140118521, 762066851, 115664450, 793779849, 681179568, 1382647461, 1886595668}
,{363297372, 1746875824, 327716568, 1317092179, 450430765, 760728285, 598332580, 1107675776, 1780731161}
,{925922418, 332422227, 867108032, 256939825, 322905963, 1941084663, 509850947, 830111270, 1106108842}
,{1042988326, 1163168642, 16979591, 1635335579, 704726315, 1442367822, 1874382865, 1516172488, 1033059403}
,{1555504378, 908892372, 1626468383, 1562995765, 2076178296, 149717475, 1037196355, 663106129, 662975145}
,{1586277845, 597074179, 677537342, 505339596, 583827478, 578747845, 1011846508, 1010186777, 2055623206}
,{1724680834, 71071699, 925007399, 101425960, 580328257, 970671960, 585272213, 1292445056, 1845959405}
,{1390907991, 1437730933, 287398809, 1609319939, 2038028953, 2132029142, 1903420621, 1050496904, 2099851766}
,{1771630422, 1335178929, 1272985708, 970594903, 2089031380, 929856646, 1591780976, 2131458479, 1235646730}
,{843790178, 1131243261, 144918539, 1385157973, 1321348024, 621168334, 1430748175, 347525200, 1199714204}
,{1043855188, 1909513271, 1194603947, 1125567580, 1192335098, 255673767, 1856737405, 1012580400, 837931958}
,{646125729, 2077101035, 146314571, 120562807, 2067429992, 669841024, 341366942, 215832742, 59153250}
,{1785246708, 1293241268, 1410735885, 620468351, 690803651, 1527398379, 768772270, 1425139880, 192004693}
,{729848103, 1247646684, 1745437355, 555285595, 2119221916, 1314322395, 748553605, 830162745, 218052220}
,{961931046, 1607355355, 1724297609, 591724895, 1162507213, 1914924583, 1376196552, 1398546290, 1746757835}
,{2074055904, 1920866919, 1138328161, 971590554, 1413228586, 1624444082, 784521097, 374885623, 1604141165}
,{149739801, 1169816725, 1671855956, 556765055, 1097553080, 1224781977, 921059717, 2059755254, 1879877294}
,{1406847028, 894226509, 1033879055, 137096324, 1556147953, 1260823508, 1451584286, 273286918, 1245749167}
,{1001818526, 1019599561, 1117515580, 1928786223, 57657777, 1313208171, 1839008726, 1929378263, 1851981579}
,{154643227, 1715826200, 1409752642, 1976055500, 88369956, 265836700, 1316709797, 1193084524, 100287102}
,{757852454, 183894370, 152773190, 483234222, 778526005, 1421884564, 84610322, 668368974, 2047835417}
,{1060647298, 816110676, 347441401, 140510155, 2078319833, 230135757, 2092254986, 52466004, 341969814}
,{1530765360, 1885553672, 1829135438, 1610463062, 1059960714, 1349785796, 927213680, 1285938274, 2113120871}
,{1507285736, 1103221203, 1299517768, 1827098779, 990862420, 146174466, 1892601799, 1057313967, 1372421808}
,{1213138218, 451695445, 1065285334, 1616255466, 1746553260, 1959832351, 1560460017, 747971913, 733143814}
,{69580334, 1976930360, 1115754973, 663980085, 797783505, 1308332149, 1624206362, 310256223, 1011316374}
,{1977975585, 1577416503, 300618796, 113300864, 1392294261, 1421995960, 1950403857, 1256020614, 308441952}
,{2083764402, 70131269, 855991557, 1100692926, 656140236, 1703045740, 259925548, 1868712443, 324213428}
,{243069951, 1507484579, 534513639, 1556325013, 1290522170, 456042455, 1985511515, 324525071, 210570754}
,{1063773589, 1067112412, 763717142, 1958476290, 667838647, 1661159297, 2007672488, 2114784378, 1860399397}
,{1371471461, 1921613555, 754433536, 712396962, 1098430008, 1548972726, 856538176, 2130595518, 137973212}
,{1854740785, 1098596671, 1776140662, 46741987, 941711622, 986308308, 142442566, 641132183, 1039437905}
,{784949080, 1948252595, 1233191459, 1245437642, 974009985, 1673520417, 325336766, 1775814868, 1211468240}
,{959286487, 2101239328, 2113680692, 210447972, 548059106, 369624687, 588447681, 111865466, 1153151287}
,{1701831810, 2068166090, 967654836, 379643417, 316543144, 398783133, 76461463, 675004452, 1368781774}
,{1662604856, 1711707640, 108817062, 1233011079, 1860174652, 700299211, 1044828344, 1296817888, 117798203}
,{871273720, 1414224741, 258744138, 1172054008, 631763561, 1109278939, 1910605974, 2006580470, 1003202220}
,{1776623249, 672925278, 1241909462, 2133223200, 1169941474, 1017474782, 1100541653, 996414345, 1358067365}
,{840964405, 1209694558, 100376239, 1842880845, 1281500384, 1452227928, 451089923, 2007121919, 471633530}
,{863799992, 255302404, 1771084823, 1857640009, 1474434473, 1241462595, 799702003, 1899866855, 272610162}
,{1599751277, 950849911, 78883161, 391483457, 2034250646, 68814417, 1880159215, 1344040467, 167972112}
,{458887656, 1260682967, 1994358033, 1712748741, 402741008, 670575558, 1544908223, 1494153410, 953529138}
,{1131225800, 1637311845, 1414176653, 1687556353, 478483194, 1241382815, 442821756, 1103072009, 1058775632}
,{2050496240, 1874603642, 727957592, 1960634440, 562992515, 1156929958, 108283742, 2137483495, 1425486027}
,{1577990908, 930164774, 1627588075, 1956603540, 1539519426, 1236861415, 657815668, 437877868, 97771984}
,{1760913265, 512133529, 1772205630, 1755647298, 521464007, 432684798, 1406017586, 1907786237, 1679099924}
,{2079469003, 254514925, 998480411, 573373244, 1739638420, 1996306968, 1158766431, 1348920999, 1160141888}
,{628574269, 49582278, 230905030, 1508193114, 1290009079, 795948925, 489193818, 1122226913, 145610383}
,{486037026, 1385548144, 232676547, 1681290107, 1986698868, 1859680279, 1621897914, 469380743, 2144538155}
,{1090448160, 1856526419, 1612748693, 211214348, 1121420386, 1474869563, 1418666893, 1110545306, 1376974623}
,{1478306805, 1597739196, 595926766, 2103010338, 853658283, 1617202726, 674150906, 363818968, 1585315103}
,{1120219166, 129122958, 1833006106, 1587642712, 305520143, 381291344, 2129050865, 1379345821, 2077606935}
,{871111006, 503215372, 2006585089, 1955119213, 346792528, 842571614, 1067439930, 507773414, 1672310386}
,{664438365, 168675054, 348077682, 627928691, 1164436829, 472992538, 2115265576, 1605572657, 280363852}
,{849854585, 758856712, 738250221, 1526046963, 625951454, 998278519, 281478557, 1105406921, 346433214}
,{936259196, 2065620391, 1779733659, 1524064583, 48358768, 1308432943, 1643904676, 519201217, 1691999442}
,{1200956416, 16093358, 1117318033, 2046692097, 729195383, 1313745530, 1731248921, 1548129157, 2012711278}
,{603786854, 1660946243, 591744902, 1253208034, 277599055, 805375750, 1383605213, 17326933, 1667398999}
,{1543180169, 1138158466, 730462859, 528245946, 1438333190, 1564413048, 1041911751, 1344692839, 860316514}
,{1277147832, 1267922964, 1927321833, 2091280456, 2035348593, 2109411411, 222660198, 1798601022, 1620586600}
,{599785944, 1399731896, 1703883698, 2011385173, 643159873, 1619303545, 1673998369, 244369738, 342882100}
,{1537603832, 2008510733, 161391348, 1163219270, 1960985641, 611355933, 1343281142, 907819235, 1106377997}
,{133162806, 126783223, 556187282, 1238681006, 245756348, 724421335, 1913252550, 1642784437, 1496142895}
,{184282810, 2120227687, 1221316859, 745299887, 1557653755, 981986880, 877621451, 1619248385, 1527787732}
,{1908901370, 1100214634, 1052314665, 1353164819, 256619194, 1505840978, 310521958, 1029733769, 862580016}
,{1573139274, 1026327941, 1821528216, 254982509, 700497258, 1978967748, 1987882963, 2028829666, 634651969}
,{966177875, 24718251, 1958456261, 1888496204, 1292059383, 1297168730, 430705382, 1952497685, 1145556105}
,{89254944, 1713499512, 202149886, 1221104079, 1449950897, 2111757011, 644150938, 780902886, 1820879981}
,{286690905, 2072548395, 1248347559, 791657663, 1717385570, 361593843, 1443912261, 393662554, 935358582}
,{1595826258, 2054081925, 1770210233, 513253344, 2052277171, 1280794370, 248190380, 426386711, 936674181}
,{901394767, 218582191, 126030575, 1203304657, 505548618, 654713299, 1936990385, 1062228934, 1637007044}
,{1215983659, 1515307232, 1924176392, 622105340, 247919290, 982338769, 399241817, 160248157, 2017193535}
,{369998399, 810485751, 1497823693, 1540074916, 439993534, 573612129, 115943442, 1714243076, 1557995375}
,{3341087, 1123127777, 1034669052, 503682976, 1354883694, 1873577673, 1774781237, 1707870643, 1079251516}
,{192464878, 1105641041, 219912432, 1755392505, 877057202, 823529969, 898044584, 1870573630, 1296098255}
,{1704657370, 502331548, 5876156, 697619803, 1160016497, 979443292, 951561192, 1653063574, 2084682799}
,{1655302196, 1526668562, 513873720, 414717180, 1235521042, 517816064, 762134827, 899589267, 1672284075}
,{344439600, 1629785064, 2143114628, 622255617, 2024650726, 1509231204, 2044380436, 1624709382, 1670750807}
,{2116685005, 689087100, 1304433295, 2061368175, 1523910070, 485118260, 1134641904, 2093740238, 576352622}
,{1965967044, 1764805926, 1085518421, 1898311603, 1100852171, 1506214190, 1632085278, 1112333391, 1892651490}
,{1665523749, 714261032, 1885002820, 1660858254, 1052104144, 1097695361, 1608183626, 1742239079, 334946284}
,{1307680765, 300213111, 231071281, 48978340, 1196410600, 379395755, 1708895502, 1813096926, 1206071770}
,{2125811071, 1309588810, 923891322, 1302258369, 2115149043, 1746670941, 419930675, 618411721, 2006079123}
,{273270858, 1863533602, 1863367840, 879224307, 349978969, 1553140437, 997646140, 530048158, 2097197396}
,{451276764, 1203825115, 427396988, 488257918, 1387202146, 1475374977, 1736376846, 524771116, 774035526}
,{556080046, 1853164386, 2073277328, 590875040, 836331545, 155297817, 1696800545, 533865761, 756852711}
,{529616136, 496690091, 1732850368, 1770273995, 206184545, 2029311361, 1129965574, 730540950, 1024237036}
,{1691561466, 271540482, 1024851371, 1408453126, 888116902, 323739063, 1233665246, 1620915384, 750293247}
,{2020035689, 647066965, 2056303706, 125411428, 1412853678, 906422115, 1202185054, 1207885212, 131100395}
,{1474275320, 1136871715, 1165400596, 775282527, 1608939273, 620875353, 196474946, 1350761390, 62430963}
,{2090409561, 2013491889, 1589742963, 191812888, 1778873135, 1389222725, 1640385921, 1872535645, 1660356571}
,{1213516651, 189842843, 847811218, 1739155025, 263443419, 946228021, 624005593, 769133791, 1175362598}
,{1746284579, 906872655, 441127152, 35116706, 1610510454, 188298870, 1448964371, 709095182, 1577075505}
,{887654762, 1807658057, 282052655, 367403652, 48453469, 1711727255, 1293560851, 1095773360, 1812416440}
,{1269400835, 1507783276, 775511620, 1592473923, 3146370, 526622072, 1879684083, 1256707041, 1818156735}
,{1366425196, 1423718275, 1991268559, 1100649815, 2093850412, 515972727, 414813734, 311219914, 1340678348}
,{1349236011, 1587405511, 1941183801, 2002695801, 1908716473, 148805266, 385863405, 257030874, 1174510573}
,{678855630, 1306412544, 1404973248, 2126119767, 827324224, 2142554897, 332956487, 613720626, 1256767099}
,{105961956, 1790077187, 1783828282, 513654377, 1731022264, 1323493773, 568986185, 851380124, 1862535950}
,{1156494375, 149195865, 411302678, 1337352234, 1058721303, 1355035569, 903407870, 2094901303, 647628394}
,{1940096492, 1895638126, 2011389157, 1498578134, 379086120, 357610895, 1349354432, 167310624, 750024131}
,{1569088760, 622764809, 1070182384, 2109613759, 1510620885, 73583743, 1366550621, 1738695224, 1331574286}
,{1988052620, 121670433, 957250362, 454697897, 2102146410, 1573335663, 728705427, 951270274, 1124956720}
,{1659136074, 789453639, 1156241047, 1749928394, 1159887375, 1711994566, 1201288043, 1651600126, 1226072425}
,{1679926512, 765368708, 1242717033, 322562451, 1928735036, 1465884617, 407534870, 1253667070, 416209738}
,{1975205727, 637902831, 143536581, 1845765078, 1124436491, 698200103, 221289092, 909320437, 484051791}
,{2119787075, 1174005348, 277459124, 407729102, 1734185630, 2042692607, 314135988, 1156774965, 1866185304}
,{129807727, 1459285211, 1917995149, 1324433446, 363285745, 17988136, 1482202572, 2094152734, 1133570841}
,{1378382226, 941370906, 1344885063, 1879556449, 363236628, 1103182221, 1066145468, 217915347, 1038585533}
,{668080986, 232448554, 521607540, 736353556, 1376548352, 642865303, 618727231, 1700915031, 1962344097}
,{84209338, 1959343104, 75342863, 2016852873, 2015052669, 1715121471, 656166354, 1142949555, 1078149413}
,{1871221330, 288557878, 2078839980, 1817066839, 595028856, 707372236, 1401762968, 37959612, 1870233180}
,{1931059499, 1003828083, 1005505696, 808089770, 963798221, 220583076, 498804890, 1707249882, 581009127}
,{561446413, 1251028442, 273961443, 1310347855, 1928308350, 302902921, 275319603, 1536108541, 1498044543}
,{1543164597, 2065148331, 821152089, 1502395982, 1469299448, 1202918175, 2010743463, 1120758317, 754911490}
,{237193077, 2061482860, 972214705, 153694266, 658300022, 1656342736, 2110481665, 2099380242, 138265302}
,{2054404431, 136682643, 1006726387, 1813256337, 996111722, 598757098, 582413038, 1394663409, 1474347170}
,{1921617460, 1284274852, 1964160301, 1899597819, 98722628, 611708741, 1857436424, 1678991800, 1658559145}
,{2085453120, 1835073069, 971971959, 1430634802, 915403745, 1105164477, 263041845, 1875441068, 1117992830}
,{1102000079, 2052472050, 852130887, 1841972927, 11395122, 1559397840, 306341643, 2056875899, 1331598022}
,{56172055, 1237408468, 1433225114, 69319009, 649180259, 680877337, 872795499, 1803950049, 984165383}
,{311328604, 1452428348, 1346660675, 1914318707, 1691488713, 1772960066, 1370369746, 2078934390, 2037603500}
,{2145152017, 1918093050, 1590203816, 1426722499, 1292600963, 845167297, 972272715, 549556137, 778133768}
,{1334020641, 1330490167, 415074363, 825884872, 143218189, 970957125, 612110281, 719020456, 1609547947}
,{1658115575, 1134592835, 880102981, 450637620, 195855087, 17381472, 15026007, 1196997794, 1280144412}
,{663174881, 265833856, 1680935819, 2130190184, 610642085, 141096830, 557810105, 324343912, 1785611904}
,{2018279184, 207692704, 644976104, 1180445315, 1566570653, 2078275366, 561824820, 983806628, 2081231792}
,{1087173406, 1399083458, 1966874287, 1328315327, 134597087, 1662817860, 1384888036, 414107785, 526530274}
,{1209453076, 201187741, 1996479007, 1114271947, 1559148513, 1495866082, 1687674243, 845708509, 82034529}
,{876273439, 1836493496, 1214217259, 2057159457, 1852710599, 1422423610, 1790110819, 1323246488, 101084140}
,{1741002523, 556150740, 379603784, 1564895738, 1745211096, 268922404, 714977672, 455660094, 941484969}
,{1610151946, 678934752, 108564166, 1297239682, 659178430, 759626665, 379565745, 1701262583, 385859073}
,{474314203, 151699629, 940081728, 1346442429, 37284057, 307812205, 1369218368, 1726209459, 1473220211}
,{700164003, 2051690675, 1442752759, 1656387584, 1804566337, 2128755821, 805062404, 1804112472, 314133513}
,{1122439277, 1923881463, 2051624782, 1945566603, 1363456642, 1208905169, 447391967, 393115044, 1655363106}
,{555441368, 511948529, 596367128, 544057753, 925481081, 1176537509, 1150946641, 248600611, 2019586873}
,{929728346, 854958682, 1053751746, 301342747, 297658467, 1173798855, 106051235, 1874837550, 715207148}
,{237582742, 197913325, 1624811193, 966273430, 501509356, 329507730, 435741664, 1243260577, 1891729254}
,{1441495293, 1384307839, 650484464, 671189984, 1970858864, 128165210, 1679368464, 512907751, 824973510}
,{1338603333, 929700192, 2012865622, 1587786631, 2084408982, 1857053396, 933400204, 1690593289, 311585325}
,{1517334878, 385836630, 1917138419, 661284533, 557301042, 1787504705, 1828382684, 450847323, 721627026}
,{390814732, 1232627239, 1059164892, 1296139599, 56305491, 236848624, 1405764603, 1126244535, 1272732639}
,{573587485, 1218772158, 428119056, 2077251222, 602254042, 1554887450, 957650272, 883080168, 1152169442}
,{620705002, 1252810540, 1940374418, 1072867472, 838560314, 1250312154, 858520070, 1846127271, 1373531775}
,{1805857878, 1181420116, 1521954774, 346314702, 376169742, 334621494, 760592157, 1905468547, 460875027}
,{873268978, 1571389378, 1572710893, 564957283, 1914884672, 1050996871, 238494347, 1233103538, 1268550069}
,{1270076131, 2100831169, 718515931, 1797301714, 11496774, 689936882, 691786387, 159312108, 814730995}
,{609744906, 1449304161, 1222895585, 413672386, 1484574608, 1880138538, 350098732, 1579628018, 716956437}
,{1723013584, 1848126772, 1759816143, 1616430190, 1587957625, 1674568747, 1925336358, 727814100, 1333222690}
,{1498995164, 298821597, 1495333560, 306176852, 77415961, 203670019, 1316674871, 1026456653, 1114020503}
,{300609700, 1237688330, 1588082200, 2049695466, 2023302541, 1132474935, 654845061, 1927873877, 1026424137}
,{1666388662, 1155697745, 1110173717, 1968591836, 799390960, 787210078, 619901825, 141013462, 964784613}
,{390754859, 523132428, 1633013679, 711798044, 558506716, 278994045, 2072236675, 1597127943, 321634483}
,{815424507, 123147612, 1489253669, 1543092371, 1411638139, 589368311, 316607119, 1324905697, 625798598}
,{2100725480, 1328906949, 1841550107, 839164007, 861880831, 593653031, 667403905, 1142443552, 1750515776}
,{882937771, 2142829562, 100914816, 1273535120, 556372921, 867433701, 2052170850, 1629017146, 630462014}
} /* End of byte 3 */
,/* Byte 4 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{2061690300, 280993994, 670311578, 1353395404, 775658222, 1706886036, 1790888383, 1048707017, 86488219}
,{1235905778, 1040255579, 242126504, 1619428425, 400690665, 392940192, 1393087625, 1668718668, 675632590}
,{975027115, 336106362, 1978250608, 2097875933, 2014765134, 1605459607, 20475375, 1553838640, 1470921610}
,{640840791, 644193035, 1718519095, 153740126, 1105836682, 1601014764, 1587660657, 1423419996, 178142954}
,{1579174447, 1112724687, 1137961733, 1398396852, 405143173, 1311632907, 1430509492, 38725071, 1572705404}
,{1594998117, 1997359956, 1942605217, 727257602, 252532250, 1116988068, 263169205, 1172019292, 1130815686}
,{1627139614, 1601167746, 1628398942, 2070007943, 205276521, 387423787, 96936246, 532841704, 448278654}
,{645596785, 836970474, 148251583, 565594342, 300207973, 1147882511, 927608710, 1314673117, 1665433482}
,{366822546, 82226669, 320165737, 1828912066, 1683698853, 598083872, 1399326570, 265405943, 1912929320}
,{1183118387, 2032522878, 1725052555, 982926438, 717138309, 222567072, 788160621, 814444403, 754205315}
,{1544488323, 1967270047, 1399656563, 812362074, 252797269, 1748821681, 894943157, 177009422, 405592336}
,{1713467194, 740840201, 1366021790, 403154430, 591914699, 1644365236, 360997228, 1135798859, 87010916}
,{1884560141, 556522894, 1885895938, 1314525354, 206036040, 4301027, 488526208, 2002534640, 1652632563}
,{1508096767, 2040870844, 1468445250, 592385397, 1070854878, 117172605, 1964305750, 978620208, 1587331264}
,{1278812801, 490494666, 1713299585, 96825831, 1290998836, 150284368, 372209868, 825545836, 572561064}
,{2024261390, 2142663074, 1483349074, 359724509, 279394285, 360429764, 1786695386, 378626218, 2079610391}
,{459714067, 790773344, 934551019, 1293589595, 817247398, 1381492697, 128906028, 2107465944, 509131047}
,{1016976246, 467770521, 79676065, 2101069837, 1694515293, 756073929, 781417831, 861205955, 856222297}
,{1320489300, 1296871414, 1834648994, 1486985809, 809301263, 834700159, 1435129756, 555193665, 1151854392}
,{1843481507, 1888843195, 1985798490, 156825006, 1820267619, 1995243944, 1716959839, 1595998527, 2013287411}
,{1488776010, 979244079, 1458673238, 560045478, 209882153, 1531914299, 815129051, 1455866316, 1418315860}
,{1313136200, 678345836, 461082824, 17559613, 371562032, 218748166, 1028766060, 1804975139, 1779930198}
,{1347264513, 2102767812, 1339656398, 958408081, 257673335, 879605956, 602297584, 1177100656, 1563675023}
,{1131074689, 1940998864, 79627382, 916728999, 2072029077, 364006717, 1777586131, 1475689859, 1448163444}
,{28698099, 1990498204, 1558148218, 109490938, 1471740917, 1873079687, 1654189604, 2107504953, 2129978554}
,{1180032600, 1817121533, 769605394, 1865564006, 596478276, 1404110123, 600289398, 1440714352, 1380718441}
,{184784247, 393920919, 931737212, 1321796828, 1851917312, 1080865709, 1519801855, 252995267, 367504598}
,{212155313, 889311085, 1126496771, 313685358, 1254522856, 63437449, 2070661305, 125824813, 458864895}
,{1155171517, 1259183249, 1711322342, 1368953554, 604579048, 532750671, 558491966, 1817726460, 2115858434}
,{1155135017, 855449260, 1122526881, 1602074447, 78633805, 129029457, 755244024, 1767235768, 319094745}
,{487172741, 129503444, 1202928190, 1808805572, 820697784, 66367167, 1931318958, 918095572, 1787854522}
,{2049214683, 414535663, 1454731976, 296213515, 1398123474, 19433071, 481562760, 127666549, 172324176}
,{1612995154, 784060240, 1751870334, 1792311206, 950323973, 1722750489, 2115275304, 1481461579, 324438238}
,{156920605, 110806737, 1769427884, 147227969, 32907640, 592755437, 1239840934, 1216522195, 1217737607}
,{605652804, 455120060, 1958689268, 1643719438, 2051508090, 672638308, 434775672, 632554494, 1185868923}
,{1548574457, 646334283, 1656917554, 2105058580, 603425119, 1929694323, 1303647088, 360291911, 1658438956}
,{1842771022, 148391234, 22668844, 955476198, 1564867304, 1865653388, 228683617, 553314385, 1908078181}
,{521643094, 430774797, 1865378146, 1059231350, 870853454, 1751438267, 2003994029, 1324201081, 514757258}
,{213528470, 1466568156, 259184982, 463902254, 1874789101, 318091368, 1006846462, 326254357, 1438477529}
,{396323802, 540178107, 122745173, 1003464217, 634220937, 616213141, 14444756, 568911823, 757224346}
,{185974220, 1682473366, 67684966, 944547051, 781253676, 1617382389, 467968771, 1961873987, 846217057}
,{1297425995, 1896232027, 917574866, 384577405, 324452809, 905262017, 278610613, 1682928061, 1990285817}
,{867472432, 62390819, 1862429337, 1587578643, 1782264244, 1047265656, 1066389628, 2073335732, 197603279}
,{1024464944, 1983831435, 588502204, 1182126472, 1625143802, 2054765434, 1372572855, 164855061, 1674818713}
,{1473507435, 1252480237, 453907400, 1751690694, 1425908263, 199881723, 706071344, 1934062453, 2144850775}
,{220702332, 1969266411, 1944664178, 769366245, 1300006268, 1033027136, 1153607951, 1546719468, 1617647913}
,{1257509405, 233086725, 1506710854, 705126839, 587500376, 329829345, 1928141495, 1640890051, 1194304864}
,{1401754326, 1207973950, 1208116912, 867586474, 599186118, 1361605030, 1805296975, 1846345268, 811958733}
,{5988062, 342857813, 1235838394, 1404990389, 2090953597, 555510356, 611913040, 1190820825, 63553573}
,{639421261, 1457399762, 147483181, 231201964, 907778976, 1340976503, 263952, 1519599769, 1342286698}
,{1020789843, 820388082, 844674179, 744074467, 1802037787, 785234184, 999338944, 823802566, 10468336}
,{981579068, 1595176336, 287790840, 1463865472, 426674735, 2110617121, 1803932677, 510366488, 1107039923}
,{722868043, 632586491, 1947571353, 1791969219, 1717118309, 701705223, 2024097072, 224541785, 27915949}
,{1409628975, 2075637494, 746423628, 674855423, 1367113118, 1045396441, 487909056, 74590524, 1770503164}
,{1550373143, 1625866570, 55650567, 1920589897, 1888755448, 1405358517, 1389136186, 2078637326, 1805477478}
,{1892449238, 448139364, 1113275502, 1651820997, 1214645833, 1718517413, 1907658778, 769392060, 945358612}
,{290398284, 1127835, 1266419457, 1879352938, 13284784, 328985165, 1688145561, 1395876174, 1027966676}
,{1528796395, 174842846, 1723217717, 1098721516, 383167836, 1250376843, 1607708369, 1502391228, 716324365}
,{70987099, 1799809506, 272441177, 184843076, 2086948208, 1667589839, 1728894198, 1512465389, 376469316}
,{1555381260, 940133819, 2111542308, 850697218, 847410717, 395750378, 1758281311, 1416414765, 888536461}
,{2101132684, 776887113, 266902660, 1570112674, 217370230, 470987403, 424179616, 992153079, 1910226117}
,{1103834175, 1677190638, 931920150, 1013519088, 1019828431, 753977765, 1204898844, 1021301222, 877121368}
,{1426481987, 1637410412, 1090599832, 1203075555, 545902807, 1889760993, 1825047855, 27437894, 2039538097}
,{531442429, 658403735, 53388633, 13882852, 1437364271, 1438766526, 194065443, 268082494, 699290304}
,{693644520, 1711037799, 84955968, 1734725574, 898088098, 1468262217, 1396425292, 386983263, 1084766503}
,{1872555447, 1333189674, 1754046934, 700581142, 1547643545, 1429414544, 1683248573, 1507308512, 161092107}
,{1525430335, 1022211943, 1768485046, 238476604, 61989714, 2098924061, 1141235807, 590663348, 714251001}
,{2062233841, 1382792228, 357771100, 450822459, 515599029, 768384566, 733081596, 1912230818, 1779053525}
,{2015071407, 185311598, 166627066, 140273984, 1182733716, 231655882, 300295872, 1323660617, 397329953}
,{1445216150, 1083894088, 1356003939, 495873418, 578873394, 1651810259, 670496166, 905444854, 1765206248}
,{928226387, 971274830, 34201928, 285424722, 650401600, 1398014871, 726012729, 997687908, 1160204291}
,{1227892263, 1483522768, 461587055, 2115020532, 1349367546, 1375883023, 302150544, 291280924, 1676915952}
,{1490156810, 775721408, 1437361508, 1041457343, 1508296422, 1327908348, 1831587045, 937272789, 2146618587}
,{647027700, 1021780673, 191999265, 4026252, 1727339256, 795320978, 1846372205, 2042150539, 905212385}
,{373142969, 1142239024, 2076392222, 1061586736, 519159519, 1978006626, 2028334997, 29546940, 508818090}
,{1237792726, 792767360, 2108893608, 1587998284, 37748953, 1003785000, 1757568858, 758100819, 1812523131}
,{1185480689, 24952725, 1038946971, 224263878, 1325046172, 576402125, 1374077139, 1783446344, 1130336681}
,{1756866049, 1023470384, 339364724, 824658491, 377355184, 1374934908, 717130965, 1110533565, 525539348}
,{1540027315, 1378169068, 1312030283, 1540398694, 2102665944, 302288596, 776732545, 1805413215, 933900407}
,{422976194, 1283669508, 1192381232, 913897898, 1797210094, 491382746, 1664642365, 868143982, 1708108065}
,{2142051263, 1755999889, 808804712, 945311133, 564609774, 1328404494, 1847101692, 1587315729, 975416910}
,{1563012016, 683607648, 29560783, 1333485697, 2112298932, 1854540360, 1527955304, 1228198070, 608687784}
,{1189677889, 1827203011, 326655487, 1130648628, 1785601530, 229699744, 675757306, 1538378665, 1784039909}
,{663586606, 1290699694, 62711311, 1543656645, 2094989336, 1702689476, 1184756912, 269998267, 604625572}
,{615333616, 703546175, 260915428, 529831076, 2121459057, 58682680, 240376490, 1519441128, 301164474}
,{1985975850, 1416334086, 2054251613, 69496461, 450403117, 1794984362, 1640755451, 676348059, 106307114}
,{192207518, 1543954104, 47395354, 1937695306, 1309893884, 1261933108, 464712225, 164433292, 393348773}
,{199704471, 242288073, 281858547, 1195914638, 663479612, 1353507514, 1311369470, 1810677147, 781361887}
,{1990851256, 813796171, 360724711, 1612521497, 2093338315, 1834985149, 684847550, 299083031, 1022291613}
,{83669455, 994725875, 1308407170, 1641688699, 1611446088, 428570654, 514162347, 65867507, 192528090}
,{906822817, 1451550805, 1390422734, 2043755777, 1931593941, 812040272, 247215180, 1957230464, 1676060008}
,{1087834299, 412464835, 1007975900, 655676231, 1919467821, 2071832295, 1893447332, 1281459853, 719337596}
,{1947569347, 2109015573, 26788886, 342114230, 1971582543, 31501931, 1196879216, 1641193422, 1611913210}
,{624788443, 480232609, 102333143, 293178401, 726081866, 583679201, 5419605, 828152451, 1617344514}
,{2020009836, 536787749, 131924718, 1111548123, 1546310610, 1547771224, 19701519, 997278409, 16147161}
,{246617937, 517213068, 784302764, 1716680405, 1404780253, 108774243, 1068354326, 1987001485, 1246200645}
,{375068471, 1290481129, 1918863582, 182621304, 656998662, 192839523, 26008649, 1139885918, 43290226}
,{283451116, 2138578625, 1107810505, 610839367, 177162264, 1609874005, 1081225515, 1843023172, 820594342}
,{130434103, 1780060614, 818378623, 33833798, 1419400397, 2056531225, 635463606, 455666630, 186175253}
,{2031320447, 1492515620, 508748592, 1115206383, 851412591, 1533172740, 2119621780, 373338760, 720744025}
,{168646215, 1723763804, 517120047, 507306039, 806489709, 1127866717, 1947727796, 1543588228, 1125227288}
,{1416334216, 130112976, 154171998, 1980571024, 1994082170, 1352382918, 1613573259, 1308446201, 1643500182}
,{1035834924, 545369028, 1491878594, 869830798, 82629208, 612323534, 633320263, 970291253, 1138891640}
,{1046477687, 2113823843, 1172308176, 1468679894, 207320561, 351692282, 1263964242, 1241040774, 1276194843}
,{532828784, 2057911955, 421355086, 1667572574, 525336297, 153434963, 689775396, 996654456, 1894494155}
,{1891701712, 1453762808, 829602151, 1915754170, 698610378, 2080157397, 1926908162, 1456084678, 2121716023}
,{56888719, 1353686615, 1637013251, 112684669, 2132824940, 1226401381, 1315146950, 726125060, 1878681169}
,{412323128, 753823342, 1028288029, 649641162, 607818356, 1468503706, 1158556826, 888391367, 1048500654}
,{1903978426, 1689284343, 716820529, 1364010446, 1936541948, 1667136988, 992316042, 1231448196, 1076038350}
,{1753735781, 1443826417, 217194941, 372577781, 1711306579, 937393076, 1607094282, 24843147, 306747107}
,{104065759, 1127076274, 49795597, 1815906775, 1230225952, 983279076, 1423634838, 1102886217, 522093229}
,{659010867, 300224996, 1764841841, 261713377, 1333996801, 821190469, 296309171, 1339121173, 113685114}
,{174924460, 937651655, 1808321746, 384985936, 1188044719, 876428094, 857326819, 2125031014, 1947522417}
,{1750024654, 1367652089, 426280387, 1676008496, 210516656, 110898366, 1031711053, 563290351, 1194481039}
,{330722267, 139658060, 1053490284, 410753733, 1572706027, 775524543, 1104179241, 131665181, 1633997396}
,{1665773641, 1887041722, 1810373680, 1989135783, 364274549, 1411395130, 242643561, 2011584719, 1700329362}
,{333893580, 920965973, 1384314129, 2107397818, 96095849, 804103220, 1102878002, 1936138882, 1780679252}
,{734963434, 1289115770, 1912846454, 150864956, 1793827755, 387633474, 1132929644, 183836310, 655743223}
,{1107266839, 596159112, 698315035, 572747669, 247992157, 101241536, 233383768, 1350056809, 741839915}
,{1049385420, 395292491, 1887234754, 1577167290, 1706034131, 28007594, 1233187002, 1360300204, 1659425789}
,{491922951, 721234370, 1553444405, 41128608, 944755034, 1730177343, 1270004882, 1958858911, 1440270421}
,{123363626, 1220514461, 699746011, 1981790825, 1938895615, 1821582286, 706437305, 123925033, 1143902577}
,{1162393950, 511196425, 554672623, 105679782, 638825267, 1178100366, 786582589, 796743517, 37842489}
,{1856458044, 1307917390, 1863904889, 1255426271, 547809507, 1087653804, 1912722787, 1337592572, 1368142295}
,{1557317545, 1379062586, 1126819808, 2038817671, 698136058, 1370872357, 324891936, 1495608764, 604169720}
,{664388853, 716225139, 1075321830, 1908295670, 1299070502, 1879553411, 1598594142, 1563536085, 1288703784}
,{2370101, 1642019685, 1297654633, 638951009, 1419043024, 1748564332, 1974147305, 484870364, 1678835126}
,{1545934198, 627492844, 1784671994, 1428716086, 780902175, 2019151157, 755846542, 1378918161, 405655001}
,{369724374, 700918065, 1726759097, 1143091679, 1985189377, 2074753087, 1752214793, 1925680872, 452207613}
,{8421513, 1195173687, 312027615, 2098769957, 1735617782, 440606881, 265189389, 824837665, 1983795511}
,{1135498696, 157267825, 712985332, 58467755, 1490118966, 1477830199, 88370580, 27538052, 621487189}
,{1172543222, 71344201, 1880544671, 351642226, 1515020678, 2023186085, 1709300453, 1149251438, 898034310}
,{1820895720, 1721295491, 150768989, 1291865727, 2138259437, 1083981989, 737742934, 389410606, 498727172}
,{1981556797, 364161094, 499157633, 276276918, 657627259, 1495977315, 1586178308, 580107969, 1420957111}
,{1678295688, 154385693, 1140072110, 907887562, 1058682100, 425975661, 1098354811, 1704579384, 891576045}
,{560861067, 886974568, 380259744, 1729390439, 436288804, 1442292471, 1056573385, 1896460666, 416772665}
,{1241217315, 774036353, 1151706421, 1644852436, 646384814, 1950223028, 1328121820, 1180009799, 126007202}
,{270971825, 1261170296, 316997104, 2041753739, 836167969, 1703201069, 687244371, 834850568, 1402963729}
,{1506355785, 1746296001, 610161575, 1347271099, 1039792595, 1755942388, 854575198, 833421415, 211224188}
,{1551166562, 1912242621, 1886197392, 307472554, 2071094723, 1584082689, 150652445, 791615110, 720287611}
,{978577797, 798567906, 606361976, 669241221, 1739058019, 133943130, 1633223704, 527109654, 910324546}
,{1494896111, 467436488, 1561525181, 1204829814, 1740556960, 1125338855, 630785670, 2057796653, 268708855}
,{1690494290, 1321037958, 1261743293, 601178102, 765972612, 1803106780, 630655920, 380448530, 1706557450}
,{12913515, 1488999640, 677681125, 317020476, 595983873, 947608998, 131269611, 1452192118, 2020451914}
,{702746958, 600570617, 602426591, 1655783903, 1503842631, 139860817, 231352758, 1360062073, 276201629}
,{1934186028, 727745499, 973112665, 1862193022, 2007388927, 909773848, 1513685277, 1010280628, 365300519}
,{501189508, 830982887, 439411383, 182856027, 1436118571, 1514560018, 1842050659, 1578247761, 991106037}
,{1006249666, 521429696, 836072416, 1359363613, 712332653, 813487407, 1302971100, 903323728, 1255162807}
,{1052639393, 926494244, 1048303243, 1079680735, 165652953, 1953934827, 1116045330, 1762806693, 944534981}
,{98877687, 1444708663, 625277929, 775459851, 151635414, 14237102, 835043308, 1342188152, 970359749}
,{2067333498, 586359672, 291246366, 285262126, 509684503, 1795528230, 736553309, 993836109, 1128209506}
,{1615516334, 1429262163, 2010916135, 679676108, 825986099, 219586352, 1075067453, 1699266870, 1304984518}
,{1266805735, 1320101410, 1908804145, 114885181, 998348294, 339848491, 823489532, 1115154033, 2003908753}
,{1553325072, 1219938383, 2059761544, 1531000854, 177923653, 1964764796, 942597136, 771595127, 659484635}
,{712380347, 1818472282, 1901958305, 1211494815, 922317240, 173274790, 767570305, 1479716120, 404416598}
,{1936674824, 133839966, 848457955, 1803140234, 865372009, 2032984154, 1640152054, 584841921, 768051519}
,{1933798956, 155852471, 1683848271, 676802130, 345136426, 1197009754, 620649763, 781697260, 71542385}
,{1175428503, 1762397548, 223283090, 1020184713, 236483384, 1673120119, 1677800513, 1828818904, 25893497}
,{492741100, 2059294479, 1413652415, 1792376281, 1070369257, 1886835094, 109015038, 1539472653, 1927418199}
,{1604721976, 1014192009, 572442527, 1325926075, 142450315, 23042588, 1423371080, 827746197, 1678100246}
,{1619098046, 643038361, 1833147233, 1816140309, 967412485, 720599139, 1473563352, 441399677, 490800051}
,{1699394601, 1735990164, 911767864, 1863484996, 228158300, 1248576236, 45269452, 988647229, 848499411}
,{1740064762, 1944836297, 1719883533, 1318761011, 1550515031, 1550561995, 640315923, 1416772985, 1591515398}
,{2095628741, 1853574760, 1206966031, 1969587057, 828144634, 1227478883, 1099470969, 403656212, 1975355148}
,{1208534203, 1048821278, 1488600083, 1320600781, 1212240610, 947956854, 512570090, 67459426, 1832874905}
,{1217590947, 1761611381, 1510599648, 2003246978, 1795505632, 2087073826, 1424368903, 1826840409, 2122479130}
,{659880907, 1526310252, 1799206344, 2123986603, 1589811980, 2046971211, 116398779, 1975872848, 1757249942}
,{1316015039, 1450125448, 1225815274, 114705330, 1303960902, 882146002, 1131812538, 1651306224, 1155500453}
,{696272615, 1729816472, 323720957, 629937770, 1968766430, 949250145, 528354318, 230265394, 1243928448}
,{879028985, 1937235529, 995728156, 1202633148, 474492273, 18962649, 1955458870, 1591964367, 668924021}
,{1199713736, 1178519216, 393956197, 810359599, 808264310, 329122679, 171389077, 428250875, 121505592}
,{615058128, 19424265, 809727416, 283903587, 1121671204, 663147379, 470251651, 1300431515, 129649262}
,{404403477, 1570900726, 1028927607, 33770086, 1940713383, 1107236962, 856143035, 1344732759, 1362682735}
,{2120084216, 2003529189, 739417198, 824884343, 623066474, 832002984, 877712883, 839807381, 436508073}
,{712600355, 608829645, 60434287, 1715777697, 1299092922, 1970032256, 1088909915, 797788315, 482996100}
,{1959443963, 133854588, 1915637931, 1744429013, 2066915272, 1917655395, 707587631, 530405701, 1988647459}
,{1984450809, 339689518, 1540716585, 950861364, 1769900968, 611499376, 897195074, 2140081641, 1128246576}
,{1117045222, 1036778207, 1565771821, 944736320, 496278852, 860560744, 1694072290, 1425552842, 212083722}
,{561332331, 799406715, 1233003407, 1012140902, 1445422665, 1784056405, 1145454577, 289109253, 592615150}
,{1258674914, 1377903475, 1694622609, 2055873321, 230524029, 293122161, 1699404035, 924210997, 1732411280}
,{2003133657, 1063930855, 1921472566, 900291749, 1246681805, 40711528, 618733343, 1005857626, 1538876730}
,{81299760, 2114064618, 1254192325, 1985021040, 368027012, 2085609702, 738977294, 1082133893, 1385144520}
,{1590885911, 440460876, 1727593452, 1792190481, 692662914, 1064766813, 603305791, 614665813, 1873394250}
,{1666703626, 1915632985, 219237626, 1638534252, 1955755830, 1976531810, 601193316, 998619862, 1820987243}
,{1835019487, 938332979, 774367200, 732644141, 172785047, 1047695435, 2018286533, 1422108225, 211918080}
,{457798036, 702889476, 2006959071, 1545894606, 1115126219, 113163434, 1323106775, 545936323, 1246171053}
,{1325940988, 1351343595, 1485410691, 1922139281, 482165630, 1746257708, 1733213952, 456520939, 215009119}
,{265388844, 285220947, 308733896, 970086531, 1368034053, 621831581, 198467025, 1437493984, 1294407888}
,{1669441967, 1647953414, 549738042, 1414510970, 509063991, 1815268002, 129142606, 2013556152, 1092809526}
,{1264941278, 494541898, 621151921, 1895019655, 682562588, 1984547733, 1322424585, 1969041962, 208146266}
,{1238212219, 654818421, 2018681568, 544458446, 530911647, 114069962, 46021799, 420976634, 1886808378}
,{1450485845, 1271042374, 1395492015, 144344964, 899254326, 1300243240, 940406881, 2070060385, 711604103}
,{478040554, 1583501697, 1638976475, 386309866, 1368338130, 771942409, 187680626, 964866741, 1138008953}
,{1942752968, 1672385861, 1354881855, 456494068, 1745824244, 1851415781, 814260514, 1674741441, 1267038922}
,{1317099136, 1795097815, 1834161789, 1527229937, 279178345, 904616388, 229055865, 1983931296, 1302789309}
,{760201411, 1768207467, 453768846, 996566607, 752061222, 535769332, 1554739768, 1311401147, 1044690097}
,{1834127769, 1533410582, 320777307, 124610754, 1909586200, 1857833504, 100032310, 1163341269, 321300750}
,{2109871860, 1185639874, 2048710830, 1189484069, 2142003409, 447184723, 66835077, 843969245, 236001438}
,{1911528981, 484188239, 794970143, 1697497884, 1427176444, 2070010925, 607337231, 518562212, 386686211}
,{1615097785, 527174324, 1386056910, 1362347727, 36609625, 498761011, 245122915, 739683795, 1388333032}
,{296218728, 1369944029, 491418052, 84752666, 527979213, 2125260933, 1143441096, 1402535200, 37091893}
,{104925333, 743813826, 1020044014, 1347161526, 1032326358, 2102954330, 1357751822, 1086696753, 2047846865}
,{767907268, 1337477903, 1593049761, 703475774, 1860907777, 1927703818, 703605060, 457214003, 2023005423}
,{5462413, 246901888, 1926530552, 516490675, 1068196234, 1640854414, 707986413, 1664559801, 592974486}
,{1916717137, 825286288, 180974819, 64340968, 137934292, 1979796390, 561787906, 511960295, 1569360924}
,{445540132, 955415994, 1210890916, 785347291, 1332250164, 2087967136, 50605689, 1734002941, 832723317}
,{996587498, 1720596685, 1982058486, 1825195560, 159731384, 1157042696, 1147596813, 671917399, 1964669759}
,{1844369318, 348163946, 1842535315, 444076729, 1823620815, 1635369967, 2021775103, 58577990, 729376695}
,{1080327704, 1337076529, 742705159, 194225647, 507776238, 1388458562, 471493027, 780822364, 663985140}
,{23854824, 162709277, 1644042517, 780558957, 208143141, 583856818, 1224944266, 996073558, 1792252678}
,{1337631426, 2126633736, 557403136, 1393376232, 139535475, 155224184, 932571401, 1174350424, 158009556}
,{1946116410, 1397525486, 1948248032, 1767117994, 125968566, 1790126566, 1965948828, 1938029363, 1917264483}
,{961713903, 2103861859, 1236039130, 1862654981, 777299735, 2089384540, 504754541, 1793365074, 1652860512}
,{1864361266, 1064599595, 1765940124, 954622877, 2108496233, 1056318246, 2040473832, 1528947224, 2002861434}
,{1318865856, 1287700224, 477210780, 1811464139, 738377573, 396600445, 875271536, 1075622372, 1650399931}
,{561312488, 1872678682, 1146454844, 1281621445, 1368743140, 387729123, 1332872198, 600367811, 192246765}
,{1955758014, 1980527414, 788387521, 1322376397, 1803529754, 651767015, 23398154, 669941949, 387934066}
,{1536125114, 1576083642, 893225462, 342138472, 1595128196, 394194758, 540508932, 1441450534, 446375415}
,{75282883, 536444993, 368352262, 321896687, 979307461, 832218531, 1258191782, 96277928, 1804603645}
,{866602872, 313331818, 296460635, 1977806542, 1103943272, 1298511001, 2096839614, 599667583, 1216182165}
,{1028561425, 394428740, 1296174163, 400023434, 957707758, 571303153, 344598783, 601536407, 1498789292}
,{1297835604, 761749126, 1830969583, 982066839, 582597738, 737003394, 928773301, 764370366, 959313204}
,{553194747, 1882698301, 224480277, 1966126293, 390078957, 293215769, 1088222399, 1477874271, 1053938968}
,{1404000864, 1758041069, 2066476716, 628063417, 1060918593, 1696137496, 1043331863, 585019812, 2144651847}
,{2120435410, 559746162, 1096429410, 836907001, 818538899, 891335592, 2072952141, 218747934, 97732028}
,{1183517321, 1374688487, 338990865, 1710417870, 635359184, 1067831607, 716869079, 1352139510, 1235852179}
,{1428198108, 1677996514, 271536524, 1993039114, 1339308101, 2037120635, 1169157037, 1261711604, 1327205769}
,{227663486, 1672785867, 2020201558, 1053697068, 392046373, 89865094, 1344503681, 690494962, 365121501}
,{61088030, 355555224, 597349370, 1308656904, 683728318, 1407916237, 1929939787, 1384797255, 1809662014}
,{991853711, 2008770887, 1321492014, 282295880, 1263546194, 695254447, 1829772234, 1145114151, 2043872247}
,{1659913490, 1555937290, 164832896, 394375352, 749163717, 2048709392, 736274797, 1962040273, 1458313992}
,{828919616, 768967248, 1737718414, 1219429107, 774886537, 49348653, 968827109, 1846707260, 128538996}
,{783474287, 1140774341, 577775628, 1605718086, 966614447, 1618177617, 1836774562, 1177595354, 2057694626}
,{2070633564, 1580709027, 928385020, 51640565, 1694929569, 1285484009, 1444833258, 1478684207, 1929009373}
,{575185968, 1237438262, 620667743, 2006453720, 1628395629, 1895116632, 1245867110, 496658408, 166074998}
,{1021878582, 1387679490, 15559515, 1251430066, 1965663954, 1244770351, 761005898, 770329637, 1373926034}
,{296045799, 219369643, 1577736849, 972138091, 2138222186, 8379431, 608436407, 1510804625, 2146311479}
,{1407334337, 1010666319, 1418521156, 703819533, 140235551, 195976754, 705684198, 242081699, 2095616421}
,{49857864, 958003674, 1760692703, 1384711819, 283360211, 1370050151, 314807509, 93413185, 787536508}
,{1426981770, 503179940, 1651029287, 2064821974, 43411605, 1077478144, 1703408666, 2112874035, 914518428}
,{802974180, 2142642804, 313363830, 873499284, 9156469, 1861601841, 1916661015, 1813280664, 1951488613}
,{1338537954, 30581606, 1459349875, 861066722, 1906395533, 1065331875, 549612807, 1420321516, 979786273}
,{1035621127, 1988572912, 489250410, 1702335350, 744313135, 230241147, 1160285285, 1833106412, 1391216079}
,{1498162797, 875686788, 2006685910, 374550473, 1692368019, 29207262, 63728086, 996490737, 1021473714}
,{1421918064, 1934277676, 1527009493, 1048181773, 446301533, 1866103083, 1195995961, 736957369, 1154734095}
,{307468311, 506688445, 2123653038, 1844940258, 755058939, 1580190542, 1785334713, 468453832, 1879590286}
,{705429712, 839826987, 309982071, 1253030431, 1811464028, 1881416776, 1369473117, 451592293, 901787449}
,{334837132, 506708772, 1833608729, 482869485, 376354013, 1678005964, 1675803371, 1077454311, 1180361010}
,{1059923350, 1992166580, 439721350, 244591221, 1459292437, 547451584, 677610025, 544298624, 1698284648}
,{240908944, 2039763651, 644910036, 701721810, 712634113, 237458603, 883922072, 1680839273, 1520204738}
,{1406973489, 643495749, 209312283, 1494081167, 1608998194, 1117422669, 784581990, 145735647, 162126347}
,{1642148701, 661872590, 508419059, 105563962, 1809511949, 194494727, 1191965181, 1943233978, 914631692}
,{2137123750, 1443441634, 125370197, 994751645, 1347486606, 1769190145, 43024558, 764944685, 1616626878}
,{1388490883, 231279612, 1610287747, 201162236, 711572161, 67430350, 512945301, 129005563, 988542761}
,{1375992771, 1819284599, 651280309, 1485838607, 625092018, 371319488, 1380311947, 1754348702, 225818337}
} /* End of byte 4 */
,/* Byte 5 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{515769131, 142138076, 1097629934, 1998204213, 98420724, 1142734886, 1341917818, 1313784612, 927086298}
,{1958301840, 1774108231, 889807719, 744516909, 1800418879, 1088702587, 1260381090, 1377125351, 352057365}
,{1823070941, 786550849, 1774542449, 1723220314, 222914731, 255850202, 1899743243, 1380432726, 384008371}
,{702516624, 606136791, 1035328649, 75277229, 26957848, 789752702, 973663568, 593488439, 1859346359}
,{593686007, 31441429, 1037101635, 6393153, 944834055, 575270603, 1157879471, 782251344, 1100542814}
,{1974179051, 393568237, 526206447, 1410242438, 1987879418, 883721575, 1827516631, 1951727623, 1626897297}
,{807390973, 890620431, 1062264610, 1476725278, 257885154, 2003897961, 468274506, 1957074295, 1183521886}
,{519245959, 1354305672, 634244421, 193274965, 1625138757, 1065195404, 690922759, 1991561464, 1357588974}
,{97470679, 1403797279, 865828124, 335889734, 726655531, 1262308359, 316646016, 1943050546, 690832934}
,{1610369097, 585726307, 1024400316, 2006532441, 1100238104, 1847458223, 699912485, 150950336, 617908757}
,{640380450, 1195812513, 875471832, 1939926379, 1270791219, 2045421179, 52200237, 1599533749, 1677322048}
,{827241867, 74436339, 1171428768, 1089377876, 2019763623, 536520962, 1198394796, 240442968, 647118396}
,{213791281, 1045195015, 231492018, 644505827, 1439212060, 1077742249, 820649872, 1645768560, 863193064}
,{712403641, 1875204994, 1231093484, 1096398631, 1236945238, 1733237902, 839208583, 1124798174, 199020783}
,{596898797, 888493813, 342236256, 227840601, 112850916, 357456687, 968745692, 1177269797, 1526167587}
,{1802857540, 1545633839, 326696530, 1821029747, 2078088446, 914633700, 6574735, 1281857678, 1414689894}
,{216145493, 739258127, 1338160559, 331417122, 2009523977, 2059471575, 80474596, 2021237931, 335723060}
,{1964494, 723713228, 1374735601, 795607489, 893978852, 78780561, 1454536587, 1038138960, 530630216}
,{531588279, 1864729830, 378141817, 764789525, 689604753, 1571299750, 901928728, 1308107929, 1198134619}
,{494943097, 2005802782, 557397232, 935902171, 1338754797, 1510450151, 1209855685, 1049323593, 219872263}
,{1257174950, 167328308, 307602352, 754255562, 15211504, 801058899, 2067341202, 245434506, 573554521}
,{1883374863, 832895091, 619298778, 1067185076, 1323425999, 495836436, 16259296, 631997593, 350607565}
,{1118041314, 797421585, 879983969, 725416048, 911225170, 1909345678, 2144448264, 404355885, 1306571522}
,{378241905, 1651014496, 711466803, 648031186, 12446920, 2055500741, 99415726, 1257480776, 1290315566}
,{864301876, 743947483, 463766285, 847705514, 1854059251, 11945097, 137462954, 155348386, 296448763}
,{761113936, 621793647, 1087517816, 352369901, 2088245136, 2073390687, 1113450352, 1251152466, 147103220}
,{1656957112, 1458020452, 827812922, 610459623, 1971897350, 19092004, 1250627450, 1246481839, 1428254524}
,{1178301318, 472012953, 916790028, 1139893279, 1037189665, 1992342774, 1722708610, 1610687089, 1439816825}
,{412796679, 1590825075, 790175784, 1048270033, 2131874570, 1338972055, 1932148300, 372018128, 713178037}
,{1238162541, 1247773808, 1387172776, 829089813, 265863321, 921791101, 1171920073, 1814782158, 1563424848}
,{375839348, 196958610, 1386764955, 1970933752, 242412215, 924408297, 741099504, 2141045655, 1103658971}
,{920069559, 22484092, 446044854, 896692077, 1765200063, 1740569211, 1017961636, 110908775, 1189511080}
,{225116938, 1175239772, 265318250, 27559398, 1357798878, 881700479, 1996832547, 80287283, 1651820876}
,{946575309, 1295952742, 1344826749, 1488845474, 1405362944, 1931019025, 1205841002, 1281549488, 1815814082}
,{584256994, 1446270005, 316379209, 1228515342, 911409134, 597534963, 429364519, 970863578, 356858517}
,{1627061835, 1743576030, 1086558276, 553465887, 916064229, 637233476, 434029830, 1475297270, 1473452008}
,{1769799548, 1477432748, 126674369, 2094790385, 1233451671, 1716996365, 62596553, 1321262315, 234138949}
,{1452751343, 101456389, 1548925694, 1750352914, 800069644, 2062599613, 75396553, 972344844, 889245678}
,{41288870, 1814869251, 1176544839, 376323424, 733880520, 726676323, 1244330673, 1336672059, 158671117}
,{242602203, 691203944, 659415329, 244392405, 1932364913, 928415720, 334469827, 878947242, 1839481743}
,{1530867026, 1606971730, 821023646, 586972799, 1988119487, 1715413365, 757499880, 321847841, 1982846925}
,{338302162, 47643598, 1278014333, 520266418, 829264611, 143986206, 1871864255, 1475726664, 742257008}
,{822914522, 137754382, 122873188, 743761397, 179344319, 64482664, 529800985, 1095201964, 225520361}
,{1737507222, 2122576617, 1741446454, 211621639, 119010374, 1729197454, 491982251, 1220543857, 1517181124}
,{984740139, 346487933, 87974107, 1254042002, 382521492, 2041938349, 339663326, 1779404932, 1048219954}
,{1798217298, 46475621, 1645608055, 695929364, 243191806, 1349834849, 2139533499, 1987202104, 2124651446}
,{1188180731, 726697795, 195607013, 256884162, 1889102901, 1491950545, 210295234, 1686519383, 1620943664}
,{278862120, 215713201, 874960371, 846119506, 2076483696, 118111459, 726247178, 377184629, 763353070}
,{1009636359, 747852160, 759666060, 1857688546, 876452674, 287105252, 1947669410, 102262429, 1807028465}
,{1904810952, 1700743253, 1055982819, 1354869209, 1397218670, 175065961, 1102083915, 432268927, 1997543822}
,{546745406, 758281161, 2084218699, 719479370, 259109418, 1211165363, 1982433369, 991995332, 1200401240}
,{404668322, 1408961536, 376469857, 78378137, 2060569284, 1482075988, 716736720, 1975003932, 510324449}
,{1950019430, 391011286, 1119675077, 193894832, 656174189, 782246309, 1812256510, 633346877, 112380235}
,{1044576417, 192319852, 1662027516, 1432939780, 1324847420, 1652079206, 1728044888, 1687117528, 1593349744}
,{1918407926, 667389315, 32649833, 1654963442, 533290201, 640191635, 1419195496, 728748289, 1244595941}
,{275703173, 1637452625, 1686746888, 1037998544, 168838366, 789337238, 1195730490, 1693234696, 1294193140}
,{1016855007, 847384026, 24485870, 603014919, 565774630, 1243104050, 663141276, 2088902390, 908400617}
,{642774878, 1168784672, 60726582, 230824238, 2063053243, 729065100, 1953037444, 2124241509, 138452709}
,{2121245, 1710782947, 1763633072, 1644994108, 1817878140, 99427776, 1299336621, 1297600473, 612098698}
,{40512603, 1938027419, 545076562, 1908452280, 774305418, 98486908, 1369599603, 1751214966, 805625274}
,{1510642849, 2135735685, 2111563020, 1174158290, 45942450, 425894682, 29764599, 1056366498, 1608949458}
,{2073791654, 977301168, 1337462367, 2009348477, 1736545412, 1754712642, 938061307, 285162649, 455622479}
,{797073400, 975784015, 1604621280, 1822308587, 1721476333, 696808325, 1897609278, 943396975, 854416480}
,{1527019952, 818851597, 1733670399, 2018382919, 52917387, 1572884802, 1397909083, 1851237287, 156603613}
,{929642414, 85833114, 1033356699, 1094563235, 1343623769, 1263696822, 805811176, 383032594, 565317267}
,{888215836, 439742985, 1384865599, 1385791276, 327436147, 1846854360, 738466573, 375708699, 88563920}
,{137027262, 871498931, 332321553, 828983832, 253138612, 1038287754, 1472159392, 1602218233, 1088226601}
,{985655190, 275194479, 1835442891, 2121559317, 791017394, 1003956986, 2128303040, 1376653253, 1705053126}
,{129963494, 1328227957, 481610153, 1852352578, 226802179, 638495640, 258136679, 1654630558, 1580416766}
,{1281815691, 1441348019, 350224880, 1141749918, 1322742180, 26344886, 985249264, 1226409405, 1430369800}
,{825652127, 734464994, 596786811, 307817971, 1853329029, 1197106026, 822044796, 1738089210, 171255982}
,{2015246779, 1861137250, 1878382694, 1443996805, 524431449, 1048426695, 974562439, 995525769, 1142365071}
,{998216866, 1665058287, 998921918, 406699795, 1131346544, 1959927478, 98198936, 2090317085, 936606476}
,{2038056788, 2094672319, 93263297, 1659498008, 297688812, 92862255, 1671374243, 1396639570, 882606786}
,{602055180, 1739773242, 738925458, 2025587000, 962186980, 1529053929, 1170124224, 2045540251, 1427363898}
,{1673437224, 223638208, 1556107233, 1480412948, 571914072, 67431909, 351390059, 1250052245, 671266698}
,{161902983, 1272870607, 1764726019, 717886458, 1286697009, 1538327110, 152569707, 1416106337, 468937244}
,{756625944, 742239012, 677823511, 988850250, 1984297806, 906916661, 467776862, 1039876713, 868721680}
,{648620439, 599840271, 841284404, 543528129, 1237493868, 265448935, 211506365, 1006371813, 133644457}
,{1754073974, 1334933764, 502600037, 1253997242, 251972055, 76762779, 368358882, 1339819998, 1715770338}
,{1241712275, 703177851, 1054459011, 1827615807, 2124936837, 1879191290, 1470483883, 1635068995, 1015791777}
,{1116340789, 1554772269, 1201988928, 1530564022, 2080058113, 2130420634, 1959580259, 737336749, 640874245}
,{1486726862, 1824691088, 1958370388, 703591436, 1428037722, 1519085051, 460322979, 435104211, 309383607}
,{754724552, 2045662252, 2090439347, 298466512, 607772801, 922515200, 1767559027, 431440129, 1423136397}
,{1121604336, 851315076, 834995891, 1352658421, 1086564989, 29256829, 1791123642, 940513511, 1938501266}
,{602969847, 839248073, 1578927883, 331047369, 79103849, 1379827903, 1418110664, 264205366, 630877065}
,{368192427, 783861679, 899596833, 1623735551, 125600974, 1406613606, 1051527165, 1792571955, 1108813525}
,{624768090, 1917443175, 655095013, 74862595, 1835705879, 625032520, 436333631, 1103261830, 375822415}
,{932538123, 420435898, 4858408, 1424765388, 302319143, 416170102, 73947237, 1076761308, 784446144}
,{1477989521, 1060497984, 107824065, 46030912, 1718769240, 650575834, 1383361076, 1279841453, 519205455}
,{418017327, 347587950, 1428602953, 97276771, 1210086294, 952901162, 1309835911, 819950208, 1782097180}
,{1793180031, 1766001809, 1448139659, 313299535, 1869941810, 172021157, 1065809707, 691885414, 983536821}
,{2013330655, 1905562204, 1654841975, 373596080, 46212625, 664025563, 1744555575, 1373614364, 2142427974}
,{47670911, 490436915, 1751481182, 1021192055, 388598290, 1654868102, 1387025711, 1716849062, 2449483}
,{529609421, 2143759483, 621077523, 809611534, 130470346, 2065770368, 757051353, 1940507931, 632408576}
,{651806519, 50112340, 684080285, 669351082, 1491313411, 1862089523, 354471499, 437911875, 801173343}
,{164527585, 1772972135, 841249754, 369792140, 1825265240, 104774745, 1341449505, 437155725, 61648001}
,{906530855, 1174171624, 1981550144, 991391269, 1833486700, 320014913, 1117925648, 1459110751, 356430296}
,{1218402099, 510051808, 429402473, 2143281846, 1820987207, 2123720538, 1618835567, 180825027, 361834077}
,{1923130579, 1321713301, 220917713, 1556882244, 1631630126, 541243416, 1749469435, 407697030, 415194342}
,{2179469, 609690059, 69613436, 1894342395, 1710512254, 1575294886, 162505320, 1837465533, 315780041}
,{36270692, 1037825391, 1998032402, 1282226484, 717757443, 166137190, 866142562, 1193824952, 1588408898}
,{562225255, 88570766, 532698182, 525047005, 337338268, 1502404662, 221604915, 2058098020, 1764487608}
,{1798244763, 1437971598, 414223605, 1134803490, 1320552903, 1130708478, 1951210920, 451879668, 2021388478}
,{222276152, 1457147625, 271947355, 907167255, 1298931937, 520357513, 1485686596, 2105163864, 991621314}
,{319302235, 1718070221, 1553134545, 1018198166, 1955750847, 1069443527, 1930448587, 1094160437, 1250672147}
,{1192924806, 941963917, 332010011, 1245045417, 1570862580, 1168685329, 889337787, 1685396468, 1839967275}
,{851973517, 1191615839, 387111526, 2115184860, 601276423, 571302561, 294529812, 1153105967, 949308611}
,{1669472740, 965059136, 2041734579, 60632687, 1989659026, 380743677, 1586351745, 42280488, 686370667}
,{670012590, 1832332715, 1806010137, 465862148, 508807690, 524086485, 11967132, 1750163840, 2043733993}
,{1476719120, 1433951589, 63057995, 230857336, 1731979729, 917099897, 1567434131, 863371396, 1000303011}
,{671598677, 1941461362, 1872136957, 553030040, 1661746848, 1384279278, 1172890845, 679389156, 994463914}
,{770152526, 2037751099, 667539323, 1014659458, 1053203541, 264959162, 789551981, 845690447, 1508831161}
,{427390684, 1796540086, 932014214, 330366587, 270478044, 2076450076, 1923243605, 194102237, 2027903361}
,{1565358186, 421767127, 1312094073, 1768148393, 1990248514, 518136856, 701249444, 1952207817, 984853231}
,{135366806, 88497843, 72143218, 1047379946, 2009851574, 685364555, 1442878359, 434998338, 1642853791}
,{960195480, 1196445698, 1888948915, 444947348, 1615317736, 860377280, 1587816867, 640830721, 1498401166}
,{1400518169, 201495279, 1162478420, 1953380864, 1647989457, 677681561, 1032664056, 1162451715, 704260445}
,{605608716, 1505051866, 1671156369, 400822662, 201344857, 863592086, 2013144233, 1092086551, 1000230570}
,{1257221066, 888818719, 667661742, 444173846, 1634252662, 1093036440, 285098988, 1203382659, 1105694549}
,{1357906605, 1696829774, 1847292277, 1987419851, 941700662, 1758433374, 587595502, 1996509790, 1316664059}
,{48081825, 1221953328, 424623000, 1391781279, 442994757, 1741995601, 2069782369, 1633725091, 1052306981}
,{220922475, 1417844445, 769180329, 577266683, 556813079, 803650667, 1239521872, 1431924216, 1773626521}
,{669596266, 1145306788, 602023746, 896717528, 550011199, 1555193189, 1024073771, 1719407714, 1451457052}
,{92541201, 819608346, 1611048619, 588473291, 994787443, 894593108, 9450843, 907288043, 999359175}
,{938426202, 1959734835, 618295775, 1591017949, 2034940986, 1846034029, 938551506, 1906770016, 938074751}
,{1651825671, 233662946, 247207981, 735797840, 338169322, 836639967, 706003269, 1455064337, 1654257904}
,{847775871, 1030920856, 686139737, 54648433, 1850192628, 1915315307, 767398656, 1504045821, 1431150773}
,{547236211, 570430078, 1554439201, 1015892524, 51280102, 1560013410, 2082176331, 1468246814, 1040632270}
,{215819797, 1939999512, 1678246799, 402647228, 1485227031, 1542340036, 708931057, 1979882458, 577519806}
,{754684385, 999858413, 1950733658, 1432981083, 420999622, 1272589790, 431585408, 2140919649, 638045386}
,{279810211, 917279069, 326356540, 1625329142, 1008283354, 2000461501, 1881052844, 1396457376, 90026543}
,{202074414, 568939790, 845926137, 449327186, 1011893245, 2001452298, 2078153115, 1332586331, 1726604035}
,{1566443720, 1695942895, 1028698528, 2097635216, 994963914, 1147687593, 2022872241, 1497004114, 1974251027}
,{1588999718, 1452361367, 264411434, 118648413, 1076121796, 681463101, 133273531, 1253118683, 959652804}
,{615974733, 390122875, 685406961, 222156034, 721608183, 1570396089, 1102135971, 1017524450, 1439216449}
,{1309501738, 1103269163, 460857759, 1707921716, 885867181, 430195190, 1384031266, 1867755867, 983771904}
,{1578659, 1664947586, 1152142551, 1225624941, 1852377629, 1557020873, 2003049616, 95060846, 315867780}
,{1627251316, 2115532838, 1122376130, 1496622099, 1377217405, 1116498148, 1590595237, 617791219, 1053616296}
,{798330724, 812623105, 107154245, 152375121, 1186075883, 962704081, 2025065646, 87847239, 1047955167}
,{1755962128, 2112648150, 2110676572, 1932455462, 912077876, 209571217, 1715204099, 1224759845, 1020282842}
,{218768794, 1171670608, 2032643776, 263018288, 1462457205, 450856436, 1445347294, 1260385288, 1880315813}
,{1877036859, 532393091, 1317793050, 1833696551, 1134825567, 1915539655, 110483259, 1687653228, 1403140396}
,{820513614, 1579150613, 684892224, 1881719163, 2145812802, 647725754, 1961426763, 1517342343, 1936865529}
,{1269119670, 1383370508, 1943328022, 1758099144, 897768486, 2012920447, 1990425142, 1276198709, 1417466579}
,{1934402521, 1817405860, 194644103, 496382229, 1617505244, 1247739160, 1905419044, 258982293, 775242165}
,{497062536, 90190940, 1975250654, 1815743134, 295780172, 1634205970, 329536741, 1807779457, 1488418462}
,{1126445422, 677982322, 1446581135, 614215882, 652537801, 498474789, 90998953, 1978230619, 2000266549}
,{125637385, 1636981303, 616565604, 623353262, 1420606666, 2129879130, 944849732, 1581146044, 1296702280}
,{1520390064, 206438961, 2050997595, 1907667719, 561802469, 103799168, 726177698, 1438757160, 58237463}
,{1884953913, 1481306577, 1952003472, 860374289, 1949900895, 69953431, 961152130, 1597712867, 2142320668}
,{229221580, 1663906507, 2086654832, 811876776, 1519542770, 1583678954, 888122104, 930530769, 12740995}
,{260556374, 1806427120, 791068281, 824428445, 1777109270, 1070987295, 2026835463, 437158580, 1624103767}
,{403499910, 1654853844, 1396814783, 502676618, 1952799022, 439887052, 383860856, 2086312236, 793093010}
,{47476392, 1906511950, 568831918, 2064654628, 838716573, 1030901754, 1711254857, 1143592881, 545833247}
,{1919072165, 873968822, 818085471, 1983958792, 290043311, 739164476, 129881175, 1401629469, 765840638}
,{1008589249, 306709215, 1707972791, 1427541697, 1333727566, 920070619, 1278573892, 121047376, 861864255}
,{361325413, 1434882317, 949229505, 331957589, 1820362412, 556799146, 1398594898, 1775184657, 876925235}
,{1852503122, 809151165, 808966323, 743560131, 1971219295, 375523437, 355410573, 2015030801, 1373332697}
,{223661742, 1407103163, 1928218616, 783983459, 358017460, 1543569918, 1173220780, 373356186, 871587889}
,{1978961952, 1201446014, 1815366200, 1449678741, 1812129998, 1797299519, 1078937186, 1716295820, 1211426461}
,{894572329, 224333821, 623717064, 1307033516, 1578036467, 877601147, 1068175939, 825797755, 752136341}
,{1562670898, 977610152, 1130139584, 1381535310, 947426100, 215926973, 302945867, 2027123533, 1633797234}
,{1421313228, 1104975564, 829653995, 1349782146, 1544332043, 1466509393, 1712240078, 535749754, 712574535}
,{2141844514, 1879543531, 250335425, 482607034, 356371656, 485652857, 1905718025, 352476637, 2058365933}
,{1289651815, 92445594, 647563420, 962527084, 1816285061, 1251680194, 1712713905, 899948129, 320187210}
,{729927277, 257945213, 1697347149, 976353862, 757543929, 1827831766, 1701851125, 917576880, 436392021}
,{361515370, 170239125, 1157617617, 1905706591, 723702366, 1117450569, 1732877559, 1191825263, 628686071}
,{1216076034, 606476712, 2023799909, 463847822, 1351160113, 1684088653, 682749604, 654382190, 48384523}
,{1291128721, 776585929, 1689292607, 376588777, 2029726806, 432065059, 1430695654, 1379083851, 580679817}
,{1828207737, 1553960567, 1305274399, 1247902618, 681898033, 591965312, 239093081, 1271594222, 451588420}
,{1062908771, 1301120901, 1571606454, 289573198, 1008916616, 2110218837, 510907314, 1613994191, 1625134655}
,{495808174, 1198673789, 1902638495, 507209953, 1453416488, 380895834, 1125079859, 6057820, 1451296251}
,{926775469, 1694930691, 1481235642, 378696744, 1573269498, 189443271, 448562856, 1939686662, 1109127531}
,{1110218712, 530117828, 1695158391, 1658154479, 273288823, 1430031467, 1946266654, 332716056, 1874825121}
,{967757115, 1273718139, 1843110943, 585744765, 733394918, 398132237, 1381642113, 1498034396, 1927069797}
,{1995530754, 1280260673, 974062690, 729049423, 1146305118, 2085063462, 459038843, 890508375, 297756275}
,{319350350, 1077192240, 1910327595, 1806517105, 1479036444, 2039161441, 1303995273, 598499483, 1591684172}
,{1892014568, 621834484, 1439725775, 81690329, 1356014130, 818622844, 1582704509, 494111392, 324081684}
,{413492036, 1884057184, 209696323, 568346019, 662284859, 1309838242, 2113484950, 902368110, 1420197844}
,{1753917772, 2059139364, 1964501144, 1206692879, 77676971, 586274685, 148692269, 2121018226, 2127049333}
,{33938924, 138818548, 1345698759, 1772136659, 221403911, 556436654, 506700836, 1165294637, 1638911888}
,{385409272, 125992990, 1945512779, 63528797, 1315115114, 1279589933, 746037026, 446660932, 299790329}
,{841342651, 1000089353, 2072016732, 2023681888, 2064917843, 1886735431, 445291690, 794087973, 1035367776}
,{141045629, 1581050638, 1162352612, 1237861732, 140611341, 350568662, 1254401945, 1335691963, 2142725348}
,{450278960, 1456614801, 309105943, 1667973816, 1649590473, 1299017165, 391320847, 135147149, 421172965}
,{1165316584, 1750897556, 2055628944, 1808267138, 1740921876, 1772533481, 1542616408, 2127344854, 352106083}
,{394543716, 1862152511, 1787270994, 1274896022, 790812510, 1294565481, 1441547258, 555606117, 274227822}
,{982982717, 1889520909, 1179966927, 333641025, 311788664, 1223354411, 1932908889, 623490555, 1704179705}
,{1516248862, 403211973, 453694848, 474468131, 14028452, 87273236, 2141136133, 1120934855, 1232313988}
,{924178515, 202592853, 1202277513, 848451657, 204373390, 1489743108, 1539860884, 738997077, 1771085513}
,{182337220, 195429464, 509950967, 1073876731, 471922239, 131611437, 141764417, 702136638, 11426051}
,{1023801411, 1832325720, 1273670033, 2060851617, 1894422702, 507627585, 1632871290, 415604842, 1963709372}
,{226034514, 850325955, 1545680774, 971129977, 1625285548, 126968781, 1608615883, 730236965, 1906573522}
,{747155155, 570488806, 1821436846, 1996064636, 144020266, 1919597237, 827455266, 203216740, 72894407}
,{1975350561, 876678435, 249400763, 383923575, 926675840, 292679921, 254336973, 1690770005, 871777250}
,{917905313, 929893082, 516017221, 1759864350, 1655623308, 2004490593, 1210309884, 1765752620, 1037609891}
,{543993721, 2001775432, 322279260, 717620696, 1783225709, 1918751812, 939206314, 1892137039, 1013603563}
,{186436467, 671457816, 1693225740, 1166392421, 737923596, 1357824329, 36871219, 1046113312, 1660274125}
,{1488529632, 477941971, 335281994, 665823556, 2031411124, 1245446788, 758369077, 78271832, 1145022524}
,{662064504, 1002313423, 705626374, 2060483791, 607064717, 1200087576, 2003582640, 1359229867, 882953540}
,{1160475030, 1307547777, 1554940538, 1318758211, 414607606, 1975123340, 863647369, 1982849273, 1116474731}
,{1569356090, 50348008, 938418331, 630542835, 1234165364, 574815200, 737233011, 1553627287, 1442015090}
,{177222733, 1466718577, 2030522915, 1369297038, 1751436145, 653071344, 83961027, 2091529955, 482529749}
,{1331412249, 574263205, 1284069591, 365232225, 1270557298, 1074636559, 800572613, 1480896181, 859617817}
,{1013758102, 331582107, 129436902, 1036033196, 2106683595, 191515136, 1780393522, 627643828, 598518067}
,{217726531, 1018298656, 793316383, 1812503834, 1262328712, 2123210376, 1768179669, 12524944, 1687441711}
,{1205301124, 609750432, 1082254582, 1402457235, 1073415867, 617391768, 1725160922, 1013399277, 396603573}
,{1308716800, 406267518, 23878212, 41101612, 788618463, 2095700285, 1974737671, 531059433, 1998612032}
,{1154016204, 1952633736, 750715555, 232798290, 2126262548, 1978224794, 1561575430, 759988713, 1108647222}
,{1291225007, 1008966579, 1981430894, 961702379, 1644001580, 1308657103, 1845523455, 1926381905, 1506638825}
,{148396952, 783917433, 182306651, 1664631949, 57467914, 2020468688, 334136069, 366568319, 1969278467}
,{657018952, 1447289791, 740583510, 1556801876, 1183668090, 358339699, 507790433, 627455948, 1823051967}
,{86983103, 162680032, 776516423, 265704861, 57321662, 454358493, 2013355321, 1815888328, 280754206}
,{775425532, 605704380, 1617239729, 167980740, 1419199813, 1622795632, 190267799, 2141496475, 132805135}
,{773257834, 1715846509, 1928506754, 1558448517, 1561198091, 800961173, 896551067, 1996274920, 862501869}
,{998616948, 248102626, 129012380, 2064178262, 1598704363, 757580153, 1259723697, 1301152421, 1250675198}
,{880343655, 1863373321, 2105035384, 1857159911, 1368714148, 1018382130, 674859815, 1084088064, 1418780055}
,{112602122, 1037404637, 74294504, 782928852, 142802223, 1212742259, 1689944178, 2124183672, 687828126}
,{1091210826, 34030865, 276601208, 1625099308, 760824267, 296352170, 1246619772, 329788929, 1611765783}
,{1856055020, 878072724, 1687970149, 645657301, 987344413, 228453467, 212147065, 1108276740, 599447554}
,{1176760179, 689306696, 1903526300, 106656819, 1929453400, 170066451, 877635401, 2054342258, 995685245}
,{1175550145, 1891557468, 621756590, 1461166639, 887764311, 2016985829, 882179191, 186036281, 930025795}
,{909540393, 1060708465, 98402807, 102147449, 501965717, 2138233698, 1282756295, 1263795481, 167011481}
,{1632029803, 1540865281, 1390654591, 543623286, 529437793, 1291777397, 616409955, 1938492537, 1676326505}
,{1563854753, 437277942, 142170779, 78785716, 410035650, 1285793149, 658510818, 1781042212, 1075161064}
,{718562211, 1268698251, 1424219845, 721223157, 916061315, 50833836, 976931273, 1775161582, 509503126}
,{21223153, 773505485, 1105855199, 812834413, 1717330029, 14458105, 1637665774, 1742889945, 1644189460}
,{1541459338, 1540433296, 2119300515, 878919434, 816910857, 786174163, 18778458, 657598609, 1553118985}
,{578040008, 366442929, 1761954929, 910551342, 1047324287, 1023122385, 1940733006, 875539884, 1170382057}
,{1908336557, 417357461, 633982328, 326626346, 1109520819, 1574800766, 727047154, 1575398395, 21260469}
,{1987084434, 51467765, 1667938435, 524159680, 664697122, 859226766, 2011454161, 2075008553, 1334308499}
,{2046164982, 648932190, 630398854, 37034695, 2076494092, 147522811, 1759881876, 1139062956, 1248325328}
,{289792054, 1407924991, 1580157097, 667929756, 842876142, 662252678, 1133620295, 1881775388, 1043248215}
,{1227599214, 492406667, 1361043840, 442534881, 374253430, 1565985495, 276078005, 882778491, 1246396964}
,{1569635301, 1967918803, 324362964, 487138452, 1831637403, 1525920630, 864032567, 936346330, 430174364}
,{551099401, 2009569442, 1985914516, 1502298049, 478785362, 1924058834, 668248830, 194669135, 2021263900}
,{1621901692, 146999733, 281750777, 574100780, 95682345, 867824329, 944379903, 565690361, 5064901}
,{1408739784, 950044490, 364786612, 2126350301, 31845545, 886482295, 1665130544, 1973051340, 1488761223}
,{1337200839, 1575285233, 70363330, 1540331222, 1862042888, 999890757, 1545762665, 1536185201, 1431752250}
,{865129884, 413898425, 1698251676, 1907289030, 2095282054, 540599695, 972170871, 600661584, 167063582}
,{1845812061, 1171622091, 946728208, 1652686902, 348778754, 632962052, 2013681678, 733149950, 1165918095}
,{1733777947, 2124763533, 465682686, 1313979999, 1286144105, 122076881, 530329960, 1128364513, 139597249}
,{1690063729, 862021189, 524051942, 1049212532, 1146452025, 1236866525, 628019570, 1903147506, 1124595492}
,{907069566, 1008625243, 114070191, 737040565, 1458969473, 1335395989, 613049236, 92976603, 1748550209}
,{2135124612, 1062079847, 1500528407, 1086191315, 822573535, 1603276421, 1046755842, 934575135, 2106084973}
,{1047627501, 257365979, 1973509842, 1292968809, 773996951, 2145554183, 1544688425, 1289198772, 1611267569}
,{164822113, 1672625393, 98994658, 1013619941, 1619336593, 863318377, 1622194326, 1090097021, 1345176818}
,{1974079094, 662816549, 901315725, 1404954277, 1273194618, 1475127419, 277646946, 1952022940, 589986589}
,{1221928966, 2053067688, 1348774124, 1015662022, 1895743936, 122153997, 983897678, 456549470, 662209798}
,{395887936, 1449987956, 755598791, 66651997, 123059575, 881930449, 1413290775, 1182612961, 1964002868}
,{754765805, 1074999961, 270866990, 233463125, 2081150829, 1347702841, 2054028825, 51423948, 774662259}
,{478040520, 1770281840, 1860050135, 1342039192, 222363887, 1602967658, 1084263090, 2036288934, 1012901313}
,{1985398838, 669804933, 1690551760, 1296620093, 420788029, 1907650622, 1237622407, 1615071527, 607132271}
,{1084921550, 722343758, 778045294, 2028913134, 741134510, 1416363039, 174944048, 679192529, 1469784766}
} /* End of byte 5 */
,/* Byte 6 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{1010845446, 1185423608, 974952887, 195798204, 965458505, 1475872796, 28900679, 78308784, 1367547160}
,{828093520, 1013625014, 896623716, 1597555669, 1511507719, 723792282, 858044782, 1511988360, 982311793}
,{606719862, 1575050371, 1434853565, 75277686, 1646024060, 1577446784, 1312243920, 1763475961, 1136181656}
,{1950229895, 722176570, 409670134, 864750680, 875145109, 1650208842, 46845404, 1277851518, 213148713}
,{1710662475, 777357780, 478529824, 416549721, 833494699, 1789238649, 818663938, 2062752625, 648402192}
,{55502294, 461865017, 1430680971, 901367456, 807489348, 1945677767, 1501313029, 1342275537, 15570363}
,{1717803522, 882607222, 1121757679, 1449874744, 1508206323, 496124354, 303869061, 1450894755, 463522247}
,{361689396, 783372337, 534639621, 855889947, 1416274984, 227284319, 347715927, 1485808649, 1862609598}
,{2044039206, 211339401, 1659135387, 2038311486, 1101016176, 140170773, 858456340, 1737851767, 1244387334}
,{526563173, 325237807, 1120876135, 272741475, 791298515, 1536805067, 1334613638, 1738361984, 612368550}
,{1159281593, 277260141, 1956808148, 1606202289, 145864340, 1267375954, 868981470, 228316951, 1247069413}
,{2094176893, 1943935479, 450852161, 1956585935, 625506981, 459354290, 1363819983, 1801371488, 854020419}
,{1165308637, 1184546107, 1354767667, 710810987, 1504234539, 454565537, 873547182, 619817838, 1072549931}
,{719472700, 1974265941, 803529679, 342444041, 402662035, 1722450496, 1918658923, 2140519612, 941970359}
,{470528619, 1294051152, 956937760, 56193556, 1218572465, 1021991953, 1995104947, 109526101, 1287612512}
,{52477752, 1549883615, 1746505134, 359921126, 1131246672, 672522943, 1833244651, 899401504, 1245952875}
,{525376659, 49874804, 277863398, 1570446196, 2022440287, 1906102438, 1543462827, 1781853483, 1720914115}
,{1724729931, 854099051, 1462999730, 737555328, 1270392188, 1431559581, 1069328059, 255916325, 1717692386}
,{614853198, 637782648, 1019750612, 101090320, 560987930, 207835664, 1376504859, 1659423084, 409686580}
,{575374891, 1329809298, 955397260, 1529414017, 571881912, 162060445, 1435547016, 597475832, 1651255559}
,{2004278544, 1166179260, 348435514, 1090688254, 1295265830, 894178499, 2075582504, 793731107, 1769079219}
,{368310082, 1380266357, 1565439336, 934082844, 281763984, 1466106193, 300682892, 2117334020, 1688180724}
,{1993165711, 116954790, 587647901, 346894244, 249534534, 600462797, 55363375, 1938129798, 1503924387}
,{1152693577, 1858223392, 1790528771, 353006094, 1538981208, 1132658093, 642479162, 342886752, 559977386}
,{532509331, 1417902496, 1385748873, 577583374, 409658593, 265156589, 111828061, 645817735, 1794235121}
,{106203131, 293680559, 1932393013, 2041452929, 1188479665, 1437625469, 248108093, 1202995820, 767431128}
,{1094414788, 1376456798, 1870212587, 1785475904, 1316671204, 1852405081, 1329244991, 353872516, 763319094}
,{1746992353, 1924720836, 1875978055, 844470466, 1600615262, 561654236, 1249902343, 729127193, 593801468}
,{918093300, 1541539892, 1546694822, 1806315871, 1552718003, 1220207237, 797499742, 1741939326, 1587284333}
,{1194747700, 493036158, 14781403, 379304211, 1628098847, 74874463, 310697888, 1129553409, 51639148}
,{908180984, 246197903, 280984950, 377923822, 1480322790, 357831288, 1014737458, 1955500994, 151652348}
,{860790103, 1419580720, 575506682, 776247381, 652000714, 1440426731, 1252473332, 1626371903, 404893095}
,{1095997185, 410359166, 1692924060, 1276460712, 402897431, 563631063, 1817775735, 103368293, 1762582899}
,{700788643, 373695532, 270740455, 216351396, 1671787384, 450542054, 1616289289, 617017786, 2100076253}
,{295209045, 144860673, 793138207, 970948928, 1291134404, 578408419, 483321078, 587038186, 1729916133}
,{160721774, 558865370, 34735709, 347523080, 1067829521, 2005851302, 1694919765, 291172074, 321674201}
,{987286429, 334815601, 585053518, 1384535913, 397784349, 955632992, 1539072159, 738486475, 1749926818}
,{433989762, 1826566807, 864164120, 1995303451, 708363326, 1889418755, 1813480291, 394217708, 1429380587}
,{495658462, 440182804, 1136235181, 819702880, 1863058362, 481450707, 1719346710, 1291673355, 874101776}
,{1523336587, 92797828, 1127202006, 706923666, 178724560, 311249299, 910890569, 1354479608, 563650156}
,{116840234, 1861664299, 465940088, 1666216813, 487147898, 1176525123, 376278842, 1749267583, 197026791}
,{934192639, 1676945721, 634487788, 468984439, 1356119833, 1242604115, 1810189083, 157540807, 2052599827}
,{1032313233, 1187764914, 478906207, 1464469376, 1049064497, 74836006, 1096958699, 973159649, 1674826261}
,{383880664, 384255930, 614654692, 728784140, 524624967, 679394427, 1128485919, 226439886, 1089616101}
,{1013951152, 978141347, 631255539, 1505843583, 731232498, 1911748632, 1250633706, 1712366880, 131904090}
,{1326859209, 953030753, 856638458, 1262237166, 2072207490, 1454997671, 1528505279, 619886136, 1425750613}
,{952621074, 39719954, 304987824, 1269739364, 1760181396, 1209541131, 1062764434, 253526718, 1456698680}
,{155845806, 1881411401, 1891472622, 1505591519, 1989851787, 323626816, 1348706507, 1785163518, 1150296191}
,{976412265, 888267828, 25044846, 1954418857, 2129542266, 1083394447, 397476066, 312063681, 2127694160}
,{1969368663, 1422688259, 1975063752, 1394555613, 758537854, 1055531774, 961382537, 735949178, 286207277}
,{165338260, 353685534, 1993994414, 908647905, 478817096, 295817143, 65097314, 241618751, 1575366539}
,{1055590865, 1636192854, 2055075841, 1889480820, 566744125, 1803606963, 1745935951, 312035596, 28034937}
,{1654678319, 720050555, 97658948, 1485260551, 1600209313, 33538961, 193294494, 773362384, 1759029208}
,{1377364732, 1972978827, 1567042576, 967841335, 935732769, 631907712, 486881418, 582684474, 87548109}
,{1190991179, 63425356, 1922068486, 1194518606, 135393121, 1686687908, 43237530, 105643647, 850037121}
,{358186177, 1567743821, 1072977755, 965443735, 1366375392, 1120256112, 1969378304, 705658058, 904646707}
,{830857982, 2022444985, 1682604904, 1753794190, 39995287, 1946393056, 571870605, 2090510390, 382058474}
,{2090688986, 772760841, 296024782, 1479792825, 1503637810, 685264790, 1129923929, 2050687091, 1967013334}
,{1965362624, 900668696, 1469804182, 1494934601, 1560439068, 642288507, 1996622481, 903490915, 707346336}
,{597301364, 925705327, 1952190270, 579191448, 1255044571, 1146146579, 1443664785, 610779685, 1578142593}
,{2004805839, 1940194343, 295537613, 1485421396, 988765874, 1131138117, 1725000372, 22677807, 1732447918}
,{1682179737, 1174549864, 1975962520, 566532675, 522782980, 263664409, 702563971, 1501751291, 1178286034}
,{1760275605, 600942796, 656286365, 2027028135, 1363349910, 521717246, 1117930423, 1957875034, 33848554}
,{394152262, 1033695231, 1526975574, 1730773736, 1926374746, 1110490304, 1567800697, 859920945, 14789680}
,{674887435, 447722514, 1637932825, 1668104124, 617943494, 1822124647, 1751808475, 518100246, 1080853502}
,{1275180795, 1521726288, 579655008, 490405482, 1006731037, 538542098, 176416909, 1395027205, 1055342427}
,{528017271, 1263741913, 1116718073, 1547155479, 462645228, 112568227, 1828938470, 1980768838, 1702111417}
,{2146131034, 1006033138, 1745886263, 1284508627, 577428485, 1758125829, 2096909864, 980086904, 663894798}
,{1327300304, 691852480, 1731280405, 1603820906, 446452113, 656794103, 1823525016, 536090421, 366194648}
,{185624074, 1421804447, 114947535, 697837090, 1873211202, 712590927, 617159987, 1233450371, 475135931}
,{1886262113, 2090975260, 229139864, 469123268, 484951768, 679164244, 420929108, 1180791539, 2111784194}
,{666314209, 2031249400, 1608192009, 1430195362, 1956791285, 616923421, 1499642994, 261075220, 1865414958}
,{446720126, 652938829, 244877894, 1898982593, 811012452, 1140831879, 1885696201, 1346363105, 876656277}
,{197388827, 1560289682, 1440707254, 1896293631, 918982494, 95339222, 1622086891, 362195578, 577465677}
,{1431327477, 570026073, 1861019631, 1121525940, 531898347, 1894422556, 768475007, 1604178964, 399932439}
,{1537642109, 294115567, 1786261383, 241706527, 1811459693, 722308470, 459711595, 1691420204, 1648959351}
,{879869690, 1483433393, 1242245767, 1540715016, 1369398001, 101737178, 991889390, 119811906, 1971708928}
,{951995167, 1113918890, 1448773123, 83424507, 1157047979, 1532339858, 697705808, 698220933, 161051376}
,{1265192764, 964797552, 235909758, 98891367, 2120873526, 951476814, 1406131426, 250990462, 100291582}
,{807388021, 1176912172, 612357473, 644080045, 797375845, 1001700547, 1013381558, 74648135, 234378086}
,{1704083175, 743851036, 1929369502, 595467590, 187789086, 1758034660, 1314057371, 1316663516, 1444943773}
,{637282926, 984196669, 2072191940, 1622017991, 1948649752, 1405392739, 1472930299, 999118204, 2028455110}
,{1438729674, 2057176357, 650544771, 173748494, 1006983618, 909375877, 10140667, 921818537, 362224583}
,{1223335986, 13454721, 1950141114, 1227290268, 1108684251, 229532220, 1869804837, 250736028, 1879920100}
,{20678459, 1961419548, 959974339, 595228733, 577450390, 1846807905, 850462484, 727063593, 1718842503}
,{1471171607, 923702165, 1867738696, 370599106, 772471217, 1160411379, 1354220531, 1077737926, 1039508037}
,{341034651, 1365402302, 964874569, 596705068, 1038814725, 279672533, 115369906, 1200570695, 81744070}
,{767929018, 178105345, 213994769, 354693950, 2076771569, 1842065471, 750387860, 92058199, 1937177017}
,{1869471583, 740620837, 1741872220, 1864535522, 673778026, 408428330, 525180481, 1788096536, 1765931156}
,{2010363455, 516804780, 1939951973, 2061420823, 527713027, 1423790306, 2085870460, 1331366162, 276483235}
,{18710932, 797409125, 535553951, 2067035857, 1202354299, 2079350487, 559400557, 1549123115, 123290297}
,{1549170331, 1118081741, 1319124961, 1388243350, 14562507, 1756988131, 1885295121, 835764240, 1869528670}
,{208131581, 669703155, 1384192103, 1123970426, 1984726690, 704231284, 1567207883, 897570214, 1026332021}
,{557649935, 1947454842, 885745216, 1962098832, 259401207, 1215406453, 1856217328, 23932579, 143650622}
,{437088638, 2032397051, 1138071816, 1739523969, 145066102, 1664667663, 877560681, 1539751913, 1002017303}
,{1714860078, 185379908, 1018700536, 1606935508, 891255932, 1517914163, 809553126, 1967837008, 1061506438}
,{221982098, 976419092, 46159185, 972685256, 765860331, 683976993, 1202631608, 1410757652, 1560585341}
,{642022817, 880466959, 2117288053, 596480403, 1533485489, 333636426, 497276751, 1603549546, 1616488242}
,{775370766, 65398679, 2033003269, 523119767, 1779188765, 438264005, 268998132, 1717460609, 2144445193}
,{1518809974, 834229170, 77683172, 67483959, 1902386099, 1805884232, 734113879, 1850653566, 1898902445}
,{133723487, 500539717, 1707467560, 2044892668, 1825367778, 1205355773, 533973763, 32640751, 847575874}
,{2092542799, 1779245601, 452253966, 1076101823, 1886229719, 939481439, 16061280, 1070480375, 974690676}
,{1216422181, 1164500429, 129975712, 1534551617, 1681264021, 846126848, 692824774, 647817852, 595534314}
,{513942070, 1918596498, 436206463, 819641979, 1191256727, 664974950, 1277207054, 1875858063, 856828952}
,{1945568126, 1235149504, 739787128, 1816496219, 1053282634, 1198445754, 1823569667, 1929866112, 1666319683}
,{1503507739, 1080982572, 1142653727, 1081092992, 466037640, 447380681, 1093444671, 1879503363, 130334010}
,{15623656, 957656189, 1272094645, 2145774783, 1185860528, 522691180, 1089152732, 905577868, 1727901733}
,{1233210498, 571730394, 914360178, 247809634, 975190414, 2072842002, 403613842, 321293154, 218492716}
,{1190979421, 501863013, 242806040, 1413351682, 1545822076, 192394398, 2108203283, 997603080, 1196660998}
,{901151912, 937927887, 1845292475, 136158271, 282712870, 1911014906, 1820682352, 787409636, 1725106415}
,{1469868505, 1160745141, 444196245, 1029213162, 1758989177, 1183162076, 1748076436, 2135373327, 1763227789}
,{1849756642, 1101130617, 723499187, 1868021180, 1516163668, 668464157, 167414185, 198813480, 1446578950}
,{1423402603, 2103058676, 556619033, 1712799159, 519814337, 1926990312, 1170662612, 444073786, 1008633606}
,{1203060718, 1788231823, 1631196970, 1503707168, 1175972270, 1152412207, 1979471974, 166646883, 1439768408}
,{505505566, 1840858588, 447371272, 1727928219, 928483086, 160693087, 1142747968, 39476242, 1129414766}
,{1060171767, 1994218857, 715422151, 806386480, 1696381076, 227294368, 1494862581, 390558759, 1452311328}
,{359544634, 637557112, 306131092, 523536355, 1935734286, 2014955043, 2070503021, 2053718127, 1968552218}
,{1813369212, 1560688442, 1450456514, 2026906495, 1387818294, 67015759, 1319626816, 1135540380, 990379161}
,{1838539962, 394368814, 2037599288, 1781561788, 1960359044, 2113111839, 1190635962, 1579587566, 2051748490}
,{489950253, 417652337, 738560623, 2098031883, 1174823902, 1642034119, 56474499, 1970891626, 1129256927}
,{2024409181, 49281830, 1550541871, 2031251228, 168300050, 1595596446, 1636328209, 277420349, 1037570569}
,{524454163, 1224624913, 1528144952, 897756704, 453230916, 1363188503, 1373151523, 739276218, 2127219522}
,{16928798, 1730108553, 151068350, 1524958826, 1780625884, 1509435109, 589133703, 1640884577, 884193735}
,{515563831, 113683729, 192273427, 1870013995, 1156226813, 1114352394, 1305488642, 1627841335, 49611434}
,{1666587842, 1269013217, 895168829, 333754400, 1298488173, 900455837, 257886739, 136119859, 1143472040}
,{153913395, 1182360144, 1938762806, 1611071696, 544951898, 591619544, 13709048, 317783341, 541101911}
,{135872676, 6531323, 365419477, 1434299482, 1980913177, 1676962804, 207979225, 1898377665, 1853351906}
,{292247445, 760282231, 1697539989, 1620593212, 1589010583, 1194866537, 957214154, 641323164, 1794044205}
,{1192713369, 714058468, 993841424, 924806375, 1266380047, 1262976430, 1625541497, 33744304, 1684066270}
,{227215546, 3982018, 99019110, 1555163464, 412004042, 568701671, 1081089531, 210414487, 1089978248}
,{1658185801, 168776531, 1567522392, 464603135, 91815065, 1843269330, 1996119950, 839702976, 1515905941}
,{2074595067, 1716105794, 1427141797, 71890982, 283545152, 1741616797, 495453371, 1079598308, 764679203}
,{1089372847, 2055094822, 901775051, 1810018705, 915668182, 277949955, 267621655, 1179480214, 2006088660}
,{187307701, 6814060, 719892900, 399039918, 138435376, 48630547, 1454491485, 426838810, 1062783616}
,{1973276954, 2098716011, 219461837, 264859058, 1801763605, 763823983, 274245990, 1242614785, 1903719342}
,{1719201297, 1871797522, 601993606, 309069010, 1783404395, 947954521, 377583706, 821159807, 100134092}
,{250761804, 1472462855, 234232391, 1788008917, 778666221, 1814192953, 995129228, 151223499, 618363814}
,{1145860259, 992413067, 176779537, 1666064051, 554948081, 483987794, 1510622362, 1352094589, 1048082616}
,{1779656535, 2145736294, 594667502, 315635018, 592112017, 985985595, 1860185279, 1704679635, 1606086880}
,{1472091638, 1672825750, 868945278, 1153865862, 798184853, 942845312, 1827439607, 1265881276, 2073113324}
,{1831226355, 96460974, 38265473, 2029198463, 1038994939, 1066144485, 201985815, 1421632516, 111926829}
,{1484413106, 877750416, 18690160, 283305159, 1403516219, 1002450923, 2137438531, 716317679, 1045430849}
,{199531952, 1325510499, 280562207, 788167776, 1364709130, 396680857, 1967085007, 851307300, 1066751575}
,{1699009518, 2053245018, 1340802145, 1986009214, 263092834, 1243843322, 1120205347, 198072972, 1482257482}
,{951993205, 703213814, 78898933, 401800135, 1024508440, 906887104, 190985176, 2052942241, 413352838}
,{1864468185, 617518341, 1649854390, 867597031, 520637674, 501825388, 185725316, 373096441, 497428102}
,{1351412907, 786315894, 1796398470, 1575336614, 265035492, 1064940653, 672049965, 1876990038, 1540824261}
,{1308134179, 2099888993, 376689495, 1556853536, 478511498, 1534667848, 698549207, 2063980590, 829860115}
,{847949649, 1932160971, 1800848372, 1458030065, 1849671803, 454262588, 353366278, 1119603503, 1779933124}
,{1517876731, 966179126, 31597992, 1384667406, 2106266160, 1616038276, 1183971513, 218658100, 2029735825}
,{1218161151, 1593427372, 82697157, 593702576, 1893319750, 200703328, 871444451, 1469813024, 1808381921}
,{2078870898, 1037924789, 1911997961, 1245739058, 205110430, 95571610, 804806074, 319811838, 2025911569}
,{965606428, 1992018918, 805966094, 781619926, 1866295335, 365566480, 678017826, 1188558781, 1557673944}
,{563692835, 841613112, 447210399, 1476113984, 230081497, 107449708, 1268596460, 1032105223, 190967216}
,{1692050376, 2103857078, 229057603, 1449771261, 858565626, 1082142717, 1675668752, 4293739, 749688635}
,{363040556, 1267496355, 869053167, 735837050, 1566618318, 710935431, 1801341772, 1393857618, 864019787}
,{1279896421, 1715966533, 2099755248, 291470816, 1192825488, 1375376968, 329883121, 283385906, 1737885515}
,{1672293953, 996982005, 2123851963, 1617038268, 1491008512, 1807543492, 632218282, 1610510234, 283084836}
,{1524551684, 1174059667, 351951849, 1541360882, 153862258, 1069963307, 729083968, 2145174952, 691541213}
,{1131618044, 189976460, 1054910698, 1265288868, 230024170, 223406874, 332473198, 934174884, 332559690}
,{1534702716, 1779253658, 641828153, 1559158331, 1235499978, 812610978, 142665946, 5986697, 911282087}
,{830446513, 586179402, 1790480180, 1653349894, 928387598, 1691387493, 446875281, 1604317728, 1225869852}
,{1226284093, 1144531320, 1604771978, 91250203, 601011083, 2111196258, 651104923, 185486027, 1287710250}
,{733008632, 831962616, 2028627776, 207088981, 1082231389, 1863899783, 1698385399, 1652763556, 74267415}
,{718589286, 157768537, 764748732, 368890814, 1239501057, 550507381, 679445331, 1312200954, 135985629}
,{1850837269, 336063224, 1482091126, 1162879153, 665354075, 654637821, 1360342071, 1867875434, 1514836226}
,{1235104549, 1807386172, 68790908, 434259384, 975275445, 193983574, 1504135210, 981553884, 1061228315}
,{314680622, 2089164201, 809852909, 1222808411, 1570565000, 46290701, 901025346, 1551694634, 383226784}
,{647403625, 1914316497, 1891670322, 1818706052, 716943720, 77112493, 1113061673, 677515190, 1553555227}
,{1728251894, 1572230442, 540569141, 1383311386, 1204201265, 323097185, 964356038, 831038408, 1772331281}
,{1181495294, 2048079723, 80646134, 439671274, 1329480148, 990311276, 1452024105, 468333749, 2131930977}
,{443550534, 18561496, 988670910, 1852465969, 99589213, 1366557362, 725124198, 1383924135, 1225828501}
,{651157109, 1166605760, 1075620610, 1640617495, 585405304, 1905068413, 1804711036, 579471903, 704712685}
,{1235958473, 251825312, 2085045576, 1475020588, 1171110748, 1215104217, 1337636553, 255228518, 867032402}
,{1981430227, 1089229747, 1975112651, 1190752420, 155160904, 1147348444, 2003037425, 1026560233, 1191729193}
,{431221025, 800454120, 1251775514, 1577814661, 595004276, 1186397673, 84917052, 689100234, 1857057747}
,{1436353394, 38819173, 1409652767, 1592278556, 611342388, 1039334038, 831137489, 120264429, 1535617479}
,{1224417350, 1266215705, 1270573229, 333129666, 1196758208, 1911411456, 172082490, 1346642618, 510684927}
,{1915837864, 488774328, 1456331758, 931445061, 797530669, 247727805, 1906497938, 49422418, 565484458}
,{157042262, 305805368, 885147765, 571124343, 136621128, 465588941, 829565818, 66027942, 328259713}
,{447844894, 2108887356, 635963100, 194225258, 1189269498, 1307896549, 1144566949, 759994467, 1674890711}
,{235921430, 217073732, 1975847267, 674388414, 733171080, 1208243055, 1016091286, 963001558, 181372170}
,{1618858950, 952088189, 2015255392, 409012218, 718158412, 1459229380, 1504525109, 848796071, 1602195793}
,{855134801, 403232160, 124414343, 982607281, 1536141395, 211309162, 265204501, 997159888, 2046128893}
,{594613820, 229903003, 1299331526, 1127040798, 610053584, 21788166, 1935950762, 664206396, 2095568263}
,{220087134, 1855874516, 2018547371, 124912172, 1800157864, 1134598925, 10825012, 1409613606, 1951038196}
,{1817722740, 203775416, 1553712757, 409854119, 742467003, 1715056104, 1382939850, 999566995, 2003231290}
,{970153623, 371226168, 547091829, 1342675502, 763886749, 353794947, 1174882874, 287221402, 878107623}
,{1697812109, 99921527, 755864798, 959261293, 477780636, 472054930, 1664387396, 2094712622, 2106863230}
,{1402959742, 1197151372, 1609212108, 227064894, 621993330, 920863827, 2038529045, 50450640, 604335606}
,{1387406191, 751516967, 1493786423, 579807091, 925269663, 1447040806, 1631567235, 1619371294, 1861728263}
,{1985472893, 2121652471, 1693982143, 2134702283, 666536199, 675117682, 1672439319, 1441974339, 54470998}
,{1476035136, 113235308, 328289661, 979114124, 1701948096, 1314609928, 727169645, 1261475660, 1260431615}
,{706485061, 799248330, 1405811264, 676178922, 769388347, 659352741, 1282411987, 978954003, 748876993}
,{2124546010, 756291060, 453283426, 653194229, 1898517096, 1086441780, 73028803, 949756695, 1029231341}
,{712379820, 1988809669, 2022765582, 522852556, 2098181671, 1739476732, 554955267, 1080237489, 538982544}
,{1320395105, 1581326962, 150040327, 3505938, 867895321, 1656059587, 322038748, 749785835, 68599367}
,{1360295476, 1493619893, 589269453, 172649957, 239771678, 588019747, 1135081818, 1708106603, 2111540561}
,{462812243, 978145801, 61650069, 1594699991, 52341322, 1138284063, 2132856841, 955574279, 1757999789}
,{1382732896, 319464357, 1424345265, 921495102, 1887618662, 1553494761, 987996281, 683070586, 145305404}
,{1368067642, 167164317, 1477778839, 1233709611, 435730739, 2050572654, 115833316, 763671221, 275819994}
,{134349804, 1498862611, 1879177119, 631145665, 1542693692, 1881336909, 39593195, 80417135, 977676783}
,{661069036, 1302979431, 369735587, 1346776540, 549222091, 642108539, 37623416, 904111615, 1843395761}
,{147750189, 1045429758, 1682017745, 839744234, 1098758052, 1960084833, 244368777, 432092615, 410654725}
,{1767102877, 1170715631, 74783703, 532235913, 1959539218, 230107077, 1819607326, 539618443, 2092795310}
,{1680377928, 696191098, 1989714369, 712178422, 1191914775, 967052282, 1865994435, 1669693105, 1144393100}
,{1454217230, 1686779397, 1388321145, 305229996, 137889119, 104061543, 1674022152, 1007812431, 966270774}
,{389270859, 1413430316, 700124209, 194339174, 1763344432, 1169811333, 1686554613, 999909430, 339634308}
,{861758454, 770349644, 320860354, 126875728, 868870218, 1971972738, 1563405178, 1512419198, 124775134}
,{333108050, 1681124953, 2070611986, 1253979125, 1127551672, 490795312, 1039840168, 1749028525, 1052262600}
,{1412535219, 356999255, 1831275468, 635521716, 1062536982, 1902721393, 413703704, 920222149, 1706826871}
,{1419813009, 1324372170, 1293393077, 327074140, 617747368, 1793805258, 450812872, 1457779823, 429646977}
,{914873828, 1069956859, 87051997, 753649970, 539450382, 1608744478, 1456726712, 1532285809, 969517990}
,{1352814085, 1738101383, 1158010126, 1532000311, 2044785652, 1801487365, 1064360460, 1159474150, 565227876}
,{414126159, 1352825814, 419485056, 1493384831, 876534663, 1100507476, 1752294357, 557959259, 573874740}
,{817331101, 521188922, 1985185985, 1541348736, 2066038716, 342497219, 1757937776, 496573785, 281625156}
,{1465038212, 1375477334, 1426233888, 1273061535, 1213063515, 3720501, 1532305895, 199883560, 1143104269}
,{640629292, 1746851030, 324715836, 727850830, 1384158595, 663637319, 2132552239, 2021567011, 1857726147}
,{1006240350, 2092955691, 175348654, 246783985, 2120763656, 667029745, 478759155, 1689831016, 1099826071}
,{735547620, 1052371432, 293551558, 1984674999, 1540412286, 794929765, 552197052, 932412572, 355074427}
,{1569238996, 1080514366, 1157852736, 1528964891, 1107658914, 101783988, 1981221799, 728163079, 745427654}
,{1569905993, 508688512, 524479433, 743317116, 538878721, 1355085785, 587070320, 537842504, 780130574}
,{459189544, 941577011, 1944266543, 1050304180, 1182200761, 1210357400, 1520641453, 1913944042, 727068711}
,{358573535, 1742250773, 1328282583, 1456948849, 317621401, 1079879124, 1379821619, 866637100, 14298416}
,{1692281929, 443545632, 1542969037, 1712807743, 81617219, 817614598, 1042055563, 1670210843, 1966649618}
,{1159563979, 1346689841, 207402004, 1354375916, 1799747049, 833468266, 559933205, 1695270206, 1850647571}
,{77583628, 1410634957, 1550810274, 1576293861, 1692175586, 94990499, 1410189417, 760605018, 1211215465}
,{1081712333, 691647348, 1836557614, 211872894, 183880968, 1708912402, 1453186638, 1635992853, 1007147840}
,{42985943, 1473289322, 398595697, 852100772, 1682339335, 1780366789, 741834962, 807206900, 1399816920}
,{545752163, 805272411, 502464095, 638009632, 2102420325, 936381701, 282104682, 754021358, 1623764485}
,{90000082, 2081493328, 979976239, 628640820, 118181986, 117258017, 616813386, 1057001229, 714599197}
,{53654646, 915260015, 863679821, 1688144697, 1179918901, 1540725983, 646548274, 817607707, 1645375480}
,{601945656, 1895990800, 2005381556, 599549580, 1907843711, 1255813091, 599479333, 1785596184, 1712381776}
,{84177897, 1532216229, 524197958, 1726488769, 1600831470, 1180631418, 2043712244, 1440686213, 344972019}
,{725116824, 1988964952, 553537649, 945408061, 1918507708, 17187146, 1191767242, 743037891, 692076805}
,{2099513153, 898978318, 1728976928, 2002894257, 2039558964, 1560013085, 1719843260, 1400949116, 1119730044}
,{814698344, 1094584584, 410622880, 1990273374, 1139331086, 1238936893, 1372859338, 1295024241, 793446784}
,{1325173476, 1254408483, 1076854655, 563665107, 53368546, 361101037, 628346745, 880614658, 1677888698}
,{1893726577, 1740341913, 547765510, 2124011856, 501205878, 755415064, 712866967, 478644047, 1085299738}
,{396063144, 801833287, 536987950, 1671298939, 55624080, 1092707916, 1872527097, 693791908, 671790371}
,{232882802, 421555040, 1902771075, 2074665671, 1850593817, 554794424, 2138089756, 896837527, 140836312}
,{2069131491, 1835264649, 1876893688, 152435881, 2009740700, 144700986, 323009790, 1113124278, 1405398838}
,{2052675473, 312136643, 1425632334, 330183317, 81986941, 989564598, 864536360, 27595591, 1253444490}
,{1321432901, 589325433, 2071807871, 847307776, 1417233122, 234198100, 1989838536, 1868851206, 1407380294}
,{1275284619, 1104066245, 1062072368, 253477458, 114707620, 550342446, 1513697606, 367928390, 1059674507}
,{1025267939, 835876814, 1906456079, 1512682204, 985675854, 369658853, 1025456204, 1583269262, 646279977}
,{210087529, 757150429, 1035237871, 964267497, 1613355547, 1649987435, 2068355004, 669839975, 842040644}
,{525567614, 2024050845, 623372761, 2133979232, 998849717, 1195857086, 849067875, 1299934564, 973369349}
,{1996671228, 35347112, 1425075323, 179965782, 2075848024, 195505641, 1464022531, 848673902, 1993560523}
,{1029272821, 742788717, 1647480326, 2128398998, 1351030436, 489917412, 617015773, 1160824201, 1052116148}
,{226233016, 1666440786, 1118066567, 2060000179, 1193699960, 1157762501, 820223874, 1128676729, 1765030746}
,{164012664, 136322314, 2044043022, 615148455, 1466870401, 508414611, 899730267, 1051862138, 883970288}
,{1427074205, 1298389145, 1630221688, 890255115, 435322111, 1335784085, 1699568170, 1369148079, 1996229748}
,{1885655985, 285217693, 933012032, 1961377935, 530210647, 162109010, 1950015702, 1493620804, 436940095}
,{999543306, 729294811, 700230177, 1976303071, 343604427, 1023699748, 1200928724, 1985474747, 400307542}
} /* End of byte 6 */
,/* Byte 7 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{123134746, 357661612, 104256492, 1229661191, 1116680535, 958809545, 305544063, 1444843316, 181488645}
,{396136447, 1687430198, 748674123, 1484021485, 717422, 152918071, 1876268804, 1256824133, 956693346}
,{154523851, 419495223, 827453217, 549929154, 1566708865, 1546527881, 393542641, 519563412, 525868212}
,{1169955142, 367211989, 338284947, 449836392, 1277278137, 172476054, 1781156881, 2081289313, 1944691248}
,{1002152021, 1015271219, 2125823378, 1401121081, 1928802436, 771892194, 1211416016, 547628746, 233893822}
,{1510960642, 519025684, 955990378, 1599515246, 17870639, 204937824, 1313758869, 2106067283, 1750353284}
,{43477722, 648488344, 305585371, 2079516671, 714179246, 848019506, 653023638, 1636210033, 1537370105}
,{1955197330, 809764655, 31651894, 1874633754, 1514230371, 1199187413, 1007800670, 448161696, 927877048}
,{267445455, 1035225757, 1608763456, 943679023, 23373955, 404748936, 1037604599, 473143231, 1038889830}
,{178320416, 1487081255, 1530185881, 1474067942, 722750938, 995172380, 1181875548, 1919404959, 2091068667}
,{1079149199, 107629429, 1047615057, 394548648, 1815054404, 1770036674, 1931340774, 1221817930, 313676305}
,{1787481863, 1327168105, 519699733, 586585656, 789698988, 1023672000, 161414533, 1925954940, 115614759}
,{558447235, 1567665171, 626980659, 1928454829, 22596000, 835337727, 1086231731, 581906082, 33677236}
,{1987519732, 1333468186, 772294508, 1235628653, 1028062522, 1926326468, 957073703, 1222775404, 1029587860}
,{1485289135, 676513693, 650039679, 2012721656, 1501415537, 1767275231, 1212609354, 944063288, 2029986845}
,{1757528319, 2123349701, 1516538111, 1399043737, 2079081112, 1400840607, 1777863175, 239920714, 1350863768}
,{819998593, 1638441127, 212773985, 1129804444, 210736184, 599822343, 324958712, 157075436, 1873746699}
,{103362981, 757523307, 49138205, 1894481598, 1293240942, 828720446, 403331143, 2075438245, 1103708468}
,{216707348, 1862116660, 2049932366, 657733570, 338824767, 497778900, 1875709751, 1182351163, 247124407}
,{1317946100, 2098850140, 357562854, 802614329, 50741583, 1637569005, 321284614, 45919620, 34669716}
,{593530710, 952801018, 1719059649, 727401232, 2118065055, 1818807093, 330857890, 1685342794, 883949983}
,{1194618149, 1297022630, 1616552053, 1289189142, 205441071, 1845273919, 863331999, 879648760, 2045047652}
,{217374305, 1709626221, 1950758475, 454630320, 1184721817, 22932479, 1835357925, 349125958, 847779367}
,{1029482705, 1843267316, 927900013, 244399144, 1924214964, 2019819281, 328337447, 881100314, 971584760}
,{1933172375, 534634968, 221159979, 62461391, 291658017, 1468637372, 995627087, 465611187, 880313378}
,{458867925, 1768609119, 1927693457, 1640085685, 1260336071, 2144876257, 854864220, 1340883208, 1650020407}
,{1257464469, 1334507532, 910242415, 435285966, 1938172564, 1860762609, 1225343143, 328538413, 1071564843}
,{944344329, 445820188, 876346820, 2139741678, 745611109, 1081667314, 2001001813, 687771767, 1256510267}
,{909742161, 991489917, 946917446, 624982845, 1816586512, 780454343, 995880973, 1672288875, 694414494}
,{1638654592, 38392454, 290288079, 2138029754, 972984468, 753353535, 1637227077, 1242877730, 1611719086}
,{649505098, 1503823967, 1769665279, 840350357, 617397356, 1061748818, 1183304828, 1285187126, 811826956}
,{365975818, 1100112637, 178870811, 1006959507, 1107187687, 757272736, 1631812271, 1616941218, 326509896}
,{835461178, 1529712136, 1071701251, 420241968, 696719054, 1022552359, 606433743, 530112705, 1048050519}
,{859342526, 2083997480, 1509895055, 255081811, 1925761030, 279989043, 1734021437, 1473535408, 121533449}
,{1684291005, 1076150484, 1319813325, 282677286, 486233308, 1023519544, 1391078214, 1299038464, 461066575}
,{479148728, 20191624, 1738039251, 1605678091, 998092391, 711483028, 200530003, 1857615137, 25804546}
,{1955745423, 1342681585, 200839647, 183529143, 1704460602, 980293234, 1360975568, 566181153, 754677922}
,{1929398051, 1500307579, 568397962, 722035651, 818242665, 2006134267, 617870245, 1042651977, 131191926}
,{1132056049, 1033461125, 1598046432, 734894765, 113016256, 1603609149, 2003150871, 1214907966, 332043562}
,{1122716082, 844309105, 471629067, 1711793203, 1606520227, 666106841, 345865855, 1053869242, 1452414357}
,{1380319497, 1303238881, 376628073, 1037858515, 1537936653, 498004510, 1276068318, 376486239, 1298792287}
,{645053139, 419604338, 1205927636, 1351452207, 641964721, 86461557, 853543544, 1482208696, 1841178595}
,{1773102801, 1347411361, 448860392, 1350316826, 1694429841, 834199563, 479013092, 323668785, 2118016678}
,{493611063, 1242524537, 811937553, 321221767, 1196674846, 1391883212, 1613268617, 1581938851, 1824112254}
,{1776077069, 161162207, 575359695, 173959434, 1190549670, 505902874, 1901532242, 1870655161, 643194546}
,{752523824, 1852226620, 901805717, 1656680817, 438854656, 2018340011, 1376502719, 1067530683, 387341464}
,{610109021, 1340189714, 1535247701, 1177371906, 802013885, 1234025739, 995964510, 1902526151, 1317245738}
,{231337308, 1768528506, 1971206979, 927585015, 2076395669, 399070260, 1831532388, 1145542138, 1460707556}
,{1934301123, 521769116, 2102348847, 1878197934, 218459429, 166886270, 970196829, 357513038, 2026065148}
,{691443805, 35091355, 581482149, 98352104, 275996119, 329956935, 2076609957, 1196899719, 1897190682}
,{1950441314, 1624110634, 1978505919, 637428475, 932290083, 297211080, 1874482041, 88733943, 1223355750}
,{604246333, 1800024561, 1314380476, 655253889, 16171487, 836919068, 1880452261, 104469780, 2127090711}
,{693453318, 1665601489, 1082256815, 1630104304, 846528218, 1208144463, 981528800, 1286569224, 825725719}
,{1122078057, 533842925, 717613666, 157780433, 1270514852, 1966476705, 566217731, 1140720233, 119643494}
,{940969191, 859636130, 2054240760, 1183057747, 1250592019, 1711412919, 1347872056, 281805798, 937211625}
,{1304989420, 260931082, 2089357219, 1473551039, 121439495, 1951668432, 547279809, 100887100, 86129010}
,{817032242, 968882242, 1417430239, 158449770, 573157175, 897669322, 1317994341, 1952009580, 2037437469}
,{495787503, 1531979357, 526578849, 986091431, 1576038241, 1680576278, 1871112434, 1834440632, 74990561}
,{2030173550, 1823828103, 718730298, 907530048, 1481671682, 39525537, 382773813, 21939937, 1151870157}
,{804145889, 1986813482, 365393209, 2107777028, 670136273, 524246415, 1792519699, 1373020951, 1370694946}
,{114116205, 390256109, 327726188, 1632011914, 998945310, 887186349, 983578607, 2039378678, 70414408}
,{1929517813, 1193096158, 754543938, 1956577998, 1822326094, 1913629294, 1588012238, 1186025350, 333475398}
,{1403902969, 1046173105, 923226369, 783700865, 814678490, 1521174707, 1142434498, 169100426, 1399738435}
,{2002562923, 631740123, 1657669595, 1651095749, 23188448, 252965647, 173267248, 301962024, 346998469}
,{1779071840, 1712321011, 35389996, 871382132, 323274507, 1089645867, 405633219, 622036323, 427684341}
,{759770986, 628853263, 1649119937, 27237910, 1868282764, 1007178512, 437613999, 1914182475, 661447986}
,{1493664113, 2129087710, 1589601093, 1351757055, 1645288278, 854685994, 646215901, 1232955672, 354760334}
,{1656655012, 343558513, 920556065, 666868010, 632059893, 1052219447, 1303898535, 1821116258, 1927404485}
,{1317059597, 71798954, 1091813165, 1075147026, 804367135, 1436189195, 52276035, 519979502, 837122674}
,{409071251, 1726720190, 1998798300, 1202035050, 1408285204, 617177611, 816418495, 379417665, 1812359933}
,{1050602744, 1161995021, 1295671741, 347700798, 455831377, 1223780790, 1941090288, 527312656, 1989258865}
,{2051952214, 748096979, 469039155, 604853365, 567580534, 1533152257, 1757796965, 526495339, 275932983}
,{1880707453, 772702515, 2055513506, 1867908902, 886073699, 1510319277, 1204779212, 587242719, 573164565}
,{1553511729, 777033341, 1247316133, 1762832694, 1217669029, 1199671877, 505308429, 404858738, 861220106}
,{332196869, 1881029640, 1963531674, 785714231, 1889932985, 583644730, 685514195, 8913351, 920445671}
,{1537111571, 1198629333, 522311975, 674948217, 1028924418, 1841756289, 844064915, 1990138232, 1620866225}
,{1691351454, 1329512344, 192125752, 1713168834, 634471733, 93294241, 266846950, 546030578, 228612666}
,{178708143, 411134228, 1434547076, 1205479772, 1204360462, 670708925, 1736669864, 1027142049, 1811228386}
,{988047804, 57134554, 2143507112, 1674361385, 469439577, 140814552, 1235228560, 1242031389, 1001096232}
,{577071902, 154408480, 1971573581, 2105557368, 2043767443, 167181679, 1805685811, 507199693, 1114628274}
,{661869431, 366751650, 1941249141, 295777014, 1020007702, 1316213355, 943703555, 1576093505, 1005245887}
,{1024775032, 325769993, 112634957, 65488660, 107781310, 1255588920, 1820662482, 1790488803, 1950716423}
,{95552488, 11076767, 752912906, 1502592946, 712124365, 836626855, 2070706242, 992594126, 1008961515}
,{1733831738, 1626943, 1857558877, 84614427, 398175990, 1327521117, 1070803939, 1749942513, 1181560481}
,{1510109028, 1709884404, 1947975832, 281944970, 1077398845, 2040339703, 1555979483, 59474698, 841834336}
,{816353512, 1931931372, 435983369, 2098719274, 1035665361, 787214184, 589955134, 1247989883, 920991449}
,{1357326146, 750147160, 1795789312, 1636816166, 952014113, 919328103, 1796147023, 1330287255, 2097026309}
,{699457710, 384881319, 1474981216, 74678464, 971872898, 1213812944, 1479643415, 1167416004, 992774026}
,{630775737, 30839447, 939486355, 1379800231, 939021574, 1635429039, 259712009, 107785022, 1116045181}
,{1667420549, 376431534, 732886380, 1459005163, 974079481, 759201983, 789288233, 431135005, 1360053141}
,{2026629903, 769576791, 174822861, 727950213, 306341689, 382378872, 1329480444, 692128787, 815715890}
,{819828940, 1311923591, 1082926478, 432080517, 528007191, 1024983462, 938930631, 1284570802, 293155775}
,{323261864, 646436355, 1408563606, 181772233, 894950685, 962791432, 212593146, 751609726, 1276132375}
,{937221796, 1887701596, 339903937, 1717912592, 939441620, 1130177238, 1877077186, 1705180671, 1523744391}
,{1041966618, 740543037, 548069840, 1016252284, 200468253, 161106356, 62185886, 1638732318, 1559647224}
,{121360316, 475954507, 433816074, 1438038775, 796866572, 3981667, 1184041767, 1775243433, 1571395741}
,{634487867, 77226572, 2089955548, 57867695, 2042270417, 606775095, 1340713353, 984482392, 838708121}
,{2036759659, 771936113, 870520753, 1359252287, 537835482, 2099179697, 1961168959, 565468969, 1306288984}
,{692086715, 1476506908, 1349448544, 739224196, 522039413, 697405646, 1749601662, 341611979, 295101038}
,{1260596640, 164417415, 76138139, 1383714992, 807514810, 1870692238, 1354131138, 695020729, 1530625196}
,{563665301, 1210692238, 1641390259, 639054915, 1611449251, 1120783565, 1785986923, 336082039, 1386446355}
,{1081329544, 1903523976, 70561138, 59844272, 1429776581, 1954555365, 2070821319, 1375166275, 411597473}
,{1405482938, 224805511, 1731065530, 445715423, 605125223, 1665621765, 1684968824, 285473064, 934706380}
,{635845828, 1488439816, 1943954105, 863892961, 1950542096, 2124887235, 663372661, 739086712, 832868288}
,{1043733062, 2130346874, 1647693633, 1640839919, 613941008, 1979788667, 1706386876, 614107783, 721125831}
,{1135953436, 1598547089, 1641911001, 1193420635, 121448708, 771466657, 1049775124, 1255496071, 863141089}
,{1271817329, 2017624995, 2120898483, 1337599982, 1631733733, 391372924, 265931042, 1459272482, 1335736729}
,{303909679, 782026594, 1067049764, 1935697426, 114719847, 568284024, 1297647085, 2018391858, 1341017979}
,{631921674, 1344870702, 629039269, 1189947616, 1699609196, 479453022, 675979076, 523858210, 442516999}
,{671899298, 1041163702, 2029076586, 286469869, 1688989452, 591250583, 2144261429, 502346010, 439426525}
,{389947344, 1322600064, 1185426437, 1135741891, 1544485958, 1615615223, 942083656, 376515882, 1627453764}
,{1493241911, 1131065694, 1497541459, 925671571, 2098520627, 1789547031, 2052316004, 636857699, 1768246250}
,{502019142, 2006891164, 924514382, 1720664081, 1097861091, 2045932829, 1174805533, 557862868, 1761602546}
,{392410312, 1071061492, 596170142, 1670806910, 525511786, 74724424, 999513323, 1643099794, 1453665411}
,{200378196, 1814881671, 1225770751, 877908432, 663664934, 1315252573, 13813074, 228828762, 798323232}
,{697800751, 283121484, 1422963239, 1062614850, 859986881, 1220007227, 1747579986, 1095206949, 1443032090}
,{2134742808, 917467863, 1773156848, 338528244, 112009984, 1029301339, 1305527197, 1706954825, 51446707}
,{1564363505, 86233387, 1789560606, 921988010, 760483746, 75577072, 904115172, 1894037888, 956563944}
,{605191584, 371341586, 648008069, 1231375969, 1726555430, 589021261, 549224810, 2112889109, 1953411883}
,{23563888, 621743753, 764645338, 1007236932, 2117680029, 1472489851, 1430389896, 77103739, 1983319538}
,{2139775670, 489301961, 867602896, 52274494, 324723115, 814122300, 582660091, 1029459468, 823405760}
,{34279338, 1176889038, 1532384765, 493992006, 1298232698, 950142905, 1736705660, 2033628672, 1874952851}
,{650635776, 1191802153, 203687531, 1832619713, 1924312855, 534582902, 231331880, 1751653555, 774363199}
,{27312923, 132021286, 592782563, 335793474, 530411242, 444584310, 1510919878, 1739706799, 1961623327}
,{586931563, 1684006599, 926827683, 1082371839, 223001368, 1800703099, 189207416, 586652514, 1520911541}
,{1706074879, 1749266577, 1329270427, 2023005645, 1774060637, 1044766187, 1715221538, 1207929742, 1566033592}
,{1589280989, 1161574129, 370099529, 1228118057, 295810105, 1571693424, 249503560, 1140791811, 1077648977}
,{416683779, 2037897807, 327145726, 1674706772, 112599622, 1784684302, 2064738232, 93054932, 1368520584}
,{707110167, 1592584761, 492081654, 350209577, 1051147310, 2096715479, 1798340701, 1975648161, 467044636}
,{975680033, 1303900372, 1227215093, 1657470587, 291551410, 210793084, 1582934243, 1425566149, 758970899}
,{1044706751, 1692367477, 1215413767, 1012336701, 1259099765, 360352473, 337671125, 1431022719, 650867631}
,{1626642685, 118459609, 1400881852, 1921294095, 1260875235, 86005581, 1478154081, 1511314349, 1698282854}
,{522113737, 1610806790, 301386820, 764188161, 1063479382, 1105944435, 1524919003, 552589057, 547950665}
,{1528118171, 394722147, 699099607, 1832333535, 273460244, 1040996793, 1730615100, 1226618250, 917885680}
,{666529869, 533233538, 554114830, 1459377997, 1514433941, 1266315725, 647775238, 1017908299, 1050791854}
,{1617098627, 1067131130, 850098477, 1475935864, 906629179, 1252765887, 1163249693, 317179195, 258745549}
,{1287416163, 1610557058, 467539778, 274827082, 808761689, 1482083948, 807803855, 1602708468, 1106807184}
,{2106658785, 468992784, 898152617, 643563985, 561066476, 268533121, 911540207, 1076144189, 1150783651}
,{1981415293, 107023716, 1255663417, 1715047036, 2017553133, 809392734, 328130866, 96962680, 1983616195}
,{825165805, 416799555, 1679242008, 1788251545, 1972124848, 664862435, 909669244, 1899364039, 1199973252}
,{570235118, 1940872107, 1490323632, 638337803, 1905403432, 1526541451, 2137150130, 964077081, 1675350636}
,{1826477593, 509382515, 1983219945, 985940997, 1018265977, 1265979728, 251636852, 38874640, 1004659853}
,{952832179, 1185683709, 1416599613, 1088319909, 1282664564, 744818264, 1585409950, 1607168250, 1793897500}
,{508100929, 213299825, 777921460, 1038831226, 518244818, 1001832141, 534654393, 100082912, 1771705987}
,{1804776856, 573627549, 1278475606, 966620189, 305811934, 1659942567, 1449114984, 888926674, 1497926151}
,{986033980, 21959762, 1381272619, 1892648977, 334562413, 1824331516, 313259859, 1675633844, 177587297}
,{2138161540, 857395021, 2014270199, 1822395140, 304176638, 109038482, 43371448, 518724945, 659493819}
,{347166499, 470293046, 404946407, 1213677482, 422482546, 241410589, 1286820342, 978038727, 831788268}
,{1454416945, 634675858, 180057489, 1754449243, 1786315817, 662500839, 1988291660, 1058547162, 1630572675}
,{1023084990, 1743817964, 1482367706, 891485539, 901081232, 311996394, 728923874, 2139600736, 1870207892}
,{80830690, 1887569385, 1345174175, 440203870, 1485866436, 785536820, 1070236288, 924614628, 2021244775}
,{2002566453, 1272864159, 2042774670, 79859483, 1354525580, 1967830271, 387666434, 1447414784, 1277427135}
,{126793264, 1909304344, 1111167498, 448588251, 1996250095, 146939784, 1489235303, 249478442, 2123681005}
,{1779187615, 1265743709, 1966094990, 1245005098, 2026429008, 106157132, 1069946535, 333652899, 1802276311}
,{482229776, 2046191179, 1459987118, 553726434, 625477061, 1447914028, 1812210667, 671071977, 68479322}
,{1400152298, 1915950803, 89676141, 1092538758, 1234429687, 1211149134, 1169012497, 361261837, 785865497}
,{1705822023, 1041238721, 763467420, 1279788270, 1200460927, 766323560, 495254174, 462204215, 501818540}
,{657548185, 337639282, 465603425, 1521379574, 591019205, 644368329, 1206442151, 132757180, 544554192}
,{631722502, 152799278, 240926631, 1488912181, 900495960, 146444767, 941290239, 1052086415, 2142878450}
,{1081524237, 868454550, 1694173090, 123993174, 1282005108, 167798520, 1635443608, 410581370, 64559805}
,{1958540286, 1778152416, 1639615404, 2136836413, 1947979151, 1736908410, 2105439284, 1829429393, 556059587}
,{419228093, 530290295, 426996526, 604783678, 991482555, 1779424833, 1200062205, 721632818, 524162704}
,{547972303, 33302796, 1020792009, 656756445, 1108483354, 163635037, 641397199, 324893243, 1316893266}
,{1360034525, 209449447, 121216774, 2010374057, 1293637621, 270308275, 1637221613, 685727216, 624136366}
,{421007573, 263630173, 247666046, 577379037, 1742084999, 1512141893, 1114280754, 1690619326, 1794613329}
,{1971153196, 1287829474, 607394838, 1404328298, 1241189860, 168429126, 1192689738, 772138525, 1507812288}
,{159323736, 561809003, 52385519, 549888458, 2135337747, 76548966, 669838475, 1926057727, 65353172}
,{375580376, 997372137, 1654625350, 194395592, 381798504, 436422276, 1072380824, 248559033, 1690667213}
,{408657643, 993084734, 1173508354, 682277175, 1029573958, 1172177106, 1407491461, 297029346, 1569858781}
,{1435421825, 1806233662, 1361475589, 1431319397, 116856681, 1840706935, 180357250, 294452536, 1731785211}
,{1460272808, 192375858, 799804066, 503774577, 217039805, 540273834, 2113946777, 1589254305, 197671710}
,{1299561582, 1311919316, 1068882818, 930079977, 241652930, 1071821127, 1445364700, 424202332, 1381672302}
,{167975528, 1586375271, 736763443, 1649013098, 1369337536, 239093648, 1512074125, 1656658066, 1433752307}
,{1547403888, 1316377072, 595246299, 1604747524, 1839991505, 1633779170, 1097165413, 469120353, 924180105}
,{954010695, 499771938, 1393140576, 622280482, 327283122, 1776480930, 1845114074, 1479729109, 775745575}
,{710705746, 863410918, 1107113286, 559271581, 925452623, 1258909001, 773869318, 73668955, 362797577}
,{1919506287, 542113216, 1183098460, 1949229541, 516764831, 621905292, 106438149, 730860183, 583086314}
,{794666577, 1314023302, 755512100, 306455518, 1305496179, 713767453, 1901557862, 2003142278, 389648017}
,{1370649598, 490196192, 1461078878, 527145439, 1672195730, 1943204115, 244086526, 1085960249, 110993637}
,{1251669300, 2049999685, 1808711693, 1484101042, 156002329, 1611915430, 170799469, 243205222, 285628866}
,{546103090, 22343531, 1700606925, 2036508542, 1938652515, 904949273, 632255403, 2137341698, 975834556}
,{432642854, 74930292, 1437508945, 566739640, 1557011946, 1426743846, 1582393693, 1408766218, 849195405}
,{1117003444, 1126718264, 39920988, 301777603, 956309094, 1165263788, 491072121, 237276543, 1195384851}
,{829848438, 424230249, 1249312539, 477771150, 140624154, 2066518578, 311282672, 1657312403, 2030171007}
,{1252391119, 1467440502, 1689705601, 514002627, 296287326, 921729428, 78610113, 1882353458, 1570198898}
,{600241506, 1551919600, 2146407760, 1357350347, 300553923, 1986080167, 55415331, 371587340, 170498354}
,{491947227, 1537432000, 1953627465, 727747509, 1329433839, 2116869747, 1964342330, 1113969517, 617421961}
,{426625672, 153135678, 1803281235, 766329205, 1309771571, 1292306881, 1458348009, 1222275043, 1204097853}
,{401823302, 1744444954, 517990039, 1485277314, 1252561419, 1818555163, 203618279, 1306636734, 150993468}
,{1228959857, 1743209716, 1948773068, 1792254917, 1944199093, 990951219, 1494565959, 1161782649, 356459160}
,{1515519086, 382614115, 583389502, 1649026398, 1353709817, 496427529, 1376007508, 2026417229, 1610831428}
,{1105873820, 960161994, 1955873397, 39549277, 533690949, 573106157, 1433163695, 1148554719, 24322615}
,{975552536, 1926664193, 1370127013, 1867356970, 919716210, 1155724076, 536126857, 1357590023, 766682249}
,{870720384, 10257571, 1363847051, 1582406984, 1754364131, 135002166, 1960841387, 1647731775, 640787098}
,{1868029546, 848473623, 1309532558, 198063721, 305315076, 838908376, 586417897, 1818178557, 1494313681}
,{1240823908, 1215153967, 557370092, 1042641414, 1008677279, 1639859058, 1916154704, 801483997, 406035333}
,{1853487671, 1812708103, 987298390, 1322920810, 96104876, 197619777, 596008921, 1221691870, 742254545}
,{703001678, 337460094, 1387139305, 197727397, 1682640344, 2024144789, 1645056270, 695699526, 2083390604}
,{1649204552, 1934835339, 998658715, 1574959228, 2007701836, 498620287, 1575760891, 57075275, 1450854578}
,{171044680, 921346714, 1379276801, 1159898289, 1651790843, 861481076, 648261695, 396135784, 1942640048}
,{509896901, 220474739, 128852592, 1563042098, 524158439, 149385006, 720812934, 666512477, 2116240172}
,{1300746182, 243491147, 1764039695, 1021673822, 276679002, 2978348, 474507085, 1392467254, 1292554016}
,{718563359, 1800794504, 218206276, 7028600, 1653282223, 1441923146, 357778269, 848173847, 2108040653}
,{198650791, 982191848, 520051063, 947094626, 1962369317, 2093780077, 1612589136, 2100675346, 1871378040}
,{436060618, 434390753, 796442150, 1570972297, 1972390201, 67021172, 2061474928, 1708449531, 408065224}
,{936984831, 311955462, 1775964356, 1213524479, 1143398689, 1165126777, 1046047437, 525285329, 1925916465}
,{1843726429, 1277456401, 214992177, 2087319888, 702553828, 1598519792, 944000438, 1542171370, 1236535672}
,{1521162238, 31208061, 1281370113, 1357450632, 676036525, 1431536560, 780332317, 762211774, 1912500957}
,{1888683805, 375453125, 952886477, 2077923630, 1242188126, 251284705, 1401807742, 1802039285, 1148345288}
,{779676629, 1332920956, 489429353, 761613459, 1218205787, 2026401777, 1605549831, 1380787024, 1164427058}
,{1231186529, 1565023901, 1875535355, 1512820274, 1607619257, 2100476257, 818246118, 1011685768, 2112522697}
,{811628379, 1705610307, 16874460, 1798372723, 2147340465, 1314166192, 845406481, 1609854637, 1261768795}
,{1810342776, 1552035946, 687657744, 1990194976, 700723248, 488919245, 1053406920, 1621502554, 1636435907}
,{1504258710, 134977415, 833102749, 778784557, 1219879966, 682228690, 1668064531, 1376077977, 416130127}
,{839100178, 1749243200, 1701101882, 271969123, 210687910, 1240809773, 515199185, 628520320, 2138189445}
,{1470366633, 2141011593, 1266203254, 666286714, 1842064823, 874763995, 423164944, 947675713, 758759047}
,{1678015270, 116859260, 1247010065, 215287851, 1591300239, 603295739, 928482374, 1393557573, 351935812}
,{411315658, 1064405443, 949149267, 1917473011, 36221335, 705696598, 487407093, 995261252, 976370085}
,{2081921739, 559198170, 1300558149, 1961823939, 1609758527, 1870635026, 1290080984, 1939792324, 1360327943}
,{915831794, 1142556041, 1144918594, 1044745561, 813535973, 177769819, 975324975, 166510908, 1614250614}
,{1077798838, 1494331585, 1385766398, 713737839, 1154290893, 902353627, 1821561905, 613062084, 1369215893}
,{789990038, 1601012174, 1776491438, 264167406, 787146142, 1109767296, 958576155, 1539278487, 1000017948}
,{498987354, 230568280, 1758899452, 588999776, 1550898805, 1559161259, 1105818829, 1168330827, 1893602292}
,{1668335203, 109656423, 1994170127, 1970675775, 575726905, 1995345296, 866302544, 2096966614, 1345027143}
,{2008871224, 170627974, 548657407, 1678084000, 954578382, 2044503422, 47829574, 1017098555, 61722976}
,{1559053548, 1158156737, 185813084, 1269770054, 238481041, 1471481454, 1174033128, 1717851918, 1569968152}
,{90603665, 2146146157, 1960137669, 2110273685, 418479584, 503822139, 1676425738, 1308776312, 1248898063}
,{1126644690, 1854586881, 431787306, 1594060896, 1048001961, 386519416, 2014492428, 1747982005, 973079171}
,{1073202696, 376374321, 1792228590, 1552289949, 197978067, 1718740295, 653798575, 786589536, 1640658647}
,{550632988, 1626443699, 1456830509, 1050914362, 484698716, 145231965, 1942870233, 1766774151, 1047443769}
,{867845328, 382011243, 1081917251, 1395520813, 1495998871, 937258837, 431754111, 278039233, 647065863}
,{42576710, 797838980, 1429053173, 1968287534, 495024721, 1856678955, 242344627, 487602544, 753860560}
,{1886852976, 1609614086, 1078478039, 699036233, 477365725, 519718815, 359705542, 787887658, 2134340327}
,{645308600, 619586273, 1140884945, 308241351, 1090448024, 393728263, 1325202600, 1991904088, 1252275052}
,{1705545768, 1340633113, 146667783, 233111376, 1835938049, 1602685553, 981592210, 963295926, 2143029569}
,{2049570472, 345754663, 1721819552, 1917163652, 1348046060, 916895186, 2045151331, 234993045, 1802669406}
,{387841506, 1893940453, 834168143, 1765397009, 948747989, 254957373, 2140797433, 264498631, 2037289474}
,{633709227, 911024675, 1332942235, 536113350, 1947923889, 1440845722, 1043394597, 1996286341, 1356597852}
,{1360410282, 1984049154, 84043418, 2056371927, 1337268247, 1722653920, 1628598193, 734743283, 1402230645}
,{1861581132, 1138995265, 113032201, 1163089445, 433497632, 1775932057, 1316817081, 809577649, 1116513096}
,{192647327, 999506358, 14160254, 1822981100, 896958741, 1683699070, 498607403, 1970591056, 1925238789}
,{552338480, 762004938, 714912894, 447600555, 212530724, 568686470, 1589614453, 1287099867, 513665047}
,{103408329, 1686018862, 1832062621, 1739962585, 111901374, 468709941, 1775388324, 1548073401, 1824933513}
,{956301795, 1275507926, 5541941, 58465823, 1877820004, 403931386, 508411050, 954263779, 1436453499}
,{829228985, 523673283, 1189093, 927624862, 888061490, 770314050, 805320999, 538388330, 1773367398}
,{188032623, 49926186, 2131902940, 2072065411, 675689834, 306237315, 1487848968, 262828084, 1013196734}
,{1603184062, 1995888420, 1626879601, 1495278515, 117147775, 1649522363, 1371669321, 1122568435, 1334861558}
,{2012239862, 1972575343, 300838702, 960303325, 1605688593, 1483039756, 1499761705, 972257640, 1265398419}
,{1044625126, 1792101421, 631212518, 816950621, 290303225, 2070727721, 13831998, 597361057, 1906587955}
,{2119676837, 1023493679, 1760373509, 908642252, 477091563, 2106967886, 1667179843, 647251854, 1753791967}
,{30867481, 1326301794, 2114042830, 962582583, 505982928, 1422872358, 1401289140, 472131384, 770233673}
,{1599030147, 541256280, 1003418393, 574008529, 1588276517, 733215005, 1283664053, 1520590481, 1163444031}
,{346690764, 533096000, 1618628798, 439565889, 1133375816, 841250962, 1538939326, 545182219, 835187857}
,{418800739, 178967331, 592929537, 517342289, 1050804147, 2072496537, 1692916849, 891189033, 2134752356}
,{734435472, 672358097, 1890019402, 1995473104, 493825606, 1703795870, 219949024, 200504028, 1282642655}
} /* End of byte 7 */
,/* Byte 8 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{76622690, 1262953926, 691591548, 1420879705, 677234486, 162394161, 742127774, 516889890, 1570064848}
,{1257138893, 1010943131, 1220316968, 1008098602, 122134259, 1966808889, 1498301498, 268944713, 1423867980}
,{1947600612, 1041288870, 1738585834, 608105213, 1406191251, 1648118844, 1591240968, 910164333, 336014458}
,{813149256, 1165547001, 1660636699, 478433473, 1231168130, 451448032, 643636429, 1219528364, 1233829932}
,{427442644, 1358489726, 31693233, 1491214197, 1661919698, 1638381530, 532001521, 1412387847, 1919203228}
,{1163746257, 1132790788, 2053379590, 175913168, 1403171617, 403357051, 945836981, 2099728054, 24021704}
,{574253195, 391526437, 382595941, 989715773, 1066814412, 1801150554, 1255431800, 550314643, 1196326834}
,{1197325563, 1936861601, 359364252, 1558261395, 56435017, 1839920201, 1970250011, 2049789897, 1164369653}
,{2056689770, 163475901, 2132214923, 1183217709, 1185731435, 517376305, 340520481, 1171409723, 1097860579}
,{1493122068, 1961058629, 262062212, 1351600809, 275021794, 149975717, 2034931811, 1820490681, 1892707885}
,{2080765645, 1908139595, 1822918331, 599078489, 2068537662, 1716742531, 255690804, 939083437, 1417735912}
,{621206210, 2028496068, 729225541, 1705293592, 129144965, 737332071, 1652275322, 1341745594, 1162772560}
,{979563241, 1207442495, 1752960832, 1754692403, 949854765, 971969543, 1734894039, 1040105307, 1337437184}
,{89730113, 28766505, 1544434888, 966869319, 1076742123, 1393225336, 627309291, 1928427073, 1797657018}
,{716446997, 1910834844, 1876012270, 1501750750, 879660094, 1337955848, 119237535, 923146701, 663932931}
,{678464481, 2031647132, 401431770, 1900286467, 1174960925, 1901558746, 380131303, 2089363058, 228935490}
,{645674026, 180105544, 1098940951, 415401971, 121048253, 598364729, 1855880937, 1591204743, 2033732787}
,{475187477, 1945199003, 435833310, 873673585, 1591154940, 1456760839, 1966076481, 292920999, 1381258408}
,{1189883719, 1011070741, 2121478862, 696398371, 1926739222, 487237257, 1628823275, 1736922960, 1318816186}
,{1703463820, 837794420, 747918282, 46128496, 44731188, 241582083, 985487100, 1848691658, 1330424390}
,{1808483547, 368341134, 1748503884, 1497101198, 910036106, 486991169, 396940969, 177055853, 96447281}
,{14564532, 583490611, 128180657, 384270063, 1897990052, 1866883213, 870635903, 401670044, 39418619}
,{1500472772, 1471888659, 1046258346, 635156539, 832705137, 87729730, 2089294764, 333400101, 823389284}
,{1965748022, 2100525572, 279807828, 1936935320, 314329830, 1627262129, 676633724, 2129610246, 213211338}
,{2091338394, 2006973819, 1457183804, 945375171, 1041497112, 630760287, 1988036980, 249562228, 1813143791}
,{715722623, 1623512518, 942303185, 399591742, 1831674063, 1695132147, 1100626210, 1088114392, 591840567}
,{1162880479, 1269408864, 637918569, 2097406683, 1609898517, 755026514, 1876897842, 1655240803, 1352686783}
,{1413049094, 1580755517, 572641518, 377441342, 53165472, 441943240, 417961384, 660651587, 573556693}
,{986060811, 1811426223, 1448247938, 546195151, 1363421232, 499937298, 1058521519, 68338437, 1017380232}
,{1570670963, 647253849, 1530207798, 941943291, 1991313601, 419641559, 1275959070, 1569099440, 1442128797}
,{1986756839, 2114635029, 494098906, 2137919979, 1792079974, 1311895953, 1430799693, 885520810, 408405867}
,{2038070407, 1028939678, 375880731, 1942535494, 1552485169, 1067234312, 1290838910, 417000052, 547541692}
,{1113380133, 16079759, 127071570, 860443876, 469982324, 1022261083, 91402738, 613704553, 1651050101}
,{1762361769, 895098348, 598978188, 217240839, 305754146, 1244639370, 1237096232, 1072747346, 574537669}
,{1891469025, 977657187, 562824439, 121341251, 224995080, 852513940, 264444560, 28785655, 1825532836}
,{654084285, 691063054, 1674600029, 1939279051, 1117940662, 999004466, 1969193560, 1250061305, 1217290117}
,{1387131850, 1703047808, 1505530887, 849829612, 1701330401, 143809837, 1133074897, 131130851, 689066962}
,{909561441, 1249312750, 1347883518, 1059848733, 629862792, 1038340428, 1843457827, 1166257770, 1295414396}
,{1157970020, 1432871617, 954756221, 103525986, 802188179, 1027582110, 1668844306, 163675573, 2140766099}
,{1685591041, 415121478, 1608126475, 1255221690, 1744804928, 2080022443, 1202241167, 1586056558, 974936768}
,{1103280009, 491074650, 13717077, 2087822327, 1362230110, 1071294288, 497757535, 1376736278, 665624048}
,{76009830, 321484326, 1224712704, 1390050165, 1126127369, 1368622709, 1712062397, 468705779, 854706164}
,{1693002609, 540731630, 1038608133, 1514516719, 682011332, 914548358, 503767843, 1660320521, 208273332}
,{1422310183, 1306692390, 394131953, 1725590381, 1278907386, 1131006373, 1394144093, 486020672, 249806692}
,{854667557, 1918759853, 563281889, 1590420603, 1327422264, 1297503661, 216529790, 440893353, 421490314}
,{2131004642, 755546158, 906029187, 684318008, 556797013, 224442898, 612725808, 1328857434, 1594935464}
,{1287557758, 497776306, 1665647943, 1348088376, 1543915337, 1979866373, 221075436, 841215244, 1678986731}
,{2008254558, 551297217, 250613028, 506148742, 2065939892, 1781143404, 56709566, 1130545488, 381758783}
,{877399111, 1364796933, 938733822, 1578427605, 478444459, 91339603, 1228887508, 2007310436, 1493872724}
,{1467339773, 1637848991, 1059214289, 1018989716, 1485901581, 372660794, 284537917, 597278898, 1062105602}
,{1027766325, 1389499643, 1025134613, 1061576237, 2025315149, 1566652341, 584180476, 427616341, 1983023612}
,{334831320, 998745135, 2106171212, 1657230965, 1557186988, 881554173, 1046007975, 754257262, 1507945311}
,{1353036720, 801630760, 590527646, 1362650625, 72198842, 328067908, 1334697337, 573384010, 945256262}
,{1509329766, 791448999, 625284690, 1465382211, 1108839132, 907428675, 1918297389, 1760345766, 1136963208}
,{797885489, 351082445, 1524897010, 183563858, 1815482816, 1038190165, 1698401364, 18620110, 735223954}
,{1779042405, 2123711584, 176502105, 985246364, 342530778, 2037917654, 966469479, 539607864, 583164755}
,{1542413066, 899242265, 142853818, 502692345, 1240208000, 1003642786, 435823937, 28031420, 1230397003}
,{2053245527, 1814360540, 90714760, 356166217, 1149464752, 1847343882, 1806376413, 1119798843, 1079657704}
,{1167764503, 1169346327, 384920818, 16550880, 262965154, 1082751298, 1938693146, 2129737020, 1665040844}
,{267014935, 1168545153, 2011752009, 1489353788, 1880555270, 538219014, 1179130260, 1076658197, 1181602354}
,{653425090, 567076855, 1565470095, 766282773, 407019736, 1412119897, 749105584, 933592548, 939255803}
,{1567102436, 1951062138, 672879107, 518637563, 47276468, 973079556, 2050288180, 1948520347, 116913623}
,{1871955034, 408411623, 1093466043, 1372178599, 1428574208, 1256465914, 9457879, 1261494159, 879677070}
,{364777060, 738090725, 138117353, 1050215045, 1219762192, 610418754, 1427709342, 765265140, 951919393}
,{787976571, 188759461, 1991471290, 540823390, 1440696140, 1094083667, 212677636, 36889488, 139294631}
,{142862968, 2117002626, 1367181895, 963505358, 1590870375, 778510399, 987329073, 2013804553, 134419505}
,{1799816503, 860356369, 2144690669, 389401004, 1548767606, 552543140, 1951300570, 780187628, 2134695395}
,{477361174, 364590810, 831133347, 839226119, 1087314929, 1700752952, 1524360002, 520448483, 1227280507}
,{437179178, 1031102247, 1678475747, 35554807, 1884060921, 663341170, 372564514, 1408395780, 1391122398}
,{1746486361, 502612195, 629728558, 852077070, 1943695453, 2146199798, 1811293911, 1399240207, 58097010}
,{2035505335, 671150566, 1142291810, 217584534, 787988775, 1569336434, 2096262611, 665008626, 1199480850}
,{650325563, 936508716, 843235054, 1434503120, 1132320060, 1031017316, 52882264, 1845491057, 164275643}
,{197795555, 1851576518, 1327292195, 1922313655, 1194817669, 1245477162, 1106123553, 783551135, 383703860}
,{2008346843, 1004350694, 639156616, 483580544, 115595160, 623551846, 1065158564, 540271776, 1644486956}
,{192172067, 1542314238, 878032154, 1426232400, 766411126, 79189644, 1279812508, 1837291481, 230733290}
,{708687977, 187069245, 2088334681, 1992506442, 1485597575, 1764389553, 719179061, 1848167953, 87119515}
,{1482919068, 802476107, 1112201519, 340109734, 1270225255, 1142447892, 2000834945, 176688415, 671513398}
,{366743957, 1870688754, 632061393, 708736410, 606032499, 1420205822, 349737720, 586328208, 1259811085}
,{1514589758, 394019090, 1824553642, 57842068, 2065705255, 1796647957, 873604310, 825822383, 2098789968}
,{348923018, 164006386, 148293217, 1097201928, 40052803, 1974618612, 745666891, 2017334522, 1696217732}
,{1377026540, 350265539, 810774812, 2023225621, 148932849, 2015789897, 1930609448, 59022126, 772520199}
,{1919023920, 573371149, 434282342, 1926994982, 1170916982, 2049196424, 146560323, 50337769, 2119666404}
,{752582528, 1078431089, 1724222442, 2024232693, 9602876, 1674020663, 1460437757, 1190956931, 773883956}
,{1498708907, 2011383693, 1619995664, 6073662, 1505439642, 90971205, 2125026654, 765552886, 1881741226}
,{1697362287, 1181738133, 1843440199, 16445477, 168687820, 1339280609, 193575980, 2096177531, 616141690}
,{1377406477, 1869719511, 248894690, 724339349, 1335957702, 1065504697, 1701329146, 773119242, 1280114332}
,{659025791, 1423813806, 631385842, 1166857595, 816463961, 226787138, 337261979, 941441720, 486959359}
,{1651918891, 1314019757, 1205225684, 630538717, 300123628, 1916964151, 1715404237, 1855508334, 1153934264}
,{2104395145, 980344374, 530997496, 1344684534, 154983898, 780676985, 794631463, 1388686069, 635311415}
,{729823239, 1127673457, 746887363, 648689552, 838331803, 764861141, 586560146, 1087638595, 2068271030}
,{156742361, 642857268, 1920541390, 1724202320, 1083897654, 695371624, 2106658051, 20504361, 1184095310}
,{1312733772, 986567525, 693137512, 237556632, 961643492, 1815600893, 1453541570, 802558906, 1324992969}
,{720395646, 39264999, 1087631797, 2026545086, 35092485, 431371617, 614529184, 93472954, 1505615504}
,{2143574953, 1511966714, 1194201662, 609847937, 1150986034, 10096204, 1620164720, 42790285, 1672880346}
,{1056896006, 1291764852, 2051099216, 1555197607, 451313423, 1566096344, 1932462225, 1415706740, 2103299711}
,{175599240, 510462923, 634527425, 725173209, 502851158, 449003189, 1443743950, 1293604650, 1123831354}
,{1407804906, 1154113091, 847671257, 1727842106, 1229036245, 1305626468, 1249695452, 431455239, 970652729}
,{215572271, 751692117, 1289358161, 2014746603, 1250085940, 1857357828, 1498276380, 1490349870, 1802205309}
,{1236212959, 1537457145, 650123605, 1203995736, 1698713151, 997408427, 1052649932, 2124058448, 1814286153}
,{1210363915, 1266097501, 990370558, 1104748946, 1049290724, 305599219, 776036649, 1799503760, 419464408}
,{1774609902, 1454986272, 216085727, 59122713, 177727448, 1048621171, 1030184323, 20680747, 922108463}
,{395394116, 1032670363, 1433756084, 1613557095, 398315265, 464785693, 519461815, 894874548, 944102632}
,{563544074, 220456072, 1807064453, 824065534, 1504023351, 1526454199, 1165431301, 523538761, 1428158967}
,{1512601276, 164759392, 1444473589, 1158444432, 585070223, 1353335863, 1292928584, 1135416157, 2013035668}
,{550896338, 171330187, 1794661369, 1947689592, 656039038, 1911251751, 159355615, 1355044215, 1792386298}
,{1144509162, 2061849034, 1084276375, 921275270, 1839361447, 1876019192, 1716159846, 964864418, 1286910536}
,{260035459, 1768641377, 95702419, 602287064, 545522111, 1570254784, 2123093621, 2114062126, 449845461}
,{680361991, 235126955, 1588773805, 419209116, 1580896409, 1178116264, 1605929842, 71882036, 1126240967}
,{1588533012, 1996079322, 2035880127, 1447059634, 457875808, 581099309, 651379363, 1541199404, 347952746}
,{1838651461, 1820274846, 1591845261, 1038123452, 1872164872, 1284121017, 820280358, 536276962, 1899216300}
,{343665224, 1091335493, 701048704, 1282811599, 640212553, 1400549394, 1714094969, 1649246001, 170350635}
,{1467553402, 371792987, 1243202082, 1414139671, 1176239882, 72897661, 1325062083, 1272490489, 1697235852}
,{414570402, 139779557, 1697344058, 510093324, 986700768, 209428281, 1731418342, 118845269, 1911472565}
,{1768747022, 953547813, 8700415, 2142883260, 133404610, 549234720, 1963951910, 381758268, 858782675}
,{530917265, 888943867, 1295573302, 1065619695, 744472631, 1239993736, 432078765, 1451263039, 129156385}
,{885246945, 694838988, 502242470, 572927460, 750190704, 921873469, 1145954397, 1997204521, 158543304}
,{1624850116, 907322948, 332044423, 1091497064, 1290242743, 2056612325, 149101021, 1146685889, 1855270323}
,{614868838, 271888387, 2022838106, 1657005186, 1514636273, 1989419576, 897044525, 1879910514, 212422499}
,{1264872439, 1543900995, 634817235, 1488893100, 400125529, 993821459, 179605542, 1640779525, 362723607}
,{1519741919, 2061686307, 828346472, 1265988049, 1404625513, 1422402453, 1726369984, 1091255284, 1452182893}
,{1069500052, 482075726, 585402511, 190050655, 1615378034, 1396240896, 1814705115, 1165516600, 1207447224}
,{1247169724, 2017869777, 1065461747, 1177956684, 2085051005, 2010857103, 683784308, 1797625635, 919011121}
,{905228797, 1698877070, 1157753191, 226594385, 539958548, 1059924444, 1216276365, 337768569, 1388107190}
,{721186231, 1016782857, 2008726936, 861739302, 180238390, 1945483682, 1542556012, 1610334886, 260866274}
,{916983402, 222413436, 66540691, 504440587, 2129129703, 116217610, 1099606351, 1300694370, 600144850}
,{1888751317, 1524694410, 1350614352, 743407049, 2002718784, 541508902, 1375834601, 369239121, 1980859274}
,{904836252, 563107399, 696338202, 1939166045, 9619299, 1534904899, 51747305, 310441753, 1296829056}
,{573968466, 475684055, 861645203, 1103492599, 426249193, 919040186, 1184226232, 1977503606, 378368025}
,{2131879849, 1731971207, 840060391, 1209390921, 211779408, 1952180901, 264171258, 902182616, 1614002410}
,{769119323, 122432434, 360007654, 159554515, 500338214, 175363218, 83882163, 452318305, 878897719}
,{1074914909, 341531336, 4980063, 836221399, 1335741444, 931678759, 1719001962, 449319077, 856357220}
,{854798137, 2142645505, 564425890, 1825052627, 819136669, 1521343610, 1105697450, 364316696, 262129096}
,{387926344, 2058165008, 785961897, 1071909447, 116726172, 204403870, 1788162366, 1090297713, 1130986343}
,{2004682437, 823863587, 1560143180, 283283465, 859354830, 1947585834, 234243044, 630795129, 1604944176}
,{756445727, 1162582200, 2038285584, 875617336, 1985188478, 1649445472, 601986210, 1738663986, 2020901177}
,{1184023761, 1207928575, 1647296032, 1658452862, 1336978353, 900894441, 1439357160, 1154677856, 503344374}
,{805482877, 1228253740, 235157337, 471340878, 169258403, 624212186, 660928166, 884261114, 1363005566}
,{396969836, 709364036, 877065720, 851734490, 1428728058, 140927864, 1257237697, 89578749, 216259490}
,{1012511944, 807057442, 461913695, 1447340927, 1089918427, 882174991, 1334268454, 336101874, 328037677}
,{1634214520, 679744277, 280583742, 1660467541, 1328258552, 107769195, 1853477259, 904376318, 796978476}
,{931340262, 339262378, 1745756429, 222084070, 1360311669, 228041651, 699574210, 1050974920, 992470447}
,{632218414, 540313674, 1732554837, 1167278053, 872862473, 1822275782, 1309506678, 494202351, 1881007656}
,{802168940, 1037385437, 1909841389, 830796228, 1682969135, 756626308, 356531993, 2101874401, 1054687277}
,{1582808524, 1147926766, 1465190782, 741343677, 1466203711, 1023440512, 247993144, 1084042806, 1731033823}
,{1943364045, 1261270946, 621881715, 1384450670, 135359718, 1117964442, 445342525, 1765451698, 1450186906}
,{1012431234, 1735181310, 1173228553, 243386076, 1799788841, 736585284, 1344920433, 1168528295, 853933026}
,{1541890707, 1567814651, 505417621, 534032953, 1621307421, 1779772180, 2029981005, 360410513, 743544465}
,{1593779600, 23070247, 147658197, 553240314, 471126225, 1612731034, 1086440888, 1031979462, 324681778}
,{1235978710, 1132892313, 698445613, 657947630, 79629679, 1774041588, 936524516, 2048150967, 114396117}
,{2037599088, 2077057749, 842471915, 1920656333, 1226252596, 75407880, 816079157, 1312906081, 874109650}
,{1293060814, 36626528, 481479626, 990568455, 2015732453, 1087293796, 1818913715, 1964561390, 254394143}
,{33498745, 1765531133, 1416715888, 1387108602, 1084857880, 143193837, 1044475498, 806793813, 50860769}
,{1071162689, 1890489523, 1398531315, 1738344809, 1118813031, 334466576, 851971284, 1869651589, 1430802616}
,{1090751444, 1977427728, 374750646, 573993547, 2129387263, 2025161452, 954434326, 884186627, 424057573}
,{235047904, 939083508, 933606250, 1180807098, 1335791362, 1286313471, 1664512541, 705601891, 122463059}
,{677385223, 1753791217, 472166664, 2041947872, 1200464059, 1838828936, 1546563454, 856405569, 1867089751}
,{1911417413, 2008827129, 64376264, 138205143, 1207704233, 588211758, 1791237431, 692388048, 1824361875}
,{1808051153, 706475871, 338398083, 2108129905, 1127000384, 1469522338, 1756800547, 97385343, 1415038061}
,{25039702, 1123887658, 490251548, 879000575, 1817434551, 1329872127, 776415580, 607255122, 1819643623}
,{1644067016, 1738850054, 1164711686, 1369425656, 1407476585, 733929869, 1874203873, 2109318671, 1850331056}
,{644817122, 2093400822, 1583883978, 718725935, 1832292357, 2082463194, 210776766, 1289199161, 1488446833}
,{1639123879, 1303062005, 662956178, 737739192, 1047565658, 621994465, 982129027, 1145613309, 1612968641}
,{396050851, 2084485094, 1155936110, 1350713767, 1179135276, 979499655, 276240319, 1146655473, 26186584}
,{1531507068, 204293137, 1896592370, 1131696804, 1418596466, 527259122, 1926800042, 672065154, 539128568}
,{1614412765, 1723831398, 773220625, 991963142, 1483839855, 1695657842, 394488745, 1390762993, 245453531}
,{1918053851, 1367641519, 502362525, 1204796354, 977914133, 1877438677, 1670620088, 1479072782, 674987024}
,{1802059851, 565675178, 1110610202, 728701659, 872868375, 1116070236, 290488890, 1700772371, 1458714051}
,{1983574300, 1604224142, 1200637877, 1429262930, 469046515, 909973137, 560024732, 374260815, 1948667965}
,{446196927, 1461964750, 1804496164, 1780785396, 127261834, 2057415102, 1299149243, 1648205708, 409383295}
,{1024705237, 866058754, 2012999889, 1308847138, 1775589388, 185289374, 1303903514, 208235814, 1058461759}
,{201069683, 2071189364, 357252992, 1051847452, 489469441, 175453875, 1906456871, 1565703553, 1337337568}
,{1927513207, 54842564, 2011192543, 1898121088, 15154534, 131832218, 1750302902, 1500263437, 1315423431}
,{52029874, 564327192, 2113726104, 531817667, 733453102, 2049355289, 215335747, 1208069432, 1599118989}
,{270725124, 1802722816, 358115325, 1203312587, 2076653487, 2137465752, 898492500, 888840212, 906565954}
,{608506474, 1417904998, 376586948, 93458120, 1623664036, 560682638, 2113083307, 105498239, 1479365470}
,{1965898957, 699426219, 1093758275, 1868652118, 711891305, 1836722684, 880402065, 2097382307, 729426774}
,{1197261809, 1331589893, 1236685621, 947176858, 884050337, 1878915901, 686439238, 492174760, 711789171}
,{1549011024, 1451677727, 257568526, 1622859760, 470880993, 1339020598, 1721646599, 912909268, 1010356208}
,{1394787119, 1123674924, 762238967, 285744223, 1238395632, 528125250, 1651136953, 1103710660, 852096901}
,{617088316, 1261533699, 2019814535, 1049821454, 2127501273, 616056059, 1696820732, 1744424386, 13714185}
,{1229814399, 464992500, 457275784, 703159643, 1247124630, 2074666925, 268013272, 1897845186, 1227123909}
,{244856929, 2012257089, 690212598, 1704390369, 1409568697, 745441158, 214559828, 340804241, 1397661395}
,{801210459, 23497828, 47638656, 991743481, 1578209794, 1783183632, 712014573, 2138402751, 2010884837}
,{1910386415, 1639338897, 844655844, 1883763804, 172886006, 433832740, 692814385, 30666034, 1665391447}
,{2015828548, 1493264004, 2062462853, 1629713792, 1009344130, 1646717471, 519744097, 1125680811, 1796471852}
,{1835501664, 1999987315, 1298877304, 1262956799, 995381157, 1787058462, 888155207, 1703721753, 1794942914}
,{928471896, 1786303435, 739441163, 1888245603, 1350376693, 1031841542, 593044988, 392189669, 31994303}
,{215153529, 1360422757, 1314678234, 1781806305, 2028357744, 104003016, 848780449, 303617142, 2136840109}
,{1686905425, 1856136307, 1636165546, 458937486, 2008354159, 1802590320, 1005258934, 1610335412, 585743306}
,{53836281, 433162304, 1550303847, 1177981514, 1177118676, 521693788, 79220833, 398634946, 1681841539}
,{1636598258, 776854993, 715220357, 1483834825, 11402748, 2136780885, 504217959, 1199984357, 1416769323}
,{571125825, 736194084, 1551573831, 1727471185, 679811713, 1073170957, 853841449, 247510672, 1881673166}
,{694808063, 1345663596, 2008714432, 1861381481, 1853926713, 1424603321, 1617477452, 436270914, 1056986799}
,{588188738, 782925145, 918504070, 1544068016, 386109458, 362310722, 1006443682, 1406561092, 1182303264}
,{1043196944, 1028615001, 1797263598, 1623567452, 467628146, 1092992800, 1414716118, 913668446, 792089284}
,{1372262391, 277285817, 2052757405, 1416532854, 7020127, 978109986, 207180357, 1021897182, 2011588125}
,{614999392, 1542665441, 1154299651, 620655532, 1993114045, 468422195, 453480971, 1643053198, 1203297061}
,{558214976, 1221144205, 633775227, 562178849, 1655590538, 596272234, 1928318498, 1820002657, 696467061}
,{1121078507, 2094285302, 743547366, 341201636, 1340852240, 306043454, 2139670710, 666058711, 141364050}
,{2017116964, 103178154, 888849439, 1886052392, 2008893210, 1974167593, 1593755152, 469244630, 770915766}
,{316586562, 1704556691, 40721257, 1245016998, 714550901, 1808499759, 1447855806, 1382512560, 230671505}
,{883209699, 824121272, 629643645, 999462239, 1553698092, 407623695, 2097588087, 1016925685, 1734645014}
,{1512741604, 1512842949, 1404484163, 1365256393, 744131015, 1412880117, 587853575, 622636730, 1599949983}
,{435846361, 1036163119, 1899301616, 1043124403, 1762919260, 990991075, 1015583828, 1546663522, 1436335392}
,{1528768065, 857235054, 1949998735, 1523557410, 741230885, 859037143, 1971458282, 1048295376, 2092314421}
,{364968367, 530845146, 1683933107, 1859028684, 1760789583, 1154725494, 635534090, 562849929, 382676102}
,{1341685924, 1770587180, 1899993552, 2023366919, 893915110, 1730493836, 864823980, 1720678526, 935922720}
,{1430119060, 823481242, 658408711, 700985412, 1979004497, 322939571, 1726495867, 848331135, 1145217688}
,{878909366, 433368474, 1210967251, 781760528, 1280202402, 769995290, 2015210723, 398559322, 1355455004}
,{2145405193, 451131243, 252625853, 1880454465, 951581873, 1096103796, 1586976442, 1539761075, 2023278232}
,{417539093, 1989662889, 1512864108, 40395813, 660031914, 1091904930, 56955559, 2060570832, 1227322311}
,{299134768, 1026768403, 1095597217, 445792330, 1077509777, 1781064693, 1868088945, 328832111, 2143534544}
,{2087337103, 892235551, 972778733, 1124765233, 2009571003, 483790112, 2091942382, 607333764, 79017268}
,{1502160616, 1289580144, 481978745, 5234339, 1265809165, 334327199, 257590043, 237200371, 216421394}
,{1962095531, 1522013735, 842419499, 1238031348, 1080954013, 2016396305, 673551704, 1431784981, 687203452}
,{37009199, 1891568535, 1330787177, 1231066769, 1662220422, 1771241133, 2106717151, 1674831772, 217035843}
,{1886041860, 1805586674, 1872553089, 141960585, 1662769191, 1145472023, 1793760428, 373137894, 887925834}
,{589810594, 1277907571, 2140501088, 1540007896, 1731753430, 427228769, 809855013, 504939365, 2091895564}
,{1499163126, 1586770542, 379703924, 945232235, 1412536799, 1706308542, 2144727946, 838822548, 1977942366}
,{1637912452, 2088818089, 360037143, 81243363, 1174608112, 656555272, 1418732986, 228931912, 235488484}
,{974469497, 1699601358, 1652401203, 90424132, 1910758610, 1036414487, 2041275409, 342345380, 1790937727}
,{1962390262, 2102192646, 1386167105, 617926013, 1945893959, 1630104419, 708256376, 2088023199, 1107711657}
,{818014600, 313950028, 277787949, 1254979295, 2145333972, 27644918, 53363863, 1558424490, 526143577}
,{285150557, 589339836, 2082702549, 1763176836, 1212435689, 167414594, 1379862347, 2031844291, 1682406193}
,{92533581, 2120317487, 1634222377, 466163232, 1603000602, 47821269, 436368927, 1065053384, 1659728647}
,{609391999, 842388673, 1118641564, 520713858, 312764066, 521727291, 2117153018, 1712445584, 142904841}
,{1087658662, 287766399, 545667014, 2061796934, 1509625936, 121834232, 1791515221, 1542140789, 1936244301}
,{1455725792, 24130898, 903326540, 394186216, 345857338, 803117059, 514861387, 965604007, 759515212}
,{510301073, 1877725462, 468132402, 1856922743, 960992364, 947500810, 1210248942, 1648083978, 169414607}
,{971143029, 1364552559, 1524652922, 72829406, 311117465, 1991020140, 827795873, 1178965221, 5853820}
,{465547966, 609118875, 1584320989, 1296827725, 1859954601, 1090422181, 1310047408, 1697304591, 299530353}
,{287548021, 125969087, 227144787, 1562477472, 859324093, 132701462, 395447640, 1424070798, 146150949}
,{1760678542, 2038931003, 1958188044, 1569455282, 1970021227, 1637441878, 1170335022, 1374586389, 307799985}
,{1775296214, 1424461092, 1020391546, 1105476547, 186640389, 158862323, 1286912280, 655057249, 923737993}
,{235084588, 1218433973, 1259894274, 2144733371, 751102764, 1995147826, 1205837629, 541574195, 24680884}
,{1117275594, 1883224087, 2121531192, 1539479964, 362313695, 847932535, 1287883893, 337101325, 781199143}
,{494124716, 1124071275, 1855910221, 408214753, 2006746944, 521630895, 1136468496, 1728437697, 1079671203}
,{1659640455, 493320848, 2067674484, 647013421, 928353718, 1523059424, 1964100415, 1570191011, 1559902870}
,{192631179, 1678231275, 986428868, 255779710, 1186868151, 1503436180, 460226205, 631571450, 2039559967}
,{497332128, 1840254579, 964092223, 233590012, 1589018354, 377698146, 1476142966, 361185068, 174113392}
,{1685059342, 1618611735, 381289364, 1324947485, 1703892832, 1243589330, 1019774922, 1182774627, 2041540989}
,{2051240620, 399175179, 69434493, 1721567317, 1917312866, 1828724609, 395748521, 1690429517, 1003539988}
,{435644048, 2093024491, 97686384, 1094732879, 517107310, 652058886, 513710868, 1773927534, 1399471226}
,{731352777, 1939752627, 435051989, 1686496879, 309064953, 1361656608, 817465635, 863512995, 1617448140}
,{1607632578, 2083517797, 779586430, 542353915, 404599709, 1306098030, 1396193943, 375808582, 487815070}
,{2012468723, 1769359693, 2095941966, 166022319, 1075511277, 1387118187, 214586230, 1164658962, 882629005}
,{1968383356, 1217792162, 1335522000, 119324315, 1044806267, 99361258, 1085997195, 1349838032, 1860853144}
,{108710118, 775000491, 1794522139, 1494630124, 1854758839, 522203585, 1138149, 313115766, 241929768}
,{2143733020, 634849364, 354331799, 728758894, 1468512080, 2032026454, 1146361093, 1723396882, 1080071536}
,{305276790, 1776002992, 1342485250, 522733922, 401869566, 595414292, 275096789, 1392817687, 666002652}
,{57975234, 1959146628, 1994647938, 2072652597, 991380182, 1294610202, 1756282732, 598960182, 996492483}
,{887998331, 119496924, 357856902, 1547597297, 1251531885, 989813514, 1514660725, 1554336955, 170643039}
,{1855889478, 1565314082, 445574055, 2143232286, 1145935616, 701004400, 200222515, 1536283494, 2111474946}
,{364207608, 1838082137, 213036909, 385389658, 1135561827, 422119286, 709540253, 1532914940, 599041598}
,{56798945, 1529571902, 1815351490, 1330486399, 1171312503, 2046563183, 1421176923, 1155197023, 1089116092}
} /* End of byte 8 */
,/* Byte 9 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{619049912, 1079722514, 1354807971, 149667838, 1552977993, 4061586, 1997631510, 524238633, 617875195}
,{535516095, 218819892, 1803453533, 806567535, 733030258, 1642001994, 584507741, 709364008, 1343746314}
,{1974506565, 1271326081, 1095774688, 1443001672, 1382192579, 1117119695, 167911883, 1706355060, 784968308}
,{1904573868, 644902499, 48552544, 965945966, 611689662, 837295104, 184995987, 1652816647, 784823112}
,{1863020615, 217684939, 1250054277, 949936848, 2137033600, 1176240459, 730995934, 49843177, 1797465941}
,{142994373, 801421864, 157686382, 812649715, 1268776227, 2147238204, 1983133387, 870172306, 1608461014}
,{2072136048, 2096251095, 1897143067, 878046842, 655472403, 82513290, 794521592, 1888454473, 1390377708}
,{6461960, 699114421, 1874375723, 1020466481, 489501780, 696852735, 1093606354, 2140942810, 813913620}
,{1635608418, 1558581574, 1377684977, 1734235561, 332236814, 771370263, 1429827932, 374802326, 845301094}
,{357748305, 2095175263, 1762713580, 1778264428, 1695858347, 359350621, 1744315045, 1704625074, 454994477}
,{151399900, 1590309679, 2058517990, 1559390799, 185298695, 1537319714, 876210996, 823220221, 4545800}
,{1394832398, 546484721, 2060952283, 898294513, 1124410352, 1991028664, 1793588980, 270538370, 2103463746}
,{90931320, 927287731, 625526632, 334578869, 399623455, 895461769, 1278482654, 1390348493, 1308865853}
,{38279199, 1423388043, 498137195, 849770688, 1258220986, 1302616141, 149595728, 1656405642, 893227376}
,{451391601, 52549319, 1765710159, 972021851, 2106230937, 1936441, 1872406587, 1068298822, 1947074714}
,{1062547187, 1665870345, 1375864241, 429355074, 1027307948, 2045598106, 1304149998, 1905637857, 38212151}
,{1001576382, 772256397, 875033500, 2000436370, 1915668879, 636453028, 1511268399, 1364118248, 149737304}
,{1644015529, 595404386, 400422380, 448997642, 455803349, 986366592, 1343684261, 86082333, 1821538903}
,{1350136621, 57403831, 1543436134, 1856879695, 314740216, 1839713843, 1221658836, 999802373, 1360673756}
,{2105336211, 1807611667, 1234099556, 2070925050, 403287763, 104376352, 338574374, 1308178925, 482296051}
,{393316110, 1360338099, 977026399, 1465102986, 133869482, 148935637, 602776379, 1146757071, 913620513}
,{1230497636, 1562221818, 1384021246, 1191407244, 1250908662, 702050916, 708806696, 513828171, 104701161}
,{88277466, 709143456, 1724818603, 1475288839, 459370051, 1322491437, 939824053, 394731144, 750710880}
,{677935013, 140364546, 483812281, 252444054, 1710605949, 1298951662, 243927564, 596566136, 1072311072}
,{78939106, 1186136771, 602572284, 817467367, 1770008375, 1802753178, 1904343954, 1654185536, 761546708}
,{429434322, 1205705214, 1007057269, 1554608428, 791363309, 1270151566, 1206749315, 595375256, 153485240}
,{978931300, 1320254265, 540214532, 1665202339, 755305851, 977628310, 1271786447, 1971696359, 2105321590}
,{1864548026, 743097659, 1339066245, 700723693, 330573693, 734750944, 1511400144, 601230372, 1408633251}
,{287651679, 1237537606, 499631393, 1113542764, 111451801, 1029485336, 1857298443, 785610856, 1554474919}
,{2093636885, 1349622704, 69079890, 839870784, 1382173384, 1153720706, 309522737, 409415279, 1131503854}
,{2104734668, 317095917, 1114941443, 1054662176, 93930203, 439432242, 1175888523, 965223740, 1903585541}
,{628554766, 1630151487, 1507185511, 930631393, 1013163798, 2054402413, 36525390, 1991589330, 1389849356}
,{758762081, 1600760383, 581144288, 522917653, 1478955063, 1442567837, 1901606645, 394226239, 1233601697}
,{523260636, 2105403486, 745814256, 1502381784, 1868292349, 1492765534, 1726584872, 39083620, 1854613082}
,{1488887086, 1129030464, 2075424761, 1088604653, 1839205978, 1252165078, 1422418619, 483383954, 1025776500}
,{1667428830, 61481922, 3488054, 1995749386, 790395665, 551623655, 1421032510, 424646184, 1071266230}
,{2138168344, 980713428, 1237089983, 1093686229, 684462007, 661851237, 683319503, 1176646950, 1346377398}
,{1792554299, 772948721, 781982845, 1254311261, 447767026, 38180534, 124374564, 1640276893, 1269534484}
,{1080038599, 2132773768, 1051228472, 2126522347, 1867664802, 1647386076, 45146616, 1681224302, 1493984661}
,{1173059173, 1290929837, 241328073, 254507121, 612986054, 128739850, 1270010949, 776370930, 1414998052}
,{416331673, 1466644764, 1530598417, 1108629674, 976898083, 569309402, 1546088491, 1748614708, 1223876394}
,{1158093271, 2135212453, 1800640037, 1132091666, 1740986325, 763512167, 137546408, 1942943882, 1275704331}
,{964198910, 796077917, 154965414, 689787887, 1958502200, 1532350122, 1980874883, 318604225, 236377986}
,{1356589332, 831882403, 2127838631, 1591000185, 1406147889, 1645566325, 2088761329, 1826301361, 874316595}
,{1089372067, 422654497, 1022472868, 427155708, 1540381111, 792993550, 1818666949, 756989917, 953312093}
,{1639559898, 1339547579, 2094179613, 1767564509, 498235731, 550953250, 827604152, 618909597, 2106978472}
,{167503462, 38951599, 1900173882, 1958753685, 449394992, 195073752, 972904610, 544495248, 1117815114}
,{524746468, 1920573749, 1153626273, 651059922, 854377130, 448422297, 674555922, 629713714, 2137215883}
,{416965971, 918319397, 1756114414, 245637043, 343430591, 1953497313, 320771078, 2066102448, 479288193}
,{263390522, 1671110345, 1988347723, 314212821, 1420378877, 1793672027, 2112181790, 648691018, 978588656}
,{998893422, 2056054170, 660163555, 2088618751, 702155549, 1921189837, 1813108212, 595037418, 279144088}
,{1539346154, 816228068, 1615584931, 103930926, 684541693, 1780695467, 1743960794, 996783201, 443041755}
,{150510525, 1452172420, 1576996487, 2024976832, 1270920475, 1936364648, 1758262319, 1302210838, 600398317}
,{1585750596, 2123791680, 1244579270, 1151556803, 1488518691, 1139282242, 1060578850, 1854096029, 1805952722}
,{2041753275, 1166253998, 1892270849, 1226528131, 1682466410, 2062253087, 1922101545, 1090663502, 1515482549}
,{2047844705, 1751937589, 1814392436, 196114955, 1589587318, 1572062854, 619925343, 301263632, 625151588}
,{651472542, 1006945562, 425812063, 1602240689, 1730929540, 993678537, 1903385675, 399184791, 624847222}
,{2133292870, 1571485875, 1407349081, 861463532, 1579821175, 1898575194, 98602669, 1793320239, 844784909}
,{730789890, 2023683424, 1870038081, 460077793, 106912621, 694165139, 1519706100, 2075664305, 2064784673}
,{1366682236, 1115529983, 395571054, 1721054062, 500723368, 207694471, 1503993995, 657785252, 1558988718}
,{1076362069, 1798837596, 1942883948, 961910767, 559288357, 240994866, 355297192, 2126046938, 1747907211}
,{628299032, 1568637984, 1727002228, 467992943, 582594107, 489739864, 1985090005, 310082353, 1869929654}
,{94760650, 583305894, 2074673943, 1991469711, 304647757, 1623881490, 432573598, 1625320587, 487404828}
,{2066465019, 2090178646, 1594328122, 2078969693, 1154649322, 1259541707, 1905107801, 1519556145, 1159675583}
,{895085680, 1963782608, 2104952676, 1808317680, 529158016, 1650502620, 1527276759, 1166396632, 1409409383}
,{1729345243, 1306402272, 482577818, 737116431, 1447421425, 1023253984, 553935653, 1939103323, 1090857809}
,{2016037859, 1545143026, 1172767164, 613561883, 1457402766, 732155593, 164893430, 1629591006, 1531291008}
,{283847497, 1801444704, 1293134334, 691826450, 1193627701, 1379975533, 58523621, 778717365, 1457947535}
,{821183652, 902587135, 1879027841, 1028735675, 1676122846, 1903663045, 1749616687, 1167989111, 300959016}
,{1472129492, 841121110, 908649993, 970421216, 1208938670, 1614365976, 2061608819, 141483947, 1911038565}
,{269452246, 686772962, 1902901736, 1213480377, 1771316637, 1236709770, 396311493, 323152078, 1443465045}
,{1029485271, 731203554, 953219060, 273126457, 751845459, 1551999715, 2020631895, 1176518029, 1306431790}
,{1419965073, 643245435, 1779965963, 1636982683, 184609028, 1542122030, 166481429, 397598177, 1870810766}
,{1247861429, 1806275313, 308074826, 619986808, 924882933, 188407807, 993751370, 1345915652, 1742302643}
,{599647232, 2000369910, 2083347069, 1213240746, 562505679, 931499920, 1757322097, 276055465, 1002757178}
,{1932636252, 2099931020, 125537017, 501738785, 1697895693, 564514873, 1323821182, 253149736, 1179796556}
,{260075885, 2040349319, 811858437, 729147395, 1551280132, 338710024, 1122806658, 121416700, 1508784287}
,{1305552928, 1598771188, 1037722356, 62473197, 1046699198, 2000821122, 552183328, 728202491, 1972944332}
,{509201895, 1408587283, 1134964849, 2041640515, 582300407, 693495708, 1751234810, 354966508, 1317055588}
,{1303147228, 961406249, 1173247523, 1737978189, 1314254776, 1364298178, 480994796, 1140133281, 1022582310}
,{1214726185, 1937767947, 2118578232, 1961567074, 1717423596, 2049484694, 757013495, 1159741439, 1204139910}
,{510264271, 1827402698, 1413529477, 1918018869, 1932026946, 682442562, 1389422945, 1968591750, 169278010}
,{1056171287, 24138713, 572456414, 2132999492, 1764613686, 406098039, 322947918, 1805219356, 703290182}
,{1837245566, 1821568270, 1642711518, 1504952911, 154329757, 1822777416, 575242693, 551998692, 1249620403}
,{1436769833, 529861437, 169192894, 1676740967, 1968499337, 1422856366, 315438260, 1566337576, 990784456}
,{496330537, 1677185697, 774381754, 1384679993, 1421260484, 1181347462, 575658507, 1934818792, 2032564095}
,{1150609023, 1645537618, 1476042798, 1556290112, 183406315, 1564947007, 1679930622, 1183062759, 1809443814}
,{107679818, 935328944, 305182780, 297007152, 1316894110, 682892643, 388292902, 375853001, 1292719742}
,{1159525526, 1777607226, 823155609, 1622622319, 2016975448, 2016018386, 761794268, 67504137, 1671226453}
,{2055981591, 1509963515, 1409020949, 1963490739, 1586675444, 1971485964, 1148497591, 595148941, 1485747497}
,{1352231775, 59852365, 2116842465, 1381470563, 594880555, 1197183979, 1590453890, 1898553525, 467257733}
,{1981981690, 892044988, 1600409512, 1864608602, 1960436332, 1272093220, 1155176885, 1997167604, 1690924400}
,{1343019630, 2042352255, 104541280, 1865154188, 365899493, 498553561, 37421716, 1422559858, 1800738825}
,{556658466, 1618255052, 1868683177, 627613784, 439658035, 2067669683, 1467121335, 1724734052, 1124491305}
,{593813029, 61437219, 959676791, 1595779669, 428073552, 819970300, 672687186, 2004584248, 585462524}
,{589506868, 715770711, 217679424, 1269352674, 534561062, 1368436766, 382971650, 61570773, 1908875662}
,{1140812398, 1681209913, 1901036319, 367970173, 91591647, 1067177209, 346408290, 1320469095, 1240040199}
,{1418471223, 1857089000, 1475679116, 582520281, 205799578, 1682746619, 779343617, 2135111115, 1069123423}
,{630870701, 327211068, 380522142, 11063386, 1446026407, 213780505, 842569877, 1541525029, 1872860288}
,{569073780, 656699519, 1694230899, 782947865, 580892177, 457523696, 1104600876, 2114324476, 1558865528}
,{554799857, 521632850, 680652494, 300995519, 2009144614, 5330353, 2074651241, 1829725414, 724495625}
,{942219264, 1648722318, 1457570136, 2076132118, 138652350, 1748442305, 1382486241, 1699636116, 1261766627}
,{815365015, 1988084037, 1877673235, 615075910, 590534597, 1165835370, 395889032, 1765004937, 1798143969}
,{1648265420, 953347173, 1114567507, 862843432, 56829526, 376454276, 553311781, 776558491, 966150060}
,{2082576863, 930072806, 621825541, 1594753343, 176492615, 523293451, 1619788839, 920453067, 605953318}
,{701034094, 2030074050, 1462994527, 1058216445, 122407161, 1140467023, 1385953616, 143635950, 1145876402}
,{538222559, 1923577402, 2110569094, 674110487, 477628269, 1627825324, 1970326960, 2131979698, 1664170657}
,{1700725382, 119647178, 1550892453, 1366958948, 2084566890, 1886625306, 1534818690, 1903514230, 344595086}
,{1385797973, 60433709, 1650118736, 33384039, 957644357, 971268789, 129058654, 1491452063, 757758682}
,{2102416586, 65112053, 2083551662, 1470724268, 235001699, 2120994989, 160087410, 340951851, 1296784557}
,{300760221, 1014582586, 1026821242, 1553025580, 2121475489, 1016690691, 1852095506, 1549037247, 183448082}
,{1111421159, 2126681308, 472687233, 741417662, 1222517844, 1630309720, 2049562478, 1956806077, 215513170}
,{1170523436, 1457723623, 1578689433, 8472112, 573522605, 618543691, 395831500, 2017414734, 1370666001}
,{1280461345, 1067618791, 350478701, 2144875656, 1156159905, 1156328232, 1234254008, 424396565, 1544492019}
,{1519498198, 1599946245, 1591842551, 1989044134, 729476589, 853373511, 326794181, 1982040353, 1431626836}
,{95586490, 157854048, 460248005, 1475852801, 2017151587, 1198140600, 40898795, 924667311, 727150387}
,{253475368, 303741073, 61520603, 1827914903, 717658381, 1074018153, 1081478620, 1449397386, 854614390}
,{412799901, 1587050682, 434458848, 1317586860, 510189272, 1549944899, 1932087598, 426607836, 2078863447}
,{1277521508, 468262381, 1831512806, 959838628, 1570309460, 880123312, 664476188, 1586265421, 1122490745}
,{252496774, 1705995803, 2100008057, 1781210528, 610186719, 973376838, 1544577799, 1133234116, 1931541696}
,{1893321191, 1305644235, 502385748, 1392237567, 298069941, 1824097824, 293448239, 1182156501, 2139172995}
,{399740898, 239063798, 643445332, 1503214092, 284010284, 2031889766, 576529008, 12938751, 1460597727}
,{1033545183, 822700471, 1192169599, 1578513548, 491506220, 783082204, 1347587557, 784735356, 1934835763}
,{603668926, 1443927199, 1790770295, 2021240672, 1085889182, 1876888169, 2100227077, 964038648, 1607469114}
,{589816837, 1216622654, 790477698, 303154437, 1366019987, 688064214, 2052893776, 1302628533, 158480724}
,{71258286, 652521893, 391939259, 687778500, 1103228172, 1271826587, 698419230, 1517073593, 1968116785}
,{1763273811, 134511335, 2024715531, 1733513426, 1489570240, 698481507, 384382633, 847360228, 2051987619}
,{1373627429, 2098903205, 483600411, 483141867, 1310375215, 437017599, 223270069, 1371364984, 1259894200}
,{1188913832, 1687990983, 1564595371, 56912222, 839757312, 412877683, 1742210729, 413506108, 1554023633}
,{333878157, 1591161215, 367113531, 352351632, 443942553, 1152181313, 1359714442, 91199942, 626770938}
,{1536434973, 901681514, 1751302415, 1924063993, 694754263, 1759793644, 1564871369, 195243008, 1485412669}
,{1548219053, 1979062825, 1843053559, 566075770, 2119350760, 685566880, 1603105151, 1186104973, 952594743}
,{1998628166, 884895768, 436444206, 605134758, 1808086276, 829621004, 575573584, 833180733, 764853743}
,{1905107722, 1909116090, 1953400399, 915112734, 1003466302, 1457076752, 584430077, 388439660, 867512421}
,{429258245, 1844867947, 1685159135, 455922488, 1193860205, 84014826, 152515393, 1798406471, 1745533946}
,{1459586820, 1798056886, 1603326090, 661070764, 1439992998, 1287201499, 1045431383, 617292327, 1975559960}
,{413675905, 1966037892, 1425614349, 510384558, 152332911, 210638590, 278149019, 305658219, 2085461999}
,{1535470393, 1383000287, 588219292, 488046713, 153228518, 93966373, 1951195432, 1093618500, 1917945690}
,{1644068647, 31251009, 1456867641, 1956305524, 164175860, 207446259, 1276887268, 220904792, 1375349371}
,{958856143, 369867509, 1047392691, 1986103047, 864586796, 356390406, 1566901143, 1759810194, 1751891781}
,{234276797, 640047063, 1915085077, 654355000, 1464907629, 235927637, 1510387857, 1951080350, 1855771330}
,{211780435, 2021921089, 647892463, 1384322200, 1354523515, 1410927025, 690938916, 712996850, 31232794}
,{1751944637, 1272104129, 910106382, 1488801641, 1816271898, 336800959, 684851864, 1678600218, 1014013457}
,{284336500, 283655605, 1591997379, 478004065, 1083146124, 184137830, 453807315, 1929784240, 407721640}
,{2048520758, 368041967, 1495191520, 235287456, 101384086, 1583794984, 1370360005, 292532366, 643242852}
,{2103342379, 1312753710, 689930560, 841299291, 773033125, 469209695, 2136319902, 308335690, 1738857457}
,{413655679, 882020609, 1764721498, 1173126636, 1280407774, 629866939, 1436893887, 1421833052, 2071256394}
,{845286788, 551250710, 1578652747, 1739753299, 1493217213, 1114393793, 436995643, 1908927371, 2006208639}
,{1662892811, 1356084002, 1676740334, 84173488, 1411820974, 1955187524, 1421921430, 660242811, 214368349}
,{576287284, 1354478177, 1478181193, 1031549141, 1920320303, 1582274357, 602262621, 928131406, 92235758}
,{45571754, 1627089989, 1233442969, 497888031, 1969599846, 966894781, 1002403978, 2079781921, 1349050356}
,{1164043895, 1714916448, 245886657, 1608124340, 1709150384, 770630173, 1372030467, 1879532953, 299694574}
,{1912549594, 1821178164, 1143210541, 166648689, 1308214400, 507540712, 106577833, 1525210818, 559642249}
,{2096016015, 163735636, 842455649, 1934104581, 1611709380, 1040742766, 1122440775, 1004379398, 1045428226}
,{365864355, 1492174029, 1844601305, 1558897888, 679198928, 827250593, 803444321, 1385066558, 2127160915}
,{1175408101, 524942709, 1622803019, 264034504, 87649688, 220924056, 1115982870, 947182323, 78258422}
,{792746515, 1426721958, 1710512169, 638254992, 1633081624, 612389766, 2140420845, 425978608, 195498923}
,{1547722614, 1120841582, 621268885, 1976123762, 469534678, 2082377220, 2039920505, 1799022497, 1665237904}
,{383526381, 80209027, 1813798528, 1098959813, 1165956383, 53348976, 1365629657, 298866886, 1705598067}
,{1559500958, 1566104373, 783540322, 1313667560, 292816618, 805530673, 1897468957, 327430878, 2110951093}
,{164244733, 1323463668, 303532759, 1228414524, 1101007457, 514358735, 1734119937, 299349999, 888860775}
,{1761764813, 791895146, 425206129, 1362894226, 843541425, 1571141832, 1163796676, 526068660, 659417394}
,{955424070, 1203069699, 1622331422, 949671905, 1584693230, 1942413098, 727738473, 2090827810, 1926645320}
,{289176373, 414080424, 82525580, 144887340, 1299169713, 1708733064, 2061449384, 2000360432, 1887533750}
,{35495351, 627100120, 803600014, 2101626142, 1948597897, 1795792753, 1570936921, 1659172402, 1582635921}
,{195766052, 213219612, 1290935459, 356961276, 1966225506, 1433191682, 1004066735, 1616150654, 756327683}
,{795705912, 1209933896, 702626070, 1334234787, 1322414671, 72443368, 1536264350, 1870284088, 1526808699}
,{1766563988, 1850259116, 1792822364, 2020343494, 366957525, 2090955179, 953613869, 505462634, 1693284156}
,{1110848268, 530073383, 1571657865, 977003158, 1988568670, 1000269948, 2080556842, 463556628, 537833662}
,{1185022118, 1844987974, 399206135, 659549632, 1567107440, 1215481868, 832780940, 368076303, 1760403281}
,{474820529, 506402005, 352844971, 246477497, 1941876447, 1306582461, 2915292, 1533947333, 653375604}
,{1813957660, 1826862239, 930142102, 1889004368, 35102199, 1191977058, 298201767, 388769379, 1939142381}
,{2109216097, 1110283343, 541158351, 1556960907, 1944022368, 49935162, 738429409, 117394608, 614444067}
,{28341930, 1635381459, 1756412386, 782302569, 372994497, 551719960, 1563310400, 235143929, 1257061763}
,{2074424817, 396887096, 1751919495, 1289902099, 2073972760, 1496199510, 154507843, 1557946901, 1636065527}
,{1622156049, 509394218, 122150654, 661615637, 1571900394, 908007242, 19096484, 1084428597, 870227838}
,{1733312990, 1715006435, 373870324, 1758647678, 2070691479, 34605065, 898629463, 1766249913, 700884733}
,{562052802, 570688881, 532211183, 328368052, 2039555316, 588718765, 863030527, 777926644, 49294073}
,{2044553845, 198926504, 1089919052, 27148878, 605670658, 20203570, 9105979, 2064071250, 956987544}
,{1102189186, 1216791771, 658820823, 486962387, 1026949328, 1475270650, 1216217919, 2101395580, 291460875}
,{1516059967, 413984823, 628524249, 492886937, 1768151242, 634983111, 1587396166, 581543373, 812991785}
,{1995710601, 184975226, 379537105, 1039855070, 846863493, 1741334345, 306940996, 502928992, 1959328534}
,{1449022295, 370510123, 145221576, 1907226171, 1853072678, 1473965055, 1239946542, 1687993246, 1906520136}
,{962612479, 1023219462, 1706029418, 983866383, 395009028, 578930912, 644410140, 758322834, 1632834589}
,{1863394742, 1018259219, 548238544, 1255559579, 1270488087, 580819424, 774448475, 447631701, 1221191174}
,{979361776, 1461834244, 1275511934, 1408726113, 1662106378, 689560893, 282546345, 1846705804, 1445256076}
,{1445700481, 341867744, 1634790720, 1632186225, 870177569, 2090730405, 1439905990, 54477305, 1387981424}
,{987170439, 498299959, 1814006099, 1310969200, 886680585, 45416123, 2120233003, 891010196, 145330287}
,{1020411739, 1988871904, 1460860449, 1841334321, 1138980170, 1524353613, 175717408, 491343040, 967010106}
,{656248936, 1279890651, 99237757, 1655926002, 769584756, 620706794, 1385228932, 312102417, 338226363}
,{874185807, 1764756575, 74015647, 188244065, 1495459754, 62927172, 1340481325, 255946513, 654259198}
,{1893940156, 1251292355, 369904389, 2016432059, 102055195, 158916571, 1495399595, 956171289, 733762885}
,{272129840, 1089479965, 116580089, 1204425718, 387846216, 994234521, 2023455836, 66719446, 1008739504}
,{326683314, 752645832, 109538896, 1982985350, 1756675287, 2141545225, 2044352626, 1911707025, 1624448740}
,{2018835270, 784821895, 1039957470, 390317070, 1248581246, 1151971374, 2140006042, 2003546967, 1298959084}
,{834613073, 1798380942, 892742648, 2005701017, 1631003476, 754647090, 1702357990, 336133897, 1191234065}
,{920381808, 200184647, 2106499752, 1883650044, 322691943, 737044473, 1096914910, 425988002, 388344507}
,{1529620847, 2120428000, 359488086, 796557186, 1000989815, 189196333, 1689133740, 205360377, 606991136}
,{1337539041, 512884878, 1727530062, 286305870, 80686088, 152120396, 1244791194, 1353867505, 143703373}
,{1060861514, 1948054679, 1473471678, 1263155918, 1244198757, 932508055, 610222947, 1801197163, 935902297}
,{1832056837, 1494275533, 607741975, 1855117362, 1306832105, 530933093, 1817779763, 1541016992, 2055966346}
,{1527891094, 1351181283, 1739978358, 1669880929, 1913381948, 172174306, 176478798, 1393364985, 1962575115}
,{301897737, 874063706, 868577897, 2133183136, 144578053, 446276951, 1893496970, 396717373, 1401591343}
,{470327144, 1016249492, 1599630548, 1411539783, 509370854, 1281131721, 1258608628, 1830020222, 83612141}
,{445488022, 1403373031, 2004209575, 331216254, 849610401, 1462200588, 203949181, 367208223, 1043210070}
,{738749595, 507307114, 1077645325, 1798206599, 474449, 1859357491, 1607986209, 54295420, 2128954699}
,{823700950, 1979499476, 605037409, 1988642335, 1814909088, 617462320, 388925597, 1530395829, 98154542}
,{2131729581, 1913763198, 1623776273, 1851993761, 929432605, 1275032027, 1929134340, 1176796742, 732302110}
,{1685905000, 250438788, 1303558336, 604323397, 812123545, 1807855685, 1207679758, 2114370764, 501590507}
,{750628464, 410716948, 152562100, 602884605, 784698536, 577493715, 1561052035, 1667129128, 1060196800}
,{1774769935, 1982770514, 1945095517, 1494603464, 1379281507, 824730125, 45827238, 1263815566, 1581191153}
,{806365178, 1694371318, 497803947, 1715093281, 1635552374, 1197395141, 186089830, 576220017, 1111752821}
,{1611155275, 122493254, 704568411, 2126053535, 1788180551, 1169201643, 617470383, 84577801, 1865952503}
,{580435932, 1942471493, 445889325, 1742946007, 11564829, 880325458, 2070510697, 696144716, 1485901096}
,{1412326130, 1330154838, 736700667, 325763702, 1293602454, 1909582516, 1786333704, 130043635, 711216270}
,{1908919849, 1858318850, 987386879, 1252264394, 2038716858, 9080897, 662086018, 805785923, 2078814332}
,{1280995392, 31082115, 1147166018, 1562791995, 1188745831, 871223916, 412740918, 1203584309, 1733858964}
,{1426433499, 872397622, 1275080167, 110231023, 2080732618, 265771715, 645189155, 1495027554, 913227880}
,{1976094693, 709216796, 982171127, 697979231, 155169301, 1413448024, 916961048, 1236036323, 44679135}
,{1131649220, 183951913, 2121524699, 277982179, 693785924, 1935358333, 692530006, 679841771, 1636957310}
,{759893394, 935272515, 97269181, 984753698, 1998312192, 25064733, 1484360895, 2075840120, 1271768150}
,{352583401, 2133494686, 1458740482, 596074212, 2084921840, 2117464678, 502879480, 420066394, 1293653508}
,{180914388, 1834029436, 743159419, 34010776, 1307056237, 482219212, 1862872883, 1851611461, 766366408}
,{1857289423, 747301768, 180767786, 963886375, 922967276, 419373017, 1859348330, 1386856283, 759690124}
,{1986036610, 1771092373, 487089987, 635018477, 1964886837, 2237736, 56810050, 1688876783, 1695902510}
,{953434726, 603009946, 2053142980, 583090698, 1972079152, 913552886, 230160917, 180411830, 835193188}
,{1779941981, 1531086100, 2107592763, 2127905544, 1111734829, 1015480589, 571058551, 961667410, 1848893059}
,{1196004493, 824695397, 1910962071, 1204802994, 218759982, 1801958195, 743405791, 2125726973, 870619912}
,{1561792146, 1146560213, 1116979035, 1023401749, 1752798107, 1244178056, 252019986, 1579439848, 2058987619}
,{955619211, 1850138070, 1388524789, 1941952926, 1625159278, 549303705, 1733139404, 1764850864, 1436958205}
,{1917545445, 1964647516, 1628862150, 1267102734, 170252932, 1830377522, 1202978382, 1060808665, 335840811}
,{526640473, 1697108947, 1999356295, 1148566770, 693813567, 761165505, 122335633, 354130022, 784616424}
,{102166420, 1834993073, 1888459495, 2128345819, 1762631008, 1799234826, 1902972400, 1462305479, 1143449908}
,{1702327287, 134798764, 390784740, 576756936, 310436941, 290603490, 1362815342, 314519931, 1381470971}
,{1492764118, 264896999, 1671654000, 64427508, 192586244, 1675164840, 1085346306, 328801395, 1688118491}
,{647443981, 1518164365, 1259358753, 446476736, 1787311236, 1398933154, 125487055, 2013169002, 1297450450}
,{532400382, 425012594, 517904683, 687478723, 157724634, 453420825, 896078306, 1018096410, 875132714}
,{198125759, 1940244571, 1529796604, 451127533, 1491976925, 1763526373, 160943097, 72674902, 1681289074}
,{286009394, 305564506, 2098970265, 201449614, 582720674, 1708699771, 534557704, 443851147, 1178992049}
,{736531850, 426457222, 1733024379, 1570021293, 4323121, 1242412898, 1083431500, 1190820268, 446529486}
,{1166868620, 728564399, 1749331893, 1138025718, 1397648936, 320160382, 1852024124, 919314004, 1505385764}
,{2069885317, 378906045, 1431778579, 1116677941, 451732501, 1479807284, 1343342759, 539133793, 1658900044}
,{526011906, 1539188071, 841387578, 1968831814, 854980508, 1999328998, 2074357711, 276438974, 113981773}
,{824114253, 674132339, 1285552937, 356106614, 339747230, 1781556721, 769382267, 838442097, 1871653596}
,{733771500, 570433661, 1306910156, 1084171615, 284114145, 313613636, 767648654, 171249492, 1512667011}
,{825953456, 281563589, 40392852, 742891150, 1191502364, 1710466797, 1267090573, 1695273820, 598156527}
,{626120451, 1537610885, 1412538906, 901936978, 1680679897, 352101086, 1933548620, 225194961, 1709235843}
,{915236941, 178535964, 1157856922, 1515820876, 681277227, 2113060864, 847782349, 37981048, 1526977319}
,{1062081674, 1882876453, 1838360455, 2127291908, 1008597142, 866902773, 783325208, 564637897, 488851331}
,{646974196, 593993124, 1761675798, 1386867461, 1497836451, 2085539073, 280117846, 1933071309, 1787629134}
,{1822701570, 903957294, 1518607466, 1027977426, 1643609851, 1339753568, 1908687176, 2077923832, 57343051}
,{493126885, 1085523956, 1072175602, 1055697808, 179105744, 1449800458, 349997077, 611076284, 394725774}
,{1908853474, 1753577499, 241786756, 1818028627, 261180613, 441233689, 1053722606, 2093470809, 494833471}
,{1100098615, 927200885, 1195301905, 1257810136, 1445970666, 5295458, 978700130, 1989006914, 105573152}
,{2060783462, 806547530, 1801018024, 878834595, 680972, 300021683, 2018132982, 387236416, 1583135144}
} /* End of byte 9 */
,/* Byte 10 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{1115622606, 1978542745, 616051321, 579961529, 1895225671, 84836280, 586358623, 696858558, 1436576811}
,{151580019, 1112607596, 161884554, 1192504207, 2062974756, 952180407, 580240688, 1848413425, 1952900969}
,{1620981068, 710698754, 761463283, 75099060, 1551062641, 1002348839, 303267007, 720574884, 1728358161}
,{759678910, 436202962, 1201666798, 788357464, 784936449, 1244057167, 1517866066, 901349435, 2080076882}
,{1079977865, 7266769, 231822328, 1079894138, 370046019, 166145428, 1032664887, 1470080704, 285014502}
,{1476440166, 210889257, 854172855, 1859687511, 388006817, 338380293, 2131604123, 1435748615, 1496107537}
,{1369850342, 1421392223, 896662151, 1224468068, 620331437, 1156692968, 1350813071, 1932799499, 1983961077}
,{357567338, 1171360660, 747095363, 756290193, 763937574, 831463727, 1207702335, 1944207111, 1264581091}
,{775390255, 1462871463, 187811404, 362836467, 332651994, 654613962, 173444882, 516879123, 473866848}
,{1014898841, 615827470, 776759122, 502574339, 1128557394, 367987058, 433215287, 1102568265, 1279529413}
,{1605231064, 1971282747, 638537751, 444802507, 1553980007, 1838948604, 424402469, 1262629284, 1541556487}
,{820532047, 1866156818, 1512537023, 870797251, 361428266, 183106231, 482082201, 809442663, 400245881}
,{1670234377, 1960237195, 150043193, 679559468, 1209434186, 1053532616, 1499293695, 1765419410, 1665279216}
,{1463440276, 1164497131, 1910620188, 1230917488, 1098058750, 222796305, 222028746, 1797452521, 576324550}
,{1743945991, 2008458073, 45172850, 1637040466, 890187606, 431709335, 1504687318, 895651540, 576709567}
,{1442124680, 1297067824, 2081839850, 1113800896, 1404161966, 792324186, 301287832, 149356696, 922390393}
,{922143195, 431308486, 887840701, 247253854, 480015806, 1719171103, 608253362, 249113542, 607574431}
,{1798558963, 1527741054, 126698358, 225473714, 673858118, 1220367964, 129066300, 180300509, 1577120581}
,{1833184250, 1022997499, 2117212902, 1598928832, 782931696, 1455124835, 1071048298, 1009313563, 751794496}
,{1141106355, 690628417, 1738585224, 1119724273, 1476549344, 1901853928, 1931929808, 1302542299, 913285357}
,{68068751, 1722379127, 503281758, 841913047, 87929490, 732187423, 569138707, 1394333776, 1317899586}
,{926924138, 1220159558, 1553643087, 1990791985, 1399824047, 680987745, 1852168288, 54042896, 951412304}
,{139149177, 923584350, 152589370, 1604734634, 1520728123, 552759591, 1140239519, 1098276620, 1136331306}
,{2094062270, 147290024, 1366306450, 317865143, 1315249260, 488110053, 443848655, 1773130849, 1661021104}
,{540054927, 166366321, 89240199, 1356217744, 2016505093, 728966373, 1229966339, 818355343, 2052203270}
,{1946791858, 1864716342, 471624000, 1277700420, 1910410683, 968590827, 884236487, 1719836926, 1952712853}
,{1693622788, 548835355, 1550093041, 510324829, 1702172004, 815060838, 1050073383, 464921692, 680455953}
,{934627241, 1542026898, 115061964, 401290651, 395706439, 1322281546, 296508517, 2122873770, 726442443}
,{1786447349, 1567925915, 1519690400, 923879158, 534170806, 1909036984, 1281024185, 1012391605, 716704656}
,{271111811, 1452427271, 617792436, 11759060, 62001389, 391170496, 51386033, 1389638319, 1615737442}
,{1701121303, 644444149, 2089287962, 1927993610, 1450595070, 371174240, 1959376978, 1027463303, 1413105461}
,{259303102, 148719470, 570626178, 1132840679, 1187597410, 272711749, 1441544707, 1379919308, 1563619111}
,{356506464, 931294393, 742299243, 1048969583, 1881254075, 1236013860, 309694392, 81583087, 456835624}
,{1347823559, 1863520331, 1431495774, 38911188, 826007633, 1284407903, 1304075555, 1193097479, 301049333}
,{1337230614, 1161496431, 657115545, 518576901, 788688954, 49220622, 854636692, 1507944247, 1012527744}
,{267743091, 1348974753, 1736413143, 592293023, 1364780740, 288897030, 561557359, 288373765, 1800720834}
,{322016920, 1607196099, 1771338872, 257482990, 1116990137, 1494490177, 926144763, 255454870, 2101322756}
,{1836871675, 1221501374, 44717493, 154234530, 1700240015, 41725376, 1748100529, 508766986, 18933017}
,{1170432103, 342562255, 1330435463, 434077055, 1279830178, 255033566, 1885418809, 740548113, 269019062}
,{1719509046, 1755965282, 143368536, 1787221104, 1666678645, 1798382210, 587676586, 78295746, 136036581}
,{941305224, 1228415013, 2124631486, 1547583459, 69042372, 1328454914, 1015427172, 333190490, 1026167524}
,{1896084146, 1155364248, 1947410895, 317451070, 2065560473, 1135979891, 583450292, 1401733656, 1145456964}
,{493028548, 1032464922, 1969102309, 330481130, 882307419, 1718898389, 186371867, 1016870253, 1922267201}
,{1404908039, 722535249, 1673142511, 1215768056, 1885919498, 123443253, 878318096, 478859993, 966699224}
,{1038377815, 2084957036, 1864229854, 10899898, 1044460445, 1184653889, 268484987, 668549429, 1776835786}
,{1170964318, 1154316418, 1644908794, 1803788788, 1249324437, 529730542, 1674251469, 36684768, 2095478227}
,{31978486, 1853236218, 1259669161, 1392374329, 714262439, 338805195, 604216431, 214530937, 317054064}
,{939530890, 1098686485, 1117009624, 502172982, 995480783, 1843592497, 149629703, 771925133, 2121171357}
,{1885130272, 1099133364, 1848250487, 1157526288, 1169795275, 1727827957, 921388155, 298169522, 1280184307}
,{1639683347, 1258366451, 233282562, 847451396, 661152176, 1436096556, 1448635685, 877299818, 139348875}
,{801770721, 228110820, 735873004, 459852815, 6907620, 54865250, 1790411990, 906950442, 2051706977}
,{1772493477, 904471684, 328248334, 689099203, 1543011679, 2109460067, 341556587, 1321173674, 436355799}
,{163152531, 1145316536, 800315970, 2081716764, 1435717936, 121845509, 1724037683, 1800426122, 1020317158}
,{2145876543, 1437485487, 1296670655, 48803041, 1843046276, 1008551244, 1373174449, 534559625, 45337672}
,{1944054762, 858547569, 1568783102, 1537815491, 8687636, 392876767, 35411863, 1847543886, 6292370}
,{1381420593, 1216991173, 2038363999, 1156839376, 1370698993, 2129358336, 863591852, 2143072416, 191126918}
,{94923200, 84410579, 2135551948, 166564759, 1875593550, 51187459, 2010262155, 1073585740, 1499838541}
,{1285850109, 1901214835, 611699982, 622013180, 1919164431, 270051291, 1698298036, 1745595785, 1845076818}
,{757102377, 102552614, 547033968, 1398725636, 581749169, 1662927882, 179757574, 691565391, 1520817288}
,{1914379236, 487869687, 824079150, 1820250725, 760262296, 1535823872, 1575030983, 1591112428, 1062366405}
,{873936607, 1958734364, 97915700, 1589009979, 1886188420, 256992181, 115342619, 1441265880, 1381745362}
,{224167644, 42349563, 2144266732, 1328140664, 1572609565, 393387617, 1684458519, 631206000, 1351687465}
,{811091423, 568118368, 358364107, 2001124696, 1682431668, 606358917, 1971499586, 1955344935, 461190029}
,{1579449245, 909182853, 55690973, 1295612330, 754803084, 893246529, 709422329, 2013682156, 1768001247}
,{1720706009, 31910836, 1698656272, 1488822599, 1900928727, 396031374, 915800197, 846169983, 1926891780}
,{499372402, 443494014, 2135628790, 746840415, 1882639121, 483462038, 1751668712, 1422246554, 740607733}
,{206974589, 1635974734, 526100768, 665580402, 1116294369, 382417622, 607002965, 990000276, 340297543}
,{1821623027, 503982490, 217533762, 2129977979, 741838856, 618033707, 752841241, 1973857727, 802607928}
,{151678199, 433214405, 737279372, 590734579, 1744919991, 272752853, 2037680244, 307043223, 1959577410}
,{851393118, 311728004, 630907460, 1503488830, 431537331, 1946475695, 1560253702, 1172148399, 2061394856}
,{1318341960, 942085324, 1162452016, 686792609, 1887121036, 480665638, 779470678, 199229507, 421526338}
,{1420886710, 71052633, 1423511525, 223674986, 433289308, 647484108, 2055885546, 148360139, 273989077}
,{1479603325, 1532955918, 483955395, 2090315129, 1701096945, 1551965666, 585012506, 382322199, 1459909993}
,{1545397516, 1981875536, 1321626107, 561807443, 603287603, 1011489676, 538107991, 1554585652, 1347672813}
,{828345582, 821303793, 705899285, 197874007, 1480800678, 746131204, 874250093, 1287750845, 1566958794}
,{257490169, 7528585, 1327669283, 1904387460, 521639513, 883774667, 1769967426, 1929143955, 1457051864}
,{530906104, 1456387897, 2095374903, 1344303514, 1204089606, 1700067490, 426609458, 980241839, 753630780}
,{1470976507, 389154296, 225620009, 1724092466, 64988395, 625931795, 1617156616, 728590921, 2064839402}
,{1619669481, 2120634051, 1833929716, 2103378281, 184980004, 1661123915, 715665434, 1208650285, 298823316}
,{419884952, 1251264873, 1243347151, 431881814, 1866990599, 245311681, 1264862245, 526050940, 1682872770}
,{1546619983, 2097527949, 2120341569, 1937005333, 309345790, 1878333502, 1677673208, 61329522, 502628822}
,{655277273, 1913578384, 173238620, 1861626965, 1968286726, 932612062, 61572763, 14715398, 1748387972}
,{1332829807, 690370390, 393250083, 819302645, 1068681583, 975499234, 1555063904, 704527008, 1681090589}
,{1723649555, 1276322140, 438922991, 13621944, 2037152468, 136173884, 820758861, 1981214172, 484734049}
,{933672374, 1197794196, 367777674, 1319769356, 1629087696, 226763771, 1469526360, 805435130, 273443577}
,{841806566, 626653036, 334756486, 1040379614, 1237549554, 976265832, 1281553633, 1987700213, 1937718850}
,{145820932, 1661852351, 2147354183, 890605719, 181120360, 751821566, 380978082, 972011755, 1451528981}
,{22852396, 2046145685, 1323576237, 718173636, 2019711520, 1608650617, 438425974, 2067963098, 1081176071}
,{2026113239, 2139232759, 941003531, 1443168292, 1882315761, 1322718943, 800174448, 2022100694, 530335424}
,{784538248, 1628961006, 2064986390, 1705595734, 1170706704, 65882431, 1075559898, 1114401405, 1317537124}
,{604522103, 1097586923, 905644032, 826649928, 1732418514, 1208126734, 1986038228, 990530007, 2137352288}
,{163621726, 923783438, 470854386, 1515245106, 140405941, 1510760928, 801081520, 1234866574, 1211441193}
,{521711829, 1937339834, 101883479, 139815728, 867078539, 573624688, 760108297, 1699138924, 701876645}
,{638767152, 114259548, 652702180, 1245229484, 1146907247, 1179181233, 1635853119, 565306710, 2022791722}
,{204606349, 34480233, 1058164842, 1315934289, 405772822, 929235652, 2021422552, 1896926584, 171345247}
,{495868311, 2104107863, 550085901, 1353660799, 1527567548, 964186313, 1608485062, 1005439392, 316670302}
,{1057728103, 1545122164, 259093828, 983831014, 1874374280, 1497039875, 1345742607, 1264440378, 1217071783}
,{2016450557, 737658419, 2070728720, 967976405, 2046325155, 1857261824, 1850183811, 1501528077, 1198667781}
,{1770377353, 1709151423, 978199447, 625822938, 1928514846, 337900073, 108315145, 1124783549, 353724598}
,{2014417356, 1816594190, 73728109, 1207850845, 1254859381, 682051386, 479255218, 825363154, 321737183}
,{1909114387, 424987994, 1669197738, 698302689, 956397302, 508417235, 954380794, 1867507964, 732266244}
,{576412311, 219927542, 1425640154, 940154247, 692914386, 1199299239, 898511238, 410926949, 1515457595}
,{255981135, 656245923, 774817965, 267641124, 836962697, 29781614, 120149306, 869833961, 639894993}
,{1442626197, 63076665, 548350804, 1161216354, 861055426, 1383518596, 1782106213, 1933343550, 1258127007}
,{144116608, 1814871471, 600673525, 1164167562, 484642972, 475940470, 1507836184, 1065921627, 863189679}
,{54510256, 1122912860, 628920450, 1337322685, 1574147549, 674337541, 500276493, 410606636, 1000938497}
,{646138325, 356472363, 1397813097, 1029213772, 79774947, 667194861, 1271669078, 369381230, 1024233340}
,{547167270, 534905118, 195847366, 1979663669, 1571517745, 1417138397, 1525210027, 49975789, 372910213}
,{206733469, 1835873626, 771974421, 322961460, 1631785153, 582400906, 1212379198, 1831364023, 1312930292}
,{1147595243, 249991232, 826741450, 436190674, 1432493013, 170455444, 1196578774, 1520626656, 470776954}
,{963334165, 39029485, 94137689, 1258405370, 1656552290, 238733439, 2050690450, 1614405536, 662013623}
,{1140643742, 1218403391, 963574537, 1480914423, 919450747, 1033926446, 1245945375, 507962630, 1815405650}
,{48770020, 1629787889, 362971306, 1931310393, 106269455, 1612718382, 13268237, 1604543963, 2014285262}
,{1464228294, 2039300166, 128038599, 1900722177, 420136034, 882575169, 785504931, 659427810, 1263078124}
,{344057837, 1000409157, 460169725, 510643480, 260048328, 557866367, 1016535773, 1550720330, 791038036}
,{707913174, 155718270, 1411410570, 1898341444, 980954978, 1411187212, 58501587, 482001518, 597506082}
,{1620130347, 1863626128, 905681255, 874598209, 157032335, 2048199783, 510933878, 481022987, 740293566}
,{820242424, 2068478811, 1028999721, 1920457775, 1791727640, 63407045, 1543678167, 2131789693, 293981971}
,{460566619, 1789059341, 1484333766, 1740617200, 318386908, 1520486842, 1166911736, 902701363, 324563978}
,{1864355730, 2096773990, 1979940075, 536386802, 2007045721, 370507661, 1957636576, 1569200918, 1243077035}
,{987367181, 5509380, 1825434016, 1011229761, 1418315637, 1914788696, 725761243, 1079335873, 288928805}
,{1647327177, 1797098175, 275691252, 1856571431, 730565470, 898411116, 1679234279, 1161465251, 1505168638}
,{1159376498, 979490874, 1793167982, 148361637, 1446749437, 358960735, 1130660813, 1148494723, 1366898831}
,{422518277, 505732092, 1419804573, 6424712, 904451782, 35728148, 118809533, 1360705746, 1072449865}
,{1959029835, 2061956739, 1943385840, 1564149752, 1026756846, 839425385, 38974220, 1550508775, 2094691510}
,{1666165114, 146030278, 226542697, 772997895, 45694936, 1066111041, 2108261662, 249854678, 1890981081}
,{327764024, 55035392, 667944601, 2011073768, 1468520511, 369814713, 1934373822, 1548791295, 389572646}
,{889601868, 1614838066, 1194056782, 416114758, 10879375, 14987224, 1618307983, 171829511, 1131855052}
,{1176735665, 1821681407, 811979138, 984519051, 792506286, 1946694836, 1848622863, 659623596, 132141298}
,{1364542423, 1842094370, 2045288075, 1129011506, 2073951336, 1686627069, 273852595, 496292186, 397010518}
,{243156371, 126697123, 1351247443, 1756031478, 115103809, 1222708399, 815925321, 1577831798, 1495597962}
,{190942247, 1362461681, 1584499177, 358633101, 1852525757, 2047251171, 2049077273, 1037320426, 2133708702}
,{276627501, 1234694793, 1542649613, 725113501, 446496957, 1245249350, 33070517, 391416273, 1705365632}
,{160043193, 2071572972, 243239920, 531138586, 891398727, 1263938578, 1733596700, 2071863388, 1025240000}
,{1248745368, 489670002, 335412578, 1666988852, 1149620277, 911602582, 231100039, 1478729656, 202568168}
,{80714118, 142616504, 707060412, 386642566, 999827812, 2047101845, 527445678, 2134771861, 360031843}
,{163551188, 1238355367, 761362153, 960197483, 541124498, 2040105910, 1121066908, 1426817524, 471235094}
,{981239367, 1165002384, 461579209, 1484657965, 1570732762, 1573187882, 125711815, 903787918, 1039658626}
,{89733954, 123299567, 1969796909, 1905741619, 1270193991, 1595131236, 982004247, 166175762, 1426780839}
,{987729496, 1013948018, 1997181327, 1116787215, 586344194, 238745965, 303007504, 1440231318, 2014998515}
,{1802940439, 1660652007, 1905320836, 1520515011, 1238077279, 922248671, 616166160, 1304856855, 244506468}
,{32535900, 1953538696, 1203837758, 507397744, 898721185, 1942151131, 1705200868, 769570899, 521997188}
,{44767772, 1136562942, 1107145374, 125786588, 267806233, 261900763, 1337602654, 1160864212, 1149993947}
,{82864898, 1028141737, 1987569020, 1056123595, 930521038, 784390658, 1498283925, 1068595133, 1952377112}
,{1522662843, 341850962, 1013339296, 259524274, 1197886144, 131041514, 323229672, 1434884077, 373554170}
,{546268464, 997632984, 398233311, 802567360, 2127244423, 1235935751, 1146771804, 830693958, 1621753173}
,{1657791963, 2010198565, 630315656, 1837999933, 358909430, 1752093925, 553953694, 999871046, 653199658}
,{340392989, 1419581027, 1167007387, 448017688, 2062595132, 119327475, 588371859, 134713355, 1478574546}
,{138636791, 962533551, 284109243, 337608659, 2111756342, 2095126680, 1698298552, 1031694354, 1106418224}
,{12184784, 488517983, 570127615, 289486866, 1679382424, 913750038, 35566737, 2102399608, 1802536020}
,{771767823, 925025364, 554603625, 1500344637, 1639428642, 977029173, 1608507785, 1474237824, 1552942133}
,{549959742, 92139596, 2028115079, 286871444, 1139656501, 114402051, 1021706720, 2076757739, 305417110}
,{1550537829, 1298226421, 1614974388, 1106517541, 1251138718, 1721905822, 1227055715, 140177129, 20960717}
,{733561477, 1712181235, 1800865954, 2030136277, 338121864, 1133312171, 437839053, 1125273332, 1232766386}
,{1905256815, 1724677537, 821807800, 776141243, 1912253717, 520966740, 639469441, 659701121, 1788732507}
,{282438130, 951347118, 591828844, 1463923887, 2037241234, 1926235596, 1635633303, 1213574778, 1397741697}
,{1816104121, 1075397972, 1839187854, 466720346, 1559775044, 547135712, 788696896, 1908663350, 1965484695}
,{840083507, 96315009, 1522105073, 891540178, 2040138114, 553905058, 1113489938, 287070335, 1402792585}
,{1444904840, 1512757937, 2140010749, 337647430, 1001788013, 1007041243, 2109668861, 1854121158, 138097243}
,{218069171, 1421594957, 1854906254, 1056714891, 604052252, 378903106, 219051614, 335801732, 1775011834}
,{1832938990, 109712779, 1770032502, 342347, 1607139057, 541925956, 1150861688, 456534215, 910481170}
,{1077201134, 2024128056, 1413104182, 1466617554, 1347564167, 1002862565, 1598973196, 477480206, 1824691189}
,{348053798, 224796151, 293615104, 1163481009, 184385846, 2035712604, 10612422, 1374515026, 347550681}
,{1896210715, 1326842730, 1781559021, 892399537, 1760890739, 126298068, 448864509, 1701664600, 1165308129}
,{1939846194, 416407866, 530262278, 335025240, 814375482, 1240700251, 1277133196, 1606015539, 144895861}
,{260342029, 379665630, 784727972, 803788969, 1590773297, 1254913561, 452882408, 1504273585, 1601148454}
,{1278569570, 1046463331, 992378703, 559895436, 499040251, 1672705805, 943802583, 551687079, 198575591}
,{712106462, 1900153302, 1599340648, 1387554985, 1758385230, 1865664405, 1483229018, 653676464, 1959095197}
,{412027077, 1998140932, 2038733800, 701963515, 693693843, 1531808908, 214110494, 2130263395, 1558714390}
,{341277336, 1773546478, 674738364, 2061390153, 736423839, 1224602681, 353501608, 649174402, 723961163}
,{1122983961, 1980662834, 817757868, 1013688345, 880122508, 1694992630, 1298252271, 1740416056, 1988963806}
,{1114367025, 1240747636, 492352899, 2093198960, 2091879489, 1387590089, 973064009, 571510551, 925483155}
,{1175322773, 1035702455, 197160247, 1827652401, 268784348, 2020203855, 1100701633, 690787553, 1422873075}
,{766351858, 1541521018, 865540375, 72750882, 1139039545, 1058549503, 817170596, 1504728091, 1793500719}
,{1804813249, 1911265205, 917515280, 313159939, 1462398200, 98464562, 1097801048, 787923031, 220257498}
,{185377792, 946649973, 1737587161, 2024943741, 1299711694, 22774287, 333751790, 834388791, 41307976}
,{397189658, 1819657163, 75928634, 2121949548, 793305578, 1465892418, 1495601570, 524747175, 72773155}
,{1997064142, 900588397, 762018234, 1250579723, 373656114, 416364312, 1904780913, 1979770774, 1183974896}
,{593388561, 880910186, 740006676, 523712564, 61404018, 780598372, 1755287434, 1412600515, 1177606573}
,{1243076446, 879185585, 1605017025, 92164323, 1258980101, 1088353923, 1546215699, 1591098293, 2097960069}
,{232131122, 1807371954, 1934503388, 252613919, 871882912, 1940874426, 1899442433, 983673438, 312848844}
,{203817725, 1698115598, 1295651812, 404733665, 1939295853, 16166659, 1182619299, 2030926375, 691600710}
,{1095723731, 1556463305, 819889564, 1113967557, 1166010755, 206592358, 532833874, 283230607, 1818499319}
,{1278292636, 13971865, 173584524, 1725982835, 1926283157, 1284556360, 1871473445, 1607838041, 1470914025}
,{102518441, 178309629, 79677151, 1477607793, 1395290506, 249802357, 1817607426, 1485690288, 1734292364}
,{2009330786, 138469177, 340759465, 428422718, 255216940, 79448496, 2030939409, 683961566, 660075486}
,{1678009980, 1279506472, 2091190058, 293086069, 1340909336, 692202979, 1741544833, 1079409748, 1822109383}
,{571931886, 790111488, 1052389013, 1154224424, 1742408664, 1234180422, 835174230, 1935655267, 1816548597}
,{177544502, 1851228128, 1017892680, 1379299315, 1151714330, 2003835917, 2141911209, 1595998033, 1988979389}
,{1533537005, 1167814356, 222344053, 1588915770, 1963691803, 1167947316, 1531549410, 2126615300, 1219376448}
,{1859170717, 1989166438, 1939412972, 1947651433, 2112657142, 1016334140, 1583696023, 1715480372, 653245553}
,{1511765931, 1776302319, 545218278, 669446942, 981922422, 280816602, 661680832, 1726348839, 270326146}
,{1272929122, 329005756, 464952629, 1052393446, 82204533, 98228928, 1289565963, 923167906, 403340860}
,{2062939520, 1516647048, 1123858748, 651247204, 151812040, 794618216, 416245643, 827683776, 1580109636}
,{1884658517, 632506609, 1795105702, 818437073, 493213182, 1798237228, 2024716131, 1290902433, 1437261701}
,{286490285, 1760518919, 787651810, 1786953163, 2026458494, 49634584, 1199776435, 830160499, 1628528137}
,{749746560, 1909788773, 174421604, 190538826, 1425486508, 1647377477, 1423200587, 444037897, 947950697}
,{517463045, 831530797, 2110074057, 1308071601, 1229441494, 1187289643, 2016645229, 1568613412, 1968916988}
,{1024534555, 824116955, 2095459470, 1550207614, 1204834010, 2076142596, 939312832, 362454664, 1507471607}
,{1487520632, 362149663, 2102321907, 1246823760, 1063334418, 1130007630, 633077413, 273627389, 1933932473}
,{1115777766, 448041557, 626849556, 2110915188, 1287059109, 701764433, 1777096375, 952060545, 1490708830}
,{1900271690, 1445799995, 848827235, 1255733047, 1058469493, 1854446727, 951390157, 600623945, 1385122024}
,{760466954, 2055913628, 646385187, 582100360, 1914142333, 1145505100, 245458402, 710938284, 2094810663}
,{274165849, 1605200162, 1231750586, 1030348217, 1651725097, 328126109, 2083274089, 1267569927, 1959694899}
,{100610353, 298086438, 1792260128, 1214829524, 1515861137, 1658737185, 1748573025, 1139699877, 794843633}
,{2121638824, 709721394, 1133760478, 1241043347, 1935472060, 396644535, 458063979, 973346772, 735807231}
,{1992824777, 50443952, 785277964, 491045862, 678347562, 1178316380, 372867231, 1541658337, 1964128244}
,{549465528, 1324182423, 1445285205, 1388439814, 1462869755, 1990862947, 1500102450, 1937145780, 784868732}
,{1499967310, 1013885506, 1846492900, 374330608, 1136447204, 1562638771, 333201689, 1224077658, 493271659}
,{88446787, 654635818, 399880228, 479202066, 483446946, 1375298719, 1958635141, 2048537596, 1484445705}
,{1921820872, 1313369631, 1643340652, 714157672, 645510970, 1929222514, 323867678, 386055620, 510391503}
,{1569025389, 1209073784, 352359465, 1160442175, 1321371561, 1626282539, 1823270041, 629671843, 1423108963}
,{2026158758, 772226556, 1377419250, 1228708449, 1813098425, 1995806273, 2074751425, 1039416362, 1664051063}
,{511227281, 1514809527, 651192479, 621884217, 1817153263, 1443254161, 253427705, 1070055432, 583640403}
,{1012398261, 1755309848, 1153189349, 1976258963, 536815837, 1508344639, 517894543, 2117118420, 1943308037}
,{596893637, 1756165695, 1903977777, 703804397, 1332035405, 1225130375, 938311234, 1878634018, 459765134}
,{1109012173, 143558188, 1906449633, 2063248404, 1057028171, 1151268298, 1288757364, 2041680056, 2060427069}
,{595916967, 1312497388, 1674516186, 1031290583, 354010913, 1855545726, 1669320276, 17531757, 25752251}
,{395474375, 49247700, 1263985950, 1339891414, 1252456359, 1521444404, 1160977138, 718801051, 34508350}
,{429426897, 1107807577, 1357011262, 1150929879, 51420477, 635385616, 705129567, 1655753807, 223525921}
,{1870965032, 250209369, 2012137451, 548395232, 1677337746, 883861796, 1273289093, 1498608420, 1152824799}
,{1910740483, 803378452, 812273707, 1424306854, 1497669463, 456741830, 1726155890, 1893894116, 405309793}
,{481948419, 1735822947, 806739071, 2037132684, 847865682, 1459766818, 81072320, 2008757372, 1433290790}
,{731834790, 2039299469, 1238364359, 1191836254, 1708180278, 709415469, 1956436586, 1151328543, 1627337835}
,{984168298, 980319853, 978650835, 1482919141, 442245949, 421112584, 1260758160, 1041653785, 614538036}
,{1821332254, 493176687, 280171187, 1601561433, 152927328, 1358948951, 985655060, 1256582162, 2075355583}
,{1731879317, 993796697, 1217534272, 308475081, 1321182924, 65516389, 13487530, 1377496269, 1805426871}
,{1867049024, 517604640, 14989477, 2012253021, 575975896, 293634211, 1093333509, 448470287, 1170630572}
,{338146380, 811031324, 1188910006, 452879004, 329458479, 692680091, 2127587980, 1855706858, 1612696361}
,{2012760707, 1220935057, 2050221669, 345764409, 799131253, 1268088086, 103152286, 953906203, 787386629}
,{1623267747, 1700808888, 449869672, 1344993217, 397806278, 1884284453, 1186234484, 1359307877, 1747409621}
,{1077262645, 1866842312, 1026954680, 110303154, 343667633, 2026671298, 102871361, 181672360, 280082307}
,{414759672, 556049480, 293498540, 127015232, 1712454395, 1699373783, 20840263, 441843687, 161944734}
,{2075251923, 1705470122, 2142749719, 685776374, 1032619188, 1301254496, 1257813916, 1963528421, 452758423}
,{1882414248, 686704358, 1547797076, 1531423983, 1346027543, 886852830, 163656762, 1688892481, 862289357}
,{160900642, 155255099, 1603455380, 1608172054, 678449739, 313688239, 1386290314, 478725350, 1584637690}
,{1487360936, 440482681, 1629717709, 1817886501, 308811788, 1637893271, 90087423, 2108478450, 644586154}
,{424528339, 813759685, 663284638, 2006452686, 1491509356, 557800584, 897551163, 2014432326, 1430703136}
,{1179883528, 964094313, 111044685, 2133899138, 1366877270, 1410128537, 1442860797, 1306775312, 50512452}
,{1424975290, 1603981378, 936424296, 1567927018, 1262794435, 31498203, 1247522472, 272300694, 523244098}
,{841341110, 756553751, 442857375, 309667938, 1700110625, 1535826186, 549563304, 546817511, 542356855}
,{707200829, 1770424361, 12036385, 2036326168, 1002028686, 341788065, 429281474, 1241832558, 352638926}
,{737115088, 1512608027, 1706194688, 157513316, 780307142, 2113611205, 1503301678, 1564093011, 1413990594}
,{691967740, 1108243686, 1318766332, 1825679850, 1400048168, 866054977, 693524543, 1905068807, 1484893362}
,{439434699, 981714438, 1123343454, 48924456, 1762163573, 720538111, 2052271121, 1585487393, 455623709}
,{1719340601, 394216615, 396759482, 1262421452, 1758001182, 2047305234, 1183441203, 1170339578, 1133633965}
,{1657704297, 1519841087, 447956352, 555279071, 2031569708, 617522036, 1949810557, 1698206476, 1007700712}
,{1394654741, 190454398, 1543081188, 2034809177, 1568716114, 1957183068, 1717060573, 682218700, 1329939569}
,{377633373, 1358004561, 16805736, 374110707, 1345739906, 474213604, 280327704, 705492740, 1098817447}
,{1775703557, 1677381430, 1576185713, 135246499, 1926398768, 733762638, 1104620425, 285500262, 2041222041}
,{1505165824, 167884955, 1871268983, 771124469, 1346205699, 70573646, 343329654, 6713996, 914359071}
,{721468982, 1782869723, 2081255132, 132193438, 2100069381, 1745103944, 1899965481, 1722063349, 316231203}
,{650154027, 529909041, 1576487469, 1682198877, 1505900929, 1053974408, 2066589885, 1495763329, 1948751405}
,{456448660, 2113339297, 1642483726, 589032793, 212298879, 1953582266, 1602975027, 779610810, 1013176919}
,{431312435, 667011605, 413934499, 1260379269, 613117992, 2126664988, 528595212, 216377486, 1181630966}
} /* End of byte 10 */
,/* Byte 11 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{1398966622, 1157247437, 916238461, 182928118, 1809268605, 882357484, 1036910071, 1001125599, 1351500463}
,{525501314, 1126053257, 1649640023, 538705899, 497139502, 297882703, 1867503624, 314576360, 387037776}
,{493227047, 1039551408, 1976422311, 2101408042, 1633135117, 1618541218, 1195184520, 1683093460, 1044052406}
,{2053804527, 415076613, 1989231709, 622589464, 1635041103, 447864305, 221866467, 1296555477, 859124139}
,{1226114955, 1879448501, 1827132510, 90592997, 599958262, 913437355, 970558524, 180394243, 858942824}
,{650681148, 1038466928, 1545372726, 1872646231, 1254857590, 1669721985, 1497840943, 1670524448, 1529167492}
,{2063004186, 336500128, 1084298421, 1932134653, 217347117, 258693757, 671633059, 1589688900, 1808439649}
,{1895731585, 1867327307, 2069915815, 1512868696, 1404866392, 1740692164, 1890170287, 1602928372, 2132331650}
,{353702917, 1636569458, 1999775804, 1236663178, 589528637, 1190650890, 509306169, 2024773660, 2028341503}
,{300551735, 1362211986, 1235079945, 1826801450, 1232815891, 720147967, 1734647433, 468055572, 210002982}
,{1766323542, 1703660757, 714693843, 2055191616, 855316045, 1300195882, 474373649, 708044833, 2003364724}
,{803448149, 927508544, 49655274, 1783395331, 1998593708, 1612669333, 881085378, 1260267084, 1986996958}
,{1776753597, 1244028694, 457382947, 1196935646, 363579328, 300042388, 1211361570, 1846700920, 1864717653}
,{1512392443, 1110028809, 43216871, 1945417866, 1013642605, 902814001, 1982089800, 1251686436, 146055623}
,{1445629082, 1700361515, 1289428643, 2102772721, 484607451, 1409139701, 903603930, 712769522, 449886483}
,{1103145403, 876725196, 1237695176, 1764973820, 590485398, 812746852, 94240319, 1624615526, 666115058}
,{935174504, 693368170, 322338346, 1752551188, 1611662084, 795553823, 366017055, 1087574307, 1982630353}
,{1220464825, 763320766, 732784283, 1896441275, 1415687376, 873028989, 1393591778, 1193935788, 1856803309}
,{1086212718, 525164634, 1058970802, 1134627238, 1214114044, 2077725465, 868637801, 937978190, 1959563501}
,{350010232, 1344503653, 1122547667, 724702066, 317915906, 363006613, 243876076, 209848710, 1318210681}
,{1633007739, 1552358037, 364535782, 1880176872, 1791760903, 873431146, 703079975, 1097227969, 226615108}
,{1572168048, 591933441, 1126961958, 1965234018, 1929670103, 900668083, 1133850676, 1138889552, 672065301}
,{604237682, 796882047, 606251523, 1508551869, 467589029, 800147223, 648322542, 1496387344, 373350188}
,{710593110, 409721883, 209617143, 1799717565, 620470793, 1234756118, 1710187784, 1630648570, 1123485617}
,{1023684369, 1405643873, 875128499, 1540151227, 1456382037, 2013721985, 707196716, 863135647, 1905903837}
,{934200240, 1254957147, 352087696, 1375644216, 579195974, 927987063, 349414771, 286981405, 1015623394}
,{1316860033, 1462526538, 479005768, 1742784992, 1375278607, 250006250, 1465245404, 1659445830, 1868214390}
,{1654454245, 1943344786, 828449314, 765684550, 2004057438, 1901156712, 1451534653, 1653511792, 566334852}
,{1880166422, 1979612943, 1781322291, 1501424526, 568938724, 1536303060, 1243616220, 1066158849, 1162150768}
,{553817157, 1728223845, 1591118988, 1685103103, 618489551, 1212265293, 415206859, 1432406520, 117147269}
,{1945326466, 533146905, 1791765405, 1517300882, 378356281, 496527736, 1188483968, 1960324952, 980732736}
,{881265815, 574418292, 1592380547, 1089008700, 475651698, 1876587992, 76581291, 95837607, 1193727812}
,{1211260224, 1769730150, 987136614, 2060036996, 743654488, 1775285801, 473160766, 391192175, 69866315}
,{1797616851, 318387576, 155609251, 199975957, 789739457, 870337684, 280487791, 1282921236, 2058703942}
,{2046050227, 2093091409, 648507465, 1305788038, 298935178, 2128593106, 547744972, 255083839, 102281652}
,{150514484, 933027114, 508943220, 1808840634, 1963306924, 1309843774, 1553736452, 406039417, 1499070581}
,{80611167, 1215338990, 1899410516, 1638862879, 202454882, 1938744358, 1006101626, 1930972404, 2103594917}
,{2073933312, 830946976, 701551158, 1353273841, 1532352338, 1393560505, 750675893, 338918002, 1628479402}
,{1590221295, 1128987600, 884564734, 1273403442, 278139764, 35539235, 335255090, 82322337, 2074688046}
,{1646297184, 3170881, 677273052, 285901470, 1285241028, 2003947243, 298014064, 611093277, 749235834}
,{2061876278, 397752882, 994261848, 1442725166, 1870965041, 1608831467, 1182790111, 62380986, 345893143}
,{1144631239, 1410710179, 586451439, 820401755, 1741051621, 902715063, 592623714, 2116222008, 1859367377}
,{1314894197, 150650687, 875768148, 1222620245, 1354377182, 1515798818, 130490197, 854826129, 2021555678}
,{249954181, 1961716985, 346432504, 1493174693, 6468776, 1981607204, 82366712, 605352885, 331466178}
,{1178215816, 1442452572, 470358948, 404027829, 919930000, 600580307, 1011775117, 1875267638, 48837962}
,{923457391, 1748741401, 208473305, 1457119565, 337368406, 459163673, 1443630390, 2025590752, 916901831}
,{238956149, 1067870626, 2110891226, 539956745, 422368628, 1950603615, 891309049, 1194366219, 2044449916}
,{237321276, 51718718, 716493265, 1775987367, 521878257, 1686643189, 1092535790, 319858017, 87805404}
,{1898798259, 1835481426, 1577814431, 439796891, 801798149, 1063527853, 453960406, 925155843, 800116151}
,{1033770189, 232965581, 1036827019, 951526395, 786405488, 837513656, 314200510, 1915298040, 116051174}
,{1351516112, 1834227575, 307253181, 1258920240, 612003897, 502203477, 1956843064, 1003521897, 2086132333}
,{1316584083, 1096099166, 107645669, 385932763, 717963673, 957417284, 202046349, 1281584182, 484157574}
,{1527381230, 1885301061, 2021681525, 2036597986, 1770456602, 276975807, 528875622, 2066239883, 1369395190}
,{779288329, 171399346, 805403216, 1257123997, 392992180, 865016072, 1791137779, 522851592, 1498484311}
,{931873234, 1644821283, 1418500644, 600622377, 196700765, 674420557, 860330902, 514254926, 660956635}
,{1826899106, 326344627, 481403760, 1089419963, 609544591, 331175034, 1868731051, 1616480101, 151150738}
,{315308470, 952397108, 2014400664, 1990710692, 671880614, 1915780967, 2095496602, 1472035246, 800913372}
,{620046641, 1737302677, 1984312927, 986410687, 2132131390, 1627660535, 877760828, 1538187856, 1164268911}
,{366175867, 951040144, 874255873, 588101475, 1267246635, 1574497867, 1074800896, 1929908079, 484404625}
,{1977908008, 1014461719, 1686246067, 2064669103, 1688443171, 1181187375, 735959574, 1162838158, 1066701451}
,{1479746864, 789710107, 1421892090, 513457306, 1437483384, 701295434, 532193594, 1830428101, 904207731}
,{5646606, 1161699467, 1961355606, 256464513, 1460009999, 1803968249, 148850713, 1600231134, 255684008}
,{872026952, 2059839395, 1350474237, 1622004364, 889421903, 895636503, 1359124008, 2005304420, 187565356}
,{1290710085, 746644977, 476082424, 226668047, 229293297, 831767359, 77589666, 843756758, 1866713714}
,{191936953, 637490372, 1037147976, 483966510, 1616649924, 1966176809, 1315353734, 1526831256, 652762255}
,{784768, 887625370, 391938724, 561722228, 1339358989, 1746344160, 1928201915, 1497464293, 63627210}
,{1385007571, 104960698, 2045726066, 380149916, 1890864345, 801324054, 1121017553, 1383658143, 1728706542}
,{1135397719, 2042057729, 2001441130, 737968413, 35965892, 1537294345, 2000259021, 145251938, 1917862996}
,{722286047, 1517234701, 1655010889, 435499663, 27148972, 2016053861, 1056766220, 65631486, 648423046}
,{800593399, 1704628110, 486994437, 1646677009, 588243494, 1317046238, 1958763536, 531613042, 616927294}
,{279126711, 808701803, 232647509, 831823426, 1533243993, 1892123460, 1717887945, 767433557, 942519701}
,{389056541, 1655211056, 1618029676, 1947512444, 518572996, 1626415449, 2015151910, 168467452, 1748099576}
,{826234399, 389486318, 1677112894, 2038078681, 1605912724, 2140413743, 1250090631, 634179319, 1276575780}
,{1632698028, 4179484, 269352576, 2055000285, 881121608, 291728633, 1563111925, 2044782023, 487570447}
,{388083739, 1429933510, 1732831862, 851772701, 258816472, 1186730019, 1210102173, 1824101311, 801497019}
,{369934423, 1383781045, 1880965013, 225583381, 1592311702, 1576426544, 1758712452, 898524211, 311460587}
,{207907950, 795734125, 1441877758, 465209994, 181098168, 1796708166, 1457274994, 1458402299, 1710766828}
,{849543287, 1215429045, 1860029455, 687438150, 73868729, 1669708171, 309263954, 2082434319, 1385126182}
,{380393887, 278358008, 1436743469, 1801092715, 804332240, 896839424, 1552346400, 1257771475, 1974724959}
,{302626196, 382875442, 1095874096, 1573891303, 220339406, 1994943191, 1578759985, 1558196220, 1822454376}
,{1646446487, 692924404, 701688456, 1100965572, 1195125918, 868287410, 1471529686, 1230175276, 335067842}
,{978640786, 981542559, 1508789559, 749214057, 1491109287, 1283263378, 844899012, 1741989591, 2062528936}
,{487521159, 287171863, 1906893312, 1194735938, 1500061430, 9165275, 1366814554, 72105750, 401082594}
,{1073474840, 1927559542, 2030890439, 264122121, 1577205144, 1551843348, 199255079, 87008661, 1654126295}
,{1341867453, 1189322900, 2060320144, 2044937683, 272617850, 2115533662, 1534631727, 111952208, 1092428036}
,{1515649155, 1195600715, 1655659861, 586405996, 407386576, 128252599, 1503397269, 1026584593, 1443946339}
,{1531393699, 1965827206, 903716797, 1713503832, 1531433103, 1322465229, 1407221608, 2079969728, 1662437157}
,{1100482959, 953180950, 846621591, 1259118780, 1764673701, 460637732, 1973888573, 1964122150, 3489125}
,{644452889, 1835123183, 245411025, 602321527, 45951756, 428448666, 739699086, 987671941, 1955124406}
,{20871227, 1799550471, 1656573689, 199586595, 53582163, 396381171, 551472638, 1939306454, 878063457}
,{1284088338, 1696296376, 412212447, 1775819728, 463617907, 1032117093, 1446835835, 732556274, 1710461764}
,{2097983531, 1520590171, 1530168582, 2144810923, 1582558553, 1967411941, 948948585, 202066183, 1833957936}
,{739783402, 671048317, 534006165, 7800097, 185436254, 626872491, 1495716799, 1524408682, 2014125227}
,{533644009, 467947796, 149860343, 1995241936, 786508843, 1247540034, 1216459884, 1998977706, 731111740}
,{390827225, 1331639005, 1325705763, 836074675, 806283665, 1731720388, 1578597086, 927810789, 1447917530}
,{1578191690, 367895804, 438901722, 233173001, 1531747770, 889270536, 1845643952, 875687806, 1869502132}
,{838185586, 2009141490, 982334759, 949263284, 1806552469, 212537982, 1122694830, 1630061917, 913908251}
,{1303712982, 1642705585, 1125284460, 734189632, 770643948, 1401154406, 1279125512, 1574985891, 1400514892}
,{283142805, 1938140395, 759461633, 45607138, 1995849163, 228427678, 464763581, 2137797714, 603374869}
,{1548874425, 1081375161, 1781755827, 337231233, 1729271761, 1076140024, 1834603548, 554113450, 461590230}
,{965561883, 1921023010, 629865647, 1520721480, 816437220, 1905198898, 1895781315, 320327108, 1563316191}
,{28935517, 974071289, 37707730, 2029279155, 233041268, 641821311, 994186277, 1359191411, 938440280}
,{1400257877, 386690611, 1408185414, 478546333, 145230459, 1721961454, 161253811, 1603223505, 120973144}
,{9613938, 444958614, 648131738, 780206330, 1325516704, 226845371, 1749851864, 597254907, 1868096763}
,{1558768028, 510958594, 170049698, 1889666137, 943273394, 824383243, 203638110, 744650975, 145774641}
,{749949171, 1413440725, 1478648567, 1753106910, 1239698577, 184590874, 769796667, 87813706, 1853208874}
,{873361886, 1634629031, 1052409966, 293082117, 2117670097, 1973197465, 898661765, 515421411, 970904962}
,{383537470, 2130801820, 1209917876, 1808721534, 1320668354, 922821882, 1101678582, 1637882395, 210538604}
,{1039273898, 1053143454, 114250876, 1433875592, 1805917767, 474649225, 484994559, 1156753314, 756809013}
,{1351134457, 896570522, 2077348590, 1405656529, 352326440, 101550915, 1504934858, 664180964, 549359832}
,{396451192, 2078577712, 673268637, 744730663, 1875155134, 1080826336, 2012966337, 1865553545, 1759082305}
,{1051263218, 1235079520, 597513880, 9430000, 423100072, 652389029, 1335545361, 1796846953, 1190068009}
,{958843346, 1289040609, 1350757214, 1083521370, 687083312, 1597690532, 2080055716, 2073488787, 712981513}
,{1964651803, 1561787912, 398144876, 557420958, 1297921561, 1196282328, 838439520, 263967126, 1849778161}
,{88744084, 1710781196, 215280907, 739504014, 864450253, 2109217496, 1195041701, 428613601, 821809675}
,{107532094, 1892664662, 1462597834, 567911525, 1219636678, 1210783335, 501952755, 2002840796, 89145670}
,{1326033436, 1246307966, 411367957, 667641698, 2068940898, 352699534, 1791161673, 1436599483, 1888253626}
,{1987819942, 2088045840, 808033482, 1723578164, 10099047, 1607057225, 782434365, 1986597048, 173783539}
,{992685984, 983487522, 1607004178, 2080761883, 2009171717, 1065544157, 911941176, 1332085742, 1757690072}
,{1788385200, 752472939, 749461243, 2107730903, 934659263, 126538119, 597798719, 1791127169, 1274132395}
,{1202770716, 314792571, 66214205, 1386639545, 2093858130, 1611809738, 1434816079, 777079735, 1285009311}
,{236235983, 69868183, 1483971075, 220770660, 261389106, 309308136, 946216498, 1822330168, 482567006}
,{1252037449, 2039225569, 2013386770, 1160797436, 1652502349, 419896456, 1436926281, 765247420, 1384665752}
,{559071297, 1264820636, 1645922792, 743623316, 2103325467, 747023135, 847593783, 769088039, 1726770471}
,{676878856, 511842652, 2139518390, 645365874, 2053603049, 1992921970, 368253877, 194354470, 911837161}
,{932328291, 326508576, 881653155, 1596653325, 520587264, 1181309680, 468194767, 36920786, 1151910083}
,{24457320, 446371652, 89173169, 950930200, 1771783645, 437811590, 1009680936, 168045143, 2035090392}
,{164039459, 783183683, 1532654219, 1119463022, 622229906, 618395690, 242473904, 1464093885, 646667958}
,{412713245, 1210139988, 238439355, 1061814555, 1847752582, 1602561264, 107039642, 1239099045, 1950938828}
,{1368313186, 1723698050, 172475083, 1378365823, 1055339874, 1889033529, 477680989, 30061471, 78450718}
,{1407761424, 1351323152, 1986641438, 2132546091, 1956780743, 1288103383, 1857927801, 1589645543, 1722527350}
,{1978818034, 1013793500, 64243355, 354544229, 629073325, 1824445741, 1680545175, 1376966153, 1542999542}
,{536280389, 1101586710, 2080902842, 1808305416, 293175122, 1880256215, 885942255, 961354715, 1567313707}
,{1375226062, 887637307, 833013759, 996299239, 117669533, 621056550, 1689385790, 1478756036, 1318091838}
,{27072304, 1271849690, 1429860353, 1172723656, 1204757870, 1691871113, 86086187, 1894212767, 1293402678}
,{468976026, 1394270241, 2143359724, 945849613, 936572377, 1874373726, 2131574014, 1703037261, 447883518}
,{172520270, 855491355, 1189813652, 1964428641, 1413578986, 1868851084, 965212502, 1841352630, 1842847389}
,{1181644909, 825392281, 2093980163, 750596481, 319354269, 1378184204, 2041112421, 143581316, 1987087726}
,{2085350436, 1595044029, 8912517, 1185191263, 1557174120, 435674647, 608045667, 1723994556, 524279296}
,{271061812, 232983259, 806857674, 1758694132, 1144695020, 1426601813, 1703411410, 88758315, 576758698}
,{1247512532, 2021692240, 999832613, 252416836, 1881593123, 726069395, 208837413, 1575188112, 471283825}
,{942301406, 296686383, 25721265, 686521775, 2120949355, 611125985, 348698082, 779225811, 881852680}
,{1356583800, 445029220, 1113476523, 400142121, 681049849, 2117708537, 50155988, 1241580984, 675993234}
,{42812365, 142013956, 2060362139, 405074473, 577042943, 663900540, 1827996950, 839017364, 712883595}
,{617926067, 1155725249, 1861714877, 360111199, 101719588, 1691070402, 1484588601, 2095363018, 1301077814}
,{199340948, 1557572766, 1595724788, 1446086899, 91043777, 199313246, 22726599, 1330758407, 376897611}
,{2105779642, 309295565, 982619772, 1693347530, 499631027, 1934504702, 520291491, 544877867, 1060543050}
,{329738186, 547447549, 312672196, 917060948, 173820334, 1076284278, 1868707981, 370387337, 1869545866}
,{22370610, 924919242, 1305035442, 1383698402, 594754831, 1990831352, 1896734657, 397718725, 992424189}
,{606965924, 947586090, 1589130488, 1943099727, 1595611000, 1164644746, 1825749921, 1411461437, 779476950}
,{343975177, 1961593522, 748711625, 178738513, 860246367, 1519079663, 539156290, 1063775635, 2098537630}
,{1167172959, 1049708067, 2063374046, 209866104, 1394181480, 533823349, 695419868, 825462609, 2071637208}
,{2019269694, 2014322171, 1285689615, 1043783283, 2138205362, 663332684, 1483019905, 1061597228, 478156144}
,{696263274, 283716399, 151531196, 942468966, 1001006519, 1543262806, 255634308, 1711990417, 79687326}
,{181774090, 1215071315, 467692519, 1054104184, 2000343845, 77708560, 440494523, 1222053648, 284134880}
,{1969335059, 906078307, 427856956, 475264194, 1774737732, 109436854, 1892792610, 886528604, 1893949450}
,{1335450325, 372635843, 1978600630, 1907224879, 1556408677, 442099141, 1931240113, 479671563, 1710646177}
,{498702562, 1828991973, 284107074, 698720885, 1612941585, 1869194441, 1884746849, 1005427129, 79926084}
,{1826563951, 466642607, 1346552446, 2036510613, 828939995, 1008461230, 2067332839, 1205569937, 84867970}
,{2026585161, 1176102193, 624589227, 2106443992, 1556761331, 574171569, 1389996819, 1512570923, 1349484055}
,{1972038738, 2128004701, 212679751, 568902647, 1753254486, 1223042778, 1717350061, 75204308, 438442790}
,{1530442143, 725590996, 1155006135, 8364550, 2055643168, 512058793, 224166281, 37280534, 1398349887}
,{909771006, 144337620, 1257163396, 1744714766, 1942383312, 470162862, 1307354759, 1179766418, 1314716701}
,{550010533, 1898560929, 468179505, 136472692, 1350791268, 954437966, 778371311, 1045158739, 877611527}
,{433837868, 1540684921, 1118499370, 1300284149, 996411380, 100351479, 976002170, 2032425213, 1680926926}
,{914651626, 1989015580, 1624695372, 863230255, 2092264129, 165648958, 1351969872, 1678530988, 216288095}
,{1996222421, 498854076, 1675863870, 1891136863, 213147483, 2055034058, 741853585, 20875373, 1601705473}
,{1240883427, 1387902716, 127081727, 438101018, 1087626032, 1705212793, 926121620, 113958451, 81372933}
,{781859265, 949103203, 424632019, 658555113, 192884729, 1212323372, 1288809027, 959091230, 930944622}
,{2026970611, 2016988139, 2011477135, 855045868, 1095487378, 341180784, 174076943, 1116241485, 1241673770}
,{591055610, 1060335675, 1258734546, 1028330541, 993560331, 1927208035, 47340822, 1226509618, 349533600}
,{474397193, 2077815902, 580698020, 851149327, 553539540, 1160157608, 1033636316, 59879887, 1284200856}
,{656792993, 1614920546, 891958362, 684540967, 1223128806, 633423722, 885008424, 804284748, 1800622603}
,{1374046595, 1159092038, 183426431, 816904277, 1149755410, 141304818, 2066705027, 845176794, 209698438}
,{208935577, 1964287894, 1290689124, 1668862826, 1236660903, 1569037895, 204654949, 416117588, 1950013206}
,{1951400851, 191927669, 620759198, 2016849123, 1199643056, 690053377, 57117969, 815741828, 1773126875}
,{567580201, 1947398309, 750034191, 1466814569, 172879173, 1856119421, 207766576, 964599538, 1768004617}
,{1703976435, 403956971, 1674973120, 1495867951, 567581048, 1418177954, 534365742, 1523588306, 678570235}
,{240430468, 909861065, 449399312, 838193191, 1902401191, 1684703860, 2007236608, 994653837, 1232149801}
,{382429979, 971253008, 2015881252, 841613755, 670171029, 730028333, 1008752059, 381176352, 1825249494}
,{5638970, 236071928, 1175674997, 869835242, 1256331244, 1307840112, 2006421146, 811832935, 435318805}
,{600110568, 1732388788, 7845031, 1917843508, 1325839414, 233859901, 33368248, 617674433, 358034086}
,{860899387, 446173476, 448040869, 1357735675, 892521554, 896477967, 885892857, 725550628, 1497572293}
,{599091214, 1896833313, 1659528594, 1844763470, 1062881770, 1042906294, 650266297, 1295047719, 502240886}
,{107988928, 513683492, 2035721322, 1121627813, 71157390, 1334880826, 65274480, 1098781745, 223454691}
,{1168435403, 2046536723, 494623867, 297125515, 651292397, 1959706879, 1848835924, 79529671, 1804573018}
,{1272983110, 1535603581, 635133898, 1798850220, 1451389407, 16317669, 1595790355, 703452684, 1634677697}
,{149233730, 1549813033, 118155068, 237659065, 993994568, 423839404, 399308006, 742146357, 412120796}
,{1726725868, 676415651, 1436326095, 2051165787, 744152047, 931055056, 680960096, 1383333024, 689482218}
,{320786719, 964875123, 1867230531, 1858909197, 97983634, 745225589, 425285569, 528943242, 127601864}
,{639205105, 1112842691, 2126553470, 1267867502, 207233447, 1425991816, 1197959605, 1385452552, 1010815242}
,{646490098, 378658310, 1277695546, 1048970500, 33685061, 2085122058, 333154461, 1039740621, 1924633673}
,{547927038, 868128903, 663679640, 1263476106, 768368143, 461606075, 2112730249, 1490388160, 246732287}
,{982469487, 1505254108, 2115476551, 950566098, 2024339439, 1913009064, 1875420108, 1153162519, 1513358375}
,{1351072732, 891651361, 1847742183, 1188223826, 1336315121, 1707266093, 498579774, 369475811, 240630946}
,{560158578, 1555620771, 413374959, 57754366, 1326328035, 1144816904, 442185772, 1191479534, 265065104}
,{320431665, 1604740872, 448628547, 1371355354, 1580880504, 2029460701, 1993175131, 1854860064, 1361176211}
,{89955220, 1090682105, 468820915, 1285301718, 489822174, 1059197778, 2030836975, 37637865, 1013519833}
,{888474915, 787136394, 389533682, 778617186, 589750374, 2086782909, 625527305, 1096553541, 313330817}
,{257128385, 261988351, 387317728, 1058215602, 902906418, 708864328, 998208672, 1997332755, 1814452689}
,{2128119878, 1335660855, 430886682, 1243796524, 1464148395, 376070533, 1695126366, 1294747390, 2084728632}
,{814846573, 1234664956, 371824798, 119779890, 1742632266, 197849379, 195203333, 1447323094, 591824089}
,{1614146356, 518076024, 1975373331, 335230768, 929980564, 1671111717, 1068361683, 283426726, 1475026304}
,{1689759980, 352732307, 2042418416, 22966012, 1153147927, 1908799961, 1481712612, 792856786, 1519893146}
,{1409537474, 80163292, 1758867761, 2081118454, 1667104871, 1090063729, 196229450, 938696323, 1660683705}
,{334429515, 223267306, 1645985444, 1695618668, 612152182, 1716829711, 78727404, 2097560806, 522250994}
,{224487197, 1738359031, 158296853, 1266353104, 1329775676, 1337665965, 226984677, 864785920, 1027100604}
,{956259928, 1640031682, 580272501, 1964931942, 1914966694, 16850519, 467000637, 1479359172, 538197525}
,{1860278891, 1967541887, 1280421864, 911263141, 1939278279, 350076634, 513649948, 1590479118, 1167862223}
,{2095115804, 1150870013, 800823993, 819668689, 1865239155, 847076211, 1685463325, 766627437, 1274797005}
,{1112133265, 1999045760, 1714571576, 385917284, 806116174, 2146670029, 1929604707, 462174913, 2040219325}
,{810986818, 92710449, 1417597035, 779109582, 223079664, 1741600345, 1344907826, 952636937, 1715017836}
,{2042480124, 1834870321, 651277110, 1906639661, 480414167, 1549499007, 860571734, 961194460, 143996106}
,{1917657618, 730596839, 293272443, 193642663, 718609911, 1992151955, 1206631994, 1274560018, 57765540}
,{1798026075, 802657864, 1223836436, 1492411252, 125048294, 1783961473, 1995314291, 1975167256, 1151726672}
,{358328779, 647096695, 266970721, 2049334699, 1211082137, 414307440, 121963117, 1684525061, 728872548}
,{1067232274, 1301740398, 773861151, 2094757157, 1497082220, 283964008, 1533467207, 162179264, 1869816207}
,{1070834770, 778777015, 685967291, 986206148, 1543454294, 1477726669, 1135130598, 374162074, 768252474}
,{106398292, 1771627323, 2110428034, 1756025633, 1540235722, 875394462, 2018788337, 727327168, 2144664672}
,{93216548, 1256027787, 1476298383, 142821831, 1697849176, 2082395454, 103214880, 106696623, 1982627905}
,{1181027311, 1308050654, 1614194062, 1792717619, 558910731, 2075807101, 995035759, 1337080921, 198306591}
,{1173854424, 61555314, 1158174443, 1006179413, 703728358, 80202995, 593232483, 1120670591, 1492609657}
,{765608620, 423640093, 106108863, 337555623, 971541500, 155677076, 1769608804, 147689271, 2100966261}
,{713128857, 1353942237, 312140353, 396517172, 796750658, 782224590, 1112103570, 1081022746, 1062637791}
,{844268523, 765060988, 1685438868, 1687945123, 888179633, 791689552, 549354066, 2139916294, 1642447483}
,{1029225728, 719684707, 1046914110, 1519526926, 1966185603, 1218449161, 727831086, 1909031140, 1298024704}
,{1617530560, 1172242034, 1624600017, 858581666, 147156156, 606106338, 1197714434, 1942620167, 218858374}
,{1921287956, 1478627779, 441763051, 434085192, 438011450, 376680265, 846795870, 1211448461, 121378671}
,{1676868090, 307766929, 1047594428, 848503335, 1779377811, 794604921, 876856888, 1400596883, 1611291449}
,{1372264271, 556373014, 984181270, 1312389175, 1839585931, 1257196525, 1081033398, 719279121, 1695212421}
,{279142459, 1559972279, 1323839805, 188997188, 1202453217, 1354898236, 420128693, 1437816516, 269720707}
,{779465324, 716247735, 1858343928, 1487792903, 830128981, 872931601, 1660192050, 1980467509, 1103332994}
,{159413008, 954437726, 421019494, 624473512, 291046085, 1757636632, 1094336861, 348829793, 1564820028}
,{800535499, 152520527, 975091273, 134052032, 2009114448, 1912442567, 1057355651, 945093560, 1678331702}
,{1582927517, 699328408, 1378228817, 475685979, 1465378021, 1970664877, 366373563, 669577779, 1123651616}
,{1026272972, 1802623481, 1489725074, 583103450, 669163628, 584434482, 1499921370, 1131872794, 1776246703}
,{370583497, 1029652728, 399576315, 1458497119, 1787454212, 577825957, 2022698330, 1824294527, 1900570404}
,{1938524356, 929860676, 356133569, 2100482268, 1781439055, 1969364069, 633223786, 375615390, 576231845}
,{1088336740, 1136563399, 896595337, 847576010, 507219524, 755645540, 632119398, 411585856, 2081151392}
,{61937080, 311891788, 1564106624, 1101631185, 1847738402, 290213810, 388789967, 1717173973, 495108235}
,{827690718, 871075368, 1488528461, 2123454305, 229458949, 366635252, 501212664, 230042955, 1759556650}
,{1270433679, 568296364, 237201804, 658485673, 194782623, 16270394, 1734745719, 2091434136, 1180767593}
,{197143922, 706242083, 331429217, 174590238, 276032628, 851990075, 1643845426, 243489433, 1586675697}
,{1292245194, 1686278886, 810038937, 543010209, 73702457, 589747974, 2106865699, 1845716679, 1467811084}
,{2044645147, 1567449295, 782276006, 1678964530, 242250023, 1603442770, 1294812860, 1225780029, 779594654}
,{1105082783, 2033988813, 1394157411, 1775411816, 848429025, 1969332386, 1349136170, 229472333, 16494432}
,{561411245, 79344421, 503289950, 710521079, 680979672, 419721397, 34264914, 54283265, 1198851441}
,{926528978, 1081022522, 1177205209, 1321610334, 1364585233, 541989015, 450638142, 1378635349, 1848430502}
,{1216030079, 1081540767, 1213112952, 1281660405, 748553241, 548556057, 698995785, 1573882792, 1486184452}
,{768179062, 803429057, 88395736, 1593946551, 2096157017, 749314750, 899877397, 956866915, 1224382685}
,{370094193, 1476057517, 332802090, 303682957, 1225542588, 594905273, 1520449615, 1382138327, 1493413450}
,{1567015798, 1286863034, 935299387, 149706418, 1617173205, 523659140, 752018688, 745493336, 926753808}
,{674221735, 27315207, 1412953567, 766416766, 1551752809, 1401950335, 385400038, 416778388, 117390185}
,{1844080660, 112143243, 8230936, 1034352967, 1442392337, 1614198659, 1483370870, 1588914986, 1530517186}
,{1344821152, 1900718879, 603842137, 102519437, 1899842950, 571038017, 81856284, 503366467, 327800316}
} /* End of byte 11 */
,/* Byte 12 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{696788222, 1812096040, 882428843, 1609134457, 1078616037, 1782968527, 1010009135, 718760348, 1686043374}
,{1202180744, 42874873, 2062234348, 161658249, 1423167426, 51608342, 111436146, 766900692, 295758725}
,{1146186515, 58536875, 446231569, 913842564, 417848192, 1267984330, 1076178877, 215057410, 1416319922}
,{783395902, 1549319768, 1597739531, 285561366, 321855346, 1489879567, 1827900959, 1685905495, 1342401158}
,{1644664828, 1464835735, 497174019, 2048306693, 1557558248, 1748196504, 314801972, 1079132544, 1609358587}
,{1395101234, 1195914052, 1021136119, 317029561, 184672441, 63383526, 1643196979, 1782020083, 2030571606}
,{419946285, 942800688, 1228070151, 1387434373, 2104593841, 1332690400, 761628511, 154496753, 1298900701}
,{2132263651, 1018867167, 1178466566, 1734643453, 647703741, 1453690895, 132803841, 1795536291, 1200382045}
,{1395839771, 1427915431, 1505127076, 1938604544, 1485266408, 2120487517, 440833898, 1569564315, 399047986}
,{398926468, 1783326693, 1500398855, 104821855, 987238773, 355212241, 1483448753, 981479162, 858469431}
,{1350872487, 117981453, 681553802, 616542729, 1803328278, 396795359, 435301244, 893684023, 1812661417}
,{2090571687, 147935214, 1391667443, 188701832, 1127409435, 167854400, 1332918403, 44302938, 897155042}
,{574690785, 2117237780, 1100582759, 207233149, 1933138402, 305483270, 2067405438, 1416376528, 148916429}
,{2147169391, 1075960581, 738666450, 2031938273, 652590330, 1505567300, 211717895, 1420966098, 2087987478}
,{1998335582, 902731590, 868146862, 1879195961, 1570165653, 418062501, 829199169, 407860246, 2071659661}
,{522007123, 279919223, 83499994, 762310327, 1561280008, 419339277, 1762092347, 897197282, 1462009324}
,{881585730, 31318601, 1893457268, 121783159, 347579961, 796505419, 1507306189, 560593634, 1654610742}
,{1270474204, 1403419848, 2069160783, 990287640, 1987433222, 1388250606, 80034518, 747527194, 1081553610}
,{1054055266, 1886420203, 901325693, 2144257583, 811873509, 14262307, 1647953162, 1782222341, 295469872}
,{2135127386, 748167985, 1342423435, 667705086, 1210803764, 645118494, 472386868, 1361363141, 841700847}
,{1037626816, 1344010387, 15086114, 1921074444, 929462577, 551344272, 1110605807, 136498041, 989062445}
,{214283764, 657559181, 141797254, 476941659, 1202755375, 1012056593, 1183441817, 1888098123, 864043579}
,{1779056755, 506495601, 1840527005, 1413621132, 1217834657, 1494438472, 458480081, 1360527286, 1385895201}
,{1258907145, 779470512, 1718159072, 1927446600, 1631923073, 875723923, 1733778246, 1964869308, 1803212274}
,{2030189366, 316269033, 1324350818, 328555118, 803802916, 249947849, 55753065, 845912457, 185611009}
,{2009180327, 368501835, 143500705, 1018775192, 1410557240, 416907997, 1822944006, 1024989884, 1481307510}
,{365378227, 1405712768, 1831417885, 1610168826, 877064681, 2029449665, 1657981414, 385630237, 419971551}
,{1939603071, 443822591, 361014742, 347336555, 909018255, 946077522, 244635974, 1764952517, 2116645974}
,{1226368805, 1558231512, 1947516869, 185668397, 1142679143, 1665873342, 1579209564, 62441779, 1013450338}
,{28079442, 1376073886, 1132410012, 1835086761, 701163152, 1580994446, 1223307050, 333187470, 1658289361}
,{127887597, 865542840, 2070864585, 1666319089, 1853316403, 498659634, 1252088619, 1120713890, 387324718}
,{30077069, 1599850421, 1571211200, 1538362238, 398560681, 408207441, 1535712778, 786114401, 230794823}
,{351427891, 132700222, 542526223, 2112293290, 1313491376, 1190816466, 769985554, 1198553252, 35004279}
,{1948300955, 1359108464, 1738305054, 834245276, 439352440, 140332218, 1474440413, 1714055502, 54181379}
,{48522004, 764918241, 1944784217, 1441792898, 1658103647, 1177432269, 351566917, 996621930, 235544148}
,{1681474937, 872028403, 589307132, 516653903, 1747866657, 2108873275, 1461814221, 112393108, 420038881}
,{1336550160, 1276029054, 194713084, 692560261, 60979972, 808236498, 1844565592, 1335452492, 882061788}
,{1659330136, 592482791, 278939816, 432317971, 1123046790, 1729704161, 1928401189, 723745196, 440239242}
,{1002421666, 982984946, 1046480286, 710863573, 723785617, 1276382457, 277623879, 506321033, 1942437626}
,{1037444329, 786435165, 1473943156, 2045064968, 1684881956, 1389697291, 21181463, 212290356, 1181083602}
,{1452982732, 770828505, 1827540207, 1152149652, 486079777, 1550774737, 962646502, 1029971741, 232963850}
,{1564004223, 1143247447, 1686414711, 1165190918, 319861074, 917968516, 935629001, 945094950, 1062819665}
,{1265122571, 945025972, 1806109020, 1352435549, 228708949, 1683349078, 1002188478, 786921217, 1778647623}
,{1542028138, 300215744, 1085348765, 1985513885, 1025744370, 1693241178, 277965988, 599735601, 923211092}
,{415459310, 1729088186, 2142495310, 1931560600, 1166869954, 509935604, 246030931, 879082133, 107571931}
,{931816679, 1482769272, 2081017449, 2708358, 724310367, 1049259252, 854783252, 992024408, 1236095283}
,{51393308, 126475116, 1091805853, 1030810527, 2049416357, 967480917, 431064985, 1416808694, 909288220}
,{1953142785, 1223314758, 1198217253, 1924082724, 1500891632, 1076585695, 284541348, 536003064, 1742787289}
,{49606365, 273298902, 1032584285, 1024310220, 559323094, 792807586, 1399346724, 964035596, 1939897553}
,{2054617134, 896076399, 476983398, 190302648, 376867992, 1680198955, 1499526499, 1705577838, 1712651031}
,{1838162657, 1648090226, 185078703, 614717173, 1606849275, 682610749, 912532582, 510079134, 784076484}
,{546857375, 2077720603, 944407365, 732612671, 995884328, 1349449610, 1545841825, 728438944, 1170427427}
,{31300942, 745346390, 1387708508, 1727091326, 1592783484, 949139425, 1592380621, 1277132291, 179419541}
,{1552595670, 1278629417, 32396315, 810091727, 495786371, 1808962756, 1117246268, 1278048992, 1336959412}
,{2104184015, 816620296, 1292947428, 922786918, 463393421, 1690768626, 1345423503, 774011239, 85115305}
,{1450852587, 701336827, 2040315511, 1847273555, 56231133, 945157698, 1494761952, 465113413, 1289305809}
,{2037115888, 1906993108, 2010734827, 1499184005, 53512755, 2052996006, 1936431319, 714058861, 2058449983}
,{1048861364, 879958584, 1289388345, 876951673, 846522117, 942353836, 530067773, 2087292323, 438092351}
,{1014365917, 607803662, 1894664988, 1985546610, 1181586190, 2078136691, 200949505, 304601548, 1933969107}
,{1786591549, 749684577, 439846277, 1146768873, 676843154, 1090745176, 1776072383, 847266086, 1024545118}
,{415302310, 2009236305, 1298126179, 945621240, 1537145333, 790138673, 269958367, 1602755088, 1835174511}
,{762461665, 1122392429, 737913527, 1759694907, 315417675, 1392401758, 35824547, 351291945, 729336406}
,{291166125, 1084528304, 1678341182, 1865772381, 1493781319, 1519491072, 287525848, 476648146, 1971199562}
,{587493162, 435182392, 737577629, 780611083, 1550649134, 444308150, 1655811600, 201077023, 1802394723}
,{1389300081, 685661134, 2093686594, 624189349, 2145957161, 306780288, 697823305, 916822104, 107082507}
,{1500724170, 1071042272, 438693193, 324778690, 1420572696, 1619530636, 1589844388, 627326613, 449389105}
,{120301961, 979833722, 983911173, 2016199557, 1279754511, 1045102218, 403377032, 1016024137, 602023516}
,{1804585813, 2146457583, 1369617381, 1529147241, 1042512084, 763233272, 243492277, 1121802397, 327758698}
,{1434626505, 388673847, 2018368120, 2133513954, 1932461723, 423425482, 2084917566, 2048037900, 1215648458}
,{1414594852, 403253272, 1315959015, 758448768, 996107119, 1196635262, 1111885038, 154540640, 942017895}
,{1666701570, 1409601197, 456044066, 1350163658, 1488657786, 1899997471, 972160957, 687784776, 818436385}
,{1892325122, 746885764, 1025488056, 416686884, 1543381686, 299320336, 491229209, 1211124302, 1334236545}
,{1046689848, 1052727161, 1905338108, 302710278, 1325250656, 1545379173, 612836121, 195591818, 1042045155}
,{1544894979, 1421822564, 1538791798, 222963445, 733724705, 773735139, 1160610317, 1642110934, 1662224431}
,{247204486, 1106757602, 1785062233, 479203957, 295937421, 1664481126, 1846738933, 1228709701, 2120746855}
,{1820458569, 110287386, 225358918, 492358696, 195124016, 439494528, 930529292, 1455163632, 974306933}
,{1901830698, 1221666851, 1720971757, 281460102, 1953786761, 341277742, 1816272912, 934254771, 660429950}
,{1441123476, 18547493, 1356114318, 1664337095, 1924168244, 1232141194, 280619806, 62881610, 828417857}
,{925085040, 482021842, 269799975, 151514061, 1442498876, 158242078, 2147150530, 473753199, 1169311074}
,{853610942, 68340728, 383862005, 1986891527, 527064322, 2131808910, 711470710, 664134955, 1905229823}
,{638050382, 1007591661, 593817482, 1538963983, 2027026200, 999022392, 1532649833, 250917265, 328592422}
,{2021784628, 1618178822, 972471659, 218022355, 1171553244, 1769547661, 460504160, 56845899, 1044282997}
,{836882401, 144379572, 1828573148, 759581331, 293387515, 2106328561, 1198732334, 1843263567, 399864543}
,{731505939, 554733719, 1433139095, 26827131, 1299504536, 191332458, 1151424370, 697570425, 1592225799}
,{448792000, 1647205168, 1835685795, 1783293045, 1449645019, 1490127968, 25650818, 1559566894, 259551512}
,{449890930, 706985545, 2016134869, 761481833, 409013262, 1623265543, 1770114476, 1859928684, 187534952}
,{555128875, 1812492930, 331660447, 519087781, 475596323, 1081326912, 1241686216, 1802484523, 166849287}
,{75741010, 517573344, 963073667, 949879509, 2003888055, 1564805814, 1918630185, 815229856, 1289060876}
,{1786005116, 121217745, 454428508, 228074382, 1791720059, 1214102872, 1317982691, 947896446, 2104201397}
,{2102458707, 1467845711, 2125122627, 1977273521, 255816182, 1483604440, 1605861073, 663602869, 989922819}
,{205920560, 1551603012, 485867102, 1067071525, 1957876812, 1479530816, 1686018234, 1980203696, 835308789}
,{2044908115, 1006954080, 256571765, 524684772, 1632046441, 1901848975, 1201537359, 2144256161, 988930967}
,{1840636971, 395990234, 116702147, 1231394391, 209194094, 932883300, 1937676639, 445833385, 590023321}
,{1069347642, 399186329, 363828558, 1149069710, 2034857074, 239663019, 172572319, 981295216, 1261907019}
,{958606121, 1396423171, 1528354469, 12270032, 73550778, 1168285211, 153087004, 388186310, 1011427393}
,{395509781, 646543780, 713701261, 1092693123, 1600818053, 615489539, 390905740, 955876114, 622016319}
,{1944295328, 629222673, 1668045255, 113291825, 577523319, 807712767, 2136965063, 1834960247, 686906509}
,{1972699124, 218060274, 2035282067, 415271547, 1687466260, 2104595957, 976550935, 1684091455, 561163339}
,{340538948, 2026629881, 183755191, 1023474272, 797374276, 618951061, 74496502, 1212148861, 480496357}
,{2105203046, 1752112332, 649657783, 1310173700, 487338346, 1659536360, 68820057, 963964133, 1545815270}
,{968299374, 1338555242, 425783176, 436699059, 1155789547, 355405367, 1053010574, 1571383911, 1786921184}
,{671888013, 95571442, 788652228, 706822491, 461065524, 1256275008, 1207949434, 1787278742, 50266329}
,{253642787, 730384959, 325337830, 127775180, 253606110, 1393229260, 1985191236, 1150945165, 387646214}
,{573100363, 73794066, 839188301, 1836971337, 146654513, 195331486, 1415067375, 804826844, 294461847}
,{1281143408, 916743304, 213924520, 27997364, 315504262, 1327336570, 823098544, 169409603, 578333448}
,{482152762, 1671922619, 324240683, 1640564451, 989228669, 1717917517, 1548330652, 239655264, 1334846056}
,{1523922440, 335809492, 1025971443, 894102921, 828255639, 1949920285, 880370255, 49102420, 811069792}
,{1392298759, 2050121255, 830746957, 630925269, 1393934553, 1455442507, 1072961356, 1973375712, 1991743242}
,{546028185, 1963482915, 1564828127, 1792921819, 1231602611, 161760315, 971341105, 1989823344, 964661170}
,{1182180557, 961156366, 2107103538, 620948757, 603423133, 225270624, 310271902, 727101956, 2066924366}
,{553119176, 1069448143, 806964722, 1366047384, 285566068, 446359702, 865487191, 96759226, 895945433}
,{627049804, 449241608, 1862282327, 1005046689, 1312018061, 659270906, 128298957, 638019507, 1840330510}
,{728114793, 1732350219, 299746032, 283623426, 1008315513, 725890765, 582555176, 1797716032, 1667605090}
,{1550224238, 1436927234, 662334548, 203451952, 1790341883, 679600386, 1068719507, 581019401, 1694359275}
,{2108463850, 1105836020, 913243453, 1836682248, 955701007, 1441615496, 2126664947, 1843968793, 1706176300}
,{2053051036, 1012355475, 1646918120, 1745995758, 1796301755, 2011774530, 2016538858, 825458793, 755771901}
,{929777915, 366920783, 723042476, 535694139, 341131414, 1652111248, 1003796998, 2117100472, 888636437}
,{13354760, 27598147, 1781244068, 1015080648, 2068444336, 22923981, 1880594857, 34749901, 16172406}
,{1229428993, 723828729, 1298165108, 1743379978, 1717119021, 1689392952, 1995367763, 1367123808, 2097619147}
,{1691212527, 1539433081, 1018411804, 1745496835, 1735214882, 1699127973, 1819581384, 1538140080, 1196865854}
,{1001857750, 151587285, 1192588884, 902674961, 1182850483, 874678896, 456954541, 850115054, 241065120}
,{287013602, 1477604294, 1560168054, 408500365, 684022791, 1591897898, 788255425, 669481878, 234955769}
,{2047790000, 1430362833, 418729393, 289858902, 53780468, 713636333, 1564821047, 493790812, 1169443872}
,{905708307, 1827109157, 2026175826, 416559650, 733216574, 114246879, 1878815236, 1476961235, 1826892877}
,{1043177264, 1741638820, 824182052, 2033778361, 1804576145, 1806023507, 943693101, 1810859958, 956026795}
,{1132337194, 1086281928, 1138322978, 129085198, 626141548, 45718816, 692824663, 208719113, 1948954229}
,{558899408, 61835412, 1662035134, 809229155, 588245303, 725214078, 1193461842, 606265875, 1986664982}
,{1845660459, 1258918258, 1108130310, 3685475, 961772168, 505562915, 394312378, 1798411425, 869199081}
,{8771549, 1235129498, 1308240580, 1005606907, 1371747760, 50943450, 216668549, 1896140556, 278601836}
,{1110412500, 522246569, 1969562154, 1130297090, 930540277, 133602194, 1278556292, 1971292576, 1958574793}
,{1651977991, 1146069666, 420222023, 1982647481, 1021390414, 1748993375, 1418687077, 1163984457, 291873307}
,{729579956, 631524691, 755842124, 1513530044, 1262140470, 2094158624, 452383335, 818324965, 1512597644}
,{155997300, 4047260, 1476717626, 206028942, 1119876262, 2096931852, 2031281666, 139325453, 319493077}
,{324522066, 74496233, 1161095448, 1374777760, 1690989543, 605158604, 1378786666, 1536447521, 1260607093}
,{1846555793, 410977853, 734181595, 1066811449, 611025804, 1160547696, 2045976819, 243345380, 525854921}
,{2064823991, 1101176654, 465628525, 1857472460, 38379719, 1559604263, 241889580, 1772811107, 169515108}
,{1278198678, 967648382, 1338594563, 601596619, 1880622149, 305530480, 1231658895, 1781128741, 1364601272}
,{1622863454, 1884557007, 1889705453, 1711805492, 151994631, 672309704, 2083893786, 1626687761, 1837511744}
,{975323767, 726579709, 1865176392, 336502747, 1619504058, 1136835667, 71419538, 96757544, 2015135647}
,{1798693059, 1387560779, 469871958, 596973526, 584989782, 650644026, 886391643, 608858998, 157140768}
,{1387517840, 524792, 917152549, 1322161764, 1740339215, 1662377195, 1945933266, 1285330215, 753602728}
,{1073040623, 1872186690, 1498627978, 1796943597, 587348573, 1818745019, 754693039, 1425426851, 1269565181}
,{221610131, 276969024, 914982218, 1982617779, 842130865, 1380521484, 861296428, 392673841, 812069203}
,{147052064, 130382886, 272096342, 241929617, 1173659237, 508411393, 1490016725, 257294675, 546735189}
,{1598627007, 1268653679, 1019793701, 1210204436, 717214709, 1460753503, 830652522, 1134224418, 105487798}
,{1175257192, 1350496773, 1602943181, 275343708, 120535901, 80163297, 1252763480, 1984360137, 1251370953}
,{6597741, 1249166044, 378512865, 1404637827, 1647341250, 1354231017, 1729893109, 1765542880, 1029401242}
,{1013526118, 1141594131, 150475186, 630143300, 1520341387, 572111625, 487107029, 1426023481, 1128607351}
,{501598278, 144746945, 1251041669, 409645552, 1228371958, 1901922830, 1659110502, 43705241, 1320449306}
,{1435765802, 392422697, 261168661, 1835173071, 2091789298, 104653031, 1405656106, 1987438528, 107695625}
,{2129917877, 476344252, 1906443348, 184387879, 168038009, 1556234095, 824890503, 217798750, 1165550270}
,{2061439204, 657050468, 184072254, 1234299438, 1806304528, 365043476, 1774293955, 76425642, 1994303918}
,{1838262640, 2006866746, 761911230, 1957642998, 314142049, 1363474822, 499443407, 386973435, 1789314082}
,{2134484684, 69066333, 684517797, 1123470945, 1328198284, 1898977070, 1093617646, 1384949863, 1165588379}
,{1847910306, 350190757, 1288825907, 111403543, 1336869928, 347667244, 1596639101, 1807437687, 1455886014}
,{585534035, 1197353841, 12595098, 2038436052, 1479739686, 455546017, 1678515092, 591962311, 306185236}
,{2140676101, 982955376, 1251591459, 1956120095, 1422954564, 341092923, 1454331483, 1769226623, 1125847793}
,{65293872, 478535934, 1253610918, 821602263, 719404596, 1744173267, 1394769551, 1514548926, 833557566}
,{1326606507, 171294718, 1191756737, 308502311, 19912814, 458601717, 709123589, 1026676696, 1378562888}
,{222816397, 1344741635, 191193932, 1471751179, 1580572134, 944633349, 826939901, 1289696396, 699681666}
,{1095103315, 247200989, 650343019, 238798154, 1744595689, 1474527564, 563755101, 148858051, 726127730}
,{1124712629, 1620973619, 1670348416, 1202065686, 1402995443, 46208193, 902556272, 27177532, 534489848}
,{1835869542, 1115078988, 1424389501, 623854089, 1460256078, 687273013, 432224670, 1546317278, 704296666}
,{871365504, 1841720215, 756160302, 338844457, 1119787051, 1966239018, 2000711928, 497588741, 1053005174}
,{918206403, 744425403, 1560489118, 318639901, 1476822892, 745175709, 118561614, 1780381889, 1820056872}
,{1483520887, 425718214, 1379410810, 83188852, 1548857702, 658105180, 1149073997, 1506374053, 270007507}
,{1874646608, 1555830941, 1649965936, 1845479994, 1980014333, 1758698087, 1786040882, 1435114050, 403982592}
,{1208650843, 947245001, 1011165527, 2101693009, 679989620, 846339753, 2110810984, 1792405894, 1555886110}
,{1361675884, 352609680, 2063732929, 1047114553, 1245371311, 305519850, 883059158, 523295483, 1384340439}
,{157719104, 493633815, 1223946279, 1370764522, 265302211, 867928858, 705422814, 479621443, 419557253}
,{147638000, 1229817733, 886843234, 829375599, 1902133389, 1178144433, 1870357053, 1570615001, 1459792321}
,{339217759, 591460430, 239168744, 702608318, 1073604734, 365360032, 1399782921, 320058478, 746835902}
,{2052469638, 1528591651, 103934190, 295477979, 410454757, 595478913, 1291380941, 2098352479, 1716140854}
,{948747824, 1729495846, 1729509126, 1339145436, 626921507, 1826930837, 767960786, 1714604255, 1637422753}
,{1298276400, 1304918110, 680860584, 31921917, 505241966, 850521370, 1456919928, 380993401, 479268458}
,{1230974009, 2091543540, 160511127, 1949759252, 1942889836, 479586631, 1173771812, 679945659, 1597534673}
,{506135064, 1223410262, 1013317395, 1169662475, 653920677, 1475759719, 389427311, 66995275, 93601419}
,{318194125, 407558162, 1161285772, 410518563, 1105733729, 833061130, 1943456200, 1877792540, 448506340}
,{591097016, 1175855096, 1648642051, 275070646, 120374848, 539474411, 493458366, 746605312, 1147553177}
,{661237332, 1527139150, 1341806185, 837083850, 1847621303, 436712625, 1725827678, 18295798, 1626469409}
,{972099054, 240476436, 1043137113, 1496481321, 2018796444, 1403480320, 551919001, 1410683853, 773437819}
,{1998055339, 1140953050, 505877128, 405598481, 1003285403, 334259499, 281512121, 414443421, 965604388}
,{1260407927, 1074793653, 108461932, 447796430, 1668659135, 1385270155, 1234230113, 1411680261, 1281945226}
,{995817444, 1198267350, 1581071724, 752105905, 1492189802, 1279227974, 489096839, 473547829, 145332056}
,{90702157, 21869220, 1019967850, 1126792586, 2074769133, 1815484565, 921419164, 1233792608, 354451532}
,{61278289, 631049944, 587298053, 1550312929, 359062690, 1575970184, 465205218, 1172742738, 1881338014}
,{888865347, 402034957, 1791626081, 767171606, 1700589769, 842551362, 1711481469, 1134520733, 551888236}
,{801841760, 1523290685, 1635886633, 1947481072, 1973991604, 1242646069, 19844503, 115174396, 1504183836}
,{1721704680, 1512647346, 2100282548, 1110260145, 1265759801, 910050708, 493274612, 78620572, 379249142}
,{164332003, 946379600, 284263445, 425200547, 1951180690, 1210985946, 504663458, 33819972, 1080350241}
,{1150352555, 1255134892, 681186957, 495525640, 171084746, 315640586, 1214957846, 714535488, 779990784}
,{954760160, 919613551, 203209638, 1027385423, 739870926, 585447495, 1608915750, 1538006734, 1704807122}
,{1785569161, 455467717, 18294489, 1505086501, 1382968614, 2085376340, 1435896417, 680400208, 2003822000}
,{386647044, 212133711, 2000150346, 1914587750, 1229602846, 506303975, 1286073043, 1786584732, 1892788378}
,{1282674225, 1732342480, 244199426, 163717237, 862670482, 1820277413, 1002966702, 27427256, 423574213}
,{755302978, 1686730540, 39211503, 453005932, 2101506579, 1207626133, 1012406727, 850282854, 1159662486}
,{964890346, 2062039726, 24603183, 1993669660, 1325755486, 221089552, 1235712028, 272680262, 1445065759}
,{474626455, 303001875, 822116707, 591310811, 1320745734, 1747495351, 1391609340, 104247542, 1499007459}
,{2022541955, 1829030478, 552467041, 1077636989, 1515635702, 584269882, 562720947, 1632303934, 300084231}
,{1337493503, 1875387439, 900862910, 474980592, 300077288, 1729387260, 295577369, 1337896154, 1335052187}
,{2131795839, 2044863927, 582905584, 901459040, 278299712, 1191361201, 273303346, 1450047963, 92905472}
,{844194869, 2145563853, 1163097478, 844934303, 168125103, 668979643, 1251077150, 1480799124, 79342139}
,{25086849, 1091341245, 1093923717, 645461960, 1811007336, 2048928489, 1343166387, 926973001, 1593940968}
,{590062510, 1283385293, 266595916, 1519456159, 1131257506, 3121591, 1446619365, 1550010293, 1984618274}
,{2033076038, 875977001, 884708989, 795393142, 1752559997, 2097135670, 1224453328, 1225827936, 894023490}
,{945867825, 1888941826, 299111810, 650601352, 38845642, 2094758999, 1606055625, 625352795, 1430584899}
,{2027781802, 137997246, 996080096, 172599027, 1348766502, 1790505627, 1121022403, 102363733, 561319919}
,{715089489, 1843597359, 92465245, 592478453, 819287880, 412200032, 1236685422, 374386920, 688252458}
,{1410186106, 179038791, 678757759, 1450206307, 2014253721, 329718748, 1422349656, 522887198, 1616494837}
,{1054226190, 1290839992, 280661107, 1811857784, 330458628, 1995602649, 548567821, 1727233229, 692294639}
,{713996323, 1775050425, 281559288, 706705728, 848198573, 1847274259, 1675122762, 335161126, 1375137273}
,{1115753396, 1491370958, 1407286590, 1441257675, 1571935353, 671360540, 1172310401, 1138323217, 851399680}
,{1557152208, 391297455, 872811925, 2010817287, 1272198611, 705287641, 733335433, 1002530609, 1887680539}
,{2106991708, 1693183140, 480396029, 1612359890, 844691734, 1814729501, 2100098533, 125706127, 1552313866}
,{470800543, 431955346, 2025966244, 1096486387, 873066888, 589297703, 2141834595, 1957457014, 1691195486}
,{1666812569, 1027442270, 1155964587, 838067873, 967175614, 1410425512, 1950805846, 799530921, 794713974}
,{1049910717, 376133398, 2103659341, 571757418, 731895055, 1834025747, 65885959, 441323150, 1651173776}
,{810003034, 992054280, 1060501291, 1853578111, 655339090, 64308124, 1467490177, 1017834002, 513845554}
,{50606696, 1926166554, 1999980735, 399160995, 1068332047, 1478957481, 1589719799, 1882868530, 1483069885}
,{454779155, 618794267, 1910106057, 1899736819, 1062802157, 2070234298, 970775688, 271475140, 967785858}
,{1181557543, 805411811, 699222251, 578339288, 641222993, 713989218, 234577660, 496237107, 1219159282}
,{1351271487, 1840832311, 2121545276, 2144526868, 1225524613, 2119506567, 2131337254, 754313735, 424643894}
,{348926897, 394945610, 847207322, 1897277336, 2033903429, 2098182296, 1830830436, 826989954, 1100211851}
,{422080834, 1641202589, 746947925, 279659322, 1054397794, 849229203, 771840251, 1190631240, 637676860}
,{40564282, 1142391286, 808418635, 1242464057, 1570058475, 1342942152, 1626879070, 1746798206, 1602924197}
,{1832840882, 487595002, 613929801, 1158955058, 932421459, 60849058, 377884266, 1670940659, 240218046}
,{2095961889, 1205962770, 514462773, 1312767920, 1756222256, 1438743750, 1084644284, 1362639150, 205556730}
,{896879037, 1791419263, 1745660397, 582966730, 2070744500, 1155389053, 589654952, 1665077685, 2024838222}
,{97417345, 1331143602, 377090146, 1517507080, 1137102178, 1833918443, 57341238, 1282689282, 902909294}
,{1246498967, 306674295, 1720575362, 56019314, 1790673416, 622018933, 751249459, 1833191468, 2008049861}
,{1269681750, 1942391216, 1259494508, 1295189125, 1664774633, 424260780, 1111003235, 369467081, 1642835337}
,{639503488, 1601559848, 1475459179, 1068091003, 2139228362, 89243439, 1336849793, 1996057946, 2084853175}
,{26786134, 1015572041, 951585019, 2038451442, 370266984, 361951800, 824901889, 568619775, 1781862798}
,{119245792, 983160623, 2130957913, 1745595182, 1188765594, 82431137, 1779984468, 1981594077, 173085120}
,{154051916, 2023996191, 1158140547, 1049709232, 852286912, 1540581291, 1800053101, 527064829, 1634181863}
,{196355780, 1730671057, 1212414231, 519451105, 1962100068, 1661946534, 2133971497, 1597922181, 1510201487}
,{141716305, 525652719, 1581716406, 1488458455, 2044627377, 1012922924, 1656690112, 49623457, 1705136620}
,{582982868, 1425825983, 749998085, 2073338064, 99785523, 2080775438, 1954461964, 1818773106, 107460030}
,{1338040740, 1365076832, 39837600, 304989506, 1217643191, 896762573, 1121835070, 986089055, 335641110}
,{105856795, 839872147, 1313614731, 1987982521, 429418702, 265951357, 898409209, 492423292, 1545560738}
,{719480449, 313367490, 1198928905, 2054398259, 1035020113, 1027916974, 194912870, 912436931, 1896783193}
,{1307280814, 1784129172, 2059963529, 1969371804, 774958541, 682335793, 1069526725, 2131908362, 243538979}
,{50921912, 904369314, 2060320511, 175142102, 1615173400, 27553186, 547438343, 305661229, 327075964}
,{1092697968, 1356931547, 5820246, 1475102913, 1588793475, 1833480717, 1189463095, 521919918, 1191373139}
,{155905087, 654430674, 1384657821, 595711584, 697754532, 1351483170, 1985380535, 1511441132, 1457240324}
,{1654744217, 1516413902, 1955999751, 200713582, 688074354, 615130879, 1633329761, 1883905995, 1639319991}
,{1624234051, 1703024017, 699219579, 1301476070, 1508611249, 168174458, 425610154, 508267422, 1545466016}
,{909347348, 629879329, 599710544, 341116305, 2046540478, 1170601216, 1907987036, 1391307760, 1559582762}
,{640258876, 2013518187, 679318538, 75620875, 1493165809, 1359930851, 1384458746, 235305408, 122318728}
,{1339470322, 1610477270, 279914798, 373082650, 245358128, 1356411022, 19919305, 902981805, 466185408}
,{542390786, 1970258377, 152863795, 307464004, 1557275013, 277183049, 1022217369, 916055566, 1880900659}
,{1606188939, 941453451, 247939228, 24445220, 1628746418, 271913140, 152769629, 937343491, 334308555}
,{1782413523, 631688668, 419933223, 1642650535, 535179529, 329966482, 519193319, 1353558691, 1242549993}
,{595660046, 1285986882, 560670748, 1711395904, 664321949, 1721750960, 2105541559, 867082176, 2078830105}
,{237907513, 1777881138, 527614332, 53612964, 411334818, 942994232, 1486056539, 1573602640, 2008057541}
} /* End of byte 12 */
,/* Byte 13 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{1200494274, 584421010, 351652233, 1469897113, 258880152, 1667486567, 519651211, 321651548, 1005295569}
,{1452300503, 1157177839, 1656547621, 1864207766, 2086696265, 970995210, 242957474, 1134927556, 1259974891}
,{2036333187, 1555969027, 2138663719, 1149597802, 1589030574, 1594871688, 291968440, 1047408848, 2081430936}
,{162727274, 1933884626, 1696899220, 467782205, 2144814259, 1232684986, 1909704569, 765753247, 480936252}
,{1876780995, 113689996, 1903460192, 1193644695, 427896150, 1067429383, 1851982455, 8579924, 854389906}
,{1407835166, 354794369, 582146892, 1258825201, 2115578683, 506297155, 2015555623, 875503280, 1809152535}
,{1595419120, 580493018, 1569116788, 1645864550, 824667339, 22096326, 58144521, 1732888150, 1291898310}
,{11958790, 1659228481, 1607975730, 1088902327, 1693602900, 581301081, 1511894534, 2062977793, 219941396}
,{1137652916, 637995172, 593005591, 1014191434, 1507428597, 1957803569, 874141983, 1030364287, 1898420373}
,{121167505, 895140703, 612350149, 420552509, 948733743, 1389602988, 984982562, 290556977, 524916254}
,{791382383, 1010140098, 250415111, 1402476991, 730892226, 1055278811, 1062183533, 279093478, 418461144}
,{640301865, 71864221, 1052355564, 794069724, 779163518, 384629480, 917735246, 1117107482, 602924264}
,{1912234702, 1899077200, 478108629, 1356548823, 376308284, 1444543419, 2012627897, 330006512, 1549296193}
,{915856699, 1810768752, 1686747602, 2051177748, 1302770816, 1382844725, 1739622860, 1442309747, 1549581681}
,{1000642938, 222347121, 775485927, 687486704, 1106042705, 1804857375, 1754398581, 180821383, 1364877768}
,{907756212, 829359942, 8841376, 1332931810, 1224856455, 1800135456, 1096780884, 1378555230, 1816103948}
,{1974102172, 294001533, 1275659882, 260776002, 825639729, 891555961, 1070989478, 1818664967, 940726267}
,{1232438541, 1982444825, 1890406577, 1934587829, 178480266, 1658465518, 1095195281, 1658643663, 2067561178}
,{1834843919, 1137834400, 1049171168, 1550235167, 220443556, 388264846, 1557523566, 1649347013, 1039036472}
,{1708546348, 68898157, 72568204, 869262894, 1784898117, 329900394, 1460341619, 250769145, 86516181}
,{1462135747, 11168930, 625895018, 157543601, 1957423934, 251040695, 1159863, 479260196, 1360186136}
,{691830099, 1882869025, 33674931, 1958049368, 794101346, 1533354308, 355994382, 2048199699, 2117340916}
,{1720834166, 265775616, 1892615248, 2054739403, 156670480, 307728752, 925916726, 227429996, 1795056899}
,{118810361, 478656871, 1676579977, 301018929, 2023058228, 544447427, 90159007, 1021082553, 914060058}
,{573120729, 1531355523, 1338313851, 1465338957, 927832885, 149647262, 1500685674, 295853877, 502161481}
,{464392355, 1081702777, 1695494985, 904158953, 1645413246, 382294541, 1669066078, 105237461, 1715954317}
,{987504448, 1820864800, 1445630546, 102778493, 1414140801, 186748055, 843397815, 1418201993, 166150827}
,{1696508696, 1790634655, 1912982232, 292504807, 355623276, 767359882, 1859154638, 1574535966, 448780782}
,{1695269387, 1599618531, 1657026275, 838040402, 1511154470, 1757893522, 1256511907, 1149880511, 142328650}
,{48898484, 1796742323, 1312667075, 726022516, 252810161, 61640936, 861596018, 209715545, 716280632}
,{1374990074, 1635714663, 2124972452, 1384508164, 1646945838, 510223043, 121033287, 2093765790, 1987869635}
,{317677507, 71040132, 618226405, 1930116180, 781950428, 711472281, 476585289, 582195323, 900304612}
,{474500859, 791580949, 236865644, 300374879, 1950066877, 518131466, 699764725, 592328137, 1420629482}
,{1688936591, 1234458625, 1826486521, 1825124938, 1393637579, 1824446824, 1617426862, 1473773901, 985190896}
,{732001600, 2114978186, 1391967087, 223807374, 1937017299, 1858777626, 1385640012, 1027323558, 1362705689}
,{7600868, 693687101, 810654566, 1990596644, 928460628, 1550020256, 1986134394, 651204512, 1989029237}
,{925341374, 2140012711, 676782354, 1759242745, 660563269, 1171898136, 1984604924, 1494127751, 1318734562}
,{209132607, 1460850620, 1378264697, 134621787, 2115550249, 2139273484, 1059015707, 515585566, 431856748}
,{110803827, 796317077, 1116693089, 216954595, 240183008, 1509155014, 2091727387, 236469286, 801663852}
,{472366446, 1625453231, 1818878664, 1173419623, 1459895220, 2104370069, 2048716781, 563742932, 403591735}
,{448315370, 857178361, 2063284167, 864242963, 1511023992, 1774009805, 1979251647, 923124163, 1510053082}
,{938780830, 2092062240, 1500001142, 1672239473, 1594084689, 689747126, 1902834742, 1220995678, 951987837}
,{1024857673, 623966526, 641823724, 288163179, 121007498, 1661130711, 1241543981, 2108480615, 993652018}
,{1089452805, 1002046777, 2139871705, 333278476, 991150785, 1157609207, 338844112, 1503173225, 1001911068}
,{1308190825, 513349361, 433974216, 1722990879, 1089267282, 592353484, 656147226, 2137584444, 770609456}
,{1654538505, 1414230627, 940180136, 760004853, 24702934, 1372013062, 674723929, 1490219119, 1543235707}
,{650358862, 1990393496, 2028633661, 644459009, 1996464630, 1977516259, 1229128788, 1073257392, 761831084}
,{1248224952, 1040376609, 2143393643, 580021059, 1662762940, 1891988064, 1910694550, 985019790, 244264190}
,{221671450, 250171952, 1325273923, 1905395637, 1199253726, 1246328768, 1748052313, 541958051, 43200767}
,{908063864, 1619895175, 2112396371, 137029226, 1604071170, 1731569552, 647353569, 876823118, 1082434714}
,{1001486752, 343177192, 437026514, 269473034, 438762955, 1281147017, 1813986158, 1567313161, 2145061178}
,{1830147107, 1826818466, 17456864, 1369164595, 1700518786, 1937886255, 949809410, 2110473125, 1498724104}
,{647704853, 1301023230, 1928055109, 509894531, 442951309, 322707255, 1278236658, 1995246983, 66737412}
,{324162503, 2071327110, 608882779, 2083303183, 867053541, 132509134, 2065743135, 823422174, 2082094333}
,{1640726872, 1820273650, 2093684357, 1952261733, 1046329030, 387025786, 454986779, 1685642178, 241605903}
,{325248946, 1918784352, 1065476766, 316793311, 1545675190, 301708668, 1818872191, 861304050, 170476949}
,{186896682, 1151731331, 348251395, 337035571, 1288140870, 1301065857, 1718239445, 272522677, 870635386}
,{2104930510, 1028790172, 1804050307, 399568373, 62251819, 1365806317, 1003895549, 1014572572, 995641607}
,{2111293976, 1304422003, 580232915, 961427815, 585773700, 135972382, 747390946, 1344086078, 2048078855}
,{1425605820, 1224611448, 904151474, 1323673809, 1715189198, 725649411, 1831290186, 1326967338, 1036002265}
,{147296806, 1841789989, 1932746956, 1531124273, 1772609220, 286997554, 1188052718, 900557457, 1135291563}
,{614561967, 1218474299, 602544551, 1812188713, 1046756516, 1754996887, 739914917, 1231095945, 1152743847}
,{76377570, 1724942483, 1636035681, 1923642990, 1713059420, 624468510, 343729879, 1695963350, 45730424}
,{3164555, 883386484, 1826015954, 407977612, 1216164586, 345407978, 1490374797, 1711707533, 2012931320}
,{1034586332, 971437532, 921530063, 1103291172, 1326384485, 1897822504, 686788916, 433418322, 1389436437}
,{1235378859, 1784172866, 1395033493, 1972008975, 1020011041, 1244416083, 1281610411, 557077690, 1898775673}
,{564883773, 1434093793, 727012553, 955112533, 1938534465, 484765196, 127848005, 1447236643, 1754124187}
,{81375460, 31013236, 128091603, 837055295, 214793850, 2531825, 664076376, 175131834, 1656736261}
,{1564804695, 721180271, 1246579115, 734282484, 1050836318, 1784111363, 944262685, 940661456, 292108079}
,{2076781938, 1886505374, 750697716, 2065428676, 1973523206, 944883331, 1601104556, 1075545352, 738088874}
,{542862404, 597028730, 472205043, 2008178648, 1447909665, 836523618, 2112157059, 1161549495, 504496430}
,{737937627, 351445703, 1009269148, 639082858, 231416201, 844786810, 1787335588, 872508861, 33412282}
,{805391007, 817860759, 976636865, 1828191616, 365118652, 1287593183, 848101798, 1316607622, 119054349}
,{224545504, 1465146886, 2008849001, 1202086218, 386063323, 1816052689, 192114554, 490601030, 1288094786}
,{824174561, 1677828162, 1653634301, 1391072658, 847053217, 1496741248, 1988768323, 1017126292, 1779549203}
,{1317135571, 384039166, 294045266, 766193355, 1401819286, 1104157722, 122338428, 1582368364, 1277487088}
,{80121714, 1094865988, 1273024180, 334886048, 240153872, 422578294, 480876683, 488701756, 1787095175}
,{2015343785, 1257580839, 1011165184, 1237914717, 1620889781, 1089081426, 1274231779, 684559412, 522300528}
,{695169473, 249612695, 1013033859, 434319635, 311367592, 1402167750, 1596016970, 949455519, 2019167625}
,{563381876, 1239170627, 1167565156, 1028534176, 1972933685, 612490241, 1060567754, 13090368, 679861868}
,{1829854279, 182315647, 2145875145, 293628564, 540457237, 1581976114, 947938411, 1142966126, 784664187}
,{134650171, 1022864782, 545037104, 300201094, 1047240479, 9369581, 649131933, 1468869288, 1392129687}
,{1182936102, 1588466972, 769906060, 1981453063, 1887547697, 1389187701, 391064547, 448716101, 1837871244}
,{426351620, 1075909628, 265138673, 2144153228, 1617646006, 926686561, 48172559, 2019918650, 1984416302}
,{1007668196, 2025847644, 780528298, 528284926, 607395720, 1243667046, 989489926, 826208546, 962467823}
,{1708817194, 533707443, 512948818, 729457272, 871369580, 1438859978, 1942083108, 908721643, 875439552}
,{1819234782, 200649439, 481304264, 264195914, 1498159687, 1926149277, 1632513117, 1518177423, 1288920136}
,{624220070, 1408477337, 864139770, 788973798, 79082705, 116258489, 1789816940, 162537869, 988562092}
,{867717464, 399293359, 283781653, 256021362, 105272681, 525088976, 1817987429, 586093403, 222262537}
,{1703332020, 1362963858, 1451761534, 943351727, 432602273, 766797651, 217014715, 330525665, 853789202}
,{1042743455, 1697449801, 733671017, 1704770469, 1363809729, 795618680, 455204479, 1438074719, 753109395}
,{705444771, 114994483, 1971839240, 738887714, 174258528, 1049204819, 1497243371, 1395968540, 1833153186}
,{1602901149, 535136375, 1233754750, 20950491, 1861575424, 940488614, 260338641, 37206149, 667437266}
,{958994636, 1188616463, 923636105, 1469083805, 478272988, 423471611, 1956537047, 1535915115, 1551315433}
,{1555431675, 1244695555, 996760214, 1340251857, 518946514, 163423557, 617439422, 1338896424, 479829495}
,{1942909900, 133819194, 621503058, 244665776, 1372645489, 1601284376, 1779040444, 51498350, 280879243}
,{224088992, 393718777, 510191561, 1756863715, 105931503, 1603777818, 1881604607, 1849921379, 1498568485}
,{1275265795, 1546768077, 363923722, 1583372023, 1806530027, 563554137, 1992040874, 241912623, 2043983156}
,{1822584124, 1125421266, 851493463, 986058820, 1480995581, 917013552, 1819028908, 1208392161, 1165432507}
,{669114605, 968251757, 1574160834, 1906731497, 57733674, 1270932557, 807211892, 765213876, 1507895735}
,{227115046, 1938638496, 1290747115, 1516386213, 1522490737, 355909677, 844056181, 1328340732, 198578022}
,{1516327729, 945995907, 824319577, 1694338054, 1508709567, 1381648843, 662865029, 281219288, 121792628}
,{1875605087, 677191876, 771831838, 516067486, 1858165119, 772349372, 1789999746, 533812669, 490214679}
,{1189120274, 616745123, 303673130, 311175491, 1933133656, 1703791580, 1854829724, 91481817, 1578992473}
,{1501544567, 325294428, 777860994, 391424392, 957540053, 551373106, 1229125501, 68604649, 1899896067}
,{1760289922, 185135011, 835860730, 1009354337, 1224489565, 1931265550, 1233659611, 1758689479, 501574065}
,{119614813, 789936648, 2016227543, 8160414, 246038497, 2143935834, 621604907, 542217973, 1452126577}
,{1111975228, 1774817775, 1360097125, 1350172274, 875077303, 97136257, 1003976888, 1857773822, 439604830}
,{56665870, 469740501, 942654306, 1031288411, 1849606303, 1523649270, 1483118885, 1049067713, 730813327}
,{1887938580, 1549243545, 275743427, 849889990, 861523536, 888465042, 179529027, 1538674107, 1074606875}
,{750893736, 1485453079, 2073285298, 824761601, 2072883695, 793143542, 2010433423, 443232450, 844010514}
,{127106376, 1408548307, 175071296, 700081482, 1115300727, 934564346, 492218869, 1494172519, 484966163}
,{1714231590, 1180803941, 802424223, 2080996754, 42101827, 1654564708, 1082320034, 1057939648, 530816409}
,{1171014509, 809302302, 173967712, 2101229908, 514735004, 1365814865, 232803421, 2127122893, 1166106362}
,{1448897630, 1367128067, 1165946482, 159422716, 1390723364, 1933755720, 489131980, 2135710170, 1716966628}
,{84099639, 2120610189, 673707980, 1385062115, 1615399153, 1983801133, 1375241954, 259482337, 1202457788}
,{1308704688, 1813758068, 1541697491, 497981314, 1827039119, 67221239, 1410795875, 1614815493, 510234420}
,{741319574, 918589616, 1898765629, 736495973, 1166974280, 14882671, 927189053, 1807844175, 2144841554}
,{1031647804, 2050931653, 367550742, 1235310834, 890087916, 2119351651, 806893670, 2026639873, 1576628331}
,{684549161, 903542497, 547862163, 69720171, 1707395434, 771324473, 1673762968, 342896591, 621013986}
,{1406501108, 695813338, 715702768, 556914216, 1826527015, 1062531409, 1310184416, 1816336588, 2049802668}
,{1472282218, 1872700236, 1599342467, 439667971, 119664141, 1142653095, 662542972, 1496268119, 1636079764}
,{90687164, 1182743333, 1237197461, 904903357, 605936407, 1580499555, 147151705, 1390271172, 1407831685}
,{1504011173, 311973073, 729740456, 549698044, 1728091602, 1181433533, 1712103090, 1024203786, 2013865371}
,{789656121, 1532489500, 1985201651, 334381427, 2039853264, 1363484040, 1507982237, 1631948744, 1898167933}
,{1020659653, 992332641, 1392926910, 201528127, 1002485876, 2030966133, 2077953134, 977483083, 982489344}
,{884748458, 1954692058, 1143105253, 797041199, 1570621621, 1430281733, 2038371017, 1908972443, 1224603813}
,{302551675, 1210275820, 597585105, 749881289, 162375867, 567511822, 613704332, 1823574961, 1778982460}
,{488636826, 115805701, 1834992293, 1879616792, 1248014043, 589027906, 125593973, 1084039625, 1405471086}
,{1857251720, 581788166, 265693550, 1914299648, 2008867304, 2079233711, 719077757, 1340499886, 36453791}
,{2035243529, 135711369, 595565872, 98096583, 1120263474, 389743775, 2069768286, 17922777, 791867955}
,{880247878, 1144191909, 2101315523, 677909956, 1666547319, 621356787, 487976277, 184995712, 742805361}
,{693297400, 1353155473, 321620458, 1722890184, 1334430293, 1110501383, 255067521, 1740990734, 80838442}
,{649369396, 771303660, 544534580, 1410948744, 922831317, 114527642, 719198270, 85487526, 1537155777}
,{1060638312, 1941310016, 1413489040, 1024728074, 1442871675, 711093148, 2023599897, 2141829210, 1766973275}
,{1515412932, 817730869, 1356492600, 787952879, 1768482575, 1575826531, 1986069320, 92828617, 1413870012}
,{1215593759, 450756585, 476932560, 2077312622, 1614685945, 2065408234, 1257278231, 670897600, 1379871785}
,{1494331310, 1177786922, 943086307, 234263971, 1385474020, 1728512787, 1089726108, 711853292, 277331909}
,{1392205185, 1624793300, 1685045080, 626682316, 1313818785, 1674392397, 776885194, 1858011051, 1844630923}
,{1241683810, 1956443584, 1158289587, 1839293468, 1607528848, 448112427, 1753069514, 581474044, 1833509662}
,{806644288, 1199003036, 1051312530, 1936765235, 712705913, 1775670994, 1933066235, 723321141, 566917696}
,{1366890918, 1891202667, 1337256391, 1157838442, 975392537, 1198042481, 1025315705, 1335848857, 185722890}
,{890570477, 1019621018, 68516289, 2004706227, 1914933621, 385798804, 362983437, 1555539477, 1048372257}
,{2020588, 148218098, 234085709, 1616933050, 957195228, 1006504351, 1062925192, 385915774, 177879613}
,{1426044866, 307469715, 1274765643, 528005453, 1999255551, 452599106, 1760601050, 985536909, 473201627}
,{571996431, 1694539803, 1219289987, 253153080, 141564953, 527973411, 68131652, 1517797190, 684077760}
,{4089460, 1267704778, 1927469727, 669078896, 336744600, 1266273467, 88409643, 960258068, 859647735}
,{1454189573, 1765053021, 111440316, 438445857, 1841651344, 504467394, 1438115281, 42479691, 860415811}
,{1385530160, 1554119095, 411682048, 221777406, 1619979578, 1069183539, 2077577274, 997863913, 519107932}
,{1699153120, 1469330044, 787228978, 192118935, 436622732, 874626452, 1769599908, 431752426, 929754477}
,{580915735, 1168639926, 653627232, 1676352531, 17606875, 428448507, 1255289531, 270633554, 1426123684}
,{572406283, 451301028, 1532324219, 1318647501, 889329945, 573081376, 902249008, 1861853807, 989300389}
,{1366928713, 65071747, 445113631, 2081389428, 860189095, 982511068, 960228216, 1315812823, 721777154}
,{855282597, 1008980416, 1693424271, 1974833570, 1053856959, 387508960, 1557796135, 892734011, 1922516061}
,{1627687897, 1249432136, 859726281, 2041354672, 1811546610, 357618825, 1271430604, 2025850690, 28757045}
,{1609750685, 505040316, 1526203397, 2009947720, 354614667, 478498488, 70238738, 1770615797, 1560260238}
,{607490313, 1517498315, 2144639518, 676331277, 1612567047, 553257429, 769883412, 1695362271, 52822611}
,{1633370849, 1864363319, 319651983, 167280003, 596597101, 111567516, 1998590270, 541515232, 1695556958}
,{781466856, 930350097, 1247815388, 262684162, 996365691, 227339968, 2027239858, 1485885494, 1979544321}
,{1597537431, 560420790, 703230222, 1162973169, 1623799131, 1182398695, 1900003414, 2001210527, 1674994723}
,{809432062, 1911558605, 598499478, 1203053715, 761913121, 1843005748, 812440925, 1455570303, 574586062}
,{2003083993, 839052212, 1768621741, 2119027772, 2077319190, 1533837185, 1768649638, 1285252034, 983235884}
,{1733668289, 1733479570, 785512611, 545664515, 579632051, 2093063310, 1126682276, 1869312636, 1643359546}
,{550073486, 1020940328, 326276729, 956344007, 1641686044, 1472659702, 25594198, 732637673, 633643582}
,{1737058099, 832381906, 1839209338, 410862950, 1168589983, 117037389, 617130246, 839112458, 660152258}
,{763143038, 197069718, 978883137, 744390083, 1452490745, 1862704937, 4813862, 957453596, 687885257}
,{1107999863, 1526608667, 1136319420, 38738390, 739734379, 1460931262, 839741843, 66951292, 2004791615}
,{24306653, 233963182, 1869265992, 1797086234, 592452074, 1621320224, 1731300643, 1513253556, 1780800247}
,{1399149137, 1209802428, 491743351, 2137492327, 1296843790, 397977683, 674573709, 720275523, 1366869904}
,{1960404809, 1212262556, 92497241, 2095493679, 1644742015, 1492488514, 1073364814, 1075570900, 1268292200}
,{205106255, 84902564, 1825663685, 1917828095, 1216865686, 1623728110, 183617023, 28201037, 884872776}
,{1355750050, 125342824, 580556021, 2081387029, 1364708640, 724572130, 1400697599, 1483768687, 1597008876}
,{566126314, 1983364089, 844609962, 528140663, 874175064, 1140974785, 982139699, 77017586, 1491464003}
,{491779761, 1788740584, 1750761753, 1341750767, 1902316506, 1723991850, 73622782, 1530731158, 1151812236}
,{1933379580, 906078824, 1289097735, 304753684, 1237845910, 229395971, 242441504, 649395887, 1762176626}
,{2017687826, 1332558434, 184545932, 1729718437, 1329519341, 1270612789, 706199097, 1160426206, 1567615263}
,{1141465822, 1071498298, 779828262, 1310503589, 2051893245, 886733625, 1006342405, 1495154659, 1421214932}
,{2040524274, 473487262, 47917015, 1959535919, 1506315769, 1262542319, 1646136668, 94897897, 1150978958}
,{152574817, 432164437, 1836856849, 1290899048, 848147554, 56640704, 99045685, 1793103970, 1294302988}
,{338152651, 1714101842, 852076758, 1486240212, 1467837863, 629380773, 2027657858, 561811597, 949081257}
,{1555148403, 1581313352, 965981525, 1052945995, 215304072, 1359133958, 275536275, 419637387, 820050263}
,{1695543744, 747178567, 614333712, 1425451715, 1256208416, 583234986, 1054446561, 1163769601, 956263238}
,{564007739, 627436674, 129081978, 1535714391, 667500713, 322659783, 338273601, 1524692813, 1529722599}
,{1249949260, 1177577887, 1308045218, 873474632, 1092145621, 740095646, 1693256836, 1998821657, 70435494}
,{1176144847, 1480021604, 1657854416, 1160942176, 382682068, 1819990184, 1450354581, 760692255, 1727052514}
,{630659393, 1755069452, 1744102163, 25888156, 876513365, 782719388, 181660189, 928674731, 1732593215}
,{538989767, 1591992414, 1728076692, 591056637, 1453674867, 7481625, 114845388, 1409854210, 2033967717}
,{898948673, 1509282426, 346379934, 1878823035, 829168507, 443031114, 1694557597, 963605329, 1458274283}
,{180604162, 1145420961, 1221631393, 1837014566, 1236820011, 1296957869, 1734063780, 1230092243, 1128410273}
,{371783242, 186487574, 751764346, 566606963, 898290441, 271111804, 1086371944, 615873512, 1971224900}
,{1651207110, 1002815378, 695388996, 1926366286, 1310490902, 1498634713, 1566731217, 1055887813, 189310891}
,{65591999, 1626100372, 1538992347, 586281105, 1121457004, 1495768990, 2089215024, 284552576, 592366873}
,{1452008625, 1742746599, 386325086, 795055151, 981908199, 1771414492, 1054820202, 640616445, 1294563355}
,{1395648846, 1331719094, 337451308, 1438873994, 412754592, 1487487092, 1105321014, 407766545, 806907213}
,{890576393, 257808890, 1464386445, 242721044, 2035891216, 1341971523, 1316818959, 1821943873, 646591584}
,{1860228638, 297451314, 451804933, 1988386832, 1404892172, 1579106573, 1294982003, 1857265678, 540100820}
,{1569432360, 1995157523, 378402504, 182662787, 1086570997, 1445856197, 1697713935, 1199075645, 1203765433}
,{1375257496, 709498455, 787922130, 2091665799, 184178031, 1946145249, 1073138934, 1071822078, 1741775586}
,{71998578, 1846098692, 460798388, 1710095048, 1236778289, 1606150825, 435516511, 591296628, 1888817928}
,{864249650, 159357303, 1538417668, 497993832, 1287916962, 930611749, 31587550, 1560680013, 2137543328}
,{226979604, 185518397, 986172090, 1951202289, 628386628, 535572265, 1335592709, 302628835, 2133079271}
,{2035646982, 328700600, 1035287026, 1727927912, 421390127, 1978456168, 1201820826, 1951535717, 1563713726}
,{118520498, 306187848, 1205467161, 980708163, 1698061499, 49957309, 6866097, 656182482, 1683145995}
,{737426782, 697983945, 1349516951, 1493720637, 2016841640, 654632936, 690898694, 607661000, 1804160290}
,{1569249813, 1743836086, 1322687688, 2006330418, 1985828068, 442979375, 630773995, 1248688608, 260296867}
,{112789397, 379037483, 431231167, 914325130, 1243635451, 17438647, 1176746917, 180341162, 1395264409}
,{557219963, 1327973817, 1067038003, 924643499, 33637077, 1455069377, 235641174, 197923994, 1467602232}
,{390318551, 36955168, 29154131, 622214316, 567017270, 1081459681, 1905501072, 1243779129, 1098062216}
,{1763420152, 1921459938, 1043940644, 601534021, 1118356122, 445269426, 1532341439, 2105323982, 596241093}
,{19534197, 331316872, 315221016, 519240802, 1405111101, 2357356, 2032382859, 1848016266, 1402550502}
,{728499354, 975793248, 1931831282, 174867479, 1241751571, 76573380, 1905030599, 1548216234, 55630795}
,{1026544882, 1466461003, 338415224, 1277894676, 665869890, 1321918659, 1915853158, 392597706, 1172705787}
,{1419809276, 184750538, 623976327, 1485990276, 1958296602, 1405804209, 1098312495, 564709991, 794280464}
,{24677126, 1132929577, 1959865013, 156892604, 336568774, 1768537677, 1018754279, 1840438071, 873531455}
,{1731923676, 1012776137, 629151427, 149141968, 547214809, 2124589768, 744538467, 640172781, 430528518}
,{2136415121, 1698711388, 861317833, 230544824, 241953998, 1058340278, 276150371, 1315008200, 1929184775}
,{316102222, 1395077279, 262250682, 1841882836, 1672503168, 1499189866, 381779323, 2030382620, 961869251}
,{1383857382, 1956502665, 455404718, 871424074, 1223196027, 1686900946, 509234481, 800566188, 1801991320}
,{792784484, 356053313, 684956279, 1122199497, 1632604420, 508075473, 1902994234, 1959390068, 1805811355}
,{701224791, 500555698, 781621049, 923639142, 634438473, 1436243027, 1030555266, 1207189893, 1607786381}
,{1428130620, 675308854, 710803313, 746640754, 81062387, 175463137, 327268668, 1491090179, 1599290289}
,{1759153113, 1323637604, 534563244, 757110797, 958362747, 2103798385, 335104721, 747198327, 1448462609}
,{467839354, 1859641396, 52174687, 641502644, 266763354, 1051652528, 543089761, 558795410, 1144926784}
,{1855063272, 578905768, 1130891034, 1868674055, 1618489654, 1218123811, 1885663706, 1852936852, 110968297}
,{1149161932, 1192820641, 282892703, 17298209, 1398164005, 2079104153, 951070620, 1831907139, 649622324}
,{557962833, 262213297, 1419386018, 1083200236, 701805104, 1462790676, 477912552, 56937384, 1111373749}
,{352101689, 394661457, 1473359864, 1900622860, 924763324, 1237793507, 1791751413, 521890589, 1708469259}
,{1838068504, 1651037690, 916337184, 391937133, 1909585076, 1437210443, 735436076, 1852521424, 1690699307}
,{194374089, 1205128519, 1297143464, 880572513, 523952397, 2103465229, 1527298275, 1956630852, 1127621787}
,{1394062762, 1329184628, 502078330, 1312610413, 1970842339, 1056011026, 776719565, 1577629477, 1096549475}
,{1646693765, 1836011884, 455269114, 1613234647, 33897579, 1796539978, 1759215404, 1427005985, 71316396}
,{1837205461, 933183040, 1453244271, 192356302, 1492045580, 397535311, 347739271, 1251763563, 1376050880}
,{386308051, 1874776915, 47147179, 1512550109, 1877273082, 773201456, 1494513587, 1741089630, 194149888}
,{1110768505, 1017581281, 821786921, 798841270, 458451084, 1165683216, 2131784713, 1448580991, 1132563743}
,{879793454, 724645654, 165929573, 1561344742, 517003146, 1919956498, 1426680081, 1169380896, 1825706750}
,{456107292, 64343412, 490668837, 247118768, 1789635425, 1193196048, 932043679, 1961012945, 1640151806}
,{1260088601, 1104909141, 1471812133, 911714227, 941843690, 771252313, 450254547, 1505744412, 1703323334}
,{1614871920, 1323174442, 515135863, 1939484356, 2077093552, 757969270, 1323588442, 1690976766, 2129496778}
,{1191619289, 1879076389, 1101303957, 912322501, 57427428, 280893632, 143997727, 1613542573, 888551898}
,{1886786406, 1259722451, 520647209, 1500105856, 912908291, 1243592303, 407512450, 283734675, 276730243}
,{950935619, 1672930809, 543155018, 1888854465, 1854435446, 1232729160, 111699812, 990766072, 648673862}
,{571671526, 13769862, 1321616932, 361020681, 143490108, 457929922, 1376089824, 308885972, 1323870206}
,{1262657593, 46632423, 470589183, 251624283, 1044085499, 1000566636, 1838377778, 1540927343, 826883476}
,{632643530, 365112619, 1908201135, 394699464, 1307483759, 1217429207, 86236542, 579265039, 997366813}
,{1100756143, 549154416, 1621128238, 71980241, 1620541401, 486012313, 699404652, 1115678801, 1337428003}
,{1639661790, 1380769514, 1470337468, 724670183, 1792126207, 1880629711, 1993233075, 1637651342, 1433696602}
,{2102597298, 1756320037, 1466233611, 5230675, 1704941746, 805662618, 969503330, 739646333, 694446253}
,{2010416569, 2121030964, 1676841892, 377653063, 217040210, 236269159, 1486773504, 286779066, 760912631}
,{1900535152, 646003116, 1542553876, 422783616, 189257842, 1269306001, 1205670958, 1933534063, 1982127114}
,{1282428388, 5301415, 1269907315, 159543298, 1633765417, 415444938, 909753222, 55878035, 1077337193}
,{1246478006, 1728719196, 1812701560, 754808794, 1604149461, 1936696988, 1993851073, 1452825289, 2062028251}
,{665456592, 190005672, 262300956, 666487720, 1199427434, 1245586053, 1222210208, 1097707606, 1459474866}
,{761045263, 1155289911, 1679447053, 1858860483, 939601513, 1211046616, 826084008, 281240892, 2129657389}
,{65350539, 1079361835, 1088380714, 1781820556, 1495505477, 949114964, 1185687206, 2011075128, 1650622641}
,{236045694, 908347007, 468017637, 1366238845, 607762885, 181485920, 404497565, 1761535609, 957593063}
} /* End of byte 13 */
,/* Byte 14 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{2053962982, 47231560, 707464806, 1440754982, 854145785, 1690576528, 974756480, 1099590214, 1972527763}
,{1179156304, 147214654, 243690464, 1901695259, 893655729, 164006677, 2011649227, 1949237306, 855667766}
,{1255063327, 1720074542, 1806961978, 2147294975, 1610713514, 2032421654, 451369305, 378702983, 1800290017}
,{2100697439, 2016875562, 1254057470, 491610572, 168265475, 1174257276, 413570694, 1850099382, 1882962703}
,{70159011, 730088128, 275408840, 1143481649, 1531102209, 1886648480, 1762565519, 419209535, 847507960}
,{1354067547, 1435854617, 628331216, 1029804677, 603223758, 1713550252, 1317078816, 139330362, 769206496}
,{1969244327, 2026108905, 82377259, 1374504282, 1202213741, 2088492667, 103024102, 1553194211, 1839711773}
,{960872402, 1923446900, 134351781, 1697209305, 1259751052, 1801889924, 2080670684, 1398471295, 233415187}
,{16773300, 2056522622, 206440911, 880601510, 637682863, 2050651967, 240331999, 689111128, 1255487427}
,{763697278, 1094098520, 1644884877, 1656699063, 765656513, 612056108, 142175051, 640570154, 2044695512}
,{1647255860, 558641235, 1630791303, 473895512, 147903142, 2129585331, 2066552443, 72724704, 551946225}
,{678244729, 1056441603, 1875956983, 613326796, 95619232, 1785423589, 69942721, 696221026, 496186967}
,{1344526200, 2075166010, 2101154545, 338441266, 1893050456, 1604581840, 1539550901, 2054456280, 1173990758}
,{116582286, 177050982, 1259972790, 1975282617, 887122679, 780492276, 557303764, 62239382, 1080087260}
,{10618569, 294382313, 832193609, 2098762597, 594695482, 1257033455, 2002844635, 860896063, 615504407}
,{1733355497, 190746464, 1477953713, 1543481307, 944675114, 1409433140, 984652114, 554607973, 2146426694}
,{1803249737, 1500651968, 787752920, 1575602530, 1956968669, 547392013, 1992030086, 1522637094, 779800592}
,{23981132, 2076811417, 770109739, 2087770426, 1651591451, 1209424562, 561559507, 495360228, 189725183}
,{1269851545, 785292728, 1938414361, 457619226, 73505434, 877715599, 1523839719, 1964384789, 352338399}
,{803725389, 1204819141, 1851856953, 895837372, 907139270, 1889882896, 760901210, 1703263772, 1599487138}
,{1104044250, 532249156, 302990234, 805422237, 1618715523, 1219575492, 1164629000, 397802087, 83620422}
,{1206171845, 1885013276, 1688755417, 656088384, 2139364122, 1999724042, 1203600367, 986447532, 935114027}
,{2024629155, 1325772702, 1534725763, 1360938044, 389325471, 80281586, 3388655, 822881103, 423439632}
,{1846892746, 1841079335, 1964554457, 2093440367, 1694104168, 1838911968, 32445080, 2082084589, 1931742203}
,{1023304719, 2026278445, 1538664881, 662236340, 1694058784, 1560747611, 26035576, 223627159, 1508415096}
,{1146960703, 67556472, 303904361, 647506336, 1486122491, 2135726420, 1703883761, 158521202, 1645388435}
,{1295669297, 863231105, 279930498, 2052048054, 2022627408, 87624304, 678758721, 1921856684, 1779474298}
,{1034735955, 749312963, 1005599667, 49348964, 1026850309, 1997768114, 1809619380, 1563465828, 1338630728}
,{393236449, 354728221, 1551392376, 535156303, 1782658956, 1183463105, 1674051201, 56612565, 152231823}
,{577577340, 1951084268, 675457477, 1350773566, 1118050002, 1748932199, 2103440672, 1903059519, 344604988}
,{64074528, 1695399172, 2115211589, 730360282, 51324078, 384482899, 1873268392, 1363206533, 970066162}
,{762801865, 1294636212, 541424563, 1125567460, 1340256317, 1564526471, 1174956304, 1143829483, 1262038545}
,{187634498, 1819467708, 918863567, 1205635013, 1593474755, 1539047269, 1346564664, 1163834110, 112390993}
,{990137207, 484497018, 1779204908, 733830636, 1022839298, 889568272, 608862554, 2023967295, 1925063480}
,{1621145066, 1608087901, 974280980, 1215342784, 209138751, 1684932222, 599303067, 1757205987, 345735932}
,{2073778764, 825086918, 1803323788, 1076149936, 181632759, 528761080, 1618257410, 1583501666, 271652270}
,{1242102071, 1419113446, 1534957299, 1756120101, 1350056261, 18010080, 1528653771, 254723640, 1260902987}
,{1632090499, 787890744, 847094156, 1513556932, 1176334278, 109284887, 1560463722, 1182142994, 118339136}
,{296988503, 1749809655, 1297440506, 1701465398, 2086245339, 1967986226, 1252890421, 1370327933, 855011286}
,{1931547604, 1979228928, 693150619, 877838332, 996131164, 1732683996, 1804698138, 31445619, 877751015}
,{316999311, 721290776, 1738133105, 1192503018, 1334218136, 1791457023, 1433245694, 46621931, 351532819}
,{514998457, 671528123, 1386155042, 467510913, 1251380063, 169156116, 897453672, 1441892316, 317221023}
,{1080628233, 1742181684, 1228924846, 789265377, 2085271149, 612342526, 1658850056, 1926654775, 874071066}
,{1373507477, 1478056851, 66622206, 1160813405, 429083361, 675206687, 581919142, 1171890070, 1092663660}
,{783056016, 2110269458, 1652209881, 352014249, 147024343, 1406703123, 1481986335, 1443931767, 393267501}
,{227138689, 1549368527, 1919461341, 272921185, 2078269363, 288753980, 1818397448, 495475537, 1582730378}
,{424607961, 1072233732, 1574593684, 1814615482, 498169810, 1030995128, 2072738848, 1948150344, 245813919}
,{2003783763, 1476288665, 602578333, 1650598641, 1094836546, 1097086895, 1829210655, 473504000, 929110146}
,{1209509021, 193411236, 1012020328, 1588181357, 1549717702, 2024940908, 1162284469, 265787, 153760306}
,{1976299175, 88910908, 37578481, 97866037, 716031764, 1007818932, 973332043, 79322369, 1654636576}
,{2096960117, 67448134, 2025415660, 754078890, 2043642750, 1158873193, 868190766, 1094457216, 1231085995}
,{679071207, 248804495, 1341031214, 2130783147, 1500020896, 762748849, 2040436758, 1201881917, 757239678}
,{1345971879, 2065655255, 536299142, 1244569367, 1505729467, 1487190915, 1626395057, 353208550, 308364651}
,{1675254622, 913896104, 1402048842, 1669975685, 2038696102, 542305351, 245874328, 295429482, 2560501}
,{147384171, 2047446480, 536068378, 1581952338, 243907531, 1029494379, 68371163, 1880144978, 1518592071}
,{1794571694, 1940008555, 492448905, 177242462, 1125618718, 667663368, 1117760185, 322897309, 1978972301}
,{1393242441, 1674287857, 768358135, 1645877545, 737573797, 1294027185, 852319532, 488572250, 1640315333}
,{553353823, 1765931429, 11660846, 836604860, 1729478866, 1894380138, 1281363080, 388127782, 1808382507}
,{1852327139, 2013908301, 615110662, 1998706563, 657971592, 1258219620, 174733795, 1400678823, 489856551}
,{539908872, 54908812, 2050312360, 645546279, 522457763, 1154430895, 1109390635, 1867872292, 1157192649}
,{758343900, 1273195830, 83663248, 1453129385, 63425343, 1007775465, 267812747, 2111014200, 1472054020}
,{41597918, 1199131232, 1790983531, 1368197654, 1305029859, 969823596, 1022048543, 1332603211, 2019734741}
,{193412841, 1527534113, 1009593424, 2054620765, 736011031, 1567213801, 568442776, 1426169064, 666348588}
,{370901673, 1798109349, 4090233, 1550685228, 1882873788, 916674487, 434873439, 1291105342, 1645735283}
,{1620341197, 1565765830, 394062849, 1345980504, 383884462, 676523864, 586776226, 1784853919, 1799295055}
,{1541093000, 1020222426, 189542826, 1940018468, 301937866, 480839876, 1739235787, 2082905219, 1613030504}
,{1409300456, 315462455, 1905047590, 913825439, 1662176735, 1540194213, 614360339, 591425138, 1240429572}
,{206375885, 1846776778, 1666913767, 1348824321, 1505481593, 163932483, 1931659945, 1209614252, 223123146}
,{540689428, 2042964742, 1742668719, 1198152380, 66988343, 565233254, 900591986, 1527190279, 1572581210}
,{204547661, 481573607, 1502631699, 1539172915, 1051813196, 517356108, 33066873, 775980233, 28600838}
,{1525513632, 1525899195, 966420744, 669382040, 1311631070, 132975965, 1456750068, 1018512975, 418280617}
,{44144578, 1759815490, 2083589271, 1639554073, 1334236932, 1529616523, 1870346035, 448062049, 2143782856}
,{174886359, 176004020, 624114494, 2045407846, 1667490599, 7419612, 1910007818, 1162156745, 1797606650}
,{772177357, 1304954740, 2108908922, 742985966, 1403367613, 712570118, 1644761016, 1885306528, 1602503787}
,{1278435762, 2026910501, 453771861, 137234245, 758024723, 1794206349, 792167280, 2007459344, 484910682}
,{1518441948, 1236478619, 1777238989, 1275143525, 523542038, 1564017396, 800336171, 28658224, 943008389}
,{173213194, 1223586929, 1190987018, 1563114710, 983782451, 518717217, 687313445, 1859425916, 2050970298}
,{1801057250, 384846761, 1156162738, 2050321637, 1342494747, 828218547, 1617905523, 1806177251, 1891448715}
,{1135927065, 762374911, 955135935, 141992480, 498197048, 901671350, 1713292786, 186585099, 325518081}
,{1480501201, 843969448, 198605596, 62650911, 1361623774, 433168210, 1120738546, 314637514, 607255550}
,{1002628957, 1433862149, 2125385750, 306494024, 1657745422, 760819467, 463234566, 788607020, 1381732965}
,{173370981, 628167527, 1886245597, 1369158713, 1592100140, 674861866, 146805391, 351409185, 2005977837}
,{1699185666, 1652452602, 962900537, 1610142186, 979414033, 1701982713, 1763070958, 778318986, 1351247482}
,{1474805014, 351826325, 1033940113, 1383213730, 1262699790, 1338883123, 887453567, 1824224258, 409472935}
,{1322593431, 925396895, 686486841, 1248129027, 1335998258, 1643605200, 111225021, 1316574452, 16203741}
,{1904513346, 916842785, 179558724, 861887735, 1124200955, 894508208, 725474310, 1681713550, 1609343036}
,{1163472308, 251507358, 1839790535, 16860893, 748919994, 703285509, 412524601, 2127335112, 1292842893}
,{853241817, 1275840731, 731259575, 839602516, 1487629538, 655083548, 1360973792, 1651751877, 587393834}
,{164035084, 1920136810, 1071892179, 715750135, 1226850776, 144737613, 269614738, 406636612, 225534503}
,{1883267612, 2062811221, 242074410, 1320073353, 2017900, 544674098, 1984173758, 410864173, 1254079810}
,{1195818278, 1617007866, 788822309, 1860156588, 1907915364, 983723104, 874015952, 695257546, 161336670}
,{1186597515, 232078677, 482591246, 580445430, 1704253714, 1427382282, 197755812, 1799721084, 1503848064}
,{712651504, 2069718651, 1917464945, 1980594557, 2025492889, 102303707, 485794277, 1488444013, 752918208}
,{502774330, 1413166149, 1242510631, 705926431, 1718505148, 163453310, 111560149, 1633248020, 1146868341}
,{1714685004, 1646232591, 1930545537, 1866046705, 117173232, 1475589569, 232973182, 1999254456, 1135421090}
,{93889833, 1793414129, 1629066329, 999854615, 291215224, 1008933411, 309571994, 247259323, 526797008}
,{1274372295, 1461504131, 1929829141, 205028245, 916049561, 478602916, 1332821391, 919656764, 1023666423}
,{317907617, 874911214, 636011034, 699860429, 1875117485, 441521001, 588852866, 1138007020, 1476815028}
,{1896183732, 688382083, 793890418, 1676877227, 1570228588, 1313207965, 871138542, 1586727102, 1229622467}
,{643697366, 2107258225, 1751403144, 1884346345, 451315460, 378725594, 974604092, 684241454, 1715069504}
,{1331697883, 1027851559, 755696240, 912476097, 662105655, 883879847, 1197957570, 1566932771, 1294795191}
,{331944797, 276104181, 712259863, 1162144654, 397907939, 1985284602, 232158617, 758766591, 1313455638}
,{1645549468, 436199825, 1737234898, 888225126, 575582367, 719250970, 411755235, 207239882, 717796773}
,{724409989, 1620785348, 952459641, 822882010, 1419405607, 186125846, 1531560689, 1253203410, 2054526237}
,{1515111844, 1827931010, 2079624515, 475198572, 930193718, 1727138594, 830880913, 1580254623, 699227147}
,{50256343, 327729207, 120603863, 1426198848, 1343700040, 632262034, 1402550704, 1364802831, 2047135055}
,{1784689130, 117851013, 709744244, 867639156, 538943155, 566951641, 618685352, 1939589471, 1041202846}
,{1543397211, 2066224407, 504954488, 1999028493, 1183389141, 559667742, 523819143, 2137814340, 435154122}
,{977423096, 1951358311, 2038231140, 766350131, 1021499048, 1179763498, 1410565551, 1453768194, 2015918733}
,{1524432275, 920771873, 2146994600, 1930453782, 1988146503, 1997833312, 588759482, 1939136328, 20958600}
,{1002867165, 23627334, 1653215375, 1785438808, 1070366154, 209494738, 1365143852, 448645001, 1020714586}
,{127933872, 1041232888, 1382619324, 480668424, 458989687, 137462574, 690262329, 1205444250, 1110826023}
,{411888676, 568421462, 334159125, 2066138632, 434685047, 2142247343, 2053978749, 166130064, 806036254}
,{182999526, 1119107450, 240983699, 1324457052, 548221311, 1699302710, 719969574, 428222607, 1149962675}
,{615902057, 140912590, 12947946, 1084295214, 665886702, 1750707970, 473941980, 918415521, 451815452}
,{974491791, 860025408, 568748980, 787176138, 426239412, 1700069287, 725344006, 1177550830, 1087838445}
,{2082172198, 2000232735, 262961619, 1376726232, 1019424918, 1056724101, 482366, 2020802498, 97395679}
,{759900403, 391138937, 427166286, 1495485084, 34507089, 447296343, 592735113, 536408518, 1779499390}
,{1827684050, 834036989, 1408757823, 911420542, 702145735, 1268089395, 428055623, 1083723621, 430216650}
,{347096371, 434248406, 292701482, 527367301, 1182602995, 1493880682, 2132075255, 1391753697, 373366654}
,{1199678507, 985819999, 1543755749, 1914968654, 981516280, 1317551798, 867864255, 1074345253, 2001359812}
,{324563158, 1856089768, 5889067, 1165536483, 1374122986, 1527744298, 1403913845, 1211037549, 1916627485}
,{1197931554, 135632113, 541241387, 1382839516, 63330846, 1219594622, 329131634, 623398350, 918883247}
,{21470942, 292413785, 1603032893, 121631405, 1301675547, 499647320, 1428156331, 802892695, 483792192}
,{2073718055, 811074407, 45733368, 841495064, 1950335923, 1695885823, 1921779524, 598368960, 1740906787}
,{731734551, 386140465, 244933680, 719226902, 2048195633, 1311149378, 1718766680, 117658564, 289028218}
,{738140197, 304312790, 1227693512, 840438257, 1714602129, 1871324821, 680104007, 1354021131, 274327092}
,{272455637, 1778044437, 1566533755, 205760325, 670017394, 1360562417, 768214362, 151633663, 939068159}
,{2105332978, 1226467401, 1206346184, 708014941, 932702676, 274852494, 465902717, 222826443, 1176835968}
,{1394253901, 862266812, 371359483, 1268698458, 1822766561, 481652129, 1169148879, 752122168, 271033614}
,{1654012533, 804185515, 450012890, 200283445, 1699700409, 1621145499, 1922721430, 601072768, 200834752}
,{2045072895, 2088150306, 1829854283, 1457177283, 1283050836, 835503881, 1644802107, 1633115822, 926208663}
,{2041441355, 1550073679, 1268852756, 1149799326, 1926296205, 298698293, 412689077, 683235602, 1901912109}
,{1452795726, 975977424, 1961887853, 1408655806, 1441894132, 1111744014, 928532354, 439424388, 2147089055}
,{1089255335, 544818158, 139780044, 575464238, 75534701, 1774191526, 163058104, 947890631, 924888986}
,{1480336749, 451080760, 1031947259, 1980702730, 723911996, 899330539, 609807388, 478779956, 126842282}
,{748102470, 1964678939, 744556120, 464901026, 1187823792, 629101251, 1705486688, 726974144, 1362879571}
,{1678533972, 1777881271, 562222158, 1482406230, 587542275, 331400922, 660980200, 1251063160, 149670169}
,{756659785, 1185288134, 548923710, 72087863, 1531091786, 1457328121, 146233050, 1094648119, 1040590762}
,{1158503120, 360897282, 1713137246, 1803834985, 1673922528, 2102904837, 654998006, 71347130, 575209209}
,{1295975507, 1384588003, 1889567694, 371894565, 588233791, 1682615852, 441658687, 968933301, 356481318}
,{1231002761, 1496493816, 162354650, 729443386, 586031580, 1173017032, 1040288877, 43219732, 1859394862}
,{1980980152, 926185984, 1743302126, 1541082007, 1122901127, 1521333119, 1103461352, 295877075, 482473239}
,{955657538, 1236882971, 1149680474, 1974550531, 497063309, 687654468, 1767994869, 174636321, 543314779}
,{917850063, 1059715872, 422485280, 1103126911, 354554697, 1060590897, 1876639566, 1090548157, 1798223942}
,{631089550, 1116952054, 1301914029, 407206436, 940769337, 1969812535, 75608730, 1454485204, 861196245}
,{1452851818, 706523931, 204613424, 372461346, 869614162, 2130761077, 1284208147, 674607771, 1814617448}
,{5121397, 643636981, 1709453638, 711226051, 2042194654, 319721945, 2134421869, 930546038, 1501755176}
,{672867283, 329050863, 1951952345, 276234979, 1326910189, 415669861, 1913842076, 1324377170, 1937120003}
,{202169609, 1330720298, 1094231, 1187523411, 1591549745, 78844929, 939866147, 1610189536, 598486678}
,{1593726013, 1617766748, 600526138, 890382912, 556957455, 308343367, 1244225236, 25407350, 1621806706}
,{2115294126, 1851168725, 363913634, 1537784180, 946784021, 1755060875, 1360255063, 1383817455, 2073060647}
,{1769611580, 1850095943, 981822255, 32981891, 1406391866, 1230731890, 1624807917, 1289660220, 955010789}
,{62898996, 1633809737, 656103315, 259534016, 1785160612, 1182298359, 134924707, 963929645, 1984656148}
,{206550281, 1971307393, 1410582401, 306335212, 510902435, 2006388977, 386223712, 1459647101, 107400132}
,{305444016, 1876260827, 1495609619, 1301552183, 724693976, 1339858692, 167665618, 1028120852, 1653341866}
,{810633683, 917250261, 1945021737, 630955948, 203075248, 955651019, 1503169698, 2037581836, 1207908523}
,{848134437, 1721581625, 619500381, 1948205273, 1465663662, 1102612025, 770457084, 2108042023, 1049971501}
,{767855270, 1389225082, 1639779228, 936272154, 873418163, 46733914, 979177264, 1023056841, 622722451}
,{1622360503, 1644301157, 2077329058, 1001269010, 52537008, 1398597528, 299329735, 359632691, 1107781572}
,{1963692884, 1014682289, 287247515, 628119621, 938670009, 756886509, 988704878, 67324549, 1881474058}
,{1409012777, 2016019416, 809783005, 1062103395, 1467222056, 1200742397, 67400619, 931138996, 711826678}
,{346587872, 1535175520, 1469900206, 1187467399, 1357156973, 554876306, 1919938593, 1911353262, 902931696}
,{860246050, 659257649, 238795460, 470993290, 1098142732, 1431913355, 1348238521, 106483990, 2134615159}
,{1977408905, 2043615476, 2083688656, 421980098, 833284872, 1888393271, 456383370, 1335987742, 580437527}
,{274903284, 10887014, 461664905, 385416229, 1888603403, 344167594, 1844444800, 1887344196, 798536205}
,{1952642234, 5378665, 1351520103, 360596346, 570098842, 825192436, 633206021, 246348421, 591251077}
,{794869814, 1147801987, 1587300784, 702076496, 1639526185, 225352458, 1293679059, 141914643, 1911978607}
,{1350526391, 1464170213, 1864030456, 291714648, 1466921687, 25118335, 1318368127, 581905080, 1263254909}
,{1204439269, 850764449, 260633949, 654937715, 621304619, 1395578346, 416426622, 723936762, 582675117}
,{682606075, 573529377, 1766038076, 48008562, 1403860906, 1945229986, 1729452675, 1053312861, 1142575587}
,{1207125239, 2107972314, 728892976, 1410108311, 27863092, 1578104298, 2109291754, 27611103, 340321753}
,{418311786, 2128783487, 2009061830, 40545021, 1275298114, 479135809, 660248291, 990574125, 1465468082}
,{1032465875, 854079965, 268590853, 1638310829, 254447478, 2103329644, 2109405854, 255773686, 809171782}
,{295434558, 1001525797, 390042397, 1442272585, 1186160207, 683375337, 1976222129, 1716962481, 692597521}
,{2116990624, 220636372, 772681306, 506388011, 1979073578, 2056670030, 1126588619, 4707812, 1118432385}
,{2001649166, 1440728422, 2144235211, 1704970640, 423139100, 1384396308, 371206315, 1038177518, 1992512251}
,{1840466677, 1117302074, 1317254032, 2044195761, 214964464, 151016190, 190656542, 44730689, 1165676364}
,{765760357, 300200860, 1331775680, 262103690, 140428866, 1857545835, 144763726, 855681480, 1988423995}
,{1680990669, 693340820, 1595922263, 1884922248, 1592607987, 1071361727, 1972564935, 657653609, 181313218}
,{329608727, 1149953213, 1750761275, 1236305918, 863386662, 1786209435, 588717061, 1137629129, 1431584108}
,{1166615418, 1443474655, 873561715, 1436555000, 1758669297, 820410894, 2089656455, 168304329, 518646776}
,{1570747727, 775552314, 2020790317, 1108447973, 133220041, 977409886, 175458304, 1476534296, 1334419955}
,{2028973475, 1794501025, 1528099569, 1958623357, 764311526, 547489397, 1014355733, 851979574, 1969357228}
,{1120618770, 1115181062, 1452765926, 206436673, 2090280905, 1796706316, 394421986, 712634072, 981865521}
,{58230883, 1130348426, 1023631904, 179215536, 668817531, 1432090164, 951897076, 1564038477, 1946765164}
,{530312954, 1102621701, 1550286273, 298706408, 1804039486, 272767073, 703198521, 159961108, 459562910}
,{1469504198, 74272864, 625140379, 1701467465, 1768133695, 1492681677, 1928062892, 167651994, 1065210403}
,{448263481, 125174331, 273797479, 575459719, 332625564, 75656295, 1857930511, 1106300046, 589665642}
,{1894279021, 1114166693, 1814724031, 915074715, 1515067755, 558543218, 1404485540, 423504776, 937344354}
,{1820522170, 672348563, 837717573, 1848709318, 424687459, 1108907627, 1201219180, 1428282605, 461659459}
,{579165384, 1466474521, 1843306625, 1932325856, 467864101, 1693263366, 1894909541, 1376481699, 2026204971}
,{1782410963, 388002121, 534383198, 1194473317, 513615808, 86280831, 1685300913, 2000482368, 672440438}
,{334476456, 1865211550, 1287162453, 1092357929, 904897657, 1318648061, 933009814, 121559400, 862351867}
,{1771703003, 350254825, 178556227, 110115503, 1815243588, 52281321, 267631947, 768063598, 1331672964}
,{1444350308, 1842029817, 1189430526, 1975525242, 1420101200, 48017021, 1279998751, 561158038, 1008947522}
,{1617086588, 1915681855, 871122525, 1153726133, 1097803897, 61211361, 957072731, 1999866683, 719973105}
,{1556003280, 1723646726, 1991565772, 419272068, 549987515, 1179045578, 1364151455, 1552431794, 1509874940}
,{1737009974, 769676279, 1496537280, 743916317, 952176121, 1664961341, 1021174993, 1782369440, 328346817}
,{1369848304, 988076129, 761232346, 447512952, 280215449, 938125952, 1721378451, 1026146500, 2068547821}
,{1828564067, 881821386, 3029176, 1275301980, 175024641, 993443239, 1695433507, 1433519529, 639660447}
,{641784965, 1956076027, 1380512917, 1429559172, 184419607, 120838554, 693974329, 5552151, 1363592360}
,{308559391, 1626044904, 1125128759, 698266105, 914409732, 551681476, 502543513, 1918240246, 887270740}
,{211183725, 1800565239, 2063129591, 50758846, 1913060701, 116038200, 2022516221, 1597735998, 1139104737}
,{1758159517, 1687469901, 757006122, 100148532, 164309250, 749991523, 816631636, 1532314377, 1453360318}
,{747001327, 741183270, 1901756588, 465252769, 498483987, 1231352082, 1685531724, 304813395, 2002412034}
,{99769295, 1297203233, 1548359217, 1679837562, 1113363122, 725413068, 5974455, 1140875591, 1895172671}
,{1990136511, 1717110409, 495778638, 937102266, 1420415484, 54194989, 1443178033, 1827854325, 1317537382}
,{424562320, 2064502358, 1104441300, 1296496008, 1487348586, 1915905546, 1078119453, 167534923, 1750833910}
,{2062210997, 1030408348, 1609149743, 1542593231, 813130404, 2133995599, 754877101, 1385760428, 1187107713}
,{628121683, 1994223382, 1233513778, 1933074867, 509576109, 2096503340, 499737609, 577050698, 92860000}
,{180145037, 447698391, 442069751, 1547098357, 34442940, 62525081, 527560155, 825581391, 926605459}
,{651133337, 1453302019, 1014954777, 1092878397, 96719959, 903282804, 1128302525, 1234843425, 201395672}
,{1490707205, 1417063090, 1592453271, 75478142, 1913874827, 1432186480, 1091188003, 15684888, 619894928}
,{1610987700, 843204504, 245855872, 1027136709, 1895907014, 279358809, 1114447930, 1710821757, 974996852}
,{2059598006, 1613595137, 500524940, 1442129920, 1090934327, 1536980929, 928949256, 687500951, 2031425000}
,{225678989, 487952917, 258741365, 548153521, 1025544487, 1375332912, 1065865034, 1678339312, 1726615916}
,{1985137678, 1678015370, 136109382, 256291874, 1718259389, 768468818, 1799880578, 1559068954, 528340161}
,{803104767, 296881645, 226385969, 922413585, 237434617, 767414253, 1353655351, 1629750218, 1599757323}
,{1678380485, 798709815, 360507423, 1598775357, 224888524, 748147557, 1924016878, 710621085, 513042056}
,{1244456524, 295916957, 822522224, 1992594854, 1016404843, 289853903, 1043195905, 16863552, 151509141}
,{937730913, 385537740, 592131110, 128471720, 1221382458, 1453196291, 1264628414, 1189090328, 1664194302}
,{807554425, 619400901, 2077667200, 1290996382, 2144960111, 873627764, 53910565, 1808998596, 1082313873}
,{235980159, 1434784671, 1672127774, 1309763106, 1640099074, 1244817433, 484822396, 710578538, 1176648221}
,{683361108, 1490336408, 115035162, 838513426, 1915160486, 2110153479, 1416590909, 1309074249, 275868493}
,{1192661407, 616207365, 1579588792, 747564412, 109871519, 950395149, 65834484, 1154019072, 994455032}
,{782054332, 2100872555, 978964205, 430912003, 2140713785, 968160080, 640108177, 2032235241, 1536805633}
,{123714077, 779431896, 1691324302, 1705851722, 662272038, 414009422, 1761169369, 2055810084, 372235238}
,{1686163241, 523111513, 780980341, 1167021634, 465702985, 40328561, 874239527, 1612657076, 1619259926}
,{1198350249, 1531721854, 1875088304, 957646898, 1398512749, 2078666908, 2006548345, 792225889, 2087601329}
,{316257426, 1737561844, 57415450, 1734154006, 141261752, 123633192, 1302634021, 1385606281, 1911157383}
,{361118689, 906947544, 184401460, 1466909259, 2015926416, 1746923644, 2474914, 821797945, 117684400}
,{1011777680, 1760542666, 1788080876, 57969949, 864222070, 1406420478, 1081085344, 109086808, 1685035146}
,{1721734165, 2137139572, 1625609137, 1869693478, 631706532, 2071403526, 1544862997, 1006868882, 494050788}
,{945235555, 1734637715, 137292771, 700483455, 115457915, 866934359, 1873975210, 1118333955, 1334653766}
,{521253252, 1511611501, 779068480, 1587597131, 500945708, 470179404, 1044002418, 1007447920, 148338936}
,{1671347839, 1406149181, 655520399, 1336530389, 1298708811, 713803255, 2016301995, 93953779, 1158450806}
,{150414666, 1329681896, 1647557492, 72089669, 574618202, 1921281857, 652986295, 1454157201, 173037364}
,{920908054, 700615417, 780210003, 1101875050, 40843099, 1881536151, 1302788356, 216402396, 1683089178}
,{1434695892, 584295967, 418949303, 1341766752, 1709091913, 1370642593, 1335088766, 1518730778, 962787776}
,{1859812862, 826352051, 646580549, 1698903957, 685280194, 390165914, 2013261032, 135269137, 315813449}
,{563211148, 875255502, 1335762272, 156607178, 2117393224, 1537125059, 690394319, 1955078631, 466748062}
,{1798541855, 539435905, 1083932192, 727956427, 1527006743, 1520558991, 1732852551, 1302574040, 1071105829}
,{1867600906, 1998289056, 203777605, 1725645795, 1252900131, 1988873833, 903536134, 765163054, 340847968}
,{1000609970, 1008277394, 1610684162, 899418403, 443956723, 1731805728, 1863545616, 1965164355, 1366761934}
,{1365641618, 6549635, 1964924299, 840389589, 1582087757, 1461310351, 487098035, 133034365, 2072396641}
,{507344798, 591024749, 827600300, 746635559, 1097154116, 198705705, 1294924215, 1904024819, 323371247}
,{1913842535, 307402358, 763822257, 1905889131, 1867038948, 1248170206, 971207779, 1458592226, 2000776763}
,{908109660, 518900668, 840649297, 1040662779, 928290197, 201062287, 1521896773, 1903863279, 154563867}
,{978414468, 186242967, 1808048982, 207830591, 820204003, 702791362, 1776820370, 659685285, 1126185065}
,{981155418, 1464092875, 817721753, 1960139916, 1149274963, 905940114, 2003881266, 1903026567, 1234834941}
,{1691452101, 1594580609, 574009314, 1858646584, 858114974, 1109946556, 1903514937, 870293809, 1519879782}
,{1793772743, 1923780212, 1923025301, 1825531957, 1761015005, 1618082299, 1249322314, 744024053, 748987310}
,{1955478094, 1144695945, 914943165, 1248257415, 452348481, 1963517165, 764583069, 1947020982, 2133123149}
,{576329442, 639032506, 738589900, 197921141, 212494508, 652065566, 1154612046, 438052019, 2099285766}
} /* End of byte 14 */
,/* Byte 15 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{623694405, 1576373966, 1333266485, 556951101, 1726684937, 1894951836, 240536613, 1868627657, 750542710}
,{932365212, 1941826503, 210053829, 679449911, 444705015, 756505220, 2092760649, 913351325, 661902963}
,{1929178848, 88070402, 1629527298, 1008803811, 545007285, 271851264, 1963876085, 858318093, 1640460323}
,{745646681, 678084980, 1593850193, 801007321, 368953126, 524482824, 1839777934, 1660323597, 1613200967}
,{998261774, 1717761052, 273040053, 202919778, 2009234144, 724124337, 1844957635, 1167797341, 1707879257}
,{539830140, 993919068, 424095015, 1421312114, 808836672, 804978519, 34978351, 442702050, 1727632778}
,{1148563976, 173677147, 931575638, 343124253, 1763757545, 1489666673, 1806056673, 1106868788, 87863281}
,{1952692826, 2119693665, 397427817, 1267453831, 975050416, 2080496270, 742995946, 1651650344, 826469525}
,{1180947376, 472776723, 716540193, 1070696145, 2141884209, 1777432059, 2134548437, 1481645, 530752898}
,{2090866867, 1416369259, 1562271221, 1734971150, 1991419255, 1436185132, 200612966, 161518059, 1827513752}
,{1356721310, 489809203, 1120922108, 868606867, 2130992480, 1088698568, 1598871068, 1167995452, 863200613}
,{1217540477, 1847827289, 1054621814, 1459019899, 2035035504, 1099565575, 132902951, 446136225, 268275090}
,{1267475706, 1780905869, 750049432, 692474695, 530953143, 1981534466, 700757463, 769332130, 1335428721}
,{111318032, 738100576, 1398464447, 461688115, 238454066, 1987629453, 2098658509, 693525278, 1928455501}
,{2023828708, 1242811753, 803293610, 807992545, 2079435593, 1393465164, 745077532, 1835705597, 1114816087}
,{1625876192, 1273875864, 587847281, 2038002420, 963024328, 1993309385, 427059908, 600041264, 1182506615}
,{609697238, 261744904, 573434050, 2017386349, 1558372740, 1981086153, 1275918267, 174950585, 2034010947}
,{1256455938, 694562660, 617843347, 852912486, 1007199265, 40054717, 281701649, 1720684550, 1478701496}
,{818354493, 695840909, 1219808972, 1886965492, 2027334647, 1805281207, 1339579826, 2062357647, 2057128747}
,{655922590, 795408217, 1227883230, 1284627045, 610268199, 1639577134, 9786139, 1173827714, 1702667052}
,{935957010, 77850882, 1804597321, 1134911600, 713885212, 824007752, 657201149, 1871258839, 2099054083}
,{462135060, 729185802, 922878382, 842377736, 1085186960, 567438531, 1905390902, 390239285, 1485474122}
,{1873838146, 1127654877, 1606786168, 248941252, 613026521, 1116056620, 1216166351, 1004399035, 1872099594}
,{1160306136, 1524105440, 560926123, 971643977, 116837155, 688501563, 642073123, 1069040931, 923788835}
,{832611462, 1764365391, 1132150603, 1197177532, 490307881, 1687699744, 970952422, 1072092549, 1081443130}
,{32356163, 949858572, 536991104, 1964869656, 640991935, 1260656462, 310509707, 460326888, 1338870983}
,{683850429, 1637220381, 1224629461, 472049729, 208216590, 216756725, 1685635519, 1922186762, 1897683235}
,{348844362, 2017954787, 1710395809, 1440179156, 377281966, 2110607625, 1508554052, 53191779, 251290661}
,{1883668674, 534425835, 253038320, 382633723, 1373461623, 1378380352, 1059095385, 548821250, 107632362}
,{285579840, 1661979019, 1495663715, 2019225327, 1115843880, 1562026075, 412083677, 1552873493, 1968202528}
,{1770536234, 419721996, 1296154629, 1220908083, 1461850575, 1303272867, 1591628402, 878586507, 1905964574}
,{1970777095, 162253888, 871468365, 1218856069, 472510520, 1083094640, 492385241, 1153895417, 1894008224}
,{197674996, 1521006880, 1306614002, 1944672497, 338774254, 1022323902, 1163549001, 1644804529, 729460310}
,{1240608732, 1777704116, 1859972532, 1144205192, 2019648099, 484497601, 1797772554, 2018223351, 346902285}
,{164843902, 1521309788, 806246401, 367716618, 472861404, 1678362787, 970438702, 242811437, 65754854}
,{1836454440, 398441208, 188238862, 1161318694, 100738657, 1903165801, 1776988661, 646746845, 411248173}
,{1911133039, 315895767, 571452649, 339498607, 1041098698, 1687108668, 839958074, 1548751834, 2002274109}
,{1549925069, 474050656, 1225158911, 66477488, 1135456567, 897944523, 1233000875, 1245539917, 377395049}
,{549090279, 188267360, 1208508391, 350302210, 1453498746, 2036195227, 1247459338, 1846426724, 1236746122}
,{698519150, 2012648994, 373163725, 1386365610, 2063854565, 694425468, 989108270, 2113919539, 1216608544}
,{349638450, 1559632922, 1204006550, 905087157, 269453240, 363517641, 1184149558, 275886447, 924512260}
,{1454425180, 235306075, 1528881164, 1242984821, 374628587, 1873740909, 1839439487, 1350800277, 1727209590}
,{1645454635, 844036361, 771523782, 2114642386, 1862025304, 1878739974, 1617203344, 1978117945, 1706554935}
,{1281262495, 1083839792, 1519819117, 67703478, 958019931, 98527007, 89470294, 1539627428, 1278143790}
,{488814054, 629005870, 1659054576, 1716051073, 1480736089, 1961159504, 1402145479, 1117925973, 337842722}
,{129452215, 1221114015, 1053649477, 452452082, 2042872755, 638934828, 1689015746, 505541665, 255320437}
,{2125640473, 227831340, 1330505958, 455781282, 2003118149, 879721545, 1559474329, 876332908, 407911443}
,{2093544668, 932191912, 483514845, 594079889, 476762545, 1058444737, 1938420287, 667252065, 780095234}
,{1619033686, 26876258, 1322889829, 1887749761, 1147307076, 104459562, 1501077347, 2113991966, 1051654982}
,{651596215, 1218477215, 138444696, 165906641, 1479590873, 453547628, 838696485, 409233783, 1658481596}
,{1957139408, 6820540, 1285371791, 1610426559, 1067084882, 180815206, 1866331563, 1279333059, 511944129}
,{728040149, 415816584, 1098115823, 1666199827, 627772545, 1127370173, 2099400633, 179790329, 1531213571}
,{516698880, 1357728997, 1246064564, 146702961, 1612462082, 1176884389, 1800522669, 482887668, 154961210}
,{189055551, 2045336815, 1401775793, 646333416, 1773336617, 1012799731, 80928246, 1262253624, 1761044578}
,{468078052, 1451385679, 1248266258, 182115252, 2077418927, 428564902, 1313016907, 474178611, 945088772}
,{2011210692, 1326989915, 639889324, 427921474, 531316562, 1911556361, 78218780, 400544901, 498500029}
,{1442592104, 1304632483, 1006579034, 998122390, 1953100449, 1229221976, 146854613, 195103999, 1777439867}
,{1043419359, 1972952867, 104808278, 1249010864, 180676689, 1500381909, 546042251, 91528435, 1759254472}
,{2086720893, 1542710676, 1542027557, 101100800, 73246755, 60739087, 124904506, 701629317, 1653037594}
,{2059384794, 1421233364, 984074729, 1280773371, 1839235874, 56203592, 105410013, 492260590, 593202844}
,{2064769331, 1374313871, 1692141445, 425941322, 1815621091, 1832760611, 84307590, 1470175489, 119943664}
,{1758258613, 1187334475, 1002396145, 1288533007, 540322311, 1967808331, 1494779235, 228489363, 782473581}
,{1870222769, 482265175, 246971803, 1692548824, 2063900854, 794054847, 720671883, 1762436212, 588054721}
,{749855844, 462345846, 1452798263, 111055432, 1081039412, 629393030, 158768879, 2095595789, 1313948527}
,{1688205188, 2115892338, 1740220991, 155126005, 390123985, 236530334, 1013190280, 1947993054, 1211564969}
,{265286467, 1990204023, 1444550849, 26694449, 507495903, 501489389, 87913557, 1821285804, 299542601}
,{140787320, 1832805588, 698573489, 1072132313, 1430962973, 1783530026, 718583074, 176383143, 1691351420}
,{1073186198, 1123839274, 1557814366, 349626413, 114502448, 533475740, 1371097857, 1695103263, 1990822500}
,{115204669, 544761534, 1754266567, 953176599, 755650111, 1470807541, 1346858110, 1555518084, 640715511}
,{1359813466, 2012524319, 1134801535, 905129095, 494129328, 33351053, 371509390, 1848842632, 1135524595}
,{465300593, 1562256642, 376501981, 483020328, 463989419, 647782158, 2082513508, 613134059, 142018316}
,{1903528651, 164271783, 720810015, 1300733133, 1697699815, 679152761, 590480731, 127697236, 1223509053}
,{1312144878, 1026190564, 569029338, 1428478695, 777138676, 352191492, 1734252504, 1250987048, 1198286668}
,{892427493, 278282995, 1727241769, 2072274239, 1449830756, 1361797743, 1572735106, 18578399, 1335954134}
,{440615119, 328962504, 1321536103, 885534010, 362377937, 67373030, 224213719, 699685026, 761733272}
,{1567339104, 2111349449, 602851150, 729326034, 158727986, 1992277492, 1194044216, 2029397476, 1166425787}
,{1498886532, 672892576, 860904337, 1531362629, 196475867, 1436772478, 143285605, 2018083579, 1530256079}
,{772816716, 669601055, 1435788407, 386924668, 1675607051, 2117102122, 909551029, 1034949449, 132258738}
,{1742707174, 1304184770, 2017809202, 752033573, 731439768, 49992165, 389699209, 1582006491, 1358853502}
,{564539503, 1503947770, 888792639, 144001874, 854856607, 1628481491, 1499940123, 2088629025, 660707360}
,{751450407, 511040999, 1501792396, 1851751331, 1204409729, 999652633, 400294698, 1496899822, 895243156}
,{1123671418, 1764230137, 1452143120, 223580702, 1064101988, 1156196776, 617977519, 2047398035, 1958125411}
,{1732891133, 834739840, 72860791, 105178465, 1476709530, 1624092314, 1007667034, 1255733556, 607706056}
,{572054193, 417661196, 341899698, 1624462613, 1434681302, 1613306599, 1817351853, 236770188, 63321548}
,{1485962385, 494574183, 2023271468, 186403683, 1937922796, 511358260, 1844296077, 1366261156, 1030234662}
,{1048176683, 1233443342, 592709344, 1939416325, 1464683559, 1188704050, 1250404750, 428032839, 1207497883}
,{1496175721, 1267876031, 1670830365, 244204076, 1435563283, 851588711, 406406675, 1712574475, 299291305}
,{2137209481, 1655701787, 2048640147, 417050503, 1935463294, 1015052651, 727004078, 2095334358, 987347904}
,{1034287027, 103559609, 740851574, 1964456416, 351021320, 493178426, 1212484219, 1432712757, 1674932955}
,{471862088, 655214154, 1855716137, 960112916, 282999493, 1349055882, 1598294943, 722832233, 1425872582}
,{1532916026, 481591190, 1889341417, 1041774901, 180412427, 589315675, 1146210019, 533227212, 1282273091}
,{1200855927, 1230128190, 678747224, 1703865082, 1045841412, 1561447892, 1420730650, 132867531, 32970135}
,{1096523282, 900544424, 1202994655, 1941296386, 981965211, 731509640, 946966281, 1717232370, 1454675705}
,{696552467, 1316795580, 730931831, 461556851, 199490213, 1824621493, 1488178679, 1980803778, 1438944173}
,{1656938245, 949686795, 1841179400, 65013682, 510269102, 906629321, 1631233320, 1641565667, 687340395}
,{1616179419, 57857682, 1356787093, 981564358, 1808352652, 1532304350, 1894222394, 1821825073, 419094600}
,{1337961661, 194187324, 2144678125, 2068227097, 930461166, 1980758572, 592419166, 357641836, 375789794}
,{999781854, 2005997300, 2071118448, 1503237388, 1462209106, 1011828557, 1394855906, 1659282915, 344589174}
,{1367194909, 1770735920, 2114444232, 2069263141, 1212322531, 111101696, 1646061909, 312025467, 1854732894}
,{1480760338, 1384028653, 102910806, 1990540710, 831986114, 651014170, 948797670, 584719702, 842082583}
,{1215634535, 1380875055, 616436881, 1611408938, 147375600, 418532056, 40453932, 738256603, 211108336}
,{714928015, 725007283, 339265806, 255364029, 570851106, 2085395213, 1005241852, 1192019569, 1120011898}
,{463082219, 220803371, 1795647808, 1014383338, 377358783, 2098834059, 953681705, 1217465653, 1347110175}
,{258635001, 1213319305, 1569834515, 1903644329, 1176629448, 1230171237, 817152035, 1080770205, 1335230788}
,{712681856, 301957019, 2062890200, 2132725152, 602896255, 1770505287, 47348979, 2008778827, 651442942}
,{2099268612, 798357706, 371687376, 1064454809, 744633594, 1352103880, 2045934665, 885248588, 1293790047}
,{1248353038, 900384495, 1571931285, 826524522, 257271361, 248712567, 89017516, 365871662, 343964644}
,{1596504514, 440252985, 1757927432, 757384175, 496282068, 1384714958, 1058528832, 1675389272, 1198386011}
,{734379844, 1590410957, 1473809171, 401511282, 149256487, 1762874741, 964177194, 1567287817, 1426066851}
,{1209426549, 1724147252, 717374954, 1272975570, 113491033, 119731954, 31223676, 832550554, 1325336892}
,{226519560, 584000264, 419012978, 267505657, 1951906006, 504718418, 27613167, 1009460325, 2135465804}
,{1669002361, 632587710, 270350337, 393034161, 1442624405, 155811066, 860061558, 2024747898, 519214276}
,{2006614560, 429030435, 1390412528, 820281603, 711947406, 1874198986, 1378687977, 15618264, 1135977743}
,{1666047384, 1976342206, 1828165825, 2000321746, 1819032721, 1822722351, 223845361, 2029350052, 249766744}
,{563544522, 181379154, 1610342986, 624897122, 444239408, 198886936, 1111235829, 1202332223, 345545677}
,{2114803350, 1914228686, 453276007, 1617646398, 2056283039, 100305690, 1491910839, 988103422, 1925195206}
,{887137523, 29715229, 1156928470, 2100847625, 230233641, 41339643, 1464582142, 1802992240, 1746670004}
,{446841453, 2131229253, 464739903, 201612804, 2103207278, 1223215468, 895607948, 747143481, 1949081242}
,{993626994, 392984121, 1957366046, 759130662, 1304553858, 1548453226, 2059652511, 1219368289, 1844142598}
,{1520270743, 157628288, 1143800224, 1378414201, 676027572, 2095139722, 625249686, 1803821905, 581560817}
,{1895154779, 768137462, 1696798866, 1646646804, 1765567261, 2144942754, 301823808, 252518283, 1999308409}
,{763637327, 36766227, 1889514945, 459141058, 1514547313, 521112439, 1599751409, 865595706, 1855833910}
,{901837104, 144765444, 1996667026, 959216315, 1962773433, 258619187, 510285940, 780616236, 1260061021}
,{1452523776, 479104145, 795757155, 395460956, 1354565931, 1688707741, 312083628, 1145225145, 1490608042}
,{151941225, 678183193, 1504205024, 785169909, 593851764, 1161874676, 668052158, 336603289, 1442408254}
,{943686876, 591044590, 216421918, 297492301, 1134287813, 1978741101, 1778720362, 2037172186, 1065522642}
,{400203457, 2115921421, 1846545327, 862912470, 1317604620, 1653727664, 1697237374, 1772117942, 912111705}
,{1881116928, 588328209, 821817512, 233134312, 945688464, 788930743, 867814299, 1969508772, 1544850582}
,{375394715, 1932445068, 2052145845, 754884286, 2111249226, 1264186006, 106442190, 1791371343, 1806391803}
,{2082158485, 74355558, 1408257582, 1491243827, 12795157, 2029954647, 1449392732, 124962912, 1257548326}
,{310958507, 2105715759, 1149082880, 2118174233, 905547884, 601015911, 2119843317, 1054106525, 1073481512}
,{980285810, 306296941, 1183566074, 1524792948, 178571440, 1036938731, 2001753524, 1253675944, 169281589}
,{1295569796, 446231718, 460558309, 50369727, 875414555, 1710769207, 613408363, 157799347, 1571804060}
,{372054694, 1708810177, 677222408, 1552355259, 703818572, 934345940, 350201329, 248668788, 1475777955}
,{1623619186, 163068185, 215683664, 1077797697, 1521450416, 239046564, 561514096, 451771034, 719085460}
,{455534502, 739894668, 846841248, 1514111265, 613202136, 795205957, 246396616, 998375720, 263770634}
,{201458568, 958949131, 1818146048, 927941975, 1042087222, 1747432091, 240177175, 307009052, 75640595}
,{1463122238, 688361860, 1632933011, 1420921818, 171339668, 2109004359, 787407303, 1467451809, 624099176}
,{1087440718, 516545516, 170815648, 721259762, 172151895, 2068676547, 198398133, 2047684786, 99707314}
,{8447147, 1657097165, 1733312898, 1104155119, 560024497, 258276293, 891296919, 684827015, 1930947777}
,{108335497, 996860340, 2065276253, 185390124, 1764866075, 636820369, 661273118, 1886679815, 567657228}
,{1297689457, 1279956830, 1364876095, 1212102391, 198740737, 503642973, 436981778, 844410404, 332693825}
,{2038986513, 1577838319, 1056158530, 754988416, 896406238, 112573763, 1338880260, 286380325, 1333044036}
,{285797031, 1657771797, 576845867, 1069650196, 151508775, 591375195, 1980477284, 805979876, 1587402736}
,{1721668291, 1291030566, 689604422, 1509928817, 1541850104, 1869562670, 1108999311, 155411417, 1165333561}
,{2092222430, 1027521356, 1523136282, 1041185155, 745865101, 1368985329, 2010359058, 122811120, 702881209}
,{1794166491, 1755726397, 2105731655, 1382697939, 574499062, 161400484, 331254568, 2125752299, 1870595222}
,{515562622, 2146368842, 970677627, 1770759807, 8451017, 33186642, 1173810667, 2111545350, 343151968}
,{561052955, 1672080195, 867236304, 1236341911, 104463623, 938840749, 1505009683, 1031711069, 709732390}
,{1782501796, 828174977, 426721676, 132622590, 1538205487, 1922286266, 398166577, 1818625388, 1760680060}
,{2010735114, 1060842892, 814585764, 233381596, 703067536, 1860324155, 1774085045, 149322742, 865552941}
,{1892409542, 1288499043, 1445910755, 1892168289, 450506037, 840904068, 2010587790, 114720739, 302979596}
,{1045551010, 2043137520, 1784283225, 116548157, 1397019516, 603452843, 1895197277, 1278590210, 2023254766}
,{205283867, 299376678, 623301715, 1505951998, 1513705178, 979146601, 387974587, 2121780169, 1737449760}
,{1677906907, 1262978848, 710060693, 1640768789, 2062106662, 1427330707, 962114222, 1518777176, 1101037921}
,{752618347, 333140253, 1778477211, 1294241475, 1465656208, 821343108, 202516935, 686489636, 795888106}
,{1897955693, 2025872766, 712717465, 54335998, 175687155, 609968997, 177105769, 958716620, 1987619985}
,{999884088, 185799845, 908412653, 155693240, 61431720, 694833873, 2061085168, 293306890, 1032516132}
,{980438216, 1237306951, 1433348619, 165509002, 558681924, 464656846, 1914963817, 1320632766, 740705550}
,{2121921807, 2081744488, 155512665, 1286432194, 1899565744, 2090691132, 712260957, 715018312, 790315876}
,{1826151921, 1801923097, 1240517123, 1079309268, 1635602897, 1270144107, 1547585962, 752402546, 2016507100}
,{2096352584, 1935550214, 471618738, 398823795, 2036662031, 1631323539, 1940764964, 149052787, 588629185}
,{957741628, 50347253, 935693039, 2041333822, 452026898, 574131142, 949495504, 2138507125, 240420316}
,{471997395, 1444926501, 169258861, 211424487, 864627036, 742856501, 16633268, 1064596302, 706766695}
,{1631614011, 1820456318, 817070747, 506000147, 225876955, 283023773, 2113876797, 1591504368, 527843885}
,{384976635, 1632450706, 64163192, 1169498074, 857955675, 960837972, 1016360090, 1887513866, 841570916}
,{782219909, 2071220844, 1729437791, 1218836398, 924099653, 616327870, 439981213, 313223697, 492348812}
,{592319342, 51832262, 1160038315, 1413952052, 1101577479, 59268857, 817171668, 1986935427, 1157769356}
,{490783086, 1642905791, 1238254441, 1959363485, 1029948330, 2115376805, 1034743471, 630835629, 1775523501}
,{2073376685, 1427427467, 1224448751, 316655656, 957597649, 1238616539, 683558780, 77410083, 1252626667}
,{942772140, 1299768876, 379797251, 1476627980, 986489099, 969798627, 791987008, 2058425986, 910285098}
,{1467190183, 2022650563, 1263668680, 2022895381, 2072987240, 1831256546, 1266973983, 1901577034, 763190184}
,{2085525969, 2113967701, 1887632250, 1283817443, 1970911839, 202917606, 313372535, 637266144, 29881771}
,{965899345, 1083664079, 1643177913, 1870512294, 1796306769, 1282470220, 1194313336, 1023005897, 1116132158}
,{116307337, 1811931850, 2021051542, 1202609345, 862388364, 1449101735, 976995023, 1182783634, 1951652645}
,{1056635722, 782140306, 663436797, 1890750346, 1165310023, 627066168, 1302957097, 1112592815, 239031626}
,{927486746, 425758963, 33306712, 502072567, 576918086, 745383696, 1490409336, 1698416217, 1807833510}
,{398617024, 1555078179, 316780458, 1701316316, 780626992, 1845058950, 744751649, 259537233, 2085653331}
,{1995646847, 1499352462, 814104952, 988428003, 1104059416, 1552495342, 485479947, 1570129298, 339765797}
,{169408965, 219447517, 1261649893, 761719166, 578073278, 697025291, 51323400, 743755976, 2035745604}
,{1206829295, 1263714485, 1398599955, 1096974643, 1712746268, 304453320, 1292041293, 2040624561, 1105901854}
,{260127816, 863858753, 1439974113, 508834300, 747485050, 343317288, 31322874, 2001109847, 798715880}
,{625788342, 994331601, 1028708285, 223131862, 1865217440, 990054046, 1326434463, 81002347, 441002248}
,{100320105, 548529987, 447657409, 922449328, 481881197, 1050288862, 294804672, 2107757881, 1999023768}
,{262471644, 2109604964, 1268946977, 2090081055, 712503475, 299752971, 197665796, 1591401297, 1980874121}
,{473030559, 1069794348, 1804789417, 352264191, 1764357903, 796859470, 452146779, 86638636, 1304537651}
,{1291407033, 1062867432, 108584141, 1432594572, 416560577, 2006739341, 1661083201, 958589634, 1584292758}
,{1959447057, 1366388017, 1942199371, 696109580, 1605246997, 358459000, 1816738721, 167625210, 472836350}
,{1970493014, 58270117, 1898184328, 197948401, 1762182949, 871836115, 1056170776, 1546275547, 1972605784}
,{1892065422, 139830892, 1390170557, 425996306, 31108323, 1839840006, 1390471649, 905698870, 787855542}
,{1589381342, 1535423760, 87337534, 107742090, 461111008, 1193888655, 1280089240, 1807194503, 150715479}
,{124663394, 1563172829, 2070485926, 2052600204, 1823369784, 875295547, 368268114, 1279461270, 301816516}
,{1782006211, 1886627679, 1787574850, 1202992854, 1295868003, 296557864, 2113276327, 1214965416, 441133378}
,{2001293427, 1061206822, 911878992, 5510510, 1047789364, 1112373399, 415174120, 2007683215, 1955712974}
,{938823309, 1819118066, 2029134605, 962353112, 2013860923, 309413902, 2135588104, 644394005, 177710286}
,{1651022085, 69923026, 1396641336, 851585293, 992508841, 28936208, 98023475, 280406165, 1431237595}
,{502654990, 615503300, 1333015030, 635865683, 1736051536, 1513315877, 481231926, 1111679603, 1669086222}
,{582509907, 1769050334, 783955896, 316380310, 1235747923, 169307496, 295201543, 535182317, 726311565}
,{940780193, 2138753706, 424940590, 1636359917, 23562583, 1057861362, 269437478, 1327308435, 1169617366}
,{510582115, 520838766, 570657754, 206954713, 1495509458, 1952747761, 2142053867, 208855136, 992913431}
,{1694410629, 585352737, 1493516613, 766938794, 559854941, 958676658, 943231418, 222059433, 1877998510}
,{1636017296, 474515734, 260775329, 1405981214, 876029068, 1619896598, 1626546802, 1051438198, 293277334}
,{1583505222, 1764709193, 833781425, 1855136552, 2131168941, 1425230730, 1408692493, 610474903, 92141625}
,{1431435716, 1786191101, 1409676558, 1259757277, 476861409, 855691871, 143816762, 1424169775, 1589014723}
,{131795085, 115284268, 867215619, 955043597, 668504420, 606026900, 655106204, 1155253155, 1874778551}
,{1981052478, 888978468, 972445438, 1714585678, 70507251, 2128345383, 549647101, 1629583375, 1110765805}
,{990453199, 402920411, 814519100, 30853137, 2006880438, 2096371256, 1798776078, 1334016792, 680981102}
,{1411651856, 773685347, 1421618125, 1641811615, 1286027969, 1997594115, 185589273, 825236093, 245576060}
,{2125313879, 1878366537, 329464400, 1728041440, 1364302836, 1027132497, 1795329126, 1743921340, 753930367}
,{1781183159, 1244334198, 511741551, 1206453918, 527572841, 513545910, 1405742306, 1358689205, 588902531}
,{1945764614, 60317990, 2142549890, 1277741415, 1637614065, 882538928, 545171077, 764808991, 403563277}
,{1822894843, 1360788894, 722599342, 1554645983, 1619603685, 1044898246, 1001130050, 130508642, 1788674025}
,{1630373533, 658152627, 484220365, 1880540622, 726980820, 1483386261, 72409356, 1425700618, 13237239}
,{1194395866, 823295297, 2072419418, 1191616146, 1610714324, 1049501838, 1490345410, 1743737076, 570909618}
,{1771274861, 397903512, 1402093714, 1439231385, 1944346073, 2032815140, 690608415, 660647528, 424629250}
,{2001509132, 1986486920, 129970454, 392289961, 479712397, 426965265, 517403523, 533321275, 1903662686}
,{1146922543, 1841081493, 1253332767, 389793808, 1558050359, 1761781150, 1710451188, 650129351, 364137445}
,{1143249891, 1448175865, 1310090100, 1497480194, 41687787, 81670762, 1496253698, 759043035, 1386635460}
,{1077265125, 1428134126, 951912109, 655156828, 1031841324, 1587962244, 825622053, 1762053480, 415122606}
,{1475209992, 1151813171, 1483573570, 1397711492, 2002127754, 1923979862, 1024928983, 1361108895, 988868729}
,{1233250265, 1318750971, 1977449552, 1123415536, 539520591, 1423984290, 405313227, 1733175183, 255031824}
,{254030986, 283326089, 1354067701, 1829580760, 724587941, 838660338, 845788218, 1318806519, 155631477}
,{1518339302, 1697908896, 1980789963, 1767122131, 305133842, 1381238505, 2034227398, 840394177, 1836356980}
,{1781826181, 444946336, 1177179160, 2095404854, 514641931, 2076165002, 665269774, 1218315339, 2128341027}
,{1081266923, 97834323, 1437063053, 271002834, 1290845103, 41513447, 26668976, 1979655610, 1671707463}
,{2035472916, 2007175649, 750466860, 1389727011, 1659756532, 865263399, 340250488, 2129632799, 807010870}
,{566571562, 2105639074, 1207481038, 1678096413, 1451077935, 2122087392, 1753240639, 957088007, 1867716409}
,{2135371574, 419539827, 1184492645, 419925426, 439862289, 1175398329, 1739470871, 992226627, 844202246}
,{1342942528, 147593141, 999593078, 1003166333, 97803499, 536556715, 274927316, 477445043, 252820281}
,{868033536, 791168657, 1787906707, 1492173736, 941136618, 57860190, 693913817, 927181353, 2012487515}
,{1275923140, 1125912544, 721415145, 1349295854, 1260381799, 679339445, 1020517251, 25747175, 714499376}
,{1413330743, 1848671324, 1658017672, 1912630636, 532811559, 1676128111, 105005192, 1016885360, 176901683}
,{622389139, 740432098, 820219608, 991002146, 643425508, 415221717, 352769460, 867532835, 1950401751}
,{113629885, 521476107, 60051008, 486126745, 1181321155, 1018576559, 1743658677, 1701207308, 1444892056}
,{376891819, 293369610, 2099432044, 1992329508, 232518157, 54083291, 1114135251, 636014529, 976280713}
,{285587548, 1934297529, 1051726845, 13475228, 654912532, 545851699, 753304568, 608812088, 1300821760}
,{2002817761, 1899840931, 1411171970, 751131711, 262067025, 1306228897, 631869309, 1496345094, 1241584795}
,{194262557, 1117782148, 123549240, 907246274, 700714331, 727667706, 513110637, 405535004, 988273809}
,{1964666209, 1240603296, 1300180199, 1502508592, 1324560031, 1519337764, 1197908711, 116872902, 1498402880}
,{620439676, 40723407, 2120842670, 1089137923, 2006576929, 1541556438, 1366549552, 1899335768, 53319222}
,{1883336775, 416420765, 1326194359, 832231417, 2137703310, 1367865629, 1813809030, 1271140080, 1692804282}
,{1235778087, 336095651, 1719843897, 2041324615, 404416544, 655192597, 1168403941, 1739757418, 1728236730}
,{1971823342, 916230683, 563763017, 66313774, 400014474, 440840878, 194720345, 535032565, 749623988}
,{1666464056, 1298083838, 89582648, 431015868, 1222872615, 285324689, 956086648, 955379000, 888489965}
,{1095061960, 633904426, 1470825111, 1446386706, 567235129, 250632740, 1383103652, 103306969, 1497445331}
,{1725568829, 813428019, 1364145848, 627001606, 1731515557, 1175846570, 2026341372, 1206272314, 665075153}
,{1744276977, 1458308952, 626727917, 1138983728, 1383551647, 525689463, 764859152, 12289566, 2023495393}
,{22936752, 1430548439, 1363077388, 1971846698, 226833052, 563527117, 843720350, 602802752, 1304342277}
,{871805714, 426157923, 841176579, 451348723, 514891717, 1624264476, 1990670930, 828355721, 332341397}
,{1180617979, 635801876, 1676295397, 989697280, 183714357, 115211401, 1743345589, 1345893965, 249522381}
,{263067008, 1331066833, 285396031, 1268352935, 1282246891, 2097986839, 1591210102, 926177764, 1387581403}
,{506059315, 1986443466, 1326003536, 867512728, 502189538, 1272451794, 236400314, 140527524, 882501634}
,{1167134528, 1195788996, 1011183380, 1959794859, 713414716, 1646285370, 1508725324, 1861237725, 598852424}
,{1196835563, 96278310, 135062745, 1142110828, 618344014, 2146992057, 1208976625, 1255839921, 286174224}
,{443913490, 501589407, 591048117, 999655871, 1125770220, 1419961509, 1060429230, 1702247464, 959480337}
} /* End of byte 15 */
,/* Byte 16 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{223523295, 1025489125, 1401138388, 1931651083, 397527729, 410385060, 1257503617, 1169402270, 1061978758}
,{853985131, 1580046827, 1667963498, 399395201, 1806173930, 314784390, 484047138, 15230338, 1586094947}
,{1775135080, 1726821872, 445741792, 1403845606, 988874153, 1105524594, 1562046737, 535006701, 908995990}
,{849940231, 1742252521, 922835159, 570951374, 110121404, 478405799, 648018947, 1095764019, 1766713462}
,{255906636, 1326048753, 1272245768, 619359662, 487957782, 806984701, 1097175305, 772219217, 2043042156}
,{1511139589, 1842257344, 1157944880, 613794144, 586330687, 132242482, 1326408376, 1867538130, 1006434165}
,{1771715722, 293277776, 1114254981, 2352789, 1472481703, 2035409285, 938732643, 2143402558, 507542443}
,{1626323495, 2071236246, 1593478516, 1497027233, 547391819, 965152111, 289307636, 966272831, 1390312334}
,{1469218138, 436474883, 1661517479, 1420804030, 640121693, 1854983817, 690150596, 749754592, 1666197774}
,{2105557366, 1241871219, 335127301, 1026308207, 916508955, 1149171235, 1543530104, 915569400, 596059665}
,{95581071, 818662481, 1759851386, 558725781, 161757618, 1966325112, 212644138, 269551968, 926407888}
,{2134033597, 907519465, 1123188558, 1928738071, 1487383156, 1985968806, 1072344570, 479675648, 31006491}
,{1066786875, 1152175584, 1618127045, 1127274289, 1557339695, 241929266, 30213249, 2083067293, 76646866}
,{331278639, 611387685, 1791243267, 1795098470, 2012913826, 388065979, 1548045992, 394288914, 1230667612}
,{2017238759, 1505421010, 747976240, 828271084, 1077265212, 692013262, 720519963, 898211644, 1192387866}
,{919641396, 1622821963, 842588455, 370355330, 1434920135, 871450745, 1074451606, 638542362, 1426178673}
,{470630360, 1208696598, 1761540347, 670177055, 1654102874, 1043975972, 537155298, 267424336, 1131007627}
,{1189942405, 631224685, 524459457, 336963563, 109004615, 751615755, 1872006678, 1614496508, 402924296}
,{1412329321, 433509458, 464771671, 664351785, 866077447, 46568378, 233724374, 2085786807, 887671341}
,{318607616, 2145276252, 2005794679, 1230450067, 1528855217, 1604266857, 1122178732, 2126177699, 677318840}
,{24826754, 1185534070, 643189535, 917054647, 909073401, 532599612, 1745269859, 198836014, 195235702}
,{1677458109, 647466595, 472949102, 542838531, 398863306, 1026723898, 2046331020, 1974106321, 1962783923}
,{1927406602, 2015085482, 1419545307, 393280857, 1226437326, 1555241536, 445714040, 237280943, 1465511543}
,{1658788917, 1919890496, 942058705, 1423687991, 1737126604, 1017191828, 1445035024, 1347422462, 1480277107}
,{453637159, 991779731, 2088517407, 2060655727, 597446066, 725632543, 9184917, 1735318459, 2030194070}
,{273768605, 2110117454, 1931204766, 566642058, 791728196, 403685707, 394050398, 213976054, 72591483}
,{2087133086, 87512000, 1226777445, 1413237485, 1049673967, 1260514, 582091171, 243137934, 320330596}
,{1322618066, 494697427, 1327873259, 199226955, 2001716844, 676026198, 876515725, 429317363, 26094667}
,{682474668, 1403027955, 1591219724, 1251819674, 2135800873, 2023155280, 883115376, 513232563, 130808376}
,{1062368858, 1293190485, 892879353, 505636402, 861660399, 706613020, 645564796, 1997675641, 269848107}
,{440663373, 260353135, 419535188, 102444378, 1634538519, 1501985325, 430711236, 1669682363, 1372272239}
,{1086793760, 1390142628, 2006248685, 203085511, 447618653, 1009764812, 793837542, 139990736, 2116608813}
,{1432210272, 1979585317, 1486034647, 1437044629, 1171224716, 619318263, 1591395802, 1094804463, 1110165701}
,{827669020, 1462960421, 1895910083, 422425510, 1878044592, 1173276264, 1914760120, 1899743115, 676263590}
,{925995387, 433053743, 2025316556, 1312518322, 1747947929, 860574634, 1079870340, 934563703, 1742973180}
,{77686912, 1464025476, 1343792219, 724142573, 1478595663, 577841598, 926799295, 2010375555, 238093307}
,{1348909193, 722357583, 527877578, 250689815, 120973060, 1646103736, 1669728188, 525278408, 796119076}
,{113463842, 1389151526, 1950883251, 1396266240, 32191395, 1564187546, 2047662704, 1109081023, 2113619231}
,{1571509129, 1602097237, 1991085272, 1229505224, 468365530, 2018436683, 1606754980, 667140981, 342240369}
,{1826555841, 1852188682, 1714016753, 547728833, 1845065164, 1320268320, 1251922841, 216930021, 1876513839}
,{297155847, 1233436431, 767115786, 275963981, 213454614, 1881051161, 216122062, 415069624, 497676975}
,{1294247312, 311385881, 1421041612, 1860941747, 1045978356, 1146889510, 2093372107, 1879880786, 1032265175}
,{1775943990, 842042943, 833453079, 29859824, 1257920880, 1294204370, 1454223373, 916746014, 292814681}
,{267693917, 2139648731, 797010463, 2053567896, 539903106, 1723833313, 1589533174, 852986191, 1422473945}
,{1007694134, 410133695, 1536574901, 756186168, 532855232, 1560679301, 1097711328, 1230980662, 316760365}
,{310236447, 1991956740, 862533285, 697713592, 436294701, 487509613, 2087769291, 1148110346, 678701174}
,{1739936348, 1334746171, 2012421120, 1118464739, 540298335, 1584761908, 169264264, 1488718976, 1737233611}
,{829499274, 1222320748, 409207839, 1928094907, 1251035516, 287750908, 1167488480, 1519473939, 611845455}
,{1209913158, 959324959, 532479039, 1888831052, 1951950396, 321106989, 1386459234, 759325226, 182948014}
,{524446385, 395582072, 474308670, 1184969278, 1424905550, 748896749, 1178562449, 1738840062, 1031059893}
,{1273118309, 1803058488, 1565530751, 1979930910, 1135055850, 176351074, 859562781, 1786458119, 402421326}
,{462022989, 71595233, 333130640, 290063693, 541239228, 876254828, 885434909, 946529116, 1927940955}
,{1381887582, 1480278183, 870549195, 88171713, 1742016131, 1211365894, 1949809685, 2090092882, 1225231987}
,{1269980690, 1569376531, 1277539104, 1294872387, 541543676, 51141466, 1903884685, 1165383659, 26285213}
,{547726552, 2020750541, 978527582, 327966382, 676688549, 970052789, 1605376747, 517703797, 173369673}
,{1044604040, 1018486168, 641300807, 695798835, 186712931, 652874754, 1916033196, 1743174134, 1079840537}
,{1292775150, 1843815935, 1647601444, 1865564024, 910683780, 317602809, 355324455, 1204329553, 1885032807}
,{363602495, 828172787, 497356448, 583418635, 219334876, 164041002, 654690462, 569025338, 1952911290}
,{1752583277, 2041023059, 1261381357, 2028682971, 2142406754, 1105496365, 1411328541, 1642092164, 1997171226}
,{779268821, 1700814130, 1085687826, 2139998914, 234046291, 1535444225, 1085153259, 1652821091, 836674915}
,{219525926, 922035042, 507452738, 1296923199, 957318675, 1489446062, 200894981, 1333984138, 275675862}
,{1641936840, 635039627, 1690529194, 1578844886, 436254410, 60361733, 917783311, 291253086, 978560924}
,{378118163, 1584767117, 1223893827, 942946486, 1508552338, 1774548307, 1354421196, 39402338, 1280013155}
,{2049211379, 968276961, 526764154, 1929214317, 389523122, 143678796, 46979846, 409532818, 31438271}
,{337264956, 572686488, 1597144666, 1119086360, 846475351, 25147024, 2017839937, 90719835, 277852497}
,{60010375, 1173990792, 1486959435, 1185435018, 565867568, 616116249, 90202742, 1368612423, 408975767}
,{696301172, 484717305, 112701850, 909227582, 104696212, 1529502083, 2046982315, 1803396507, 225126711}
,{1097563001, 485716605, 1491431895, 394703865, 1000906112, 208943737, 1096179040, 11041201, 1104415874}
,{837822360, 1592728607, 490919115, 881013571, 1544987599, 1299490781, 143471524, 1260295267, 1855503532}
,{1810594670, 1129719408, 1898184786, 397895612, 841814798, 1569753097, 1026648655, 1649181318, 49852972}
,{1148065692, 1399087275, 403842138, 504449579, 349704456, 138270824, 784944079, 122344969, 1273251358}
,{1534233413, 1322862237, 1042355207, 1904877994, 1932118809, 34261266, 1674608858, 1104016529, 364597018}
,{146439224, 80144672, 408071237, 1095235860, 1004094135, 458387673, 1643499922, 734759252, 2035418585}
,{365242051, 1127806849, 1553111822, 1504395508, 831751654, 33604490, 467608427, 2092351835, 1710041247}
,{134200336, 499861679, 782099148, 1656298692, 71821612, 23394832, 2062401145, 1471329203, 2026462585}
,{477926562, 1750652618, 457537578, 1660216645, 1905441001, 1798126686, 472438332, 635869770, 609144758}
,{445539178, 67716279, 1378360629, 402396655, 594792020, 1844512849, 1699148773, 1203558955, 1784159121}
,{519731379, 120928463, 1659286309, 370142272, 361318030, 1979131945, 1503461040, 1991414555, 1274935671}
,{558202640, 810549876, 39125478, 1142785324, 1669255805, 615178952, 1112303033, 1859986, 165366257}
,{577067822, 1653776495, 1868803133, 490297932, 1571845365, 1666551122, 1500258605, 502078332, 537124490}
,{1325285395, 1866146196, 1661241371, 11481549, 112608175, 865425396, 928845753, 1082249906, 378065802}
,{459588744, 949708177, 909646427, 1099137979, 1323168579, 1218348846, 1127469425, 1241394592, 29687013}
,{401971259, 649471418, 1665589777, 322160381, 41001445, 401407295, 988226312, 1264771360, 1745815116}
,{967027595, 502779802, 1445801340, 1224436016, 588057672, 1774932879, 717200650, 2047157612, 1160042696}
,{1481560948, 739364078, 1620632552, 1819198037, 707048498, 1079811205, 1779569542, 1411097062, 427912891}
,{1207550056, 1098410139, 709782626, 204999653, 1778047425, 246270890, 1697329638, 1543669562, 1452511443}
,{642969438, 1575001814, 1554538832, 1849529612, 830399684, 1135214146, 1801458975, 682148059, 484273681}
,{1236753582, 1062730775, 1760956479, 793806717, 802160764, 844615386, 2109586188, 914445010, 1555846834}
,{166547920, 422013447, 1431970870, 1548092177, 1265831018, 363721675, 767797372, 420369638, 903953280}
,{425464415, 1457152098, 128663169, 1333156753, 1503451654, 771368641, 2113541974, 2096655615, 232751277}
,{1062490814, 695388628, 2012720170, 151630697, 326004380, 1826755396, 943859052, 1109770217, 1762945904}
,{2135563997, 709300399, 659452132, 873671779, 556245077, 1787214100, 33927197, 539674713, 1763223298}
,{80787050, 802729382, 1184356895, 2115722390, 196314828, 1848195738, 322740022, 70028100, 171804993}
,{236697177, 109863136, 412056265, 63614693, 597772784, 1852380224, 193219916, 1713974640, 2145365307}
,{1398791973, 1019171607, 1650290676, 310006283, 358382039, 1925458787, 1727999377, 1530567601, 2077541208}
,{416015238, 1815539106, 575510147, 1794310113, 2143451005, 1455185408, 925188713, 718874787, 495980459}
,{932153929, 1525500991, 1560165771, 146442693, 1552419555, 18131110, 480078059, 2018524837, 523813315}
,{936688562, 1799520590, 1107357327, 846356525, 1042453794, 1686427958, 302359177, 418408266, 1540255995}
,{171557034, 410683462, 1907207375, 219381586, 376204941, 1853877053, 506731130, 1115767514, 1488924864}
,{78001352, 633322192, 1037081321, 2031692799, 1147245299, 1894511907, 1125621430, 563983475, 412980171}
,{1355960228, 2102076481, 1207482718, 844346939, 339310211, 339000213, 337213367, 1477941537, 1647391773}
,{299509334, 728979327, 73453302, 1827113258, 409158556, 2127389613, 724731308, 284967112, 1597910255}
,{1748228553, 449350868, 1647701692, 1621495881, 302778226, 1233335245, 1752905268, 1866972496, 1367324770}
,{2145031417, 1988395726, 584368490, 95473289, 396557445, 915512183, 586061773, 1286216039, 1986491296}
,{2048128873, 135238167, 463244662, 481727264, 1616502189, 814257432, 1466744749, 934734243, 1611467751}
,{1672025356, 1552431560, 1269866221, 819830565, 1219462304, 537725956, 652165407, 1343191949, 1715712763}
,{396713163, 2117808252, 1425759638, 1151027638, 1464864027, 153597545, 1908871409, 1449290286, 1601857521}
,{226001914, 1412172121, 342032278, 753566173, 1841880084, 688826357, 1926304153, 1475224090, 251809829}
,{30212810, 617459054, 860804773, 1040392410, 1890581446, 1680626394, 1775824088, 1277847438, 1228586478}
,{652875686, 1241101472, 1815768248, 181502961, 522402409, 1656840076, 806377492, 1187761027, 858305326}
,{853558208, 258737431, 1821966692, 1021694689, 1271528064, 818985517, 1213199214, 2100313517, 214295786}
,{924476164, 1948539335, 1461324675, 573349687, 232478488, 2054469407, 1493879659, 2090601271, 681884049}
,{1065927734, 69066928, 2017552858, 52391754, 1402121196, 1199230774, 953576993, 1114504177, 689922187}
,{2080468886, 525162191, 1708911861, 487478353, 919211453, 441243175, 1017605838, 2064051592, 1485283584}
,{2118800469, 1971178528, 1042559909, 703594553, 1958730154, 48482358, 810842719, 312425419, 1742007075}
,{1481566343, 732137456, 1676176488, 1979597632, 1908453975, 535711783, 1810799911, 379450022, 1990394832}
,{2084346891, 1197829255, 1868594919, 1321214235, 1001788473, 1649423849, 110180566, 1926896131, 1508490833}
,{1469457653, 758145398, 2095248141, 1566705031, 1115450901, 1639699590, 447611461, 1517415202, 799103945}
,{868081575, 1130812981, 1160496838, 201504703, 106820467, 290113518, 844208742, 120780008, 923540923}
,{899218883, 1728795737, 1688519828, 770700100, 1888283309, 1736191675, 543981830, 1654048283, 1631488957}
,{533833139, 572633620, 1492278787, 809612847, 1421109344, 278417839, 1287458361, 466376603, 1580000080}
,{545570547, 231035737, 556853125, 1946432295, 66817305, 791705322, 494504627, 377491305, 471566260}
,{1749451652, 1012373070, 217170871, 817474552, 1271746647, 1688555180, 1773660205, 733564392, 156005651}
,{1221827446, 1485350762, 1546978682, 1611539162, 425465498, 1701956250, 2025035818, 688028261, 292029789}
,{1354255778, 601725722, 171970271, 1946712708, 586109677, 2060471482, 1813037264, 1741646395, 1587146174}
,{138488776, 1007443591, 1553028013, 411031345, 1566033713, 529475790, 293969663, 630264510, 271699311}
,{408421952, 599280178, 321591128, 1257791427, 1326733186, 1744822683, 33025736, 816735690, 2114617518}
,{334766756, 87274498, 1212215085, 291530924, 125057531, 1688093051, 1802498523, 2130262635, 990078344}
,{294897963, 1491888644, 272139730, 660751090, 1072597790, 1710796904, 102857543, 946873497, 419069519}
,{105842703, 2006660239, 1249426153, 1307288776, 848024680, 144254193, 447317909, 2117095149, 1073409019}
,{1726804295, 1802106356, 558991994, 758931625, 332327964, 660853941, 435067297, 284535524, 662237260}
,{487399339, 2050522543, 1407174245, 1895572680, 835492554, 1772558592, 2075268926, 1813473650, 1274518600}
,{1219515172, 638640061, 2073658948, 1343370500, 177224780, 1500299939, 1441179787, 1590576851, 801638888}
,{1751825004, 1990375726, 1990869541, 945208254, 376578560, 1806733704, 1788009474, 1822643491, 919769344}
,{213136811, 1590258726, 163448994, 151190755, 553633534, 549293201, 78570556, 1841677978, 1841781524}
,{1338055127, 1461876497, 2050277175, 37898263, 1997423171, 1745176479, 1332091225, 108537246, 456196582}
,{409291015, 153960984, 1143162188, 1818063991, 1338194019, 1621321864, 1005196265, 2003116210, 1710845169}
,{1914239813, 1193361616, 158438786, 1892909212, 903956322, 1919242052, 775194256, 1192094493, 1882587620}
,{425368906, 1839156397, 891179040, 557166877, 1521492134, 187038727, 1577334762, 219848075, 907674396}
,{206150651, 1057538916, 1689475159, 1289838812, 472196022, 19018547, 286954297, 867282261, 276021413}
,{1342036273, 1129207523, 199329127, 916884194, 1531082643, 2055359198, 1412688690, 1496184976, 577921172}
,{1908049327, 62051406, 2090741658, 1086935780, 813230892, 145853418, 1824944597, 126082624, 1198598703}
,{1038536063, 1779889323, 1761043209, 1504575564, 1439022374, 1738524248, 2011553181, 907906133, 369635951}
,{2114446120, 759803948, 905372158, 245011845, 1618358019, 19747806, 1542705520, 1852548560, 969966023}
,{732329926, 757937125, 2066936269, 1018598386, 844743928, 1145866745, 55270173, 247634549, 1724439477}
,{1482972484, 1535724968, 2052042137, 426321187, 1419128503, 1446181328, 257388484, 765731947, 1902741598}
,{1398330699, 1076966975, 942750550, 1781806978, 490492792, 480177048, 1633682042, 1424766628, 850817931}
,{1763550760, 2062033935, 1099851829, 407801075, 2136125816, 1444791169, 1560542561, 2116469008, 106871475}
,{838417096, 1391594188, 1192657137, 491578305, 1869837505, 668651475, 421758209, 1310330573, 659602528}
,{1085411, 2053027096, 910334900, 971119977, 799074360, 1086279561, 637511236, 318494315, 150730438}
,{1018057676, 155091718, 838459032, 1917911213, 2079584278, 1516758449, 1273961216, 1042528058, 1499951093}
,{1632179241, 214999102, 1643501908, 1935241004, 1353861866, 1898594197, 656871392, 846342558, 842893606}
,{657395709, 1158522828, 61205592, 1090322203, 1541619216, 1894801971, 579140908, 1746616344, 1359800225}
,{657647475, 633022745, 1064980167, 279621675, 736434330, 2013458864, 789766294, 1506442025, 746050533}
,{2063664129, 981616406, 853745909, 792182560, 1921440959, 1225701602, 639498713, 1964222381, 1341337308}
,{161035245, 828825708, 667015830, 5962827, 810379649, 1309334284, 638245560, 1930060528, 993851923}
,{170050672, 1773183209, 1775036452, 1210728658, 1798932405, 854672015, 939030335, 345197129, 496997404}
,{824211981, 1528722463, 479124871, 1043819297, 1592724428, 1347622452, 1295947201, 697611959, 1241186060}
,{1638192236, 1581777915, 1877899994, 1890088154, 716715288, 578913903, 1997402675, 654126306, 1132016175}
,{184748183, 44983240, 774478493, 439281541, 1798226246, 302443923, 990745667, 844139834, 858532210}
,{1619224420, 1846165262, 762302013, 1127059166, 2111796024, 1279436715, 1925111898, 238521637, 1706190904}
,{684016305, 1016936260, 293253039, 1568656614, 691614780, 1314401465, 370083438, 1224379046, 907327208}
,{2008735816, 1274823826, 398198586, 2102893573, 2084227966, 142269561, 455778422, 10635843, 255144321}
,{896512862, 1371130006, 1075683462, 161347179, 1056490306, 1272098869, 1596816499, 1642570869, 1447958143}
,{358700755, 193244922, 882501382, 259791622, 431131737, 1678312393, 1690956415, 1278569612, 1272122561}
,{983877854, 1105564924, 1811547721, 458482734, 1240709155, 116962581, 1877985000, 1948671631, 1739790090}
,{1515247795, 1547987137, 1171584556, 1140774859, 1353659514, 480779497, 681189185, 1439229556, 420432440}
,{389407623, 216755886, 1577014842, 304945845, 287187374, 402585765, 1972980736, 1332389505, 357721959}
,{1457993497, 711595233, 1137138925, 234616447, 319581644, 2026658609, 1394176053, 1142519199, 1959809389}
,{1832412452, 1970859433, 214819669, 307013693, 199714796, 470521616, 1768124021, 1943028115, 1130267691}
,{1159423080, 1700155673, 854435725, 1202436499, 465251655, 1294357595, 2012023302, 712976708, 1074703266}
,{1503971342, 1729005776, 260437798, 230124223, 1612951800, 1417364171, 945476344, 328704232, 638911860}
,{1689707333, 1293019396, 763484704, 1822417118, 2137882100, 2097486200, 29364534, 1167894437, 1406996500}
,{1384409974, 624568535, 351122954, 1572323597, 767942707, 1643263597, 1601341795, 474983057, 279576090}
,{1942336268, 1039971834, 817394467, 1169552181, 106908209, 1219147599, 283318625, 1607261574, 1047605211}
,{1253964126, 1841612748, 1433876001, 2028651582, 880944227, 2046970398, 502687997, 1530588236, 2063225262}
,{1214647686, 846997726, 825471574, 82841715, 711375817, 1066116130, 1526072752, 837206440, 418353563}
,{2013648589, 1460326455, 1971405043, 1379074360, 1332164537, 1721917090, 294992238, 1061078712, 101793529}
,{1908256449, 668460584, 2136150745, 1789902572, 881722849, 966938468, 509232093, 860052063, 902827174}
,{552121189, 1787938648, 1172837317, 1724581555, 1398350261, 413580226, 1672329332, 280272830, 226512947}
,{1479969071, 2139725635, 942440704, 1718049489, 1683153637, 655368819, 605687435, 969187975, 807066934}
,{1557268610, 1832320981, 539761321, 1575334369, 1538468572, 213535144, 894520750, 188510283, 1794612520}
,{2079844769, 554450196, 1609608275, 1959189682, 541579934, 792872168, 43612828, 655435004, 741674412}
,{1121795453, 1849720166, 583260021, 959752358, 1571202418, 705472481, 443718835, 267579854, 833304375}
,{1485547472, 1805916081, 1459194222, 446363093, 2090540124, 426981007, 2061370146, 369128636, 1531848372}
,{1944776229, 994631589, 788946773, 299719373, 1007607257, 1280327550, 1414573954, 1307857042, 764605657}
,{1427140809, 1426854223, 1144428634, 257784898, 402902332, 1893722581, 588999913, 1447499299, 1936387042}
,{764494856, 1984477791, 1017864413, 267572526, 833534348, 1287407862, 782020026, 638110611, 1791311640}
,{1664456003, 866916741, 414783290, 1547034914, 1442051899, 606479687, 954134676, 783591048, 2026788491}
,{1516657960, 693142882, 582422663, 1604504620, 1452789002, 162312596, 96116525, 2107734748, 1574075299}
,{141618298, 279199877, 51456606, 1400615672, 170278318, 14885108, 905240277, 761659028, 762691117}
,{1038327399, 371996024, 1143116903, 757628447, 1962344271, 2048690899, 318700906, 1290328224, 1940226122}
,{905773289, 1202637985, 1697430101, 1041859240, 2103783480, 1851984975, 512364448, 1721525142, 1715998045}
,{1989132506, 1446747261, 1171070210, 393388575, 1596299869, 1394795978, 468143253, 1625402807, 1779028163}
,{790891442, 1243478696, 1347652009, 939158092, 1971277425, 1181415056, 504660010, 1836268912, 469589847}
,{478207456, 1242668667, 354946953, 1957562925, 1317227760, 645799328, 987910647, 215929828, 2100645779}
,{1783701298, 1786946163, 1928418690, 1622375577, 1403131170, 947902344, 2026149599, 805849035, 584213096}
,{2090722057, 807742894, 2033434971, 1017507251, 1968961982, 1901351274, 723716931, 1945322331, 321731525}
,{1156255403, 697066471, 957657724, 986046062, 1604025970, 614499627, 1494202131, 1644191322, 1243307372}
,{24415550, 1912515670, 1620318675, 1947389834, 1293133985, 830228404, 1759235463, 1653779969, 1680678637}
,{873916901, 367105889, 661706073, 1483261173, 1215768936, 168609156, 173196139, 1315268342, 970173381}
,{799313445, 982562897, 1107613956, 2013836073, 776449124, 1239674103, 995850567, 269424004, 1111647452}
,{566204686, 1447551346, 1237270386, 756092603, 1052436672, 1893624800, 574501706, 1199262100, 632694937}
,{887136480, 898139704, 38629669, 1519532409, 331980814, 587641936, 1550941909, 1943070382, 2014138192}
,{1170627715, 1840914862, 743413046, 301653047, 427860334, 894324250, 1504141382, 2137435575, 1290370618}
,{1463320829, 2009463259, 1697123918, 1175228484, 103225561, 316184963, 1908581728, 1366218338, 1557784425}
,{672241311, 779415481, 2030824335, 2048250718, 702053575, 1725247028, 138497687, 603912157, 168561773}
,{1812147094, 584285553, 1595476825, 2009589887, 1978581843, 1453272623, 1408953954, 872210909, 492192019}
,{451449159, 2046327920, 584844818, 1371323150, 1943205526, 1780946532, 1250003720, 933821848, 327877691}
,{1667883701, 1132066908, 1794068152, 2019057017, 2118364838, 655681795, 1027708612, 800365544, 120249980}
,{134397566, 1321796762, 1884440796, 1613555205, 1018071272, 1919002708, 2071783307, 1640050324, 873550388}
,{2098873413, 560425954, 437918978, 1376032210, 963457437, 1221684630, 1084071181, 1103981479, 469689737}
,{862309383, 1725273372, 852043221, 1808081428, 596923494, 1745587635, 1851700284, 1589483058, 1973339706}
,{1070983352, 1066048083, 1772158549, 1971909992, 1369969081, 1979694098, 864517250, 595587131, 195145944}
,{1916371590, 385962573, 994066771, 1291943977, 835352104, 788974340, 717925752, 181676505, 1308636239}
,{1622322282, 676367482, 248750426, 1877105722, 757724400, 323303048, 994587818, 1314251621, 405110515}
,{628637869, 1359462453, 1054824880, 627240855, 2035435458, 2105806840, 317792486, 1160258579, 1521970773}
,{158720130, 692411897, 1689856749, 1578616607, 1392567356, 1710601675, 554275251, 418252628, 2105172395}
,{101055087, 2089239259, 728172972, 789309869, 186978161, 658005608, 1356864468, 1173400764, 205784623}
,{1259610744, 1351152006, 332828347, 366828892, 223779042, 1339784975, 1474516341, 2105606002, 1849048953}
,{396193657, 1807696619, 561265769, 1533694003, 1150980119, 1346906292, 1378338755, 514404527, 1722557811}
,{1306285891, 1019257088, 989688090, 790037912, 330762267, 1743173032, 1668067717, 2031575311, 800826914}
,{2022402336, 1354132757, 1559971733, 85386999, 1055434838, 216795210, 410351586, 586009038, 635583310}
,{1419768867, 435785735, 579421408, 450529151, 1127492244, 690485908, 1196744799, 2071777936, 2120356884}
,{677942381, 1924491943, 655384088, 1805035377, 2016393591, 1794586292, 1315769074, 700271107, 375434333}
,{535020863, 1636247256, 1588887059, 284050765, 922902605, 1771112501, 1076371866, 551565706, 1374099702}
,{1407616408, 344623617, 1697625483, 449354057, 2086433182, 710951333, 697173240, 205343423, 189201884}
,{1462371999, 1703222327, 1207326708, 248655720, 1586526919, 1084017945, 1364791339, 1811088216, 523149314}
,{1827760757, 1279126920, 827596300, 2058016199, 1801688977, 317288108, 1575538921, 1613951759, 1129431679}
,{982619232, 601624326, 1378535521, 695121907, 681801971, 1546197783, 1159488844, 718797541, 537597014}
,{1414595979, 510639857, 531712470, 1524698519, 1871705712, 2055640729, 778605856, 822689740, 1583764303}
,{64063490, 2064213911, 66811450, 30070950, 287786723, 968049866, 372239091, 1080903274, 1412709130}
,{125223970, 1657114878, 1635862367, 304507517, 723300102, 539544665, 1983892646, 254390185, 1171134449}
,{1704424368, 113535336, 2016220467, 1125208064, 603828228, 375980266, 381388821, 1099096159, 111770390}
,{1119913715, 1163455458, 744347299, 263922829, 832135240, 234614746, 1306477232, 1095145986, 1093079239}
,{229933607, 134375554, 739598441, 1701798316, 360896614, 805518068, 1316660576, 811470385, 613752891}
,{884007041, 1328596665, 357672085, 489932172, 1763818813, 1126568383, 285495698, 175260313, 950858427}
,{1937763234, 1833986293, 42663906, 1036060660, 800887878, 511524067, 615249759, 713349062, 765964071}
,{493923492, 155671779, 515202321, 508065838, 167612663, 1271288745, 607122172, 1584063266, 257435614}
,{1453850820, 810776756, 1198911285, 1937619653, 1674840926, 301928969, 2059603787, 1968377604, 1415405494}
,{603550263, 1826190330, 853437143, 1343273899, 1092607449, 1937070285, 712333455, 557086278, 682878887}
,{874285960, 561817353, 1545132470, 1735573306, 392999075, 255817267, 59732056, 1285170357, 1768550550}
,{1817466758, 762739157, 757549889, 13947675, 1613102658, 2059361074, 1540012066, 119188546, 68148114}
,{49441206, 1783893261, 1641237503, 562523782, 1922219888, 263350286, 45826350, 1620226383, 1677263201}
,{2066876865, 1937439456, 467425335, 219110253, 1879416819, 1750903465, 391480034, 1585330794, 680305947}
,{1138462770, 330892636, 954607752, 233301757, 688767833, 1006172490, 1676189204, 76132265, 1198992452}
,{2025665991, 1183210953, 792794777, 1094332631, 658846763, 1655343862, 428172790, 2018213978, 1885207153}
,{447836956, 1522015072, 1284168586, 1036521322, 1982074594, 935775408, 1815640652, 1689537282, 1605484910}
,{955762518, 1946791041, 789709610, 690502262, 2002908196, 1397525795, 1911188317, 1846947451, 390108947}
,{540438691, 917726590, 1635603487, 1254819961, 1195619626, 1909689054, 322967224, 1571278162, 860420634}
,{1875936266, 364129035, 1642238430, 1798313845, 1639714489, 351796436, 1089612948, 128459125, 2131624707}
,{1156178060, 286247000, 1642538317, 285409002, 1315947704, 2029068326, 1576609497, 1558956778, 555564467}
,{2129155606, 923456181, 43356433, 508705578, 1324319964, 822496813, 1151346919, 2145843621, 680648879}
,{1708174815, 1614356757, 526948438, 1360638710, 1377494285, 479624660, 359079807, 1986580054, 37946172}
,{469966405, 367657238, 665578393, 1219104812, 299246280, 740039908, 1265557884, 1205656282, 92014946}
} /* End of byte 16 */
,/* Byte 17 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{455019054, 1550225934, 132415801, 1617396107, 1976346353, 746724171, 1052178347, 2000752962, 1490831181}
,{1082808950, 713628854, 1281218388, 235656116, 464816746, 1408387545, 1325153659, 1811961800, 1978113763}
,{1396268210, 1406025640, 1794948036, 1961802270, 1196856078, 1991073590, 24330456, 1523789881, 770903802}
,{690608210, 1366648766, 739659085, 1902422778, 1283772595, 51194069, 1688108802, 1526333062, 964304388}
,{143554826, 1865044570, 1858738681, 1354692319, 1047214589, 311561828, 461201853, 441652394, 248391157}
,{1842439355, 75490519, 1398351733, 2133264321, 1803112379, 1041249480, 1033914415, 1234523364, 1800525931}
,{1598076393, 1013673947, 611129001, 1617921713, 1703235924, 1850211580, 2111038595, 342187293, 831851095}
,{1344467875, 568830291, 700025871, 1740909111, 1236425344, 1708879270, 1175659950, 255686305, 1683806049}
,{1178440401, 344159862, 498881820, 988717601, 1167576492, 39079904, 914700583, 990694944, 1250329991}
,{1744531456, 2052535526, 16156272, 112810127, 471327566, 1012452787, 413417073, 1363978522, 208683448}
,{870687656, 1722705725, 1420898305, 2017226429, 737270655, 988468316, 645437665, 180710511, 1855137982}
,{966483491, 2062934711, 1704269053, 79770930, 1334977196, 758297848, 150374618, 1959875096, 219524183}
,{2104059284, 1729244554, 1300994771, 1457265318, 2050803807, 1422320534, 158937490, 327895373, 1184421150}
,{897318534, 1313551760, 1092141947, 2040649989, 1127066701, 355245179, 860431952, 880505347, 1570509106}
,{46115545, 1013871755, 1113573323, 1244408497, 737780464, 1534602226, 1220707638, 602412459, 419561921}
,{646648513, 33299793, 1492269104, 1838463661, 1194902393, 1202837184, 1500772819, 1205818992, 2061355934}
,{1803832403, 1353531983, 2047876518, 1533488645, 2097674012, 1044313189, 501250896, 928554102, 1987428441}
,{2026473095, 2120007090, 1361173108, 1427448184, 85248009, 873870595, 518447488, 2104985022, 1710735263}
,{1607062800, 1214092048, 945222454, 972592529, 155577077, 651620068, 1576148889, 1387308059, 1710128721}
,{132931117, 942880140, 1438053719, 350723947, 1683693642, 574231702, 1451820988, 701580060, 1438140621}
,{1643531356, 860338899, 378788589, 438206729, 1097086871, 2003089842, 2039939156, 1442214942, 807060006}
,{648334242, 956713716, 733261650, 1493248999, 1820227431, 944545096, 2120657784, 1180515504, 1836543734}
,{745021447, 1047892810, 1578617517, 1472318866, 460158763, 1538754580, 1040053414, 464055077, 2123100586}
,{605888800, 1033515578, 1460597707, 1970313397, 667708529, 821454498, 958340268, 346404115, 9596177}
,{483882242, 1834695558, 822681119, 1886527752, 1773421849, 1880085138, 2097386603, 1474564988, 2116317562}
,{1194446144, 551690294, 1560025700, 2125754307, 145231743, 563130873, 611310618, 1697047226, 195510308}
,{861905665, 2019903392, 161982712, 671909913, 194760360, 107623674, 1627552979, 2142375835, 1055541553}
,{733870656, 1841414684, 88913700, 923246892, 1478356352, 1155063225, 221512483, 1167840070, 2143354151}
,{516706376, 472722695, 10530779, 1794277853, 287218646, 2009862381, 1561936775, 710729070, 1112205295}
,{166714962, 850425392, 778159155, 409379473, 137057811, 1006823520, 425770923, 582482197, 1758550142}
,{1678269028, 977145221, 1635965833, 1260509466, 770256933, 1248744520, 1198905251, 2022008634, 1565792464}
,{845990576, 270987682, 42915810, 1216492589, 1984441636, 1692877898, 1383143828, 1591385708, 138966450}
,{485873409, 1929244822, 509648955, 1367693233, 2064835539, 578073123, 629445909, 2113754806, 1969266451}
,{1772327659, 361667446, 1556520471, 1994068909, 1675974447, 1781462230, 2006750514, 647461135, 912492138}
,{1984621422, 1786932034, 272489576, 1603157324, 23979877, 1307062901, 2069875390, 1952442761, 1595482740}
,{962502760, 655500371, 563007920, 396188213, 232123468, 318624405, 988360290, 1124003935, 1449334826}
,{1737131451, 682766741, 1341308899, 1743836139, 1030783200, 1255886739, 794374358, 1251056749, 1559280979}
,{1089499105, 2032157061, 139918424, 984439543, 1462176078, 40219639, 18467368, 1836969423, 1141359327}
,{818802518, 102501801, 2104717305, 1409163981, 119606608, 1748656202, 1707435999, 704187199, 1913250553}
,{177365790, 1845081806, 1570857513, 1759652630, 1006579141, 709261956, 893119099, 1339686763, 859811321}
,{1964806367, 2125742743, 1353222502, 2003621744, 1019981742, 42196144, 1767950581, 1492213223, 819829661}
,{781826106, 1604458390, 134841192, 1861222482, 669856812, 1975064347, 195187947, 397042330, 1604633820}
,{1461174872, 96383350, 213651492, 886370250, 793623344, 873462333, 1303804046, 1074283928, 1276617428}
,{1445495600, 368681784, 645024505, 2074001294, 166032313, 2010241971, 1122555821, 1788480939, 1580173449}
,{589216705, 1390296076, 2030663648, 220307377, 1075317131, 1720941141, 1535708431, 863925432, 2061433418}
,{283147884, 392989471, 1651711808, 1721617197, 1537061622, 723384441, 1505901436, 765054965, 1806163296}
,{126437613, 1194727961, 1214016442, 921089, 1643487743, 398855520, 1074417010, 698616959, 1291597625}
,{1331773919, 446623959, 425468067, 1374691945, 542732605, 2020612552, 1006743862, 1237079958, 2042489902}
,{1870605352, 40728126, 1050498832, 1302850231, 560808009, 674506441, 921795760, 980221715, 1828343144}
,{309583315, 484228290, 255663038, 944081024, 1336090052, 1986486865, 97263547, 1661658059, 647135549}
,{1504027387, 1722223816, 1344396267, 379745345, 304573756, 1947433507, 1414413816, 1602687427, 495252433}
,{513860254, 1445078800, 1891885344, 1374383715, 22461234, 620982617, 1352418881, 1008411289, 635884924}
,{162036363, 1519745494, 59707100, 1887944797, 572871529, 1785155314, 938141293, 807976068, 1149073364}
,{1854768505, 1703687840, 50927162, 587003598, 1785513626, 1591389775, 1104553476, 1918396799, 1006700564}
,{1375436327, 1027454009, 1151881733, 1313663428, 1495101553, 1297670571, 1878813039, 548704682, 1517803279}
,{241985100, 2027144224, 1901142041, 903368388, 173092981, 1942449439, 1113910555, 2124112429, 396021272}
,{1926676787, 373665149, 1511979394, 255391382, 1429220972, 450947550, 280405928, 1016242766, 1717781098}
,{2004292277, 1589976465, 2027192470, 1031908472, 1847211815, 566351621, 1936343585, 891059129, 354225114}
,{1067920304, 427745124, 1627177880, 527865749, 1095656609, 483469747, 62099098, 291382700, 993672664}
,{458854315, 33992238, 1020932692, 1950211654, 1376545688, 1484005963, 823653152, 802939592, 236523372}
,{1630439267, 96883147, 1694400781, 419700071, 1483065302, 277848680, 1597153743, 182818808, 130588531}
,{1406937700, 572188395, 1129254942, 2115223808, 1994156880, 79760200, 1101338872, 870761744, 2118886999}
,{791352707, 92096718, 356706501, 1196418124, 992978901, 566742547, 909965886, 1522175158, 245469231}
,{81880653, 499254819, 1410225263, 20844280, 1099698171, 1866460961, 864338109, 684693583, 1182177964}
,{1125891749, 1534186165, 460449506, 43096372, 448442090, 2106491694, 1226415966, 258095878, 143360168}
,{881685480, 1468395237, 2090603072, 486183956, 2123362325, 1159805319, 1577297028, 793123455, 1308898204}
,{1290302516, 970102874, 1755047236, 2116266120, 1842968226, 89000264, 5910226, 610958988, 340642392}
,{1040915829, 2130146201, 1099342462, 1539052872, 1660905819, 584709655, 357823171, 447447121, 1611673509}
,{435379506, 788987806, 109735859, 98602886, 760550315, 1280850073, 1057302704, 158043669, 1671602672}
,{953476682, 1250439407, 849079070, 1270558835, 1881760111, 1131470933, 1848928614, 2126672373, 385805504}
,{934361875, 1453406880, 95235266, 561690873, 691618659, 589509312, 1177397195, 1760214590, 1149285376}
,{2017210424, 1945999386, 927208557, 1464296555, 1565340234, 1669472467, 1094512470, 7475367, 166859485}
,{968528997, 596257840, 103174453, 1500520340, 236281082, 2082633791, 555368632, 447147860, 79779331}
,{1271844547, 1806661056, 1120091294, 1020165374, 1800815282, 2113134741, 487868317, 527288916, 1186767432}
,{1678257484, 807670991, 436524463, 113573980, 451064551, 649777601, 316476485, 539678122, 476925573}
,{262644175, 294912828, 1167381289, 686180404, 510024298, 1813600102, 747369618, 209852673, 1832440233}
,{1666613310, 1780966513, 908017565, 527425294, 1034168918, 123910031, 2045765060, 1958891170, 1422298832}
,{765029539, 499443402, 548444132, 1663153546, 1789789496, 124308881, 612310206, 2093090068, 1916201431}
,{1529639084, 1890054744, 403278512, 458197755, 1761271763, 1784177794, 1282054837, 1834839662, 1333826246}
,{1191859918, 621985017, 1480665512, 1050151020, 1320625124, 2137569328, 1591509706, 1137367717, 600423876}
,{1505072291, 2026893458, 2075384689, 1954150016, 19669540, 533750515, 1351701097, 1158932085, 150914222}
,{693259703, 1263873427, 1247698486, 691868552, 1731101020, 1343355078, 562775844, 924393736, 978066483}
,{1816757012, 948184676, 1851303609, 482011106, 1210525086, 1600809910, 217278317, 83841148, 2040174143}
,{1645047632, 2046753956, 122634913, 269841355, 407475943, 611993826, 552682168, 935396055, 1690700059}
,{725354916, 630528082, 1441968408, 695745993, 1917319421, 2094640779, 601469953, 1881095880, 1258935863}
,{2088216714, 573810266, 270687423, 1120362802, 1913596916, 1346172969, 765894525, 1389170630, 353645372}
,{912332880, 1512376067, 1213073635, 2001023973, 292946116, 1193059559, 688128607, 364314590, 1239177833}
,{820281085, 522347982, 1929295712, 1984947603, 1111727443, 772120514, 1014808438, 1415016285, 1153342655}
,{1102415492, 1711016402, 1905130465, 967903662, 1998776202, 466177521, 1849342515, 1931246517, 1000717374}
,{1425535659, 751413869, 1530117135, 393205393, 1088223971, 145102206, 2123576457, 1475314733, 1967728185}
,{2144191257, 595382973, 375145827, 295096860, 1206728066, 1670277133, 1186413201, 2134710069, 1425947183}
,{1656416688, 453418931, 1333822203, 1469230017, 2023978926, 732666549, 2043756552, 2042405412, 450143573}
,{815409934, 2131042943, 1606261854, 1475473347, 686675466, 1201404372, 326776865, 1897972356, 1740372059}
,{4999581, 482807215, 187487100, 474862493, 1891007852, 588431544, 411032789, 1297368658, 1222180457}
,{854375266, 1524449813, 625918261, 954317526, 1728996114, 1869663564, 401032743, 1136078613, 579749323}
,{1122446409, 2054491729, 936921850, 1338650206, 671389925, 480519480, 768740032, 210597736, 168183988}
,{760077205, 202097296, 1523747143, 419426682, 1183504607, 472941186, 2002462822, 2080796706, 1099470854}
,{789887366, 2012294615, 1931853364, 1543360078, 575488264, 232595484, 132458027, 101135398, 325342510}
,{1284679545, 1031326895, 816644861, 130141487, 1083338748, 1118559966, 1069393967, 507472875, 368852330}
,{1227689013, 513022572, 1982708923, 1531707017, 292645402, 1479195788, 176500209, 288815938, 2124295106}
,{1164655994, 734138350, 817976809, 2026880190, 203430739, 1829747305, 714436164, 58840615, 1793449259}
,{1139411138, 1556376142, 674236251, 155587362, 722356695, 942833879, 236870211, 1253798794, 1786765670}
,{1876217809, 1904631906, 416183386, 222096417, 166052722, 66839667, 1144408779, 1861662809, 480838380}
,{919846125, 288778934, 469736415, 669960992, 1463848020, 503703702, 508188484, 921591752, 282297448}
,{1775459149, 231076394, 227598253, 1685223038, 32672934, 576033333, 1421850957, 865452656, 1447751422}
,{166083160, 646683605, 1503646502, 226004307, 1171144459, 588554499, 1190278245, 346277833, 298327941}
,{1700770044, 1150530321, 171345205, 552422098, 400042385, 984757535, 1437292803, 371492164, 699516857}
,{2133824528, 1219292745, 1179131236, 1458123482, 634623717, 1331447619, 605750387, 1997730123, 257540721}
,{929387190, 1358507774, 743745052, 1480089788, 1292267649, 2106964081, 113681806, 2084546250, 1636418285}
,{1079968138, 774203378, 1808237101, 1379093465, 801480502, 181466165, 134111208, 43664354, 141952884}
,{1948338563, 933225331, 452008393, 514200289, 1568530464, 573429381, 358682292, 1677151675, 444656466}
,{1452741606, 1554738967, 2058372446, 1738300480, 331949773, 1690524499, 1359314689, 81424333, 1699315627}
,{1188439451, 2119882686, 347220175, 681295900, 1380744218, 737561237, 1914434842, 225992070, 227905170}
,{1812311073, 587117151, 1307926284, 496661376, 752316256, 1348672646, 1480135952, 1267103981, 416204045}
,{1548127543, 1147121081, 691475661, 1045331519, 1153406860, 927398988, 1225879474, 2119512256, 2129564488}
,{1832440634, 473371977, 21668977, 1610226426, 830148651, 1335256308, 413322728, 177073283, 1090295549}
,{1831190577, 337963444, 1376151405, 1252599157, 2033599897, 1522407842, 1058057025, 1419150110, 463155535}
,{1700477244, 319748854, 1948230388, 1555208008, 1533629383, 154078548, 605812708, 54683372, 655496744}
,{1047591322, 1897213648, 1109130620, 531520655, 785676904, 1745305050, 1572015676, 1699302086, 835166268}
,{610832282, 1526829436, 1121876232, 243275984, 373883716, 1527786502, 801899321, 69985404, 778885554}
,{1595201545, 796148177, 1961202947, 696601310, 1996491873, 774698548, 401568267, 1307671693, 787187504}
,{111638491, 1891858793, 593080923, 1424076927, 293303774, 886417680, 282834735, 788213858, 1950164206}
,{1354675604, 711341975, 708263905, 1067188837, 105559620, 666702243, 797040667, 1539843880, 1714097909}
,{159571586, 2073823440, 629129170, 1717233859, 691087873, 514734832, 1737462167, 1860741377, 1865189491}
,{1197639407, 930741056, 1340727751, 854221253, 1092443755, 1263186540, 898612462, 754455256, 398887686}
,{2100710244, 1877904796, 463992368, 520793128, 98912985, 1572152940, 2061222486, 443610352, 1232139550}
,{274203357, 1720901803, 342197995, 161220553, 1885925704, 1302808081, 2141072996, 1129831922, 1705232896}
,{1478452635, 495549876, 1836602187, 330268632, 122357989, 1588704189, 99439715, 1906077467, 206850152}
,{541095524, 51594486, 1719338381, 400788586, 1753550412, 291707782, 1831989022, 2121824440, 1010911858}
,{148200136, 1435245873, 914298010, 541536161, 1111674880, 953879096, 150943597, 166583355, 516499794}
,{713735343, 557043401, 363698625, 814584918, 554850779, 629358578, 2076158405, 624800388, 1195164102}
,{1378989887, 901985411, 635377771, 387313430, 901272477, 1170958227, 977161668, 904300305, 1121638282}
,{1677226629, 1907894925, 1309491142, 2093964069, 2078842925, 139749599, 1153204600, 1690342459, 1379848882}
,{1693553067, 927649606, 1637215561, 786967198, 811694213, 1086525309, 75815697, 438309157, 336163461}
,{1474140775, 875431688, 1213183560, 137564676, 1795049360, 1970095152, 1812140445, 40186432, 921990650}
,{168498856, 1494280123, 110590120, 1444635801, 1989997273, 1005808794, 1261795595, 466136431, 860481881}
,{2127395594, 1022425557, 267547083, 669128888, 1466217583, 1781075652, 932660793, 1738561981, 965095072}
,{960563546, 1201875630, 298775168, 941811940, 1790140999, 1509923483, 92424496, 479966637, 1300030129}
,{2103400601, 580614767, 1306864966, 1253206914, 636351683, 2099282386, 1216361225, 1364351397, 1984189794}
,{1336741240, 1237624979, 90871617, 1430792502, 828632619, 769765545, 358324588, 16888708, 1681715030}
,{1470816993, 1586050848, 24069577, 924832922, 715167851, 319645624, 341690338, 590492939, 401121140}
,{1526053920, 2080062954, 1442814557, 1919001453, 718288194, 1545944082, 1116482493, 776613304, 875970267}
,{2029864062, 486953310, 1112545692, 1446288170, 1862235788, 2138732900, 363989177, 996769411, 546998853}
,{1125522384, 1626289394, 989273223, 1212183979, 1278298763, 1661853289, 1373992213, 2127151075, 2144408622}
,{135605959, 466457951, 1961885214, 943253591, 1171237933, 311621683, 27456525, 1470514719, 549183599}
,{238569845, 648109808, 1471460680, 452097022, 938266087, 1101481274, 441697140, 297503023, 1156262576}
,{1650346294, 943541288, 2000135813, 1108708441, 891864793, 1869158638, 1131181925, 1249665679, 347107623}
,{1216392419, 1480293304, 930042839, 1036120115, 56776113, 947181775, 61658306, 1658784802, 1838433444}
,{1518281379, 1290466821, 246587912, 1034689893, 1699116179, 651694156, 1736481875, 2037533972, 126985830}
,{434030463, 37605211, 1859562721, 1477058467, 407820228, 207817096, 1357330008, 1860986423, 78597527}
,{2063098309, 1622990425, 16075399, 941410856, 1414412370, 1867273658, 1295264901, 595442323, 1528130104}
,{1820346898, 1861083307, 556707219, 873349886, 1274001768, 1009213528, 1062475943, 823605853, 234120443}
,{1907321536, 1344515488, 383250684, 738082123, 596724361, 247456221, 404015189, 1563044448, 908652263}
,{132333785, 758250454, 2040214114, 1060510255, 1949918511, 1248804541, 1710506707, 1213452184, 1954706976}
,{736957332, 632801945, 52274138, 75566328, 117535002, 1663357067, 221718441, 105339056, 1798653250}
,{1235405022, 581164043, 1523742143, 104122870, 659920833, 363520650, 322768092, 1279644226, 856290448}
,{550810506, 1407915444, 884775631, 892459133, 1129443824, 1859376861, 341367912, 2061470552, 1696419319}
,{895150316, 1333164521, 142480115, 330572160, 1136504564, 590365181, 43226484, 744939620, 875775697}
,{537215950, 1029417973, 1780216713, 2106304622, 1997729153, 1915134214, 465674644, 225331579, 1811488918}
,{97811947, 1268414865, 397260784, 1559076028, 1765488733, 1985799960, 1849714445, 1448546884, 1687994053}
,{487408230, 246543501, 2113065871, 1560845768, 1842664969, 1149692444, 2140170245, 1348657094, 726389892}
,{1754518217, 1797288875, 758951964, 368439755, 1639593043, 861084952, 1746062419, 1797427373, 258874186}
,{1805123059, 1924867342, 1674774440, 1857729698, 2084378634, 1185700089, 78663403, 4836407, 686821521}
,{696110563, 1453648999, 1299290326, 1979553720, 1309134697, 1532132849, 1729411470, 300517882, 1418701803}
,{635183300, 1797907543, 1254754509, 1171311760, 203885877, 1575593388, 1562290282, 839258526, 554390487}
,{1449833143, 510798627, 1549188418, 46168978, 478309757, 325479574, 791025473, 1165174433, 1466363076}
,{1251363620, 746444116, 29018775, 1853971792, 2040512109, 308466849, 887423019, 1435999553, 786292989}
,{25011648, 1824584871, 830292627, 1028263794, 910636691, 527340606, 1289968509, 1543142457, 48414284}
,{669951086, 1646488566, 464806405, 1968772052, 1940829468, 2056250780, 1892602279, 1198987072, 662190534}
,{973774752, 1072402006, 606820925, 805929175, 1609048493, 2020071031, 1080912070, 1501351774, 439330278}
,{1803475324, 1531521329, 134018229, 321215915, 646323775, 1329801695, 1279200371, 1054611791, 1458325654}
,{198026815, 5259024, 2007218638, 1895796324, 687040846, 1546562374, 821676713, 856590198, 172485912}
,{808167690, 715064347, 2095086494, 791827315, 1696886265, 1505947479, 1460630694, 1783845125, 1609411383}
,{962899958, 1671961540, 371173022, 1773574411, 1312064111, 1120198190, 1052845479, 1297704311, 2038983608}
,{985267819, 964527032, 2111625143, 1321411270, 363563541, 512309113, 1180697677, 586044536, 1876434871}
,{1042746176, 394789794, 2019032508, 1939343349, 587466193, 1747815550, 333555242, 2037347379, 1914504075}
,{591399592, 161561473, 1343200634, 1556466177, 614240970, 1243061804, 1337625732, 229303712, 1822474859}
,{381605808, 1784911299, 1260743297, 203478944, 653455785, 362859078, 918994255, 1492443690, 1204842317}
,{1399834854, 1627255836, 658462493, 819277093, 830633857, 493068543, 808011050, 1610215049, 1233426231}
,{1568531169, 411897839, 904625069, 1561032348, 925896023, 829279571, 1795493225, 1469480631, 1592693808}
,{1944347271, 1889379996, 873964594, 1135749687, 477037862, 242490810, 1862834634, 1986996118, 1714324503}
,{334000181, 853771883, 1787226818, 159970699, 1231176182, 844246002, 1706489082, 1924931532, 1845940334}
,{869048805, 1154054050, 1609699721, 1807235094, 1092053976, 1386757880, 1124334463, 1735840579, 377135591}
,{1043812189, 1945872313, 146758514, 417660095, 1314839594, 399422041, 114334982, 592391216, 678005939}
,{938206637, 1795274716, 2100345445, 1453958225, 1348110638, 1144779846, 196433946, 2029451438, 423054270}
,{1734875971, 1888800351, 867404862, 767645962, 1414913763, 315226120, 757075720, 502669460, 1238979452}
,{2071813165, 730938669, 208653793, 288814710, 1141960803, 542674973, 1414581923, 115833584, 1530915778}
,{1198231836, 607370941, 379501726, 1012274310, 633329469, 1905976716, 1430396793, 1907499803, 287827085}
,{1242687709, 897472054, 1937022714, 887147459, 2098097719, 1428898997, 1114785518, 282224163, 173958162}
,{1620056983, 486056991, 1790488433, 1697590218, 1943609752, 241159594, 1061837481, 1433321282, 1656695668}
,{174586388, 1655348984, 1184370198, 717634027, 1181694676, 305753560, 1077356452, 1629049457, 289404819}
,{1009015472, 1312430815, 1868281143, 1425896281, 1027359044, 100281941, 1940556646, 1390946549, 1506895393}
,{1709143011, 682104333, 1007552461, 1878428229, 1970184381, 1265020285, 1101666273, 741225757, 1388632819}
,{1525107794, 608021222, 1240427623, 1647055363, 1883821058, 1577472047, 1439931683, 1465201821, 1478374603}
,{596703976, 540452550, 1111289739, 407124249, 216277398, 2049593076, 2111919080, 1815436218, 1513610004}
,{841414369, 550154699, 1499140204, 2020636195, 955663676, 1222027311, 105063004, 265877226, 1506851741}
,{236723991, 1625312913, 1126984735, 931259831, 1020652857, 576091875, 388481667, 1546910524, 908730903}
,{1001919838, 60275013, 612504944, 328881702, 1747713617, 1642338352, 1326176709, 831087495, 919962082}
,{17515213, 422838074, 69265712, 1142616168, 1241921074, 1812572120, 1582349117, 266671430, 74844250}
,{1429539879, 988017477, 355796574, 2110095018, 716246163, 380304337, 2048051156, 963780384, 808419758}
,{2122589891, 449701947, 754427624, 892851404, 540770152, 887911491, 1624774608, 1183173750, 1200401163}
,{399127208, 1092988461, 579985049, 2113457630, 725368176, 1812287397, 1771045560, 1493591684, 632107816}
,{1680546656, 1726148208, 1382565887, 850090082, 708256489, 708460149, 2100972476, 1832736633, 1462663038}
,{5922570, 1505574308, 2107815, 266490098, 1750513681, 429759585, 388562872, 1418983281, 1361243715}
,{1797891968, 346333621, 1159992796, 2015277404, 473907355, 576064891, 2139486090, 192715449, 1372824589}
,{906922679, 1281834, 464731868, 1513975692, 1876731540, 1723968361, 2116200771, 1954421428, 2051677499}
,{239846480, 1269880014, 1333282880, 1862523938, 126307625, 1185933746, 1562451163, 1637788578, 1485816905}
,{252463858, 2112937600, 1701686170, 526934851, 806771531, 1809826885, 1605117672, 1253633854, 1871238235}
,{1014419097, 1816908969, 1991389619, 1068281593, 142999685, 1139739738, 1377732346, 1230320366, 356632651}
,{1879180594, 1373620707, 249074870, 1194680652, 1163352411, 71662405, 760863940, 928378273, 86800739}
,{1606650274, 683278714, 545004564, 640792429, 994108840, 1624189671, 1157654185, 772484376, 294242579}
,{1222397828, 1556293343, 585694207, 500145579, 40667624, 1987447562, 104835925, 1000323917, 1515915353}
,{982917335, 1445324331, 1002861103, 1955520248, 1739277283, 564432352, 2093799968, 2081683718, 795939617}
,{1628789111, 251356061, 2129104914, 1074239070, 972553208, 862395878, 1612517992, 1798347961, 450505604}
,{1100562795, 1166394798, 1535107641, 1148587265, 1129586607, 244326908, 161103035, 555330291, 2116337011}
,{2037210034, 712769297, 816044005, 1844321030, 1491324786, 1502984338, 75251122, 2032731502, 994617125}
,{441803471, 1203603604, 1852182679, 821245384, 256093899, 82604748, 534780829, 1600439735, 1306552727}
,{327968970, 677629272, 387979721, 664059401, 1145537290, 1636581956, 1533162954, 1630690920, 574795468}
,{2146641032, 667972048, 1481285509, 271723866, 1564576298, 1205137875, 522616341, 196182329, 314796477}
,{529784385, 497341898, 28267636, 1258447301, 565100001, 848788275, 1341804475, 251944823, 799041860}
,{901304267, 814951740, 676712376, 823479715, 523378227, 1251114523, 560944840, 627149021, 411250252}
,{318025619, 854371230, 1566014656, 562373992, 876940091, 635688081, 1450898010, 1128301512, 1770524750}
,{877054362, 340422831, 26759663, 305412431, 1086994971, 248202169, 906256545, 525093552, 44218769}
,{364773408, 1228275134, 987631528, 1578808327, 2046977139, 1959597900, 301772263, 1795310582, 1633366800}
,{1844353860, 1407063574, 454573134, 389743269, 1017584817, 761539723, 510156778, 2143923679, 159837347}
,{151005266, 40030963, 287262370, 1262028671, 727072134, 631424214, 1569502448, 1571696273, 1572842809}
,{2053756184, 33865073, 1539574757, 2076119070, 76346249, 1892030785, 1414234894, 936891763, 70556679}
,{326190601, 65675367, 295319651, 1456208804, 947088172, 58882839, 167336475, 860770126, 1290063722}
,{1198598768, 90261271, 279710849, 47351371, 2116343219, 529745161, 201674225, 513636304, 218583595}
,{141808684, 537218884, 784640970, 1957710455, 716642647, 917082574, 1966900622, 1484043602, 411975751}
,{263758467, 667046205, 1297477154, 1131952785, 187143743, 1326350018, 618506283, 1237598133, 1149848767}
,{1271921149, 344277931, 1612272674, 2109377717, 1804792980, 865473170, 235615241, 2134286565, 1594663594}
,{1568317571, 1144969552, 1578867066, 135153470, 1787102228, 1777174408, 1815849482, 1861838163, 1457581418}
,{1288455820, 856082873, 1320193479, 554813522, 1704794986, 405123336, 748903582, 507445633, 775569738}
,{758238951, 285208328, 1050743863, 425504871, 337979782, 1200940701, 737788800, 167278791, 1460548211}
,{1837876889, 1060036568, 922741856, 841158161, 1701730645, 1383147298, 223891937, 440673436, 1117998213}
,{594726370, 1013883797, 1341007149, 1262260860, 552465288, 1879471391, 1864308168, 1468746460, 38404027}
,{471766285, 1643881974, 1220571740, 1403617870, 304231006, 2015634686, 1159344694, 353602133, 1576334630}
,{406988149, 613910698, 1195762344, 1991526327, 2003710364, 1437832487, 1444140250, 412335416, 1429896239}
,{491868557, 1186467803, 1953445705, 1146180222, 744777951, 692191261, 1818552946, 1153931785, 877772738}
,{922597176, 108612594, 264354500, 1048876262, 1942269253, 1503690500, 1885546472, 1677425185, 1892041350}
,{1593247654, 382603336, 1240881788, 1552043258, 295931234, 1328205210, 1849751788, 1763623591, 859778983}
,{1609343321, 262208205, 126589786, 663523067, 1177341346, 1953795872, 731251419, 300210888, 1931260994}
,{1520598520, 665228507, 761019462, 333647365, 422685473, 133019025, 1251333715, 1506293300, 2042966465}
,{1097016528, 1623904342, 572864075, 1479807508, 664844565, 2098942016, 1556221287, 1686749334, 1577720851}
,{1269015458, 311202331, 941305246, 1529191619, 1983395326, 1799782203, 633504022, 1522213576, 1021123845}
,{1135535316, 2104985281, 2088047406, 536170909, 989928051, 1278051191, 674361848, 1159137721, 691604031}
,{356887695, 400562126, 1033935278, 1811052431, 1667525575, 1886264168, 1232736278, 1164846369, 78713340}
,{1409901248, 708972449, 943412118, 733485921, 1319677152, 1074500471, 1748452959, 14153203, 1636846078}
,{360301483, 274593691, 698099699, 852056241, 1985043186, 470610719, 426011765, 1239823035, 1014234212}
,{729032433, 375501753, 2079126435, 1161910273, 1416383542, 1516204772, 689461306, 1564843910, 1942429997}
,{147443586, 323604944, 270500844, 1638667279, 777560814, 916244783, 271937717, 1221366531, 1531198534}
,{132134403, 1515727784, 28341638, 1282863331, 1196460773, 1791603290, 475022310, 42863699, 1074084296}
,{1931547208, 1834286149, 1886066663, 1206940598, 1709375143, 2017238085, 1716904245, 1142895024, 1738858614}
,{1231401610, 1165608281, 225535545, 911252005, 1475396422, 1808359541, 1847467535, 1875251103, 1141181083}
} /* End of byte 17 */
,/* Byte 18 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{1753405884, 1769995508, 1751487212, 587978929, 268979410, 1585800178, 981708785, 781251766, 639786386}
,{2044682940, 1925017135, 1167540168, 242764772, 1195986832, 1209378106, 1388502925, 293659660, 1093205951}
,{90174143, 49121981, 1308219488, 590873747, 1198967477, 339683578, 974581871, 1430886098, 483408978}
,{464443298, 109588475, 1270927330, 731151190, 755270269, 1879768603, 935130866, 1048100481, 495809283}
,{1608982539, 1376684911, 870095503, 318512698, 1170221375, 706166840, 622937109, 1066852351, 1977939058}
,{1852660140, 81357140, 863997760, 794207386, 2080494457, 1580409738, 96105987, 1512189567, 155363738}
,{1606806871, 1285548667, 1764575808, 56721127, 1338621801, 1904282268, 1957070473, 767610832, 210964551}
,{457668442, 2089043212, 947195759, 1901595471, 191881782, 2143607622, 411681456, 1328010232, 1015568389}
,{1433955147, 1685894823, 1599250514, 1130008689, 1363112442, 1935629798, 707031720, 882547587, 1805840063}
,{1067937213, 618209737, 1344518697, 409308012, 1688638990, 1640296083, 1092537615, 1637532582, 1330250951}
,{164703860, 147086445, 1596297040, 1933021531, 896854398, 89440094, 920747913, 859140485, 1133026140}
,{1291345133, 1831607270, 920665965, 1866347688, 185108035, 628017197, 378737399, 1626415416, 1333849557}
,{484459995, 1215201814, 1413090899, 1265559751, 136534953, 508769904, 1664518162, 897726759, 1862440052}
,{750831838, 56105442, 117633728, 345487278, 340714242, 1726049581, 1768374204, 156053036, 2004082597}
,{277944505, 768383713, 1993056902, 2103970385, 1313572034, 1207974048, 281775797, 1683226309, 1690810467}
,{531587774, 1671831140, 392208509, 2078044769, 212459430, 841530325, 634543807, 460090066, 695673001}
,{636044649, 495206447, 267171055, 1026884893, 765192511, 1238958461, 2088393423, 618069742, 763616466}
,{382052543, 276947441, 541473254, 1840032781, 1539008437, 465351598, 163851830, 708942817, 968755904}
,{1517176482, 535968028, 975764284, 1639521728, 1062080369, 219704624, 1328364395, 637723278, 624128857}
,{1233585529, 1423255503, 445077056, 101462396, 666254403, 1957868209, 726190824, 276792337, 247254538}
,{1659334594, 1592570870, 1227653720, 183118528, 1722135184, 367558668, 1958001645, 34688864, 2139490811}
,{1857324397, 940642945, 1679506079, 524148561, 183617249, 2115940506, 1583171996, 1688025280, 1740292048}
,{1405601631, 1156848757, 692403527, 977589477, 1550305516, 1846004192, 1334792248, 1369405649, 2037381180}
,{637541051, 1914472393, 2038650752, 391575069, 2036312406, 724965384, 66925950, 1441892810, 457908099}
,{83264522, 1297911687, 597538937, 1855410074, 981647163, 1483517469, 1044675464, 1919270391, 1491018532}
,{1278029851, 1153372097, 156234847, 68468547, 599830840, 1887177655, 1106335403, 3373703, 920772591}
,{1239777941, 413831440, 1712531127, 114303244, 458992927, 87283343, 2004104504, 1989847044, 192043191}
,{1029774917, 1555476109, 194240364, 2007966507, 31415665, 872932341, 2143571874, 613471257, 1642061825}
,{1957027266, 1007092817, 1857775737, 1568047372, 1024287236, 966378368, 231053120, 1487178780, 503774663}
,{847767066, 1270395251, 1934370630, 1982987730, 2045300905, 1617921872, 509104516, 1375304420, 1563944358}
,{626507006, 260574830, 279289282, 951533488, 824232784, 685542472, 1327839605, 1023660538, 251203231}
,{1602810846, 1647703761, 563472650, 73985347, 72773775, 1730965059, 1997744344, 1951755203, 1859265648}
,{2122410562, 2131838485, 216708364, 1437419916, 1271125407, 1603743153, 1051262631, 103110354, 590791689}
,{897490748, 1056766174, 320740086, 1686283694, 1593477934, 1601637871, 941282837, 1034712154, 587083586}
,{927311022, 1155656641, 879939955, 1995673876, 1628337985, 1045846148, 1372887262, 1729911699, 915494708}
,{2058520682, 2014803609, 2098046911, 242233817, 1522311464, 1509069735, 1033001592, 1169173448, 1972094216}
,{1714219000, 1687683951, 1006852688, 319461976, 556687945, 1087707301, 733529315, 1673339451, 1152135143}
,{2129535743, 101004771, 558136913, 531196592, 1514101252, 1825273455, 1529903939, 2035839678, 586809000}
,{1214829751, 2096074784, 1944426662, 1727421090, 342931452, 704516730, 624103983, 1831107243, 24166464}
,{1778587597, 520987524, 1656653721, 2133026491, 989561702, 1723842965, 1590179595, 1039592721, 733078432}
,{806633354, 430084343, 389932910, 407859347, 1357068248, 1651001222, 26830847, 1150087827, 1706156785}
,{345086190, 937224198, 1290285867, 249322220, 81290853, 1890185855, 91849906, 861419847, 531413183}
,{653771688, 1372099649, 461439901, 1065283409, 243725297, 2002273017, 1586005445, 1047052415, 1487869687}
,{100665786, 1622701627, 732314954, 1587851258, 432890606, 84230049, 1669202078, 788882166, 801026942}
,{1318493959, 1913814356, 1762366808, 1715119442, 786282447, 1344967288, 3138069, 532905112, 1888255846}
,{736310788, 2102620438, 1588698673, 1634368997, 2136096799, 556421474, 749706063, 1694094422, 1757912116}
,{1599127011, 46577473, 1048770751, 978710010, 1370020551, 667999025, 1351842004, 935062038, 828000473}
,{807670171, 1094142179, 1608008550, 991990412, 198434374, 1992573625, 1447478693, 1881237240, 1687484987}
,{265464773, 566894555, 1182529601, 421324757, 1979928987, 1869846255, 2138696147, 424400382, 2046137218}
,{79233778, 737326225, 268618117, 1773273213, 1631571786, 494846038, 2028157286, 1707663257, 711767099}
,{404199915, 2059281779, 2081824292, 1341799973, 1811304001, 1380940897, 1276372431, 2076185628, 1943632490}
,{1456215895, 1031054119, 1030118313, 323348484, 238965148, 2131991291, 1465037367, 562204682, 256941874}
,{125600335, 1857101505, 353296118, 1616860542, 562859847, 1276270476, 1624588369, 2014622254, 1360980611}
,{2013080266, 308718285, 1735213640, 879847232, 1118210911, 248385286, 145040237, 506960602, 1155523065}
,{2043468827, 626148519, 1268740648, 687423790, 302865008, 662553706, 1681091945, 635912521, 187669808}
,{1062819161, 1513813348, 106496566, 567683021, 1489807225, 1249200824, 98773007, 1284731745, 1577022026}
,{1709479091, 1074093687, 353796926, 252736033, 511903574, 1656892011, 955000728, 777479237, 2065146941}
,{450445629, 1580422675, 1186512315, 1533752850, 101018809, 241650699, 994560388, 2044650173, 781255619}
,{217364360, 1544027822, 1049323016, 1354192882, 286674213, 1467950296, 106043683, 1639200140, 1860437811}
,{331764363, 814135352, 281829312, 1842620346, 74898550, 1020140372, 1213270370, 344233893, 1350281058}
,{891260147, 1518756119, 1679590822, 1583964072, 1406201744, 1958434344, 1423635531, 119323995, 827567434}
,{332717109, 1828007153, 202550184, 1855816772, 1026388795, 530440159, 2057005593, 384245538, 677154492}
,{1411266440, 1402779861, 372352181, 1299332238, 714993960, 908836607, 805878645, 1554145624, 600539721}
,{810800389, 1719915188, 780376533, 36861089, 1149436640, 255019046, 1335364961, 1824341182, 725665591}
,{1292843760, 635263803, 1797284436, 1387114617, 671079100, 182768487, 337451095, 732135822, 629579927}
,{1839825053, 46782759, 1667676507, 954094846, 257168483, 798708361, 1495502763, 752791426, 637936578}
,{328394033, 1885252162, 765297046, 1032782370, 792397607, 1018694974, 1911992358, 148836680, 740304769}
,{1163416241, 1742410785, 1439526530, 1047124015, 1631672393, 1442589283, 1256742828, 1466257484, 1547979003}
,{738407437, 491790506, 850386453, 776293565, 1929404548, 2058709003, 451960849, 1584474909, 193722324}
,{2060587073, 766875857, 601602823, 1555527104, 897956725, 1441534752, 1385555848, 1929576016, 1940592037}
,{1643546180, 2006713140, 2133290079, 144324825, 1499737465, 1431471977, 1310152340, 1833255153, 106478885}
,{2074763817, 1790344201, 773264172, 641740783, 679621493, 742054318, 298796796, 107665175, 105067770}
,{901365789, 1628630459, 389819717, 1713085921, 512904226, 132036427, 1095226244, 1759253465, 434036425}
,{173041388, 139211702, 479519508, 1950328798, 1445322607, 937640675, 1761958007, 2085262772, 2003970825}
,{1600444188, 1990629013, 17508298, 1550052211, 575493160, 141983359, 2007923587, 1598898232, 1304368967}
,{70430698, 129487528, 1753763480, 1572093111, 886133023, 2037572066, 1684858351, 16025047, 1632143533}
,{805469680, 760770495, 1674988839, 1511661887, 711592772, 478856107, 111524831, 506260555, 2145014313}
,{1730719692, 1689332384, 1913307014, 1223216846, 1699790622, 224587033, 1083082276, 414641320, 1554666160}
,{486985839, 1113958997, 1540178251, 569356404, 1983308391, 836139771, 281161690, 1544691636, 1013169589}
,{2029787859, 690751443, 563140957, 1576744897, 945129625, 2020435157, 1681349752, 129279025, 1436752878}
,{1575460876, 1113356932, 1315858019, 634677551, 698744569, 132448896, 840004176, 2058663007, 729831793}
,{611809301, 27893866, 1384039724, 2140490186, 1014329296, 2068618440, 660023270, 835740218, 507071855}
,{1766877320, 823542173, 1988651737, 932731464, 2126862703, 205135111, 950486807, 1351680358, 1439026213}
,{1837134490, 660637864, 708571879, 1750721362, 1581458742, 1091382616, 2044574881, 1142494336, 752088995}
,{326236199, 436508074, 2144917591, 690878884, 863094585, 214951222, 136949381, 750320783, 922604858}
,{370800030, 1982757427, 1176271055, 864510792, 2030730288, 779235593, 151919321, 918713212, 850187205}
,{1261133616, 564904069, 61509129, 1554133119, 2069294162, 1841494221, 1671843981, 1076361908, 404077306}
,{2074382005, 1583202064, 393743342, 1120629885, 1636169904, 1394039274, 1409068149, 1593823674, 434086524}
,{506143672, 1211782675, 461244034, 222153447, 484578579, 175638931, 1580886414, 1172081755, 1684962700}
,{1148497677, 601540323, 818184713, 341905762, 751127746, 2024927236, 1827447098, 1635034560, 178865650}
,{1096357131, 959996576, 1297673757, 1210549640, 1937530758, 1757387586, 1326833196, 316661292, 967705847}
,{378497362, 1677394894, 1076445271, 568026627, 1663847155, 1759669182, 675309422, 546416694, 828356218}
,{629152162, 2093411549, 1429945971, 1506487220, 1091014080, 2087956657, 377038594, 730278618, 620668199}
,{474840910, 1022916065, 113843446, 1939502976, 692595978, 1930463570, 511923020, 1760329919, 2116589971}
,{975598288, 145289375, 1109777004, 1754127867, 1381488587, 662823433, 126169526, 206729980, 1309133094}
,{744620263, 1063040426, 824291071, 729265736, 2002757533, 1124545881, 538074195, 4068856, 1571836704}
,{2027084896, 503856682, 1567277508, 947866019, 1202971471, 1512363886, 1682195242, 573846049, 1968492995}
,{1616250442, 1185815638, 1333416251, 462479640, 1729003775, 834876348, 1900202988, 656402053, 747643780}
,{661310878, 325390820, 1957939639, 1157043123, 224352768, 2026667295, 711735174, 263690718, 2064796299}
,{939930037, 185908257, 204032938, 842590057, 969318369, 1682293844, 44726822, 1041684034, 2107967510}
,{249227816, 1131389066, 1094558885, 1636454669, 1415300682, 614662601, 986801157, 968951717, 754045034}
,{97914932, 1037128838, 1824564969, 935767272, 33685082, 143865153, 1022592889, 1544227402, 1855437503}
,{1074975749, 290572089, 1438734652, 1132201277, 547083717, 880698218, 915741911, 1992807614, 1352839605}
,{250362038, 1381599928, 347404038, 1760420280, 1457434435, 1079224850, 1043417164, 1824721002, 1033530449}
,{1548929720, 1341348958, 1269963648, 1974608844, 42255324, 799223556, 560738674, 1134105214, 1148557052}
,{1171457408, 268102129, 377246356, 1709284230, 1038372491, 717085495, 1736879168, 1960369980, 352242998}
,{386050500, 626503280, 1365971188, 5133365, 368826076, 491385605, 1730857138, 1332068649, 868969690}
,{332719310, 1869428023, 386800100, 1377044646, 6747529, 679234441, 256442228, 2146283325, 1939902924}
,{614007450, 149410958, 2035349108, 504683433, 466694393, 1008250174, 1038216453, 1537172045, 465555089}
,{1653764691, 1768747111, 866044310, 664423155, 1747549850, 223561828, 1646791129, 1054413901, 1485879167}
,{1347642948, 1601333279, 1618886919, 814104041, 17539847, 1988651706, 1567103916, 1983604588, 182271700}
,{608801499, 1375994953, 611142325, 1515745863, 2141556097, 1055540246, 885948333, 454044670, 908898639}
,{502252022, 1081289833, 572924726, 1119033478, 232877938, 1120249990, 1254579053, 2075795180, 1653833272}
,{1000378956, 1036040998, 1709923213, 3070674, 780594122, 256411634, 975934047, 413113034, 2139243113}
,{1231126399, 987997749, 1152476231, 1947707674, 1643976254, 449729327, 1746690243, 1658488636, 634008502}
,{1939971967, 1015677977, 435519316, 655489574, 899983765, 1088307065, 591482755, 2059083345, 716175506}
,{2114113204, 329358989, 2077255715, 296275380, 1062927488, 18916097, 1748512534, 1187534546, 1291535491}
,{1204040145, 231343630, 370028692, 1931495457, 366496335, 346800491, 1429764526, 2075322967, 391752573}
,{572913749, 272333798, 1480076138, 923365800, 1116904318, 1038181627, 1868331845, 806320566, 1478937214}
,{515599240, 699295032, 1866172858, 2010349623, 531071864, 518835298, 1469832139, 1388534192, 1333575085}
,{1414951818, 1119432163, 280254861, 830050185, 337083195, 409450662, 351695494, 62711668, 1174127570}
,{1896916370, 2078540156, 1494437473, 1224011188, 1516768430, 340877603, 1697252859, 522598505, 1870729865}
,{1280881998, 1772476208, 1093296541, 1910030294, 944487479, 1109419144, 1134496570, 439214671, 1971878350}
,{505666293, 493793530, 718637096, 1328602835, 1753941362, 1712674698, 2051936234, 2113663706, 1658404159}
,{2143963153, 1722368743, 388411820, 1833517377, 171522777, 220362363, 633407540, 1611824738, 1540119401}
,{841368718, 1826356568, 1381227046, 2077013793, 845579160, 1317129152, 61238384, 337593441, 834795638}
,{1958176111, 1362066357, 2007883405, 1532329326, 1051664298, 1857820724, 1787142881, 1014203500, 696693073}
,{327485071, 1752504138, 1750876093, 1648594465, 494684725, 671516722, 1730593605, 1579757615, 1552382295}
,{2108223303, 270373764, 1234479717, 996178777, 1340309645, 2002101802, 748492451, 1915322425, 1206817708}
,{746482448, 2077034009, 1763426695, 1487108773, 507844300, 527274964, 2008377187, 1535796034, 1473554241}
,{488472089, 1941862098, 1417024028, 1244183253, 1037634427, 589780861, 1317974909, 1749259676, 2143380458}
,{406765810, 2071701225, 803903500, 1422935879, 97721901, 855591368, 1363439980, 945731886, 410960835}
,{1212220581, 396925483, 491312421, 1837839334, 813570300, 80022462, 678052924, 2030017540, 103064161}
,{919189322, 1919974499, 1500261053, 901379807, 1335737752, 1813006601, 973082925, 1097671695, 414681747}
,{1473056810, 1303681806, 1552317804, 1244544795, 35234342, 1861853746, 1544914087, 1455444141, 1780686722}
,{486674752, 2099521702, 828358042, 1962862416, 2044725826, 1070192080, 453790814, 1159790537, 1316676591}
,{1001264394, 465769113, 191439495, 276516820, 912494321, 1618565128, 483935159, 2147248979, 53582996}
,{1296893596, 1853165565, 1710364022, 1648285648, 1984652717, 969756077, 1048826848, 2140189710, 161844495}
,{1259382698, 196153504, 789701677, 1417093047, 1410484018, 1688104842, 413613347, 2023947652, 1990592528}
,{114523193, 92395130, 734181548, 935198428, 1797074484, 1448176383, 549442361, 850267784, 1928977489}
,{542857381, 124625111, 444023154, 750773438, 885672672, 41999210, 1503065254, 1727939152, 1569384157}
,{1319806064, 51984722, 1272432375, 1692046166, 1337759610, 19549329, 84474104, 2091976959, 498193492}
,{1662729394, 1954877312, 896117083, 1799680802, 1242801733, 728676930, 1714821981, 1306867291, 1966514187}
,{1639247113, 369454356, 1960386258, 1563192577, 174822087, 1191582237, 1221209928, 1994762561, 1731513020}
,{1920646753, 1962194403, 1786427868, 849801926, 639252726, 538348763, 1920128348, 1036750463, 598512611}
,{1042063854, 710509891, 1257134092, 1195775763, 411825246, 1849026095, 73104047, 1921808623, 61314775}
,{1163706368, 569383794, 1107663301, 1300766957, 298797601, 1021080024, 1179782022, 1102591614, 1416129089}
,{1664025949, 1699040400, 1605851209, 180953605, 2053870503, 2082998959, 769427391, 1092770024, 2100897724}
,{278256194, 700762957, 630170880, 884695403, 1687552755, 2106611842, 107605100, 847033618, 1176342624}
,{861751986, 965680951, 1941715713, 193129131, 1264519660, 1308312850, 410064392, 1338096782, 1010934151}
,{51267305, 2006102034, 998516084, 1149542172, 492793834, 377545742, 1081123439, 663774786, 904717753}
,{1172138283, 472684534, 2084483272, 747213655, 1661796494, 384558682, 17152451, 1922326303, 1311104835}
,{1095490839, 1025857426, 1634489002, 2101933518, 615521323, 535061450, 373475671, 1004849392, 1552314224}
,{68014517, 1168109190, 400426938, 603522928, 2042652431, 822181562, 327411756, 59432084, 303864284}
,{1898685754, 298052896, 1062528567, 2125282109, 544817969, 1136751435, 127794653, 1328934704, 509187552}
,{852270820, 95962237, 1724428165, 1236523939, 752970970, 1945151097, 721502734, 339752888, 1493050918}
,{28476022, 10636551, 2115109178, 416332136, 1264181727, 215418308, 751222582, 368152597, 813205771}
,{839676798, 793980418, 1815056195, 1506215502, 1980200197, 285345174, 1070940290, 57644577, 1637902048}
,{1198792905, 1989118555, 51276092, 1878511014, 97205667, 2106411950, 387773733, 991170931, 931411847}
,{850187531, 140069448, 1384165096, 521771042, 1068278298, 1023619818, 811149795, 1848481349, 1387677467}
,{1636003768, 645902786, 1876830052, 1873334687, 560788500, 932294926, 47288275, 1319912879, 2138928457}
,{1389402584, 1850595603, 1169522231, 781724372, 1865030230, 1793801428, 1400685190, 1365226379, 1279826815}
,{1594879766, 1468907571, 726176036, 805918607, 1215530003, 1554944747, 933564651, 1660286985, 1171300470}
,{192584802, 1949043919, 1593951598, 1547251604, 1439915602, 2096387603, 1182338484, 1562929864, 1322386879}
,{571947755, 359847083, 1312681763, 862880463, 482386975, 1770278104, 1122581156, 40480329, 1434586501}
,{221219697, 736399884, 196439139, 664457906, 62562530, 336850443, 400664031, 1705341501, 1720550834}
,{835876257, 1165507444, 541721440, 828237654, 1449535079, 550828207, 1100550844, 872541588, 2003138982}
,{1682825439, 1846584434, 2113322218, 2085558455, 528949803, 1150221348, 422254558, 1723020995, 1536505190}
,{470878312, 759880669, 663851205, 1434886468, 2134746201, 1883563448, 863597457, 810504811, 884701876}
,{1915746184, 204749535, 2053084661, 346712453, 732003819, 2111218747, 2065777376, 1341677289, 1228784409}
,{198618425, 281145144, 1874533762, 577329005, 1764594191, 748499607, 216694529, 1467853561, 1358219627}
,{1074688741, 652063001, 574999352, 1519928420, 1541359476, 383296124, 870342438, 1000560202, 1405911770}
,{1673185752, 1577335814, 761250701, 2063876977, 1054053528, 132727342, 929544496, 1309039674, 2099128195}
,{255959462, 1770564243, 333664229, 592001419, 709162929, 1895726796, 1388146768, 965143509, 1337730342}
,{1798450381, 1707124006, 1750607295, 2022109959, 1525326641, 1185040555, 798972189, 561353687, 2080472127}
,{2063354890, 602445287, 808259773, 1931593566, 1345639140, 954012899, 366981076, 1373653460, 864860429}
,{939950489, 128089993, 1826844900, 292863019, 1535260207, 765474963, 1881051210, 708236919, 1180003103}
,{1307730048, 1227511927, 1811558968, 703767570, 605175387, 1628799869, 60363008, 1541633976, 1783590413}
,{1970235810, 1001879748, 80970160, 1163419557, 1772646543, 1386039038, 1528591779, 521038252, 731289696}
,{1131168901, 1504125507, 1144184599, 164920781, 1516276491, 1215590574, 74482804, 568348529, 136238500}
,{1492002979, 1057755255, 1936900000, 326384279, 1737251926, 428180030, 713295166, 720976065, 2021781215}
,{359949084, 560723925, 261089093, 1420845913, 2093522714, 864238475, 1569218362, 1086367851, 746926325}
,{178620966, 2112523540, 1685681019, 617625631, 922052442, 546767649, 313196336, 78510184, 1324070607}
,{997662180, 881827819, 2047764294, 1729486786, 1534658897, 532646056, 1431841445, 1765213192, 57840821}
,{639669337, 162369276, 1845688860, 1583595909, 1897033393, 1763846108, 2087336181, 960402428, 272402355}
,{1216284354, 1356102994, 515488120, 1569747819, 104083624, 286158376, 2133319606, 1467331051, 771633762}
,{1060373286, 2118538359, 1147949396, 1443642278, 657898668, 244169156, 632993462, 1436842931, 1336119648}
,{1942773372, 668983248, 253129061, 1891385429, 1058055895, 1277502756, 1523990014, 713743830, 1881987378}
,{1872779044, 1720463451, 627255923, 330483987, 578705786, 1494487515, 1714798028, 589298136, 2090438356}
,{1319255264, 1188941230, 1364483006, 1988281180, 990820277, 1156698617, 637515943, 2087273147, 1011893952}
,{2145320968, 10520887, 1585526616, 1728008632, 2125157160, 118652007, 148134043, 1998284264, 1855254397}
,{315279014, 1428364007, 660131680, 1363643729, 1933425604, 673771258, 209563375, 753425636, 1240229902}
,{284485148, 989205550, 1931675388, 1895016022, 838987663, 1748629207, 2071274442, 1384818332, 1642791435}
,{1435251266, 1323143812, 759018009, 559129503, 1979625847, 1465050963, 568372719, 252824869, 710021137}
,{665679823, 1983670992, 342885111, 753898094, 2123937391, 2072805601, 1872260980, 98606751, 1123057387}
,{867446351, 593403812, 1468556035, 609169679, 2008848259, 1470913595, 416865365, 1000137713, 907180783}
,{223253621, 1348970463, 1637429505, 613854348, 354642186, 1378205878, 940309083, 606868716, 1860225053}
,{1740959866, 1457108073, 1756995992, 182483012, 1002440536, 1492017214, 590544873, 1371656677, 200238152}
,{1305927276, 758336455, 440687916, 569384987, 695226577, 2019248826, 378195921, 544390597, 182567321}
,{385506294, 1916800977, 1628385909, 19132242, 1618460360, 1137628680, 1230215871, 769672693, 597455641}
,{762344379, 340650251, 1671937891, 668414077, 1979369034, 2113939183, 1254181724, 658947203, 2070860060}
,{826834439, 574625490, 1368475816, 843023754, 223531709, 177859254, 661979983, 933453307, 648442328}
,{1763892279, 698310533, 1031072396, 424171116, 1628072207, 2121057700, 288702201, 430499528, 188907184}
,{1343448910, 1317856871, 1020736345, 709961332, 5679526, 1921391576, 2066236211, 1167894664, 26413123}
,{1195594260, 1626778990, 698080527, 1976089702, 1019389317, 1208320999, 167793995, 1380490892, 1824707693}
,{1735514033, 176556155, 2003383295, 1851376908, 1372294781, 964990614, 1021391486, 2138134982, 838695902}
,{335049882, 410295943, 1271019837, 2093631489, 1441864301, 507776849, 2059665581, 1480122718, 709072985}
,{617983549, 827143772, 1171800325, 1077177590, 1190573109, 1577217171, 1800890321, 554240160, 996589053}
,{1708592011, 2043325341, 457660647, 1825884934, 37913942, 478950496, 2008014891, 156712081, 90438896}
,{1527956587, 603604210, 1151194418, 602897841, 913489133, 820438681, 971415062, 477650934, 102066212}
,{1815961807, 1058724759, 2015810963, 1160487444, 1209469129, 1174757403, 1926768419, 378744315, 540037707}
,{999811959, 403262317, 301974079, 202899860, 649319450, 2093437596, 1287257838, 1685094899, 2099645700}
,{1394178014, 931494868, 1780765883, 706839024, 691227860, 1946771116, 669899263, 794496829, 1379521611}
,{1557539345, 918159551, 1234959904, 792432347, 2038313538, 909891692, 57504353, 20686547, 635208876}
,{829153233, 2012390922, 800730700, 1119747596, 2096954506, 970568014, 1964270448, 1291246398, 1430269902}
,{1632705714, 1114369226, 1773962493, 1092075732, 2018750592, 2046160779, 950432038, 544676154, 1183868844}
,{893526567, 1115145836, 1737300617, 2002168648, 2041512493, 1554030814, 549144738, 1595219625, 2020056536}
,{588257836, 856640117, 505597191, 772171983, 1854094074, 221253145, 965029502, 649160022, 1841636431}
,{1294405911, 1239476154, 1524767408, 1288692731, 1662269593, 1967550091, 2017240882, 797406787, 2134796494}
,{2109314548, 2006692866, 873391425, 1431558046, 408825329, 2127426233, 1524656614, 253683137, 2145397331}
,{1526774927, 718897603, 1024634012, 1892507245, 640240568, 936145655, 160112121, 1943842185, 496766715}
,{1117735840, 1767263501, 806750619, 852118579, 1495990295, 1376055957, 3008487, 1602684885, 1471921497}
,{2018329443, 1613177125, 1052895039, 241786167, 1542708512, 1228884367, 1374404787, 512363768, 74891005}
,{1090587196, 1769836808, 269337534, 316450007, 1348847261, 958634560, 2081299085, 124866239, 1197769895}
,{1741101978, 111561175, 1168656972, 240083116, 1936494521, 1434910765, 1203309116, 1536911843, 1720697923}
,{2079513246, 1379661789, 2066472203, 2125574517, 1043469669, 1296077247, 216980796, 1620134509, 1442790232}
,{1748258923, 43373580, 375223862, 1661785543, 1678554676, 1901089151, 890958665, 1886820922, 1232915718}
,{612102430, 1444107624, 1013236673, 1789836195, 624177347, 156129950, 2032333014, 434274546, 1116426350}
,{1248311709, 452406052, 1312079225, 762448458, 417385542, 981864595, 431555705, 1148274873, 1411353707}
,{658199818, 1887449895, 645766590, 879477987, 267071788, 1442331594, 1644189085, 840882719, 2012872301}
,{1188144361, 1480052455, 1944963495, 1771714952, 224700047, 1171576111, 568544215, 1035853565, 399022211}
,{1860794749, 1013901033, 369906048, 1380579908, 561596491, 1328990512, 1193249557, 640820016, 2055296403}
,{509555163, 1524210269, 39151243, 266935549, 2033612612, 1667988509, 421722906, 763577626, 263121890}
,{986181541, 1109702647, 1896964684, 891853196, 834656188, 871913290, 518053209, 1676902214, 1750978966}
,{397395638, 724524678, 811460998, 740432820, 160293368, 370687840, 681720254, 2005069284, 2035499495}
,{2026415083, 569947385, 795390814, 176782719, 697603319, 182824653, 2127266556, 1150226342, 509772464}
,{1390524040, 1999543890, 853452483, 688661341, 1932206410, 1512860476, 2041938234, 1477221512, 556185422}
,{943571039, 1660924794, 1493233529, 576872650, 1375324404, 1223300748, 1924071726, 547692640, 1183631980}
,{1518897251, 1467441261, 1305807262, 903230612, 621468653, 2116943229, 1531244773, 1763159312, 1292853082}
,{1517754389, 2038845972, 255477464, 473403636, 344756459, 326515930, 1214938712, 1336665275, 1487536348}
,{163523837, 1238876949, 1459952181, 1073812193, 1258673800, 859521059, 1051745628, 1564694682, 925779026}
,{1265576315, 1950609457, 2083218582, 1273392951, 1143185166, 970786835, 1206572278, 636344289, 1886832306}
,{1924187863, 651419096, 527850389, 520471711, 135425547, 300075891, 1603939833, 1354957140, 1808674195}
,{387896912, 1075805166, 1858713150, 1226353070, 313329847, 81047788, 541856236, 607206658, 2033542799}
,{913574281, 1650495991, 2077388179, 1733091804, 821265780, 1438119175, 1037175556, 940583630, 1565803333}
,{953410960, 1643848175, 1846607840, 595745872, 1463147358, 1310154339, 1280679639, 40133816, 1019603793}
,{457644923, 865430521, 1489415609, 1545215641, 582174112, 1027377345, 1666704450, 2035610907, 621329571}
,{1061847360, 1614447597, 441289755, 429178553, 2082226095, 1242801021, 113683580, 389389300, 1945939840}
,{1549924761, 1701629981, 1243375820, 1111785892, 24183902, 1266530497, 477967599, 837123050, 804932658}
,{1369318169, 524402505, 2095274523, 1735230598, 1113888714, 1011508787, 345382077, 1292224324, 1091106930}
,{1723003553, 1455654405, 959797375, 1167799649, 1659559597, 100983885, 1461323927, 1407955551, 200407513}
,{136963636, 1661367368, 119729455, 528578519, 134925923, 1183933366, 349597871, 1587335270, 653100402}
,{462289241, 1969005912, 4552981, 1596912064, 389707894, 1914578860, 1481934027, 1829854431, 930374849}
,{639565797, 88467471, 891234721, 5469723, 1817215961, 1689980542, 514489896, 1543385177, 954908602}
,{1853462410, 953432986, 910788562, 909440150, 1961064185, 1378462966, 1072571618, 319291719, 1741937901}
} /* End of byte 18 */
,/* Byte 19 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{224069202, 940167678, 1265804454, 187150741, 625651655, 1076491822, 1103780681, 2055685332, 1194651977}
,{895554552, 717275268, 396264426, 2113216803, 486029730, 1692321363, 126074496, 1639594000, 2059655477}
,{869441263, 2005641267, 1911187837, 572980734, 332280544, 2023459372, 343983598, 237838110, 1322779029}
,{1259175009, 1755939251, 165881407, 1785139761, 155369278, 134111992, 1407432398, 1077790057, 1603503858}
,{755207726, 2061872424, 1362416010, 1660538798, 1641169488, 294311167, 2011026378, 741931201, 1488867750}
,{1020733683, 1871644402, 1573902582, 1571837473, 706027430, 1836510160, 931124026, 1890189158, 1977196507}
,{824281435, 1663202092, 38276168, 920271310, 471406236, 837832678, 1677770013, 1728916176, 1584272492}
,{1577095969, 1036947225, 369643279, 1800149053, 431349669, 1451331277, 1687038432, 350692143, 630729966}
,{1494347735, 6449902, 1870820201, 1871356634, 1503052259, 1094040930, 414164052, 867405757, 877337697}
,{2120574983, 989546032, 1119791626, 1549670970, 1397510158, 569467335, 383607147, 19544098, 1175921359}
,{1204394378, 1026989262, 496189994, 122178885, 1097256354, 390702865, 896314903, 130591216, 622052699}
,{34550696, 681518768, 6976002, 983175239, 960341858, 239929295, 1533613484, 875907249, 1620141962}
,{356174000, 1946292312, 933107752, 1135990737, 1123872970, 1821631412, 1584026946, 1225826400, 694833730}
,{545391482, 1451705608, 723601227, 1069642231, 878672069, 1475566954, 1710391346, 1115250354, 1991572998}
,{1534973459, 196589047, 1991102685, 1165446365, 1340258020, 1699457801, 1503666637, 317040495, 692422935}
,{1494923393, 488224762, 203390741, 1934278450, 1013737600, 1360865945, 263958572, 982899491, 1850838784}
,{1943446761, 538835895, 2127113894, 371680804, 1251575156, 2113487358, 1172644954, 1464493623, 1061124935}
,{330291731, 1391155759, 365837374, 812478357, 928551173, 309186012, 1546112458, 56590632, 49750993}
,{1190660530, 1986365736, 1493979198, 2038010546, 1357862874, 495892727, 2072036020, 1312797939, 791009413}
,{1460237431, 607080324, 1451762638, 616971299, 322144625, 1277113229, 1971140649, 893439713, 1405305096}
,{1667581109, 1636110437, 1230251455, 1175450993, 1757214439, 2018824690, 1597404637, 294236823, 1976354098}
,{786673529, 492453878, 1970523761, 894710268, 349976285, 1183485784, 1985800466, 843925199, 2094711930}
,{118828726, 5104755, 1854130366, 1811051998, 611591885, 607759795, 1104543526, 2068706858, 1820279767}
,{2029920267, 237077554, 1585421196, 1128394448, 1718486641, 278224737, 1165919991, 1901438457, 730860634}
,{1363409318, 1721710592, 2127095214, 1366134806, 692027662, 1029768966, 263437523, 1596054220, 133408685}
,{2010258762, 1566503832, 1867212842, 1408241287, 1664659508, 1077661630, 416289448, 2121533439, 814748622}
,{181870805, 1338077783, 1696867803, 966527488, 1650821445, 1811293913, 311295657, 535451461, 1570787616}
,{2079694704, 1712706616, 135350303, 49526641, 1497915380, 1414310199, 1680451198, 1394104004, 1439475708}
,{198738539, 1074966301, 660431440, 807400713, 1169340878, 898033630, 1419861493, 1706313561, 679505314}
,{876791217, 1518665562, 2049966360, 1063517031, 1476150578, 889666089, 557467223, 1190526050, 2133208284}
,{1195722961, 1263691161, 1430521190, 1300058655, 2068963536, 952562307, 1848743659, 545624192, 1019001610}
,{714406650, 1472939049, 2080385818, 178250362, 2075461961, 149154898, 1813433239, 1583658193, 769562317}
,{1530532545, 796307435, 432548006, 1561636717, 1491551147, 797491956, 1871412739, 799268076, 782310591}
,{292545182, 638856542, 225273344, 227763756, 2061683409, 751214093, 1358212782, 2114621294, 677320862}
,{355768090, 391304215, 1136046257, 1933548441, 1228445455, 1340527207, 1418481011, 470712446, 1707941193}
,{1961957318, 228822903, 2019833410, 638430239, 1853486747, 1474873124, 1933124546, 606081742, 1856106502}
,{1811202200, 543755968, 382087234, 582369678, 526239243, 2057318845, 198896550, 1586497709, 937332686}
,{1433217028, 1447812746, 818498558, 246554672, 1368563658, 1805180245, 723556384, 422806667, 1972200601}
,{1164078400, 636810452, 183226118, 518813840, 868356638, 229383012, 1391432252, 1320302740, 406250679}
,{1833301356, 1800143190, 738073471, 65177407, 1291194840, 728990986, 1127171720, 1818150952, 1943332195}
,{669180920, 1182664238, 1588411346, 1912886069, 377062673, 948450916, 826956796, 1193954461, 66552794}
,{1685316804, 493102090, 1922324412, 65645707, 1878089272, 332737938, 1591117769, 475122129, 530680521}
,{1054619267, 410314736, 1898841497, 1017505021, 2081439923, 792817269, 440239447, 1615040491, 549808110}
,{422266869, 1646891955, 698937896, 1078561488, 870217302, 317982545, 1124514917, 362858720, 743216750}
,{205704044, 1502580408, 1101573537, 1805446118, 782083244, 1860080722, 665293649, 391477335, 108195265}
,{2122161833, 4000972, 2101579122, 1411641860, 1559404706, 657108701, 1978965259, 611629837, 1023211716}
,{136356341, 2054110452, 860250712, 790878436, 1683634460, 2113759915, 1731047477, 2010372919, 872629756}
,{767232435, 1704724234, 980047257, 838640953, 1254408822, 899321970, 1739026391, 1411097722, 975272984}
,{85002857, 480595359, 1549700533, 1513682629, 1799038694, 128721812, 645654440, 1287703575, 811471531}
,{1160401096, 1918594049, 1808717587, 236691048, 729218309, 1082956936, 1000185930, 960367796, 2111076498}
,{1471587058, 884800600, 1679321723, 785928316, 262809455, 154996765, 14460120, 780867274, 1492744462}
,{154965831, 2048621478, 466687441, 1264215266, 1722328900, 1028015796, 1824868830, 1699252999, 1020216168}
,{1691962701, 413921713, 1141820085, 79914720, 1345223090, 2076592597, 92890129, 1013770816, 560906575}
,{112593413, 684871370, 849777808, 1675655277, 1046987083, 2041328046, 779558086, 2047055041, 437892001}
,{1078196359, 869364087, 1095885261, 559672765, 1270515271, 384621179, 853893666, 905055076, 524249265}
,{2003239846, 1961498268, 2139473128, 1415953133, 867408308, 1904609416, 1043363103, 1372898594, 1890132353}
,{258679922, 664779814, 427715572, 1215238184, 725164223, 788990206, 795814094, 292458157, 300681321}
,{1888268386, 1576466037, 2030140778, 1021957144, 643702240, 1559817196, 1054708030, 1726551157, 1864770435}
,{718616949, 1250957074, 1895924748, 938734615, 1267543975, 1965808783, 1530014221, 510603820, 1088832638}
,{1383098212, 10357757, 461132440, 113241699, 1706953814, 814763571, 1357081196, 1356250198, 1017517880}
,{1739156827, 823376654, 909124806, 1643283191, 1130611229, 966291041, 678297124, 1835375927, 1239848931}
,{261956829, 1734954674, 1700032864, 1706120301, 1937830290, 462153778, 1323530677, 920762017, 1293441037}
,{2040317225, 2043060651, 1072159458, 6119115, 2043218294, 1865820534, 1922360824, 1910273203, 1696745220}
,{2003215337, 1479730651, 2139001281, 605100140, 661738824, 456482736, 1549735594, 1565931235, 1724275627}
,{437829660, 838862814, 1275796922, 2091311471, 1618822658, 685622433, 284209794, 1347592949, 2113532767}
,{1428996710, 1680976556, 91386649, 295833663, 1923167957, 107085884, 483403319, 19268319, 1809049863}
,{1934223985, 1920828091, 1307141542, 166844889, 1858546978, 1780720608, 1542882788, 485829372, 829513239}
,{1367295213, 291674595, 766281848, 2128003995, 673612112, 1208985641, 450511056, 1540828396, 1852309091}
,{1465670918, 79259836, 1725766070, 564500849, 648054236, 1499010297, 878989919, 149480195, 58865256}
,{945857338, 1337251472, 1258319967, 1650928553, 2094928199, 791453039, 873937221, 1452424708, 298089363}
,{1151260501, 1625315757, 2089642980, 1870034788, 1694388956, 1329748764, 2053727642, 1365978478, 1001548124}
,{967674494, 673279798, 904823587, 1355648412, 643039396, 1837832278, 46833925, 1876127450, 1060136612}
,{2136579396, 1958919846, 953002606, 776781771, 243123485, 781620328, 1538428950, 1312073462, 2146129785}
,{1530865123, 358288590, 169254006, 532895396, 1116384132, 144608344, 548137815, 1951659896, 3806439}
,{698391991, 426024407, 1880745204, 2063215938, 91564276, 1362703216, 974302349, 541094110, 975923161}
,{1911100755, 160694465, 1819476798, 1331920299, 939331374, 1102421936, 1004132391, 14106321, 1041768344}
,{42247411, 190521427, 973765708, 83623927, 768531850, 1356850839, 391493732, 1235091309, 1302617533}
,{522216350, 950458756, 1399747187, 1299914167, 2129978285, 1841424857, 433125940, 1148320088, 439576066}
,{1448926471, 612224558, 702599138, 168964326, 1759063161, 1286216299, 145049251, 1299313094, 660058484}
,{699869573, 1010191551, 1917788136, 65132973, 393542369, 13617924, 195034945, 533741654, 1709971343}
,{2116601939, 182935938, 2022133029, 1515146831, 2033840076, 1482234724, 1932847605, 1375521081, 1659781880}
,{2024651567, 1059030374, 1473682171, 1866097430, 1606063365, 1102021602, 1302722975, 873273477, 978295101}
,{565179629, 1268420338, 234676979, 932564538, 704807156, 2036975109, 488103332, 2050218560, 2134714307}
,{2105282016, 1303663453, 795778395, 1047362632, 485647794, 137318018, 532968724, 1290320308, 893029145}
,{1804424069, 1656970582, 1844785587, 1685562660, 1079214547, 1991789799, 1040284975, 784592007, 1556226621}
,{325684045, 213928578, 771527952, 147727341, 1475617772, 636810074, 2051882979, 1469677763, 1283353102}
,{281638492, 368465257, 2090553191, 1731423302, 1247632460, 1558381973, 793500224, 1883194953, 1984378597}
,{1390461553, 1295580455, 1284998274, 1844686867, 465430865, 57299635, 728072490, 516119672, 1573279168}
,{461514990, 310664595, 1907328334, 473097619, 1872561763, 1222876172, 620328215, 1866595479, 1111879163}
,{321129994, 436475861, 1706690107, 1136883231, 1431200373, 1827318916, 637631757, 591589569, 43028018}
,{196210911, 646036091, 1161754518, 970314824, 1998971588, 1758478278, 331538756, 424779530, 776556929}
,{1952775001, 1703538743, 719294109, 1763876487, 923009283, 1127441675, 1935302258, 871772332, 1906379155}
,{718708436, 2084968322, 2087092838, 1970839350, 97599228, 1403923640, 1595719564, 1841834232, 741447435}
,{1783675173, 1201249070, 1254498502, 834766100, 475348695, 1972827130, 1637750830, 1335295133, 1403859769}
,{922216225, 462233075, 1598893042, 257115280, 1993242784, 676328308, 73687946, 875569593, 935174150}
,{1799932392, 1753322193, 803527788, 685467344, 1799927719, 264352397, 388881902, 764107043, 1854612801}
,{567267242, 1502859610, 1248844848, 1301053775, 2041137295, 1519064440, 717172294, 405798378, 1684359498}
,{1813761182, 1629745619, 1162619783, 1269349184, 933748927, 1209677570, 1812974457, 312685577, 502186428}
,{1703837190, 390337458, 1956697438, 1633844683, 755171395, 867736265, 42377010, 545270906, 1208571672}
,{1542574244, 259721997, 1952043256, 53631255, 807059082, 149317741, 2007156229, 317494216, 266439377}
,{2076515046, 1085008249, 1471574251, 1967530313, 1648855487, 1002473077, 798586127, 292160798, 1331850843}
,{255514418, 1683364953, 642296202, 1349566250, 1679216691, 1379186542, 374329277, 896428456, 935970002}
,{2116173759, 1671839322, 1640011816, 1117013603, 897717569, 823804324, 1781177032, 1782091830, 1347727252}
,{620805464, 87208436, 918942942, 467991399, 531083590, 299423253, 62525527, 1197726741, 219253413}
,{249898659, 1102217864, 789520220, 1967421481, 443335617, 907379151, 1653053523, 1710975295, 1455106999}
,{1147075214, 1823217421, 1491933134, 1374600863, 1601299999, 777496002, 1434557898, 657757234, 726839873}
,{1881449971, 762851540, 516077206, 613584613, 1441721430, 1285460056, 1784153104, 418627756, 865704986}
,{907748144, 251641790, 1598143069, 1063518104, 173533849, 363426669, 289827729, 1928446040, 1646061382}
,{388331993, 1639240494, 1911804992, 228631846, 1422143913, 751705020, 1648708235, 1262196699, 873000886}
,{1718510837, 1057716932, 919165695, 926122479, 1001843754, 134115592, 13231779, 814117174, 72657649}
,{1771327975, 149120133, 1984872286, 1165129023, 1019766659, 1139768077, 1263830912, 2011278392, 1604309410}
,{1960107747, 343975357, 1787519761, 231148885, 846036017, 409556922, 1858923203, 2050131958, 2061225114}
,{274853686, 1271303463, 1044706786, 1378452102, 724150696, 1819349767, 1159553644, 1402105056, 1951400844}
,{1874051005, 273751227, 2002892693, 1643554723, 1483789954, 154704287, 552881822, 1664794450, 1974853720}
,{194805762, 324804174, 2068832536, 1996854627, 954562650, 471370522, 2118601218, 1040595836, 160857378}
,{426738284, 978071147, 68781324, 2126520391, 731761853, 1258209477, 1453769107, 221836833, 2049983332}
,{1442621441, 369008482, 1064737958, 569544519, 1524187590, 2118948807, 1074725152, 1481880904, 1435026003}
,{1710129767, 118260425, 572123782, 2130450862, 551824850, 271334283, 799279330, 776961273, 1675752775}
,{788589366, 653316318, 1705514043, 2014775479, 1452102801, 1021555134, 703724652, 909478392, 274791905}
,{1708456168, 1014686439, 1983998882, 1829291280, 139603726, 1275720239, 1430122616, 899251635, 1435478389}
,{1938290648, 279957179, 1241831685, 1876220335, 1851846129, 460548911, 114271925, 225612426, 269643227}
,{455803066, 871225532, 1933091128, 762981448, 882933226, 1355939553, 706893008, 1374562354, 1905074955}
,{2012536233, 136273847, 1302655484, 1823683135, 1083963142, 827750536, 583446067, 223612641, 1113072900}
,{1035668578, 2117722644, 2087451978, 1492754485, 813450335, 1540490277, 1226157834, 419354149, 1633044024}
,{1944595159, 849642627, 531507040, 1264233518, 811575622, 61660139, 402551809, 2089708026, 724422907}
,{684538509, 799531575, 1431451633, 1164129117, 1224226057, 1526310738, 682494510, 1998997862, 739325170}
,{696055681, 1428980888, 1949635000, 292860134, 1477553838, 1292735582, 1711925911, 874802071, 205005472}
,{1348636741, 121254949, 1906454706, 568330512, 739654254, 1904089919, 1132649052, 1460393532, 1517006706}
,{1027691990, 65333986, 1704174552, 197474590, 1420199758, 779383613, 679771011, 1140670742, 1235713363}
,{854671999, 773524070, 108305487, 1378291784, 32163761, 367014688, 838843805, 1621567364, 2041595560}
,{2054526705, 162954288, 406335259, 377586337, 168924576, 2128224500, 1808940331, 925573227, 596593223}
,{1793023969, 801851271, 1519484383, 1877574050, 1719865451, 1140660523, 1442375747, 2124361815, 439467904}
,{181352823, 1421818942, 1431348695, 1004459355, 753745420, 1680468001, 950559077, 349641131, 1812487030}
,{638490271, 2101182796, 1808273566, 625038432, 1799638493, 911343560, 1489303320, 103782270, 689572875}
,{184826010, 848430075, 1974634600, 1245620356, 1518944791, 1857482795, 787644710, 2043686531, 1984770511}
,{1363805241, 1808507829, 612623155, 1365940592, 1889817326, 1438550177, 1934491693, 2084732870, 86418998}
,{2085482567, 190670189, 1117407734, 1380343196, 678178128, 212370567, 1794697312, 2040689325, 500807776}
,{1266034815, 431432424, 2100013709, 1335283161, 533962614, 1598932974, 896142074, 377388341, 509250877}
,{629139355, 2138652814, 488551924, 780208230, 883493986, 1381583128, 863991288, 1336804738, 200863963}
,{2146257871, 681189602, 1022698355, 1255157152, 451428377, 1111115353, 2029672352, 1833262017, 1635147771}
,{439703485, 1558020871, 646503317, 585500426, 680162595, 1940995563, 289277417, 162130783, 945539368}
,{2114359551, 1042288381, 1452366489, 211884544, 939801900, 1483378544, 1362680728, 593939748, 1590916447}
,{1763451938, 330189596, 639819329, 1264464861, 2067954854, 1645716700, 63818484, 1671625121, 1590260244}
,{1731012370, 1825727116, 1919650867, 627006079, 1295676644, 146130963, 43143930, 1685647527, 1457319286}
,{1147640766, 2067200818, 1600731719, 58771332, 2116495830, 162287997, 1221808373, 1027002772, 898051401}
,{816737525, 781445571, 90707392, 107847628, 476454642, 1032458349, 695714611, 1045463520, 110542410}
,{1584126686, 1829183198, 1515354242, 1057176252, 2145736620, 1918983952, 2132200047, 1062525512, 914859232}
,{1946609531, 1012071555, 2120900562, 611709295, 404738355, 1511874225, 2128615495, 1477306607, 639300319}
,{605734822, 679755647, 1096573276, 1444380858, 1996346680, 682496824, 1287196117, 671739670, 687495972}
,{1697233631, 81679193, 1851588950, 280737346, 1809993422, 668763428, 329031562, 380324664, 2106376519}
,{1115151700, 1442507147, 1640551129, 1828619277, 1247246418, 1052204052, 413286252, 824975630, 2032246771}
,{1288422081, 41074026, 2054061158, 1462160066, 1435069051, 1844984127, 12452343, 1779924293, 60476272}
,{902755993, 138814887, 1195260442, 970171525, 1423319562, 1218714660, 768710311, 1882238922, 979284304}
,{503182265, 217056555, 1630076982, 612518450, 75002783, 577813674, 783597102, 553007829, 847817684}
,{1022257753, 1915985941, 136752577, 174980491, 1785046173, 806576451, 1035566825, 349403014, 1018510024}
,{54277785, 92754583, 661433769, 1649561797, 760724308, 610628270, 1521455698, 902303938, 230169565}
,{805838011, 559709560, 830244895, 478940590, 1560157802, 1458782262, 534419468, 1150887235, 620476358}
,{2147110783, 121073358, 133413541, 1518683691, 1841875166, 1839190778, 1422432051, 54464832, 963819016}
,{1279158324, 1390067517, 527138231, 709743744, 2104925655, 1371858833, 1442852010, 1278487364, 2086951807}
,{592015050, 685518994, 696010883, 780892235, 711452671, 1552055550, 1226533264, 781102322, 652815223}
,{2062663866, 849502134, 1210777140, 1609503427, 902216545, 771814875, 403771022, 1682931934, 1816428921}
,{1075002654, 1991923761, 791028277, 820450688, 119785824, 971427738, 236280204, 1167249346, 355493647}
,{1683316526, 1663912948, 2062690873, 603317258, 857998882, 1718997258, 1246293468, 1026108003, 820983628}
,{1403960889, 779341290, 21969634, 1002597846, 2001771230, 1254322620, 1730440364, 826440448, 424026885}
,{629935001, 449124645, 1007189913, 793562173, 1801529074, 336021577, 567459814, 2098189791, 100795006}
,{1812263361, 1639185726, 1909848476, 1045417262, 571046673, 1112267732, 661561311, 1994986809, 839168931}
,{1859038612, 848079573, 1555833214, 1643376059, 887304527, 1485906247, 1787385304, 1447287932, 1669243939}
,{2099192700, 172518049, 55754573, 1632348355, 1462884724, 1585284251, 682021091, 427273332, 454345599}
,{1951917886, 1798191840, 1196783936, 1215872756, 1811797413, 1759265674, 2084279503, 808497477, 1206700749}
,{254873674, 2064319586, 264169534, 412900814, 1984949021, 1330868706, 442610581, 365361293, 1105453292}
,{770008860, 1137850787, 350529158, 43145319, 2119903447, 868347686, 690852562, 16095287, 718405726}
,{2108059419, 1195558340, 335137915, 1764727478, 213279863, 350363361, 319884921, 1760141938, 1529965467}
,{1721050611, 1755481036, 1638109591, 1179372387, 52368464, 186745681, 1680122785, 1128200324, 86995080}
,{1108838960, 1554146624, 1603854779, 221736959, 1800157958, 1166052988, 2054729692, 791071755, 986411932}
,{1540582626, 490989857, 1659084141, 1089168907, 2076703574, 596031032, 591479821, 989525152, 871376312}
,{656922648, 1846257828, 1876226202, 884262323, 1411553895, 327844713, 1009861276, 101314147, 2127713220}
,{1738331222, 435662726, 1782517160, 133324626, 301108797, 41995163, 2089941400, 1549328783, 80966270}
,{1188464265, 1173112258, 1529870171, 53260481, 1966876409, 1294020677, 754840387, 540685622, 1992384806}
,{1746129753, 56866372, 1370117130, 173269596, 1865517877, 841585329, 2041929906, 102839223, 4163049}
,{403844792, 759681119, 348071666, 190052854, 97082485, 1906825630, 754424519, 1966113737, 1234303777}
,{878877166, 2026237240, 150657395, 260631675, 1108624424, 215391679, 2115379772, 808738943, 1147688341}
,{1934323763, 2014192616, 1470812648, 925577446, 2004382320, 1659301846, 1534302096, 2106558630, 193344976}
,{2137054764, 2138560395, 1239832814, 886887333, 1294862063, 1820087957, 280679482, 1003397879, 836298623}
,{2086536103, 1960420303, 905164518, 1613730951, 1827887863, 1552255214, 1865113746, 1605186234, 621827587}
,{1899248451, 55235570, 1001979255, 514842378, 1867627358, 571614618, 587165774, 1812572910, 159485016}
,{1900194377, 950459652, 1271111842, 850267831, 793212722, 2110752720, 532348390, 1737687718, 2117757303}
,{177142920, 465064078, 1686352619, 1597975491, 440418453, 403064056, 1115783470, 133709453, 1789484515}
,{266461828, 276406817, 1667525216, 634968984, 296419933, 1042138636, 873279964, 758817726, 675078823}
,{1234560660, 1642089858, 502764970, 99198734, 1067992241, 1209993627, 830886477, 1633749567, 1247799169}
,{1988595382, 2524269, 75854653, 888456184, 1927117775, 1668747897, 1317153989, 1393819929, 258164794}
,{806233933, 104605955, 280043086, 1503944961, 1753128410, 1218288449, 820009770, 143199718, 1488260144}
,{1519623756, 661079024, 1062641559, 1135701936, 147788696, 1847197627, 1167741666, 1371237447, 419474866}
,{1862051480, 1128218083, 744876945, 750958361, 2120558873, 476500065, 1114972834, 1863956422, 1682922490}
,{255594434, 160066322, 1008759389, 1704510493, 1253154834, 1573846823, 780035548, 143007863, 1525224629}
,{722144647, 342517302, 1692949352, 319170105, 34614019, 841385952, 539470810, 1204097145, 150050930}
,{707678599, 2006882559, 545325147, 40748199, 742682443, 1651820867, 718807120, 1936649354, 1568093722}
,{2058430684, 1843046874, 1743277071, 1387383374, 1243961460, 1552092864, 1629541298, 602295555, 496287104}
,{1141897953, 1538803612, 657343709, 1358478414, 1451293759, 387520052, 1810201879, 617533574, 590911914}
,{2073028486, 1707750318, 1648156762, 1975880136, 3829591, 1626435442, 1293652722, 1737850048, 254575533}
,{757970798, 330952186, 999508078, 1495992228, 291684203, 1998397689, 1266436528, 1911732902, 1984809699}
,{1715739345, 1352865252, 1258406525, 683923787, 1319398970, 1289227393, 55557661, 771852788, 1908243953}
,{1000661620, 1682247519, 458572934, 528134842, 87947364, 1332015952, 1280857539, 2119944168, 204960364}
,{736217460, 1395826950, 1372692173, 2001593474, 2011552687, 1138150839, 866965197, 1376033108, 241450911}
,{2101230035, 1960032920, 502470924, 966276483, 1024327206, 687731496, 1979751818, 58614982, 1218560327}
,{614559327, 214620668, 522082565, 1050396628, 1004768593, 751144180, 259182102, 1174431257, 271595106}
,{125278026, 1036667609, 1917348145, 1247457709, 1764861016, 1637065394, 700130163, 1324534699, 1193899689}
,{673858257, 2026131203, 1028939427, 2027900591, 346750891, 1468158266, 1159329128, 333645208, 1088968527}
,{1382291676, 1717144864, 639238488, 1812324097, 2092711566, 803846048, 1216885830, 312672233, 1231657008}
,{1087003426, 1992620714, 123661247, 526758917, 624918221, 108930962, 300283775, 2032272331, 1094738911}
,{800762247, 1130237249, 1211718942, 1264369540, 1214783325, 1601589739, 543788351, 107675976, 747600004}
,{210366924, 849074977, 699635082, 1215797972, 1014941671, 1204943283, 600027480, 1005788354, 18670653}
,{1467365579, 1268414241, 661913986, 1850477808, 553295278, 647462106, 1079547343, 1794758785, 462000615}
,{83239785, 1128118033, 1768481842, 1132910099, 1033893516, 1635855020, 1195932585, 1251204606, 1240216998}
,{1583473088, 1426305959, 522000824, 643675676, 1952270898, 1731083837, 1741482581, 141530955, 1365864152}
,{1273029620, 338299903, 2115772360, 1817936998, 1536223139, 2040223183, 1723816793, 462981844, 1803676223}
,{266641722, 278708955, 968982543, 398710068, 1169311100, 400132717, 506813545, 758196239, 796568234}
,{489403740, 387472058, 2043891459, 1250937394, 1871465795, 216180749, 787112191, 9080486, 365049072}
,{1218438816, 1182445150, 582509383, 1697784885, 1910037976, 326494835, 2078848926, 1614432855, 808115565}
,{2141438715, 877107049, 724114978, 1508004380, 275837403, 1738216234, 1618977849, 1907729774, 1111370535}
,{1461854384, 635158806, 1822479404, 1732768371, 886065765, 553006501, 1736301311, 158695271, 2011638426}
,{480610959, 923473919, 2126113416, 579326734, 945017929, 647885901, 2006727533, 1272836468, 1358518624}
,{647693355, 1392071757, 969230320, 1600066751, 1708617414, 628508171, 1930368466, 1568682882, 1374326770}
,{103419126, 1903864243, 1927593512, 1762191222, 1899789387, 364428996, 1370546634, 638254806, 173161652}
,{1846176344, 1238907708, 36852983, 461040286, 1145863001, 1022242717, 1504429132, 1222980278, 503071539}
,{648546288, 1839258631, 1210759661, 64019053, 911636655, 323777242, 1312185991, 785869962, 851330533}
,{1770031485, 1021748250, 1765923389, 958549231, 2079614051, 939108142, 1651983904, 809851594, 1151920899}
,{857694519, 755971488, 18664023, 1154017072, 1095630392, 348404224, 326219274, 395595232, 98178602}
,{618518704, 2034880871, 941340281, 1540470474, 2021189741, 100412407, 1657815215, 423364237, 1550889711}
,{234341828, 1776143788, 2087945730, 332141440, 1138043713, 405633960, 1945973844, 80547324, 1539348879}
,{53742823, 495088528, 1750071345, 1266312088, 392766969, 798401087, 74766594, 595898686, 828447523}
,{1800388266, 2128687854, 674394947, 1448156380, 257969743, 2142797055, 1535028713, 159277054, 1705461327}
,{629348521, 766844628, 14873287, 1446649637, 1580871502, 653946828, 644985356, 791408033, 552259414}
,{551035313, 1977956196, 637937513, 339428318, 1289282683, 1033474560, 276221417, 2068111922, 780191875}
,{922519739, 1250255857, 1117829134, 2060918877, 1032293149, 819999766, 1475074310, 470901045, 1404348695}
,{15447422, 239907709, 992272540, 700841405, 571670874, 449249476, 405538494, 2062141485, 386792266}
,{236708849, 863912481, 1641506348, 362289303, 1658364631, 1747432025, 1566776051, 995033691, 343735198}
,{970093161, 595062049, 225855619, 1323903212, 2092363776, 362903625, 635710262, 1316152985, 21097496}
,{662134054, 2058295933, 1320053301, 1356955682, 517245962, 1182303920, 906246929, 2006136898, 1747833330}
,{827536188, 2016108696, 1813986196, 9880910, 1629226286, 437980842, 1875441266, 2145205208, 1737397922}
,{920214924, 673207292, 1026441789, 1697577249, 1702801160, 780628798, 538473345, 2045982490, 2060449112}
,{631743781, 2065752937, 2053170836, 84794889, 1739485277, 1952016796, 940468228, 614624468, 235876525}
,{1025177208, 1039388646, 2114696014, 2092693698, 268905792, 1539570082, 1790594850, 392243677, 1956981605}
,{1030276901, 82038243, 46058912, 1276262702, 345997962, 1976299157, 1179715307, 1614981621, 2035612224}
,{285239666, 1292710535, 1718728606, 1288066722, 787269268, 1837121746, 1719981089, 1652440982, 423663289}
,{1503962926, 16337520, 567916968, 1215059225, 1014207967, 134195590, 1594690335, 120046696, 542694894}
,{997610914, 732889404, 689558067, 1697098256, 582527870, 1886964137, 1854481719, 940655472, 1541788643}
,{937918483, 664116158, 1865346423, 424452223, 1590029988, 657967236, 1205561161, 109879999, 1444197806}
,{871215631, 1375953232, 520604750, 1524267767, 2126697116, 104660120, 483973708, 66386781, 63352278}
,{764508237, 876995658, 590207498, 1262634354, 1113985855, 287295228, 1348647627, 1542226961, 681580601}
,{955487631, 477620811, 36668487, 271326909, 1822489881, 1324316146, 217402930, 1268977707, 2129966960}
,{1385861199, 1884536017, 1234729716, 296691035, 1766615667, 1292110415, 245620159, 103349797, 1408320691}
,{509303627, 2032766214, 1382156396, 661894582, 690011421, 458645574, 577385351, 1640925715, 1726300156}
,{1333648783, 1705462589, 758323473, 1366569260, 744105674, 1560808062, 426789016, 787696375, 2079024678}
,{806830011, 1064875755, 180417669, 1799663562, 734380071, 1963601257, 1318518329, 157066141, 786781104}
,{1370351540, 420766473, 1924365831, 1859038734, 74293307, 2088627081, 1300720535, 866915635, 1381485570}
} /* End of byte 19 */
,/* Byte 20 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{2057004684, 1493057145, 1403435589, 293942661, 1174538212, 1202602235, 445653895, 1748576533, 1958400796}
,{326008212, 689616539, 1998408437, 63370352, 1029484943, 1864003853, 594387812, 1143824806, 1273391355}
,{1991460307, 845822920, 1698463356, 12530034, 708098079, 900431089, 202196469, 908128911, 1571368966}
,{480611564, 698258957, 58642830, 1110046136, 889827382, 959541639, 1762912721, 600400866, 1323932173}
,{712899949, 1065416683, 305072726, 1390372915, 1311110036, 1782269342, 2043510468, 1534137710, 1847903661}
,{1706688237, 1256055868, 1106147095, 1142001677, 1797420356, 336592438, 773591377, 549122148, 316737416}
,{766703538, 719114066, 1921605263, 481446117, 2126558390, 236019463, 13637351, 1657789050, 472590399}
,{1367753254, 359163413, 725928939, 990141691, 490259944, 1383688327, 993257301, 749875818, 13055043}
,{1175102122, 934858792, 1322929596, 1359031153, 1119796165, 738321414, 420272007, 1963555227, 1184171112}
,{969205947, 1083680420, 2055462417, 1276984209, 994006866, 1496077615, 994964340, 928746690, 1742687348}
,{664321318, 1988110833, 627277680, 176994333, 803361452, 1540850007, 839599454, 1386373077, 2143491975}
,{347138697, 1886700805, 1233995738, 1488854386, 1139037303, 1443079714, 84672744, 1888715655, 2088599559}
,{480553805, 583484032, 1293153906, 1188439078, 2013920560, 952161112, 100904058, 938374293, 933870281}
,{575473114, 1841922782, 279698223, 530344551, 1325314017, 996262098, 1863741771, 1059150937, 632488642}
,{1940277118, 1727655663, 1975174645, 1208221905, 507608333, 833810307, 287476667, 1859224348, 715873270}
,{616793459, 1893772612, 537768115, 1979763980, 2095739426, 1570520719, 2028195328, 2128889074, 706383156}
,{1738409816, 1876319505, 2117553733, 394433677, 576334867, 838379340, 1824120964, 185488069, 1477781684}
,{1349385451, 1747691947, 863519181, 1884406015, 1465093658, 309039398, 540607027, 299329185, 1897199355}
,{301386811, 1357166917, 1216558054, 605534421, 1291569734, 285313892, 257704733, 1911202135, 85778075}
,{1288693491, 1926031733, 948855490, 269038594, 350253807, 508621370, 1844579834, 1126674205, 492788747}
,{75150727, 1373002184, 132913261, 131710748, 951732606, 2098318706, 42746808, 975742824, 1039719347}
,{1453280185, 2123445014, 2027741438, 33663365, 1294110971, 4184214, 1382031869, 516319852, 254405564}
,{1284250444, 1969851, 1326290493, 265308663, 170056795, 2129462572, 1869468781, 1865852419, 1557117913}
,{1827860233, 417096538, 817753285, 556481419, 172468690, 1173209827, 1158749122, 365667591, 1225601039}
,{1382175323, 1601853364, 211445990, 1954752359, 446050799, 1440214548, 566364357, 1434402287, 695629275}
,{728921357, 1299124112, 1636783535, 1310232837, 1498764388, 721879090, 310384030, 127856603, 1776137124}
,{1722027948, 2022404006, 769842024, 1242438766, 1918816568, 131204036, 294423882, 1354138772, 585874539}
,{56523793, 1178202244, 1279801811, 1629999487, 87069940, 406799640, 1137421937, 909779262, 2130879210}
,{1646469568, 1813726467, 1768631429, 1763016005, 1626690277, 2096455577, 712749430, 439811881, 1580130301}
,{2043143516, 594289127, 1763183781, 427509242, 784244863, 1597175814, 559927152, 1520024927, 1003855415}
,{1641396282, 833612367, 752114710, 1014522539, 1094253068, 473872033, 1982123869, 1287146826, 1287996773}
,{2083010532, 1501616673, 1403337849, 341625420, 1443048607, 2061053175, 145098326, 935086100, 1115768472}
,{430929789, 1480829660, 631763621, 390019347, 1472726769, 148888228, 1438646200, 2033976824, 1190331610}
,{644268113, 1321725811, 1977221359, 1132143998, 53405643, 344612695, 1319736004, 1099733126, 312744143}
,{547255956, 359183077, 144275905, 602785390, 1920908123, 1885630410, 558047085, 944339822, 1805137550}
,{206043346, 2036960365, 1479390315, 2143271149, 1339806661, 461606641, 1425202173, 892831112, 168322313}
,{712465269, 1543059027, 572680534, 1148363014, 1176116939, 112580177, 211154925, 1611329702, 826899908}
,{1465614201, 1471082812, 1805884848, 156731835, 271574675, 1065492024, 1862459369, 471238697, 1932002231}
,{1270709803, 100955866, 1015156851, 863327247, 1096923192, 2125813629, 1237489722, 644954209, 1837556860}
,{727929514, 1948106865, 572209447, 1745743603, 1275198458, 1049086441, 312403375, 1099531510, 1964104253}
,{2142818190, 2108077654, 1507385127, 760826643, 739671474, 284349464, 1837359544, 547247532, 1836625026}
,{1186819277, 1129947851, 1227986454, 1570016853, 1088063414, 392438557, 768510412, 442044750, 2007089575}
,{249335502, 1514656136, 1389742207, 867186605, 279194099, 1642137075, 278123319, 662469261, 154704040}
,{1138412749, 1623600618, 205913796, 1517990223, 1877993034, 44040150, 1264241567, 1827124951, 272131660}
,{663890081, 1764683428, 806037818, 294096034, 1837711895, 1639696082, 446892554, 1640922193, 511769851}
,{1008526460, 230454402, 170022707, 1675639553, 1052959513, 2024953788, 320282340, 1563540734, 2008958809}
,{1660855320, 941987691, 1060879003, 249241510, 1813690913, 360688329, 1901005729, 550257146, 117862359}
,{1899564477, 1832171501, 1400234077, 1547516366, 817894989, 845201387, 1856032868, 1756898321, 310354759}
,{880456370, 619896595, 57205613, 1617732026, 376485306, 1385478689, 538655713, 677583250, 1213072992}
,{2026697930, 1844882896, 579423998, 1782140172, 1006044093, 61796797, 235672572, 840661313, 2107677004}
,{1943018722, 238557294, 1278424050, 2082227745, 689203945, 1920732753, 2002031957, 1059394013, 1714280411}
,{1013094480, 1680723598, 569448587, 1467290270, 1465319305, 1326140530, 1179041225, 1591496753, 156676165}
,{1309927476, 338633984, 118072147, 339059229, 1661193861, 1598146285, 95591859, 198227896, 1162317368}
,{1278685986, 644455209, 1499911703, 2004241805, 1890989696, 841160208, 2064499089, 100551118, 1641048440}
,{138254466, 1649990131, 776736739, 1031896897, 666806794, 98362515, 205438947, 1711612542, 282490769}
,{65609183, 931570160, 2120906090, 1078114125, 176304470, 1230729493, 938531491, 1930744109, 466911673}
,{418439914, 1231832626, 812676509, 955359417, 1920465287, 441133562, 873028127, 1401418849, 248852684}
,{84918408, 575069803, 2074988856, 1605708142, 1705896001, 1404341819, 1476095037, 766952332, 578469226}
,{724412980, 1681762713, 1081799755, 1234150971, 1002177208, 1428218170, 1655665219, 547293962, 1025372686}
,{660596556, 1680691597, 1677014213, 270065565, 2059534794, 1019986073, 568399811, 1252241637, 1299127674}
,{130737665, 2032055621, 1450509013, 1894913407, 218662900, 375684703, 1641141914, 676268643, 304091422}
,{894783980, 1407582811, 355752025, 182139961, 318954145, 1094408418, 1905444361, 1770092264, 128918582}
,{1277048915, 1369428435, 2037345251, 23945992, 2071363564, 385590403, 77027510, 1070728187, 300744328}
,{137115395, 1628275660, 1784018621, 2007697726, 1809746453, 1043414360, 1418823595, 974238556, 1039266229}
,{1087815444, 801813743, 801901824, 1449960409, 1266550873, 850575067, 1255819827, 1762042746, 1294081006}
,{2021558774, 2083202094, 1610611287, 1883727423, 404570486, 731411429, 173501281, 1930376157, 1554299931}
,{634366808, 544704234, 1098709761, 1413488902, 1102900700, 1181669310, 1758867326, 1442130432, 1886645237}
,{190265193, 107584443, 521623222, 1711714456, 1403142458, 1436855996, 1736710541, 1211239949, 1945576117}
,{1818602194, 2092037187, 374865297, 1547772754, 448454786, 811281325, 627787745, 1864917955, 1406346237}
,{2044115671, 2139609086, 115072713, 923540241, 1709597056, 457800960, 2102326024, 617222862, 1708317419}
,{1707750326, 1358883296, 1652818483, 1919943202, 1520306675, 1405415776, 731426232, 804950562, 1453322343}
,{82321616, 1570670509, 1225734244, 1921722893, 740737498, 1112361214, 192162725, 243853204, 1084885606}
,{1800522813, 1681801037, 1600138858, 1955892059, 822737553, 299998776, 1065642557, 1260794617, 59336890}
,{1269136998, 1706926680, 297452019, 1468904992, 552494670, 1047980366, 574903985, 1804559509, 887266483}
,{2028352630, 2122245347, 2137521670, 1929120995, 119527489, 338580103, 193657534, 1217095764, 123040701}
,{36704471, 70787896, 1962599131, 1064489833, 1908465246, 1239057337, 1206806516, 212610823, 8690358}
,{764347042, 1919468321, 478508774, 1189573328, 886333285, 1866695221, 2080023276, 32068452, 16115868}
,{536230677, 1868841066, 852130758, 1663627798, 670337787, 1359037782, 591215899, 1349328051, 1379188975}
,{922671861, 819429522, 1679590862, 515660101, 1658781112, 603868553, 716617592, 1553334132, 256120707}
,{608759690, 1062201155, 195757463, 1169110146, 438667547, 1922877087, 274369759, 965706548, 959663714}
,{1996138415, 88135852, 2080271182, 542415663, 1854431987, 1000100142, 1944984497, 1687032189, 82854715}
,{69289307, 832953555, 41165152, 1421502474, 1196042622, 1560321574, 998291154, 427855476, 616796476}
,{907022725, 1589082094, 929655828, 360232601, 682413225, 1808495052, 1585172748, 664415598, 235119401}
,{922332699, 1955670008, 526617204, 1670254808, 1417796156, 1525479381, 100073603, 1957351594, 88478004}
,{461288356, 48510635, 1275216754, 654297052, 839612329, 1605046628, 1572443893, 1499806060, 314680208}
,{62059565, 1637224251, 1821988584, 845595377, 1238601031, 4890668, 531786662, 874218410, 2006366411}
,{272261086, 10810502, 1405339653, 1663090291, 1536265991, 559764216, 779925448, 101626196, 1071200093}
,{1548002523, 2068820898, 926794170, 69199758, 778258472, 1849142562, 494342638, 1828915117, 1963499281}
,{943944450, 1442100177, 1408001706, 1670660799, 1925505101, 37977796, 965212748, 44346149, 728867943}
,{1833705841, 141856507, 1415403399, 1212071752, 1867933203, 1754917292, 2089672535, 2091299100, 1135978518}
,{1263987074, 170853200, 1902360234, 939073390, 465439292, 1231696277, 1149143990, 1610614817, 1727085206}
,{1349961093, 117282506, 716266506, 1662890979, 2109454225, 1040660494, 1855273886, 1657790213, 777608374}
,{15692750, 914064985, 40220112, 99885715, 593440015, 1982314346, 2138138832, 640027312, 1657920644}
,{2117397139, 1250953331, 686411107, 722256296, 1481257655, 402490150, 1619281378, 2088243096, 320875842}
,{1701834681, 221640701, 712242194, 1276667654, 998202161, 1880856392, 1342432645, 1343687140, 527911662}
,{41024946, 680714362, 2097045829, 1166150095, 685898560, 1310600725, 27722840, 1156447101, 388516257}
,{1353838906, 256739459, 1441465660, 1378538060, 1830574470, 427140936, 862469603, 1398796246, 1985240878}
,{455496733, 778366753, 107886443, 1086551098, 869025462, 2014807267, 154323176, 603041257, 335838475}
,{995779296, 1145738914, 1212548768, 1581579267, 2126078561, 367719431, 117479326, 37222950, 1039308705}
,{1345113064, 1543772084, 1420996703, 598883302, 1700800231, 1502479241, 1002503151, 1429123205, 1737478021}
,{339392649, 1766615626, 928972046, 1862233809, 757469527, 466712193, 14003502, 2072080948, 891234654}
,{2048161926, 752903797, 198118197, 138006366, 1998702660, 680292217, 1785377021, 1767982238, 1165160512}
,{221898995, 1969379135, 1712731882, 727264407, 604208883, 1354843575, 701665719, 374307223, 795323902}
,{1866254603, 403834476, 477538122, 486215991, 1248544165, 72767912, 1740559541, 2131877981, 609761054}
,{211286536, 2089234005, 1090547832, 629559641, 1027457124, 1585786988, 1394515845, 678350007, 1541653898}
,{1369032307, 1889264390, 755153950, 1599554749, 2090824723, 14238712, 2038660589, 1852391184, 1871984538}
,{1893020416, 968868136, 1705921458, 1096561632, 896971375, 2024297908, 1856410649, 1413185546, 1046601528}
,{1828409107, 842565863, 1414626544, 102984408, 282760759, 954241952, 221648402, 240155832, 1487196406}
,{1875937515, 159673286, 1090617554, 404009963, 280719950, 1113707891, 1881935798, 1678067716, 1411383167}
,{1428513094, 1165653877, 1024233153, 50531304, 1892690499, 21429200, 1016732933, 1090539148, 222511609}
,{328608006, 615802672, 994110260, 1977223467, 1102791125, 1145034964, 1022844752, 760234345, 1373580016}
,{1697025031, 907102920, 1860392660, 2038778295, 736675553, 1494260165, 1874272514, 738537821, 585561498}
,{256823182, 1071325612, 1154862252, 1785816641, 1148349916, 1196185863, 1058319747, 19540359, 1034136608}
,{166066923, 1330794871, 469892707, 1167297925, 440177505, 1594793995, 1522327544, 1063980014, 1571023053}
,{2048005445, 120104848, 156657157, 1908445946, 1986298278, 2087657501, 1680834861, 1427527927, 90799133}
,{2046833158, 251671288, 507811883, 937916346, 1369093037, 1468014965, 530877557, 429986654, 574352614}
,{1319348657, 1870795646, 1137486658, 587553504, 275966951, 1086901069, 1723438925, 521220792, 1382029862}
,{1569545426, 1734275187, 138525607, 1137563859, 586585611, 1829179382, 1860544835, 2096985902, 67388910}
,{1629127555, 627987180, 1972450653, 1322654405, 171544828, 1668405271, 422141167, 1926524273, 893125703}
,{457137643, 1692642621, 1602085667, 408254420, 899354357, 1854766993, 201297943, 659922687, 1420000693}
,{166889960, 1215824772, 820848592, 1287191136, 1390483878, 1157413286, 1059999036, 486694744, 1649637447}
,{1454276444, 84012725, 1363748340, 439618185, 40935553, 4512699, 1254794713, 1396304406, 196474105}
,{453585048, 887879589, 2070186427, 842738028, 2103148370, 1588118281, 1621719211, 1456252310, 519718238}
,{980330825, 1147448985, 216597502, 535811394, 573353957, 1341204108, 606169523, 645774608, 1206326303}
,{1862778847, 1677915241, 505799207, 1697118042, 1640898196, 1777561921, 2138510364, 1378136144, 1588047275}
,{966382062, 371634878, 601138122, 120979948, 2073397983, 248149521, 621659972, 1191889146, 152998229}
,{879465693, 1892922280, 1534689256, 53270682, 462938373, 833490011, 276600111, 1137576649, 1031154187}
,{822720115, 1665783426, 1399979741, 1621236283, 535910657, 840960474, 461398481, 1567120997, 1812779408}
,{1407085920, 817122760, 1903203561, 1280041918, 460431220, 324642688, 501088432, 245667508, 1233441051}
,{1021412468, 591689948, 1399765765, 1185604133, 1376125921, 1093189043, 1339272147, 1468478158, 647410201}
,{2077666017, 1019122209, 48528326, 1659407681, 1872532762, 2117300203, 1777079449, 2003797515, 634332720}
,{176928420, 1524874174, 1261972413, 1905304288, 1022970798, 1462949227, 1501559460, 1379758477, 217932784}
,{2144043126, 905849904, 1769953057, 1164530457, 2009019497, 799679904, 1919561820, 492683820, 1836580160}
,{1914967213, 1333759079, 1647977708, 1631439162, 1286323842, 2059384925, 1464063896, 689520069, 2011359459}
,{939108538, 147309699, 39945260, 108571273, 1917978409, 1751356993, 822834683, 142701681, 1975407003}
,{104049758, 1273874899, 228435507, 340873588, 933298442, 1022709119, 1910596780, 1819648362, 511169880}
,{1053544681, 1758026420, 1079175538, 778007214, 2047496311, 1496659964, 984841280, 433312766, 392598290}
,{62607493, 2139471811, 116654020, 243923686, 108388892, 1365939912, 927065239, 1959560317, 1247903881}
,{1985354821, 1225099286, 909071388, 1111633495, 751716544, 637599281, 793285822, 1048731822, 599408771}
,{1820374917, 1451011354, 589710026, 373076879, 1562250613, 491757586, 1061839965, 538303803, 1695966829}
,{657562169, 1313634731, 309894095, 1988969894, 6146398, 2049855613, 988309001, 1750435926, 37738918}
,{258158906, 561494664, 1300685037, 1768149191, 1736924180, 1115384141, 1017674770, 338221198, 1403798396}
,{678289878, 481867072, 570225486, 1717976783, 6196551, 29717250, 1633566272, 287235129, 872031897}
,{1815994883, 202012536, 1907654835, 302965204, 489818965, 2036020521, 1302421835, 1457730297, 1804974461}
,{1276619775, 2124812907, 1631780063, 1411248032, 1850068663, 1141125074, 588141193, 453779338, 1476497530}
,{1210256512, 642345096, 1228226870, 917624692, 1156188033, 1507245505, 1237561855, 2095090778, 208157896}
,{1300275027, 470417926, 1028321440, 477598224, 714822590, 766940943, 1082014657, 1924254194, 1544572175}
,{1770774274, 1152570880, 248933009, 1088003609, 1638136842, 2036038437, 1361680656, 181918650, 500723491}
,{400266524, 1835766434, 1679488049, 289205855, 104772934, 2125169880, 935678591, 1142834354, 1208019463}
,{786315665, 1806420325, 928372064, 2060320050, 1445046500, 2068080754, 1707782353, 710976950, 122462744}
,{696936287, 733295196, 991656552, 1293886922, 907757460, 1240986772, 1845801370, 1721598266, 949443161}
,{1939197430, 27106071, 116953023, 104332173, 1878773052, 422128794, 1472446498, 1521711181, 1453548321}
,{1480896784, 578266830, 1832465514, 1351427601, 1063096624, 489449685, 50545389, 152620709, 1976047017}
,{637654235, 1915246905, 1567480458, 1934386520, 824679151, 1155197011, 196793863, 40147568, 1669620961}
,{1917376833, 126329144, 374249217, 1312083260, 784543139, 851111623, 398482696, 743117587, 1276071260}
,{1465807910, 747783709, 1294202988, 154731936, 1145730119, 1308492764, 1265727250, 785469222, 1085564256}
,{1420930961, 371692148, 892808574, 349525768, 1773394468, 2129224141, 254954307, 1011911037, 1097103058}
,{216188009, 1086480272, 1758022831, 306894384, 1643090979, 1977182110, 2139750916, 1275968481, 1303876193}
,{745338691, 1473530219, 1444931315, 374922516, 950655526, 246294036, 929231673, 1445555801, 552085178}
,{553241649, 971522886, 936634687, 735873458, 529654142, 1744255950, 1399886928, 1104906492, 1431427058}
,{232750703, 785695828, 1513146475, 525212045, 342501429, 1026355629, 2120289646, 427355763, 515049459}
,{616795467, 1880743007, 1382888497, 1649372664, 1530166237, 269000223, 1610860966, 1729816420, 924730490}
,{256900901, 1223521154, 220994492, 2079035778, 1543759636, 1240980021, 2041509314, 720262424, 1721164517}
,{189298541, 781379966, 1758904165, 571206238, 2117094881, 822499659, 1363655102, 308675317, 1686935973}
,{1695002579, 992932681, 1507391678, 311920161, 1323658181, 292307596, 975587290, 1044206433, 206566653}
,{1184378926, 693043162, 2026947558, 1005291478, 1125616763, 922755632, 844989628, 1568538567, 1328247541}
,{1301840932, 1320359322, 1320734712, 561306263, 1387352864, 5721537, 996609627, 964112170, 2123655428}
,{1594327070, 908866805, 158672950, 1726867196, 1998576114, 1424594154, 1163303549, 138594948, 661577694}
,{1309810234, 1939905820, 563090219, 1062726250, 1804311021, 837227144, 699557448, 1676874602, 1432153275}
,{1849006473, 1647753069, 1570474047, 1980062720, 879395550, 248981527, 87466142, 16714288, 1732539008}
,{1835889368, 1062451910, 730358081, 2086389517, 1326886100, 1278884090, 1356490473, 859998575, 489015687}
,{1966340073, 703036270, 334905369, 1273778385, 1543245316, 1410681615, 1451779539, 658281364, 1742343568}
,{1876012182, 400832747, 1991322179, 965684671, 1706706417, 2106460495, 717190094, 70423044, 2111555081}
,{1614736880, 942625395, 382915403, 1810347324, 1855709780, 377467487, 1002421059, 1352125771, 416098119}
,{75595196, 630726092, 1399993586, 1976793080, 327521453, 174771044, 1768320450, 284139099, 120698586}
,{1330439058, 353524112, 44891079, 1836690278, 126625535, 1820605724, 1984659452, 283066011, 1208668437}
,{1361723932, 1378969322, 1054237058, 1169185815, 101736841, 43372487, 1790675964, 971945957, 1586916856}
,{1757542592, 1682923440, 309370064, 1620553368, 1045385908, 208790074, 1739532091, 45349495, 492642673}
,{1734754686, 867989865, 1635404065, 1589545818, 1814478574, 905070819, 1211255096, 736109805, 2093711054}
,{143299662, 1790454966, 1513728122, 1541843050, 1770203935, 1955048449, 1795828806, 1557548062, 1439810478}
,{165232477, 2127677345, 1768771369, 2000361179, 1185536690, 1747723431, 190700527, 537751624, 1426820351}
,{1979567700, 2089075067, 2140009702, 212440451, 469408332, 1396226372, 792337013, 1330986867, 755182205}
,{2104116209, 393809822, 476670078, 132420882, 426046696, 623736790, 1332104025, 203055385, 891848223}
,{555103591, 1294126210, 1216991812, 1114694245, 1295042758, 133472312, 311283044, 1435241909, 1759075731}
,{439125168, 1359154129, 576971756, 755079807, 1601662257, 2064453779, 1894522398, 2024732250, 1966747793}
,{855510106, 1455253641, 728595301, 549991771, 1396043187, 1800070198, 2031306014, 1342273219, 818873378}
,{1859442239, 1673880699, 568807490, 986201072, 1078843585, 915701733, 1644176254, 1913977936, 2126938225}
,{1064891660, 1361192847, 817178800, 2135744164, 2145733028, 988480766, 1115455626, 242096142, 128264096}
,{106883270, 673468143, 528116108, 1451087769, 743011848, 1709800822, 144434185, 1937406716, 1782906415}
,{2145318509, 1491682535, 298272092, 1618831722, 722747330, 1707188698, 1633491330, 2120851580, 2127932924}
,{1374195960, 1562017539, 1445590134, 1074363836, 1704007235, 1081048953, 315603724, 1393399262, 1431065858}
,{403528979, 1098957500, 17742788, 761309221, 420623310, 850348175, 1330733662, 510300804, 671514758}
,{1702409222, 137507493, 2032688955, 100485412, 1325508694, 1903896277, 1044202711, 916130469, 1971230671}
,{216940987, 1305251850, 937626368, 1675424382, 834697759, 1980941781, 351670927, 156475187, 1531550720}
,{122689033, 432005165, 1590026522, 300515980, 467829269, 1784928374, 321056497, 2013507306, 407346806}
,{1377003669, 1406788686, 559015045, 1331447348, 1099051468, 1891205043, 219215827, 2006341085, 933703000}
,{768040764, 2022685739, 1790799745, 1821639325, 118092335, 35884554, 1133995057, 1102618508, 591169316}
,{93739404, 1152657056, 1635761003, 502799350, 2129159844, 1549344994, 449245161, 452937726, 467393087}
,{387320065, 700647489, 534510455, 1870904535, 1345759746, 1101956378, 2081561005, 1679229095, 221158651}
,{301606822, 334271278, 794480381, 493045350, 789587880, 443457255, 102399430, 1530947373, 1649620670}
,{402124027, 1678645082, 2147322363, 277277928, 2040466554, 142037302, 164874025, 756442931, 809343887}
,{1076424509, 259126313, 658987844, 843769049, 1786876319, 848745376, 2079913880, 115799191, 779978961}
,{1553979347, 1976050397, 154181223, 458355687, 280603977, 36293255, 1752091319, 1885233190, 694514184}
,{1708493109, 722745426, 1342252957, 692787191, 1271090363, 1091140479, 852979607, 1145863605, 1406909013}
,{1536272169, 1088816042, 181007615, 784162184, 583789790, 1517488424, 79383396, 326881266, 1113626441}
,{1596019616, 1238944601, 645425291, 407935606, 415769623, 250841006, 1201876033, 1383143365, 468540357}
,{332041095, 1106519810, 1835612312, 1613550299, 1699162283, 453434515, 1140039320, 1214536537, 951951948}
,{1663956313, 1752131066, 1069677304, 2049118868, 473399760, 418127080, 923332826, 222713826, 1039695015}
,{284070660, 1182755765, 1802462338, 1821128803, 2106406221, 1083331034, 779051382, 1763209184, 1918779736}
,{770437803, 1464517849, 1187088010, 453302910, 1147799773, 1516987165, 1193013414, 1538986768, 1468147863}
,{851640141, 571395732, 726520172, 2010787717, 2001315731, 380695865, 1130147531, 648868440, 1774211777}
,{251862914, 1795373475, 1108090510, 1899793922, 133920035, 1062976361, 843422966, 101357331, 1279560975}
,{338872660, 1680889385, 888922396, 1148830503, 1175873298, 1562283954, 1845361565, 1039824867, 1563553871}
,{757373824, 1192954155, 1511139082, 312473714, 923514060, 1357370046, 1680304660, 1871850730, 1342108128}
,{491147954, 1124606586, 203645330, 2012634112, 1335685733, 308459708, 954677891, 712129209, 2052808151}
,{2097126388, 543175164, 1806049620, 1394377251, 1924606110, 668174387, 176324214, 688418682, 1898407936}
,{731556925, 1777499416, 1971218854, 432667385, 1743990349, 2058325404, 1680301692, 1133051981, 1454796338}
,{510998238, 1506451534, 1471876853, 1274060635, 609270729, 1112853786, 1726623122, 884490907, 1051318188}
,{758242898, 1680957232, 1617953001, 1244363555, 611457824, 1200823853, 446045376, 658750944, 810391723}
,{1351434703, 1599723675, 1181382983, 1702810649, 1334196431, 1448844076, 244790827, 221269489, 75636104}
,{1590290128, 1266378229, 96487915, 1405717653, 2135027022, 1568770143, 2017024559, 592384875, 1390821404}
,{233593815, 1224371145, 1698914018, 1836089362, 1640312575, 605742398, 1272155814, 746589733, 627386892}
,{495315470, 43744453, 1057313119, 1366214093, 1024019037, 944125862, 941617432, 177654533, 210220674}
,{609090859, 710632614, 197114081, 1629480982, 799315607, 390076631, 1241703266, 1839129933, 1551484501}
,{1622569665, 958854843, 946811184, 1942694351, 1852918747, 927826372, 1910562142, 1488745966, 1976102753}
,{1487733798, 935442081, 1821552070, 745161312, 1887755466, 199986887, 355692289, 1587036652, 473299179}
,{1020642836, 1686992151, 591932964, 251945569, 1679924084, 900270429, 1243205955, 1427436220, 1180321407}
,{1487199256, 201258517, 1718648221, 1900618663, 329805695, 1398725695, 692223316, 1228843773, 1295574745}
,{274864149, 1807517757, 1122168880, 403968046, 686387110, 530199299, 829224943, 435861863, 855958005}
,{923878429, 1399729361, 1200494271, 1573038502, 2019897952, 2042739552, 1774025616, 522632807, 441112099}
,{600553436, 1924218002, 393842986, 704712586, 1908942896, 1284527570, 1018000160, 348412530, 713256687}
,{447337056, 210195996, 1065144760, 2082025386, 1405207892, 1771614944, 874576259, 1668549513, 69596286}
,{2111671870, 712033947, 365449845, 1827104293, 565368111, 1046690616, 1717095047, 1977740303, 195281552}
,{548617277, 225748296, 485507770, 1446059785, 370283907, 1859093349, 801192739, 1058268144, 429386786}
,{622405982, 53924596, 1431529503, 1006642557, 2106160934, 50830867, 17874329, 1966754183, 1739919517}
,{864877770, 1112549513, 172520829, 670727837, 153335563, 1883584117, 1338627488, 1061201675, 1392527712}
,{1527898985, 68410947, 192325308, 662764602, 975915045, 70517214, 1670882836, 1795942527, 1098954437}
,{1732988893, 613793825, 74809453, 1704851143, 101267460, 1403112083, 2053326418, 771438532, 1119754162}
,{34962299, 1289233833, 315059498, 1746830584, 1368441953, 1169622846, 549936596, 1124856046, 1618987431}
,{1179028329, 1432636593, 721726717, 1272378365, 1727895735, 1986344319, 2066615670, 1622804704, 1589410818}
,{1392043069, 580830734, 1467673564, 1277876437, 784104027, 1059983301, 1204053591, 2037573686, 715198331}
,{1974445695, 640445217, 430296961, 1517930168, 1598396618, 1560001959, 1924405740, 844388159, 1195486580}
,{675628427, 512994496, 1809470881, 753650488, 529292449, 1028143823, 1771743090, 1207282230, 1180485492}
,{258496357, 392026500, 731946532, 1729316455, 1200057515, 2126913875, 1598139411, 203600020, 1128797508}
,{1494736128, 1699206280, 1623494288, 369053223, 430736496, 2035042094, 266984461, 382982844, 726189230}
,{505282018, 1373061092, 1382019723, 1270854314, 1590570459, 981464021, 1145881498, 655047425, 1898174772}
,{1653513238, 1212251443, 1003380939, 515893316, 1250721280, 418773445, 1393681145, 564975827, 1804599017}
,{1669243476, 93272440, 1438109968, 604569456, 277528467, 1227024141, 1116025701, 321334643, 487183889}
,{2046113607, 1738022177, 394593398, 151104319, 2045346901, 1773501691, 954983812, 1105350173, 906855064}
,{1443231914, 2086427426, 1416081885, 1168959682, 251711124, 937051162, 1302862343, 1035080409, 855800622}
,{2025540788, 787744990, 1861918659, 543361818, 1776916410, 937799173, 782069672, 1558127621, 1338945925}
,{1449115513, 849978619, 1490078638, 1222368236, 1932836404, 1738652560, 881550242, 1128574016, 1108338769}
,{1659697305, 1030692677, 657979399, 975464585, 146284724, 986669532, 1601135251, 1596183253, 983484956}
,{365352603, 1445465349, 1969021034, 1110812350, 207575146, 1639714485, 1931991876, 5118464, 635280413}
,{961171775, 1476726629, 1116797452, 246470163, 1357572234, 603445596, 476094764, 509333725, 1716176342}
} /* End of byte 20 */
,/* Byte 21 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{210997660, 1557582898, 1719011835, 1944147003, 409192291, 1483733717, 877582487, 241128762, 539545589}
,{1398635851, 1233273130, 1516291190, 1096519297, 1637945024, 1924866588, 1701581343, 823088374, 705618999}
,{334475908, 893158909, 381493814, 2143924386, 624145901, 1635085800, 1956697431, 492712644, 881193205}
,{1824557261, 1121769354, 241704164, 1443974509, 604523276, 590548495, 1645624926, 1512250879, 2115344933}
,{2104203008, 1988675383, 952426612, 1437121763, 1928972774, 2111197789, 1179875027, 1775784780, 1307448101}
,{702697761, 1550683677, 1641878892, 1424779876, 76914261, 1412113643, 1469761664, 1339860023, 2077569447}
,{858109643, 416471811, 474010628, 1530306080, 1682439836, 438255983, 642743581, 1412720009, 6871827}
,{1342583156, 2141317189, 287858349, 1460527303, 1985477685, 1241916355, 1463800360, 948197177, 1331486126}
,{793884928, 300382619, 1015049017, 1857755295, 1886625305, 881516353, 2102382664, 1551418092, 484635514}
,{904544725, 1645258227, 1157206246, 845060312, 1293352642, 1865538485, 1041138688, 1339655230, 1898215135}
,{1002366471, 1443423510, 1450863893, 525391138, 1583756296, 126227604, 1836180961, 2137585631, 2016717331}
,{1880428105, 1612168162, 994734279, 1185999225, 2058672891, 1490889237, 150800499, 1025844868, 411136999}
,{260740416, 840354793, 1812657414, 1975117321, 2107250941, 319598918, 1486049428, 166509833, 1297353223}
,{1553393401, 2144836428, 1550178122, 882388266, 1422190950, 1708522920, 952195100, 549119981, 1337373139}
,{380959521, 177824475, 2110602948, 547260371, 688857759, 151862825, 446838224, 820363722, 616478821}
,{1628180416, 1204115781, 1774850471, 1919257556, 891496279, 204755906, 199947351, 882908342, 797226724}
,{1757551073, 537597402, 1568984535, 257364020, 961410805, 1318318897, 785450374, 1485694942, 656165940}
,{1447401152, 1725027655, 1780325003, 592877022, 921678356, 359443793, 902900399, 169487700, 862357661}
,{1625244687, 807105989, 209844789, 1024549869, 408149175, 989889625, 400254332, 25467394, 1902474140}
,{1423620752, 1813057048, 1802899441, 482898325, 1116321068, 455796326, 140132174, 1937090617, 116216805}
,{1837831319, 1107964093, 392886813, 196448471, 1920566840, 357348949, 1234182322, 2126716670, 102474182}
,{532800938, 1814228771, 789136266, 252007229, 437760117, 1413000078, 79837874, 1727302526, 477449327}
,{1403437605, 1281356209, 247670274, 374837583, 1652318677, 1327023959, 1823321353, 1132539480, 1255929859}
,{1578482247, 753076267, 1749938687, 1318749904, 1138454152, 952226757, 1094536775, 398791121, 1106251442}
,{445893919, 472356689, 648608555, 2043046116, 1228698044, 208919635, 38615771, 1063162955, 319720098}
,{2141973913, 64398422, 1394024392, 1741603911, 963189299, 710571923, 1467446766, 1872242637, 2133265476}
,{1821408808, 1160946169, 2050114523, 1597038748, 1464790138, 1382446545, 2103129144, 1290440277, 691013494}
,{1256595685, 539855061, 752424525, 819619439, 464742621, 636917483, 851625953, 1917414726, 437892561}
,{1249445131, 1171414028, 1219701891, 1776795727, 525648957, 1915224238, 334625043, 1981799509, 798386770}
,{615203729, 372665610, 74435857, 1993601975, 2102447749, 1123711946, 325381770, 805976532, 961362504}
,{282052697, 2062593966, 411765713, 199370949, 517935070, 1641752975, 1447764907, 444272508, 1221433151}
,{1408532066, 1810372542, 1770892434, 566400572, 158431763, 1961631413, 332702163, 772077362, 102953051}
,{328101964, 412187182, 190161073, 1030108533, 153139458, 726848224, 365260924, 1331763939, 1727131544}
,{1930887892, 806076054, 1247676639, 1896582300, 276912786, 1418755480, 751112221, 1741067205, 204410176}
,{1206074979, 212980922, 1487601372, 1668525234, 143647379, 1830964144, 1276260651, 255466365, 591105063}
,{420093790, 649728261, 1515387150, 506638012, 1342364435, 1576437258, 748885745, 781398913, 317255539}
,{1393906548, 1954995897, 42556172, 808766760, 1603802171, 178638417, 1805393826, 821124015, 779537854}
,{1157598304, 1412243475, 1749154711, 1354676945, 628113512, 1436580798, 1031603067, 1745235148, 876274669}
,{594217230, 1937648894, 751601090, 75877500, 1720305325, 655931040, 522025226, 537413765, 2069081419}
,{189858672, 923273354, 1920399363, 1703136678, 1946930032, 2145313007, 1921159087, 1782350274, 678862342}
,{881848031, 1208062555, 1854685909, 321623030, 1382338284, 2080531750, 52757973, 1699022110, 1861422369}
,{1910134281, 861785629, 1481216727, 194183193, 1694608165, 300612813, 624382966, 619894249, 296529172}
,{721366117, 1564848920, 9209032, 171474064, 173048055, 990248232, 951112339, 1878694210, 589246905}
,{1568588272, 1206635953, 1904004807, 311536730, 1313677793, 1946120246, 1867107815, 1268661730, 1943388011}
,{908437422, 1779937401, 2000587072, 1077223391, 896354426, 1676732668, 125240450, 2107131057, 1984219656}
,{846234951, 1692785478, 1006348240, 347840493, 714003376, 430235604, 1285262508, 756992074, 1952294703}
,{501273529, 1201499268, 1019754623, 197417871, 1937281622, 1992421174, 1825645765, 95312765, 293309419}
,{986684420, 1159875987, 208122617, 1680899520, 338634680, 814536440, 2070518510, 2029959865, 165019639}
,{1722842074, 2099637322, 180001785, 57915308, 87493384, 852397502, 96411024, 453668179, 1968818986}
,{132542035, 215431719, 2044410825, 670166546, 857554540, 1779654742, 1636280030, 1171210359, 340256732}
,{1474287898, 1490400818, 1905123028, 1709888679, 56562069, 759651689, 458128931, 583175912, 1140940688}
,{615948042, 946196426, 1983074291, 1536881655, 919788479, 945362976, 1222700520, 1108936473, 1927525084}
,{405321570, 1525144409, 1888771598, 1601263032, 591013788, 309540036, 1350935856, 698938753, 1742249595}
,{1577309626, 1729900611, 885052451, 463349167, 1626432833, 1822087336, 1965308801, 2030348916, 486313983}
,{1352563301, 679384343, 1334127186, 92521802, 1917643980, 313361370, 150708883, 361896240, 1790980296}
,{1899852190, 1060434910, 1150427336, 451695246, 1817548998, 1469228467, 1387816649, 932327060, 1491234527}
,{495323042, 610197167, 1747174926, 1711249138, 1345078840, 651582467, 123256692, 809115837, 739765750}
,{1357546386, 1162046573, 907671694, 2129309545, 1944036706, 1331150432, 1730259231, 557351173, 997344519}
,{1766268736, 624863522, 1349055709, 268096242, 597782181, 1724656382, 960174340, 2079518870, 943692400}
,{479830461, 1887126981, 334025055, 1243971227, 192112758, 1826560502, 2056960891, 1705150921, 1850167765}
,{17955374, 765731852, 2100283941, 1181167454, 611540981, 115202377, 1737840562, 787671622, 550604278}
,{1330469933, 2008959102, 569049385, 343593369, 522858145, 791675349, 1258330654, 1406416193, 1426476848}
,{1977140793, 1882096587, 168078474, 186377428, 1447846935, 438478033, 1410475533, 1088575362, 1281593923}
,{925416498, 1511402149, 458227598, 791566554, 22979407, 972758010, 1191504118, 159791748, 826045754}
,{2058801684, 50488107, 1481226397, 1726227871, 335194579, 1906887894, 169570540, 1666878182, 2114019227}
,{526383150, 1979518552, 982153409, 1366952802, 921786160, 721542626, 1373858584, 1315599027, 1212945777}
,{1774043700, 399406944, 1500109697, 1756229863, 1104169367, 1925975296, 396521614, 973202204, 1193045325}
,{1791403815, 1595318686, 526240355, 1974432843, 1586788711, 431456439, 267856419, 1773308914, 321885497}
,{778815353, 269522906, 1165328397, 593687504, 1092234629, 1392801903, 999970278, 28949542, 1296850654}
,{19374049, 1925796710, 1675016265, 59614254, 847763241, 737899787, 20886969, 1018671456, 61731502}
,{1671158254, 807455199, 1059615401, 1002463047, 433200328, 1725788363, 777712021, 837638022, 354775385}
,{117370444, 912934941, 1039378723, 1514573891, 120401927, 1301352015, 47034606, 2142356873, 766441392}
,{672447086, 1535687894, 128983578, 723109473, 1838535629, 1944576508, 307375259, 508153950, 586939923}
,{1068722429, 1649112385, 1797233911, 1301933918, 82600534, 93943529, 583237670, 24058910, 1147921739}
,{776063734, 212216088, 335742243, 1693093170, 1928004155, 1443832334, 1333639479, 1118945170, 462631522}
,{298613660, 1369189622, 1990587159, 1161415970, 191327119, 1264674841, 1655325147, 1333671445, 261413025}
,{82367028, 1100457507, 355540998, 1688569278, 54067248, 1930622506, 324068215, 1038274491, 871787182}
,{595111659, 120795042, 603117432, 2083238131, 508149579, 1952615426, 563947127, 489992266, 980897380}
,{1255582371, 266154997, 1356466304, 2037748152, 72325468, 1007817173, 252111017, 1627386749, 469107747}
,{570764003, 1001799993, 173223297, 1313750134, 1664411928, 1124155139, 1819878463, 998915219, 1207417419}
,{295729869, 1741590789, 962448446, 2064042142, 1753498093, 456969329, 1768876822, 1659683619, 1065234644}
,{1378319774, 961753644, 1709957193, 108760917, 1578603667, 1982580276, 586557367, 655085115, 812452965}
,{75869675, 1887690947, 1200068824, 1278245823, 346051186, 1324265649, 1702943488, 1541209061, 2061485844}
,{1550769241, 510524775, 1515614093, 218884463, 532002926, 75129723, 1811441237, 1410241130, 1112160354}
,{210919391, 1469252680, 198807974, 858014697, 1599359608, 429063252, 1337126024, 819517318, 1097780182}
,{238313740, 1634038046, 117216558, 36697102, 1770699708, 1550628237, 1399857775, 1157359616, 936127113}
,{1784960213, 1715289263, 1330130974, 1408805404, 431365435, 338933912, 352510646, 1929671772, 1044470503}
,{1323776252, 902965601, 1077542519, 553630570, 480768161, 1728134101, 580069766, 994851246, 776815319}
,{164907106, 1054903580, 1202416077, 155074847, 243232777, 1641431187, 1379534438, 1604973076, 1399479118}
,{1840507038, 873363526, 1362897687, 1486312807, 890956630, 14437698, 1143096128, 1945222621, 317502895}
,{1410828922, 1291913041, 1298885665, 1970898546, 215152468, 1012871283, 1266503263, 1849744951, 748857741}
,{58537349, 1994926011, 1860029135, 1897586103, 1590500113, 793321933, 830747182, 1821572954, 308704485}
,{744498641, 1673198832, 2073917089, 36334383, 859519648, 1519998490, 468175794, 1375711408, 1237014114}
,{580460862, 1810103150, 516011387, 1832612255, 1409336815, 1821690213, 1274284183, 1708735338, 596531082}
,{3515954, 976629809, 419299284, 977417884, 2021426265, 331154131, 1484511344, 887335643, 2032981337}
,{1848357512, 1132396330, 94527352, 1408903871, 1942274614, 1773961389, 440008507, 2006520170, 166250731}
,{972631984, 1573128395, 225579030, 729472834, 1177847549, 1662203930, 717549478, 263582152, 226600337}
,{2112370750, 1648865392, 1187708692, 451527245, 1690620755, 1983277453, 1889425954, 1293466714, 593662311}
,{853363435, 2128713642, 874745857, 1496616583, 1782311965, 1293165528, 1281131381, 1155594782, 1592602156}
,{439195034, 633584272, 1004889640, 332846781, 157754771, 1983104563, 632925590, 1185064648, 601975603}
,{116661648, 1749077311, 342837153, 2139622971, 519630879, 419755546, 840167806, 342325686, 1044061983}
,{1988330181, 950234246, 301914116, 1919503007, 568301287, 361692865, 712192525, 59622888, 1406426735}
,{278809932, 957385774, 1791302887, 1815409768, 78524626, 808839, 1683645611, 259766163, 309729016}
,{1408096968, 850865283, 1749425351, 191671124, 376218691, 1217024803, 1645589217, 1441075998, 1804577374}
,{1900981529, 561325072, 716222830, 271690689, 1562318510, 862349178, 199652253, 1597676771, 36763999}
,{998491852, 670869177, 1632096619, 1146486805, 1142908165, 1609887217, 746513546, 1529452158, 392851545}
,{431358143, 2082392598, 1763762999, 294259020, 1754887240, 106056148, 438153159, 33531232, 884157850}
,{655826326, 347656814, 711080464, 1812699854, 425459271, 762362694, 1947492074, 486832534, 1758306437}
,{1886844009, 1954801863, 1721027657, 41483945, 1147293787, 113290454, 1037130904, 107984858, 894750722}
,{306707958, 148296918, 38486497, 1574126439, 915308700, 1973684415, 958309687, 1990337325, 755561548}
,{926142560, 630992084, 1008482085, 2069450272, 403511135, 306842188, 1760080778, 1238673635, 1580588098}
,{603065048, 338904226, 1557255459, 848775905, 192830344, 986824918, 997118373, 1895648845, 66224160}
,{314760962, 622418851, 661477319, 1893709542, 1152227534, 727000879, 299652829, 607715046, 1583772362}
,{1305411726, 2134583786, 73776439, 1696105772, 742309579, 174451129, 979086185, 1890296519, 2120815262}
,{831232162, 1087551186, 1093938148, 1552498498, 300855213, 1011368294, 95714151, 552630904, 536931126}
,{938236172, 303862892, 481920272, 1862533360, 1329324232, 1263970420, 39530625, 238084906, 1890239609}
,{683621578, 113936821, 2054287896, 1235223781, 1032734038, 1807404551, 455328662, 1290607114, 2084260101}
,{1876097738, 876615826, 187229324, 1399460119, 1047691751, 5233253, 1581262056, 416521670, 1975650990}
,{1995469003, 98021091, 660297570, 1722237324, 1870222239, 1869213134, 1509311526, 1838223384, 1656340784}
,{967088273, 2040613891, 1381303519, 2019078387, 2023246789, 950513719, 941249535, 1152770138, 1405028876}
,{1664769830, 591075767, 1139926915, 1740709484, 447232264, 1082148150, 1834916886, 1742733668, 696268939}
,{706257206, 773154472, 548159078, 135784622, 337278541, 1170209544, 568082467, 688575035, 1706890014}
,{42702924, 1236324577, 769196410, 1624756658, 19847423, 1650875283, 513512608, 156537057, 1790420553}
,{1426600741, 941477046, 2107948387, 1863633032, 555282405, 549540969, 734493042, 1821308832, 1206314502}
,{1695283043, 916373015, 654303053, 1969781746, 1777319200, 645146497, 106379232, 1073555556, 972078026}
,{14181901, 2022087383, 5209773, 1629777289, 838882843, 2124497900, 1409119155, 599714673, 2024847078}
,{1122177603, 870086878, 281929448, 2094528601, 532842019, 799447264, 431619635, 1406590817, 1865896333}
,{591802549, 1702774888, 1021205910, 1480398903, 692707898, 314527990, 1984862937, 463574328, 1389450342}
,{1222376730, 1249350243, 1889985175, 1823241736, 439164912, 1895260211, 260276460, 1732144975, 1443526906}
,{592299173, 1988284106, 737200206, 646555147, 2125013537, 1029360119, 269549982, 766961018, 1641828816}
,{1027883877, 1786211297, 1870312000, 2112057651, 413274810, 1104329393, 732177436, 1426298819, 1954179688}
,{312460720, 17655692, 994122018, 1508320027, 1172943541, 2055778087, 1026218964, 1578640119, 513045317}
,{410262048, 2064886617, 1475184731, 610987369, 1776850423, 632804712, 394709623, 794961548, 210123693}
,{533619983, 1750627057, 927836449, 777315717, 1543127285, 1885982610, 1341484359, 200422009, 291863576}
,{1351605108, 1737597431, 1692387228, 46112467, 614055860, 2123529102, 1163900407, 390366601, 584801732}
,{2095014343, 496751055, 1723087941, 618610520, 1049612627, 85618582, 1907869802, 165604454, 347405974}
,{1909674430, 1387214188, 1181202352, 1338962644, 1469058623, 1792569961, 661177048, 1966544136, 351271446}
,{2080311393, 1487025744, 440582554, 1070744790, 1526068456, 7297480, 438028436, 487938108, 66799930}
,{1640367415, 2016830100, 1120440373, 459034553, 1752991361, 905830593, 51780389, 1407279058, 1045695326}
,{418778612, 828708250, 1600267404, 294597015, 1601542925, 1970625924, 1340045060, 1973064891, 2092086219}
,{1825292716, 733936359, 1896104310, 208652418, 1248928907, 1813070826, 1905774615, 1152244038, 630895129}
,{1383368756, 78255562, 765896230, 1948641230, 603677737, 1312063415, 1917286770, 203849037, 210208027}
,{754175911, 1719147112, 19466458, 102829279, 932828185, 488817902, 63128557, 1906095384, 695191530}
,{284561158, 632025897, 637228848, 375671231, 197032432, 317681127, 1277659730, 1002232149, 812618821}
,{1523135988, 780395666, 235438018, 1257357815, 2060668737, 1106291529, 814603624, 1401743276, 1140561861}
,{1534299235, 593192429, 1570377687, 104981017, 800033672, 1666800432, 128004077, 1885540690, 1213836254}
,{227695286, 1175373744, 1593077250, 1057255968, 1168992404, 1739925488, 1843096788, 1271165551, 1905200467}
,{1070351438, 2142138932, 1243628788, 338367478, 1520169520, 1552767564, 1129656870, 1016729397, 842191684}
,{2138533734, 1987217277, 528443480, 644031186, 1964199923, 594485376, 2038360516, 1648591923, 1709315297}
,{1878944746, 764933665, 1312693170, 1471159530, 1790833304, 751548098, 834902971, 212575849, 535315751}
,{451556731, 743743310, 1528341250, 625726365, 1207027837, 1766814460, 909956189, 1273609340, 761264568}
,{1637333611, 140199413, 818352951, 1057208213, 2004557817, 748248655, 19595010, 2124091783, 1583248240}
,{1134938492, 130189750, 190635208, 2057632469, 1251704321, 1021451466, 477372289, 709436162, 1218150029}
,{656727302, 543641357, 2013957669, 1748468997, 115715938, 528535166, 325731808, 345972676, 1712612419}
,{1004915674, 1406931403, 203020545, 1107023677, 1644563918, 410965823, 381305054, 329321814, 885056189}
,{2100092036, 818571575, 447546948, 349049466, 2062967719, 1801526280, 1964496397, 1486836827, 146710459}
,{491574884, 1217925302, 1230015548, 1081610411, 968858488, 1475370822, 928384125, 1293546879, 408742898}
,{1172817999, 1559109304, 907083830, 592795155, 540740006, 1041248822, 757395107, 1079326144, 592472585}
,{569663201, 737998692, 2139763696, 1747288378, 729528645, 645990754, 1269421838, 1088242370, 1422385639}
,{790164150, 900693636, 325473658, 855690701, 1024134774, 1697878007, 482835632, 1975188418, 118070463}
,{490203362, 1846822748, 1426528383, 1997450150, 385128480, 1889999936, 335531676, 1450517334, 1380685966}
,{829887857, 1966653468, 1674128126, 744711593, 754947408, 1354070171, 233835925, 1199739931, 1516154438}
,{1531547513, 542162739, 784353031, 1926161286, 1340494860, 1401985480, 293658113, 1071761670, 2039120099}
,{1738478093, 594404261, 1593890729, 951004226, 1707411345, 1381801299, 1002886092, 1888484282, 1261190211}
,{1555684468, 513462646, 1375526423, 1407832914, 1877433224, 827966541, 1193112316, 560746874, 1036926724}
,{869425811, 596604722, 26097264, 465830201, 1065718565, 1473421694, 1013638224, 540553552, 17531363}
,{1252363497, 240906065, 584050870, 1034082861, 129603097, 1627939941, 121646073, 239887459, 2085751951}
,{722097334, 236143771, 568649022, 1795194896, 13554007, 1486648257, 1873683232, 280412306, 559654794}
,{2030687460, 447262003, 2144684984, 733449912, 1019475249, 1884005039, 1377909038, 36433384, 1436220294}
,{1387685949, 1603175816, 318434280, 396085640, 748552276, 2083853375, 1199698334, 1479429776, 1948738772}
,{1682442199, 290984934, 278386337, 1912859630, 1941354040, 1742239629, 634970108, 402129904, 82621656}
,{2068442, 1412375413, 1321600245, 1058312275, 430753217, 1626399823, 1074482854, 37998182, 632202218}
,{386288915, 1008602777, 991218576, 1057305110, 481005634, 336530745, 1410303727, 1315318082, 235392670}
,{1235085781, 26137904, 1585271307, 1299494164, 834995648, 1848715710, 2011962632, 1883797431, 1356782553}
,{30282270, 180255790, 1804188160, 1572562886, 111511747, 1243398819, 1261624391, 164626708, 912885858}
,{2141346344, 1180430663, 1740098668, 136211115, 1016854141, 860456172, 579945549, 899597814, 57749812}
,{1754603513, 1144525465, 1285833696, 2126643303, 1312250992, 1926278606, 1255881278, 1472336491, 1978049226}
,{418370077, 517537276, 2138093699, 252116279, 1553601472, 48985202, 188213596, 1904228438, 672373154}
,{1285053250, 754699600, 1322313818, 393122329, 962132486, 1378672786, 1484023437, 1699075226, 1011701251}
,{497119785, 1359637256, 795694129, 359376777, 211876702, 1722516243, 514935991, 1360265186, 380028981}
,{780878375, 474899254, 1313766992, 530799002, 876522960, 1812204558, 464747032, 1656627729, 1263070896}
,{167474124, 1243191137, 319789473, 1262301708, 992240162, 1614071923, 76986847, 1986975588, 1155792165}
,{986104285, 851822290, 1123881359, 1440779004, 1546165334, 1043556702, 973384878, 885345932, 377300799}
,{1252342863, 1007360516, 790194401, 1782228300, 620927658, 891163061, 585661512, 718253312, 1658029187}
,{531323688, 434129869, 1486302957, 1326032864, 941152736, 1107160037, 776674397, 732074278, 470178374}
,{1316163257, 424072873, 1795062523, 1829427607, 1284252252, 1788776833, 417371355, 1146476721, 990359582}
,{807218705, 943508188, 1401342637, 1772685739, 1001152660, 1197852553, 230474967, 296155130, 1971240188}
,{1982861102, 1872226202, 40037765, 1924611085, 953008995, 1392973181, 1807354981, 1399411024, 892711866}
,{1424073665, 458071724, 1563953364, 177419560, 549503852, 496516853, 638553189, 920293585, 764272009}
,{933117098, 1958868094, 1512511174, 1185048037, 832599301, 1417528745, 1553247025, 1178282490, 1493942253}
,{2013028260, 482507281, 975283648, 1306089580, 321934210, 1925431037, 2126743885, 1098600132, 1225914352}
,{1624078303, 1231311499, 336835077, 1372461148, 1115372058, 985006640, 2139566400, 623612908, 1756100999}
,{375458744, 1460391468, 452606986, 1391314657, 462980024, 1758477637, 1662500913, 57820061, 550117185}
,{974899209, 645704940, 1323834023, 826738602, 1823039817, 1270857858, 664285408, 1453232168, 25918370}
,{44936232, 2019645694, 2109357712, 2075799552, 1617185129, 1144924554, 1064389024, 2132760123, 1192841633}
,{1482136533, 2104974295, 1641826359, 1237715577, 2116373231, 834607232, 490720369, 2032331089, 223941344}
,{865496928, 1379185122, 679089244, 610124547, 783365612, 1935254656, 1719737032, 1296706774, 1832756794}
,{878457627, 1399099243, 1457548233, 1016398808, 1377483558, 1843258350, 1886149634, 1859484989, 1693031453}
,{1424410828, 1492976000, 1751990749, 2036494489, 1362738702, 1853425669, 2069659406, 1956358791, 1859129131}
,{1655929366, 2013211950, 1653519642, 80563324, 67931716, 2077881766, 1570506118, 1463621465, 92535115}
,{679826767, 1598814398, 912289011, 1124453312, 1485967496, 2112494306, 817161098, 623698563, 1589795546}
,{95683842, 986363499, 1408896983, 76627890, 1132427136, 1167622519, 892854919, 1804238133, 1896808160}
,{1500766562, 1413334479, 1386018830, 1188504376, 2107350080, 1102814547, 1751653015, 38554909, 912739856}
,{634650384, 1459447000, 363259374, 550759044, 1573490051, 1666816095, 1390486476, 1782075695, 412892646}
,{1349362761, 1056547762, 849031039, 1010315949, 1574244779, 691644978, 392206932, 14050064, 1998823986}
,{1938246536, 1039405288, 1110174106, 216881308, 2064497847, 2079005918, 167773670, 1231902347, 237467655}
,{1975929887, 391901366, 269641404, 694303564, 1578182836, 451945182, 326208955, 43259518, 1348293640}
,{1146890982, 1067668693, 860587194, 909887140, 1916941916, 881005214, 2055473922, 405219632, 916121586}
,{1728493464, 675501693, 715103321, 51144113, 1305907736, 500025710, 1613836146, 130546556, 830719712}
,{1902584066, 1444402561, 1248531986, 1821518320, 1528074668, 1422743232, 2039062043, 1429969152, 168592006}
,{1612348857, 123157282, 1865393370, 505691183, 687379734, 1384196282, 1595673036, 1665192538, 2037297628}
,{1621753310, 941494368, 34243631, 2038693443, 38985979, 1447622533, 923440115, 1178618491, 807585513}
,{1500273564, 1629184799, 1614001086, 2044495337, 59076075, 1533585781, 1184620245, 19265516, 1770320062}
,{785705290, 1163224718, 1155791328, 479784791, 1828166623, 1961969690, 1542843760, 2087273122, 1312369395}
,{1840477248, 1472784549, 951851026, 1455194333, 723178149, 1687610971, 1542303192, 56394757, 381317437}
,{438762515, 683893563, 152113363, 1348151863, 86089222, 2140939418, 1335519849, 1847087557, 1264969677}
,{1157663106, 1774838122, 1412425749, 145924544, 1471929561, 2013901129, 1673927995, 1170662481, 1169571875}
,{799713976, 255515666, 165430761, 651555562, 1416534893, 1693602692, 861160007, 1101768836, 601419071}
,{1773003692, 178361050, 1780249129, 1980663964, 969094418, 846299946, 2128289242, 1450511094, 1964654088}
,{2031480822, 1626856687, 981386796, 895437040, 678441515, 1130586764, 1015178158, 325376320, 564559803}
,{2099988055, 531242415, 1076848431, 1717373957, 1241508917, 1406859331, 1765563671, 133059031, 228570615}
,{1766548823, 2135509351, 223852433, 816812207, 1191917316, 1839339810, 653825323, 910095891, 2054499760}
,{1561145827, 1866481927, 536796646, 1055885545, 1576908026, 2071700164, 241198318, 756717769, 130940801}
,{1915527009, 157974833, 455903331, 396695582, 1040866708, 453908021, 404409706, 899397279, 618703478}
,{116748050, 1681313444, 1842888348, 366685398, 1573762805, 566709803, 1966662464, 107850265, 2072886124}
,{236858992, 1757542742, 2106078684, 655007348, 1200363599, 815648636, 1391104932, 1456610810, 1764291639}
,{207824018, 185053237, 766903978, 335889893, 1895817367, 2045050113, 1447000809, 795807251, 650917188}
,{1219914349, 2105948856, 736784257, 494849438, 1616691577, 497862828, 1421525726, 641560528, 103655204}
,{1142084744, 54620956, 601089684, 103397527, 2145023230, 84388025, 1108424787, 467068128, 693059197}
,{1188032267, 886832692, 1465072885, 1661600884, 1411451547, 1222640675, 500277737, 1164381129, 1970285793}
,{1140172947, 1090754644, 1666430898, 288232629, 1513200846, 1443313389, 2127638853, 939423401, 1291776838}
,{1604373154, 1106112956, 761791418, 1501956747, 1706466831, 2047820905, 581925013, 546663716, 1085637801}
,{1853782078, 1192851194, 2063954885, 520865268, 2135142182, 329279102, 15971917, 193152550, 993796966}
,{571527497, 735828936, 339819937, 1902193038, 713925628, 213671765, 801905731, 655158391, 1665736197}
,{107984311, 1436288300, 236409412, 938601339, 2130000344, 432126150, 1158907083, 855106407, 434848731}
,{849594010, 70026723, 1465812277, 41451631, 1229255616, 846410643, 1491691099, 1559469211, 1864557398}
,{476601236, 435150740, 527392280, 768776246, 197274718, 1986327225, 369048663, 961731761, 2115526431}
,{459109589, 1695901038, 157999622, 536609232, 551936406, 354096676, 1752052703, 245513333, 1861871110}
,{539337691, 618999768, 1892050151, 918459819, 1598937272, 1611347582, 1542826862, 593218782, 1606059063}
,{1182075734, 1304994973, 264493927, 855695580, 196689728, 247023372, 1466525935, 1953024238, 1445866527}
,{1752619196, 367964629, 282534689, 1048153728, 1983685178, 1472534828, 626140958, 614256028, 480205380}
,{1114619587, 945653598, 1435784, 842563625, 904316852, 1307156019, 1584040642, 502897765, 1265173546}
,{1599483851, 617912038, 1434649377, 1369293689, 447911053, 1239208696, 571429604, 1598790057, 1713695633}
,{185088603, 1978003265, 11282593, 197066674, 445244378, 87861154, 1483769742, 2039979276, 1449238549}
,{534965257, 1317235924, 1012469654, 880529159, 813860933, 807875537, 407590669, 1704240066, 861622004}
,{1803385746, 1606850956, 664186598, 1716771587, 993212991, 1619911039, 2137327199, 861332885, 1228617393}
,{823241476, 1507551028, 1844346897, 2042776322, 196584131, 789919782, 1782623338, 1204109789, 741513346}
,{2037765786, 1932755841, 196958814, 1422573998, 1764113388, 1863782910, 618376619, 1098767999, 1272170406}
,{811175941, 1056848704, 2057388612, 273125696, 957170543, 880311307, 353415284, 578927076, 325049337}
,{1477467625, 446725857, 1042479301, 711868445, 179722401, 466483100, 879210216, 2122131787, 188595552}
,{1276769037, 1306150225, 592356810, 1968222601, 111518872, 537158515, 2014964065, 1799856090, 555311064}
,{1662677927, 2003078838, 1497946111, 908744809, 936099677, 25277193, 737479817, 328504326, 1465368257}
,{944313267, 183443236, 1067084131, 1647776311, 1951040634, 604552869, 1821858268, 1869119376, 867352123}
,{1987657701, 1825000652, 934543979, 1412438502, 1763624317, 1236694639, 394326673, 522804890, 506900782}
,{1280547324, 2145349823, 1122795051, 1315318750, 1726585498, 2126654989, 1344833446, 1488872138, 990731885}
} /* End of byte 21 */
,/* Byte 22 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{830479910, 1794659137, 289793637, 1628702739, 1294382859, 430249345, 676077278, 1177327327, 1633937571}
,{540083923, 527919552, 1119740234, 1719527232, 1989259573, 661012726, 1532630874, 538861332, 941664648}
,{1471566037, 390282643, 1881089938, 1531882762, 474769918, 1700595630, 212139644, 1887130616, 1532261120}
,{951243526, 934367116, 944980615, 118049640, 222315535, 1245676976, 79496944, 519722439, 2080522146}
,{830107389, 713601975, 83741652, 1948337523, 1198914653, 1389432030, 671182265, 102810995, 626059893}
,{549213751, 2048939614, 1849102329, 412342094, 741529438, 890347160, 1537480823, 196346353, 2068672585}
,{1872133274, 1267515442, 1319100510, 811837676, 2051997894, 2115507349, 1545040579, 163699291, 1028335501}
,{688238414, 469254966, 853653465, 555686741, 1935881502, 1731610074, 1750828492, 1231386957, 1182773520}
,{2093115007, 862460879, 732908368, 701585765, 1246735135, 881489903, 746509449, 1406675099, 647024668}
,{355789465, 1883703385, 2138479202, 1073227722, 415876197, 148564337, 1764359137, 133579477, 47000833}
,{1254222562, 971285188, 306363865, 2003146107, 827088961, 747055656, 1762881303, 1708260198, 14449527}
,{1806893704, 1722011289, 1454296078, 278460709, 159580419, 1519095222, 1137361115, 1490997054, 1140970003}
,{394286648, 160240476, 120641989, 838061518, 1997496153, 881233608, 671203685, 1140011140, 1598492254}
,{533807328, 760261689, 1702564310, 1179569014, 1739296838, 1432422583, 1523338953, 2042264751, 58136534}
,{234601043, 937402147, 1990664135, 184045859, 984300354, 1177414329, 397453114, 1548175822, 657187181}
,{926698163, 519333020, 28578128, 1228303993, 144500496, 517111210, 921331028, 1764831592, 815299627}
,{1246052069, 1970776477, 1584607011, 1751792780, 977333426, 353542247, 65002854, 1084796687, 1349273862}
,{2067846377, 1558753257, 400174012, 1177918721, 1373094423, 512884478, 5916080, 1175848093, 209928314}
,{1728576597, 2027104741, 1763426137, 2074717360, 1196214783, 1099980385, 1915303914, 760115896, 500677490}
,{203598501, 1143511129, 22492132, 1435953811, 1599678251, 1179625836, 1327610007, 1722909585, 1533530808}
,{473893174, 818335314, 240208293, 957228643, 1862867769, 8227609, 1203696271, 1717050954, 617654841}
,{1675649207, 67315705, 1990020985, 1283666075, 155606393, 232457759, 1801286587, 1404835046, 308159221}
,{423614230, 182423525, 222519908, 112730058, 14536198, 1107900368, 693748238, 1802653457, 1208968588}
,{433311750, 985226303, 630809646, 1856831059, 2101697261, 1907309014, 1466490044, 1773056273, 729508289}
,{146795205, 1603285531, 494055434, 196836926, 1038842035, 2088778121, 2017840921, 1745450433, 1823798754}
,{1892649493, 1984277645, 1742296585, 2139912558, 1563060490, 570231046, 39477166, 622856997, 1022937994}
,{1272725399, 471178143, 1687882299, 1871022779, 260590717, 2065833718, 750092133, 856871256, 1678672695}
,{97172064, 763608884, 1065046172, 197250769, 2050635722, 1159095391, 1840478791, 734199198, 1578917638}
,{151334233, 1998617687, 843899651, 1114139732, 1457385697, 1194031429, 1392356292, 1915305218, 572280506}
,{609206370, 1669554855, 1255737995, 2081496231, 1701193204, 382590802, 1336579579, 1506319196, 744484756}
,{2008840430, 1341323772, 1800338829, 1906808627, 201649625, 1711971445, 1738019735, 761630808, 794072301}
,{119364410, 1352476599, 696531199, 1047088305, 908002355, 236957232, 1902086356, 2099344998, 2105966888}
,{127564715, 109412361, 1051721303, 2016589246, 591884813, 1675377446, 1109584291, 661867103, 682353874}
,{954468869, 744984898, 54778966, 890308668, 1882341890, 1377288435, 1183340749, 1442507047, 2129083700}
,{1625499109, 647515480, 1589668309, 1848461757, 579753435, 1044958433, 472313581, 1790526186, 597314364}
,{413688924, 710609570, 1081533800, 1124465942, 1590839706, 995315176, 303676759, 910804894, 627812899}
,{2739715, 1174115769, 1725025111, 1713449178, 46729170, 636285957, 1180202479, 1193004128, 488171184}
,{1886905752, 1678999199, 1829477097, 841281027, 1793257772, 159588727, 377756672, 1997556380, 1094113039}
,{658477306, 249866384, 899029767, 1325173001, 997735747, 663644421, 774128402, 1268976425, 1090464910}
,{1395827617, 1083091364, 1611222277, 1314534429, 257680263, 272429151, 549504433, 370588601, 235348435}
,{151298207, 1593011344, 685520671, 1050961967, 1292696313, 144192601, 742427443, 1113234909, 1869431736}
,{1991205298, 87751359, 1614597222, 1372891529, 416534837, 1323564787, 1902968823, 1028974988, 915387050}
,{174775029, 2063181017, 636579845, 678628944, 1913950820, 820893751, 1530383038, 2075730163, 1509567810}
,{1559974870, 1133280966, 460734300, 1571996614, 95838675, 1661031585, 416244157, 1104188612, 997862849}
,{1626315773, 1575052584, 592625487, 735628179, 543341018, 254222787, 1255266788, 1155278262, 353741991}
,{1202275217, 1924221716, 1713508067, 1506701724, 1571148157, 1181302843, 1622174619, 831566425, 1816970820}
,{653308745, 1899336963, 302113335, 1214238862, 438502544, 60066167, 1032709453, 101171277, 80970050}
,{1603157337, 419743205, 1130002512, 1235035434, 1126762454, 1683999237, 218480232, 299573894, 377666497}
,{1795423893, 337065636, 538590492, 2077125000, 17135380, 1104531644, 581501146, 370361046, 531754108}
,{912136132, 1616262699, 1396547787, 1646958236, 561336963, 438021594, 598995135, 900439027, 997802580}
,{502739012, 590557541, 550756926, 1795627020, 189158920, 1703089075, 1565288485, 1784646962, 1926764495}
,{1694903237, 495449421, 940319988, 266835384, 673202244, 1727594388, 1194655411, 1029821996, 2030751463}
,{348483118, 1714285561, 543182328, 1586920940, 399863752, 1377568105, 597904403, 1699776946, 449186915}
,{1592566977, 1735774939, 608732883, 267587144, 111554345, 856253950, 1382785824, 213108835, 1647578988}
,{665440179, 942202384, 741614749, 675692183, 681869792, 601924139, 423567118, 312695327, 1324045704}
,{844191310, 1609227659, 1712198879, 283303099, 843689550, 240018877, 1306069725, 363049607, 1468241464}
,{108420809, 117126116, 1432508087, 800522866, 1879745252, 1045546474, 422321727, 1404329538, 1173481549}
,{927071622, 1577656080, 665938096, 612141990, 610131262, 1467929377, 1977523914, 1423247173, 1507859122}
,{1164144893, 98758678, 573563279, 1812933770, 579949690, 1087172336, 1911039879, 1036695630, 1290848043}
,{1419689520, 1278832722, 138628911, 1018732223, 1129559471, 781941390, 938691248, 1549183745, 1124216072}
,{384031924, 1953628520, 1242047610, 729279779, 2032759976, 101230429, 25500954, 76109351, 1039754062}
,{178897876, 583290460, 1663940582, 1948192131, 1965137330, 91179474, 987782672, 743474737, 1382798625}
,{1415503536, 1413709086, 1265646137, 819967411, 791233566, 488786119, 987196813, 1870312897, 1961985152}
,{1105367358, 1433790011, 354681563, 509628073, 377031513, 832163071, 1244540494, 1577277453, 1220848775}
,{1666027017, 606571375, 61747344, 46326611, 84096564, 1848823019, 1454768752, 825647736, 1513106774}
,{1786876702, 774198831, 1588353698, 148071126, 532193202, 363540350, 387372752, 1300980851, 1257965910}
,{2123304298, 1569421683, 769537194, 658339521, 2110402934, 611516814, 2058744862, 152527184, 339031502}
,{1533994889, 663195024, 579098314, 618659957, 1973960116, 1092664454, 1699904308, 882686908, 376529510}
,{1184888108, 1167878240, 872398056, 1065754656, 355592174, 1612160861, 415515937, 598201531, 1327337644}
,{610120446, 1343851308, 1447107842, 1776569873, 1162956082, 1774554246, 1470258950, 188237417, 774950439}
,{858926677, 1868470831, 995432248, 1762797691, 914742031, 1245723947, 1311048143, 1626053388, 1204616804}
,{1751325573, 726727920, 389258292, 1860118361, 1828348140, 1509628340, 2008372020, 1157188154, 779087192}
,{1415585880, 1969655639, 1503645471, 1989936937, 1918959186, 1474200581, 1095049450, 532927806, 229195901}
,{92258237, 929544120, 2111514120, 1422055004, 185565996, 575131190, 836476380, 1129288271, 971431107}
,{14493679, 1008319722, 274475989, 356879077, 1265070500, 895872752, 1249410714, 1422823880, 767082142}
,{1123184068, 1969274095, 649956655, 768333694, 1879793998, 287669483, 871394883, 785468032, 745886728}
,{457047983, 1429263824, 769437512, 631273986, 1473188251, 1591419577, 2037801760, 2065527017, 428131248}
,{1267615435, 163145016, 1834984144, 124260122, 1236845007, 1330092349, 188335816, 1776329504, 1044626410}
,{851591291, 1224218367, 1092723974, 1299628051, 395935534, 2107277421, 462551059, 1030360998, 1968044467}
,{682822368, 855225039, 355446847, 1296647157, 1297434638, 1475230669, 805483754, 370482749, 2134133317}
,{948448201, 151050427, 2142455967, 137865529, 1349753076, 2073951386, 2060818076, 1134566120, 952654934}
,{1507091096, 69138514, 1312072972, 134118789, 1587353492, 1259643548, 1730191189, 494919514, 769886568}
,{2050992938, 1942722164, 55675332, 539082626, 274934457, 1010126271, 883247143, 552068673, 1477574792}
,{1785521297, 539837085, 1043287252, 810107827, 485111751, 1740265731, 1615310252, 1533239427, 700097228}
,{1403278000, 1322797936, 2110888885, 1396977682, 1107954009, 831650636, 1390314433, 158748354, 408146991}
,{1038993429, 611702005, 952251962, 860615678, 914354468, 1214421946, 1171449375, 2061221132, 1481417260}
,{1861402283, 576671087, 1271644758, 1877481974, 1382549769, 247302835, 648217854, 1006360631, 63955907}
,{1711373545, 13555846, 310515214, 2001084780, 1699571227, 2098929664, 5386561, 2106573666, 1606515538}
,{92010044, 1175282307, 339245895, 1473761777, 929241556, 1572575521, 2043702292, 2046956163, 534929446}
,{1034985785, 1101727169, 858547236, 456714252, 1869253829, 1276353056, 2129810584, 140777277, 1497859235}
,{921082648, 278635969, 1113905648, 70280158, 103774094, 815250501, 935942155, 1353313139, 1670223960}
,{1703198245, 1654789240, 1612628120, 1067887378, 2116410673, 1595540766, 1698433946, 1117680591, 758854825}
,{1561777283, 822737917, 307754131, 861905292, 2095799122, 598238415, 1708983421, 371135277, 1643779228}
,{1150819634, 1360935809, 191926246, 181126879, 1272549820, 313903843, 404182448, 898656481, 1065394234}
,{614711053, 1143460231, 546263694, 1320991662, 1479497290, 176692086, 1128666558, 711210002, 1445322154}
,{1885127262, 2033016511, 640400269, 1998366560, 512060373, 1587970334, 1587849205, 1979490250, 892980609}
,{1002960973, 1090973987, 1426730446, 359951904, 1899563709, 2135802017, 357077193, 1630839257, 1006647422}
,{207627164, 300717896, 1129594457, 1514215034, 855064609, 657545118, 684705301, 1381942361, 478758965}
,{1877837215, 526209521, 1561673863, 1785981862, 1044933148, 1479579231, 77453491, 792505499, 660182041}
,{2006663059, 2115507187, 1094225165, 583146452, 1825561555, 1305800934, 2140433391, 1015110771, 503905144}
,{1421531561, 2064631581, 1620317295, 1420277965, 2033495095, 1674469717, 712130347, 1922651620, 1360567028}
,{1412904878, 530787894, 607248398, 406381994, 1156803080, 1969006469, 884463775, 1707432408, 1418195196}
,{1007407696, 1148326583, 1161196814, 2142717923, 526781056, 366237160, 1033013808, 492672902, 569093905}
,{2006825592, 311438556, 1440727151, 1640426474, 1657355540, 897539787, 1878207502, 1197802213, 486505489}
,{830530912, 1204425871, 1221824687, 83468612, 1692119594, 54746593, 451044103, 247725723, 1927538138}
,{778201839, 1902572069, 1093876763, 1400971458, 1001721613, 1522827243, 883009775, 555047125, 1344326031}
,{103161635, 1688004465, 162374807, 1500536189, 217462495, 1115026981, 2026973193, 39476295, 1814258527}
,{865436769, 1850559756, 883415135, 1125126024, 252289238, 366540881, 862500009, 1789618662, 1988154718}
,{1177514251, 101701883, 284201607, 358641573, 230124138, 870687901, 86835024, 757746044, 394409752}
,{407701908, 689730612, 1913171901, 1926112295, 1683773487, 1406028403, 1743184507, 2067616994, 315862445}
,{323378752, 360887981, 1225572632, 151571104, 1410193449, 1536195606, 1332149264, 114815591, 403640351}
,{171996772, 365209489, 1739215546, 316634754, 2103579737, 1593500807, 1933109540, 1787530062, 1692692580}
,{1631268827, 52146631, 80504886, 1509665161, 300447645, 232821818, 1931752532, 2122041963, 2027339152}
,{955236179, 1755354551, 563438769, 1827894597, 1846135829, 1313647706, 747907166, 62888536, 1394628112}
,{291520790, 582791811, 1659533092, 2122295055, 1308578444, 1554501358, 900709252, 948053358, 762011404}
,{1131153277, 597485562, 1787466697, 54449831, 1553914431, 2026943015, 541165730, 938783649, 44224148}
,{1007373967, 1394549788, 1185339310, 69888589, 1328463343, 1793588176, 128086719, 2065167702, 820403012}
,{935812399, 1563916446, 1820244427, 1382858184, 704943540, 1129007924, 1854588383, 1055874797, 13264814}
,{2124070025, 1868545868, 663950994, 1149141341, 1159361613, 1661992271, 1263369705, 1243832856, 294523384}
,{1847344083, 583684034, 601511842, 1529258120, 1664220857, 259236152, 1799269377, 577391291, 495766264}
,{1117857865, 1456059434, 901611712, 873310541, 2037099180, 1601680093, 1736888050, 1339141547, 172535268}
,{188286737, 431354712, 1777559097, 2140104720, 1711336224, 43331807, 366528594, 1367574618, 1017936743}
,{591636248, 1456948453, 387589203, 1614347911, 416967271, 2108869225, 186355886, 45884798, 186159227}
,{406071277, 1794976176, 899271194, 1047509939, 1178479706, 1330728682, 648264520, 820617357, 569141084}
,{136437532, 1074099337, 1257204668, 1725917362, 982464671, 167352742, 1413379573, 479615235, 277728427}
,{1235649003, 2117584289, 18674078, 896495507, 1258798064, 2007229685, 2000498247, 947940397, 604896378}
,{1720327279, 1421196901, 1069349816, 598038703, 953353738, 711708171, 2001367962, 112308281, 1021420022}
,{334441850, 1422582910, 777315810, 645645681, 684640942, 231786439, 588816374, 1942798503, 326784013}
,{338733442, 1488985052, 1254329816, 811989752, 263634393, 1585176712, 1988008155, 1726799633, 1055118892}
,{146070469, 1944209267, 1817877663, 1291733162, 1169191002, 711968597, 1246566107, 1607054301, 714096179}
,{1493978590, 2140142484, 1856200646, 637179326, 794860502, 543508154, 1021727698, 64826267, 1523790585}
,{4543481, 1736493404, 1246711802, 1368190953, 635413685, 1842277368, 1460908182, 1071621454, 1763584729}
,{219623360, 220663945, 1184234835, 148962359, 620270765, 143795769, 2100273957, 1239227574, 1689779667}
,{1388286736, 459572982, 204387885, 500496776, 602813831, 1691659542, 1974539057, 1634769206, 674889703}
,{2061048776, 1395128411, 1209239070, 523951643, 991078564, 244264610, 50004633, 1902314392, 1613758715}
,{2096500461, 1508763888, 1118869898, 2031893725, 1274128692, 882362909, 1776410521, 1517524225, 1279866125}
,{1531882368, 9788234, 283266980, 581389271, 1243296876, 332622864, 596345707, 1124287550, 1923538057}
,{848700317, 323296748, 2005544336, 254519431, 1363663004, 1851612737, 1035357331, 1073260371, 21654233}
,{114892097, 141467705, 1919133423, 1511742930, 1495359595, 1926616571, 278449982, 1629033801, 1032571947}
,{2027071249, 972643663, 615609688, 1149164757, 437087921, 1229536367, 936891236, 391756095, 1910586023}
,{775059396, 1371226372, 1354472501, 1020103322, 2116029743, 670458854, 475566808, 1712648398, 1301854439}
,{661609872, 697035997, 1008333910, 1186219616, 974973492, 1753962730, 1468188778, 457605179, 707437497}
,{1615443382, 762533388, 1826460516, 512778746, 735188590, 411876569, 501187711, 1518479053, 1599585292}
,{625938588, 1365575644, 1859907694, 2020929969, 545264277, 2005490597, 835863438, 1513629401, 122868169}
,{366521516, 983789778, 755811413, 25548785, 58498973, 426010518, 391959367, 639982283, 156627721}
,{866463066, 555254796, 1836752149, 968575157, 1765089604, 896237817, 1200946366, 1192489023, 1669840993}
,{667359018, 1433907689, 492208916, 599829683, 1614142506, 985583668, 1006248091, 1428758857, 791954359}
,{1946611151, 1168053657, 1611509800, 178538300, 1304275065, 1667855760, 1027760284, 248318930, 143621616}
,{1803066941, 396888298, 124768763, 958612159, 317978988, 2020672698, 1350268601, 593392331, 1291407678}
,{1431970428, 1960118937, 286945424, 46508697, 1193937006, 170439099, 119917557, 1829898652, 1841962666}
,{1534160090, 459482943, 1208941327, 448859428, 1625406261, 996268735, 323376358, 120929338, 1368332628}
,{987184589, 2068378457, 1361446311, 1144394185, 343609366, 541747845, 1708705477, 224224721, 372504896}
,{1021333437, 949815666, 940732137, 1008635773, 380658423, 270226416, 416656162, 1077554481, 110888537}
,{2048439308, 2123294626, 1732841234, 839165398, 1631270631, 117850680, 1691593496, 1965094592, 84494065}
,{689087958, 799963119, 1435019032, 604385516, 731958113, 344033969, 133491137, 235541071, 1830634744}
,{1196425110, 1667090323, 1414617056, 449063799, 429028662, 174599711, 387139516, 2031551886, 362230596}
,{116178628, 1550148079, 1049149684, 1190388150, 744035059, 220995371, 5433663, 1510608915, 1825934674}
,{729239467, 1203142640, 667405974, 1157295213, 991137239, 1699528103, 394693685, 1556023335, 2057141807}
,{338666116, 1889459843, 1616135438, 13518278, 1826633619, 1341429973, 395015671, 1056378799, 1349526857}
,{1367986244, 1211852374, 1693169944, 253203791, 583771287, 1162553918, 1071527708, 737162552, 1614254582}
,{376844919, 1105747048, 1195358271, 323134243, 619648152, 866477144, 1321588000, 2008062090, 990568244}
,{825575175, 341752316, 447597726, 284009899, 381386935, 1663413740, 2012886564, 8996331, 1559354225}
,{1267786832, 930849023, 1685917751, 1870674690, 652023321, 1775613820, 1064452914, 853871076, 2071155362}
,{1966413479, 561792163, 615149423, 1014364710, 242575016, 1913656910, 1019111328, 1516669204, 141237524}
,{1407755186, 1870961708, 1664288276, 1635685429, 1225447518, 2022492487, 1708035182, 1252621480, 787030000}
,{1943664148, 1102949034, 689226279, 922822800, 999920217, 1281660041, 348019447, 1552635270, 1530239696}
,{1202072895, 171965954, 1616285377, 1723293528, 1216360695, 1361853176, 710140036, 1045247786, 1494769064}
,{1601760195, 1698923897, 1812104757, 199895564, 1877820805, 601436917, 1279479289, 718445454, 982119802}
,{1080689250, 159990439, 346456087, 1174234879, 1029692080, 855491025, 583905140, 969868080, 690238252}
,{569646490, 1436551375, 1542491571, 1341486187, 529483043, 163433280, 1485289923, 1143757261, 236542184}
,{1012087154, 366784632, 1299450994, 1233835714, 1289163429, 1515792681, 876297738, 712383141, 1147203512}
,{1430867445, 1320989701, 992386445, 467337283, 1910851804, 1357057007, 1269035769, 989542405, 2101382874}
,{84492138, 1638430421, 1094479344, 1066781652, 196642452, 1394109808, 1465534370, 1627586446, 271232290}
,{1158613679, 438080924, 456753764, 1147557183, 333914710, 2072675601, 1986175133, 1848260257, 1512384720}
,{410649521, 1767867111, 2065987416, 1345643887, 1571011079, 1731513961, 1567331712, 2085567976, 411565558}
,{1625450648, 609198079, 1004434185, 980950739, 2129520162, 899283811, 1870800857, 4047892, 1269109941}
,{978611523, 2040130666, 1005500302, 774327012, 775896910, 2019739180, 298591589, 1636187597, 2028592351}
,{602938933, 1240954846, 1532935432, 1460509605, 711757975, 278306943, 1757650549, 1811699554, 1580901684}
,{1450230319, 2113584776, 994388662, 743920844, 592298277, 1498629982, 684616533, 1900169428, 786232436}
,{529456683, 1520308682, 559847032, 1353083583, 854088030, 1657121390, 1053596369, 1950692495, 1781958392}
,{1036471653, 1761679983, 1439039400, 332557107, 973248084, 647295628, 2071479389, 573906962, 987129012}
,{2111219964, 581071661, 338558280, 1415371499, 1528402998, 271641403, 565606336, 787912552, 393640146}
,{411680946, 70445163, 1992855299, 1527716500, 820879940, 570601926, 289906072, 567255916, 192843640}
,{1688968240, 1266419701, 1882478328, 759186754, 1031195358, 675280817, 1324007495, 1906396866, 1008201549}
,{1535908222, 1882589086, 2021181854, 1458964278, 1012901858, 1559769573, 723643227, 1957308425, 864714821}
,{236739784, 896782404, 903205511, 1331600710, 361973117, 1243420209, 1434376079, 722194900, 109204902}
,{1739721671, 410018027, 147076909, 1248235822, 219490246, 982231448, 662289361, 1123808728, 685150650}
,{85764848, 299562260, 364340767, 986755585, 1384087083, 1128538022, 184100824, 1351817026, 1555196218}
,{164113992, 2019512638, 1391528984, 484936490, 138309881, 1103079282, 1220199600, 57866630, 1191798384}
,{1825461910, 1014851002, 1325432199, 492004062, 466153470, 1449681157, 495921247, 1070050902, 306384019}
,{1937485265, 1231518939, 1687670637, 104298812, 1692795134, 1090270008, 17585946, 742388825, 1969829957}
,{372894604, 377119619, 1592143637, 186653414, 1166221341, 676103237, 1033384957, 1650329779, 481231736}
,{853274347, 2119911166, 1293473317, 1555040403, 1828403164, 425400774, 500337952, 520928661, 1753452315}
,{1724077601, 1856770486, 2098897721, 1843324, 577992776, 1360717508, 334156405, 421759494, 1933615506}
,{7982797, 1229780803, 2124003976, 335663937, 1568901979, 904864277, 1485178932, 1104341499, 1075008272}
,{592424435, 1187401829, 927269505, 1947405388, 430141622, 443182365, 1309026589, 308130076, 2040283013}
,{1045174844, 344260368, 356323779, 1378487667, 879268837, 407945902, 428291078, 1013837425, 1061296650}
,{1329624230, 1097596352, 762976016, 1064529491, 935869885, 1705969695, 1776499358, 228006351, 2037183668}
,{1559721133, 621413895, 1390823575, 423961225, 937415881, 1471375869, 1842209662, 2141419855, 1798531667}
,{958679605, 1022533784, 90386964, 94099148, 2068306837, 1223866834, 1165272125, 223124816, 1560716422}
,{1126945847, 1601215724, 1080865381, 1141636527, 596699167, 1958403954, 1898662550, 1550846458, 1986638189}
,{355839794, 59860888, 317615418, 581206083, 1967756661, 1438555513, 1209985359, 50337025, 339525967}
,{2104547316, 622052528, 886124417, 1499384650, 903512531, 1603447885, 861788569, 16642645, 1434558517}
,{508635576, 920672475, 840119608, 1524002635, 1513994801, 313153294, 86270861, 1601182016, 524084366}
,{1481645114, 482914367, 901899466, 1703276169, 1492301707, 1467577130, 1461989294, 547334822, 1515482884}
,{534973085, 1345667760, 1323465382, 965860477, 1312897079, 1695298092, 2018601238, 44601679, 1189912309}
,{1400173070, 1735939325, 1105259136, 877852332, 4584412, 172136927, 1796630488, 1108025120, 1764259267}
,{1665639677, 2042064294, 223934287, 91664479, 1519985383, 1136967860, 1973479183, 1870552959, 757917665}
,{126715201, 250367910, 47831693, 800350204, 2004534812, 313391772, 1226634761, 478402220, 1837094035}
,{1302977872, 1317907557, 1951517368, 1903679930, 796093997, 2871843, 125151123, 34515937, 461890872}
,{1464537992, 1979444687, 1447341521, 2113186487, 1355682163, 607862931, 268221854, 1375063744, 1303906582}
,{1485840193, 1475890408, 1173702353, 904234729, 1105983041, 204227064, 1531719610, 1441874689, 1567694541}
,{1974965931, 1156870300, 1136643490, 1957566738, 853658558, 1646748230, 1634023433, 634039260, 631744817}
,{1180179895, 1993179254, 1380725500, 33820388, 6708911, 1043245379, 908215435, 1326557721, 1797271538}
,{861765981, 1185478730, 1790843923, 1406998294, 1565844369, 991234819, 1336537554, 294965056, 1033109870}
,{981282970, 1467603974, 537952221, 625097406, 1442251013, 50413111, 1701423638, 1962334415, 218563056}
,{2089015022, 1269298916, 1108004601, 331524876, 1665535774, 752892023, 1166614940, 2070693294, 296548027}
,{1776120468, 1086775605, 175375179, 761119834, 1821851424, 1324126900, 1859414411, 1291440796, 940350416}
,{373886771, 107155443, 702293514, 627324680, 22400152, 1157411079, 189825454, 2064124324, 1876937015}
,{2017966742, 2110893402, 2059568504, 1325318973, 445777158, 1619353407, 96930441, 1398767501, 63582715}
,{698468373, 1047798652, 1580668213, 1263205341, 1832881092, 51715445, 2099438719, 421027607, 289657059}
,{1347576704, 338861215, 551489723, 1809912034, 1951451442, 296706098, 232894224, 10412138, 1733058829}
,{90484426, 911378429, 879139009, 761852382, 1288206979, 1523953974, 1848993671, 1295820603, 1156792315}
,{611530620, 1589477392, 1311482304, 144782464, 632887921, 1375441675, 293206806, 686405176, 2110633027}
,{1962297046, 1877546621, 1290826117, 238259988, 2002598116, 1834987749, 1614948046, 317273266, 388313920}
,{1215471404, 246141506, 1094101248, 246051632, 1294589770, 250223244, 1436019842, 2105676699, 450228743}
,{1842165803, 1859429123, 688847115, 1622599279, 1842304862, 1536793639, 43595159, 1499966791, 350948844}
,{529983345, 1520035700, 2008171575, 1638555726, 78831510, 1871412441, 1460551403, 1449485282, 1933743673}
,{1654436478, 1715019868, 1823466128, 1597511773, 1572237529, 921218736, 1071542841, 1329845961, 1214165625}
,{1732046604, 457339394, 1671127494, 1527116715, 1556033723, 280619812, 713917101, 1384619912, 212423295}
,{1943370323, 1321452720, 402708920, 29080080, 1750031760, 963395654, 1568758994, 983252985, 1942764127}
,{474131943, 53148639, 203211655, 2090539731, 1153452664, 209325006, 774192997, 1497449635, 92474380}
,{367478457, 924867797, 527222464, 1610723125, 376089466, 474127790, 1435019561, 1215840461, 57919487}
,{162788913, 1714548435, 962281087, 267421818, 820671693, 905953039, 537823341, 186557831, 1936492458}
,{1343801416, 1970730414, 1700717717, 1933090570, 1957651046, 1070903189, 144785595, 1345544700, 1486710401}
,{1526316849, 445796063, 475198985, 67281223, 214852626, 877952807, 919534779, 1785553515, 1200521631}
,{2026884378, 2128665958, 1590432330, 1340596187, 2109302952, 1204508061, 1276228691, 2075032151, 416674058}
,{328043878, 293485072, 1709411464, 1853979371, 1664392685, 2111404997, 648297168, 338585174, 901642780}
,{1128678366, 510216607, 1869453990, 1864635551, 1980238505, 759424273, 852036218, 1351674510, 433410603}
,{434119110, 472176415, 1901775936, 1247282641, 1731355489, 1393976550, 340142320, 1128232829, 155357993}
,{516735143, 618396371, 1112354534, 1583323462, 1847013542, 759661618, 631724603, 1684342398, 1042745338}
,{1411170895, 1732775980, 306884712, 724216300, 1271508797, 701995255, 1671799108, 998080071, 2103296778}
,{317758836, 1497425249, 360899174, 193096637, 517600830, 956712927, 777668926, 565157607, 1071414944}
,{681309257, 775246349, 2110814126, 555233250, 674139162, 214665562, 102945897, 197079639, 497787106}
,{960497128, 436742629, 198883878, 1511075894, 1510298322, 959562093, 1258901516, 640963634, 641715956}
,{1006042321, 549151852, 219555950, 726102885, 870890388, 304411222, 327760387, 1363006026, 1038083373}
,{402331789, 1182189242, 1140202379, 1758581032, 290680438, 2007740757, 115370567, 322336963, 499985048}
,{996793298, 1415403507, 214717054, 579238471, 1504214356, 851777488, 775588392, 1596272722, 1160267268}
,{1342569667, 1480339500, 699908198, 1107694609, 493121935, 1762059393, 1479051781, 697400668, 1108025160}
,{1879927132, 251242985, 1864168841, 856314330, 1676369704, 881758677, 1233430757, 102491018, 1425749483}
,{1511087953, 535460625, 836818987, 1771539554, 827163641, 1402444212, 1634453701, 1306854941, 477138594}
,{1838799398, 297249360, 1580537134, 1804756305, 1737920113, 1586404853, 273676159, 1601595467, 1928980976}
,{339897586, 1731634858, 1839951536, 1122613228, 943591825, 109432150, 727908460, 867845267, 1391465258}
,{996573669, 868678533, 222252578, 465445913, 1717762051, 1486245390, 775808515, 1804525668, 1945171526}
,{290341348, 1829182561, 207777404, 1814595121, 116013322, 439072901, 161476071, 791710855, 1449527258}
} /* End of byte 22 */
,/* Byte 23 */ {
{0, 0, 0, 0, 1, 0, 0, 0, 1}
,{888704518, 1662553068, 513723693, 942116488, 754406834, 1136758122, 330606715, 289533226, 1297315299}
,{99251579, 1760376624, 1213006780, 807232700, 1644350103, 1618264389, 1612981885, 779559952, 512510661}
,{1179407538, 1335155435, 2145337203, 51213872, 690716235, 707614432, 1038678693, 1083540700, 311476990}
,{1019164940, 2326200, 1799892028, 1316680099, 1510448879, 1793102118, 957557922, 1196283191, 125382121}
,{68056367, 1260493245, 339132442, 1545960707, 2132509156, 1988292793, 1039094335, 645406778, 1691868419}
,{727710984, 719764431, 1610762372, 929537600, 819203442, 894562316, 868168832, 1914168697, 1974605498}
,{681433799, 1807083028, 1889912342, 2069867223, 2103156131, 1461207016, 948993157, 1415597071, 641329515}
,{795084912, 677868040, 1509515536, 616860870, 1435789420, 1688078509, 1885055699, 1997200840, 736769126}
,{507196177, 550578213, 1906346346, 80136086, 1280020212, 50937004, 170531477, 1845811169, 1600353944}
,{1545257346, 227785103, 171063641, 2115159265, 775498923, 331190126, 699392191, 904004357, 136974851}
,{1501639035, 1631166679, 1819408562, 73426789, 1161607070, 568722105, 9743090, 1143758531, 1942407590}
,{1531916142, 990258340, 435599067, 1340798907, 1997907459, 668836055, 1812119183, 1028679740, 1637869550}
,{1955620485, 725695530, 1129185115, 1872130726, 1096168655, 578099272, 67856911, 882013166, 1431709141}
,{658871130, 369295673, 1629307280, 1694660455, 1062242656, 1212432601, 1312330052, 376351478, 715771274}
,{2021438529, 724198535, 556699324, 1737525862, 1521239760, 983062691, 1393743388, 2108709135, 246320651}
,{862518949, 683738620, 1428753937, 558042005, 1674904386, 1348224629, 1923242958, 1817642252, 179736139}
,{1373590688, 1198029740, 647758785, 1710180622, 1134930592, 1576266458, 2028800807, 1751719616, 92331997}
,{2070278674, 576438408, 2107021660, 1820566444, 1998135625, 1355079425, 22131126, 309581997, 1567659379}
,{800584669, 411967974, 318860284, 437301930, 159893627, 1098130981, 41888623, 744763459, 1939335881}
,{1004711627, 1788865944, 1945913686, 1661886119, 1319546103, 41352504, 213453392, 728183973, 313062436}
,{1137760069, 1177255120, 621144254, 1663744059, 1482973137, 1745406737, 1835612996, 591782781, 1383223346}
,{1433046643, 1773183568, 1519403731, 1834997415, 1432432827, 1593739790, 2035336628, 1981623054, 94619664}
,{1380032275, 490239285, 112948071, 2074534975, 391473350, 1617840724, 2123684194, 687000413, 1546887576}
,{1837425764, 1688530854, 452945670, 332746049, 918463587, 1474928566, 580965766, 773653125, 1184555905}
,{1337571166, 298967127, 1095290084, 1892314303, 1649156486, 475062313, 177398998, 1615476289, 2050107426}
,{1758614940, 888983177, 820074147, 2123827483, 1625296055, 273467948, 1583353824, 1999150374, 281348330}
,{872399666, 573851934, 39869569, 1638730917, 1447716844, 912757145, 257275480, 1548684499, 798303087}
,{1101156310, 1219151290, 2019601037, 2144131301, 395661129, 1907488405, 1888851007, 87169078, 150403587}
,{589084380, 239656158, 987437161, 1401922395, 1113709747, 33476625, 1512591254, 228270362, 1248563485}
,{1901677851, 548026240, 1263078896, 785101920, 1209842352, 1063554998, 676086282, 763220086, 1586960416}
,{685527763, 629702361, 49517760, 18670305, 185181791, 711890191, 981472199, 1715264500, 1732322863}
,{61252081, 1782605775, 747086108, 78534797, 2118619971, 1868541729, 1059464144, 137219076, 1641096565}
,{727622793, 364309418, 373434631, 1724287432, 64005849, 1505505811, 1027623603, 1686741718, 1406381530}
,{1217840915, 1959313803, 1784911662, 739034313, 85590296, 996198571, 469874521, 681952413, 1242771322}
,{935037669, 175740507, 843766586, 1182447044, 179281963, 814844964, 988067578, 621875027, 713244980}
,{1186899423, 830749021, 493118664, 948840567, 2104666862, 737456770, 1308750618, 276029486, 504817989}
,{323608743, 1771773550, 1872263378, 2140636126, 742251007, 1980633037, 856919268, 552224194, 2052211092}
,{1042420836, 859927383, 1544676094, 1954804862, 1643291274, 65583361, 1629096100, 1384621227, 1910293121}
,{341817606, 1743525133, 1731121515, 1417257616, 1067339544, 120650418, 1474145504, 1330803419, 494184165}
,{1604092269, 1164415005, 908038262, 1134280904, 1941543002, 1569350352, 1647255827, 1309360641, 1590640960}
,{10114346, 1044206374, 1294574029, 1057267870, 1212214345, 1684911447, 1456474299, 440127683, 1380292203}
,{672745273, 1725476651, 846916208, 652605907, 649003803, 738396529, 1554845064, 537906864, 890233130}
,{1544788813, 157593431, 299889416, 2117510797, 990033014, 1012781717, 1770776915, 960867965, 1190600138}
,{1241329891, 732538749, 1521573556, 342067072, 358987965, 1479189293, 252238027, 1005403039, 2047206366}
,{1937729278, 1697275304, 1455019122, 441564199, 196877847, 804321692, 314513071, 1512600630, 741209450}
,{39813409, 118344595, 2008381535, 1638518941, 182010274, 1070777543, 1955731942, 1168761126, 417188974}
,{443727908, 981304811, 1344343814, 1372501325, 314011796, 1684993502, 969344453, 281611490, 1289189598}
,{1498321159, 1889361365, 1830085196, 1588493995, 1641130000, 1257574777, 1067811477, 892637431, 362339258}
,{537953253, 671514760, 1238073044, 1093423376, 409478492, 1879095492, 1868879810, 1513057266, 524153543}
,{65475981, 60818738, 333872960, 482139416, 584482953, 145276327, 1034645522, 334761848, 1326309765}
,{287485731, 52589998, 857858073, 311110122, 2029249128, 1133085998, 246529609, 1191192400, 1612331788}
,{2038972584, 1588554475, 1518998257, 625430056, 1721060791, 1304410800, 1062454477, 2078932089, 1959802201}
,{1919568766, 435253085, 514289875, 2008018775, 1062621891, 407649111, 1552728692, 605570274, 206427201}
,{885517218, 883254522, 132884505, 932260800, 566099635, 358575131, 1188499215, 2126898810, 895426875}
,{425717066, 1046532688, 1464113345, 930131237, 1526495938, 1541776809, 1890859103, 590453098, 1105088989}
,{1884818974, 3887221, 913111630, 869905800, 476394749, 847439133, 2012372304, 595072223, 590242018}
,{1587850379, 178076963, 1815638392, 2029566478, 1736513992, 1662438795, 696913902, 2000394977, 714253203}
,{778719155, 57533722, 1548078888, 1782676692, 1553449117, 1395594753, 308001448, 386385997, 1096091471}
,{1804045245, 814288996, 1916570413, 822415151, 1271274781, 719744072, 805666529, 1506799507, 1495514230}
,{379543066, 1215645511, 958639764, 419767216, 1194237004, 1727050532, 1642662266, 489080517, 700936276}
,{1838771631, 241764181, 190086080, 2017860485, 151008849, 779310116, 1635314457, 1123377878, 1690561874}
,{699022414, 175910571, 2140959505, 1186143906, 2117447795, 1434491279, 1531513375, 113243313, 1326193359}
,{1241954280, 1040291278, 2081952245, 808230752, 416621597, 605607280, 1869989697, 1979095458, 1656673469}
,{483314018, 1345391885, 1445610917, 339144201, 1440538434, 961483614, 464846558, 2109183188, 1561317777}
,{1777290606, 808336043, 837020496, 1526802907, 2028695092, 830529425, 1083206109, 2114048410, 215036325}
,{46838832, 1877532407, 752313272, 657330189, 1185748914, 1002145357, 79439850, 1703268065, 1126476003}
,{1146108145, 1268458186, 1131752120, 193290655, 1522371847, 1404191247, 1391904460, 1853607741, 980983320}
,{532886526, 1541988453, 1144989174, 669899430, 226983020, 1999723263, 767208579, 294002245, 660953146}
,{867648320, 1173252112, 1336060949, 1810279440, 133074272, 869575200, 1031709629, 268065064, 468748229}
,{1930051930, 685857021, 1920527792, 364196139, 2116568249, 1084080669, 359983552, 1526692084, 2011722749}
,{1672777416, 1339034204, 1209486918, 873072853, 1140473540, 324065379, 988689853, 1922895775, 2078072754}
,{50251230, 810590416, 327403468, 561564090, 2044466530, 1866744309, 640410601, 444794562, 2103536798}
,{987978441, 1127091705, 6888177, 752984718, 1562529700, 888673433, 1091716846, 1015128916, 777737850}
,{1853388008, 632422193, 1544149753, 180835269, 197299700, 842729849, 68078300, 157007864, 571843677}
,{1967903351, 847180231, 1228426920, 722201818, 1077875910, 1339530064, 1189088127, 1057640250, 1781443646}
,{1708902509, 606578314, 1214329340, 1200019704, 307971548, 974830077, 551016266, 1651409105, 266849273}
,{1301392699, 1375280350, 1409206245, 500364225, 997878489, 812418723, 1446982239, 2034029906, 930213116}
,{666952657, 1581000734, 985236509, 353807397, 1398732830, 1025444757, 304208297, 2072695593, 2069153743}
,{422945563, 1143252035, 228728317, 749846767, 380050659, 1317343635, 1378270780, 267451994, 1789681014}
,{1708915195, 1069935285, 1375736390, 1626509552, 2046412075, 2082427702, 861635059, 1217563099, 1512890808}
,{1648843464, 337040439, 2028585282, 298126986, 799728736, 189441956, 1532783868, 620646091, 1763618610}
,{595144079, 1848891437, 222546242, 1808121777, 1245869441, 1533216827, 437539629, 473866901, 1187390208}
,{1517760567, 1455142420, 702340332, 1423755239, 1181915015, 1675545498, 2048634690, 1780474921, 666122975}
,{1058349463, 146727653, 1413188677, 1077956939, 1020933484, 98427888, 90248728, 1905363343, 676178861}
,{880993135, 907681384, 374681005, 496723222, 26896808, 62462875, 1963431734, 1312760668, 426424033}
,{2092905987, 1573506072, 991051186, 180819699, 268183277, 1592608253, 1829502668, 399487224, 557859478}
,{76393674, 1141282790, 1068115541, 285095624, 2038502350, 470299910, 1977252396, 1311044203, 1901759426}
,{732398430, 2041488792, 131307101, 385159733, 1504905663, 711407518, 1815049752, 1038382508, 2000465244}
,{1118698071, 354493713, 1551111275, 1375467369, 193548894, 1144065394, 506443293, 446592940, 37241365}
,{128000008, 905871116, 580544675, 714500527, 1072866910, 1720064572, 1267012263, 593168782, 1080006201}
,{1772787718, 312490087, 113369939, 2110097082, 1343416291, 1624749668, 403701014, 1324556633, 128202193}
,{743891320, 1043447795, 70890599, 294827184, 715449535, 783085833, 1192674928, 736384418, 242966624}
,{899158099, 2054533513, 1057049438, 842531983, 433361979, 1632453996, 56193810, 1896605973, 1165414259}
,{345121506, 1546237762, 1374818642, 2016336311, 1519275276, 2069690329, 1616794668, 161967989, 281470474}
,{1005306205, 422340836, 1937287842, 55168588, 1189763229, 607354576, 1295217011, 782958640, 915727705}
,{1684350207, 1422469912, 1477788124, 1453344409, 1501059142, 1584566616, 493693079, 1173177649, 2056814606}
,{806992707, 230487941, 153041518, 1462221625, 1958026265, 699659770, 981902422, 1333304142, 1706118180}
,{1293118667, 329694629, 272544543, 1313633561, 1648876455, 628722372, 1340981901, 951664078, 1101045663}
,{858813508, 2109305595, 128799819, 1384610517, 8852852, 520230679, 1986828546, 1333465620, 1689623532}
,{1808874614, 1557598351, 1324190156, 542249655, 1731229644, 280247142, 1440852021, 145325630, 606623850}
,{711743270, 1491113328, 1647852939, 1152238279, 1424200532, 168261360, 1588961463, 918224314, 458576928}
,{1910797016, 1369896835, 1939668721, 336199437, 1042287103, 271621150, 770859495, 1677011349, 1421700772}
,{247406034, 2060668687, 687107420, 1200016941, 302283770, 1329831481, 329040584, 1514349466, 416506551}
,{338850640, 21900952, 2017287196, 501962232, 1344975446, 2050786875, 762640878, 20289560, 1337874100}
,{628208965, 131465200, 126538273, 1033536284, 2120785453, 448462974, 1472814785, 1054922021, 1858936928}
,{78444217, 1357128848, 1530972465, 1094503779, 172289190, 1007550825, 1715200588, 1353177032, 987167417}
,{331450875, 1329099548, 1564322941, 1343604297, 1632335489, 676220830, 253903827, 2006357234, 285865363}
,{1007803743, 1020309324, 802359679, 967660714, 498342648, 989700561, 563338665, 1307363093, 1436629936}
,{943403092, 1541112137, 1910423602, 36173705, 881768083, 351927596, 1357752214, 1064198983, 831420986}
,{1649586584, 1547878719, 1605005330, 321596696, 1359900240, 1400020144, 1544495009, 747391084, 1527810090}
,{1875829741, 1167149339, 756532042, 1972712882, 17583753, 617983112, 1828728141, 1440032215, 1124185213}
,{2082111831, 1088646291, 356832754, 850130324, 101979689, 1541023156, 1535461567, 1064338052, 1017951559}
,{729136700, 2086654468, 2043248230, 1039041014, 1506135822, 1831456623, 1724360959, 328140496, 961789919}
,{452970582, 561631939, 1429548534, 2072545721, 1865254223, 617840600, 1213304324, 2077385937, 816046598}
,{105652976, 511684712, 1005027955, 1747229813, 1020634981, 45209441, 1096578833, 611807945, 1343373112}
,{576209740, 1214898698, 562746863, 1718305491, 212717810, 1013225289, 1174485653, 555616236, 1736239045}
,{265449968, 1698826258, 1655467372, 732288424, 1840525450, 317428581, 1651741551, 1335294887, 2124791516}
,{1253216642, 606285883, 1007015386, 151405406, 996191674, 382408423, 1195294985, 269923073, 150099322}
,{113914044, 714453173, 1856291403, 1224065829, 1959756021, 1104079987, 73999037, 876043619, 1545766572}
,{180089365, 31150886, 257171305, 262340553, 1933668258, 504990520, 80424623, 556314482, 1953706554}
,{653314729, 750776501, 2145900897, 1929106169, 1037783521, 1488483987, 229067131, 882565032, 2017375948}
,{552071542, 926979477, 2065543706, 1693569457, 917728958, 948502802, 1948316090, 2085400649, 1154206372}
,{230357708, 256606225, 600760190, 175401219, 1523550118, 1034974486, 882764214, 1369672297, 722042267}
,{1353698821, 1443641354, 1191653066, 98268669, 52696472, 1077220949, 1780993375, 1085533723, 2142365863}
,{1688246430, 167623444, 503700173, 59603534, 1834766095, 650479017, 2101393617, 1149476868, 143959003}
,{788776745, 1149923265, 1744530297, 347134845, 635003041, 1410722316, 821280757, 1885047680, 2122720000}
,{747179417, 1751689709, 93826211, 763454072, 1533240210, 1168054366, 329252277, 326093228, 130617392}
,{902778833, 639315084, 1084168759, 1283804631, 1204490120, 1719206863, 804568904, 1431702244, 1132891064}
,{867801247, 1741150822, 534317634, 1055758761, 214850586, 41556015, 2130385764, 739620473, 1351847791}
,{980782314, 35251984, 1491852334, 1785449067, 1941708822, 1195216727, 1825145756, 717151323, 2012828406}
,{766946307, 1374309895, 1436863565, 1927147077, 1884120600, 1857497964, 1216236007, 105600104, 129173646}
,{1885402416, 739019030, 545283684, 128607592, 94277565, 1797115102, 131041763, 404865522, 167322997}
,{1671865956, 1055655027, 1871588815, 655396639, 2122626080, 41005213, 1105617714, 1771668992, 643548474}
,{506230229, 1109438022, 1085383502, 101609263, 1497751185, 1898625083, 742987270, 915305542, 1392142610}
,{1772250407, 1464366450, 1819931800, 81714976, 1912639850, 1595698084, 1690934200, 1637371653, 587940495}
,{1813957398, 1493627555, 504109943, 118260845, 1578796450, 643994101, 386086190, 1056872502, 1423639439}
,{1485158271, 469861517, 757644866, 1072228752, 121103165, 1131172128, 1220728080, 644973924, 324859021}
,{156486153, 911534875, 169363832, 883054057, 2105728739, 105145445, 669433838, 434003849, 772711386}
,{140441843, 125788989, 953646098, 525110200, 1392662388, 398756891, 1995197192, 1115281371, 35946267}
,{1001501032, 777806188, 2113289614, 1587409798, 1830372311, 1715771015, 975898847, 1997464478, 916511379}
,{1420638284, 47671442, 116720674, 1581673734, 458778533, 2127428419, 1412230192, 228399708, 1237832094}
,{830392375, 400157242, 1308325582, 1074108730, 737803280, 1720132646, 1350524250, 1675167066, 795739901}
,{1636308395, 2115845661, 2103610042, 1854259082, 508865792, 1006266811, 785049017, 1257369379, 2108895974}
,{972869632, 1962435832, 1557825517, 1468103014, 700180905, 762938002, 1505545581, 1048536243, 655315767}
,{17838325, 32750089, 1613963745, 685357051, 824971126, 563377587, 20917219, 570661842, 839987846}
,{2080224261, 1970840956, 11641567, 1603176824, 578710454, 1350136647, 290713017, 1179304721, 273327984}
,{1850522389, 1185539240, 89347544, 1800247218, 288407290, 1504224045, 99740040, 1765338204, 100035648}
,{2027454713, 112299962, 448640290, 359412624, 945774278, 798555183, 491268523, 509591188, 1626410821}
,{370226007, 534903305, 760095605, 1904604692, 1994646390, 1156943479, 247791205, 207265035, 1385232716}
,{10753570, 1180594183, 129261327, 13395651, 1135533406, 103088610, 1166922137, 1430334003, 957039052}
,{283577490, 1019177979, 616320933, 1534437219, 2008372207, 1993667681, 25911427, 129503849, 781929854}
,{1337354204, 876869541, 504990890, 1944353286, 441675107, 838292299, 104214403, 82517145, 949665018}
,{1525301549, 2034983521, 1232179781, 1611560139, 1766337087, 320011973, 368556314, 838320335, 936054058}
,{281066592, 1046052726, 888057209, 1004726110, 281660621, 88685960, 857017123, 1026892664, 351744918}
,{517787712, 2115090911, 1866809182, 1619863442, 255124759, 1074871753, 738984792, 502225306, 723590996}
,{358375818, 499079648, 1614401240, 901262808, 1969979871, 158899747, 162921958, 629491758, 1534909391}
,{1523414038, 146419940, 790959535, 1313912192, 1016832670, 1545960309, 1216241068, 1747221277, 83182676}
,{1506352153, 377898691, 344920568, 1735644633, 384225065, 1246539808, 767625094, 822730756, 1814256218}
,{441190918, 1528363576, 56950817, 1757696656, 33256910, 1159198390, 724973204, 1933452764, 1933528819}
,{718527959, 798951260, 1341042777, 1590884155, 553501097, 440092292, 328017016, 1046581470, 1260939406}
,{1019207266, 1299711548, 1104948162, 1365506296, 243281662, 1587234099, 442042045, 1747662492, 920089978}
,{723760222, 75366178, 1655075156, 1896460446, 40630693, 36798465, 31582958, 1670658046, 100145471}
,{611090211, 955827667, 971137646, 1947383044, 1286234092, 1923062099, 190691729, 699524933, 289781089}
,{1442015672, 1580578427, 62395413, 2091976789, 906647955, 216879483, 308610141, 2091337422, 389428222}
,{1879968236, 1709567395, 840108216, 456780363, 1905188049, 622081783, 944502139, 18830432, 1039603939}
,{1531181384, 1427852057, 360213478, 2020777831, 1977336860, 1321435302, 112459487, 585552640, 1772393436}
,{1672567953, 1130046518, 1866045104, 984902556, 1295977366, 1940512105, 1723736779, 274220501, 1307374373}
,{995818253, 1616337384, 134186405, 970571907, 724381916, 516181719, 1993632268, 1131794072, 1616744179}
,{1986405829, 801342666, 1791849757, 320239713, 129995312, 1072468267, 20621493, 1708886561, 1786742845}
,{1904272510, 1271468622, 1172846292, 413744305, 909021527, 1678715067, 1336915295, 1556436687, 1115887963}
,{1155700101, 1777429807, 1733744535, 2094491132, 958714493, 1480308954, 571391307, 498639804, 1213687291}
,{653774252, 622971674, 16832206, 1878498621, 199520724, 1682640874, 931146453, 800978086, 563423488}
,{834875140, 526993975, 2069754407, 1634085691, 1452773610, 234787676, 1558081991, 122389353, 658318060}
,{808236483, 1955505896, 539850467, 1782825094, 766894245, 1994745674, 1344928034, 667855188, 1392138388}
,{251796301, 832660781, 1547291704, 1835766639, 1129652128, 851906258, 1034376149, 292618981, 1134609667}
,{650189050, 534103921, 1772194399, 1766241714, 1496072957, 1380279577, 966974000, 1427800814, 352097578}
,{1539268390, 1802099038, 1732828711, 2117843991, 1386879090, 189613278, 1451715699, 1502369314, 216804736}
,{128830494, 810912144, 134539106, 261796872, 1130859305, 121576377, 1917348474, 1093963459, 640597912}
,{1800300769, 1418583782, 1014988938, 2104190004, 208339634, 1754846154, 1904031337, 1115656489, 784227504}
,{1601429117, 972023510, 1080594817, 1485110258, 849173338, 89400772, 190311088, 130011189, 1770037063}
,{801989306, 2122670670, 1511343969, 1446823603, 1070945406, 338735860, 1392786418, 744599992, 2098825597}
,{1474121652, 1284090416, 1615559898, 879699354, 402401755, 2056638026, 1539981683, 662828312, 1458902034}
,{920447909, 907256367, 814053439, 471578464, 1979591770, 1551467240, 271039905, 913585409, 478720871}
,{1424961031, 751705093, 1253778648, 1194474639, 870202748, 1434084738, 537100625, 1543360332, 1618516820}
,{1674292368, 1772461085, 306432515, 1165387284, 1920771405, 2045452215, 1201136975, 208118431, 881562313}
,{2021457172, 234151611, 1755542051, 1781486147, 1189764241, 815131789, 289129919, 1572916493, 2035107839}
,{1861107207, 1045793432, 2094992904, 280408762, 1185593830, 1146025186, 190645536, 1931669742, 724257785}
,{667548678, 1215815684, 671044902, 977529525, 1108325336, 767425282, 617194418, 546763114, 272713152}
,{787728674, 712665123, 959902336, 1345679085, 458863280, 1940198640, 280832812, 388394142, 537673695}
,{106121270, 1421349656, 800376057, 192623482, 891789488, 310465388, 1550706630, 723617985, 1175394105}
,{1276552452, 1752754258, 396329688, 1742914497, 911900851, 446967441, 347265266, 118133101, 977928448}
,{674671788, 923257324, 1573499495, 271223647, 633016363, 1546115657, 710236468, 881634342, 539192708}
,{1862905487, 404812794, 2127599282, 269272703, 320036172, 504415150, 877312156, 1680200366, 376211138}
,{2066753331, 136408375, 247833380, 144016533, 1097781771, 594160715, 1650489865, 103215992, 202662715}
,{561664989, 1320123821, 20547721, 1085990622, 265434474, 2089908716, 1007066588, 1592510775, 1318668438}
,{2014571333, 371033380, 1948551601, 501093964, 246771683, 1061797784, 2006419106, 443092371, 1272805359}
,{1379525157, 1131689368, 471342666, 1061919500, 996436135, 326733016, 2074973028, 1194672399, 470926613}
,{126825737, 502347018, 61979871, 2110816565, 1976514458, 135835463, 336560006, 489782010, 731348931}
,{945623733, 2073217663, 2099287976, 1939632323, 1613364417, 990630080, 1752567448, 37743528, 311663994}
,{14818725, 538679347, 635867174, 37583454, 1003661699, 1070363705, 798110789, 724612319, 642673205}
,{1145044236, 977172434, 1812507188, 871669422, 349742333, 146910222, 1975836975, 1790739216, 1440970525}
,{2107161609, 2146525662, 1811386826, 338062963, 997668309, 227903093, 980001467, 746681726, 1380574799}
,{486202949, 1684798545, 1161244629, 218281322, 1461153196, 333511507, 1796134878, 1522369397, 606201360}
,{1770699527, 678996876, 664230875, 346930560, 1662234515, 810871318, 165677549, 1255311059, 1115501462}
,{1883003536, 1252399137, 1559563186, 1466043470, 766217871, 163851170, 428473453, 564458485, 1105528313}
,{1682255259, 958348699, 1772317063, 263258874, 202317483, 262185176, 1251055522, 40137806, 832514345}
,{166234425, 1519004393, 1525141877, 1836294417, 1470918674, 1997693256, 396452508, 409045851, 361384985}
,{1736502975, 1603387171, 873545358, 1005386490, 1584270069, 458740218, 498237917, 508880672, 547168375}
,{408694021, 1464134103, 1630453440, 210964587, 981865791, 1428465378, 915742193, 1286319190, 1605395898}
,{1423250515, 2035496656, 678150920, 1540911197, 1189370216, 1000488064, 757476827, 2027413584, 909272191}
,{618049052, 821853822, 2099383110, 210810420, 1785114773, 1894026842, 1651222080, 1780366339, 1054612372}
,{276659015, 1171381207, 758177633, 145721993, 574568418, 537679640, 569989759, 1986335042, 94099518}
,{1480435469, 1694297772, 454654773, 1360025867, 908035892, 2142512999, 1744884688, 1608309685, 881998414}
,{1968615079, 1656755375, 1993366622, 998344367, 28937154, 860365915, 1155367640, 593965693, 1216913532}
,{1140906854, 237792652, 906433879, 1591468549, 949033143, 1341713806, 1081079141, 1320465032, 1775360570}
,{613364720, 1724206024, 883134537, 252612976, 2132670796, 1509528372, 1643289795, 1288074283, 2003967344}
,{440308260, 950464619, 1286381340, 11837465, 1635313787, 796356728, 1866649944, 1935490361, 1065266343}
,{48434322, 1947443176, 286025632, 898329135, 2055921025, 2145239528, 394185256, 330867566, 436878824}
,{2094553555, 1576281646, 555214550, 880884357, 138298721, 306316990, 1521745427, 1099892734, 1256574817}
,{488778025, 1397109876, 680230558, 898172365, 39413646, 1762908479, 2103322055, 1772622381, 1673853642}
,{1293451821, 59094087, 2005768020, 168479994, 751187423, 1861243779, 1246958515, 1342657249, 1247625027}
,{197676655, 1054970504, 1253670760, 1861827422, 1176633591, 448912763, 1418673840, 1365291578, 484033174}
,{1170201996, 162623306, 1849560204, 151071672, 442546301, 1471039525, 1871567282, 1858255135, 1402769739}
,{306394847, 1954003877, 199684492, 487159794, 2018661236, 450901310, 1115610857, 1922057094, 701696159}
,{417828677, 778092456, 1418686854, 1140367019, 602882650, 309729872, 451287940, 982416240, 259036966}
,{197931806, 1888412940, 1503994486, 518266328, 1571294798, 315898255, 856559185, 755341525, 1843796176}
,{1963027320, 1388958721, 303016177, 1022801991, 1322230057, 701899159, 2097460936, 2006677581, 727267086}
,{2087663868, 368351760, 1789847648, 481900297, 305547629, 496662865, 5402910, 1553751102, 2123966755}
,{1072138688, 1676849683, 218557230, 1256822838, 880034729, 13110954, 1717206949, 1622168047, 1063895189}
,{582332959, 954073947, 730393917, 1329494272, 1362173142, 642883229, 1471752428, 1673374381, 883866291}
,{812516952, 1169885791, 1236462108, 1907745011, 2141412313, 26511399, 1119808936, 1086315154, 1116956915}
,{203051139, 2075086320, 248931142, 1036905681, 1470459045, 1896644239, 1625215123, 790202935, 657017106}
,{391526948, 1166894491, 888493202, 909554027, 192286649, 1888853518, 12646106, 1549372984, 79502834}
,{338210054, 1042552550, 1724435100, 326327395, 1505840460, 279933937, 1304335675, 1480422644, 1417189264}
,{1625241944, 142114324, 374886669, 663196355, 1328351252, 2776373, 1769779309, 1010141238, 760053454}
,{1303473866, 1270914567, 825253458, 1325476945, 31003778, 1458943808, 1173616584, 1344207870, 634272847}
,{1704532647, 781317474, 678900205, 1922726371, 862814454, 936350324, 780674321, 1756864541, 355286047}
,{691828202, 1345625827, 854220906, 1600789169, 1666051590, 1318234321, 500213076, 996217863, 1639497885}
,{1343132366, 808188170, 877696809, 668030531, 213052010, 982336936, 104382152, 1919986966, 185301666}
,{1840737732, 1990316468, 1668774220, 78646902, 931008248, 487064803, 1820424086, 85743566, 1545243094}
,{126865257, 729339235, 1177600601, 1350999620, 255056574, 362813848, 1909609213, 2078500766, 171826400}
,{2086263230, 1437545402, 1173875958, 1316635030, 1615749964, 1781095095, 13621763, 1633989960, 1043853478}
,{460635857, 734408275, 1764807375, 1898333329, 1292536636, 251063178, 1462186898, 1171819550, 1956141617}
,{1431162880, 490029232, 1262503105, 1329807571, 751264109, 2136605871, 1695807056, 306778372, 1932381867}
,{1988937863, 2027480699, 1535004989, 214807941, 2139126656, 1653474994, 312043647, 857211941, 1946476659}
,{818958097, 1388277664, 1397334511, 1660691557, 2048007464, 779519783, 1446625134, 510127666, 366224228}
,{280969743, 1174250323, 2079217420, 951676236, 261379682, 1398033460, 1589905709, 1361576879, 1824937239}
,{1951571268, 271837383, 1355560478, 684631178, 1435288185, 447780898, 984088799, 232825916, 924302385}
,{2046214895, 44243714, 300323637, 765919960, 206501312, 938681236, 1689772122, 389248082, 285012848}
,{1830236273, 1288660329, 1970837636, 1933369000, 460566881, 2043942004, 1684954664, 1773131597, 1665331922}
,{201676699, 1998072415, 533742967, 2134059984, 1557115276, 1820111388, 755697440, 1869565880, 902767218}
,{1723567772, 32608884, 1354576639, 1369109799, 1284454825, 717850352, 244335557, 961469807, 1699559669}
,{1018127524, 506279999, 199983247, 153825334, 104450790, 579432095, 1607794749, 557464813, 1305202476}
,{974706551, 1778115591, 121543928, 420407332, 175662788, 470600657, 1782194798, 1729374435, 1180994471}
,{2126178131, 1227265672, 1799394210, 1089246034, 482572629, 1664219332, 357923985, 1500843551, 1138393392}
} /* End of byte 23 */
};
#endif

};
