/*
 * Copyright © 2010 inria.  All rights reserved.
 * Copyright © 2010 Université Bordeaux 1
 * Copyright © 2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

/** \file
 * \brief Macros to help interaction between hwloc and the CUDA Runtime API.
 *
 * Applications that use both hwloc and the CUDA Runtime API may want to
 * include this file so as to get topology information for CUDA devices.
 *
 */

#ifndef HWLOC_CUDART_H
#define HWLOC_CUDART_H

#include <hwloc.h>
#include <hwloc/autogen/config.h>
#include <hwloc/linux.h>

#include <cuda_runtime_api.h>


#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup hwlocality_cudart CUDA Runtime API Specific Functions
 * @{
 */

/** \brief Get the CPU set of logical processors that are physically
 * close to device \p cudevice.
 *
 * For the given CUDA Runtime API device \p cudevice, read the corresponding
 * kernel-provided cpumap file and return the corresponding CPU set.
 * This function is currently only implemented in a meaningful way for
 * Linux; other systems will simply get a full cpuset.
 */
static __hwloc_inline int
hwloc_cudart_get_device_cpuset(hwloc_topology_t topology __hwloc_attribute_unused,
			       int device, hwloc_cpuset_t set)
{
#ifdef HWLOC_LINUX_SYS
  /* If we're on Linux, use the sysfs mechanism to get the local cpus */
#define HWLOC_CUDART_DEVICE_SYSFS_PATH_MAX 128
  cudaError_t cerr;
  struct cudaDeviceProp prop;
  char path[HWLOC_CUDART_DEVICE_SYSFS_PATH_MAX];
  FILE *sysfile = NULL;
  int pciDomainID = 0;

  cerr = cudaGetDeviceProperties(&prop, device);
  if (cerr) {
    errno = ENOSYS;
    return -1;
  }

#if CUDART_VERSION >= 4000
  pciDomainID = prop.pciDomainID;
#endif

  sprintf(path, "/sys/bus/pci/devices/%04x:%02x:%02x.0/local_cpus", pciDomainID, prop.pciBusID, prop.pciDeviceID);
  sysfile = fopen(path, "r");
  if (!sysfile)
    return -1;

  hwloc_linux_parse_cpumap_file(sysfile, set);

  fclose(sysfile);
#else
  /* Non-Linux systems simply get a full cpuset */
  hwloc_bitmap_copy(set, hwloc_topology_get_complete_cpuset(topology));
#endif
  return 0;
}

/** @} */


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* HWLOC_CUDART_H */
