/*
 * Copyright(c) 2012-2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __OCF_CORE_PRIV_H__
#define __OCF_CORE_PRIV_H__

#include "ocf/ocf.h"
#include "ocf_env.h"
#include "ocf_data_obj_priv.h"

struct ocf_core_io {
	struct ocf_io base;
	ocf_core_t core;

	env_atomic ref_counter;

	bool dirty;
	/*!< Indicates if io leaves dirty data  */

	struct ocf_request *req;
	ctx_data_t *data;

	log_sid_t sid;
	/*!< Sequence ID */

	uint64_t timestamp;
	/*!< Timestamp */
};

struct ocf_core {
	char name[OCF_CORE_NAME_SIZE];

	struct ocf_data_obj obj;

	struct {
		uint64_t last;
		uint64_t bytes;
		int rw;
	} seq_cutoff;

	env_atomic flushed;

	/* This bit means that object is open*/
	uint32_t opened : 1;

	struct ocf_counters_core *counters;
};

bool ocf_core_is_valid(ocf_cache_t cache, ocf_core_id_t id);

int ocf_core_set_user_metadata_raw(ocf_core_t core, void *data, size_t size);

#define ocf_core_log_prefix(core, lvl, prefix, fmt, ...) \
	ocf_cache_log_prefix(ocf_core_get_cache(core), lvl, ".%s" prefix, \
			fmt, ocf_core_get_name(core), ##__VA_ARGS__)

#define ocf_core_log(core, lvl, fmt, ...) \
	ocf_core_log_prefix(core, lvl, ": ", fmt, ##__VA_ARGS__)

#endif /* __OCF_CORE_PRIV_H__ */
