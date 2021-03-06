#ifndef __INCLUDE_GUARD_SWUPD_H
#define __INCLUDE_GUARD_SWUPD_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <curl/curl.h>
#include "list.h"
#include <limits.h>
#include <dirent.h>
#include "swupd-error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WARNING: keep SWUPD_VERSION_INCR in sync with server definition  */
#define SWUPD_VERSION_INCR 10
#define SWUPD_VERSION_IS_DEVEL(v) (((v) % SWUPD_VERSION_INCR) == 8)
#define SWUPD_VERSION_IS_RESVD(v) (((v) % SWUPD_VERSION_INCR) == 9)

#ifndef LINE_MAX
#define LINE_MAX _POSIX2_LINE_MAX
#endif

#define PATH_MAXLEN 4096
#define CURRENT_OS_VERSION -1

#define UNUSED_PARAM __attribute__((__unused__))

#define MAX_TRIES 3

struct sub {
	/* name of bundle/component/subscription */
	char *component;

	/* version meanings:
	 *   non-zero -> version has been read from MoM
	 *   -1       -> error, local bundle do not match any bundle on MoM */
	int version;

	/* oldversion: set to 0 by calloc(), possibly overridden by MoM read */
	int oldversion;
};

struct manifest {
	int version;
	int manifest_version;
	uint64_t contentsize;
	struct list *files;
	struct list *manifests;    /* struct file for possible manifests */
	struct list *submanifests; /* struct manifest for subscribed manifests */
	char *component;
};

struct header;

extern bool force;
extern int verbose;
extern int update_count;
extern int update_skip;
extern bool update_complete;
extern bool need_update_boot;
extern bool need_update_bootloader;

struct update_stat {
	uint64_t st_mode;
	uint64_t st_uid;
	uint64_t st_gid;
	uint64_t st_rdev;
	uint64_t st_size;
};

#define DIGEST_LEN_SHA256 64
/* +1 for null termination */
#define SWUPD_HASH_LEN (DIGEST_LEN_SHA256 + 1)

struct file {
	char *filename;
	char hash[SWUPD_HASH_LEN];
	bool use_xattrs;
	int last_change;
	struct update_stat stat;

	unsigned int is_dir : 1;
	unsigned int is_file : 1;
	unsigned int is_link : 1;
	unsigned int is_deleted : 1;
	unsigned int is_manifest : 1;

	unsigned int is_config : 1;
	unsigned int is_state : 1;
	unsigned int is_boot : 1;
	unsigned int is_rename : 1;
	unsigned int is_orphan : 1;
	unsigned int do_not_update : 1;

	struct file *peer;      /* same file in another manifest */
	struct file *deltapeer; /* the file to do the binary delta against; often same as "peer" except in rename cases */
	struct header *header;

	char *staging; /* output name used during download & staging */
	CURL *curl;    /* curl handle if downloading */
};

extern bool download_only;
extern bool verify_esp_only;
extern bool have_manifest_diskspace;
extern bool have_network;
extern bool verify_bundles_only;
extern bool ignore_config;
extern bool ignore_state;
extern bool ignore_orphans;
extern bool fix;
extern char *format_string;
extern char *path_prefix;
extern bool set_format_string(char *userinput);
extern bool init_globals(void);
extern void free_globals(void);
extern bool has_telemetry;
extern void *tm_dlhandle;
extern char *bundle_to_add;
extern struct timeval start_time;

extern char *version_server_urls[];
extern char *preferred_version_url;
extern char *content_server_urls[];
extern char *preferred_content_url;
extern long update_server_port;

extern void check_root(void);
extern void clean_curl_multi_queue(void);
extern void increment_retries(int *retries, int *timeout);

extern int main_update(void);
extern int main_verify(int current_version);

extern void read_versions(int *current_version, int *latest_version, int *server_version, char *path_prefix);
extern int check_versions(int *current_version, int *latest_version, int *server_version, char *path_prefix);
extern int read_version_from_subvol_file(char *path_prefix);

extern bool check_network(void);

extern bool ignore(struct file *file);
extern bool is_config(char *filename);
extern bool is_state(char *filename);
extern void apply_heuristics(struct file *file);

extern int file_sort_filename(const void *a, const void *b);
extern int load_manifests(int current, int version, char *component, struct file *file, struct manifest **manifest);
extern struct list *create_update_list(struct manifest *current, struct manifest *server);
extern void link_manifests(struct manifest *m1, struct manifest *m2);
extern void link_submanifests(struct manifest *m1, struct manifest *m2);
extern void free_manifest(struct manifest *manifest);

extern void account_new_file(void);
extern void account_deleted_file(void);
extern void account_changed_file(void);
extern void account_new_manifest(void);
extern void account_deleted_manifest(void);
extern void account_changed_manifest(void);
extern void account_delta_hit(void);
extern void account_delta_miss(void);
extern void print_statistics(int version1, int version2);

extern int download_subscribed_packs(int oldversion, int newversion, bool required);

extern void try_delta(struct file *file);
extern void full_download(struct file *file);
extern int start_full_download(bool pipelining);
extern struct list *end_full_download(void);

extern int do_staging(struct file *file);
extern int rename_all_files_to_final(struct list *updates);
extern int rename_staged_file_to_final(struct file *file);

extern int update_device_latest_version(int version);

extern int swupd_curl_init(void);
extern void swupd_curl_cleanup(void);
extern void swupd_curl_set_current_version(int v);
extern void swupd_curl_set_requested_version(int v);
extern double swupd_query_url_content_size(char *url);
extern size_t swupd_download_file(void *ptr, size_t size, size_t nmemb, void *userdata);
extern int swupd_curl_get_file(const char *url, char *filename, struct file *file,
			       char *tmp_version, bool pack);
#define SWUPD_CURL_LOW_SPEED_LIMIT 1
#define SWUPD_CURL_CONNECT_TIMEOUT 30
#define SWUPD_CURL_RCV_TIMEOUT 120
extern CURLcode swupd_curl_set_basic_options(CURL *curl, const char *url);

extern struct list *subs;
extern void free_subscriptions(void);
extern void read_subscriptions(void);
extern void read_subscriptions_alt(void);
extern int component_subscribed(char *component);
extern int subscription_versions_from_MoM(struct manifest *MoM, int is_old);

extern void hash_assign(char *src, char *dest);
extern bool hash_compare(char *hash1, char *hash2);
extern bool hash_is_zeros(char *hash);
extern int compute_hash_lazy(struct file *file, char *filename);
extern int compute_hash(struct file *file, char *filename) __attribute__((warn_unused_result));

/* manifest.c */
extern int recurse_manifest(struct manifest *manifest, const char *component);
extern void consolidate_submanifests(struct manifest *manifest);
extern void debug_write_manifest(struct manifest *manifest, char *filename);
extern void populate_file_struct(struct file *file, char *filename);
extern bool verify_file(struct file *file, char *filename);
extern void unlink_all_staged_content(struct file *file);
extern void link_renames(struct list *newfiles, struct manifest *from_manifest);
extern void dump_file_descriptor_leaks(void);
extern FILE *fopen_exclusive(const char *filename); /* no mode, opens for write only */
extern int rm_staging_dir_contents(const char *rel_path);
extern int create_required_dirs(void);
extern void dump_file_info(struct file *file);
void free_file_data(void *data);
void remove_files_in_manifest_from_fs(struct manifest *m);
void deduplicate_files_from_manifest(struct manifest **m1, struct manifest *m2);
bool manifest_has_component(struct manifest *manifest, const char *component);

extern char *mounted_dirs;
extern void get_mounted_directories(void);
extern char *mk_full_filename(const char *prefix, const char *path);
extern bool is_directory_mounted(const char *filename);
extern bool is_under_mounted_directory(const char *filename);

extern void run_scripts(void);
extern void run_preupdate_scripts(struct manifest *manifest);

/* lock.c */
int p_lockfile(void);
void v_lockfile(int fd);

/* helpers.c */
extern int swupd_rm(const char *path);
extern int rm_bundle_file(const char *bundle);
extern void print_manifest_files(struct manifest *m);
extern int swupd_init(int *lock_fd);
extern void copyright_header(const char *name);
extern void string_or_die(char **strp, const char *fmt, ...);
void update_motd(int new_release);
void delete_motd(void);
extern int is_dirname_link(const char *fullname);

/* subscription.c */
struct list *free_list_file(struct list *item);
struct list *free_bundle(struct list *item);
extern void create_and_append_subscription(const char *component);

/* bundle.c */
extern int remove_bundle(const char *bundle_name);
extern int list_installable_bundles();
extern int install_bundles(char **bundles);

/* some disk sizes constants for the various features:
 *   ...consider adding build automation to catch at build time
 *      if the build's artifacts are larger than these thresholds */
#define MANIFEST_REQUIRED_SIZE (1024 * 1024 * 100) /* 100M */
#define FREE_MARGIN 10				   /* 10%  */

/****************************************************************/

#ifdef __cplusplus
}
#endif

#endif
