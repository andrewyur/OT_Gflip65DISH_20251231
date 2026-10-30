
#ifndef __SCP_RESERVEDMEM_DEFINE_H__
#define __SCP_RESERVEDMEM_DEFINE_H__

static struct scp_reserve_mblock scp_reserve_mblock[] = {
#ifdef CONFIG_MTK_VOW_SUPPORT
#if defined(CONFIG_MTK_VOW_AMAZON_SUPPORT) || \
	defined(CONFIG_MTK_VOW_GVA_SUPPORT)
	{
		.num = VOW_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x1DC00,  /* 119KB */
	},
#else
	{
		.num = VOW_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x1DC00,  /* 119KB */
	},
#endif
#endif
	{
		.num = SENS_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x100000, /* 1MB */
	},
#ifdef CONFIG_MTK_AUDIO_TUNNELING_SUPPORT
	{
		.num = MP3_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x400000, /* 4MB */
	},
#endif
	{
		.num = FLP_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x1000, /* 4KB */
	},
	{
		.num = SCP_A_LOGGER_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x180000, /* 1.5MB */
	},
	{
		.num = AUDIO_IPI_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x40000, /* 256KB */
	},
#ifdef CONFIG_SND_SOC_MTK_SCP_SMARTPA
	{
		.num = SPK_PROTECT_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x30000, /* 192KB */
	},
#endif
#ifdef CONFIG_MTK_VOW_BARGE_IN_SUPPORT
	{
		.num = VOW_BARGEIN_MEM_ID,
		.start_phys = 0x0,
		.start_virt = 0x0,
		.size = 0x2000,  /* 8KB */
	},
#endif
};


#endif
