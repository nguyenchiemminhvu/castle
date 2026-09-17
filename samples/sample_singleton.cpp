#include "castle/design_patterns/singleton.h"

#include <iostream>
#include <unordered_map>
#include <string>

using namespace castle::design_patterns;

class config_manager
{
public:
    config_manager() = default;
    ~config_manager() = default;

    void set_config(const std::string& key, const std::string& value)
    {
        m_config[key] = value;
    }

    std::string get_config(const std::string& key) const
    {
        auto it = m_config.find(key);
        if (it != m_config.end())
        {
            return it->second;
        }
        return "";
    }

private:
    std::unordered_map<std::string, std::string> m_config;
};

using config_manager_singleton = singleton<config_manager>;

int main()
{
    if (!config_manager_singleton::is_valid())
    {
        config_manager_singleton::create();
    }

    config_manager_singleton::instance().set_config("app_name", "MyApp");
    std::string app_name = config_manager_singleton::instance().get_config("app_name");
    std::cout << "App Name: " << app_name << std::endl;

    config_manager_singleton::destroy();

    return 0;
}