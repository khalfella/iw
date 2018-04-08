#ifndef __iwl_xdma_h__
#define __iwl_xdma_h__

#include "iwl-devtrace.h"
#include "iwl-trans.h"

typedef struct {
	union {
		uint8_t cmd;
		uint64_t command;
	} c;
	uint64_t status;
	uint64_t out1;
	uint64_t out2;
	uint64_t out3;
	uint64_t in1;
	uint64_t in2;
	uint64_t in3;
	uint64_t in4;
	uint64_t in5;
	uint64_t in6;
	uint64_t in7;
} xdma_command_t;

dma_addr_t
x_dma_map_page(struct iwl_trans *trans, struct page *page,
    unsigned long offset, size_t size, enum dma_data_direction dir)
{
	return dma_map_page(trans->dev, page, offset, size, dir);
}

void
x_dma_unmap_page(struct iwl_trans *trans, dma_addr_t dma_handle,
	       size_t size, enum dma_data_direction dir)
{
	dma_unmap_page(trans->dev, dma_handle, size, dir);
}

int
x_dma_mapping_error(struct iwl_trans *trans, dma_addr_t dma_addr)
{
	return (dma_addr == 0);
}

void
x_dma_sync_single_for_cpu(struct iwl_trans *trans, dma_addr_t dma_handle,
			size_t size, enum dma_data_direction dir)
{
	dma_sync_single_for_cpu(trans->dev, dma_handle, size, dir);
}

void
x_dma_sync_single_for_device(struct iwl_trans *trans, dma_addr_t dma_handle,
			   size_t size, enum dma_data_direction dir)
{
	dma_sync_single_for_device(trans->dev, dma_handle, size, dir);
}

void
*x_dma_alloc_coherent_common(struct iwl_trans *trans, size_t size, dma_addr_t *dma_handle, gfp_t gfp,int bzero)
{
	xdma_command_t *cmd;

	cmd = (xdma_command_t *) trans->dma_base;

	cmd->c.command = 0;		/* Reset Command area to zero */
	cmd->in1 = bzero; 		/* in1 flags: bzero */
	cmd->in2 = size;		/* in2 size */
	cmd->c.cmd = 1;			/* Trigger XDMA command */
	
	dev_err(trans->dev, "XDMA allocating %lx bytes phys = %llx offset = %llx\n",
	    size, cmd->out1, cmd->out2);

	*dma_handle = cmd->out1;
	return (char *)(trans->dma_base) + cmd->out2;

	/*
		return dma_alloc_coherent(trans->dev, size, dma_handle, gfp);
	*/
}
void
*x_dma_alloc_coherent(struct iwl_trans *trans, size_t size, dma_addr_t *dma_handle, gfp_t gfp)
{
	return x_dma_alloc_coherent_common(trans, size, dma_handle, gfp, 0);
}

void
*x_dma_zalloc_coherent(struct iwl_trans *trans, size_t size, dma_addr_t *dma_handle, gfp_t gfp)
{
	return x_dma_alloc_coherent_common(trans, size, dma_handle, gfp, 1);
}

void
x_dma_free_coherent(struct iwl_trans *trans, size_t size, void *kvaddr,
		       dma_addr_t dma_handle)
{
	/*
	dma_free_coherent(trans->dev, size, kvaddr, dma_handle);
	*/
}



dma_addr_t
x_dma_map_single(struct iwl_trans *trans, void *cpu_addr, size_t size,
	       enum dma_data_direction dir)
{
	return dma_map_single(trans->dev, cpu_addr, size, dir);
}

void
x_dma_unmap_single(struct iwl_trans *trans, dma_addr_t dma_addr,
		 size_t size, enum dma_data_direction dir)
{
	dma_unmap_single(trans->dev, dma_addr, size, dir);
}

dma_addr_t x_skb_frag_dma_map(struct iwl_trans *trans, const skb_frag_t *frag,
    size_t offset, size_t size, enum dma_data_direction dir)
{
	return skb_frag_dma_map(trans->dev, frag, offset, size, dir);
}

#endif
