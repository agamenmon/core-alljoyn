/**
 * @file
 * @brief  Sample implementation of an AllJoyn client. That has an implmentation
 * a secure client that is using a shared keystore file.
 */

/******************************************************************************
 *
 *
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
#include <qcc/platform.h>

#include <signal.h>
#include <stdio.h>
#include <vector>

#include <qcc/Mutex.h>
#include <qcc/String.h>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/Init.h>
#include <alljoyn/Status.h>
#include <alljoyn/version.h>
#include <alljoyn/KeyStoreListener.h>

using namespace std;
using namespace qcc;
using namespace ajn;

/*
 * This creates instance of CustomKeyStoreListener
 */
extern KeyStoreListener* CreateKeyStoreListenerInstance(const char* fname);

/** Static top level message bus object */
static BusAttachment* s_msgBus = nullptr;

static KeyStoreListener* keyStoreListener = nullptr;
static AuthListener* authListener = nullptr;

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.bus.samples.secure.SecureInterface";
static const char* SERVICE_NAME = "org.alljoyn.bus.samples.secure";
static const char* SERVICE_PATH = "/SecureService";
static const SessionPort SERVICE_PORT = 42;

static bool s_joinComplete = false;
static String s_sessionHost;
static SessionId s_sessionId = 0;
static qcc::Mutex* s_sessionLock = nullptr;

static volatile sig_atomic_t s_interrupt = false;

static void CDECL_CALL SigIntHandler(int sig)
{
    QCC_UNUSED(sig);
    s_interrupt = true;
}

/*
 * get a line of input from the the file pointer (most likely stdin).
 * This will capture the the num-1 characters or till a newline character is
 * entered.
 *
 * @param[out] str a pointer to a character array that will hold the user input
 * @param[in]  num the size of the character array 'str'
 * @param[in]  fp  the file pointer the sting will be read from. (most likely stdin)
 *
 * @return returns the same string as 'str' if there has been a read error a null
 *                 pointer will be returned and 'str' will remain unchanged.
 */
char*get_line(char*str, size_t num, FILE*fp)
{
    char*p = fgets(str, num, fp);

    // fgets will capture the '\n' character if the string entered is shorter than
    // num. Remove the '\n' from the end of the line and replace it with nul '\0'.
    if (p != nullptr) {
        size_t last = strlen(str) - 1;
        if (str[last] == '\n') {
            str[last] = '\0';
        }
    }
    return p;
}

/** Inform the app thread that JoinSession is complete, store the session ID. */
class MyJoinCallback : public BusAttachment::JoinSessionAsyncCB {
    void JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context) {
        QCC_UNUSED(opts);
        QCC_UNUSED(context);

        if (ER_OK == status) {
            printf("JoinSession SUCCESS (Session id=%u).\n", sessionId);
            s_sessionLock->Lock(MUTEX_CONTEXT);
            s_sessionId = sessionId;
            s_joinComplete = true;
            s_sessionLock->Unlock(MUTEX_CONTEXT);
        } else {
            printf("JoinSession failed (status=%s).\n", QCC_StatusText(status));
        }
    }
};

/** AllJoynListener receives discovery events from AllJoyn */
class MyBusListener : public BusListener, public SessionListener {
  public:
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        printf("FoundAdvertisedName(name='%s', transport = 0x%x, prefix='%s')\n", name, transport, namePrefix);
        s_sessionLock->Lock(MUTEX_CONTEXT);
        if (0 == strcmp(name, SERVICE_NAME) && s_sessionHost.empty()) {
            s_sessionHost = name;
            s_sessionLock->Unlock(MUTEX_CONTEXT);
            /* We found a remote bus that is advertising basic service's  well-known name so connect to it */
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            QStatus status = s_msgBus->JoinSessionAsync(name, SERVICE_PORT, this, opts, &joinCb);
            if (ER_OK != status) {
                printf("JoinSessionAsync failed (status=%s)", QCC_StatusText(status));
            }
        } else {
            s_sessionLock->Unlock(MUTEX_CONTEXT);
        }
    }

    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        if (newOwner && (0 == strcmp(busName, SERVICE_NAME))) {
            printf("NameOwnerChanged: name=%s, oldOwner=%s, newOwner=%s\n",
                   busName,
                   previousOwner ? previousOwner : "<none>",
                   newOwner ? newOwner : "<none>");
        }
    }

  private:
    MyJoinCallback joinCb;
};

/*
 * This is the local implementation of the an AuthListener.  SrpKeyXListener is
 * designed to only handle SRP Key Exchange Authentication requests.
 *
 * When a Password request (CRED_PASSWORD) comes in using ALLJOYN_SRP_KEYX the
 * code will ask the user to enter the pin code that was generated by the
 * service. The pin code must match the service's pin code for Authentication to
 * be successful.
 *
 * If any other authMechanism is used other than SRP Key Exchange authentication
 * will fail.
 */
class SrpKeyXListener : public AuthListener {
    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds) {
        QCC_UNUSED(userId);

        printf("RequestCredentials for authenticating %s using mechanism %s\n", authPeer, authMechanism);
        if (strcmp(authMechanism, "ALLJOYN_SRP_KEYX") == 0) {
            if (credMask & AuthListener::CRED_PASSWORD) {
                if (authCount <= 3) {
                    /* Take input from stdin and send it as a chat messages */
                    printf("Please enter one time password : ");
                    const int bufSize = 7;
                    char buf[bufSize];
                    get_line(buf, bufSize, stdin);
                    creds.SetPassword(buf);
                    return true;
                } else {
                    return false;
                }
            }
        }
        return false;
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        QCC_UNUSED(authPeer);

        printf("Authentication %s %s\n", authMechanism, success ? "successful" : "failed");
    }
};

/** Static bus listener */
static MyBusListener g_busListener;
static char clientName[] = "Client%u";

void MakeClientName(void)
{
#ifndef CLIENT
#define CLIENT 0
#endif

    int client = CLIENT;

    // Prevent overwriting the client name buffer.
    if (client < 0 || client > 99) {
        client = 0;
    }

    sprintf(clientName, clientName, client);
}

/** Create the interface, report the result to stdout, and return the result status. */
QStatus CreateInterface(void)
{
    /* Add org.alljoyn.Bus.method_sample interface */
    InterfaceDescription* testIntf = nullptr;
    QStatus status = s_msgBus->CreateInterface(INTERFACE_NAME, testIntf, AJ_IFC_SECURITY_REQUIRED);

    if (status == ER_OK) {
        printf("Interface '%s' created.\n", INTERFACE_NAME);
        testIntf->AddMethod("Ping", "s",  "s", "inStr,outStr", 0);
        testIntf->Activate();
    } else {
        printf("Failed to create interface '%s'.\n", INTERFACE_NAME);
    }

    return status;
}

/** Start the message bus, report the result to stdout, and return the result status. */
QStatus StartMessageBus(void)
{
    QStatus status = s_msgBus->Start();

    if (ER_OK == status) {
        printf("BusAttachment started.\n");
    } else {
        printf("BusAttachment::Start failed.\n");
    }

    return status;
}

/** Enable security, report the result to stdout, and return the result status. */
QStatus EnableSecurity()
{
    /*
     * Registering custom KeyStoreListener instance
     */
    QCC_ASSERT(keyStoreListener == nullptr);
    keyStoreListener = CreateKeyStoreListenerInstance("/.alljoyn_keystore/central.ks");
    QStatus status = s_msgBus->RegisterKeyStoreListener(*keyStoreListener);

    if (ER_OK == status) {
        printf("BusAttachment::RegisterKeyStoreListener successful.\n");
    } else {
        printf("BusAttachment::RegisterKeyStoreListener failed (%s).\n", QCC_StatusText(status));
    }

    QCC_ASSERT(authListener == nullptr);
    authListener = new SrpKeyXListener();
    status = s_msgBus->EnablePeerSecurity("ALLJOYN_SRP_KEYX", authListener);

    if (ER_OK == status) {
        printf("BusAttachment::EnablePeerSecurity successful.\n");
    } else {
        printf("BusAttachment::EnablePeerSecurity failed (%s).\n", QCC_StatusText(status));
    }

    return status;
}

/** Handle the connection to the bus, report the result to stdout, and return the result status. */
QStatus ConnectToBus(void)
{
    QStatus status = s_msgBus->Connect();

    if (ER_OK == status) {
        printf("BusAttachment connected to '%s'.\n", s_msgBus->GetConnectSpec().c_str());
    } else {
        printf("BusAttachment::Connect('%s') failed.\n", s_msgBus->GetConnectSpec().c_str());
    }

    return status;
}

/** Register a bus listener in order to get discovery indications and report the event to stdout. */
void RegisterBusListener(void)
{
    /* Static bus listener */
    static MyBusListener s_busListener;

    s_msgBus->RegisterBusListener(s_busListener);
    printf("BusListener Registered.\n");
}

/** Begin discovery on the well-known name of the service to be called, report the result to
   stdout, and return the result status. */
QStatus FindAdvertisedName(void)
{
    /* Begin discovery on the well-known name of the service to be called */
    QStatus status = s_msgBus->FindAdvertisedName(SERVICE_NAME);

    if (status == ER_OK) {
        printf("org.alljoyn.Bus.FindAdvertisedName ('%s') succeeded.\n", SERVICE_NAME);
    } else {
        printf("org.alljoyn.Bus.FindAdvertisedName ('%s') failed (%s).\n", SERVICE_NAME, QCC_StatusText(status));
    }

    return status;
}

/** Wait for join session to complete, report the event to stdout, and return the result status. */
QStatus WaitForJoinSessionCompletion(void)
{
    unsigned int count = 0;

    while (!s_joinComplete && !s_interrupt) {
        if (0 == (count++ % 10)) {
            printf("Waited %u seconds for JoinSession completion.\n", count / 10);
        }

#ifdef _WIN32
        Sleep(100);
#else
        usleep(100 * 1000);
#endif
    }

    return s_joinComplete && !s_interrupt ? ER_OK : ER_ALLJOYN_JOINSESSION_REPLY_CONNECT_FAILED;
}

/** Do a method call, report the result to stdout, and return the result status. */
QStatus MakeMethodCall(void)
{
    s_sessionLock->Lock(MUTEX_CONTEXT);
    ProxyBusObject remoteObj(*s_msgBus, SERVICE_NAME, SERVICE_PATH, s_sessionId);
    s_sessionLock->Unlock(MUTEX_CONTEXT);
    const InterfaceDescription* alljoynTestIntf = s_msgBus->GetInterface(INTERFACE_NAME);

    QCC_ASSERT(alljoynTestIntf);
    remoteObj.AddInterface(*alljoynTestIntf);

    /* The method call below specifies a small timeout value. Avoid timing out
     * during the method call by prompting the user for a password here, instead
     * of prompting the user during the method call.
     */
    QStatus status = remoteObj.SecureConnection(true);

    if (ER_OK != status) {
        printf("SecureConnection failed.\n");
    } else {
        Message reply(*s_msgBus);
        MsgArg inputs[1];
        char buffer[80];

        sprintf(buffer, "%s says Hello AllJoyn!", clientName);

        inputs[0].Set("s", buffer);

        status = remoteObj.MethodCall(INTERFACE_NAME, "Ping", inputs, 1, reply, 5000);

        if (ER_OK == status) {
            printf("%s.Ping (path=%s) returned \"%s\".\n", INTERFACE_NAME,
                   SERVICE_PATH, reply->GetArg(0)->v_string.str);
        } else {
            printf("MethodCall on %s.Ping failed.\n", INTERFACE_NAME);
        }
    }

    return status;
}

/** Main entry point */
int CDECL_CALL main(int argc, char** argv, char** envArg)
{
    QCC_UNUSED(argc);
    QCC_UNUSED(argv);
    QCC_UNUSED(envArg);

    if (AllJoynInit() != ER_OK) {
        return 1;
    }
#ifdef ROUTER
    if (AllJoynRouterInit() != ER_OK) {
        AllJoynShutdown();
        return 1;
    }
#endif

    printf("AllJoyn Library version: %s.\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s.\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    MakeClientName();

    QStatus status = ER_OK;

    /* Create the application name. */
    char buffer[40];

    sprintf(buffer, "SRPSecurity%s", clientName);

    /* Create message bus */
    s_msgBus = new BusAttachment(buffer, true);
    s_sessionLock = new qcc::Mutex();

    /* This test for nullptr is only required if new() behavior is to return nullptr
     * instead of throwing an exception upon an out of memory failure.
     */
    if ((s_msgBus == nullptr) || (s_sessionLock == nullptr)) {
        status = ER_OUT_OF_MEMORY;
    }

    if (ER_OK == status) {
        status = CreateInterface();
    }

    if (ER_OK == status) {
        status = StartMessageBus();
    }

    if (ER_OK == status) {
        status = EnableSecurity();
    }

    if (ER_OK == status) {
        status = ConnectToBus();
    }

    if (ER_OK == status) {
        RegisterBusListener();
        status = FindAdvertisedName();
    }

    if (ER_OK == status) {
        status = WaitForJoinSessionCompletion();
    }

    if (ER_OK == status) {
        status = MakeMethodCall();
    }

    /* Deallocate bus */
    delete s_msgBus;
    delete s_sessionLock;
    delete keyStoreListener;
    delete authListener;

    printf("Basic client exiting with status 0x%04x (%s).\n", status, QCC_StatusText(status));

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif
    AllJoynShutdown();
    return (int) status;
}
