/*
 * fs/jbd-kernel.c
 *
 * Support code for the Journalling Block Device layer.
 * This file contains things which have to be in-kernel when
 * JBD is a module.
 *
 * 15 May 2001	Andrew Morton <andrewm@xxxxxxxxxx>
 *	Created
 */

#include <linux/config.h>
#include <linux/fs.h>
#include <linux/jbd.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/mm.h>

/*
 * Some sanity testing which is called from mark_buffer_clean(),
 * and must be present in the main kernel.
 */

void jbd_preclean_buffer_check(struct buffer_head *bh)
{
	if (buffer_jbd(bh)) {
	struct journal_head *jh = bh2jh(bh);

	transaction_t *transaction = jh->b_transaction;
	journal_t *journal;

	if (jh->b_jlist == 0 && transaction == NULL)
	return;

	J_ASSERT_JH(jh, transaction != NULL);
	/* The kernel may be unmapping old data. We expect it
	* to be dirty in that case, unless the buffer has
	* already been forgotten by a transaction. */
	if (jh->b_jlist != BJ_Forget) {
#if 1
	if (!buffer_dirty(bh)) {
	printk("%s: clean of clean buffer\n",
	__FUNCTION__);
	print_buffer_trace(bh);
	return;
	}
#endif
	J_ASSERT_BH(bh, buffer_dirty(bh));
	}

	journal = transaction->t_journal;
	J_ASSERT_JH(jh,
	transaction == journal->j_running_transaction ||
	transaction == journal->j_committing_transaction);
	}
}
EXPORT_SYMBOL(jbd_preclean_buffer_check);

/*
 * Support functions for BUFFER_TRACE()
 */

static spinlock_t trace_lock = SPIN_LOCK_UNLOCKED;

void buffer_trace(const char *function, struct buffer_head *dest,
	struct buffer_head *src, char *info)
{
	struct buffer_history_item *bhist_i;
	struct page *page;
	unsigned long flags;

	if (dest == 0 || src == 0)
	return;

	spin_lock_irqsave(&trace_lock, flags);

	/*
	* Sometimes we don't initialise the ring pointers. (locally declared
	* temp buffer_heads). Feebly attempt to detect and correct that here.
	*/
	if ((dest->b_history.b_history_head - dest->b_history.b_history_tail >
	BUFFER_HISTORY_SIZE)) {
	dest->b_history.b_history_head = 0;
	dest->b_history.b_history_tail = 0;
	}
	bhist_i = dest->b_history.b +
	(dest->b_history.b_history_head & (BUFFER_HISTORY_SIZE - 1));
	bhist_i->function = function;
	bhist_i->info = info;
	bhist_i->b_state = src->b_state;
	page = src->b_page;
	if (page)
	bhist_i->pg_dirty = !!PageDirty(page);
	else
	bhist_i->pg_dirty = 0;

#if defined(CONFIG_JBD) || defined(CONFIG_JBD_MODULE)
	bhist_i->b_trans_is_running = 0;
	bhist_i->b_trans_is_committing = 0;
	bhist_i->b_blocknr = src->b_blocknr;
	if (buffer_jbd(src)) {
	struct journal_head *jh;
	journal_t *journal;
	transaction_t *transaction;

	/* Footwork to avoid racing with journal_remove_journal_head */
	jh = src->b_private;
	if (jh == 0)
	goto raced;
	transaction = jh->b_transaction;
	if (src->b_private == 0)
	goto raced;
	bhist_i->b_jcount = jh->b_jcount;
	bhist_i->b_jbd = 1;
	bhist_i->b_jlist = jh->b_jlist;
	bhist_i->b_frozen_data = jh->b_frozen_data;
	bhist_i->b_committed_data = jh->b_committed_data;
	bhist_i->b_transaction = !!jh->b_transaction;
	bhist_i->b_next_transaction = !!jh->b_next_transaction;
	bhist_i->b_cp_transaction = !!jh->b_cp_transaction;

	if (transaction) {
	journal = transaction->t_journal;
	bhist_i->b_trans_is_running = transaction ==
	journal->j_running_transaction;
	bhist_i->b_trans_is_committing = transaction ==
	journal->j_committing_transaction;
	}
	} else {
raced:
	bhist_i->b_jcount = 0;
	bhist_i->b_jbd = 0;
	bhist_i->b_jlist = 0;
	bhist_i->b_frozen_data = 0;
	bhist_i->b_committed_data = 0;
	bhist_i->b_transaction = 0;
	bhist_i->b_next_transaction = 0;
	bhist_i->b_cp_transaction = 0;
	}
#endif	/* defined(CONFIG_JBD) || defined(CONFIG_JBD_MODULE) */

	bhist_i->cpu = smp_processor_id();
	bhist_i->b_count = atomic_read(&src->b_count);

	dest->b_history.b_history_head++;
	if (dest->b_history.b_history_head - dest->b_history.b_history_tail >
	BUFFER_HISTORY_SIZE)
	dest->b_history.b_history_tail =
	dest->b_history.b_history_head - BUFFER_HISTORY_SIZE;

	spin_unlock_irqrestore(&trace_lock, flags);
}

static const char *b_jlist_to_string(unsigned int b_list)
{
	switch (b_list) {
#if defined(CONFIG_JBD) || defined(CONFIG_JBD_MODULE)
	case BJ_None:	return "BJ_None";
	case BJ_SyncData:	return "BJ_SyncData";
	case BJ_Metadata:	return "BJ_Metadata";
	case BJ_Forget:	return "BJ_Forget";
	case BJ_IO:	return "BJ_IO";
	case BJ_Shadow:	return "BJ_Shadow";
	case BJ_LogCtl:	return "BJ_LogCtl";
	case BJ_Reserved:	return "BJ_Reserved";
	case BJ_Locked:	return "BJ_Locked";
#endif
	default:	return "Bad b_jlist";
	}
}

static void print_one_hist(struct buffer_history_item *bhist_i)
{
	printk(" %s():%s\n", bhist_i->function, bhist_i->info);
	printk(" b_state:0x%lx b_jlist:%s cpu:%d b_count:%d b_blocknr:%lu\n",
	bhist_i->b_state,
	b_jlist_to_string(bhist_i->b_jlist),
	bhist_i->cpu,
	bhist_i->b_count,
	bhist_i->b_blocknr);
#if defined(CONFIG_JBD) || defined(CONFIG_JBD_MODULE)
	printk(" b_jbd:%u b_frozen_data:%p b_committed_data:%p\n",
	bhist_i->b_jbd,
	bhist_i->b_frozen_data,
	bhist_i->b_committed_data);
	printk(" b_transaction:%u b_next_transaction:%u "
	"b_cp_transaction:%u b_trans_is_running:%u\n",
	bhist_i->b_transaction,
	bhist_i->b_next_transaction,
	bhist_i->b_cp_transaction,
	bhist_i->b_trans_is_running);
	printk(" b_trans_is_comitting:%u b_jcount:%u pg_dirty:%u",
	bhist_i->b_trans_is_committing,
	bhist_i->b_jcount,
	bhist_i->pg_dirty);
#endif
	printk("\n");
}

void print_buffer_fields(struct buffer_head *bh)
{
	printk("b_blocknr:%llu b_count:%d\n",
	(unsigned long long)bh->b_blocknr, atomic_read(&bh->b_count));
	printk("b_this_page:%p b_data:%p b_page:%p\n",
	bh->b_this_page, bh->b_data, bh->b_page);
#if defined(CONFIG_JBD) || defined(CONFIG_JBD_MODULE)
	if (buffer_jbd(bh)) {
	struct journal_head *jh = bh2jh(bh);

	printk("b_jlist:%u b_frozen_data:%p b_committed_data:%p\n",
	jh->b_jlist, jh->b_frozen_data, jh->b_committed_data);
	printk(" b_transaction:%p b_next_transaction:%p "
	"b_cp_transaction:%p\n",
	jh->b_transaction, jh->b_next_transaction,
	jh->b_cp_transaction);
	printk("b_cpnext:%p b_cpprev:%p\n",
	jh->b_cpnext, jh->b_cpprev);
	}
#endif
}

void print_buffer_trace(struct buffer_head *bh)
{
	unsigned long idx, count;
	unsigned long flags;

	printk("buffer trace for buffer at 0x%p (I am CPU %d)\n",
	bh, smp_processor_id());
	BUFFER_TRACE(bh, "");	/* Record state now */

	spin_lock_irqsave(&trace_lock, flags);
	for (	idx = bh->b_history.b_history_tail, count = 0;
	idx < bh->b_history.b_history_head &&
	count < BUFFER_HISTORY_SIZE;
	idx++, count++)
	print_one_hist(bh->b_history.b +
	(idx & (BUFFER_HISTORY_SIZE - 1)));

	print_buffer_fields(bh);
	spin_unlock_irqrestore(&trace_lock, flags);
	dump_stack();
	printk("\n");
}

static struct buffer_head *failed_buffer_head;	/* For access with debuggers */

void buffer_assertion_failure(struct buffer_head *bh)
{
	console_verbose();
	failed_buffer_head = bh;
	print_buffer_trace(bh);
}
EXPORT_SYMBOL(buffer_trace);
EXPORT_SYMBOL(print_buffer_trace);
EXPORT_SYMBOL(buffer_assertion_failure);
EXPORT_SYMBOL(print_buffer_fields);
