/*****************************************************************************/
/**
 * @file    geCpuid.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/11/23
 * @brief   This file's purpose is to serve as an include guard for cpuid.h.
 *
 * This file's only purpose is to serve as an include guard for cpuid.h by
 * checking for two of it's internal macros.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/
#pragma once

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#if !(defined(__cpuid) && defined(__cpuid_count))
# include "cpuid.h"
#endif
