#ifndef __iwl_xdma_h__
#define __iwl_xdma_h__

#include "iwl-devtrace.h"
#include "iwl-trans.h"


/* xdma commands */
#define	XDMA_CMD_COMMAND_ALLOC		0x01	/* allocate new map */
#define	XDMA_CMD_COMMAND_REMOVE		0x02	/* remove an existing map */
#define	XDMA_CMD_COMMAND_INQUIRY	0x03	/* get map info */
#define	XDMA_CMD_COMMAND_SYNC		0x04	/* sync cpu/device map view */

/* xdma command return status */
#define	XDMA_CMD_STATUS_OK		0x00	/* We are good */
#define	XDMA_CMD_STATUS_ER		0x01	/* Something wrong */

/* xdma command map type */
#define	XDMA_CMD_MAP_TYPE_COH		0x01	/* coherent map */
#define	XDMA_CMD_MAP_TYPE_STR		0x02	/* streaming map */

typedef struct xdma_cmd_s {
	uint64_t	xc_command;		/* alloc, rem, inq */
	uint64_t	xc_status;		/* ok, error */
	uint64_t	xc_type;		/* coherent, streaming */
	uint64_t	xc_dir;			/* map direction */
	uint64_t	xc_size;		/* map size */
	uint64_t	xc_gx_off;		/* offset in xdma map */
	uint64_t	xc_hx_phys;		/* cookie or dma_addr_t */

	/* guest buffer for streaming dma */
	uint64_t	xc_gb_vir;		/* guest buffer vir addr */
	uint64_t	xc_gb_phys;		/* guest buffer phys addr */
	uint64_t	xc_gb_off;		/* guest buffer map off */
} xdma_cmd_t;

dma_addr_t
x_dma_map_page(struct iwl_trans *trans, struct page *page,
    unsigned long offset, size_t size, enum dma_data_direction dir)
{
	/*
		We need to consider this offset thing

		vir, phys  = qemu_allocate_buffer(size, dir, page [keep it with you]);
		if (dir is (to_device) or (bi_directional)) {
			pvir = map_page_to_vir(page);
			copy(pvir, vir);		// copy the page to the vir
			qemu_sync_to_device(vir);
		}

		return phys;
	 */
	return dma_map_page(trans->dev, page, offset, size, dir);
}

void
x_dma_unmap_page(struct iwl_trans *trans, dma_addr_t dma_handle,
	       size_t size, enum dma_data_direction dir)
{
	/*
		if (dir is (from_device) or (bi_directional)) {
			vir, page = qemu_tell_me_about_this_cookie(dma_handle);
			qemu_sync_from_device(vir);
			pvir = map_page_to_vir(page);
			copy(vir, pvir)			// copy sync the page we saved
		}

		qemu_free_this_virtual_address(vir);
	 */
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
	/*
		if (dir is (from_device) or (bi_directional)) {
			vir, page = qemu_tell_me_about_this_cookie(dma_handle);
			qemu_sync_from_device(vir);
		}
	 */
	dma_sync_single_for_cpu(trans->dev, dma_handle, size, dir);
}

void
x_dma_sync_single_for_device(struct iwl_trans *trans, dma_addr_t dma_handle,
			   size_t size, enum dma_data_direction dir)
{
	dma_sync_single_for_device(trans->dev, dma_handle, size, dir);
}

void *
x_dma_alloc_coherent_common(struct iwl_trans *trans, size_t size, dma_addr_t *dma_handle, gfp_t gfp,int bzero)
{
	xdma_cmd_t *cmd;

	cmd = (xdma_cmd_t *) trans->dma_base;

	cmd->xc_command = 0;				/* Reset Command area to zero */
	cmd->xc_type = XDMA_CMD_MAP_TYPE_COH;		/* Allocate coherent map */
	cmd->xc_dir = bzero;				/* 1 -> zero allocated mem */
	cmd->xc_size = size;				/* size */

	cmd->xc_hx_phys = 0;
	cmd->xc_gx_off = 0;
	cmd->xc_gb_vir = 0;
	cmd->xc_gb_phys = 0;
	cmd->xc_gb_off = 0;

	smp_wmb();

	*((char *) (&cmd->xc_command)) = XDMA_CMD_COMMAND_ALLOC;	/* Trigger XDMA command */
	
	dev_err(trans->dev, "XDMA allocating %lx bytes phys = %llx offset = %llx\n",
	    size, cmd->xc_hx_phys, cmd->xc_gx_off);

	if (cmd->xc_status != XDMA_CMD_STATUS_OK) {
		*dma_handle = 0;
		return (NULL);
	}

	*dma_handle = cmd->xc_hx_phys;
	return (char *)(trans->dma_base) + cmd->xc_gx_off;

	/*
	 * return dma_alloc_coherent(trans->dev, size, dma_handle, gfp);
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
	/*
		Allocate mapping, sync it if necessary. save the vaddr, viraddress
	 */
	return dma_map_single(trans->dev, cpu_addr, size, dir);
}

void
x_dma_unmap_single(struct iwl_trans *trans, dma_addr_t dma_addr,
		 size_t size, enum dma_data_direction dir)
{
	/*
		query the map, get the vaddr and the viraddress
		sync back if necessary
		remove the mapping
	 */
	dma_unmap_single(trans->dev, dma_addr, size, dir);
}

dma_addr_t x_skb_frag_dma_map(struct iwl_trans *trans, const skb_frag_t *frag,
    size_t offset, size_t size, enum dma_data_direction dir)
{
	return skb_frag_dma_map(trans->dev, frag, offset, size, dir);
}

#endif
