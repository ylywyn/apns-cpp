#include "Config.h"

/// 读取解析配置文件
/// \param file 配置文件路径名
/// \retval true 解析成功
/// \retval false 解析失败
bool Config::load( const string &file )
{
    if (file.empty())
        return false;
    file_ = file;

    string line;
    const int LineLength = 1024;
    char pline[LineLength];
    FILE* fp = fopen(file.c_str(), "r");
    if(fp == NULL)
    {
        return false;
    }

    //
    size_t pos;
    bool line_continue = false;
    string curr_section, curr_name, curr_value;

    while (fgets(pline, LineLength, fp))
    {
        //先过滤一遍,减少无用计算
        if (*pline == '#' || strlen(pline) < LINE_MIN_LING)
        {
            continue;
        }

        char* p = pline;
        line.assign(p, trim(p));
        trim(line, '\r');
        trim(line, '\n');
        if ( line_continue )
        {
            if ( line.substr(0,2) == " \\" ) {
                // continue...
                line_continue = true;
                line = line.substr( 0, line.length()-2 );
                trim(line);
                curr_value += line;
            } else
            {
                // end
                line_continue = false;
                curr_value += line;
                config_[ curr_section ][ curr_name ] = curr_value;
            }
        }
        // comment line
        else if ( line[0] == '#' )
        {
            continue;
        }
        // section define
        else if ( line[0]=='[' && line[line.length()-1]==']' )
        {
            curr_section = line.substr( 1, line.length()-2 );
            trim(curr_section);
            if ( curr_section != "" )
            {
                value_def null_value;
                config_[ curr_section ] = null_value;
            }
        }
        // value define
        else if ( (pos=line.find("=")) != line.npos )
        {
            string name = line.substr( 0, pos );
            string value = line.substr( pos+1 );
            trim(name);
            trim(value);

            if ( name == "" )
                continue;

            if ( value.substr(0,2) == " \\" )
            {
                // multi-line
                line_continue = true;
                value = value.substr( 0, value.length()-2 );
                trim(value);
                curr_name = name;
                curr_value = value;
            } else
            {
                // single line
                line_continue = false;
                config_[ curr_section ][ name ] = value;
            }
        }
        // unknown
        else
        {
            continue; // ignore
        }
    }

    fclose(fp);
    return true;
}

/// 保存配置文件
/// \param file 配置文件路径名，默认为空则使用读取文件参数
bool Config::save( const string &file ) {
    string configfile_ = file_;
    if ( file != "" )
        configfile_ = file;

    // for each section
    string content;
    content.reserve( 2048 );

    biter_ = config_.begin();
    for ( ; biter_!=config_.end(); ++biter_ )
    {
        if ( biter_->first != "" )
            content += "[" + (biter_->first) + "]\n";

        // for each value
        value_def valuelist = biter_->second;
        viter_ = valuelist.begin();
        for ( ; viter_!=valuelist.end(); ++viter_ )
            content += (viter_->first) + " = " + (viter_->second) + "\n";
    }

    bool ret = false;
    FILE* fp = fopen(configfile_.c_str(), "w");
    if (fp != NULL)
    {
        if (fwrite(content.c_str(), content.length(), 1, fp) == 1)
        {
            ret = true;
        }
        fclose(fp);
    }
    unsaved_ = !ret;
    return ret;
}

/// 检查配置项是否存在
bool Config::valueExist( const string &section, const string &name ) {
    if ( name == "" )
        return false;

    // locate section
    biter_ = config_.find( section );
    if ( biter_ != config_.end() ) {
        // locate name
        value_def valuelist = biter_->second;
        viter_ = valuelist.find( name );
        if ( viter_ != valuelist.end() )
            return true;
    }

    return false;
}

/// 检查配置块是否存在
/// \param section 配置块名，为空表示全局配置块
bool Config::sectionExist( const string &section ) {
    if ( section == "" )
        return true;

    biter_ = config_.find( section );
    if ( biter_ != config_.end() )
        return true;
    else
        return false;
}

/// 读取配置项参数值
string Config::getValue( const string &section, const string &name,
                          const string &default_value ) {
    if ( name == "" )
        return default_value;

    // locate section
    biter_ = config_.find( section );
    if ( biter_ == config_.end() )
        return default_value;

    // locate value
    value_def valuelist = biter_->second;
    viter_ = valuelist.find( name );
    if ( viter_ == valuelist.end() )
        return default_value;

    return viter_->second;
}

/// 读取指定配置块的全部配置项参数值
void Config::getSection( const string &sectionName ,map<string,string>& section) {
    value_def valuelist;
    if ( sectionExist(sectionName) )
        section = config_[sectionName];
}

/// 读取全部配置块列表
/// \return 全部配置块列表，包括全局配置块（section值为空）
void Config::sectionList(vector<string>& sectionlist)
{
    biter_ = config_.begin();
    for ( ; biter_!=config_.end(); biter_++ )
        sectionlist.push_back( biter_->first );
}

/// 更新配置项
bool Config::setValue( const string &section, const string &name,
                        const string &value )
{
    if ( name == "" )
        return false;
    config_[ section ][ name ] = value;
    unsaved_ = true;
    return unsaved_;
}

/// 更新指定配置块的配置项列表
bool Config::setSection( const string &section,
                          const map<string,string> &valuelist )
{
    if ( valuelist.size() <= 0 )
        return false;

    value_def::const_iterator i = valuelist.begin();
    for ( ; i!=valuelist.end(); ++i )
        config_[ section ][ i->first ] = i->second;

    unsaved_ = true;
    return unsaved_;
}

/// 删除配置项
void Config::delValue( const string &section, const string &name ) {
    biter_ = config_.find( section );
    if ( biter_ != config_.end() ) {
        viter_ = (biter_->second).find( name );
        if ( viter_ != (biter_->second).end() ) {
            (biter_->second).erase( viter_ );
            unsaved_ = true;
            return;
        }
    }
}

/// 删除配置块
/// \param section 配置块名，为空表示全局配置块
void Config::delSection( const string &section ) {
    biter_ = config_.find( section );
    if ( biter_ != config_.end() ) {
        config_.erase( biter_ );
        unsaved_ = true;
        return;
    }
}

char* Config::trim(char *&p)
{
    char *rp = p + strlen(p) -1;

    while(rp != p && *rp == ' ') --rp;

    while (*p && *p == ' ') ++p;

    return (*rp =='\n')? rp : rp + 1;
}

void Config::trim(std::string& src, const char csRemoved )
{
    if (!src.empty())
    {
        std::string::size_type pos = src.find_last_not_of(csRemoved);

        if (pos != std::string::npos)
        {
            src.erase(pos+1);
            pos = src.find_first_not_of(csRemoved);
            if (pos != std::string::npos)
                src.erase(0, pos);
        }
        else
            src.clear(); // make empty
    }
}


