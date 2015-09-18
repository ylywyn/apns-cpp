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
    /// ���캯��
    Config(): file_(""), unsaved_(false) {}

    /// ����Ϊ�����ļ����Ĺ��캯��
    Config( const string& file): file_(file), unsaved_(false)
    {
        this->load( file );
    }


    ~Config()
    {
        if ( unsaved_ ) this->save();
    }


    ////////////////////////////////////////////////////////////////////////////
    /// ��ȡ���������ļ�
    bool load(const string &file);

    /// ���������ļ�
    bool save(const string &file = "");

    /// ����������Ƿ����
    bool valueExist(const string &section, const string &name);

    /// ������ÿ��Ƿ����
    bool sectionExist(const string &section);

    /// ��ȡ���������ֵ
    string getValue(const string &section, const string &name, const string &default_value = "");

    /// ��ȡָ�����ÿ��ȫ�����������ֵ
    void getSection(const string &sectionName, map<string, string>& section);

    /// ��ȡȫ�����ÿ��б�
    void sectionList(vector<string>& sectionlist);


    /// ����������
    bool setValue(const string &section, const string &name, const string &value);

    /// ����ָ�����ÿ���������б�
    bool setSection(const string &section, const map<string,string> &valuelist);

    ////////////////////////////////////////////////////////////////////////////
    /// ɾ��������
    void delValue(const string &section, const string &name);

    /// ɾ�����ÿ�
    void delSection(const string &section);

    ////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////
    /// ȫ��������: ��ȡ���������ֵ
    inline string operator[] (const string &name)
    {
        return this->getValue( "", name );
    }

    ////////////////////////////////////////////////////////////////////////////
    /// ȫ��������: ��ȡ���������ֵ
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


