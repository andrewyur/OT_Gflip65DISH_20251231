/*
 *  History 1.0: Add by bin.song.hz for task:9148940 on 2020-03.31
 *  Update by dingting.meng for T-9615384 on 11/7/2020
 *  Update by dingting.meng for T-10091767 on 2020-10-20
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/sysfs.h>

#include <linux/uaccess.h>

extern char eMMC_module_name[256];

extern struct device* get_deviceinfo_dev(void);

extern unsigned int get_ddr_sz_GB(void);

static u32 *pcid = NULL;
static u32 cap_secs = 0;

static u8 *emmc_tab[][5] = {
    /* eMMC + DDR */
    {"13014E53304A394B38", "MT29VZZZAD8DQKSL_046_W_9K8", "Micron", "64G+4G", "AMD0000675C1"},
    {"150100444436384D42", "KMDD60018M_B320", "Samsung", "32G+3G", "AMD0000615C1"},
    {"150100444836444142", "KMDH6001DA_B422", "Samsung", "64G+4G", "AMD0000607C1"},
    {"150100445636444D42", "KMDV6001DM_B620", "Samsung", "128G+4G", "AMD0000661C1"},
    {"150100335636434D42", "KM3V6001CM_B705", "Samsung", "128G+6G", "AMD0000588C1"},
    {"150100514536334D42", "KMQE60013M_B318", "Samsung", "16G+2G", "AMD0000555C1"},
    {"150100515836334142", "KMQX60013A_B419", "Samsung", "32G+2G", "AMD0000677C1"},
    {"150100514436334D42", "KMQD60013M_B318", "Samsung", "32G+2G", "AMD0000623C1"},
    //Begin Add eMCP_MT29VZZZAD8GQFSL_046W_9R8_Dev_Info for T-9438089,on 20200515
    {"13014E47314A395238", "MT29VZZZAD8GQFSL_046W_9R8", "Micron", "64G+4G", "AMD0000700C1"},
    {"13014E47314A395339", "MT29VZZZAD9GQFSM_046W_9S9", "Micron", "128G+4G", "AMD0000711C1"},
    //End Add eMCP_MT29VZZZAD8GQFSL_046W_9R8_Dev_Info for T-9438089,on 20200515
    //Begin add by dingting.meng for T-9574860 on 1/7/2020
    {"150100475836424142", "KMGX6001BA_B514", "Samsung", "32G+3G", "AMD0000674C1"},
    {"150100525036344D42", "KMRP60014M_B614", "Samsung", "64G+4G", "AMD0000635C1"},
    //End add by dingting.meng for T-9574860 on 1/7/2020
    //Begin add by dingting.meng for T-9612163 on 9/7/2020
    {"8F0100555935435330", "UNPVN5G4CACA4BS", "UNIC", "32G+2G", "AMD0000704C1"},
    {"8F0100555935435330", "UNPVN5GACACA4BS", "UNIC", "32G+3G", "AMD0000664C1"},
    {"8F0100555936435330", "UNPVN6GACACA4CS", "UNIC", "64G+3G", "AMD0000XXXXX"},
    {"8F0100555936435330", "UNPVN6G5CACA4CS", "UNIC", "64G+4G", "AMD0000663C1"},
    {"150100445836384D42", "KMDX60018M_B425", "Samsung", "XXG+XXG", "XXXXXXXX"},
    //Begin add by dingting.meng for T-9612163 on 9/7/2020
    //begin add by linsheng.liu for task 10446482 deviceinfo on 2020/12/9
    #ifdef CONFIG_WRIGHTPRO_DEVICEINFO	
    {"F40122426977696E20", "BWCTAK411G32G", "BIWIN", "32G+1G", "AMD0000689C1"},
    {"F40122426977696E20", "BWCTAK411G32G", "BIWIN", "32G+2G", "AMD0000689C1"},
	{"880103534C44333247", "NCEMASLD-32G", "LONGSYS", "32G+1G", "AMD0000653C1"},
	{"880103534C44333247", "NCEMASLD-32G", "LONGSYS", "32G+2G", "AMD0000653C1"},
    #endif    
    //end add by linsheng.liu for task 10446482 deviceinfo on 2020/12/9
    /* Separate eMMC */
    {"8F0100555935435330", "UNMEN05GC1C31A", "UNIC", "32G", "AMD0000658C1"},
    {"880103534C44333247", "NCEMASLD-32G", "LONGSYS", "32G", "AMD0000653C1"},
    {"F40122426977696E20", "BWCTAK411G32G", "BIWIN", "32G", "AMD0000689C1"},
    {"880103534C44363447", "NCEMASLD-64G", "LONGSYS", "64G", "AMD0000724C1"},
    {"880103534C44313238", "NCEMASLD-128G", "LONGSYS", "128G", "AMD0000725C1"},
    {"F40122426977696E20", "BWCTASC21P64G", "BIWIN", "64G", "AMD0000716C1"},
    {"700100443236303332", "EMMC32G-TD26", "KINGSTON", "32G", "01.EM9.K32GFA1532A"},
    {"880103534C44363447", "NCEMASLD-64G", "LONGSYS", "64G", "01.EM9.F64GF15301A"},
    {"D6010335384B373231", "FEMDNN064G-58K72", "FORESEE", "64G", "01.EM9.F64GF15302A"},
    {"D601004D4D43363447", "HG-EMC064-N1510", "HOSIN", "64G", "01.EM9.H64GFA1531A"},
    {"F4012242575A543134", "BWCTAK421G64G", "BIWIN", "64G", "AMD0000765C1"},
    //add by jingqing.yan for cruze&cruze pro emmc begin 2021/08/12
    {"EA2D00534136323131", "KASA621101T0000", "KOWIN", "32G", "AMD0000746C1"},
    {"EA2D00534137323131", "KASA721101T0000", "KOWIN", "64G", "AMD0000784C1"},
    {"F40122426977696E20", "BWCTASC41P128G", "BIWIN", "128G", "AMD0000737C1"},
    //add by jingqing.yan for cruze&cruze pro emmc end 2021/08/12
    {NULL,NULL,NULL,NULL,NULL},
};

static u32 get_emmc_sz_GB(u32 cap)
{
    u32 tmp = 0;
    u32 cap_GB = 1;

    tmp = cap >> 21;/* *512/1024/1024/1024 */

    while(cap_GB < tmp)
    {
        cap_GB = cap_GB << 1;
    }

    return cap_GB;
}

void get_emmc_cid_and_cap(u32 *cid,u32 cap_sec)
{
	pcid = cid;
	cap_secs = cap_sec;
}

EXPORT_SYMBOL(get_emmc_cid_and_cap);

void set_emmc_devinfo(u32 * cid,u32 cap)
{
    bool found = false;
    int index = 0;
    unsigned int ddr_sz_GB = 0;
    unsigned int emmc_sz_GB = 0;
    u8 emmc_id[32]; /* store the emmc id. */
    u8 emdr[16] = {'\0'};

    ddr_sz_GB = get_ddr_sz_GB();
    emmc_sz_GB = get_emmc_sz_GB(cap);

    printk("#### %s|%d dsize = %d GB esize= %d GB####\n",__func__,__LINE__,ddr_sz_GB,emmc_sz_GB);

    sprintf(emmc_id, "%08X%08X%08X", cid[0], cid[1], cid[2]);

    sprintf(emdr,"%dG+%dG",emmc_sz_GB,ddr_sz_GB);

    printk("#### %s | %d %s ####\n",__func__,__LINE__,emdr);

    printk("#### %s | %d %s %d ####",__func__,__LINE__,emmc_id,ddr_sz_GB);

    while(emmc_tab[index][0] != NULL) {
        if(!strncmp(emmc_tab[index][0], emmc_id, 18)) {

		    found = true;

	        if(!strncmp("8F0100555935435330",emmc_id,18) && (2 == ddr_sz_GB)){
                sprintf(eMMC_module_name, "%s:%s:%s:%s","UNPVN5G4CACA4BS","UNIC",emdr,"AMD0000704C1");
		        break;
	        }
	        if(!strncmp("8F0100555935435330",emmc_id,18) && (3 == ddr_sz_GB)){
                sprintf(eMMC_module_name, "%s:%s:%s:%s","UNPVN5GACACA4BS","UNIC",emdr,"AMD0000664C1");
		        break;
	        }
	        if(!strncmp("8F0100555936435330",emmc_id,18) && (3 == ddr_sz_GB)){
                sprintf(eMMC_module_name, "%s:%s:%s:%s","UNPVN6GACACA4CS","UNIC",emdr,"AMD0000XXXXX");
		        break;
	        }
	        if(!strncmp("8F0100555936435330",emmc_id,18) && (4 == ddr_sz_GB)){
                sprintf(eMMC_module_name, "%s:%s:%s:%s","UNPVN6G5CACA4CS","UNIC",emdr,"AMD0000663C1");
		        break;
	        }
		//begin add by linsheng.liu for task 10446482 deviceinfo on 2020/12/9
		#ifdef CONFIG_WRIGHTPRO_DEVICEINFO
	        if(!strncmp("F40122426977696E20",emmc_id,18) && (1 == ddr_sz_GB)){
                sprintf(eMMC_module_name, "%s:%s:%s:%s","BWCTAK411G32G","BIWIN",emdr,"AMD0000689C1");
		        break;
	        }
	        if(!strncmp("F40122426977696E20",emmc_id,18) && (2 == ddr_sz_GB)){
                sprintf(eMMC_module_name, "%s:%s:%s:%s","BWCTAK411G32G","BIWIN",emdr,"AMD0000689C1");
		        break;
	        }
	        if(!strncmp("880103534C44333247",emmc_id,18) && (1 == ddr_sz_GB)){
                sprintf(eMMC_module_name, "%s:%s:%s:%s","NCEMASLD-32G","LONGSYS",emdr,"AMD0000653C1");
		        break;
	        }	
	        if(!strncmp("880103534C44333247",emmc_id,18) && (2 == ddr_sz_GB)){
                sprintf(eMMC_module_name, "%s:%s:%s:%s","NCEMASLD-32G","LONGSYS",emdr,"AMD0000653C1");
		        break;
	        }				
		#endif
		//end add by linsheng.liu for task 10446482 deviceinfo on 2020/12/9
            sprintf(eMMC_module_name, "%s:%s:%s:%s",emmc_tab[index][1],emmc_tab[index][2],emdr,emmc_tab[index][4]);
	        break;
        }

        index++;
    }

    if(false == found)
    {
        sprintf(eMMC_module_name,"%s:NULL:%s:NULL",emmc_id,emdr);
    }
}

EXPORT_SYMBOL(set_emmc_devinfo);

/* Begin added by hailong.chen for task 9330410 on 2020-04-27 */
extern char CPU_module_name[256];

static u8 *cpu_tab[][5] = {
    {"MT6762V/WD", "MT6762V/WDA", "MTK", "NULL", "AMA0001126C1"},
    {"MT6762V/WB", "MT6762V/WB", "MTK", "NULL", "AMA0001080C1"},
    {"MT8766B", "MT8766V/WB", "MTK", "NULL", "AMA0001131C1"},
    {"MT6763V/V", "MT6763V/V", "MTK", "NULL", "AMA0001116C1"},
    {"MT6739WW", "MT6739V/WWZA", "MTK", "NULL", "AMA0001057C1"},
    {"MT6739WA", "MT6739WA", "MTK", "NULL", "AMA0001068C1"},
    /* Begin added by hailong.chen for task 9801062 on 2020-08-25 */
    {"MT8768WE", "MT8768V/WE", "MTK", "NULL", "AMA0001143C1"},
    /* End added by hailong.chen for task 9801062 on 2020-08-25 */
    {NULL,NULL,NULL,NULL,NULL},
};

void set_cpu_devinfo(const char *name)
{
    int index = 0;

    while(cpu_tab[index][0] != NULL) {
        if(!strncmp(cpu_tab[index][0], name, strlen(name))) {
            sprintf(CPU_module_name, "%s:%s:%s:%s",
                        cpu_tab[index][1],
                        cpu_tab[index][2],
                        cpu_tab[index][3],
                        cpu_tab[index][4]);
        }
        index++;
    }
}
EXPORT_SYMBOL(set_cpu_devinfo);
/* End added by hailong.chen for task 9330410 on 2020-04-27 */

static ssize_t emmc_dev_info_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    set_emmc_devinfo(pcid,cap_secs);

    return sprintf(buf,"%s\n",eMMC_module_name);
}

static DEVICE_ATTR(eMMC,S_IRUGO,emmc_dev_info_show,NULL);

static int create_emmc_info_node(void)
{
        struct device *emmc_dev;

        emmc_dev = get_deviceinfo_dev();

        if(device_create_file(emmc_dev, &dev_attr_eMMC) < 0)
        {
                printk("Failed to create ctp device_info node file.\n");
        }

        return 0;
}

static void destroy_emmc_info_node(void)
{
        struct device *emmc_dev;

        emmc_dev = get_deviceinfo_dev();

        device_remove_file(emmc_dev, &dev_attr_eMMC);
}

static int get_dev_info_emmc_init(void)
{
        create_emmc_info_node();

        return 0;
}

static void get_dev_info_emmc_exit(void)
{
        destroy_emmc_info_node();
}

late_initcall(get_dev_info_emmc_init);
module_exit(get_dev_info_emmc_exit);
MODULE_LICENSE("GPL");

