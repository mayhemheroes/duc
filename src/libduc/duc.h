
#ifndef duc_h
#define duc_h

#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <dirent.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/time.h>
#include <locale.h>

#define DUC_PATH_MAX 16384

/* Don't expect to find files size 2^48 or larger, so we need a function that uses smaller buckets when we have a larger number of buckets.  Still being worked on. */
#define DUC_HISTOGRAM_BUCKETS_MAX 512
#define DUC_HISTOGRAM_BUCKETS_DEF 48

/* Number of Largest files to track */
#define DUC_TOPN_CNT 10
/* Maximum number of topN files we can track, totally arbitrary... */
#define DUC_TOPN_CNT_MAX 1000

/* minimum file size to track in topN list: 10 kilobytes */
#define DUC_TOPN_MIN_FILE_SIZE 10240

#ifdef WIN32
typedef int64_t duc_dev_t;
typedef int64_t duc_ino_t;
#else
typedef dev_t duc_dev_t;
typedef ino_t duc_ino_t;
#endif

typedef struct duc duc;
typedef struct duc_dir duc_dir;
typedef struct duc_index_req duc_index_req;

typedef enum {
	DUC_OPEN_RO = 1<<0,        /* Open read-only (for querying)*/
	DUC_OPEN_RW = 1<<1,        /* Open read-write (for indexing) */
	DUC_OPEN_COMPRESS = 1<<2,  /* Create compressed database */
	DUC_OPEN_FORCE = 1<<3,     /* Force over-write of database for indexing */
	DUC_FS_BIG = 1<<4,       /* Tune for large filesystems to index */
	DUC_FS_BIGGER = 1<<5,       /* Tune for large filesystems to index */
	DUC_FS_BIGGEST = 1<<6,       /* Tune for large filesystems to index */
} duc_open_flags;


typedef enum {
	DUC_INDEX_XDEV             = 1<<0, /* Do not cross device boundaries while indexing */
	DUC_INDEX_HIDE_FILE_NAMES  = 1<<1, /* Hide file names */
	DUC_INDEX_CHECK_HARD_LINKS = 1<<2, /* Count hard links only once during indexing */
	DUC_INDEX_DRY_RUN          = 1<<3, /* Do not touch the database */
	DUC_INDEX_TOPN_FILES       = 1<<4, /* Keep side DB of top N largest files */
} duc_index_flags;

typedef enum {
	DUC_SIZE_TYPE_APPARENT,
	DUC_SIZE_TYPE_ACTUAL,
	DUC_SIZE_TYPE_COUNT,
} duc_size_type;

typedef enum {
	DUC_SORT_SIZE = 1,
	DUC_SORT_NAME = 2,
} duc_sort;

typedef enum {
	DUC_OK,                     /* No error, success */
	DUC_E_DB_NOT_FOUND,         /* Database not found */
	DUC_E_DB_CORRUPT,           /* Database corrupt and unusable */
	DUC_E_DB_VERSION_MISMATCH,  /* Database version mismatch */
	DUC_E_DB_TYPE_MISMATCH,     /* Database compiled in type mismatch */
	DUC_E_PATH_NOT_FOUND,       /* Requested path not found */
	DUC_E_PERMISSION_DENIED,    /* Permission denied */
	DUC_E_OUT_OF_MEMORY,        /* Out of memory */
	DUC_E_DB_BACKEND,           /* Unable to initialize database backend */
	DUC_E_NOT_IMPLEMENTED,      /* Some feature request is not supported */
	DUC_E_UNKNOWN,              /* Unknown error, contact the author */
} duc_errno;


typedef enum {
	DUC_LOG_FTL,
	DUC_LOG_WRN,
	DUC_LOG_INF,
	DUC_LOG_DBG,
	DUC_LOG_DMP
} duc_log_level;

typedef enum {
	DUC_FILE_TYPE_BLK,
	DUC_FILE_TYPE_CHR,
	DUC_FILE_TYPE_DIR,
	DUC_FILE_TYPE_FIFO,
	DUC_FILE_TYPE_LNK,
	DUC_FILE_TYPE_REG,
	DUC_FILE_TYPE_SOCK,
	DUC_FILE_TYPE_UNKNOWN,
} duc_file_type;

struct duc_devino {
	duc_dev_t dev;
	duc_ino_t ino;
};

struct duc_size {
	off_t actual;
	off_t apparent;
	off_t count;
};

/* Track largest files found */

typedef struct duc_topn_file {
    // Should be dynamically allocated...
    char name[DUC_PATH_MAX];
    size_t size;
} duc_topn_file;

struct duc_index_report {
	char path[DUC_PATH_MAX];        /* Indexed path */
	struct duc_devino devino;   /* Index top device id and inode number */
	struct timeval time_start;  /* Index start time */
	struct timeval time_stop;   /* Index finished time */
	size_t file_count;          /* Total number of files indexed */
	size_t dir_count;           /* Total number of directories indexed */
	struct duc_size size;       /* Total size */
        size_t topn_min_size;       /* minimum size in bytes to get added to topN list of files */
        int topn_cnt;               /* Max topN number of files to report on*/
        int topn_cnt_max;           /* Maximum number of topN files to track */
        int histogram_buckets;      /* Number of buckets in histogram */
        size_t histogram[DUC_HISTOGRAM_BUCKETS_MAX];      /* histogram of file sizes, log(size)/log(2) */
        duc_topn_file* topn_array[DUC_TOPN_CNT_MAX];    /* pointer to array of structs, stores each topN filename and size */
};

struct duc_dirent {
	char *name;                 /* File name */
	duc_file_type type;         /* File type */
	struct duc_size size;       /* File size */
	struct duc_devino devino;   /* Device id and inode number */
};


/*
 * Duc context, logging and error reporting
 */

typedef void (*duc_log_callback)(duc_log_level level, const char *fmt, va_list va);

duc *duc_new(void);
void duc_del(duc *duc);

void duc_set_log_level(duc *duc, duc_log_level level);
void duc_set_log_callback(duc *duc, duc_log_callback cb);

duc_errno duc_error(duc *duc);
const char *duc_strerror(duc *duc);


/*
 * Open and close database
 */

int duc_open(duc *duc, const char *path_db, duc_open_flags flags);
int duc_close(duc *duc);


/*
 * Index file systems
 */

typedef void (*duc_index_progress_cb)(struct duc_index_report *report, void *ptr);

duc_index_req *duc_index_req_new(duc *duc);
int duc_index_req_set_username(duc_index_req *req, const char *username);
int duc_index_req_set_uid(duc_index_req *req, int uid);
int duc_index_req_add_exclude(duc_index_req *req, const char *pattern);
int duc_index_req_add_fstype_include(duc_index_req *req, const char *types);
int duc_index_req_add_fstype_exclude(duc_index_req *req, const char *types);
int duc_index_req_set_maxdepth(duc_index_req *req, int maxdepth);
int duc_index_req_set_progress_cb(duc_index_req *req, duc_index_progress_cb fn, void *ptr);
int duc_index_req_set_topn(duc_index_req *req, int topn);
int duc_index_req_set_buckets(duc_index_req *req, int topn);
struct duc_index_report *duc_index(duc_index_req *req, const char *path, duc_index_flags flags);
int duc_index_req_free(duc_index_req *req);
int duc_index_report_free(struct duc_index_report *rep);


/*
 * Querying the duc database
 */

struct duc_index_report *duc_get_report(duc *duc, size_t id);

duc_dir *duc_dir_open(duc *duc, const char *path);
duc_dir *duc_dir_openat(duc_dir *dir, const char *name);
duc_dir *duc_dir_openent(duc_dir *dir, const struct duc_dirent *e);
struct duc_dirent *duc_dir_read(duc_dir *dir, duc_size_type st, duc_sort sort);
char *duc_dir_get_path(duc_dir *dir);
void duc_dir_get_size(duc_dir *dir, struct duc_size *size);
size_t duc_dir_get_count(duc_dir *dir);
struct duc_dirent *duc_dir_find_child(duc_dir *dir, const char *name);
int duc_dir_seek(duc_dir *dir, size_t offset);
int duc_dir_rewind(duc_dir *dir);
int duc_dir_close(duc_dir *dir);

/* 
 * Helper functions
 */

off_t duc_get_size(struct duc_size *size, duc_size_type st);
int humanize(double v, int exact, double scale, char *buf, size_t maxlen);
int duc_human_number(double v, int exact, char *buf, size_t maxlen);
int duc_human_size(const struct duc_size *size, duc_size_type st, int exact, char *buf, size_t maxlen);
int duc_human_duration(struct timeval start, struct timeval end, char *buf, size_t maxlen);
void duc_log(struct duc *duc, duc_log_level lvl, const char *fmt, ...);
char duc_file_type_char(duc_file_type t);
char *duc_file_type_name(duc_file_type t);

char *duc_db_type_check(const char *path_db);
#endif  /* ifndef duc_h */
