#pragma once

#include <string>
#include <vector>

#include <Windows.h>
#include <winhttp.h>

#include "../../mscript-core/module.h"

using namespace mscript;

class http
{
public:
    http()
        : m_session(nullptr)
        , m_connection(nullptr)
        , m_request(nullptr)
    {
    }

    ~http()
    {
        if (m_request != nullptr) 
            ::WinHttpCloseHandle(m_request);
        
        if (m_connection != nullptr) 
            ::WinHttpCloseHandle(m_connection);
        
        if (m_session != nullptr) 
            ::WinHttpCloseHandle(m_session);
    }

    // Inputs: server (string), usetls (bool), port (number), verb (string), path (string), headers (index), inputfile (string), outputfile (string)
    // Returns: statuscode (number), headers (index)
    object::index ProcessRequest(const object::index& param)
    {
        //
        // Unpack input parameters
        //
        object server;
        if (!param.tryGet(object("server"), server))
            throw std::exception("server missing from input");
        if (server.typeStr() != "string")
            throw std::exception("server field not string");

        object use_tls = false;
        param.tryGet(object("usetls"), use_tls);
        if (use_tls.typeStr() != "bool")
            throw std::exception("usetls field not bool");

        object port = double(use_tls.boolVal() ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT);
        param.tryGet(object("port"), port);
        if (port.typeStr() != "number")
            throw std::exception("port field not number");

        object verb = "GET";
        param.tryGet(object("verb"), verb);
        if (verb.typeStr() != "string")
            throw std::exception("verb field not string");

        object path;
        if (!param.tryGet(object("path"), path))
            throw std::exception("path missing from inputs");
        if (path.typeStr() != "string")
            throw std::exception("path field not string");

        object input_headers = object::index();
        param.tryGet(object("headers"), input_headers);
        if (input_headers.typeStr() != "index")
            throw std::exception("headers field not index");
        object::index headers = input_headers.indexVal();

        object accept_header;
        headers.tryGet(L"Accept", accept_header);
        std::wstring accept_header_str;
        if (accept_header.type() == object::object_type::STRING)
            accept_header_str = accept_header.stringVal();

        //
        // Build the request headers
        // Accept and Content-Length are handled specially by WinHttp
        //
        std::wstring headers_combined;
        {
            object::list header_keys = headers.keys();
            for (size_t h = 0; h < headers.size(); ++h)
            {
                std::wstring header_name = header_keys[h].stringVal();
                std::wstring header_name_lower = toLower(header_name);

                std::wstring header_value = headers.getAt(h).stringVal();

                if (header_name_lower != L"accept" && header_name_lower != L"content-length")
                    headers_combined += header_name + L": " + header_value + L"\r\n";
            }
        }

        //
        // Read the input file
        //
        std::vector<uint8_t> input_data;
        {
            object input_path = std::wstring();
            param.tryGet(object("inputfile"), input_path);
            if (input_path.typeStr() != "string")
                throw std::exception("inputfile field not string");
            if (!input_path.stringVal().empty())
            {
                FILE* file = _wfopen(input_path.stringVal().c_str(), L"rb");
                if (!file)
                    throw std::exception("Cannot open input file");

                fseek(file, 0, SEEK_END);
                input_data.reserve(ftell(file));
                fseek(file, 0, SEEK_SET);

                uint8_t buffer[4 * 1024];
                int count = 0;
                while ((count = fread(buffer, 1, sizeof(buffer), file)) > 0)
                {
                    for (int b = 0; b < count; ++b)
                        input_data.push_back(buffer[b]);
                }

                fclose(file);
            }
        }

        object output_path = std::wstring();
        param.tryGet(object("outputfile"), output_path);
        if (output_path.typeStr() != "string")
            throw std::exception("outputfile field not string");

        //
        // Connect to the server
        //
        m_session =
            ::WinHttpOpen
            (
                L"mscript",
                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                WINHTTP_NO_PROXY_NAME,
                WINHTTP_NO_PROXY_BYPASS,
                0
            );
        if (!m_session)
            throw std::exception("Creating web session failed");

        m_connection =
            ::WinHttpConnect
            (
                m_session,
                server.stringVal().c_str(),
                INTERNET_PORT(port.numberVal()),
                0
            );
        if (!m_connection)
            throw std::exception("Connecting to server failed");

        const wchar_t* accept_all_types[] =
        {
            !accept_header_str.empty() ? accept_header_str.c_str() : L"*/*",
            nullptr
        };

        //
        // Send the request
        //
        m_request =
            ::WinHttpOpenRequest
            (
                m_connection,
                verb.stringVal().c_str(),
                path.stringVal().c_str(),
                NULL,
                WINHTTP_NO_REFERER,
                accept_all_types,
                WINHTTP_FLAG_REFRESH | (use_tls.boolVal() ? WINHTTP_FLAG_SECURE : 0)
            );
        if (!m_request)
            throw std::exception("Opening request to server failed");

        if (!::WinHttpSendRequest(m_request, headers_combined.c_str(), (DWORD)- 1L, input_data.data(), input_data.size(), input_data.size(), 0))
            throw std::exception("Sending request to server failed");

        if (!::WinHttpReceiveResponse(m_request, NULL))
            throw std::exception("Receiving response from server failed");

        //
        // Unpack the response status code
        //
        DWORD dwStatusCode = 0;
        {
            DWORD dwSize = sizeof(dwStatusCode);
            if
            (
                !WinHttpQueryHeaders
                (
                    m_request, 
                    WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, 
                    WINHTTP_HEADER_NAME_BY_INDEX,
                    &dwStatusCode, 
                    &dwSize, 
                    WINHTTP_NO_HEADER_INDEX
                )
            )
            {
                throw std::exception("Getting response status code failed");
            }
        }

        //
        // Process the response headers
        //
        object::index output_headers;
        {
            DWORD dwHeadersSize = 0;
            WinHttpQueryHeaders
            (
                m_request,
                WINHTTP_QUERY_RAW_HEADERS_CRLF | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
                WINHTTP_HEADER_NAME_BY_INDEX,
                NULL,
                &dwHeadersSize,
                WINHTTP_NO_HEADER_INDEX
            );
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                throw std::exception("Getting response header info failed");

            std::vector<WCHAR> buffer;
            buffer.resize(dwHeadersSize / sizeof(WCHAR));
            if
            (
                !WinHttpQueryHeaders
                (
                    m_request,
                    WINHTTP_QUERY_RAW_HEADERS_CRLF | WINHTTP_QUERY_FLAG_REQUEST_HEADERS,
                    WINHTTP_HEADER_NAME_BY_INDEX,
                    buffer.data(),
                    &dwHeadersSize,
                    WINHTTP_NO_HEADER_INDEX
                )
            )
            {
                throw std::exception("Getting response headers failed");
            }
            for (const auto& header_str : split(buffer.data(), L"\r\n"))
            {
                auto f = header_str.find(':');
                if (f == std::wstring::npos)
                    continue;
                output_headers.set(header_str.substr(0, f), header_str.substr(f + 1));
            }
        }

        //
        // Process response bytes
        //
        if (!output_path.stringVal().empty())
        {
            FILE* output = _wfopen(output_path.stringVal().c_str(), L"wb");
            if (!output)
                throw std::exception("Opening output file failed");

            bool success = false;
            std::vector<uint8_t> buffer;
            DWORD dwSize = 0;
            while (true)
            {
                dwSize = 0;
                if (!::WinHttpQueryDataAvailable(m_request, &dwSize))
                    break;
                if (dwSize == 0)
                {
                    success = true;
                    break;
                }
                buffer.resize(dwSize);

                DWORD dwDownloaded = 0;
                if (!::WinHttpReadData(m_request, buffer.data(), dwSize, &dwDownloaded))
                    break;

                if (fwrite(buffer.data(), dwDownloaded, 0, output) != 1)
                    break;
            }

            fclose(output);
            if (!success)
                throw std::exception("Getting response data from server failed");
        }

        //
        // Set the outputs
        //
        object::index output_index;
        output_index.set(L"statuscode", double(dwStatusCode));
        output_index.set(L"headers", output_headers);

        // All done.
        return output_index;
    }

private:
    HINTERNET m_session;
    HINTERNET m_connection;
    HINTERNET m_request;
};
