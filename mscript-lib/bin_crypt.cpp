#include "pch.h"
#include "bin_crypt.h"
#include "utils.h"

#include <wincrypt.h>
#include <wintrust.h>
#pragma comment(lib, "crypt32.lib")

#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

mscript::bin_crypt_info mscript::getBinCryptInfo(const std::wstring & filePath)
{
    mscript::bin_crypt_info crypt_info;

    // Error handling with Win32 "objects" is fun and easy!
    std::string exp_msg;
#define BIN_CRYPT_ERR(msg) { exp_msg = (msg); goto Cleanup; }

    // Pre-declare *everything*
    DWORD dwEncoding = 0, dwContentType = 0, dwFormatType = 0, dwSignerInfo = 0, dwData = 0;
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL;
    BOOL fResult = FALSE;
    PCMSG_SIGNER_INFO pSignerInfo = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    LPTSTR szName = NULL;

    // Get the certificate from the file
    fResult =
        CryptQueryObject
        (
            CERT_QUERY_OBJECT_FILE,
            filePath.c_str(),
            CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
            CERT_QUERY_FORMAT_FLAG_BINARY,
            0,
            &dwEncoding,
            &dwContentType,
            &dwFormatType,
            &hStore,
            &hMsg,
            NULL
        );
    if (!fResult)
        return crypt_info;

    // Get signer info
    fResult = CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &dwSignerInfo);
    if (!fResult)
        BIN_CRYPT_ERR("Getting binary security signer info failed");
    pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
    if (!fResult)
        BIN_CRYPT_ERR("Allocating signer info memory failed");
    fResult = CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, pSignerInfo, &dwSignerInfo);
    if (!fResult)
        BIN_CRYPT_ERR("Getting signer info failed");

    // Get the cert from the store
    CERT_INFO CertInfo;
    CertInfo.Issuer = pSignerInfo->Issuer;
    CertInfo.SerialNumber = pSignerInfo->SerialNumber;
    pCertContext = 
        CertFindCertificateInStore
        (
            hStore,
            ENCODING,
            0,
            CERT_FIND_SUBJECT_CERT,
            (PVOID)&CertInfo,
            NULL
        );
    if (!fResult)
        BIN_CRYPT_ERR("Finding signing certificate failed");

    // Get the issuer
    dwData =
        CertGetNameString
        (
            pCertContext,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            CERT_NAME_ISSUER_FLAG,
            NULL,
            NULL,
            0
        );
    if (dwData == 0)
        BIN_CRYPT_ERR("Getting certificate issuer length failed");
    szName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(TCHAR));
    if (!szName)
        BIN_CRYPT_ERR("Allocating memory for certificate issuer failed");
    if (!(CertGetNameString(pCertContext,
        CERT_NAME_SIMPLE_DISPLAY_TYPE,
        CERT_NAME_ISSUER_FLAG,
        NULL,
        szName,
        dwData)))
    {
        BIN_CRYPT_ERR("Getting certificate issuer failed");
    }
    crypt_info.publisher = szName;
    LocalFree(szName);
    szName = NULL;

    // Get the subject
    dwData =
        CertGetNameString
        (
            pCertContext,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            0,
            NULL,
            NULL,
            0
        );
    if (dwData == 0)
        BIN_CRYPT_ERR("Getting certificate subject length failed");
    szName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(TCHAR));
    if (!szName)
        BIN_CRYPT_ERR("Allocating memory for certificate subject failed");
    if (!(CertGetNameString(pCertContext,
        CERT_NAME_SIMPLE_DISPLAY_TYPE,
        0,
        NULL,
        szName,
        dwData)))
    {
        BIN_CRYPT_ERR("Getting certificate subject failed");
    }
    crypt_info.subject = szName;
    LocalFree(szName);
    szName = NULL;

Cleanup:
    if (pSignerInfo != NULL) LocalFree(pSignerInfo);
    if (szName != NULL) LocalFree(szName);
    if (pCertContext != NULL) CertFreeCertificateContext(pCertContext);
    if (hStore != NULL) CertCloseStore(hStore, 0);
    if (hMsg != NULL) CryptMsgClose(hMsg);

    if (!exp_msg.empty())
        raiseError(exp_msg.c_str());
    else
        return crypt_info;
}
