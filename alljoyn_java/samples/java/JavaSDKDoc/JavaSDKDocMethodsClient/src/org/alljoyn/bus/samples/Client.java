/*
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
*/
package org.alljoyn.bus.samples;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;

public class Client {
    static {
        System.loadLibrary("alljoyn_java");
    }
    private static final short CONTACT_PORT=42;
    static BusAttachment mBus;

    private static ProxyBusObject mProxyObj;
    private static SampleInterface mSampleInterface;

    private static SampleOnJoinSessionListener mOnJoined;

    static class MyBusListener extends BusListener {
        public void foundAdvertisedName(String name, short transport, String namePrefix) {
            System.out.println(String.format("BusListener.foundAdvertisedName(%s, %d, %s)", name, transport, namePrefix));

            if (!mOnJoined.acquire()) return;

            if (mOnJoined.isConnected()) {
                mOnJoined.release();
                return;
            }

            Status status = mBus.joinSession(name,
                    CONTACT_PORT,
                    new SessionOpts(),
                    new PrintSessionListener(),
                    mOnJoined,
                    null);

            if (status != Status.OK) {
                mOnJoined.release();
                System.out.println("BusAttachment.joinSession call failed " + status);
            }
        }
        public void nameOwnerChanged(String busName, String previousOwner, String newOwner){
            if ("com.my.well.known.name".equals(busName)) {
                System.out.println("BusAttachment.nameOwnerChagned(" + busName + ", " + previousOwner + ", " + newOwner);
            }
        }

    }

    private static class MyRunnable implements Runnable {
        private int mThreadNumber;

        MyRunnable(int n) {
            mThreadNumber = n;
        }

        public void run() {
            try {
                System.out.println("Thread " + mThreadNumber + ": Starting callculate P1");
                System.out.println("Thread " + mThreadNumber + ": Pi(1000000000) = " + mSampleInterface.Pi(1000000000));
            } catch (BusException e1) {
                e1.printStackTrace();
            }
        }
    }

    public static void main(String[] args) {
        mBus = new BusAttachment("AppName", BusAttachment.RemoteMessage.Receive);
        mOnJoined = new SampleOnJoinSessionListener();

        BusListener listener = new MyBusListener();
        mBus.registerBusListener(listener);

        Status status = mBus.connect();
        if (status != Status.OK) {
            return;
        }

        System.out.println("BusAttachment.connect successful on " + System.getProperty("org.alljoyn.bus.address"));

        status = mBus.findAdvertisedName("com.my.well.known.name");
        if (status != Status.OK) {
            return;
        }
        System.out.println("BusAttachment.findAdvertisedName successful " + "com.my.well.known.name");

        while(!mOnJoined.isConnected()) {
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
                System.out.println("Program interupted");
            }
        }

        mProxyObj = mBus.getProxyBusObject("com.my.well.known.name",
                "/myService",
                mOnJoined.getSessionId(),
                new Class<?>[] {SampleInterface.class});

        mSampleInterface = mProxyObj.getInterface(SampleInterface.class);

        try {
            System.out.println("Ping : " + mSampleInterface.Ping("Hello World"));
            System.out.println("Concatenate : " + mSampleInterface.Concatenate("The Eagle ", "has landed!"));
            System.out.println("Fibonacci(4) : " + mSampleInterface.Fibonacci(4));
        } catch (BusException e1) {
            e1.printStackTrace();
        }

        Thread thread1 = new Thread(new MyRunnable(1));
        Thread thread2 = new Thread(new MyRunnable(2));

        thread1.start();
        thread2.start();

        try {
            thread2.join();
            thread1.join();
        } catch (InterruptedException ex) {
            /*
             * we don't expect an InterrupdedExpection however just incase print
             * a stack trace to aid with debugging.
             */
            ex.printStackTrace();
        }

    }
}

