/*************************************************************************/
/*                                                                       */
/*  Copyright (c) 1994 Stanford University                               */
/*                                                                       */
/*  All rights reserved.                                                 */
/*                                                                       */
/*  Permission is given to use, copy, and modify this software for any   */
/*  non-commercial purpose as long as this copyright notice is not       */
/*  removed.  All other uses, including redistribution in whole or in    */
/*  part, are forbidden without prior written permission.                */
/*                                                                       */
/*  This software is provided with absolutely no warranty and no         */
/*  support.                                                             */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>

#define FitsInCache 2048

#define MAX_PROC 128
#define PAGE_SIZE 4096

typedef struct {
	long n, m, *col, *startrow, *row;
	double *nz;
	} SMatrix;

struct Pair {
	long block_num;
	struct Pair *next;
	};

typedef struct {
        long i, j, owner, remaining, nmod;
	long length; /* number of full rows in block */
	long parent; /* block number of parent block */
	double checksum;
	volatile unsigned long done;
	long *structure, *relative;
	double *nz;
	struct Pair *pair;
      } Block;

typedef union {
        Block *block;
	double nz;
      } Entry;
       
typedef struct {
        long n, *col, *row, n_blocks, n_entries, entries_allocated;
	long *partition_size, *renumbering, *mapI, *mapJ;
	long *domain, *domains, n_domains, *proc_domains;
	long n_partitions, max_partition;
	double **proc_domain_storage;
	Entry *entry;
      } BMatrix;

#define BLOCK(bl_no) (LB.entry[bl_no].block)
#define BLOCKROW(bl_no) (BLOCK(bl_no)->i)
#define BLOCKCOL(bl_no) (BLOCK(bl_no)->j)
#define OWNER(bl_no) (BLOCK(bl_no)->owner)

struct Update {
	long i, j, src, remaining;
	long dimi, dimj, *structi, *structj;
	double *update;
	struct Update *next;
      };

struct Task {
  long block_num, desti, destj, src;
  struct Update *update;
  struct Task *next;
  };

struct GlobalMemory {
	BARDEC(start)
	LOCKDEC(waitLock)
	LOCKDEC(memLock)
	unsigned long runtime[MAX_PROC];
	};

struct BlockList {
	long theBlock, row, col, length;
	long *structure;
	double *nz;
	struct BlockList *next;
	};

struct LocalCopies {
       double *blktmp;
       long max_panel;
       long *link;
       long *first;
       double *storage;
       double *updatetmp;
       long *relative;
       struct Update *freeUpdate;
       struct Task *freeTask;
       unsigned long rs;
       unsigned long rf;
       unsigned long us;
       unsigned long uf;
       unsigned long ss;
       unsigned long sf;
       unsigned long runtime;
       unsigned long runs;
       };

#define DISTRIBUTED 888

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#define LB_DOMAINS 2

#define EMBED 1

#define NO_PERM 1

#define FAN_OUT 2

/*
 * amal.C
 */
long OpsFromSuper(long size, long nz);
long CountSupers(long cols, long *node);
void Amalgamate2(long join, SMatrix M, long *T, long *nz, long *node, long *domain, long target_size);
void ConsiderMerge(long join, long super, SMatrix M, long *nz, long *node, long *domain, long target_size, long traversal_order);
void JoinTwoSupers2(long *nz, long *node, long child, long parent);
void ReorderMatrix(SMatrix M, long super, long *node, long *counter, long *PERM);
void FixNodeNZAndT(SMatrix M, long *PERM, long *node, long *nz, long *T);
void InvertPerm(long n, long *PERM, long *INVP);
double PathLength(long cols, long rows, long target_panel_size);

/*
 * assign.C
 */
void PDIV(long src_col, long src_nz, double *ops, double *misses, double *runtime);
void PMOD(long src_col, long dest_col, long dest_nz, double *ops, double *misses, double *runtime);
void PADD(long cols, long rows, double *misses, double *runtime);
void AssignBlocksNow(void);
void EmbedBlocks(void);

/*
 * bfac.C
 */
void BFac(long diag, struct LocalCopies *lc);
void OneFac(double *A, long n1, long n2);
void BDiv(long n1, long n3, double *diag_nz, double *below_nz, struct LocalCopies *lc);
void OneDiv(double *A, double *B, long n1, long n3, long n4);
void BMod(long n1, long n2, long n3, double *top_nz, double *bend_nz, double *dest_nz, struct LocalCopies *lc);
void CopyBlock(double *B, double *dest, long n3, long is, long ks, long il, long kl);
void CopyBlockBack(double *B, double *src, long n3, long is, long ks, long il, long kl);
void OneMatmat(double *B, double *A, double *C, long n1, long n2, long n3, long n4, long n5);
void BLMod(long n1, long n2, double *left_nz, double *dest_nz, struct LocalCopies *lc);
void OneLower(double *A, double *C, long n1, long n2, long n3);
void FindBlockUpdate(long domain, long blj, long bli, double **update, long *stride);

/*
 * bksolve.C
 */
double *TriBSolve(BMatrix LB, double *b, long *PERM);
double ComputeNorm(double *x, long n);
double *CreateVector(SMatrix M);

/*
 * block2.C
 */
void CreateBlockedMatrix2(SMatrix M, long block_ub, long *T, long *firstchild, long *child, long *PERM, long *INVP, long *domain, long *partition);
long FindNumPartitions(long set_size, long piece_size);
void ComputeBlockParents(long *T);
void FillInStructure(SMatrix M, long *firstchild, long *child, long *PERM, long *INVP);
void FillInNZ(SMatrix M, long *PERM, long *INVP);
void FindDomStructure(long super, long *nz, long n_nz);
void FindDummyDomainStructure(long which_domain);
void CheckColLength(long col, long n_nz);
void FindBlStructure(SMatrix M, long super, long *PERM, long *INVP, long *firstchild, long *child, long *structure, long *nz);
void FindSuperStructure(SMatrix M, long super, long *PERM, long *INVP, long *firstchild, long *child, long *structure, long *nz, long *n_nz);
void FindDetailedStructure(long col, long *structure, long *nz, long n_nz);
void AllocateNZ(void);
void FillIn(SMatrix M, long col, long *PERM, long *INVP, double *scatter);
void InsSort(long *nz, long n);
long BlDepth(long col);
void SortByKey(long n, long *blocks, long *keys);
void DumpSizes(BMatrix LB, long *domain, long *sizes);
void ComputePartitionNumbering(long *numbering);
void FindMachineDimensions(long P);
long EmbeddedOwner(long block);

/*
 * fo.C
 */
void PreProcessFO(long MyNum);
void PreAllocate1FO(void);
void PreAllocateFO(long MyNum, struct LocalCopies *lc);
void BNumericSolveFO(long MyNum, struct LocalCopies *lc);
void DriveParallelFO(long MyNum, struct LocalCopies *lc);
long HandleTaskFO(long MyNum, struct LocalCopies *lc);
void DiagReceived(long diag, long MyNum, struct LocalCopies *lc);
void BlockReceived(long block, long MyNum, struct LocalCopies *lc);
struct BlockList *CopyOneBlock(long block, long MyNum);
void FreeColumnListFO(long p, long col);
void DecrementRemaining(long dest_block, long MyNum, struct LocalCopies *lc);
void PerformUpdate(struct BlockList *above_bl, struct BlockList *below_bl, long MyNum, struct LocalCopies *lc);
void DistributeUpdateFO(long which_domain, long MyNum, struct LocalCopies *lc);
void HandleUpdate2FO(long which_domain, long bli, long blj, long MyNum, struct LocalCopies *lc);
void FindRelativeIndices(long *src_structure, long src_len, long *dest_structure, long *relative);
void BlockReadyFO(long block, long MyNum, struct LocalCopies *lc);
void BlockDoneFO(long block, long MyNum, struct LocalCopies *lc);
void CheckRemaining(void);
void CheckReceived(void);
void ComputeRemainingFO(void);
void InitRemainingFO(long MyNum);
void ComputeReceivedFO(void);
void InitReceivedFO(long MyNum);
void ScatterUpdateFO(long dimi, long *structi, long dimj, long *structj, long destdim, double *oldupdate, double *newupdate);
void ScatterUpdateFO2(long dimi, long *structi, long dimj, long *structj, long stride, long destdim, double *oldupdate, double *newupdate);

/*
 * malloc.C
 */
void MallocInit(long P);
void InitOneFreeList(long p);
void MallocStats(void);
long FindBucket(long size);
char *MyMalloc(long size, long home);
void MigrateMem(long *start, long length, long home);
void MyFree(long *block);
void MyFreeNow(long *block);

/*
 * mf.C
 */
void InitTaskQueues(long P);
long FindBlock(long i, long j);
void Send(long src_block, long dest_block, long desti, long destj, struct Update *update, long p, long MyNum, struct LocalCopies *lc);
long TaskWaiting(long MyNum);
void GetBlock(long *desti, long *destj, long *src, struct Update **update, long MyNum, struct LocalCopies *lc);

/*
 * numLL.C
 */
void FactorLLDomain(long which_domain, long MyNum, struct LocalCopies *lc);
void CompleteSupernodeB(long super);
void CompleteColumnB(long j);
void FindRelativeIndicesLeft(long *src_structure, long rows_in_update, long offset, long *indices, long *relative);
void ScatterSuperUpdate(double *update, long cols_in_update, long rows_in_update, double *dest_nz, long dest_len, long *relative);
void ModifySuperByColumn(double *src_nz, long cols_in_update, long rows_in_update, double *dest_nz, long dest_len, long *relative);
void SetDestIndices(long super, long *indices);
void SetDomainIndices(long super, long *indices);
void ModifySuperBySuper(long src, long theFirst, long theLast, long length, double *dest);
void ModifyTwoBySupernodeB(long super, long lastcol, long theFirst, double *destination0, double *destination1);
void ModifyBySupernodeB(long super, long lastcol, long theFirst, double *destination);

/*
 * parts.C
 */
void Partition(SMatrix M, long parts, long *T, long *assigned_ops, long *domain, long *domains, long *proc_domains);
void MarkSubtreeAsDomain(long *domain, long root);
void NumberPartition(long parts, long *assigned_ops, long distribute);
long MaxBucket(long *assigned_ops, long parts);
long MinBucket(long *assigned_ops, long parts);
struct Chunk *NewChunk(void);
struct Chunk *GetChunk(void);

/*
 * seg.C
 */
void ComputeTargetBlockSize(SMatrix M, long P);
void FindMaxHeight(SMatrix L, long root, long height, long *maxm);
void NoSegments(SMatrix M);
void CreatePermutation(long n, long *PERM, long permutation_method);

/*
 * solve.C
 */
void Go(void);
void PlaceDomains(long P);
void ComposePerm(long *PERM1, long *PERM2, long n);

/*
 * tree.C
 */
void EliminationTreeFromA(SMatrix A, long *T, long *P, long *INVP);
void ParentToChild(long *T, long n, long *firstchild, long *child);
void ComputeNZ(SMatrix A, long *T, long *nz, long *PERM, long *INVP);
void FindSupernodes(SMatrix A, long *T, long *nz, long *node);
void ComputeWorkTree(SMatrix A, long *nz, double *work_tree);

/*
 * util.C
 */
SMatrix NewMatrix(long n, long m, long nz);
void FreeMatrix(SMatrix M);
double *NewVector(long n);
double Value(long i, long j);
SMatrix ReadSparse(char *name, char *probName);
void DumpLine(FILE *fp);
void ParseIntFormat(char *buf, long *num, long *size);
void ReadVector(FILE *fp, long n, long *where, long perline, long persize);
SMatrix LowerToFull(SMatrix L);
void ISort(SMatrix M, long k);

