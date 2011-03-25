#include <string>
#include <sstream>
#include <mysql/mysql.h>

#include "plugin.hpp"

using namespace yappi::plugins;

class mysql_t: public source_t {
    public:
        mysql_t(const std::string& uri):
            m_connect_timeout(1),
            m_read_timeout(1),
            m_write_timeout(1)
        {
            // uri: mysql://user:pass@host.yandex.net:3306/db
            size_t s, e;

            s = uri.find_first_of(':') + 3;
            e = uri.find_first_of(':', s);
            m_username = uri.substr(s, e - s);

            s = e + 1;
            e = uri.find_first_of('@', s);
            m_password = uri.substr(s, e - s);
            
            s = e + 1;
            e = uri.find_first_of(':', s);
            m_host = uri.substr(s, e - s);
            
            s = e + 1;
            e = uri.find_first_of('/', s);
            std::istringstream ss(uri.substr(s, e - s));
            ss >> m_port;

            s = e + 1;
            m_db = uri.substr(s);
        }

        dict_t fetch() {
            dict_t dict;

            MYSQL handle;
            mysql_init(&handle);

            mysql_options(&handle, MYSQL_OPT_CONNECT_TIMEOUT, reinterpret_cast<const char*>(&m_connect_timeout));
            mysql_options(&handle, MYSQL_OPT_READ_TIMEOUT, reinterpret_cast<const char*>(&m_read_timeout));
            mysql_options(&handle, MYSQL_OPT_WRITE_TIMEOUT, reinterpret_cast<const char*>(&m_write_timeout));
            MYSQL* result = mysql_real_connect(&handle, m_host.c_str(), m_username.c_str(), m_password.c_str(), m_db.c_str(), m_port, NULL, 0);
            dict["alive"] = result ? "yes" : "no";

            if(result) {
                MYSQL_RES* result = mysql_list_processes(&handle);
                
                // Getting the number of processes
                std::ostringstream s_total;
                s_total << mysql_num_rows(result);
                dict["processes:total"] = s_total.str();

                mysql_free_result(result);
            }
            
            mysql_close(&handle);
            return dict;
        }

    private:
        std::string m_host, m_username, m_password, m_db;
        unsigned int m_port, m_connect_timeout, m_read_timeout, m_write_timeout;
};

void* create_instance(const char* uri) {
    return new mysql_t(uri);
}

static const plugin_info_t plugin_info = {
    1, 
    {
        { "mysql", &create_instance }
    }
};

extern "C" {
    const plugin_info_t* initialize() {
        return &plugin_info;
    }
}