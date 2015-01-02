#pragma once


#include "../base/noncopyable.h"
#include "../data/NArrayT.h"

namespace NUI
{
    namespace UI
    {
        class NUI_CLASS NIdleHandler
        {
        public:
            // return false to continue idle
            virtual bool OnIdle(int idleCount) = 0;
        };

        NUI_TEMPLATE template class NUI_CLASS Data::NArrayT<NIdleHandler*>;

        class NUI_CLASS NMsgLoop : protected NUI::Base::Noncopyable
        {
        public:
            bool Loop();
            bool Loop(HWND window);
            void Stop();

            bool AddIdleHandler(NIdleHandler* handler);
            bool RemoveIdleHandler(NIdleHandler* handler);

        private:
            bool Loop(HWND window, bool useWindow);
            bool DoIdleHandler(int idleCount);

        private:
            MSG msg_;
            bool looping_;
            Data::NArrayT<NIdleHandler*> idleHandlers_;
        };
    }
}
