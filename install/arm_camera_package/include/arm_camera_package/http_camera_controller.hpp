#pragma once

#include <string>
#include <vector>

#include "arm_camera_package/camera_node_base.hpp"

namespace arm_camera_package
{

    struct HttpRequest
    {
        std::string method{"GET"};
        std::string path{"/"};
        std::string query;
        std::string body;
        std::string content_type{"application/json"};
        long timeout_ms{3000};
        std::vector<std::string> extra_headers;
    };

    struct HttpResponse
    {
        long status_code{0};
        std::string body;
        std::string error;

        bool ok() const { return error.empty() && status_code >= 200 && status_code < 300; }
    };

    class HttpCameraController
    {
    public:
        HttpCameraController() = default;

        HttpResponse send_command(const CameraConfig &config, const HttpRequest &request) const;

    private:
        static std::string build_url(const std::string &base_url, const HttpRequest &request);
        static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
    };

} // namespace arm_camera_package
