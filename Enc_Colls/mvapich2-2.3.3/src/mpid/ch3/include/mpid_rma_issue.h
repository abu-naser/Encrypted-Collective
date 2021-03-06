/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPID_RMA_ISSUE_H_INCLUDED)
#define MPID_RMA_ISSUE_H_INCLUDED

#include "mpl_utlist.h"
#include "mpid_rma_types.h"

/* =========================================================== */
/*                    auxiliary functions                      */
/* =========================================================== */

/* immed_copy() copys data from origin buffer to
   IMMED packet header. */
#undef FUNCNAME
#define FUNCNAME immed_copy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int immed_copy(void *src, void *dest, size_t len)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_IMMED_COPY);

    MPIDI_FUNC_ENTER(MPID_STATE_IMMED_COPY);

    if (src == NULL || dest == NULL || len == 0)
        goto fn_exit;

    switch (len) {
    case 1:
        *(uint8_t *) dest = *(uint8_t *) src;
        break;
#ifndef NEEDS_STRICT_ALIGNMENT
        /* Following copy is unsafe on platforms that require strict
         * alignment (e.g., SPARC). Because the buffers may not be aligned
         * for data type access except char. */
    case 2:
        *(uint16_t *) dest = *(uint16_t *) src;
        break;
    case 4:
        *(uint32_t *) dest = *(uint32_t *) src;
        break;
    case 8:
        *(uint64_t *) dest = *(uint64_t *) src;
        break;
#endif
    default:
        MPIU_Memcpy(dest, (void *) src, len);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_IMMED_COPY);
    return mpi_errno;
}

/* =========================================================== */
/*                  extended packet functions                  */
/* =========================================================== */

/* Copy derived datatype information issued within RMA operation. */
#undef FUNCNAME
#define FUNCNAME fill_in_derived_dtp_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void fill_in_derived_dtp_info(MPIDI_RMA_dtype_info * dtype_info, void *dataloop,
                                            MPID_Datatype * dtp)
{
    MPIDI_STATE_DECL(MPID_STATE_FILL_IN_DERIVED_DTP_INFO);
    MPIDI_FUNC_ENTER(MPID_STATE_FILL_IN_DERIVED_DTP_INFO);

    /* Derived datatype on target, fill derived datatype info. */
    dtype_info->is_contig = dtp->is_contig;
    dtype_info->max_contig_blocks = dtp->max_contig_blocks;
    dtype_info->size = dtp->size;
    dtype_info->extent = dtp->extent;
    dtype_info->dataloop_size = dtp->dataloop_size;
    dtype_info->dataloop_depth = dtp->dataloop_depth;
    dtype_info->basic_type = dtp->basic_type;
    dtype_info->dataloop = dtp->dataloop;
    dtype_info->ub = dtp->ub;
    dtype_info->lb = dtp->lb;
    dtype_info->true_ub = dtp->true_ub;
    dtype_info->true_lb = dtp->true_lb;
    dtype_info->has_sticky_ub = dtp->has_sticky_ub;
    dtype_info->has_sticky_lb = dtp->has_sticky_lb;

    MPIU_Assert(dataloop != NULL);
    MPIU_Memcpy(dataloop, dtp->dataloop, dtp->dataloop_size);
    /* The dataloop can have undefined padding sections, so we need to let
     * valgrind know that it is OK to pass this data to writev later on. */
    MPL_VG_MAKE_MEM_DEFINED(dataloop, dtp->dataloop_size);

    MPIDI_FUNC_EXIT(MPID_STATE_FILL_IN_DERIVED_DTP_INFO);
}

/* Set extended header for ACC operation and return its real size. */
#undef FUNCNAME
#define FUNCNAME init_accum_ext_pkt
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int init_accum_ext_pkt(MPIDI_CH3_Pkt_flags_t flags,
                              MPID_Datatype * target_dtp, MPIDI_msg_sz_t stream_offset,
                              void **ext_hdr_ptr, MPI_Aint * ext_hdr_sz)
{
    MPI_Aint _ext_hdr_sz = 0, _total_sz = 0;
    void *dataloop_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_INIT_ACCUM_EXT_PKT);
    MPIDI_FUNC_ENTER(MPID_STATE_INIT_ACCUM_EXT_PKT);

    if ((flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) && target_dtp != NULL) {
        MPIDI_CH3_Ext_pkt_accum_stream_derived_t *_ext_hdr_ptr = NULL;

        /* dataloop is behind of extended header on origin.
         * TODO: support extended header array */
        _ext_hdr_sz = sizeof(MPIDI_CH3_Ext_pkt_accum_stream_derived_t);
        _total_sz = _ext_hdr_sz + target_dtp->dataloop_size;

        _ext_hdr_ptr = (MPIDI_CH3_Ext_pkt_accum_stream_derived_t *) MPIU_Malloc(_total_sz);
        MPL_VG_MEM_INIT(_ext_hdr_ptr, _total_sz);
        if (_ext_hdr_ptr == NULL) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                 "**nomem %s", "MPIDI_CH3_Ext_pkt_accum_stream_derived_t");
        }

        _ext_hdr_ptr->stream_offset = stream_offset;

        dataloop_ptr = (void *) ((char *) _ext_hdr_ptr + _ext_hdr_sz);
        fill_in_derived_dtp_info(&_ext_hdr_ptr->dtype_info, dataloop_ptr, target_dtp);

        (*ext_hdr_ptr) = _ext_hdr_ptr;
    }
    else if (flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) {
        MPIDI_CH3_Ext_pkt_accum_stream_t *_ext_hdr_ptr = NULL;

        _total_sz = sizeof(MPIDI_CH3_Ext_pkt_accum_stream_t);

        _ext_hdr_ptr = (MPIDI_CH3_Ext_pkt_accum_stream_t *) MPIU_Malloc(_total_sz);
        MPL_VG_MEM_INIT(_ext_hdr_ptr, _total_sz);
        if (_ext_hdr_ptr == NULL) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                 "**nomem %s", "MPIDI_CH3_Ext_pkt_accum_stream_t");
        }

        _ext_hdr_ptr->stream_offset = stream_offset;
        (*ext_hdr_ptr) = _ext_hdr_ptr;
    }
    else if (target_dtp != NULL) {
        MPIDI_CH3_Ext_pkt_accum_derived_t *_ext_hdr_ptr = NULL;

        /* dataloop is behind of extended header on origin.
         * TODO: support extended header array */
        _ext_hdr_sz = sizeof(MPIDI_CH3_Ext_pkt_accum_derived_t);
        _total_sz = _ext_hdr_sz + target_dtp->dataloop_size;

        _ext_hdr_ptr = (MPIDI_CH3_Ext_pkt_accum_derived_t *) MPIU_Malloc(_total_sz);
        MPL_VG_MEM_INIT(_ext_hdr_ptr, _total_sz);
        if (_ext_hdr_ptr == NULL) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                 "**nomem %s", "MPIDI_CH3_Ext_pkt_accum_derived_t");
        }

        dataloop_ptr = (void *) ((char *) _ext_hdr_ptr + _ext_hdr_sz);
        fill_in_derived_dtp_info(&_ext_hdr_ptr->dtype_info, dataloop_ptr, target_dtp);

        (*ext_hdr_ptr) = _ext_hdr_ptr;
    }

    (*ext_hdr_sz) = _total_sz;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_INIT_ACCUM_EXT_PKT);
    return mpi_errno;
  fn_fail:
    if ((*ext_hdr_ptr))
        MPIU_Free((*ext_hdr_ptr));
    (*ext_hdr_ptr) = NULL;
    (*ext_hdr_sz) = 0;
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME init_get_accum_ext_pkt
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int init_get_accum_ext_pkt(MPIDI_CH3_Pkt_flags_t flags,
                                  MPID_Datatype * target_dtp, MPIDI_msg_sz_t stream_offset,
                                  void **ext_hdr_ptr, MPI_Aint * ext_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_INIT_GET_ACCUM_EXT_PKT);
    MPIDI_FUNC_ENTER(MPID_STATE_INIT_GET_ACCUM_EXT_PKT);

    /* Check if get_accum still reuses accum' extended packet header. */
    MPIU_Assert(sizeof(MPIDI_CH3_Ext_pkt_accum_stream_derived_t) ==
                sizeof(MPIDI_CH3_Ext_pkt_get_accum_stream_derived_t));
    MPIU_Assert(sizeof(MPIDI_CH3_Ext_pkt_accum_derived_t) ==
                sizeof(MPIDI_CH3_Ext_pkt_get_accum_derived_t));
    MPIU_Assert(sizeof(MPIDI_CH3_Ext_pkt_accum_stream_t) ==
                sizeof(MPIDI_CH3_Ext_pkt_get_accum_stream_t));

    mpi_errno = init_accum_ext_pkt(flags, target_dtp, stream_offset, ext_hdr_ptr, ext_hdr_sz);

    MPIDI_FUNC_EXIT(MPID_STATE_INIT_GET_ACCUM_EXT_PKT);
    return mpi_errno;
}

/* =========================================================== */
/*                      issuinng functions                     */
/* =========================================================== */

/* issue_from_origin_buffer() issues data from origin
   buffer (i.e. non-IMMED operation). */
#undef FUNCNAME
#define FUNCNAME issue_from_origin_buffer
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int issue_from_origin_buffer(MPIDI_RMA_Op_t * rma_op, MPIDI_VC_t * vc,
                                    void *ext_hdr_ptr, MPI_Aint ext_hdr_sz,
                                    MPIDI_msg_sz_t stream_offset, MPIDI_msg_sz_t stream_size,
                                    MPID_Request ** req_ptr)
{
    MPI_Datatype target_datatype;
    MPID_Datatype *target_dtp = NULL, *origin_dtp = NULL;
    int is_origin_contig;
    MPL_IOV iov[MPL_IOV_LIMIT] = {0};
    int iovcnt = 0;
    MPID_Request *req = NULL;
    MPI_Aint dt_true_lb;
    MPIDI_CH3_Pkt_flags_t flags;
    int is_empty_origin = FALSE;
#if defined(CHANNEL_PSM)
    int is_replace_op = FALSE;
#endif
    int mpi_errno = MPI_SUCCESS;
#if defined(CHANNEL_MRAIL)
    MPIDI_msg_sz_t total_length;
#endif
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_FROM_ORIGIN_BUFFER);

    MPIDI_FUNC_ENTER(MPID_STATE_ISSUE_FROM_ORIGIN_BUFFER);

    /* Judge if origin buffer is empty (this can only happens for
     * GACC and FOP when op is MPI_NO_OP). */
    if ((rma_op->pkt).type == MPIDI_CH3_PKT_GET_ACCUM || (rma_op->pkt).type == MPIDI_CH3_PKT_FOP) {
        MPI_Op op;
        MPIDI_CH3_PKT_RMA_GET_OP(rma_op->pkt, op, mpi_errno);
        if (op == MPI_NO_OP)
            is_empty_origin = TRUE;
    }
#if defined(CHANNEL_PSM)
    /* MPI_REPLACE is a unique operation for RMA Accumulate, and
     * it is not commutative. So we need to make sure the origin data
     * be sent in the order */
    /* Similar logic in MRAIL channel is handled in MPIDI_CH3_iStartRmaRndv */
    if ((rma_op->pkt).type == MPIDI_CH3_PKT_ACCUMULATE || (rma_op->pkt).type == MPIDI_CH3_PKT_GET_ACCUM) {
        MPI_Op op;
        MPIDI_CH3_PKT_RMA_GET_OP(rma_op->pkt, op, mpi_errno);
        if (op == MPI_REPLACE)
            is_replace_op = TRUE;
    }
#endif

    /* Judge if target datatype is derived datatype. */
    MPIDI_CH3_PKT_RMA_GET_TARGET_DATATYPE(rma_op->pkt, target_datatype, mpi_errno);
    if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
        MPID_Datatype_get_ptr(target_datatype, target_dtp);

        /* Set dataloop size in pkt header */
        MPIDI_CH3_PKT_RMA_SET_DATALOOP_SIZE(rma_op->pkt, target_dtp->dataloop_size, mpi_errno);
    }

    if (is_empty_origin == FALSE) {
        /* Judge if origin datatype is derived datatype. */
        if (!MPIR_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype)) {
            MPID_Datatype_get_ptr(rma_op->origin_datatype, origin_dtp);
        }

        /* check if origin data is contiguous and get true lb */
        MPID_Datatype_is_contig(rma_op->origin_datatype, &is_origin_contig);
        MPID_Datatype_get_true_lb(rma_op->origin_datatype, &dt_true_lb);
    }
    else {
        /* origin buffer is empty, mark origin data as contig and true_lb as 0. */
        is_origin_contig = 1;
        dt_true_lb = 0;
    }

    iov[iovcnt].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) & (rma_op->pkt);
    iov[iovcnt].MPL_IOV_LEN = sizeof(rma_op->pkt);
    iovcnt++;

    MPIDI_CH3_PKT_RMA_GET_FLAGS(rma_op->pkt, flags, mpi_errno);
    if (!(flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) && target_dtp == NULL && is_origin_contig) {
        /* Fast path --- use iStartMsgv() to issue the data, which does not need a request
         * to be passed in:
         * (1) non-streamed op (do not need to send extended packet header);
         * (2) target datatype is predefined (do not need to send derived datatype info);
         * (3) origin datatype is contiguous (do not need to pack the data and send);
         */

        if (is_empty_origin == FALSE) {
            iov[iovcnt].MPL_IOV_BUF =
                (MPL_IOV_BUF_CAST) ((char *) rma_op->origin_addr + dt_true_lb + stream_offset);
            iov[iovcnt].MPL_IOV_LEN = stream_size;
            iovcnt++;
        }

#if defined(CHANNEL_MRAIL)
        Calculate_IOV_len(iov, iovcnt, total_length);

        if (total_length > vc->eager_max_msg_sz)
        {
            MPIDI_CH3_Pkt_t pkt_rndv;
            MPIDI_msg_sz_t copy_size, control_iov_count;

            control_iov_count = 1;

            if ((rma_op->pkt).type == MPIDI_CH3_PKT_PUT) {
                /* Put */
                copy_size = sizeof(MPIDI_CH3_Pkt_put_rndv_t);
                memcpy((void *) &pkt_rndv, (void *)(&(rma_op->pkt.put)), copy_size);
                pkt_rndv.type = MPIDI_CH3_PKT_PUT_RNDV;
                ((MPIDI_CH3_Pkt_put_rndv_t *) & pkt_rndv)->data_sz = stream_size;
            } else if((rma_op->pkt).type == MPIDI_CH3_PKT_ACCUMULATE) {
                /*Accumulate*/
                copy_size = sizeof(MPIDI_CH3_Pkt_accum_rndv_t);
                memcpy((void *) &pkt_rndv, (void *)(&(rma_op->pkt.accum)), copy_size);
                pkt_rndv.type = MPIDI_CH3_PKT_ACCUMULATE_RNDV;
                ((MPIDI_CH3_Pkt_accum_rndv_t *) & pkt_rndv)->data_sz = stream_size;
            } else {
                /*Get_acc*/
                copy_size = sizeof(MPIDI_CH3_Pkt_get_accum_rndv_t);
                memcpy((void *) &pkt_rndv, (void *)(&(rma_op->pkt.get_accum)), copy_size);
                pkt_rndv.type = MPIDI_CH3_PKT_GET_ACCUM_RNDV;
                ((MPIDI_CH3_Pkt_get_accum_rndv_t *) & pkt_rndv)->data_sz = stream_size;
            }

            req = MPID_Request_create();
            MPIR_ERR_CHKANDJUMP(req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

            MPIU_Object_set_ref(req, 2);
            req->kind = MPID_REQUEST_SEND;
            MV2_INC_NUM_POSTED_SEND();
            req->dev.iov_count = iovcnt;

            int i = 0;

            for (; i < iovcnt; ++i)
            {
                req->dev.iov[i].MPL_IOV_BUF = iov[i].MPL_IOV_BUF;
                req->dev.iov[i].MPL_IOV_LEN = iov[i].MPL_IOV_LEN;
            }

            req->dev.iov[0].MPL_IOV_BUF = (void*) &pkt_rndv;
            req->dev.iov[0].MPL_IOV_LEN = copy_size;
            req->dev.OnFinal = 0;
            req->dev.OnDataAvail = 0;

            MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
            mpi_errno = MPIDI_CH3_iStartRmaRndv(vc, req, control_iov_count, 
                                                stream_offset, stream_size);
            MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
            MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

            /* When R3 is used, the target process will send a get_accum_resp packet
             * after data has been transferred. So, this request needs to be used  
             * twice. */
            if ((pkt_rndv.type == MPIDI_CH3_PKT_GET_ACCUM_RNDV)
                 && (req->mrail.protocol == MV2_RNDV_PROTOCOL_R3 || 
                     req->mrail.protocol == MV2_RNDV_PROTOCOL_UD_ZCOPY)) {
                int tmp = 0;
                MPID_Request *sreq;
                MPIDI_CH3_Pkt_get_accum_t *gacc_pkt = &rma_op->pkt.get_accum;
                MPID_Request_get_ptr(gacc_pkt->request_handle, sreq);
                MPIDI_CH3U_Request_increment_cc(sreq, &tmp);
            }
        }
        else
#endif /* defined(CHANNEL_MRAIL) */
        {
            MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
            mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, &req);
            MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
            MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
#if defined(CHANNEL_PSM)
        /* For non-streamed op, it is always the last stream unit */
        req->last_stream_unit = 1;
#endif

        if (origin_dtp != NULL) {
            if (req == NULL) {
                MPID_Datatype_release(origin_dtp);
            }
            else {
                /* this will cause the datatype to be freed when the request
                 * is freed. */
                req->dev.datatype_ptr = origin_dtp;
            }
        }

        goto fn_exit;
    }

    /* Normal path: use iSendv() and sendNoncontig_fn() to issue the data, which
     * always need a request to be passed in. */

    /* create a new request */
    req = MPID_Request_create();
    MPIR_ERR_CHKANDJUMP(req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    MPIU_Object_set_ref(req, 2);
    req->kind = MPID_REQUEST_SEND;
    MV2_INC_NUM_POSTED_SEND();

    /* set extended packet header, it is freed when the request is freed.  */
    if (ext_hdr_sz > 0) {
        req->dev.ext_hdr_sz = ext_hdr_sz;
        req->dev.ext_hdr_ptr = ext_hdr_ptr;
    }

    if (origin_dtp != NULL) {
        req->dev.datatype_ptr = origin_dtp;
        /* this will cause the datatype to be freed when the request
         * is freed. */
    }

    if (is_origin_contig) {
        /* origin data is contiguous */

        if (is_empty_origin == FALSE) {
            iov[iovcnt].MPL_IOV_BUF =
                (MPL_IOV_BUF_CAST) ((char *) rma_op->origin_addr + dt_true_lb + stream_offset);
            iov[iovcnt].MPL_IOV_LEN = stream_size;
            iovcnt++;
        }

#if defined(CHANNEL_PSM)
        if (req->dev.ext_hdr_sz > 0) {
            int i;               
            for (i = iovcnt-1; i >= 1; i--) {
                iov[i+1].MPL_IOV_BUF = iov[i].MPL_IOV_BUF;
                iov[i+1].MPL_IOV_LEN = iov[i].MPL_IOV_LEN;
            }
            iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) req->dev.ext_hdr_ptr;
            iov[1].MPL_IOV_LEN = req->dev.ext_hdr_sz;
            iovcnt++;
        }   
        MPI_Aint type_size;
        MPID_Datatype_get_size_macro(rma_op->origin_datatype, type_size);
        if ((type_size * rma_op->origin_count) == (stream_offset + stream_size)) {
            req->last_stream_unit = 1;
        }
        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, &req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);

        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        goto fn_exit;
#endif

#if defined(CHANNEL_MRAIL)
        Calculate_IOV_len(iov, iovcnt, total_length);

        if (total_length > vc->eager_max_msg_sz)
        {
            MPIDI_CH3_Pkt_t pkt_rndv;
            MPIDI_msg_sz_t copy_size, control_iov_count;

            control_iov_count = 1;

            if (ext_hdr_sz > 0) {
                int i;               
                for (i = iovcnt-1; i >= 1; i--) {
                    iov[i+1].MPL_IOV_BUF = iov[i].MPL_IOV_BUF;
                    iov[i+1].MPL_IOV_LEN = iov[i].MPL_IOV_LEN;
                }
                iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) req->dev.ext_hdr_ptr;
                iov[1].MPL_IOV_LEN = req->dev.ext_hdr_sz;
                iovcnt++;
                control_iov_count = 2;
                MPI_Aint type_size;
                MPID_Datatype_get_size_macro(rma_op->origin_datatype, type_size);
                if ((type_size * rma_op->origin_count) < (stream_offset + stream_size))
                    req->mrail.is_rma_last_stream_unit = 0;
            }

            if ((rma_op->pkt).type == MPIDI_CH3_PKT_PUT) {
                /* Put */
                copy_size = sizeof(MPIDI_CH3_Pkt_put_rndv_t);
                memcpy((void *) &pkt_rndv, (void *)(&(rma_op->pkt.put)), copy_size);
                pkt_rndv.type = MPIDI_CH3_PKT_PUT_RNDV;
                ((MPIDI_CH3_Pkt_put_rndv_t *) & pkt_rndv)->data_sz = stream_size;
            } else if((rma_op->pkt).type == MPIDI_CH3_PKT_ACCUMULATE) {
                /*Accumulate*/
                copy_size = sizeof(MPIDI_CH3_Pkt_accum_rndv_t);
                memcpy((void *) &pkt_rndv, (void *)(&(rma_op->pkt.accum)), copy_size);
                pkt_rndv.type = MPIDI_CH3_PKT_ACCUMULATE_RNDV;
                ((MPIDI_CH3_Pkt_accum_rndv_t *) & pkt_rndv)->data_sz = stream_size;
            } else {
                /*Get_acc*/
                copy_size = sizeof(MPIDI_CH3_Pkt_get_accum_rndv_t);
                memcpy((void *) &pkt_rndv, (void *)(&(rma_op->pkt.get_accum)), copy_size);
                pkt_rndv.type = MPIDI_CH3_PKT_GET_ACCUM_RNDV;
                ((MPIDI_CH3_Pkt_get_accum_rndv_t *) & pkt_rndv)->data_sz = stream_size;
            }

            int i = 0;

            for (; i < iovcnt; ++i)
            {
                req->dev.iov[i].MPL_IOV_BUF = iov[i].MPL_IOV_BUF;
                req->dev.iov[i].MPL_IOV_LEN = iov[i].MPL_IOV_LEN;
            }

            req->dev.iov[0].MPL_IOV_BUF = (void*) &pkt_rndv;
            req->dev.iov[0].MPL_IOV_LEN = copy_size;
            req->dev.OnFinal = 0;
            req->dev.OnDataAvail = 0;
            req->dev.iov_count = iovcnt;

            MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
            mpi_errno = MPIDI_CH3_iStartRmaRndv(vc, req, control_iov_count,
                                                stream_offset, stream_size);
            MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
            MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

            /* When R3 is used, the target process will send a get_accum_resp packet
             * after data has been transferred. So, this request needs to be used  
             * twice. */
            if ((pkt_rndv.type == MPIDI_CH3_PKT_GET_ACCUM_RNDV)
                 && (req->mrail.protocol == MV2_RNDV_PROTOCOL_R3 ||
                     req->mrail.protocol == MV2_RNDV_PROTOCOL_UD_ZCOPY)) {
                int tmp = 0;
                MPID_Request *sreq;
                MPIDI_CH3_Pkt_get_accum_t *gacc_pkt = &rma_op->pkt.get_accum;
                MPID_Request_get_ptr(gacc_pkt->request_handle, sreq);
                MPIDI_CH3U_Request_increment_cc(sreq, &tmp);
            }
        }
        else
#endif /* defined(CHANNEL_MRAIL) */
        {
            MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
            mpi_errno = MPIDI_CH3_iSendv(vc, req, iov, iovcnt);
            MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        }

        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }
    else {
        /* origin data is non-contiguous */
#if defined (CHANNEL_PSM)
        req->psm_flags |= PSM_1SIDED_NON_CONTIG_REQ;
        mpi_errno=  psm_do_pack(rma_op->origin_count, rma_op->origin_datatype,
                rma_op->comm_ptr, req, rma_op->origin_addr, stream_offset,
                stream_offset + stream_size, PACK_RMA_STREAM);
        if(mpi_errno) MPIR_ERR_POP(mpi_errno);

        iov[iovcnt].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)(req->dev.ext_hdr_ptr);
        iov[iovcnt].MPL_IOV_LEN = req->dev.ext_hdr_sz;
        iovcnt++;
        iov[iovcnt].MPL_IOV_BUF = req->pkbuf;
        iov[iovcnt].MPL_IOV_LEN = req->pksz;
        iovcnt++;

        MPI_Aint type_size;
        MPID_Datatype_get_size_macro(rma_op->origin_datatype, type_size);
        if ((type_size * rma_op->origin_count) == (stream_offset + stream_size)) {
            req->last_stream_unit = 1;
        }

        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, &req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);

        if(mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }

        goto fn_exit;
#endif



        req->dev.segment_ptr = MPID_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1(req->dev.segment_ptr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");

        MPID_Segment_init(rma_op->origin_addr, rma_op->origin_count,
                          rma_op->origin_datatype, req->dev.segment_ptr, 0);
        req->dev.segment_first = stream_offset;
        req->dev.segment_size = stream_offset + stream_size;

        req->dev.OnFinal = 0;
        req->dev.OnDataAvail = 0;

#if defined(CHANNEL_MRAIL)
        int iov_n;

        iov_n = MPL_IOV_LIMIT - iovcnt;
        if (ext_hdr_sz > 0) iov_n--;
        mpi_errno = MPIDI_CH3U_Request_load_send_iov(req, &iov[iovcnt], &iov_n);   

        iov_n += iovcnt;

        Calculate_IOV_len(iov, iov_n, total_length);

        if (total_length > vc->eager_max_msg_sz)
        {
            if (ext_hdr_sz > 0) {
                /* insert the ext hdr to iov when needed */
                int i;
                for (i = iov_n-1; i >= 1; i--) {
                    iov[i+1].MPL_IOV_BUF = iov[i].MPL_IOV_BUF;
                    iov[i+1].MPL_IOV_LEN = iov[i].MPL_IOV_LEN;
                }
                iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) req->dev.ext_hdr_ptr;
                iov[1].MPL_IOV_LEN = req->dev.ext_hdr_sz;
                iovcnt++;
                iov_n++;
                MPI_Aint type_size;
                MPID_Datatype_get_size_macro(rma_op->origin_datatype, type_size);
                if ((type_size * rma_op->origin_count) < (stream_offset + stream_size))
                    req->mrail.is_rma_last_stream_unit = 0;
            }

            MPIDI_CH3_Pkt_t pkt_rndv;
            MPIDI_msg_sz_t copy_size;

            if ((rma_op->pkt).type == MPIDI_CH3_PKT_PUT) {
                /* Put */
                copy_size = sizeof(MPIDI_CH3_Pkt_put_rndv_t);
                memcpy((void *) &pkt_rndv, (void *)(&(rma_op->pkt.put)), copy_size);
                pkt_rndv.type = MPIDI_CH3_PKT_PUT_RNDV;
                ((MPIDI_CH3_Pkt_put_rndv_t *) & pkt_rndv)->data_sz = stream_size;

            } else if((rma_op->pkt).type == MPIDI_CH3_PKT_ACCUMULATE) {
                /*Accumulate*/
                copy_size = sizeof(MPIDI_CH3_Pkt_accum_rndv_t);
                memcpy((void *) &pkt_rndv, (void *)(&(rma_op->pkt.accum)), copy_size);
                pkt_rndv.type = MPIDI_CH3_PKT_ACCUMULATE_RNDV;
                ((MPIDI_CH3_Pkt_accum_rndv_t *) & pkt_rndv)->data_sz = stream_size;

            } else {
                /*Get_acc*/
                copy_size = sizeof(MPIDI_CH3_Pkt_get_accum_rndv_t);
                memcpy((void *) &pkt_rndv, (void *)(&(rma_op->pkt.get_accum)), copy_size);
                MPIDI_CH3_Pkt_get_accum_rndv_t *get_acc_rndv = (MPIDI_CH3_Pkt_get_accum_rndv_t *) &pkt_rndv;
                get_acc_rndv->type = MPIDI_CH3_PKT_GET_ACCUM_RNDV;
                get_acc_rndv->data_sz = stream_size;

            }

            int i = 0;

            for (; i < iov_n; ++i)
            {
                req->dev.iov[i].MPL_IOV_BUF = iov[i].MPL_IOV_BUF;
                req->dev.iov[i].MPL_IOV_LEN = iov[i].MPL_IOV_LEN;
            }
            req->dev.iov_count = iov_n;
            req->dev.iov[0].MPL_IOV_BUF = (void *) &pkt_rndv;
            req->dev.iov[0].MPL_IOV_LEN = copy_size;

            MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
            mpi_errno = MPIDI_CH3_iStartRmaRndv(vc, req, iovcnt,
                                                stream_offset, stream_size);
            MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
            MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

            /* When R3 is used, the target process will send a get_accum_resp packet
             * after data has been transferred. So, this request needs to be used  
             * twice. */
            if ((pkt_rndv.type == MPIDI_CH3_PKT_GET_ACCUM_RNDV)
                && (req->mrail.protocol == MV2_RNDV_PROTOCOL_R3)) {
                int tmp = 0;
                MPID_Request *sreq;
                MPIDI_CH3_Pkt_get_accum_t *gacc_pkt = &rma_op->pkt.get_accum;
                MPID_Request_get_ptr(gacc_pkt->request_handle, sreq);
                MPIDI_CH3U_Request_increment_cc(sreq, &tmp);
            }

        } else {
            MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
            mpi_errno = MPIDI_CH3_iSendv(vc, req, iov, iov_n);      
            MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
            MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }

        goto fn_exit;
#endif

        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = vc->sendNoncontig_fn(vc, req, iov[0].MPL_IOV_BUF, iov[0].MPL_IOV_LEN);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

  fn_exit:
#if defined(CHANNEL_PSM)
    /* For MPI_REPLACE operation, in order to ensure the order,
     * we make sure the RNDV data has been sent before progressing further. */
    if (is_replace_op == TRUE && req->psm_flags & PSM_RNDVSEND_REQ
            && req->last_stream_unit == 1) {
        while (!(req->psm_flags & PSM_COMPQ_PENDING))
            MPID_Progress_poke();
    }
#endif
    /* release the target datatype */
    if (target_dtp)
        MPID_Datatype_release(target_dtp);
    (*req_ptr) = req;

    MPIDI_FUNC_EXIT(MPID_STATE_ISSUE_FROM_ORIGIN_BUFFER);
    return mpi_errno;
  fn_fail:
    if (req) {
        if (req->dev.datatype_ptr)
            MPID_Datatype_release(req->dev.datatype_ptr);
        if (req->dev.ext_hdr_ptr)
            MPIU_Free(req->dev.ext_hdr_ptr);
        MPID_Request_release(req);
    }

    (*req_ptr) = NULL;
    goto fn_exit;
}


/* issue_put_op() issues PUT packet header and data. */
#undef FUNCNAME
#define FUNCNAME issue_put_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int issue_put_op(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr,
                        MPIDI_RMA_Target_t * target_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_VC_t *vc = NULL;
    MPID_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_put_t *put_pkt = &rma_op->pkt.put;
    MPID_Request *curr_req = NULL;
    MPI_Datatype target_datatype;
    MPID_Datatype *target_dtp_ptr = NULL;
    MPIDI_CH3_Ext_pkt_put_derived_t *ext_hdr_ptr = NULL;
    MPI_Aint ext_hdr_sz = 0;
    int mpi_errno = MPI_SUCCESS;
#if defined(CHANNEL_MRAIL) && defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_PUT_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ISSUE_PUT_OP);

    put_pkt->flags |= flags;

    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

#if defined(CHANNEL_MRAIL)
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(put_pkt, seqnum);
#endif

#if defined (CHANNEL_PSM) /* psm uses 2-sided, fill up rank */
    rma_op->comm_ptr = win_ptr->comm_ptr;
    put_pkt->target_rank = rma_op->target_rank;
    put_pkt->source_rank = win_ptr->comm_ptr->rank;
    put_pkt->mapped_trank = win_ptr->rank_mapping[rma_op->target_rank];
    put_pkt->mapped_srank = win_ptr->rank_mapping[win_ptr->comm_ptr->rank];
#endif

    if (rma_op->pkt.type == MPIDI_CH3_PKT_PUT_IMMED) {
        /* All origin data is in packet header, issue the header. */
        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, put_pkt, sizeof(*put_pkt), &curr_req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }
    else {
        MPI_Aint origin_type_size;
        MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);

        /* If derived datatype on target, add extended packet header. */
        MPIDI_CH3_PKT_RMA_GET_TARGET_DATATYPE(rma_op->pkt, target_datatype, mpi_errno);
        if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
            MPID_Datatype_get_ptr(target_datatype, target_dtp_ptr);

            void *dataloop_ptr = NULL;

            /* dataloop is behind of extended header on origin.
             * TODO: support extended header array */
            ext_hdr_sz = sizeof(MPIDI_CH3_Ext_pkt_put_derived_t) + target_dtp_ptr->dataloop_size;
            ext_hdr_ptr = MPIU_Malloc(ext_hdr_sz);
            MPL_VG_MEM_INIT(ext_hdr_ptr, ext_hdr_sz);
            if (!ext_hdr_ptr) {
                MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                     "**nomem %s", "MPIDI_CH3_Ext_pkt_put_derived_t");
            }

            dataloop_ptr = (void *) ((char *) ext_hdr_ptr +
                                     sizeof(MPIDI_CH3_Ext_pkt_put_derived_t));
            fill_in_derived_dtp_info(&ext_hdr_ptr->dtype_info, dataloop_ptr, target_dtp_ptr);
        }

        mpi_errno = issue_from_origin_buffer(rma_op, vc, ext_hdr_ptr, ext_hdr_sz,
                                             0, rma_op->origin_count * origin_type_size, &curr_req);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

    if (curr_req != NULL) {
        rma_op->reqs_size = 1;

        rma_op->single_req = curr_req;
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ISSUE_PUT_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    rma_op->single_req = NULL;
    rma_op->reqs_size = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#define ALL_STREAM_UNITS_ISSUED (-1)

/* issue_acc_op() send ACC packet header and data. */
#undef FUNCNAME
#define FUNCNAME issue_acc_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int issue_acc_op(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr,
                        MPIDI_RMA_Target_t * target_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_VC_t *vc = NULL;
    MPID_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_accum_t *accum_pkt = &rma_op->pkt.accum;
    int i, j;
    MPI_Aint stream_elem_count, stream_unit_count;
    MPI_Aint predefined_dtp_size, predefined_dtp_extent, predefined_dtp_count;
    MPI_Aint total_len, rest_len;
    MPI_Aint origin_dtp_size;
    MPID_Datatype *origin_dtp_ptr = NULL;
    MPID_Datatype *target_dtp_ptr = NULL;
    void *ext_hdr_ptr = NULL;
    MPI_Aint ext_hdr_sz = 0;
    int mpi_errno = MPI_SUCCESS;
#if defined(CHANNEL_MRAIL)
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif /* defined(MPID_USE_SEQUENCE_NUMBERS) */
#endif /* defined(CHANNEL_MRAIL) */

    MPIDI_STATE_DECL(MPID_STATE_ISSUE_ACC_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ISSUE_ACC_OP);

    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

#if defined(CHANNEL_MRAIL)
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(accum_pkt, seqnum);
#endif /* defined(CHANNEL_MRAIL) */


#if defined (CHANNEL_PSM)
    accum_pkt->target_rank = rma_op->target_rank;
    accum_pkt->source_rank = win_ptr->comm_ptr->rank;
    accum_pkt->mapped_trank = win_ptr->rank_mapping[rma_op->target_rank];
    accum_pkt->mapped_srank = win_ptr->rank_mapping[win_ptr->comm_ptr->rank];
#endif /* CHANNEL_PSM */

    if (rma_op->pkt.type == MPIDI_CH3_PKT_ACCUMULATE_IMMED) {
        MPID_Request *curr_req = NULL;

        accum_pkt->flags |= flags;

        /* All origin data is in packet header, issue the header. */
        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, accum_pkt, sizeof(*accum_pkt), &curr_req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

        if (curr_req != NULL) {
            MPIU_Assert(rma_op->reqs_size == 0 && rma_op->single_req == NULL);

            rma_op->reqs_size = 1;
            rma_op->single_req = curr_req;
        }
        goto fn_exit;
    }

    /* Get total length of origin data */
    MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_dtp_size);
    total_len = origin_dtp_size * rma_op->origin_count;

    /* Get size and count for predefined datatype elements */
    if (MPIR_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype)) {
        predefined_dtp_size = origin_dtp_size;
        predefined_dtp_count = rma_op->origin_count;
        MPID_Datatype_get_extent_macro(rma_op->origin_datatype, predefined_dtp_extent);
    }
    else {
        MPID_Datatype_get_ptr(rma_op->origin_datatype, origin_dtp_ptr);
        MPIU_Assert(origin_dtp_ptr != NULL && origin_dtp_ptr->basic_type != MPI_DATATYPE_NULL);
        MPID_Datatype_get_size_macro(origin_dtp_ptr->basic_type, predefined_dtp_size);
        predefined_dtp_count = total_len / predefined_dtp_size;
        MPID_Datatype_get_extent_macro(origin_dtp_ptr->basic_type, predefined_dtp_extent);
    }
    MPIU_Assert(predefined_dtp_count > 0 && predefined_dtp_size > 0 && predefined_dtp_extent > 0);

    /* Calculate number of predefined elements in each stream unit, and
     * total number of stream units. */
    stream_elem_count = MPIDI_CH3U_Acc_stream_size / predefined_dtp_extent;
    stream_unit_count = (predefined_dtp_count - 1) / stream_elem_count + 1;
    MPIU_Assert(stream_elem_count > 0 && stream_unit_count > 0);

    /* If there are more than one stream unit, mark the current packet
     * as stream packet */
    if (stream_unit_count > 1)
        flags |= MPIDI_CH3_PKT_FLAG_RMA_STREAM;

    /* Get target datatype */
    if (!MPIR_DATATYPE_IS_PREDEFINED(accum_pkt->datatype))
        MPID_Datatype_get_ptr(accum_pkt->datatype, target_dtp_ptr);

    rest_len = total_len;
    MPIU_Assert(rma_op->issued_stream_count >= 0);
    for (j = 0; j < stream_unit_count; j++) {
        MPIDI_msg_sz_t stream_offset, stream_size;
        MPID_Request *curr_req = NULL;

        if (j < rma_op->issued_stream_count)
            continue;

        accum_pkt->flags |= flags;

        if (j != 0) {
            accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED;
            accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE;
        }
        if (j != stream_unit_count - 1) {
            accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_UNLOCK;
            accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
            accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER;
        }

        stream_offset = j * stream_elem_count * predefined_dtp_size;
        stream_size = MPIR_MIN(stream_elem_count * predefined_dtp_size, rest_len);
        rest_len -= stream_size;

        /* Set extended packet header if needed. */
        init_accum_ext_pkt(flags, target_dtp_ptr, stream_offset, &ext_hdr_ptr, &ext_hdr_sz);

        mpi_errno = issue_from_origin_buffer(rma_op, vc, ext_hdr_ptr, ext_hdr_sz,
                                             stream_offset, stream_size, &curr_req);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        if (curr_req != NULL) {
            if (rma_op->reqs_size == 0) {
                MPIU_Assert(rma_op->single_req == NULL && rma_op->multi_reqs == NULL);
                rma_op->reqs_size = stream_unit_count;

                if (stream_unit_count > 1) {
                    rma_op->multi_reqs =
                        (MPID_Request **) MPIU_Malloc(sizeof(MPID_Request *) * rma_op->reqs_size);
                    for (i = 0; i < rma_op->reqs_size; i++)
                        rma_op->multi_reqs[i] = NULL;
                }
            }

            if (rma_op->reqs_size == 1)
                rma_op->single_req = curr_req;
            else
                rma_op->multi_reqs[j] = curr_req;
        }

        rma_op->issued_stream_count++;

        if (accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
            accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE) {
            /* if piggybacked with LOCK flag, we
             * only issue the first streaming unit */
            MPIU_Assert(j == 0);
            break;
        }
    }   /* end of for loop */

    if (rma_op->issued_stream_count == stream_unit_count) {
        /* Mark that all stream units have been issued */
        rma_op->issued_stream_count = ALL_STREAM_UNITS_ISSUED;
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ISSUE_ACC_OP);
    return mpi_errno;
  fn_fail:
    if (rma_op->reqs_size == 1) {
        rma_op->single_req = NULL;
    }
    else if (rma_op->reqs_size > 1) {
        if (rma_op->multi_reqs != NULL) {
            MPIU_Free(rma_op->multi_reqs);
            rma_op->multi_reqs = NULL;
        }
    }
    rma_op->reqs_size = 0;
    goto fn_exit;
}


/* issue_get_acc_op() send GACC packet header and data. */
#undef FUNCNAME
#define FUNCNAME issue_get_acc_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int issue_get_acc_op(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr,
                            MPIDI_RMA_Target_t * target_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_VC_t *vc = NULL;
    MPID_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_get_accum_t *get_accum_pkt = &rma_op->pkt.get_accum;
    int i, j;
    MPI_Aint stream_elem_count, stream_unit_count;
    MPI_Aint predefined_dtp_size, predefined_dtp_count, predefined_dtp_extent;
    MPI_Aint total_len, rest_len;
    MPI_Aint target_dtp_size;
    MPID_Datatype *target_dtp_ptr = NULL;
    void *ext_hdr_ptr = NULL;
    MPI_Aint ext_hdr_sz = 0;
    int mpi_errno = MPI_SUCCESS;
#if defined(CHANNEL_MRAIL)
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif /* defined(MPID_USE_SEQUENCE_NUMBERS) */
#endif /* defined(CHANNEL_MRAIL) */
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_GET_ACC_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ISSUE_GET_ACC_OP);

    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

#if defined(CHANNEL_MRAIL)
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(get_accum_pkt, seqnum);
#endif /* defined(CHANNEL_MRAIL) */

#if defined (CHANNEL_PSM)
    get_accum_pkt->target_rank = rma_op->target_rank;
    get_accum_pkt->source_rank = win_ptr->comm_ptr->rank;
    get_accum_pkt->mapped_trank = win_ptr->rank_mapping[rma_op->target_rank];
    get_accum_pkt->mapped_srank = win_ptr->rank_mapping[win_ptr->comm_ptr->rank];
    get_accum_pkt->target_win_handle = win_ptr->basic_info_table[rma_op->target_rank].win_handle;
    get_accum_pkt->source_win_handle = win_ptr->handle;
    get_accum_pkt->target_ptr = target_ptr;
#endif /* CHANNEL_PSM */

    if (rma_op->pkt.type == MPIDI_CH3_PKT_GET_ACCUM_IMMED) {
        MPID_Request *resp_req = NULL;
        MPID_Request *curr_req = NULL;

        get_accum_pkt->flags |= flags;

        rma_op->reqs_size = 1;

        /* Create a request for the GACC response.  Store the response buf, count, and
         * datatype in it, and pass the request's handle in the GACC packet. When the
         * response comes from the target, it will contain the request handle. */
        resp_req = MPID_Request_create();
        MPIR_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

        MPIU_Object_set_ref(resp_req, 2);

        resp_req->dev.user_buf = rma_op->result_addr;
        resp_req->dev.user_count = rma_op->result_count;
        resp_req->dev.datatype = rma_op->result_datatype;
        resp_req->dev.target_win_handle = MPI_WIN_NULL;
        resp_req->dev.source_win_handle = win_ptr->handle;

        /* Note: Get_accumulate uses the same packet type as accumulate */
        get_accum_pkt->request_handle = resp_req->handle;

        /* All origin data is in packet header, issue the header. */
        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, get_accum_pkt, sizeof(*get_accum_pkt), &curr_req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

        if (curr_req != NULL) {
            MPID_Request_release(curr_req);
        }

        rma_op->single_req = resp_req;

        goto fn_exit;
    }

    /* Get total length of target data */
    MPID_Datatype_get_size_macro(get_accum_pkt->datatype, target_dtp_size);
    total_len = target_dtp_size * get_accum_pkt->count;

    /* Get size and count for predefined datatype elements */
    if (MPIR_DATATYPE_IS_PREDEFINED(get_accum_pkt->datatype)) {
        predefined_dtp_size = target_dtp_size;
        predefined_dtp_count = get_accum_pkt->count;
        MPID_Datatype_get_extent_macro(get_accum_pkt->datatype, predefined_dtp_extent);
    }
    else {
        MPID_Datatype_get_ptr(get_accum_pkt->datatype, target_dtp_ptr);
        MPIU_Assert(target_dtp_ptr != NULL && target_dtp_ptr->basic_type != MPI_DATATYPE_NULL);
        MPID_Datatype_get_size_macro(target_dtp_ptr->basic_type, predefined_dtp_size);
        predefined_dtp_count = total_len / predefined_dtp_size;
        MPID_Datatype_get_extent_macro(target_dtp_ptr->basic_type, predefined_dtp_extent);
    }
    MPIU_Assert(predefined_dtp_count > 0 && predefined_dtp_size > 0 && predefined_dtp_extent > 0);

    /* Calculate number of predefined elements in each stream unit, and
     * total number of stream units. */
    stream_elem_count = MPIDI_CH3U_Acc_stream_size / predefined_dtp_extent;
    stream_unit_count = (predefined_dtp_count - 1) / stream_elem_count + 1;
    MPIU_Assert(stream_elem_count > 0 && stream_unit_count > 0);

    /* If there are more than one stream unit, mark the current packet
     * as stream packet */
    if (stream_unit_count > 1)
        flags |= MPIDI_CH3_PKT_FLAG_RMA_STREAM;

    rest_len = total_len;

    rma_op->reqs_size = stream_unit_count;

    if (rma_op->reqs_size > 1) {
        rma_op->multi_reqs =
            (MPID_Request **) MPIU_Malloc(sizeof(MPID_Request *) * rma_op->reqs_size);
        for (i = 0; i < rma_op->reqs_size; i++)
            rma_op->multi_reqs[i] = NULL;
    }

    MPIU_Assert(rma_op->issued_stream_count >= 0);

    for (j = 0; j < stream_unit_count; j++) {
        MPIDI_msg_sz_t stream_offset, stream_size;
        MPID_Request *resp_req = NULL;
        MPID_Request *curr_req = NULL;

        if (j < rma_op->issued_stream_count)
            continue;

        get_accum_pkt->flags |= flags;

        if (j != 0) {
            get_accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED;
            get_accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE;
        }
        if (j != stream_unit_count - 1) {
            get_accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_UNLOCK;
            get_accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
            get_accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER;
        }

        /* Create a request for the GACC response.  Store the response buf, count, and
         * datatype in it, and pass the request's handle in the GACC packet. When the
         * response comes from the target, it will contain the request handle. */
        resp_req = MPID_Request_create();
        MPIR_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

        MPIU_Object_set_ref(resp_req, 2);

        resp_req->dev.user_buf = rma_op->result_addr;
        resp_req->dev.user_count = rma_op->result_count;
        resp_req->dev.datatype = rma_op->result_datatype;
        resp_req->dev.target_win_handle = MPI_WIN_NULL;
        resp_req->dev.source_win_handle = win_ptr->handle;
        resp_req->dev.flags = flags;

        if (!MPIR_DATATYPE_IS_PREDEFINED(resp_req->dev.datatype)) {
            MPID_Datatype *result_dtp = NULL;
            MPID_Datatype_get_ptr(resp_req->dev.datatype, result_dtp);
            resp_req->dev.datatype_ptr = result_dtp;
            /* this will cause the datatype to be freed when the
             * request is freed. */
        }

        /* Note: Get_accumulate uses the same packet type as accumulate */
        get_accum_pkt->request_handle = resp_req->handle;

        stream_offset = j * stream_elem_count * predefined_dtp_size;
        stream_size = MPIR_MIN(stream_elem_count * predefined_dtp_size, rest_len);
        rest_len -= stream_size;

        /* Set extended packet header if needed. */
        init_get_accum_ext_pkt(flags, target_dtp_ptr, stream_offset, &ext_hdr_ptr, &ext_hdr_sz);

        /* Note: here we need to allocate an extended packet header in response request,
         * in order to store the stream_offset locally and use it in PktHandler_Get_AccumResp.
         * This extended packet header only contains stream_offset and does not contain any
         * other information. */
        init_get_accum_ext_pkt(flags, NULL /* target_dtp_ptr */ , stream_offset,
                               &(resp_req->dev.ext_hdr_ptr), &(resp_req->dev.ext_hdr_sz));

        mpi_errno = issue_from_origin_buffer(rma_op, vc, ext_hdr_ptr, ext_hdr_sz,
                                             stream_offset, stream_size, &curr_req);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        if (curr_req != NULL) {
            MPID_Request_release(curr_req);
        }

        if (rma_op->reqs_size == 1)
            rma_op->single_req = resp_req;
        else
            rma_op->multi_reqs[j] = resp_req;

        rma_op->issued_stream_count++;

        if (get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
            get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE) {
            /* if piggybacked with LOCK flag, we
             * only issue the first streaming unit */
            MPIU_Assert(j == 0);
            break;
        }
    }   /* end of for loop */

    if (rma_op->issued_stream_count == stream_unit_count) {
        /* Mark that all stream units have been issued */
        rma_op->issued_stream_count = ALL_STREAM_UNITS_ISSUED;
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ISSUE_GET_ACC_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (rma_op->reqs_size == 1) {
        /* error case: drop both our reference to the request and the
         * progress engine's reference to it, since the progress
         * engine didn't get a chance to see it yet. */
        MPID_Request_release(rma_op->single_req);
        MPID_Request_release(rma_op->single_req);
        rma_op->single_req = NULL;
    }
    else if (rma_op->reqs_size > 1) {
        for (i = 0; i < rma_op->reqs_size; i++) {
            if (rma_op->multi_reqs[i] != NULL) {
                /* error case: drop both our reference to the request
                 * and the progress engine's reference to it, since
                 * the progress engine didn't get a chance to see it
                 * yet. */
                MPID_Request_release(rma_op->multi_reqs[i]);
                MPID_Request_release(rma_op->multi_reqs[i]);
            }
        }
        MPIU_Free(rma_op->multi_reqs);
        rma_op->multi_reqs = NULL;
    }
    rma_op->reqs_size = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME issue_get_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int issue_get_op(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr,
                        MPIDI_RMA_Target_t * target_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_CH3_Pkt_get_t *get_pkt = &rma_op->pkt.get;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    MPID_Comm *comm_ptr;
    MPID_Datatype *dtp;
    MPI_Datatype target_datatype;
    MPID_Request *curr_req = NULL;
    MPID_Request *req = NULL;
    MPIDI_CH3_Ext_pkt_get_derived_t *ext_hdr_ptr = NULL;
    MPI_Aint ext_hdr_sz = 0;
    MPL_IOV iov[MPL_IOV_LIMIT] = {0};
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_GET_OP);
#if defined(CHANNEL_MRAIL)
    MPI_Aint type_size, origin_type_size;
    MPIDI_msg_sz_t total_size;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif /* defined(MPID_USE_SEQUENCE_NUMBERS) */
#endif /* defined(CHANNEL_MRAIL) */
#if defined(CHANNEL_PSM)
    int origin_type_size; 
    MPIDI_msg_sz_t total_size;
#endif 


    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ISSUE_GET_OP);

    rma_op->reqs_size = 1;

    /* create a request, store the origin buf, cnt, datatype in it,
     * and pass a handle to it in the get packet. When the get
     * response comes from the target, it will contain the request
     * handle. */
    curr_req = MPID_Request_create();
    if (curr_req == NULL) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    }

    MPIU_Object_set_ref(curr_req, 2);

    curr_req->dev.user_buf = rma_op->origin_addr;
    curr_req->dev.user_count = rma_op->origin_count;
    curr_req->dev.datatype = rma_op->origin_datatype;
    curr_req->dev.target_win_handle = MPI_WIN_NULL;
    curr_req->dev.source_win_handle = win_ptr->handle;

#if defined(CHANNEL_PSM)
    curr_req->dev.rma_target_ptr = target_ptr;
#endif
    if (!MPIR_DATATYPE_IS_PREDEFINED(curr_req->dev.datatype)) {
        MPID_Datatype_get_ptr(curr_req->dev.datatype, dtp);
        curr_req->dev.datatype_ptr = dtp;
        /* this will cause the datatype to be freed when the
         * request is freed. */
    }

#if defined(CHANNEL_MRAIL)
    MPID_Datatype_get_size_macro(curr_req->dev.datatype, type_size);
    curr_req->dev.recv_data_sz = (MPIDI_msg_sz_t) type_size * curr_req->dev.user_count;

    mpi_errno = MPIDI_CH3U_Post_data_receive_found(curr_req);
    if (mpi_errno != MPI_SUCCESS) {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
                    __LINE__, MPI_ERR_OTHER,
                    "**ch3|postrecv",
                    "**ch3|postrecv %s",
                    "MPIDI_CH3_PKT_GET_RESP");
    }
#endif /* defined(CHANNEL_MRAIL) */

    get_pkt->request_handle = curr_req->handle;

    get_pkt->flags |= flags;

#if defined (CHANNEL_PSM)
    get_pkt->source_rank = win_ptr->comm_ptr->rank;
    get_pkt->target_rank = rma_op->target_rank;
    get_pkt->mapped_srank = win_ptr->rank_mapping[win_ptr->comm_ptr->rank];
    get_pkt->mapped_trank = win_ptr->rank_mapping[rma_op->target_rank];
#endif /* CHANNEL_PSM */

    comm_ptr = win_ptr->comm_ptr;
    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    MPIDI_CH3_PKT_RMA_GET_TARGET_DATATYPE(rma_op->pkt, target_datatype, mpi_errno);
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
#if defined(CHANNEL_MRAIL)
        MPID_Datatype_get_size_macro(rma_op->origin_datatype,
                origin_type_size);
        total_size = (MPIDI_msg_sz_t) origin_type_size * rma_op->origin_count +
            sizeof(MPIDI_CH3_Pkt_get_resp_t);
        if (total_size <= vc->eager_max_msg_sz) {
            curr_req->mrail.protocol = MV2_RNDV_PROTOCOL_EAGER;
            MPIDI_VC_FAI_send_seqnum(vc, seqnum);
            MPIDI_Pkt_set_seqnum(get_pkt, seqnum);
#endif /* defined(CHANNEL_MRAIL) */

#if defined (CHANNEL_PSM)
        MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);
        total_size = (MPIDI_msg_sz_t) origin_type_size * rma_op->origin_count + 
            sizeof(MPIDI_CH3_Pkt_t);
    
        if(total_size < vc->eager_max_msg_sz) {
            get_pkt->rndv_mode = 0;
        } else {
            get_pkt->rndv_mode = 1;
            get_pkt->rndv_tag = psm_get_rndvtag();
            MPIDI_msg_sz_t target_tsz;
            MPID_Datatype_get_size_macro(target_datatype, target_tsz);
    
            if(MPIR_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype)) {
                get_pkt->rndv_len = target_tsz*get_pkt->count;
            } else {
                get_pkt->rndv_len = target_tsz*get_pkt->count;
                curr_req->dev.real_user_buf = curr_req->dev.user_buf;
                curr_req->dev.user_buf = MPIU_Malloc(get_pkt->rndv_len);
                curr_req->psm_flags |= PSM_RNDVRECV_GET_PACKED;
                //ODOT: needs free
            }
        }

#endif /* CHANNEL_PSM */

        /* basic datatype on target. simply send the get_pkt. */
        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
#if defined(CHANNEL_PSM)
        mpi_errno = MPIDI_CH3_iStartMsg(vc, get_pkt, sizeof(*get_pkt), &curr_req);
#else
        mpi_errno = MPIDI_CH3_iStartMsg(vc, get_pkt, sizeof(*get_pkt), &req);
#endif
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);

#if defined(CHANNEL_MRAIL)
        } else {
            MPIDI_CH3_Pkt_get_rndv_t get_rndv;
            MPIDI_Pkt_init(&get_rndv, MPIDI_CH3_PKT_GET_RNDV);
            memcpy((void *) &get_rndv, (void *) get_pkt,
                    sizeof(MPIDI_CH3_Pkt_get_t));
            /*The packed type should be set again because the memcpy would have 
              have overwritten it*/ 
            get_rndv.type = MPIDI_CH3_PKT_GET_RNDV;

            MPIDI_VC_FAI_send_seqnum(vc, seqnum);
            MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
            mpi_errno = MPIDI_CH3_iStartGetRndv(vc, &get_rndv, curr_req, NULL, 0);
            MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
            /*When R3 is used, the target process will send a get_resp packet
             * after data has been transferred. So, this request needs to be used  
             * twice. */
            if (curr_req->mrail.protocol == MV2_RNDV_PROTOCOL_R3) {
                int tmp = 0;
                MPIDI_CH3U_Request_increment_cc(curr_req, &tmp);
            }
        }
#endif /* defined(CHANNEL_MRAIL) */
    }
    else {
        /* derived datatype on target. */
        MPID_Datatype_get_ptr(target_datatype, dtp);
        void *dataloop_ptr = NULL;

        /* set extended packet header.
         * dataloop is behind of extended header on origin.
         * TODO: support extended header array */
        ext_hdr_sz = sizeof(MPIDI_CH3_Ext_pkt_get_derived_t) + dtp->dataloop_size;
        ext_hdr_ptr = MPIU_Malloc(ext_hdr_sz);
        MPL_VG_MEM_INIT(ext_hdr_ptr, ext_hdr_sz);
        if (!ext_hdr_ptr) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                 "**nomem %s", "MPIDI_CH3_Ext_pkt_get_derived_t");
        }

        dataloop_ptr = (void *) ((char *) ext_hdr_ptr + sizeof(MPIDI_CH3_Ext_pkt_get_derived_t));
        fill_in_derived_dtp_info(&ext_hdr_ptr->dtype_info, dataloop_ptr, dtp);

        /* Set dataloop size in pkt header */
        MPIDI_CH3_PKT_RMA_SET_DATALOOP_SIZE(rma_op->pkt, dtp->dataloop_size, mpi_errno);

        iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) get_pkt;
        iov[0].MPL_IOV_LEN = sizeof(*get_pkt);
        iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) ext_hdr_ptr;
        iov[1].MPL_IOV_LEN = ext_hdr_sz;

#if defined (CHANNEL_PSM)
        MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);
        total_size = (MPIDI_msg_sz_t) origin_type_size * rma_op->origin_count + 
            sizeof(MPIDI_CH3_Pkt_t);

        if(total_size < vc->eager_max_msg_sz) {
            get_pkt->rndv_mode = 0;
        } else {
            get_pkt->rndv_mode = 1;
            get_pkt->rndv_tag = psm_get_rndvtag();

            MPIDI_msg_sz_t origin_tsz, target_tsz;
            /* Calculate target size and setting rndv_len accordingly */
            MPID_Datatype_get_size_macro(target_datatype, target_tsz);
            get_pkt->rndv_len = target_tsz * get_pkt->count;

            /* origin is contiguous */
            if(MPIR_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype)) {
                MPID_Datatype_get_size_macro(rma_op->origin_datatype,
                        origin_tsz);
                MPIU_Assert((origin_tsz*rma_op->origin_count) ==
                            (target_tsz*get_pkt->count));
            } else { /* origin is non-contiguous */
                curr_req->dev.real_user_buf = curr_req->dev.user_buf;
                curr_req->dev.user_buf = MPIU_Malloc(get_pkt->rndv_len);
                curr_req->psm_flags |= PSM_RNDVRECV_GET_PACKED;
            }
        }
        req = curr_req;
#endif /* CHANNEL_PSM */

#if defined(CHANNEL_MRAIL)
        MPID_Datatype_get_size_macro(rma_op->origin_datatype,
                origin_type_size);
        total_size = (MPIDI_msg_sz_t) origin_type_size * rma_op->origin_count
            + sizeof(MPIDI_CH3_Pkt_get_resp_t);

        if (total_size <= vc->eager_max_msg_sz) {
            /* basic datatype on target. simply send the get_pkt. */
            curr_req->mrail.protocol = MV2_RNDV_PROTOCOL_EAGER;
            MPIDI_VC_FAI_send_seqnum(vc, seqnum);
            MPIDI_Pkt_set_seqnum(get_pkt, seqnum);
#endif /* defined(CHANNEL_MRAIL) */	

        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, 2, &req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);

#if defined(CHANNEL_MRAIL)
        } else {
            MPIDI_CH3_Pkt_get_rndv_t get_rndv;
            MPIDI_Pkt_init(&get_rndv, MPIDI_CH3_PKT_GET_RNDV);
            MPIU_Memcpy((void *) &get_rndv, (void *) get_pkt,
                    sizeof(MPIDI_CH3_Pkt_get_t));
            /*The packed type should be set again because the memcpy would have 
              have overwritten it*/
            get_rndv.type = MPIDI_CH3_PKT_GET_RNDV;
            curr_req->mrail.protocol = MV2_RNDV_PROTOCOL_RPUT;
            MPIDI_VC_FAI_send_seqnum(vc, seqnum);
            MPIDI_Pkt_set_seqnum(&get_rndv, seqnum);
            MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
            mpi_errno = MPIDI_CH3_iStartGetRndv(vc, &get_rndv, curr_req, &iov[1], 1);
            MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
            /*When R3 is used, the target process will send a get_resp packet
             * after data has been transferred. So, this request needs to be used  
             * twice. */
            if (curr_req->mrail.protocol == MV2_RNDV_PROTOCOL_R3) {
                int tmp = 0;
                MPIDI_CH3U_Request_increment_cc(curr_req, &tmp);
            }
        }
#endif /* defined(CHANNEL_MRAIL) */

        /* release the target datatype */
        MPID_Datatype_release(dtp);

        /* If send is finished, we free extended header immediately.
         * Otherwise, store its pointer in request thus it can be freed when request is freed.*/
        if (curr_req != NULL) {
            curr_req->dev.ext_hdr_ptr = ext_hdr_ptr;
        }
        else {
            MPIU_Free(ext_hdr_ptr);
        }
    }

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

    /* release the request returned by iStartMsg or iStartMsgv */
    if (req != NULL) {
        MPID_Request_release(req);
    }

    rma_op->single_req = curr_req;

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ISSUE_GET_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    rma_op->single_req = NULL;
    rma_op->reqs_size = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME issue_cas_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int issue_cas_op(MPIDI_RMA_Op_t * rma_op,
                        MPID_Win * win_ptr, MPIDI_RMA_Target_t * target_ptr,
                        MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_VC_t *vc = NULL;
    MPID_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_cas_t *cas_pkt = &rma_op->pkt.cas;
    MPID_Request *rmw_req = NULL;
    MPID_Request *curr_req = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_CAS_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ISSUE_CAS_OP);

    rma_op->reqs_size = 1;

    /* Create a request for the RMW response.  Store the origin buf, count, and
     * datatype in it, and pass the request's handle RMW packet. When the
     * response comes from the target, it will contain the request handle. */
    curr_req = MPID_Request_create();
    MPIR_ERR_CHKANDJUMP(curr_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    /* Set refs on the request to 2: one for the response message, and one for
     * the partial completion handler */
    MPIU_Object_set_ref(curr_req, 2);

    curr_req->dev.user_buf = rma_op->result_addr;
    curr_req->dev.datatype = rma_op->result_datatype;

    curr_req->dev.target_win_handle = MPI_WIN_NULL;
    curr_req->dev.source_win_handle = win_ptr->handle;

    cas_pkt->request_handle = curr_req->handle;
    cas_pkt->flags |= flags;

#if defined (CHANNEL_PSM)
    cas_pkt->source_rank = win_ptr->comm_ptr->rank;
    cas_pkt->mapped_srank = win_ptr->rank_mapping[win_ptr->comm_ptr->rank];
    cas_pkt->mapped_trank = win_ptr->rank_mapping[rma_op->target_rank];
#endif /* CHANNEL_PSM */

    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);
    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, cas_pkt, sizeof(*cas_pkt), &rmw_req);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

    if (rmw_req != NULL) {
        MPID_Request_release(rmw_req);
    }

    rma_op->single_req = curr_req;

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ISSUE_CAS_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    rma_op->single_req = NULL;
    rma_op->reqs_size = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME issue_fop_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int issue_fop_op(MPIDI_RMA_Op_t * rma_op,
                        MPID_Win * win_ptr, MPIDI_RMA_Target_t * target_ptr,
                        MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_VC_t *vc = NULL;
    MPID_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_fop_t *fop_pkt = &rma_op->pkt.fop;
    MPID_Request *resp_req = NULL;
    MPID_Request *curr_req = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_FOP_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ISSUE_FOP_OP);

    rma_op->reqs_size = 1;

    /* Create a request for the GACC response.  Store the response buf, count, and
     * datatype in it, and pass the request's handle in the GACC packet. When the
     * response comes from the target, it will contain the request handle. */
    resp_req = MPID_Request_create();
    MPIR_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    MPIU_Object_set_ref(resp_req, 2);

    resp_req->dev.user_buf = rma_op->result_addr;
    resp_req->dev.datatype = rma_op->result_datatype;
    resp_req->dev.target_win_handle = MPI_WIN_NULL;
    resp_req->dev.source_win_handle = win_ptr->handle;

    fop_pkt->request_handle = resp_req->handle;

#if defined (CHANNEL_PSM)
    fop_pkt->source_rank = win_ptr->comm_ptr->rank;
    fop_pkt->mapped_srank = win_ptr->rank_mapping[win_ptr->comm_ptr->rank];
    fop_pkt->mapped_trank = win_ptr->rank_mapping[rma_op->target_rank];
#endif /* CHANNEL_PSM */

    fop_pkt->flags |= flags;

    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    if (rma_op->pkt.type == MPIDI_CH3_PKT_FOP_IMMED) {
        /* All origin data is in packet header, issue the header. */
        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, fop_pkt, sizeof(*fop_pkt), &curr_req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }
    else {
        MPI_Aint origin_dtp_size;
        MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_dtp_size);
        mpi_errno = issue_from_origin_buffer(rma_op, vc, NULL, 0,       /*ext_hdr_ptr, ext_hdr_sz */
                                             0, 1 * origin_dtp_size, &curr_req);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

    if (curr_req != NULL) {
        MPID_Request_release(curr_req);
    }

    rma_op->single_req = resp_req;

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ISSUE_FOP_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    rma_op->single_req = NULL;
    rma_op->reqs_size = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


/* issue_rma_op() is called by ch3u_rma_progress.c, it triggers
   proper issuing functions according to packet type. */
#undef FUNCNAME
#define FUNCNAME issue_rma_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int issue_rma_op(MPIDI_RMA_Op_t * op_ptr, MPID_Win * win_ptr,
                               MPIDI_RMA_Target_t * target_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_RMA_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ISSUE_RMA_OP);

    switch (op_ptr->pkt.type) {
    case (MPIDI_CH3_PKT_PUT):
    case (MPIDI_CH3_PKT_PUT_IMMED):
        mpi_errno = issue_put_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_ACCUMULATE):
    case (MPIDI_CH3_PKT_ACCUMULATE_IMMED):
        mpi_errno = issue_acc_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_GET_ACCUM):
    case (MPIDI_CH3_PKT_GET_ACCUM_IMMED):
        mpi_errno = issue_get_acc_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_GET):
        mpi_errno = issue_get_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_CAS_IMMED):
        mpi_errno = issue_cas_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_FOP):
    case (MPIDI_CH3_PKT_FOP_IMMED):
        mpi_errno = issue_fop_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    default:
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**winInvalidOp");
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ISSUE_RMA_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#endif /* MPID_RMA_ISSUE_H_INCLUDED */
