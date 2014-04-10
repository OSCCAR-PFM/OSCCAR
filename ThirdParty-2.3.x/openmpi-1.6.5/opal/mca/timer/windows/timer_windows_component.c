/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "opal_config.h"

#include "opal/mca/timer/timer.h"
#include "opal/mca/timer/windows/timer_windows_component.h"
#include "opal/constants.h"

opal_timer_t opal_timer_windows_freq;
opal_timer_t opal_timer_windows_start;

static int opal_timer_windows_open(void);

const
opal_timer_base_component_2_0_0_t mca_timer_windows_component = {
    /* First, the mca_component_t struct containing meta information
       about the component itself */
    {
        OPAL_TIMER_BASE_VERSION_2_0_0,

        /* Component name and version */
        "windows",
        OPAL_MAJOR_VERSION,
        OPAL_MINOR_VERSION,
        OPAL_RELEASE_VERSION,

        /* Component open and close functions */
        opal_timer_windows_open,
        NULL
    },
    {
        /* The component is checkpoint ready */
        MCA_BASE_METADATA_PARAM_CHECKPOINT
    },
};

int
opal_timer_windows_open(void)
{
    LARGE_INTEGER now;

    if( 0 != QueryPerformanceFrequency( &now ) ) {
        opal_timer_windows_freq = now.QuadPart;
        QueryPerformanceCounter( &now );
        opal_timer_windows_start = now.QuadPart;
        return OPAL_SUCCESS;
    }
    return OPAL_SUCCESS;
}
