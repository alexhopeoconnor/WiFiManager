/**
 * RootSelector.h
 * Allows overriding the root template header via build flag
 * Define WM_CUSTOM_ROOT_TEMPLATE_HEADER to a quoted header path.
 */
#ifndef _WM_ROOT_SELECTOR_H_
#define _WM_ROOT_SELECTOR_H_

#ifdef WM_CUSTOM_ROOT_TEMPLATE_HEADER
#include WM_CUSTOM_ROOT_TEMPLATE_HEADER
#else
#include "RootShell.h"
#endif

#endif // _WM_ROOT_SELECTOR_H_


