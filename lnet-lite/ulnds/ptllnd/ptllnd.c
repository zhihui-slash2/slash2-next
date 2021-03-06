/*
 * GPL HEADER START
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is included
 * in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; If not, see
 * http://www.lustre.org/lustre/docs/GPLv2.pdf
 *
 * Please contact Xyratex Technology, Ltd., Langstone Road, Havant, Hampshire.
 * PO9 1SA, U.K. or visit www.xyratex.com if you need additional information or
 * have any questions.
 *
 * GPL HEADER END
 */
/*
 * Copyright (c) 2002, 2013, Xyratex Technology, Ltd . All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Some portions of Lustre® software are subject to copyrights help by Intel Corp.
 * Copyright (c) 2011-2013 Intel Corporation, Inc.
 */
/*
 * This file is part of Lustre, http://www.lustre.org/
 * Lustre® and the Lustre logo are registered trademarks of
 * Xyratex Technology, Ltd  in the United States and/or other countries.
 *
 * lnet/ulnds/ptllnd/ptllnd.c
 *
 * Author: Eric Barton <eeb@bartonsoftware.com>
 */

#include "ptllnd.h"

lnd_t               the_ptllnd = {
        .lnd_type       = PTLLND,
        .lnd_startup    = ptllnd_startup,
        .lnd_shutdown   = ptllnd_shutdown,
        .lnd_ctl        = ptllnd_ctl,
        .lnd_send       = ptllnd_send,
        .lnd_recv       = ptllnd_recv,
        .lnd_eager_recv = ptllnd_eager_recv,
        .lnd_wait       = ptllnd_wait,
        .lnd_setasync   = ptllnd_setasync,
};

static int ptllnd_ni_count = 0;

static struct list_head ptllnd_idle_history;
static struct list_head ptllnd_history_list;

void
ptllnd_history_fini(void)
{
        ptllnd_he_t *he;

        while (!list_empty(&ptllnd_idle_history)) {
                he = list_entry(ptllnd_idle_history.next,
                                ptllnd_he_t, he_list);

                list_del(&he->he_list);
                LIBCFS_FREE(he, sizeof(*he));
        }

        while (!list_empty(&ptllnd_history_list)) {
                he = list_entry(ptllnd_history_list.next,
                                ptllnd_he_t, he_list);

                list_del(&he->he_list);
                LIBCFS_FREE(he, sizeof(*he));
        }
}

int
ptllnd_history_init(void)
{
        int          i;
        ptllnd_he_t *he;
        int          n;
        int          rc;

        CFS_INIT_LIST_HEAD(&ptllnd_idle_history);
        CFS_INIT_LIST_HEAD(&ptllnd_history_list);

        rc = ptllnd_parse_int_tunable(&n, "PTLLND_HISTORY", 0);
        if (rc != 0)
                return rc;

        for (i = 0; i < n; i++) {
                LIBCFS_ALLOC(he, sizeof(*he));
                if (he == NULL) {
                        ptllnd_history_fini();
                        return -ENOMEM;
                }

                list_add(&he->he_list, &ptllnd_idle_history);
        }

        PTLLND_HISTORY("Init");

        return 0;
}

void
ptllnd_history(const char *fn, const char *file, const int line,
               const char *fmt, ...)
{
        static int     seq;

        va_list        ap;
        ptllnd_he_t   *he;

        if (!list_empty(&ptllnd_idle_history)) {
                he = list_entry(ptllnd_idle_history.next,
                                ptllnd_he_t, he_list);
        } else if (!list_empty(&ptllnd_history_list)) {
                he = list_entry(ptllnd_history_list.next,
                                ptllnd_he_t, he_list);
        } else {
                return;
        }

        list_del(&he->he_list);
        list_add_tail(&he->he_list, &ptllnd_history_list);

        he->he_seq = seq++;
        he->he_fn = fn;
        he->he_file = file;
        he->he_line = line;
        gettimeofday(&he->he_time, NULL);

        va_start(ap, fmt);
        vsnprintf(he->he_msg, sizeof(he->he_msg), fmt, ap);
        va_end(ap);
}

void
ptllnd_dump_history(void)
{
        ptllnd_he_t    *he;

        PTLLND_HISTORY("dumping...");

        while (!list_empty(&ptllnd_history_list)) {
                he = list_entry(ptllnd_history_list.next,
                                ptllnd_he_t, he_list);

                list_del(&he->he_list);

                CDEBUG(D_WARNING, "%d %d.%06d (%s:%d:%s()) %s\n", he->he_seq,
                       (int)he->he_time.tv_sec, (int)he->he_time.tv_usec,
                       he->he_file, he->he_line, he->he_fn, he->he_msg);

                list_add_tail(&he->he_list, &ptllnd_idle_history);
        }

        PTLLND_HISTORY("complete");
}

void
ptllnd_assert_wire_constants (void)
{
        /* Wire protocol assertions generated by 'wirecheck'
         * running on Linux fedora 2.6.11-co-0.6.4 #1 Mon Jun 19 05:36:13 UTC 2006 i686 i686 i386 GNU
         * with gcc version 4.1.1 20060525 (Red Hat 4.1.1-1) */


        /* Constants... */
        CLASSERT (PTL_RESERVED_MATCHBITS == 0x100);
        CLASSERT (LNET_MSG_MATCHBITS == 0);
        CLASSERT (PTLLND_MSG_MAGIC == 0x50746C4E);
        CLASSERT (PTLLND_MSG_VERSION == 0x04);
        CLASSERT (PTLLND_RDMA_OK == 0x00);
        CLASSERT (PTLLND_RDMA_FAIL == 0x01);
        CLASSERT (PTLLND_MSG_TYPE_INVALID == 0x00);
        CLASSERT (PTLLND_MSG_TYPE_PUT == 0x01);
        CLASSERT (PTLLND_MSG_TYPE_GET == 0x02);
        CLASSERT (PTLLND_MSG_TYPE_IMMEDIATE == 0x03);
        CLASSERT (PTLLND_MSG_TYPE_NOOP == 0x04);
        CLASSERT (PTLLND_MSG_TYPE_HELLO == 0x05);
        CLASSERT (PTLLND_MSG_TYPE_NAK == 0x06);

        /* Checks for struct kptl_msg_t */
        CLASSERT ((int)sizeof(kptl_msg_t) == 136);
        CLASSERT ((int)offsetof(kptl_msg_t, ptlm_magic) == 0);
        CLASSERT ((int)sizeof(((kptl_msg_t *)0)->ptlm_magic) == 4);
        CLASSERT ((int)offsetof(kptl_msg_t, ptlm_version) == 4);
        CLASSERT ((int)sizeof(((kptl_msg_t *)0)->ptlm_version) == 2);
        CLASSERT ((int)offsetof(kptl_msg_t, ptlm_type) == 6);
        CLASSERT ((int)sizeof(((kptl_msg_t *)0)->ptlm_type) == 1);
        CLASSERT ((int)offsetof(kptl_msg_t, ptlm_credits) == 7);
        CLASSERT ((int)sizeof(((kptl_msg_t *)0)->ptlm_credits) == 1);
        CLASSERT ((int)offsetof(kptl_msg_t, ptlm_nob) == 8);
        CLASSERT ((int)sizeof(((kptl_msg_t *)0)->ptlm_nob) == 4);
        CLASSERT ((int)offsetof(kptl_msg_t, ptlm_cksum) == 12);
        CLASSERT ((int)sizeof(((kptl_msg_t *)0)->ptlm_cksum) == 4);
        CLASSERT ((int)offsetof(kptl_msg_t, ptlm_srcnid) == 16);
        CLASSERT ((int)sizeof(((kptl_msg_t *)0)->ptlm_srcnid) == 8);
        CLASSERT ((int)offsetof(kptl_msg_t, ptlm_srcstamp) == 24);
        CLASSERT ((int)sizeof(((kptl_msg_t *)0)->ptlm_srcstamp) == 8);
        CLASSERT ((int)offsetof(kptl_msg_t, ptlm_dstnid) == 32);
        CLASSERT ((int)sizeof(((kptl_msg_t *)0)->ptlm_dstnid) == 8);
        CLASSERT ((int)offsetof(kptl_msg_t, ptlm_dststamp) == 40);
        CLASSERT ((int)sizeof(((kptl_msg_t *)0)->ptlm_dststamp) == 8);
        CLASSERT ((int)offsetof(kptl_msg_t, ptlm_srcpid) == 48);
        CLASSERT ((int)sizeof(((kptl_msg_t *)0)->ptlm_srcpid) == 4);
        CLASSERT ((int)offsetof(kptl_msg_t, ptlm_dstpid) == 52);
        CLASSERT ((int)sizeof(((kptl_msg_t *)0)->ptlm_dstpid) == 4);
        CLASSERT ((int)offsetof(kptl_msg_t, ptlm_u.immediate) == 56);
        CLASSERT ((int)sizeof(((kptl_msg_t *)0)->ptlm_u.immediate) == 72);
        CLASSERT ((int)offsetof(kptl_msg_t, ptlm_u.rdma) == 56);
        CLASSERT ((int)sizeof(((kptl_msg_t *)0)->ptlm_u.rdma) == 80);
        CLASSERT ((int)offsetof(kptl_msg_t, ptlm_u.hello) == 56);
        CLASSERT ((int)sizeof(((kptl_msg_t *)0)->ptlm_u.hello) == 12);

        /* Checks for struct kptl_immediate_msg_t */
        CLASSERT ((int)sizeof(kptl_immediate_msg_t) == 72);
        CLASSERT ((int)offsetof(kptl_immediate_msg_t, kptlim_hdr) == 0);
        CLASSERT ((int)sizeof(((kptl_immediate_msg_t *)0)->kptlim_hdr) == 72);
        CLASSERT ((int)offsetof(kptl_immediate_msg_t, kptlim_payload[13]) == 85);
        CLASSERT ((int)sizeof(((kptl_immediate_msg_t *)0)->kptlim_payload[13]) == 1);

        /* Checks for struct kptl_rdma_msg_t */
        CLASSERT ((int)sizeof(kptl_rdma_msg_t) == 80);
        CLASSERT ((int)offsetof(kptl_rdma_msg_t, kptlrm_hdr) == 0);
        CLASSERT ((int)sizeof(((kptl_rdma_msg_t *)0)->kptlrm_hdr) == 72);
        CLASSERT ((int)offsetof(kptl_rdma_msg_t, kptlrm_matchbits) == 72);
        CLASSERT ((int)sizeof(((kptl_rdma_msg_t *)0)->kptlrm_matchbits) == 8);

        /* Checks for struct kptl_hello_msg_t */
        CLASSERT ((int)sizeof(kptl_hello_msg_t) == 12);
        CLASSERT ((int)offsetof(kptl_hello_msg_t, kptlhm_matchbits) == 0);
        CLASSERT ((int)sizeof(((kptl_hello_msg_t *)0)->kptlhm_matchbits) == 8);
        CLASSERT ((int)offsetof(kptl_hello_msg_t, kptlhm_max_msg_size) == 8);
        CLASSERT ((int)sizeof(((kptl_hello_msg_t *)0)->kptlhm_max_msg_size) == 4);
}

int
ptllnd_parse_int_tunable(int *value, char *name, int dflt)
{
        char    *env = getenv(name);
        char    *end;

        if (env == NULL) {
                *value = dflt;
                return 0;
        }

        *value = strtoull(env, &end, 0);
        if (*end == 0)
                return 0;

        CERROR("Can't parse tunable %s=%s\n", name, env);
        return -EINVAL;
}

int
ptllnd_get_tunables(lnet_ni_t *ni)
{
        ptllnd_ni_t *plni = ni->ni_data;
        int          max_msg_size;
        int          msgs_per_buffer;
        int          rc;
        int          temp;

        /*  Other tunable defaults depend on this */
        rc = ptllnd_parse_int_tunable(&plni->plni_debug, "PTLLND_DEBUG", 0);
        if (rc != 0)
                return rc;

        rc = ptllnd_parse_int_tunable(&plni->plni_portal,
                                      "PTLLND_PORTAL", PTLLND_PORTAL);
        if (rc != 0)
                return rc;

        rc = ptllnd_parse_int_tunable(&temp,
                                      "PTLLND_PID", PTLLND_PID);
        if (rc != 0)
                return rc;
        plni->plni_ptllnd_pid = (ptl_pid_t)temp;

        rc = ptllnd_parse_int_tunable(&plni->plni_peer_credits,
                                      "PTLLND_PEERCREDITS", PTLLND_PEERCREDITS);
        if (rc != 0)
                return rc;
        if (plni->plni_peer_credits > PTLLND_MSG_MAX_CREDITS) {
                CERROR("PTLLND_PEERCREDITS must be <= %d\n", PTLLND_MSG_MAX_CREDITS);
                return -EINVAL;
        }

        rc = ptllnd_parse_int_tunable(&max_msg_size,
                                      "PTLLND_MAX_MSG_SIZE",
                                      PTLLND_MAX_ULND_MSG_SIZE);
        if (rc != 0)
                return rc;

        rc = ptllnd_parse_int_tunable(&msgs_per_buffer,
                                      "PTLLND_MSGS_PER_BUFFER", 64);
        if (rc != 0)
                return rc;

        rc = ptllnd_parse_int_tunable(&plni->plni_msgs_spare,
                                      "PTLLND_MSGS_SPARE", 256);
        if (rc != 0)
                return rc;

        rc = ptllnd_parse_int_tunable(&plni->plni_peer_hash_size,
                                      "PTLLND_PEER_HASH_SIZE", 101);
        if (rc != 0)
                return rc;


        rc = ptllnd_parse_int_tunable(&plni->plni_eq_size,
                                      "PTLLND_EQ_SIZE", 1024);
        if (rc != 0)
                return rc;

        rc = ptllnd_parse_int_tunable(&plni->plni_checksum,
                                      "PTLLND_CHECKSUM", 0);
        if (rc != 0)
                return rc;

        rc = ptllnd_parse_int_tunable(&plni->plni_max_tx_history,
                                      "PTLLND_TX_HISTORY",
                                      plni->plni_debug ? 1024 : 0);
        if (rc != 0)
                return rc;

        rc = ptllnd_parse_int_tunable(&plni->plni_abort_on_protocol_mismatch,
                                      "PTLLND_ABORT_ON_PROTOCOL_MISMATCH", 1);
        if (rc != 0)
                return rc;

        rc = ptllnd_parse_int_tunable(&plni->plni_abort_on_nak,
                                      "PTLLND_ABORT_ON_NAK", 0);
        if (rc != 0)
                return rc;

        rc = ptllnd_parse_int_tunable(&plni->plni_dump_on_nak,
                                      "PTLLND_DUMP_ON_NAK", plni->plni_debug);
        if (rc != 0)
                return rc;

        rc = ptllnd_parse_int_tunable(&plni->plni_watchdog_interval,
                                      "PTLLND_WATCHDOG_INTERVAL", 1);
        if (rc != 0)
                return rc;
        if (plni->plni_watchdog_interval <= 0)
                plni->plni_watchdog_interval = 1;

        rc = ptllnd_parse_int_tunable(&plni->plni_timeout,
                                      "PTLLND_TIMEOUT", 50);
        if (rc != 0)
                return rc;

        rc = ptllnd_parse_int_tunable(&plni->plni_long_wait,
                                      "PTLLND_LONG_WAIT",
                                      plni->plni_debug ? 5 : plni->plni_timeout);
        if (rc != 0)
                return rc;
        plni->plni_long_wait *= 1000;           /* convert to mS */

        plni->plni_max_msg_size = max_msg_size & ~7;
        if (plni->plni_max_msg_size < PTLLND_MIN_BUFFER_SIZE)
                plni->plni_max_msg_size = PTLLND_MIN_BUFFER_SIZE;
        CLASSERT ((PTLLND_MIN_BUFFER_SIZE & 7) == 0);
        CLASSERT (sizeof(kptl_msg_t) <= PTLLND_MIN_BUFFER_SIZE);

        plni->plni_buffer_size = plni->plni_max_msg_size * msgs_per_buffer;

        CDEBUG(D_NET, "portal          = %d\n",plni->plni_portal);
        CDEBUG(D_NET, "ptllnd_pid      = %d\n",plni->plni_ptllnd_pid);
        CDEBUG(D_NET, "max_msg_size    = %d\n",max_msg_size);
        CDEBUG(D_NET, "msgs_per_buffer = %d\n",msgs_per_buffer);
        CDEBUG(D_NET, "msgs_spare      = %d\n",plni->plni_msgs_spare);
        CDEBUG(D_NET, "peer_hash_size  = %d\n",plni->plni_peer_hash_size);
        CDEBUG(D_NET, "eq_size         = %d\n",plni->plni_eq_size);
        CDEBUG(D_NET, "max_msg_size    = %d\n",plni->plni_max_msg_size);
        CDEBUG(D_NET, "buffer_size     = %d\n",plni->plni_buffer_size);

        return 0;
}

ptllnd_buffer_t *
ptllnd_create_buffer (lnet_ni_t *ni)
{
        ptllnd_ni_t     *plni = ni->ni_data;
        ptllnd_buffer_t *buf;

        LIBCFS_ALLOC(buf, sizeof(*buf));
        if (buf == NULL) {
                CERROR("Can't allocate buffer descriptor\n");
                return NULL;
        }

        buf->plb_ni = ni;
        buf->plb_posted = 0;
        CFS_INIT_LIST_HEAD(&buf->plb_list);

        LIBCFS_ALLOC(buf->plb_buffer, plni->plni_buffer_size);
        if (buf->plb_buffer == NULL) {
                CERROR("Can't allocate buffer size %d\n",
                       plni->plni_buffer_size);
                LIBCFS_FREE(buf, sizeof(*buf));
                return NULL;
        }

        list_add(&buf->plb_list, &plni->plni_buffers);
        plni->plni_nbuffers++;

        return buf;
}

void
ptllnd_destroy_buffer (ptllnd_buffer_t *buf)
{
        ptllnd_ni_t     *plni = buf->plb_ni->ni_data;

        LASSERT (!buf->plb_posted);

        plni->plni_nbuffers--;
        list_del(&buf->plb_list);
        LIBCFS_FREE(buf->plb_buffer, plni->plni_buffer_size);
        LIBCFS_FREE(buf, sizeof(*buf));
}

int
ptllnd_size_buffers (lnet_ni_t *ni, int delta)
{
        ptllnd_ni_t     *plni = ni->ni_data;
        ptllnd_buffer_t *buf;
        int              nmsgs;
        int              nbufs;
        int              rc;

        CDEBUG(D_NET, "nposted_buffers = %d (before)\n",plni->plni_nposted_buffers);
        CDEBUG(D_NET, "nbuffers = %d (before)\n",plni->plni_nbuffers);

        plni->plni_nmsgs += delta;
        LASSERT(plni->plni_nmsgs >= 0);

        nmsgs = plni->plni_nmsgs + plni->plni_msgs_spare;

        nbufs = (nmsgs * plni->plni_max_msg_size + plni->plni_buffer_size - 1) /
                plni->plni_buffer_size;

        while (nbufs > plni->plni_nbuffers) {
                buf = ptllnd_create_buffer(ni);

                if (buf == NULL)
                        return -ENOMEM;

                rc = ptllnd_post_buffer(buf);
                if (rc != 0) {
                        /* TODO - this path seems to orpahn the buffer
                         * in a state where its not posted and will never be
                         * However it does not leak the buffer as it's
                         * already been put onto the global buffer list
                         * and will be cleaned up
                         */
                        return rc;
                }
        }

        CDEBUG(D_NET, "nposted_buffers = %d (after)\n",plni->plni_nposted_buffers);
        CDEBUG(D_NET, "nbuffers = %d (after)\n",plni->plni_nbuffers);
        return 0;
}

void
ptllnd_destroy_buffers (lnet_ni_t *ni)
{
        ptllnd_ni_t       *plni = ni->ni_data;
        ptllnd_buffer_t   *buf;
        struct list_head  *tmp;
        struct list_head  *nxt;

        CDEBUG(D_NET, "nposted_buffers = %d (before)\n",plni->plni_nposted_buffers);
        CDEBUG(D_NET, "nbuffers = %d (before)\n",plni->plni_nbuffers);

        list_for_each_safe(tmp, nxt, &plni->plni_buffers) {
                buf = list_entry(tmp, ptllnd_buffer_t, plb_list);

                //CDEBUG(D_NET, "buf=%p posted=%d\n",buf,buf->plb_posted);

                LASSERT (plni->plni_nbuffers > 0);
                if (buf->plb_posted) {
                        time_t   start = cfs_time_current_sec();
                        int      w = plni->plni_long_wait;

                        LASSERT (plni->plni_nposted_buffers > 0);

#ifdef LUSTRE_PORTALS_UNLINK_SEMANTICS
                        (void) PtlMDUnlink(buf->plb_md);

                        while (buf->plb_posted) {
                                if (w > 0 && cfs_time_current_sec() > start + w/1000) {
                                        CWARN("Waited %ds to unlink buffer\n",
                                              (int)(cfs_time_current_sec() - start));
                                        w *= 2;
                                }
                                ptllnd_wait(ni, w);
                        }
#else
                        while (buf->plb_posted) {
                                rc = PtlMDUnlink(buf->plb_md);
                                if (rc == PTL_OK) {
                                        buf->plb_posted = 0;
                                        plni->plni_nposted_buffers--;
                                        break;
                                }
                                LASSERT (rc == PTL_MD_IN_USE);
                                if (w > 0 && cfs_time_current_sec() > start + w/1000) {
                                        CWARN("Waited %ds to unlink buffer\n",
                                              cfs_time_current_sec() - start);
                                        w *= 2;
                                }
                                ptllnd_wait(ni, w);
                        }
#endif
                }
                ptllnd_destroy_buffer(buf);
        }

        CDEBUG(D_NET, "nposted_buffers = %d (after)\n",plni->plni_nposted_buffers);
        CDEBUG(D_NET, "nbuffers = %d (after)\n",plni->plni_nbuffers);

        LASSERT (plni->plni_nposted_buffers == 0);
        LASSERT (plni->plni_nbuffers == 0);
}

int
ptllnd_create_peer_hash (lnet_ni_t *ni)
{
        ptllnd_ni_t *plni = ni->ni_data;
        int          i;

        plni->plni_npeers = 0;

        LIBCFS_ALLOC(plni->plni_peer_hash,
                     plni->plni_peer_hash_size * sizeof(*plni->plni_peer_hash));
        if (plni->plni_peer_hash == NULL) {
                CERROR("Can't allocate ptllnd peer hash (size %d)\n",
                       plni->plni_peer_hash_size);
                return -ENOMEM;
        }

        for (i = 0; i < plni->plni_peer_hash_size; i++)
                CFS_INIT_LIST_HEAD(&plni->plni_peer_hash[i]);

        return 0;
}

void
ptllnd_destroy_peer_hash (lnet_ni_t *ni)
{
        ptllnd_ni_t    *plni = ni->ni_data;
        int             i;

        LASSERT( plni->plni_npeers == 0);

        for (i = 0; i < plni->plni_peer_hash_size; i++)
                LASSERT (list_empty(&plni->plni_peer_hash[i]));

        LIBCFS_FREE(plni->plni_peer_hash,
                    plni->plni_peer_hash_size * sizeof(*plni->plni_peer_hash));
}

void
ptllnd_close_peers (lnet_ni_t *ni)
{
        ptllnd_ni_t    *plni = ni->ni_data;
        ptllnd_peer_t  *plp;
        int             i;

        for (i = 0; i < plni->plni_peer_hash_size; i++)
                while (!list_empty(&plni->plni_peer_hash[i])) {
                        plp = list_entry(plni->plni_peer_hash[i].next,
                                         ptllnd_peer_t, plp_list);

                        ptllnd_close_peer(plp, 0);
                }
}

int
ptllnd_ctl(lnet_ni_t *ni, unsigned int cmd, void *arg)
{
        switch (cmd) {
        case IOC_LIBCFS_DEBUG_PEER:
                ptllnd_dump_debug(ni, *((lnet_process_id_t *)arg));
                return 0;

        default:
                return -EINVAL;
        }
}

__u64
ptllnd_get_timestamp(void)
{
        struct timeval  tv;
        int             rc = gettimeofday(&tv, NULL);

        LASSERT (rc == 0);
        return ((__u64)tv.tv_sec) * 1000000 + tv.tv_usec;
}

void
ptllnd_shutdown (lnet_ni_t *ni)
{
        ptllnd_ni_t *plni = ni->ni_data;
        int          rc;
        time_t       start = cfs_time_current_sec();
        int          w = plni->plni_long_wait;

        LASSERT (ptllnd_ni_count == 1);
        plni->plni_max_tx_history = 0;

        ptllnd_cull_tx_history(plni);

        ptllnd_close_peers(ni);
        ptllnd_destroy_buffers(ni);

        while (plni->plni_npeers > 0) {
                if (w > 0 && cfs_time_current_sec() > start + w/1000) {
                        CWARN("Waited %ds for peers to shutdown\n",
                              (int)(cfs_time_current_sec() - start));
                        w *= 2;
                }
                ptllnd_wait(ni, w);
        }

        LASSERT (plni->plni_ntxs == 0);
        LASSERT (plni->plni_nrxs == 0);

        rc = PtlEQFree(plni->plni_eqh);
        LASSERT (rc == PTL_OK);

        rc = PtlNIFini(plni->plni_nih);
        LASSERT (rc == PTL_OK);

        ptllnd_destroy_peer_hash(ni);
        LIBCFS_FREE(plni, sizeof(*plni));
        ptllnd_ni_count--;
}

int
ptllnd_startup (lnet_ni_t *ni)
{
        ptllnd_ni_t *plni;
        int          rc;

        /* could get limits from portals I guess... */
        ni->ni_maxtxcredits =
        ni->ni_peertxcredits = 1000;

        if (ptllnd_ni_count != 0) {
                CERROR("Can't have > 1 instance of ptllnd\n");
                return -EPERM;
        }

        ptllnd_ni_count++;

        rc = ptllnd_history_init();
        if (rc != 0) {
                CERROR("Can't init history\n");
                goto failed0;
        }

        LIBCFS_ALLOC(plni, sizeof(*plni));
        if (plni == NULL) {
                CERROR("Can't allocate ptllnd state\n");
                rc = -ENOMEM;
                goto failed0;
        }

        ni->ni_data = plni;

        plni->plni_stamp = ptllnd_get_timestamp();
        plni->plni_nrxs = 0;
        plni->plni_ntxs = 0;
        plni->plni_ntx_history = 0;
        plni->plni_watchdog_peeridx = 0;
        plni->plni_watchdog_nextt = cfs_time_current_sec();
        CFS_INIT_LIST_HEAD(&plni->plni_zombie_txs);
        CFS_INIT_LIST_HEAD(&plni->plni_tx_history);

        /*
         *  Initilize buffer related data structures
         */
        CFS_INIT_LIST_HEAD(&plni->plni_buffers);
        plni->plni_nbuffers = 0;
        plni->plni_nposted_buffers = 0;

        rc = ptllnd_get_tunables(ni);
        if (rc != 0)
                goto failed1;

        rc = ptllnd_create_peer_hash(ni);
        if (rc != 0)
                goto failed1;

        /* NB I most probably won't get the PID I requested here.  It doesn't
         * matter because I don't need a fixed PID (only connection acceptors
         * need a "well known" PID). */

        rc = PtlNIInit(PTL_IFACE_DEFAULT, plni->plni_ptllnd_pid,
                       NULL, NULL, &plni->plni_nih);
        if (rc != PTL_OK && rc != PTL_IFACE_DUP) {
                CERROR("PtlNIInit failed: %s(%d)\n",
                       ptllnd_errtype2str(rc), rc);
                rc = -ENODEV;
                goto failed2;
        }

        rc = PtlEQAlloc(plni->plni_nih, plni->plni_eq_size,
                        0, &plni->plni_eqh);
        if (rc != PTL_OK) {
                CERROR("PtlEQAlloc failed: %s(%d)\n",
                       ptllnd_errtype2str(rc), rc);
                rc = -ENODEV;
                goto failed3;
        }

        /*
         * Fetch the Portals NID
         */
        rc = PtlGetId(plni->plni_nih, &plni->plni_portals_id);
        if (rc != PTL_OK) {
                CERROR ("PtlGetID failed : %s(%d)\n",
                        ptllnd_errtype2str(rc), rc);
                rc = -EINVAL;
                goto failed4;
        }

        /*
         * Create the new NID.  Based on the LND network type
         * and the lower ni's address data.
         */
        ni->ni_nid = ptllnd_ptl2lnetnid(ni, plni->plni_portals_id.nid);

        CDEBUG(D_NET, "ptl id  =%s\n", ptllnd_ptlid2str(plni->plni_portals_id));
        CDEBUG(D_NET, "lnet id =%s (passed back)\n",
               libcfs_id2str((lnet_process_id_t) {
                       .nid = ni->ni_nid, .pid = the_lnet.ln_pid}));

        rc = ptllnd_size_buffers(ni, 0);
        if (rc != 0)
                goto failed4;

        return 0;

 failed4:
        ptllnd_destroy_buffers(ni);
        PtlEQFree(plni->plni_eqh);
 failed3:
        PtlNIFini(plni->plni_nih);
 failed2:
        ptllnd_destroy_peer_hash(ni);
 failed1:
        LIBCFS_FREE(plni, sizeof(*plni));
 failed0:
        ptllnd_history_fini();
        ptllnd_ni_count--;
        CDEBUG(D_NET, "<<< rc=%d\n",rc);
        return rc;
}

const char *ptllnd_evtype2str(int type)
{
#define DO_TYPE(x) case x: return #x;
        switch(type)
        {
                DO_TYPE(PTL_EVENT_GET_START);
                DO_TYPE(PTL_EVENT_GET_END);
                DO_TYPE(PTL_EVENT_PUT_START);
                DO_TYPE(PTL_EVENT_PUT_END);
                DO_TYPE(PTL_EVENT_REPLY_START);
                DO_TYPE(PTL_EVENT_REPLY_END);
                DO_TYPE(PTL_EVENT_ACK);
                DO_TYPE(PTL_EVENT_SEND_START);
                DO_TYPE(PTL_EVENT_SEND_END);
                DO_TYPE(PTL_EVENT_UNLINK);
        default:
                return "<unknown event type>";
        }
#undef DO_TYPE
}

const char *ptllnd_msgtype2str(int type)
{
#define DO_TYPE(x) case x: return #x;
        switch(type)
        {
                DO_TYPE(PTLLND_MSG_TYPE_INVALID);
                DO_TYPE(PTLLND_MSG_TYPE_PUT);
                DO_TYPE(PTLLND_MSG_TYPE_GET);
                DO_TYPE(PTLLND_MSG_TYPE_IMMEDIATE);
                DO_TYPE(PTLLND_MSG_TYPE_HELLO);
                DO_TYPE(PTLLND_MSG_TYPE_NOOP);
                DO_TYPE(PTLLND_MSG_TYPE_NAK);
        default:
                return "<unknown msg type>";
        }
#undef DO_TYPE
}

const char *ptllnd_errtype2str(int type)
{
#define DO_TYPE(x) case x: return #x;
        switch(type)
        {
                DO_TYPE(PTL_OK);
                DO_TYPE(PTL_SEGV);
                DO_TYPE(PTL_NO_SPACE);
                DO_TYPE(PTL_ME_IN_USE);
                DO_TYPE(PTL_NAL_FAILED);
                DO_TYPE(PTL_NO_INIT);
                DO_TYPE(PTL_IFACE_DUP);
                DO_TYPE(PTL_IFACE_INVALID);
                DO_TYPE(PTL_HANDLE_INVALID);
                DO_TYPE(PTL_MD_INVALID);
                DO_TYPE(PTL_ME_INVALID);
                DO_TYPE(PTL_PROCESS_INVALID);
                DO_TYPE(PTL_PT_INDEX_INVALID);
                DO_TYPE(PTL_SR_INDEX_INVALID);
                DO_TYPE(PTL_EQ_INVALID);
                DO_TYPE(PTL_EQ_DROPPED);
                DO_TYPE(PTL_EQ_EMPTY);
                DO_TYPE(PTL_MD_NO_UPDATE);
                DO_TYPE(PTL_FAIL);
                DO_TYPE(PTL_AC_INDEX_INVALID);
                DO_TYPE(PTL_MD_ILLEGAL);
                DO_TYPE(PTL_ME_LIST_TOO_LONG);
                DO_TYPE(PTL_MD_IN_USE);
                DO_TYPE(PTL_NI_INVALID);
                DO_TYPE(PTL_PID_INVALID);
                DO_TYPE(PTL_PT_FULL);
                DO_TYPE(PTL_VAL_FAILED);
                DO_TYPE(PTL_NOT_IMPLEMENTED);
                DO_TYPE(PTL_NO_ACK);
                DO_TYPE(PTL_EQ_IN_USE);
                DO_TYPE(PTL_PID_IN_USE);
                DO_TYPE(PTL_INV_EQ_SIZE);
                DO_TYPE(PTL_AGAIN);
        default:
                return "<unknown error type>";
        }
#undef DO_TYPE
}
