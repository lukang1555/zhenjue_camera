#include "arm_camera_package/http_camera_controller.hpp"

#include <algorithm>
#include <cctype>
#include <mutex>
#include <sstream>

#include <curl/curl.h>

namespace arm_camera_package
{
    namespace
    {
        std::once_flag curl_init_once;

        std::string to_upper(std::string input)
        {
            std::transform(
                input.begin(), input.end(), input.begin(), [](unsigned char ch)
                { return static_cast<char>(std::toupper(ch)); });
            return input;
        }
    } // namespace

    HttpResponse HttpCameraController::send_command(
        const CameraConfig &config,
        const HttpRequest &request) const
    {
        std::call_once(curl_init_once, []()
                       { curl_global_init(CURL_GLOBAL_DEFAULT); });

        HttpResponse response;
        if (config.http_base_url.empty())
        {
            response.error = "http_base_url is empty";
            return response;
        }

        CURL *curl = curl_easy_init();
        if (curl == nullptr)
        {
            response.error = "curl_easy_init failed";
            return response;
        }

        const std::string url = build_url(config.http_base_url, request);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, request.timeout_ms);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpCameraController::write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);

        if (!config.username.empty())
        {
            const std::string userpwd = config.username + ":" + config.password;
            curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
            curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd.c_str());
        }

        const std::string method = to_upper(request.method.empty() ? "GET" : request.method);
        if (method == "POST")
        {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        }
        else if (method != "GET")
        {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        }

        if (!request.body.empty() && method != "GET")
        {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request.body.size());
        }

        struct curl_slist *headers = nullptr;
        if (!request.body.empty() && !request.content_type.empty())
        {
            headers = curl_slist_append(
                headers,
                ("Content-Type: " + request.content_type).c_str());
        }
        for (const auto &header : request.extra_headers)
        {
            headers = curl_slist_append(headers, header.c_str());
        }
        if (headers != nullptr)
        {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }

        const CURLcode code = curl_easy_perform(curl);
        if (code != CURLE_OK)
        {
            response.error = curl_easy_strerror(code);
        }

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status_code);

        if (headers != nullptr)
        {
            curl_slist_free_all(headers);
        }
        curl_easy_cleanup(curl);

        return response;
    }

    std::string HttpCameraController::build_url(const std::string &base_url, const HttpRequest &request)
    {
        std::string base = base_url;
        std::string path = request.path.empty() ? "/" : request.path;

        if (!path.empty() && path.front() != '/')
        {
            path.insert(path.begin(), '/');
        }

        if (!base.empty() && base.back() == '/' && !path.empty() && path.front() == '/')
        {
            base.pop_back();
        }

        std::string url = base + path;
        if (!request.query.empty())
        {
            if (url.find('?') == std::string::npos)
            {
                url += "?";
            }
            else
            {
                url += "&";
            }
            url += request.query;
        }

        return url;
    }

    size_t HttpCameraController::write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
    {
        if (ptr == nullptr || userdata == nullptr)
        {
            return 0;
        }

        auto *out = static_cast<std::string *>(userdata);
        const size_t total_size = size * nmemb;
        out->append(ptr, total_size);
        return total_size;
    }

} // namespace arm_camera_package
