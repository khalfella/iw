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

/* sync options */
#define	XDMA_CMD_SYNC_FORCPU		0x01	/* sync for cpu (from device ) */
#define	XDMA_CMD_SYNC_FORDEV		0x02	/* sync for dev (to device ) */

#define XDMA_ISSUE_COMMAND(cmd, C)	((*((char *) (&(cmd)->xc_command))) = (C))

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

static void
x_dma_init_cmd(xdma_cmd_t *cmd, uint64_t command, uint64_t status,
    uint64_t type, uint64_t dir, uint64_t size, uint64_t gx_off,
    uint64_t hx_phys, uint64_t gb_vir, uint64_t gb_phys, uint64_t gb_off)
{
	cmd->xc_command = command;
	cmd->xc_status = status;
	cmd->xc_type = type;
	cmd->xc_dir = dir;
	cmd->xc_size = size;
	cmd->xc_gx_off = gx_off;
	cmd->xc_hx_phys = hx_phys;
	cmd->xc_gb_vir = gb_vir;
	cmd->xc_gb_phys = gb_phys;
	cmd->xc_gb_off = gb_off;
}

static int
x_dma_map_info(struct iwl_trans *trans, xdma_cmd_t *cmd, dma_addr_t dma_handle)
{

	x_dma_init_cmd(cmd, 0, 0, 0, 0, 0, 0, dma_handle, 0, 0, 0);
	smp_wmb();
	XDMA_ISSUE_COMMAND(cmd, XDMA_CMD_COMMAND_INQUIRY);

	if (cmd->xc_status != XDMA_CMD_STATUS_OK) {
		dev_err(trans->dev, "XDMA %s:failed\n", __func__);
		return (1);
	}
	return (0);
}

static dma_addr_t
x_dma_alloc_map(struct iwl_trans *trans, uint64_t type, uint64_t zero,
    size_t size, uint64_t *gx_off, uint64_t *hx_phys,
    uint64_t gb_vir, uint64_t gb_phys, uint64_t gb_off) {

	xdma_cmd_t *cmd;

	/* This is a workaround for a bug in the driver */
	if (size == 0x200) {
		size = 0x1000;
	}

	cmd = (xdma_cmd_t *) trans->dma_base;
	x_dma_init_cmd(cmd, 0, 0, type, zero, size, 0, 0, gb_vir, gb_phys, gb_off);
	smp_wmb();
	XDMA_ISSUE_COMMAND(cmd, XDMA_CMD_COMMAND_ALLOC);

	dev_err(trans->dev, "XDMA %s:allocating %lx bytes phys = %llx offset = %llx\n",
	    __func__, size, cmd->xc_hx_phys, cmd->xc_gx_off);

	if (cmd->xc_status != XDMA_CMD_STATUS_OK) {
		dev_err(trans->dev, "XDMA %s:failed allocating xdma map\n", __func__);
		return (0);
	}

	*hx_phys = cmd->xc_hx_phys;
	*gx_off = cmd->xc_gx_off;
	return (*hx_phys);
}

static void
x_dma_free_map(struct iwl_trans *trans, dma_addr_t dma_handle, uint64_t type)

{
	xdma_cmd_t *cmd;
	cmd = (xdma_cmd_t *) trans->dma_base;
	x_dma_init_cmd(cmd, 0, 0, type, 0, 0, 0, dma_handle, 0, 0, 0);
	smp_wmb();
	XDMA_ISSUE_COMMAND(cmd, XDMA_CMD_COMMAND_REMOVE);

	if (cmd->xc_status != XDMA_CMD_STATUS_OK) {
		dev_err(trans->dev, "%s: failed freeing phys = %llx\n", __func__, cmd->xc_hx_phys);
	}
}

static void
x_dma_sync_streaming_map(struct iwl_trans *trans, dma_addr_t dma_handle, uint64_t gx_off,
    uint64_t gb_vir, uint64_t gb_off, size_t size, uint8_t sdir)
{
	xdma_cmd_t *cmd;

	if (sdir == XDMA_CMD_SYNC_FORDEV) {
		memcpy((char *) (trans->dma_base) + gx_off, (char *) gb_vir + gb_off, size);
	}

	cmd = (xdma_cmd_t *) trans->dma_base;
	x_dma_init_cmd(cmd, 0, 0, XDMA_CMD_MAP_TYPE_STR, sdir, 0, 0, dma_handle, 0, 0, 0);
	smp_wmb();
	XDMA_ISSUE_COMMAND(cmd, XDMA_CMD_COMMAND_SYNC);

	if (cmd->xc_status != XDMA_CMD_STATUS_OK) {
		dev_err(trans->dev, "XDMA %s:failed syncing\n", __func__);
		return;
	}

	if (sdir == XDMA_CMD_SYNC_FORCPU) {
		memcpy((char *) gb_vir + gb_off, (char *) (trans->dma_base) + gx_off, size);
	}
}

static dma_addr_t
x_dma_alloc_streaming_map(struct iwl_trans *trans, void *cpu_addr,
    unsigned long offset, size_t size, enum dma_data_direction dir)
{
	dma_addr_t hx_phys, gx_off;

	if (x_dma_alloc_map(trans, XDMA_CMD_MAP_TYPE_STR, 0, size, &gx_off, &hx_phys, (uintptr_t) cpu_addr, 0, offset) == 0) {
		return (0);
	}

	if (dir == DMA_TO_DEVICE || dir == DMA_BIDIRECTIONAL || 1 == 1) {
		x_dma_sync_streaming_map(trans, hx_phys, gx_off, (uintptr_t) cpu_addr, offset, size, XDMA_CMD_SYNC_FORDEV);
	}

	return (hx_phys);
}

static void
x_dma_free_streaming_map(struct iwl_trans *trans, dma_addr_t dma_handle,
    size_t size, enum dma_data_direction dir)
{

	xdma_cmd_t *cmd;
	cmd = (xdma_cmd_t *) trans->dma_base;

	if (x_dma_map_info(trans, cmd, dma_handle) != 0)
		return;

	if (dir == DMA_FROM_DEVICE || dir == DMA_BIDIRECTIONAL || 1 == 1) {
		x_dma_sync_streaming_map(trans, cmd->xc_hx_phys, cmd->xc_gx_off, cmd->xc_gb_vir, cmd->xc_gb_off, cmd->xc_size, XDMA_CMD_SYNC_FORCPU);
	}

	x_dma_free_map(trans, cmd->xc_hx_phys, XDMA_CMD_MAP_TYPE_STR);
}

static void *
x_dma_alloc_coherent_common(struct iwl_trans *trans, size_t size, dma_addr_t *dma_handle, int bzero)
{

	uint64_t hx_phys, gx_off;

	*dma_handle = 0;
	if (x_dma_alloc_map(trans, XDMA_CMD_MAP_TYPE_COH, bzero, size, &gx_off, &hx_phys, 0, 0, 0) != 0) {
		*dma_handle = hx_phys;
		return (char *)(trans->dma_base) + gx_off;
	}
	return (NULL);
}

dma_addr_t
x_dma_map_page(struct iwl_trans *trans, struct page *page,
    unsigned long offset, size_t size, enum dma_data_direction dir)
{
	dma_addr_t phys;
	mutex_lock(&trans->xdma_mutex);
	phys = x_dma_alloc_streaming_map(trans, page_address(page), offset, size, dir);
	mutex_unlock(&trans->xdma_mutex);
	return (phys);
}

void
x_dma_unmap_page(struct iwl_trans *trans, dma_addr_t dma_handle,
	       size_t size, enum dma_data_direction dir)
{
	mutex_lock(&trans->xdma_mutex);
	x_dma_free_streaming_map(trans, dma_handle, size, dir);
	mutex_unlock(&trans->xdma_mutex);
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
	xdma_cmd_t *cmd;
	cmd = (xdma_cmd_t *) trans->dma_base;

	mutex_lock(&trans->xdma_mutex);
	if (x_dma_map_info(trans, cmd, dma_handle) == 0) {
		x_dma_sync_streaming_map(trans, cmd->xc_hx_phys,
		    cmd->xc_gx_off, cmd->xc_gb_vir, cmd->xc_gb_off,
		    cmd->xc_size, XDMA_CMD_SYNC_FORCPU);
	}
	mutex_unlock(&trans->xdma_mutex);
}

void
x_dma_sync_single_for_device(struct iwl_trans *trans, dma_addr_t dma_handle,
			   size_t size, enum dma_data_direction dir)
{
	xdma_cmd_t *cmd;
	cmd = (xdma_cmd_t *) trans->dma_base;

	mutex_lock(&trans->xdma_mutex);
	if (x_dma_map_info(trans, cmd, dma_handle) == 0) {
		x_dma_sync_streaming_map(trans, cmd->xc_hx_phys,
		    cmd->xc_gx_off, cmd->xc_gb_vir, cmd->xc_gb_off,
		    cmd->xc_size, XDMA_CMD_SYNC_FORDEV);
	}
	mutex_unlock(&trans->xdma_mutex);
}

void *
x_dma_alloc_coherent(struct iwl_trans *trans, size_t size, dma_addr_t *dma_handle, gfp_t gfp)
{
	void *ret;
	mutex_lock(&trans->xdma_mutex);
	ret =  x_dma_alloc_coherent_common(trans, size, dma_handle, 0);
	mutex_unlock(&trans->xdma_mutex);
	return (ret);
}

void *
x_dma_zalloc_coherent(struct iwl_trans *trans, size_t size, dma_addr_t *dma_handle, gfp_t gfp)
{
	void *ret;
	mutex_lock(&trans->xdma_mutex);
	ret =  x_dma_alloc_coherent_common(trans, size, dma_handle, 1);
	mutex_unlock(&trans->xdma_mutex);
	return (ret);
}

void
x_dma_free_coherent(struct iwl_trans *trans, size_t size, void *kvaddr,
		       dma_addr_t dma_handle)
{
	mutex_lock(&trans->xdma_mutex);
	x_dma_free_map(trans, dma_handle, XDMA_CMD_MAP_TYPE_COH);
	mutex_unlock(&trans->xdma_mutex);
}

dma_addr_t
x_dma_map_single(struct iwl_trans *trans, void *cpu_addr, size_t size,
	       enum dma_data_direction dir)
{
	dma_addr_t phys;
	mutex_lock(&trans->xdma_mutex);
	phys = x_dma_alloc_streaming_map(trans, cpu_addr, 0, size, dir);
	mutex_unlock(&trans->xdma_mutex);
	return (phys);
}

void
x_dma_unmap_single(struct iwl_trans *trans, dma_addr_t dma_addr,
		 size_t size, enum dma_data_direction dir)
{
	mutex_lock(&trans->xdma_mutex);
	x_dma_free_streaming_map(trans, dma_addr, size, dir);
	mutex_unlock(&trans->xdma_mutex);
}

dma_addr_t x_skb_frag_dma_map(struct iwl_trans *trans, const skb_frag_t *frag,
    size_t offset, size_t size, enum dma_data_direction dir)
{
	dma_addr_t phys;

	mutex_lock(&trans->xdma_mutex);
	phys = x_dma_map_page(trans, skb_frag_page(frag),
			    frag->page_offset + offset, size, dir);
	mutex_unlock(&trans->xdma_mutex);

	return (phys);
}

#endif
