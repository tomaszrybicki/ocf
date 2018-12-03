/*
 * Copyright(c) 2012-2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __OCF_TRACE_PRIV_H__
#define __OCF_TRACE_PRIV_H__

#include "ocf/ocf_trace.h"
#include "engine/engine_common.h"
#include "ocf_request.h"
#include "ocf_core_priv.h"
#include "ocf_queue_priv.h"

static inline uint64_t ocf_trace_seq_id(ocf_cache_t cache)
{
	return env_atomic64_inc_return(&cache->trace_seq_ref);
}

static inline void ocf_trace_init_io(struct ocf_core_io *io)
{
	ocf_cache_t cache = ocf_core_get_cache(io->core);

	io->timestamp = env_get_tick_ns();
	io->sid = ocf_trace_seq_id(cache);
}

static inline void ocf_trace_prep_io_event(struct ocf_event_io *ev,
		struct ocf_core_io *io, ocf_event_io_dir_t dir)
{
	struct ocf_request *rq = io->req;

	ocf_event_init_hdr(&ev->hdr, ocf_event_type_io, io->sid,
		io->timestamp, sizeof(*ev));

	ev->lba = rq->byte_position >> SECTOR_SHIFT;
	if (ocf_event_io_dir_discard == dir) {
		ev->len = rq->discard.nr_sects;
	} else {
		ev->len = rq->byte_length >> SECTOR_SHIFT;
	}

	ev->dir = dir;
	ev->core_id = rq->core_id;

	ev->io_class = rq->io->io_class;
}

static inline void ocf_trace_push(ocf_cache_t cache, uint32_t io_queue,
	void *trace, uint32_t size)
{
	if (!env_atomic_read(&cache->stop_trace_pending)) {
		env_atomic64_inc(&cache->io_queues[io_queue].trace_ref_cntr);

		//Double stop_trace_pending flag check prevents from using
		//callback when traces are being stopped
		if (!env_atomic_read(&cache->stop_trace_pending) &&
			cache->trace_callback) {
			cache->trace_callback(cache, cache->trace_ctx,
				io_queue, trace, size);
		}

		env_atomic64_dec(&cache->io_queues[io_queue].trace_ref_cntr);
	}
}

static inline void ocf_trace_io(struct ocf_core_io *io, ocf_event_io_dir_t dir)
{
	struct ocf_event_io ev;
	struct ocf_request *rq;
	ocf_cache_t cache = ocf_core_get_cache(io->core);

	if (!cache->trace_callback)
		return;

	rq  = io->req;
	ocf_trace_prep_io_event(&ev, io, dir);

	ocf_trace_push(cache, rq->io_queue, &ev, sizeof(ev));
}

static inline void ocf_trace_io_cmpl(struct ocf_core_io *io)
{
	struct ocf_event_io_cmpl ev;
	struct ocf_request *rq;
	ocf_cache_t cache = ocf_core_get_cache(io->core);

	if (!cache->trace_callback)
		return;

	rq = io->req;
	ocf_event_init_hdr(&ev.hdr, ocf_event_type_io_cmpl,
		ocf_trace_seq_id(cache), env_get_tick_ns(), sizeof(ev));
	ev.rsid = io->sid;
	ev.is_hit = ocf_engine_is_hit(rq);

	ocf_trace_push(cache, rq->io_queue, &ev, sizeof(ev));
}

#endif /* __OCF_TRACE_PRIV_H__ */
