/*
 * Copyright(c) 2012-2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __OCF_TRACE_H__
#define __OCF_TRACE_H__

typedef uint64_t log_sid_t;

#define OCF_EVENT_VERSION	1

/**
 * @brief OCF trace (event) type
 */
typedef enum {
	/** IO trace description, this event is pushed first to indicate version
	 * of traces, number of cores and provides details about cache */
	ocf_event_type_cache_desc,

	/** Event describing ocf core */
	ocf_event_type_core_desc,

	/** IO */
	ocf_event_type_io,

	/** IO completion */
	ocf_event_type_io_cmpl,

	/** IO in file domain */
	ocf_event_type_io_file,
} ocf_event_type;

/**
 * @brief Generic OCF trace event
 */
struct ocf_event_hdr {
	/** Event sequence ID */
	log_sid_t sid;

	/** Time stamp */
	uint64_t timestamp;

	/** Trace event type */
	ocf_event_type type;

	/** Size of this event */
	uint32_t size;
};

/**
 * @brief Initialize OCF trace event header
 *
 * @param[out] hdr OCF trace event header
 * @param[in] type Type of OCF trace event
 * @param[in] sid Event sequence id
 * @param[in] timestamp Timestamp
 * @param[in] size Size of event
 */
static inline void ocf_event_init_hdr(struct ocf_event_hdr *hdr,
		ocf_event_type type, uint64_t sid, uint64_t timestamp,
		uint32_t size)
{
	hdr->sid = sid;
	hdr->timestamp = timestamp;
	hdr->type = type;
	hdr->size = size;
}

/**
 *  @brief Cache trace description
*/
struct ocf_event_cache_desc {
	/** Event header */
	struct ocf_event_hdr hdr;

	/** Cache Id */
	ocf_cache_id_t id;

	/** Cache line size */
	ocf_cache_line_size_t cache_line_size;

	/** Cache mode */
	ocf_cache_mode_t cache_mode;

	/** Cache size in bytes*/
	uint64_t cache_size;

	/** Number of cores */
	uint32_t cores_no;

	/** Number of IO queues */
	uint32_t io_queues_no;

	/** Trace version */
	uint32_t version;
};

/**
 *  @brief Core trace description
*/
struct ocf_event_core_desc {
	/** Event header */
	struct ocf_event_hdr hdr;

	/** Core Id */
	ocf_core_id_t id;

	/** Core size in bytes */
	uint64_t core_size;
};

/** @brief IO operation */
typedef enum {
	/** Read */
	ocf_event_operation_rd = 'R',

	/** Write */
	ocf_event_operation_wr = 'W',

	/** Flush */
	ocf_event_operation_flush = 'F',

	/** Discard */
	ocf_event_operation_discard = 'D',
} ocf_event_operation_t;

/**
 * @brief IO trace event
 */
struct ocf_event_io {
	/** Trace event header */
	struct ocf_event_hdr hdr;

	/** Address of IO in sectors */
	uint64_t lba;

	/** Size of IO in bytes */
	uint32_t len;

	/** IO class of IO */
	uint32_t io_class;

	/** Core ID */
	ocf_core_id_t core_id;

	/** Operation type: read, write, trim or flush **/
	ocf_event_operation_t dir;
};

/**
 * @brief IO completion event
 */
struct ocf_event_io_cmpl {
	/** Trace event header */
	struct ocf_event_hdr hdr;

	/** Reference event sequence ID */
	log_sid_t rsid;

	/** Was IO a cache hit or miss */
	bool is_hit;
};


/** @brief Push log callback.
 *
 * @param[in] cache OCF cache
 * @param[in] trace_ctx Tracing context
 * @param[in] qid Queue Id
 * @param[out] trace Event log
 * @param[out] size Size of event log
 *
 * @return 0 If pushing trace succeeded
 * @return Non-zero error
 */
typedef void (*ocf_trace_callback_t)(ocf_cache_t cache, void *trace_ctx,
		uint32_t qid, const void* trace, const uint32_t size);

/**
 * @brief Start tracing
 *
 * @param[in] cache OCF cache
 * @param[in] trace_ctx Tracing context
 * @param[in] trace_callback Callback used for pushing logs
 *
 * @retval 0 Tracing started successfully
 * @retval Non-zero Error
 */
int ocf_mgnt_start_trace(ocf_cache_t cache, void *trace_ctx,
		ocf_trace_callback_t trace_callback);

/**
 * @brief Stop tracing
 *
 * @param[in] cache OCF cache
 *
 * @retval 0 Tracing stopped successfully
 * @retval Non-zero Error
 */
int ocf_mgnt_stop_trace(ocf_cache_t cache);

/**
 * @brief Structure used for aggregating trace-related ocf_cache fields
 */
struct ocf_trace {
    /* Placeholder for push_event callback */
	ocf_trace_callback_t trace_callback;

	/* Telemetry context */
	void *trace_ctx;

	env_atomic stop_trace_pending;

	env_atomic64 trace_seq_ref;
};

#endif /* __OCF_TRACE_H__ */
