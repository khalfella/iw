#ifndef __iwl_xdma_h__
#define __iwl_xdma_h__

#include "iwl-devtrace.h"
#include "iwl-trans.h"

dma_addr_t
x_dma_map_page(struct device *dev, struct page *page,
    unsigned long offset, size_t size, enum dma_data_direction dir)
{
	return dma_map_page(dev, page, offset, size, dir);
}

void
x_dma_unmap_page(struct device *dev, dma_addr_t dma_handle,
	       size_t size, enum dma_data_direction dir)
{
	dma_unmap_page(dev, dma_handle, size, dir);
}

int
x_dma_mapping_error(struct device *dev, dma_addr_t dma_addr)
{
	return (dma_addr == 0);
}

void
x_dma_sync_single_for_cpu(struct device *dev, dma_addr_t dma_handle,
			size_t size, enum dma_data_direction dir)
{
	dma_sync_single_for_cpu(dev, dma_handle, size, dir);
}

void
x_dma_sync_single_for_device(struct device *dev, dma_addr_t dma_handle,
			   size_t size, enum dma_data_direction dir)
{
	dma_sync_single_for_device(dev, dma_handle, size, dir);
}

void
*x_dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle, gfp_t gfp)
{
	return dma_alloc_coherent(dev, size, dma_handle, gfp);
}

void
*x_dma_zalloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle, gfp_t gfp)
{
	return dma_zalloc_coherent(dev, size, dma_handle, gfp);
}

void
x_dma_free_coherent(struct device *dev, size_t size, void *kvaddr,
		       dma_addr_t dma_handle)
{
	dma_free_coherent(dev, size, kvaddr, dma_handle);
}



dma_addr_t
x_dma_map_single(struct device *dev, void *cpu_addr, size_t size,
	       enum dma_data_direction dir)
{
	return dma_map_single(dev, cpu_addr, size, dir);
}

void
x_dma_unmap_single(struct device *dev, dma_addr_t dma_addr,
		 size_t size, enum dma_data_direction dir)
{
	dma_unmap_single(dev, dma_addr, size, dir);
}

dma_addr_t x_skb_frag_dma_map(struct device *dev, const skb_frag_t *frag,
    size_t offset, size_t size, enum dma_data_direction dir)
{
	return skb_frag_dma_map(dev, frag, offset, size, dir);
}

#endif