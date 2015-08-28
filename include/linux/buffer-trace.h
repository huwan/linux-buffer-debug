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
#if defined(CONFIG_JBD2) || defined(CONFIG_JBD2_MODULE)
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

#endif	/* CONFIG_BUFFER_DEBUG */

#endif	/* BUFFER_TRACE_H_INCLUDED */
