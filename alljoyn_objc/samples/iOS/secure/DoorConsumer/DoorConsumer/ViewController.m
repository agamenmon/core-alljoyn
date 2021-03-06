////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
//    Project (AJOSP) Contributors and others.
//    
//    SPDX-License-Identifier: Apache-2.0
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//    
//    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
//    Alliance. All rights reserved.
//    
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//    
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//    PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import "ViewController.h"

@interface ViewController ()

@property (nonatomic, strong) AppDelegate *appDelegate;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    _appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
}

- (void)didReceiveAllJoynStatusMessage:(NSString *)message {
    [self didReceiveStatusUpdateMessage:message];
}

- (void)didReceiveStatusUpdateMessage:(NSString *)message
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSMutableString *string = self.textView.text.length ? [self.textView.text mutableCopy] : [[NSMutableString alloc] init];
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        [formatter setTimeStyle:NSDateFormatterMediumStyle];
        [formatter setDateStyle:NSDateFormatterShortStyle];
        [string appendFormat:@"[%@] ",[formatter stringFromDate:[NSDate date]]];
        [string appendString:message];
        [self.textView setText:string];
        NSLog(@"%@",message);
    });
}

- (IBAction)didTouchOpenButton:(id)sender {
    [_appDelegate.doorConsumerAllJoynService openDoors];
}

- (IBAction)didTouchCloseButton:(id)sender {
    [_appDelegate.doorConsumerAllJoynService closeDoors];
}

- (IBAction)didTouchGetStateButton:(id)sender {
    [_appDelegate.doorConsumerAllJoynService getDoorsState];
}

- (IBAction)didTouchGetStatePropertyButton:(id)sender {
    [_appDelegate.doorConsumerAllJoynService getDoorsStateProperty];
}

@end
