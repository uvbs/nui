#pragma once


#include "../base/BaseObj.h"

namespace NUI
{
    namespace Data
    {
        enum DataReaderType
        {
            ReaderUnknown,
            ReaderJson,
            ReaderXml,
        };

        class NUI_CLASS NDataReader : public NUI::Base::NBaseObj
        {
        public:
            NDataReader(void){}
            virtual ~NDataReader(void){}

            virtual bool ParseUtf8(const char* data, size_t length) = 0;

            virtual bool ReadValue(LPCTSTR valueName, NUI::Base::NString& value) = 0;

            virtual bool ReadPath(LPCTSTR path, LPCTSTR valueName, NUI::Base::NString& value) = 0;
            virtual bool ReadNode(LPCTSTR nodeName, NDataReader*& value) = 0;

            virtual bool ReadPath(size_t index, LPCTSTR path, LPCTSTR valueName, NUI::Base::NString& value) = 0;
            virtual bool ReadNode(size_t index, LPCTSTR nodeName, NDataReader*& value) = 0;

            virtual bool ParseUtf8(const char* data);

            virtual bool ReadValue(LPCTSTR name, INT32& value);
            virtual bool ReadValue(LPCTSTR name, DWORD& value);
            virtual bool ReadValue(LPCTSTR name, INT64& value);
            virtual bool ReadValue(LPCTSTR name, bool& value);
            virtual bool ReadValue(LPCTSTR name, NUI::Base::NPoint& value);
            virtual bool ReadValue(LPCTSTR name, NUI::Base::NSize& value);
            virtual bool ReadValue(LPCTSTR name, NUI::Base::NRect& value);
        };

        NUI_API NDataReader* CreateDataReader(DataReaderType type);

    }
}