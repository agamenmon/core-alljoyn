/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
******************************************************************************/

#import <Foundation/Foundation.h>
#import "AJNAboutService.h"
#import "AJNAboutPropertyStoreImpl.h"

/**
 AJNAboutServiceApi is wrapper class that encapsulates the AJNAboutService using a shared instance.
 */
__deprecated
@interface AJNAboutServiceApi : AJNAboutService

/**
 Destroy the shared instance.
 */
- (void)destroyInstance __deprecated;

/**
 * Create an AboutServiceApi Shared instance.
 * @return AboutServiceApi instance(created only once).
 */
+ (id)sharedInstance __deprecated;

/**
 Start teh service using a given AJNBusAttachment and PropertyStore.
 @param bus A reference to the AJNBusAttachment.
 @param store A reference to a property store.
 */
- (void)startWithBus:(AJNBusAttachment *)bus andPropertyStore:(AJNAboutPropertyStoreImpl *)store __deprecated;

/**
 Return a reference to the property store.
 */
- (ajn ::services ::AboutPropertyStoreImpl *)getPropertyStore __deprecated;

@end
