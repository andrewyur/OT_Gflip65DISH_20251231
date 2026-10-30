

#ifndef __KREE_MEM_H__
#define __KREE_MEM_H__

#if defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT) || defined(CONFIG_TRUSTY)

#include "tz_cross/trustzone.h"
#include "tz_cross/ree_service.h"
#include "kree/system.h"

#define KREE_SESSION_HANDLE_NULL    ((KREE_SESSION_HANDLE)0)
#define KREE_SESSION_HANDLE_FAIL    ((KREE_SESSION_HANDLE)-1)


/* Secure memory handle define */
typedef uint32_t KREE_SECUREMEM_HANDLE;

/* Secure chunk memory handle define */
typedef uint32_t KREE_SECURECM_HANDLE;

/* Release Secure chunk memory handle define */
typedef uint32_t KREE_RELEASECM_HANDLE;

struct KREE_SHAREDMEM_PARAM {
	void *buffer;
	uint32_t size;
};

/* map_p: 0 = no remap, 1 = remap */
int kree_register_sharedmem(KREE_SESSION_HANDLE session,
		KREE_SHAREDMEM_HANDLE *mem_handle, void *start,
		uint32_t size, void *map_p, const char *tag);

int kree_unregister_sharedmem(KREE_SESSION_HANDLE session,
					KREE_SHAREDMEM_HANDLE mem_handle);


int KREE_RegisterSharedmem(KREE_SESSION_HANDLE session,
		KREE_SHAREDMEM_HANDLE *shm_handle,
		struct KREE_SHAREDMEM_PARAM *param);


int KREE_UnregisterSharedmem(KREE_SESSION_HANDLE session,
					KREE_SHAREDMEM_HANDLE shm_handle);


int KREE_AllocSecuremem(KREE_SESSION_HANDLE session,
	KREE_SECUREMEM_HANDLE *mem_handle, uint32_t alignment, uint32_t size);

int KREE_AllocSecurememWithTag(KREE_SESSION_HANDLE session,
	KREE_SECUREMEM_HANDLE *mem_handle, uint32_t alignment, uint32_t size,
	const char *tag);

int KREE_ZallocSecurememWithTag(KREE_SESSION_HANDLE session,
	KREE_SECUREMEM_HANDLE *mem_handle, uint32_t alignment, uint32_t size,
	const char *tag);

int KREE_ReferenceSecuremem(KREE_SESSION_HANDLE session,
					KREE_SECUREMEM_HANDLE mem_handle);

int KREE_UnreferenceSecuremem(KREE_SESSION_HANDLE session,
					KREE_SECUREMEM_HANDLE mem_handle);


int KREE_AllocSecurechunkmem(KREE_SESSION_HANDLE session,
	KREE_SECURECM_HANDLE *cm_handle, uint32_t alignment, uint32_t size);

int KREE_AllocSecurechunkmemWithTag(KREE_SESSION_HANDLE session,
	KREE_SECURECM_HANDLE *cm_handle, uint32_t alignment, uint32_t size,
	const char *tag);


int KREE_ZallocSecurechunkmemWithTag(KREE_SESSION_HANDLE session,
	KREE_SECURECM_HANDLE *cm_handle, uint32_t alignment, uint32_t size,
	const char *tag);

int KREE_ReferenceSecurechunkmem(KREE_SESSION_HANDLE session,
					KREE_SECURECM_HANDLE cm_handle);

int KREE_UnreferenceSecurechunkmem(KREE_SESSION_HANDLE session,
					 KREE_SECURECM_HANDLE cm_handle);

int KREE_ReadSecurechunkmem(KREE_SESSION_HANDLE session,
				uint32_t offset, uint32_t size, void *buffer);

int KREE_WriteSecurechunkmem(KREE_SESSION_HANDLE session,
			uint32_t offset, uint32_t size, void *buffer);

int KREE_GetSecurechunkReleaseSize(KREE_SESSION_HANDLE session,
					uint32_t *size);


int KREE_StartSecurechunkmemSvc(KREE_SESSION_HANDLE session,
				unsigned long start_pa, uint32_t size);

int KREE_StopSecurechunkmemSvc(KREE_SESSION_HANDLE session,
					unsigned long *cm_pa, uint32_t *size);

int KREE_QuerySecurechunkmem(KREE_SESSION_HANDLE session,
				unsigned long *cm_pa, uint32_t *size);

#ifdef CONFIG_MTEE_CMA_SECURE_MEMORY
int KREE_ServGetChunkmemPool(u32 op,
				   u8 uparam[REE_SERVICE_BUFFER_SIZE]);

int KREE_ServReleaseChunkmemPool(u32 op,
				       u8 uparam[REE_SERVICE_BUFFER_SIZE]);
#endif  /* CONFIG_MTEE_CMA_SECURE_MEMORY */

#endif	/* CONFIG_MTK_IN_HOUSE_TEE_SUPPORT || CONFIG_TRUSTY*/
#endif	/* __KREE_MEM_H__ */
