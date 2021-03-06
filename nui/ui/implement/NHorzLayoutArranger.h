#pragma once


#include "../NLayoutArranger.h"

namespace nui
{
    namespace Ui
    {
        class NHorzLayoutArranger : public NLayoutArranger
        {
        public:
            virtual void RelayoutChild(NFrameBase* child, NLayoutArrangerParam& param) const;
            virtual bool IfCouldLayout(const NFrameBase* const &child) const;
        };
    }
}