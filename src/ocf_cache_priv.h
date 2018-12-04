/*
 * Copyright(c) 2012-2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __OCF_CACHE_PRIV_H__
#define __OCF_CACHE_PRIV_H__

#include "ocf/ocf.h"
#include "ocf_env.h"
#include "ocf_data_obj_priv.h"
#include "ocf_core_priv.h"
#include "metadata/metadata_structs.h"
#include "metadata/metadata_partition_structs.h"
#include "metadata/metadata_updater_priv.h"
#include "utils/utils_list.h"
#include "ocf_stats_priv.h"
#include "cleaning/cleaning.h"
#include "ocf_logger_priv.h"
#include "ocf/ocf_trace.h"

#define DIRTY_FLUSHED 1
#define DIRTY_NOT_FLUSHED 0

struct ocf_metadata_uuid {
	uint32_t size;
	uint8_t data[OCF_DATA_OBJ_UUID_MAX_SIZE];
} __packed;

#define OCF_CORE_USER_DATA_SIZE 64

struct ocf_core_meta_config {
	uint8_t type;

	/* This bit means that object was added into cache */
	uint32_t added : 1;

	/* Core sequence number used to correlate cache lines with cores
	 * when recovering from atomic device */
	ocf_seq_no_t seq_no;

	/* Sequential cutoff threshold (in bytes) */
	uint32_t seq_cutoff_threshold;

	/* Sequential cutoff policy */
	ocf_seq_cutoff_policy seq_cutoff_policy;

	/* core object size in bytes */
	uint64_t length;

	uint8_t user_data[OCF_CORE_USER_DATA_SIZE];
};

struct ocf_core_meta_runtime {
	/* Number of blocks from that objects that currently are cached
	 * on the caching device.
	 */
	env_atomic cached_clines;
	env_atomic dirty_clines;
	env_atomic initial_dirty_clines;

	env_atomic64 dirty_since;

	struct {
		/* clines within lru list (?) */
		env_atomic cached_clines;
		/* dirty clines assigned to this specific partition within
		 * cache device
		 */
		env_atomic dirty_clines;
	} part_counters[OCF_IO_CLASS_MAX];
};

/**
 * @brief Initialization mode of cache instance
 */
enum ocf_mngt_cache_init_mode {
	/**
	 * @brief Set up an SSD as new caching device
	 */
	ocf_init_mode_init,

	/**
	 * @brief Set up an SSD as new caching device without saving cache
	 * metadata on SSD.
	 *
	 * When using this initialization mode, after shutdown, loading cache
	 * is not possible
	 */
	ocf_init_mode_metadata_volatile,

	/**
	 * @brief Load pre-existing SSD cache state and set all parameters
	 *		to previous configurations
	 */
	ocf_init_mode_load,
};

/* Cache device */
struct ocf_cache_device {
	struct ocf_data_obj obj;

	ocf_cache_line_t metadata_offset_line;

	/* Hash Table contains contains pointer to the entry in
	 * Collision Table so it actually contains collision Table
	 * indexes.
	 * Invalid entry is collision_table_entries.
	 */
	unsigned int hash_table_entries;
	unsigned int collision_table_entries;

	int metadata_error;
		/*!< This field indicates that an error during metadata IO
		 * occurred
	 */

	uint64_t metadata_offset;

	struct ocf_part *freelist_part;

	struct {
		struct ocf_cache_concurrency *cache;
	} concurrency;

	enum ocf_mngt_cache_init_mode init_mode;

	struct ocf_superblock_runtime *runtime_meta;
};

struct ocf_cache {
	ocf_ctx_t owner;

	struct list_head list;
	/* set to make valid */
	uint8_t valid_ocf_cache_device_t;
	/* unset running to not serve any more I/O requests */
	unsigned long cache_state;

	env_atomic ref_count;

	struct ocf_superblock_config *conf_meta;

	struct ocf_cache_device *device;

	struct ocf_lst lst_part;
	struct ocf_user_part user_parts[OCF_IO_CLASS_MAX + 1];

	struct ocf_metadata metadata;

	ocf_eviction_t eviction_policy_init;

	int cache_id;

	char name[OCF_CACHE_NAME_SIZE];

	env_atomic pending_requests;

	env_atomic pending_cache_requests;
	env_waitqueue pending_cache_wq;

	env_atomic pending_dirty_requests;
	env_waitqueue pending_dirty_wq;

	uint32_t fallback_pt_error_threshold;
	env_atomic fallback_pt_error_counter;

	env_atomic pending_read_misses_list_blocked;
	env_atomic pending_read_misses_list_count;

	env_atomic last_access_ms;

	env_atomic pending_eviction_clines;

	struct ocf_queue *io_queues;
	uint32_t io_queues_no;

	uint16_t ocf_core_inactive_count;
	struct ocf_core core_obj[OCF_CORE_MAX];
	struct ocf_core_meta_config *core_conf_meta;
	struct ocf_core_meta_runtime *core_runtime_meta;

	env_atomic flush_in_progress;
	env_atomic flush_started;

	/* 1 if cache device attached, 0 otherwise */
	env_atomic attached;

	env_atomic cleaning[OCF_IO_CLASS_MAX];

	struct ocf_cleaner cleaner;
	struct ocf_metadata_updater metadata_updater;

	env_rwsem lock;
	env_atomic lock_waiter;
	/*!< most of the time this variable is set to 0, unless user requested
	 *!< interruption of flushing process via ioctl/
	 */
	int flushing_interrupted;
	env_mutex flush_mutex;

	struct {
		uint32_t max_queue_size;
		uint32_t queue_unblock_size;
	} backfill;

	bool pt_unaligned_io;

	bool use_submit_io_fast;

	void *cleaning_policy_context;

	struct ocf_trace trace;
};

#define ocf_cache_log_prefix(cache, lvl, prefix, fmt, ...) \
	ocf_log_prefix(ocf_cache_get_ctx(cache), lvl, "[Cache %s] ", \
			prefix fmt, ocf_cache_get_name(cache), ##__VA_ARGS__)

#define ocf_cache_log(cache, lvl, fmt, ...) \
	ocf_cache_log_prefix(cache, lvl, "", fmt, ##__VA_ARGS__)

#define ocf_cache_log_rl(cache) \
	ocf_log_rl(ocf_cache_get_ctx(cache))

#endif /* __OCF_CACHE_PRIV_H__ */
