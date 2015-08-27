/*
 * include/linux/buffer-trace.h
 *
 * Debugging support for recording buffer_head state transitions
 *
 * May 2001, akpm
 *	Created
 */

#ifndef BUFFER_TRACE_H_INCLUDED
#define BUFFER_TRACE_H_INCLUDED

#include <linux/config.h>

#ifdef CONFIG_BUFFER_DEBUG

/* The number of records per buffer_head. Must be a power of two */
#define BUFFER_HISTORY_SIZE	32

struct buffer_head;

/* This gets embedded in struct buffer_head */
struct buffer_history {
	struct buffer_history_item {
	const char *function;
	char *info;
	unsigned long b_state;
	unsigned b_list:3;
	unsigned b_jlist:4;
	unsigned pg_dirty:1;
	unsigned cpu:3;
	unsigned b_count:8;
	unsigned long b_blocknr;	/* For src != dest */
#if defined(CONFIG_JBD) || defined(CONFIG_JBD_MODULE)
	unsigned b_jcount:4;
	unsigned b_jbd:1;
	unsigned b_transaction:1;
	unsigned b_next_transaction:1;
	unsigned b_cp_transaction:1;
	unsigned b_trans_is_running:1;
	unsigned b_trans_is_committing:1;
	void *b_frozen_data;
	void *b_committed_data;
#endif
	} b[BUFFER_HISTORY_SIZE];
	unsigned long b_history_head;	/* Next place to write */
	unsigned long b_history_tail;	/* Oldest valid entry */
};

static inline void buffer_trace_init(struct buffer_history *bhist)
{
	bhist->b_history_head = 0;
	bhist->b_history_tail = 0;
}
extern void buffer_trace(const char *function, struct buffer_head *dest,
	struct buffer_head *src, char *info);
extern void print_buffer_fields(struct buffer_head *bh);
extern void print_buffer_trace(struct buffer_head *bh);

#define BUFFER_STRINGIFY2(X)	#X
#define BUFFER_STRINGIFY(X)	BUFFER_STRINGIFY2(X)

#define BUFFER_TRACE2(dest, src, info)	\
	do {	\
	buffer_trace(__FUNCTION__, (dest), (src),	\
	"["__FILE__":"	\
	BUFFER_STRINGIFY(__LINE__)"] " info);	\
	} while (0)

#define BUFFER_TRACE(bh, info) BUFFER_TRACE2(bh, bh, info)
#define JBUFFER_TRACE(jh, info)	BUFFER_TRACE(jh2bh(jh), info)

#else	/* CONFIG_BUFFER_DEBUG */

#define buffer_trace_init(bh)	do {} while (0)
#define print_buffer_fields(bh)	do {} while (0)
#define print_buffer_trace(bh)	do {} while (0)
#define BUFFER_TRACE(bh, info)	do {} while (0)
#define BUFFER_TRACE2(bh, bh2, info)	do {} while (0)
#define JBUFFER_TRACE(jh, info)	do {} while (0)

#endif	/* CONFIG_BUFFER_DEBUG */

#endif	/* BUFFER_TRACE_H_INCLUDED */

