/*
 * Copyright(c) 2012-2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ocf_env.h"
#include "ocf_priv.h"
#include "ocf/ocf.h"
#include "ocf/ocf_trace.h"
#include "ocf_core_priv.h"
#include "ocf_cache_priv.h"
#include "ocf_trace_priv.h"

struct core_trace_visitor_ctx {
	ocf_cache_t cache;
	uint32_t io_queue;
};

static int _ocf_core_desc(ocf_core_t core, void  *ctx)
{
	struct ocf_event_core_desc core_desc;
	struct core_trace_visitor_ctx *visitor_ctx =
			(struct core_trace_visitor_ctx *) ctx;
	ocf_cache_t cache = visitor_ctx->cache;

	ocf_event_init_hdr(&core_desc.hdr, ocf_event_type_core_desc,
			ocf_trace_seq_id(cache), env_get_tick_ns(), sizeof(core_desc));
	core_desc.id = ocf_core_get_id(core);
	core_desc.core_size = ocf_data_obj_get_length(
			ocf_core_get_data_object(core));

	ocf_trace_push(cache, visitor_ctx->io_queue,
			&core_desc, sizeof(core_desc));

	return 0;
}

static int _ocf_trace_cache_info(ocf_cache_t cache, uint32_t io_queue)
{
	struct ocf_event_cache_desc cache_desc;
	int retval;
	struct core_trace_visitor_ctx visitor_ctx;

	ocf_event_init_hdr(&cache_desc.hdr, ocf_event_type_cache_desc,
			ocf_trace_seq_id(cache), env_get_tick_ns(), sizeof(cache_desc));

	cache_desc.id = ocf_cache_get_id(cache);
	cache_desc.cache_line_size = ocf_cache_get_line_size(cache);
	cache_desc.cache_mode = ocf_cache_get_mode(cache);

	if (ocf_cache_is_device_attached(cache)) {
		/* Detached cache */
		cache_desc.cache_size = ocf_data_obj_get_length(
				ocf_cache_get_data_object(cache));
	} else {
		cache_desc.cache_size = 0;
	}

	cache_desc.cores_no = ocf_cache_get_core_count(cache);
	cache_desc.version = OCF_EVENT_VERSION;
	cache_desc.io_queues_no = cache->io_queues_no;

	ocf_trace_push(cache, io_queue, &cache_desc, sizeof(cache_desc));

	visitor_ctx.cache = cache;
	visitor_ctx.io_queue = io_queue;

	retval = ocf_core_visit(cache, _ocf_core_desc, &visitor_ctx, true);

	return retval;
}

int ocf_mgnt_start_trace(ocf_cache_t cache, void *trace_ctx,
	ocf_trace_callback_t trace_callback)
{
	int result;
	uint32_t i;

	OCF_CHECK_NULL(cache);

	if (!trace_callback) {
		return -EINVAL;
	}

	result = ocf_mngt_cache_lock(cache);
	if (result)
		return result;

	if (cache->trace.trace_callback) {
		ocf_cache_log(cache, log_err,
				"Tracing already started for cache %u\n",
				ocf_cache_get_id(cache));
		ocf_mngt_cache_unlock(cache);
		return -EINVAL;
	}

	cache->trace.trace_callback = trace_callback;
	cache->trace.trace_ctx = trace_ctx;

	for (i = 0; i < cache->io_queues_no; i++) {
		result = _ocf_trace_cache_info(cache, i);
		if (result)
			goto trace_deinit;
	}

	ocf_cache_log(cache, log_info,
			"Tracing started for cache %u\n", ocf_cache_get_id(cache));

	ocf_mngt_cache_unlock(cache);
	return result;

trace_deinit:
	cache->trace.trace_callback = NULL;
	ocf_mngt_cache_unlock(cache);

	return result;
}

int ocf_mgnt_stop_trace(ocf_cache_t cache)
{
	int result, i;
	bool trace_ongoing;

	result = ocf_mngt_cache_lock(cache);
	if (result)
		return result;

	if (!cache->trace.trace_callback) {
		ocf_cache_log(cache, log_err,
				"Tracing not started for cache %u\n",
				ocf_cache_get_id(cache));
		ocf_mngt_cache_unlock(cache);
		return -EINVAL;
	}

	env_atomic_inc(&cache->trace.stop_trace_pending);

	//Poll for all ongoing traces completion
	do  {
		trace_ongoing = false;
		for (i = 0; i < cache->io_queues_no; i++) {
			if (env_atomic64_read(&cache->io_queues[i].trace_ref_cntr)) {
				trace_ongoing = true;
				break;
			}
		}
		if (trace_ongoing) {
			env_msleep(20);
		} else {
			break;
		}

	} while (true);

	cache->trace.trace_callback = NULL;
	cache->trace.trace_ctx = NULL;

	env_atomic_dec(&cache->trace.stop_trace_pending);
	ocf_mngt_cache_unlock(cache);

	return result;
}
