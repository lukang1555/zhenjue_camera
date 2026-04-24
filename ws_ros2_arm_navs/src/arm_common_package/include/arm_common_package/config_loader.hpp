#pragma once

#include <yaml-cpp/yaml.h>

namespace arm_common_package
{
    template <typename T>
    class YamlConfigLoader
    {
    public:
        YamlConfigLoader(const std::string &file_path)
        {
            try
            {
                config_ = YAML::LoadFile(file_path);
            }
            catch (const YAML::Exception &e)
            {
                throw std::runtime_error("Failed to load YAML config file");
            }
        }

        T get_config() const
        {
            T config;
            if (config_.IsDefined())
            {
                config = config_.as<T>();
            }
            return config;
        }

    private:
        YAML::Node config_;
    };
}