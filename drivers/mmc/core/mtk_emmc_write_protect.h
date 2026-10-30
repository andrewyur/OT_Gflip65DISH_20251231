
#ifndef _MTK_EMMC_WRITE_PROTECT_H_
#define _MTK_EMMC_WRITE_PROTECT_H_


#define EXT_CSD_USR_WP                  171     /* R/W */
#define EXT_CSD_CMD_SET_NORMAL          (1<<0)
#define EXT_CSD_USR_WP_EN_PERM_WP       (1<<2)
#define EXT_CSD_USR_WP_EN_PWR_WP        (1)


int set_power_on_write_protect(struct mmc_card *card);
int emmc_set_wp_by_partitions(struct mmc_card *card,
	char *partition_nametab[], int partition_num, unsigned char wp_type);

#endif
