// Copyright (c) 2009 - Mozy, Inc.

#include "auth.h"

#include "basic.h"
#include "client.h"
#include "digest.h"
#include "mordor/socket.h"

#ifdef WINDOWS
#include <memory>
#include "negotiate.h"
#elif defined (OSX)
#include "mordor/util.h"

#include <Security/SecItem.h>
#include <Security/SecKeychain.h>
#include <Security/SecKeychainItem.h>
#include <Security/SecKeychainSearch.h>
#endif

namespace Mordor {
namespace HTTP {

static void authorize(const ChallengeList *challenge, AuthParams &authorization,
    const URI &uri, const std::string &method, const std::string &scheme,
    const std::string &realm, const std::string &username,
    const std::string &password)
{
    if (stricmp(scheme.c_str(), "Basic") == 0) {
        BasicAuth::authorize(authorization, username, password);
    } else if (stricmp(scheme.c_str(), "Digest") == 0) {
        MORDOR_ASSERT(challenge);
        DigestAuth::authorize(
            challengeForSchemeAndRealm(*challenge, "Digest", realm),
            authorization, uri, method, username, password);
    }
}

ClientRequest::ptr
AuthRequestBroker::request(Request &requestHeaders, bool forceNewConnection,
    boost::function<void (ClientRequest::ptr)> bodyDg)
{
    ClientRequest::ptr priorRequest;
    std::string scheme, realm, username, password;
    size_t attempts = 0, proxyAttempts = 0;
#ifdef WINDOWS
    std::unique_ptr<NegotiateAuth> negotiateAuth, negotiateProxyAuth;
#endif
    while (true) {
#ifdef WINDOWS
        // Reset Negotiate auth sequence if the server didn't continue the
        // handshake
        if (negotiateProxyAuth) {
            const ChallengeList &challenge =
                priorRequest->response().response.proxyAuthenticate;
            if (!isAcceptable(challenge, scheme) ||
                !negotiateProxyAuth->authorize(
                    challengeForSchemeAndRealm(challenge, scheme),
                    requestHeaders.request.proxyAuthorization,
                    requestHeaders.requestLine.uri))
                negotiateProxyAuth.reset();
        }
        if (negotiateAuth) {
            const ChallengeList &challenge =
                priorRequest->response().response.wwwAuthenticate;
            if (!isAcceptable(challenge, scheme) ||
                !negotiateAuth->authorize(
                    challengeForSchemeAndRealm(challenge, scheme),
                    requestHeaders.request.authorization,
                    requestHeaders.requestLine.uri))
                negotiateAuth.reset();
        }
        // Negotiate auth is a multi-request transaction; if we're in the
        // middle of one, just do the next step, and skip asking for
        // credentials again
        if (!negotiateAuth && !negotiateProxyAuth) {
#endif
            // If this is the first try, or the last one failed UNAUTHORIZED,
            // ask for credentials, and use them if we got them
            if ((!priorRequest ||
                priorRequest->response().status.status == UNAUTHORIZED) &&
                m_getCredentialsDg &&
                m_getCredentialsDg(requestHeaders.requestLine.uri, priorRequest,
                scheme, realm, username, password, attempts++)) {
#ifdef WINDOWS
                MORDOR_ASSERT(
                    stricmp(scheme.c_str(), "Negotiate") == 0 ||
                    stricmp(scheme.c_str(), "NTLM") == 0 ||
                    stricmp(scheme.c_str(), "Digest") == 0 ||
                    stricmp(scheme.c_str(), "Basic") == 0);
#else
                    MORDOR_ASSERT(
                    stricmp(scheme.c_str(), "Digest") == 0 ||
                    stricmp(scheme.c_str(), "Basic") == 0);
#endif
#ifdef WINDOWS
                if (scheme == "Negotiate" || scheme == "NTLM") {
                    negotiateAuth.reset(new NegotiateAuth(username, password));
                    negotiateAuth->authorize(
                        challengeForSchemeAndRealm(priorRequest->response().response.wwwAuthenticate, scheme),
                        requestHeaders.request.authorization,
                        requestHeaders.requestLine.uri);
                } else
#endif
                authorize(priorRequest ?
                    &priorRequest->response().response.wwwAuthenticate : NULL,
                    requestHeaders.request.authorization,
                    requestHeaders.requestLine.uri,
                    requestHeaders.requestLine.method,
                    scheme, realm, username, password);
            } else if (priorRequest &&
                priorRequest->response().status.status == UNAUTHORIZED) {
                // caller didn't want to retry
                return priorRequest;
            }
            // If this is the first try, or the last one failed (for a proxy)
            // ask for credentials, and use them if we got them
            if ((!priorRequest ||
                priorRequest->response().status.status ==
                    PROXY_AUTHENTICATION_REQUIRED) &&
                m_getProxyCredentialsDg &&
                m_getProxyCredentialsDg(requestHeaders.requestLine.uri,
                priorRequest, scheme, realm, username, password, proxyAttempts++)) {
#ifdef WINDOWS
                MORDOR_ASSERT(
                    stricmp(scheme.c_str(), "Negotiate") == 0 ||
                    stricmp(scheme.c_str(), "NTLM") == 0 ||
                    stricmp(scheme.c_str(), "Digest") == 0 ||
                    stricmp(scheme.c_str(), "Basic") == 0);
#else
                    MORDOR_ASSERT(
                    stricmp(scheme.c_str(), "Digest") == 0 ||
                    stricmp(scheme.c_str(), "Basic") == 0);
#endif
#ifdef WINDOWS
                if (scheme == "Negotiate" || scheme == "NTLM") {
                    negotiateProxyAuth.reset(new NegotiateAuth(username, password));
                    negotiateProxyAuth->authorize(
                        challengeForSchemeAndRealm(priorRequest->response().response.proxyAuthenticate, scheme),
                        requestHeaders.request.proxyAuthorization,
                        requestHeaders.requestLine.uri);
                } else
#endif
                authorize(priorRequest ?
                    &priorRequest->response().response.proxyAuthenticate : NULL,
                    requestHeaders.request.proxyAuthorization,
                    requestHeaders.requestLine.uri,
                    requestHeaders.requestLine.method,
                    scheme, realm, username, password);
            } else if (priorRequest &&
                priorRequest->response().status.status ==
                    PROXY_AUTHENTICATION_REQUIRED) {
                // Caller didn't want to retry
                return priorRequest;
            }
#ifdef WINDOWS
        }
#endif
        if (priorRequest) {
            priorRequest->finish();
        } else {
            // We're passed our pre-emptive authentication, regardless of what
            // actually happened
            attempts = 1;
            proxyAttempts = 1;
        }
        priorRequest = parent()->request(requestHeaders, forceNewConnection,
            bodyDg);
        const ChallengeList *challengeList = NULL;
        if (priorRequest->response().status.status == UNAUTHORIZED)
            challengeList = &priorRequest->response().response.wwwAuthenticate;
        if (priorRequest->response().status.status == PROXY_AUTHENTICATION_REQUIRED)
            challengeList = &priorRequest->response().response.proxyAuthenticate;
        if (challengeList &&
            (isAcceptable(*challengeList, "Basic") ||
            isAcceptable(*challengeList, "Digest")
#ifdef WINDOWS
            || isAcceptable(*challengeList, "Negotiate") ||
            isAcceptable(*challengeList, "NTLM")
#endif
            ))
            continue;
        return priorRequest;
    }
}

#ifdef OSX
bool getCredentialsFromKeychain(const URI &uri, ClientRequest::ptr priorRequest,
    std::string &scheme, std::string &realm, std::string &username,
    std::string &password, size_t attempts)
{
    if (attempts != 1)
        return false;
    bool proxy =
       priorRequest->response().status.status == PROXY_AUTHENTICATION_REQUIRED;
    const ChallengeList &challengeList = proxy ?
        priorRequest->response().response.proxyAuthenticate :
        priorRequest->response().response.wwwAuthenticate;
    if (isAcceptable(challengeList, "Basic"))
        scheme = "Basic";
    else if (isAcceptable(challengeList, "Digest"))
        scheme = "Digest";
    else
        return false;

    ScopedCFRef<CFMutableDictionaryRef> query = CFDictionaryCreateMutable(NULL, 0, NULL, NULL);
    const std::string host = uri.authority.host();
    CFDictionaryAddValue(query, kSecClass, kSecClassInternetPassword);
    ScopedCFRef<CFStringRef> server = CFStringCreateWithCStringNoCopy(NULL, host.c_str(), kCFStringEncodingUTF8, kCFAllocatorNull);
    CFDictionaryAddValue(query, kSecAttrServer, server);


    if (uri.authority.portDefined()) {
        int port = uri.authority.port();
        ScopedCFRef<CFNumberRef> cfPort = CFNumberCreate(NULL, kCFNumberIntType, &port);
        CFDictionaryAddValue(query, kSecAttrPort, cfPort);
    }
    CFTypeRef protocol = NULL;
    if (proxy && priorRequest->request().requestLine.method == CONNECT)
        protocol = kSecAttrProtocolHTTPSProxy;
    else if (proxy)
        protocol = kSecAttrProtocolHTTPProxy;
    else if (uri.scheme() == "https")
        protocol = kSecAttrProtocolHTTPS;
    else if (uri.scheme() == "http")
        protocol = kSecAttrProtocolHTTP;
    else
        MORDOR_NOTREACHED();
    CFDictionaryAddValue(query, kSecAttrProtocol, protocol);
    CFDictionaryAddValue(query, kSecReturnRef, kCFBooleanTrue);
    ScopedCFRef<SecKeychainItemRef> item;
    OSStatus status = SecItemCopyMatching(query, (CFTypeRef*)&item);
    if (status != errSecSuccess)
        return false;
    // SecItemCopyMatching() just returns one record by default.
    MORDOR_ASSERT(CFGetTypeID(item) == SecKeychainItemGetTypeID());
    SecKeychainAttributeInfo info;
    SecKeychainAttrType tag = kSecAccountItemAttr;
    CSSM_DB_ATTRIBUTE_FORMAT format = CSSM_DB_ATTRIBUTE_FORMAT_STRING;
    info.count = 1;
    info.tag = (UInt32 *)&tag;
    info.format = (UInt32 *)&format;
    
    SecKeychainAttributeList *attrs = NULL;
    UInt32 passwordLength = 0;
    void *passwordBytes = NULL;

    status = SecKeychainItemCopyAttributesAndData(item, &info, NULL, &attrs,
        &passwordLength, &passwordBytes);
    if (status != errSecSuccess)
        return false;
    try {
        username.assign((const char *)attrs->attr[0].data, attrs->attr[0].length);
        password.assign((const char *)passwordBytes, passwordLength);
    } catch (...) {
        SecKeychainItemFreeAttributesAndData(attrs, passwordBytes);
        throw;
    }
    SecKeychainItemFreeAttributesAndData(attrs, passwordBytes);
    return true;
}
#endif
	
}}
