#ifndef __iwl_xdma_h__
#define __iwl_xdma_h__

#include "iwl-devtrace.h"
#include "iwl-trans.h"

dma_addr_t x_dma_map_page(struct iwl_trans *trans, struct page *page,
    unsigned long offset, size_t size, enum dma_data_direction dir);

void x_dma_unmap_page(struct iwl_trans *trans, dma_addr_t dma_handle,
    size_t size, enum dma_data_direction dir);

int x_dma_mapping_error(struct iwl_trans *trans, dma_addr_t dma_addr);

void x_dma_sync_single_for_cpu(struct iwl_trans *trans, dma_addr_t dma_handle,
    size_t size, enum dma_data_direction dir);
void x_dma_sync_single_for_device(struct iwl_trans *trans, dma_addr_t dma_handle,
    size_t size, enum dma_data_direction dir);

void *x_dma_alloc_coherent(struct iwl_trans *trans, size_t size, dma_addr_t *dma_handle, gfp_t gfp);
void *x_dma_zalloc_coherent(struct iwl_trans *trans, size_t size, dma_addr_t *dma_handle, gfp_t gfp);
void x_dma_free_coherent(struct iwl_trans *trans, size_t size, void *kvaddr, dma_addr_t dma_handle);

dma_addr_t x_dma_map_single(struct iwl_trans *trans, void *cpu_addr,
    size_t size, enum dma_data_direction dir);
void x_dma_unmap_single(struct iwl_trans *trans, dma_addr_t dma_addr,
    size_t size, enum dma_data_direction dir);


dma_addr_t x_skb_frag_dma_map(struct iwl_trans *trans, const skb_frag_t *frag,
    size_t offset, size_t size, enum dma_data_direction dir);

#endif
