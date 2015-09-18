#ifndef _BASIC_ECONFIG_H_
#define _BASIC_ECONFIG_H_

#include <string.h>
#include <string>
#include <vector>
#include <map>

#include <boost/noncopyable.hpp>



using std::string;
using std::vector;
using std::map;

class  Config : boost::noncopyable
{
public:
    /// 构造函数
    Config(): file_(""), unsaved_(false) {}

    /// 参数为配置文件名的构造函数
    Config( const string& file): file_(file), unsaved_(false)
    {
        this->load( file );
    }


    ~Config()
    {
        if ( unsaved_ ) this->save();
    }


    ////////////////////////////////////////////////////////////////////////////
    /// 读取解析配置文件
    bool load(const string &file);

    /// 保存配置文件
    bool save(const string &file = "");

    /// 检查配置项是否存在
    bool valueExist(const string &section, const string &name);

    /// 检查配置块是否存在
    bool sectionExist(const string &section);

    /// 读取配置项参数值
    string getValue(const string &section, const string &name, const string &default_value = "");

    /// 读取指定配置块的全部配置项参数值
    void getSection(const string &sectionName, map<string, string>& section);

    /// 读取全部配置块列表
    void sectionList(vector<string>& sectionlist);


    /// 更新配置项
    bool setValue(const string &section, const string &name, const string &value);

    /// 更新指定配置块的配置项列表
    bool setSection(const string &section, const map<string,string> &valuelist);

    ////////////////////////////////////////////////////////////////////////////
    /// 删除配置项
    void delValue(const string &section, const string &name);

    /// 删除配置块
    void delSection(const string &section);

    ////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////
    /// 全局配置项: 读取配置项参数值
    inline string operator[] (const string &name)
    {
        return this->getValue( "", name );
    }

    ////////////////////////////////////////////////////////////////////////////
    /// 全局配置项: 读取配置项参数值
    inline bool setValue( const string &name, const string &value )
    {
        return this->setValue( "", name, value );
    }

private:
    const static int LINE_MIN_LING = 3;

    char* trim(char *&p);
    void trim(std::string& src, const char csRemoved = ' ');

    typedef map<string,string> value_def; // name -> value
    typedef map<string,value_def> section_def; // section -> ( name -> value )

    string file_;
    bool unsaved_;
    section_def config_;

    value_def::iterator viter_;
    section_def::iterator biter_;
};

#endif


