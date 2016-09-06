#include "StdAfx.h"
#include "MainUi.h"

#include "config.h"
#include "WeChatLogic.h"
#include "ImageLoader.h"

MainUi::MainUi(void) : window_(MemToolParam)
{
    needShowLogin_ = false;
    currentUser_ = NULL;
    msgContainer_ = NULL;
    msgLayout_ = NULL;
}

MainUi::~MainUi(void)
{
}

bool MainUi::Show()
{
    window_->SetStyle(WindowStyle::Top);
    window_->WindowCreatedEvent.AddHandler(this, &MainUi::OnWindowCreated);
    window_->SetVisible(true);
    window_->DoModalWithStyle(NULL, _T("@MainUi:MainUi"));

    HttpUtil::StopAllRequest();
    loadMsgThread_.StopAndWait();
    CImageLoader::Get().Stop();

    return needShowLogin_;
}

bool MainUi::OnWindowCreated(Base::NBaseObj* source, NEventData* eventData)
{
    NFrame* rootFrame = window_->GetRootFrame();
    NLayout* layout = rootFrame->GetChildById<NLayout*>(_T("contactList"));
    msgContainer_ = rootFrame->GetChildById<NFrame*>(_T("msgContainer"));
    WeChatLogic::UserInfoList& userInfoList = WeChatLogic::Get().GetUserInfoList();
    WeChatLogic::UserInfoList::iterator ite = userInfoList.begin();

    editContent_ = rootFrame->GetChildById<NEdit*>(_T("MsgContent"));
    rootFrame->GetChildById<NFrame*>(_T("sendMsg"))->ClickEvent.AddHandler(this, &MainUi::OnBtnSend);

    NInstPtr<NParser> parser(MemToolParam);
    for(; ite != userInfoList.end(); ++ ite)
    {
        UserInfo* & user = *ite;
        NAutoPtr<NFrame> frame = dynamic_cast<NFrame*>((NBaseObj*)parser->Parse(layout, _T("@MainUi:RbContact")));
        NLabel* title = frame->GetChildById<NLabel*>(_T("title"));
        NImage* avatar = frame->GetChildById<NImage*>(_T("avatar"));
        title->SetText(user->GetName());
        frame->SetData(reinterpret_cast<DWORD>(user));
        user->data = reinterpret_cast<DWORD>((NFrame*)frame);

        frame->ClickEvent.AddHandler(this, &MainUi::OnContactClicked);
        CImageLoader::Get().LoadImage(avatar, user->headImgUrl, true);
    }

    loadMsgThread_.Start(MakeDelegate(this, &MainUi::LoadMsgThreadProc));
    return false;
}

bool MainUi::OnContactClicked(Base::NBaseObj* source, NEventData* eventData)
{
    NAutoPtr<NFrame> contactFrame = dynamic_cast<NFrame*>(source);
    UserInfo* user = reinterpret_cast<UserInfo*>(contactFrame->GetData());
    ShowContact(user, contactFrame);
    return false;
}

bool MainUi::OnBtnSend(Base::NBaseObj* source, NEventData* eventData)
{
    if(currentUser_ == NULL)
        return false;
    NString text = editContent_->GetText();
    if(text.IsEmpty())
        return false;
    WeChatLogic::Get().SendTextMsg(currentUser_->userName, text);
    editContent_->SetText(_T(""));
    return false;
}

UserInfo* MainUi::AddMsgs(const WeChatMsgList& listMsgs)
{
    if(currentUser_ == NULL)
        return NULL;

    WeChatMsg* lastMsg = NULL;
    UserInfo* lastUserInfo = NULL;
    NInstPtr<NParser> parser(MemToolParam);
    const UserInfo& selfInfo = WeChatLogic::Get().GetSelfInfo();
    WeChatMsgList::const_iterator ite = listMsgs.begin();
    for(; ite != listMsgs.end(); ++ ite)
    {
        WeChatMsg* msg = *ite;

        if(msg->MsgType != WeChatMsgText)
            continue;

        UserInfo* userInfo = WeChatLogic::Get().GetUserInfo(msg->FromUserName);
        lastUserInfo = userInfo->userName == selfInfo.userName ? WeChatLogic::Get().GetUserInfo(msg->ToUserName) : userInfo;
        if(userInfo == NULL)
            continue;

        if(msg->FromUserName != currentUser_->userName && msg->ToUserName != currentUser_->userName)
            continue;

        lastMsg = msg;

        bool isSelf = userInfo->userName == selfInfo.userName;
        NString styleName = isSelf ? _T("@MainUi:SelfMsg") : _T("@MainUi:OtherMsg");
        NFrame* msgFrame = dynamic_cast<NFrame*>((NBaseObj*)parser->Parse(msgLayout_, styleName));
        NString text = userInfo->GetName() + _T("\r\n");
        text += msg->Content;
        msgFrame->SetText(text);
    }
    if(lastMsg != NULL)
    {
        NFrame* frame = reinterpret_cast<NFrame*>(currentUser_->data);
        frame->GetChildById(_T("msg"))->SetText(lastMsg->Content);
    }
    return lastUserInfo;
}

void MainUi::ShowContact(UserInfo* user, NFrame* frame)
{
    if(currentUser_ == user)
        return;

    currentUser_ = user;
    if(msgLayout_ != NULL)
    {
        msgContainer_->RemoveChild(msgLayout_);
        msgLayout_ = NULL;
    }
    NInstPtr<NParser> parser(MemToolParam);
    msgLayout_ = dynamic_cast<NLayout*>((NBaseObj*)parser->Parse(msgContainer_, _T("@MainUi:msgLayout")));

    const WeChatMsgList& msgList = currentUser_->GetMsgList();
    if(msgList.empty())
        return;
    AddMsgs(msgList);
}

void MainUi::LoadMsgThreadProc(bool& stopping)
{
    int retCode = 0;
    int selector = 0;
    WeChatMsgList msgs;
    for(;!stopping;)
    {
        if(!WeChatLogic::Get().QueryMsgExists(retCode, selector))
        {
            if(retCode == 1100)
            {
                NInstPtr<NStringBundle> bundle(MemToolParam);
                NString msg = bundle->GetString(_T("@MainUi:errAppExited"));
                NString title = bundle->GetString(_T("@Common:appTitle"));
                if(::MessageBox(window_->GetNative(), msg, title, MB_YESNO | MB_ICONQUESTION) == IDYES)
                    needShowLogin_ = true;
                break;
            }
            else if(retCode == 1101)
            {
                NInstPtr<NStringBundle> bundle(MemToolParam);
                NString msg = bundle->GetString(_T("@MainUi:errLogon"));
                NString title = bundle->GetString(_T("@Common:appTitle"));
                if(::MessageBox(window_->GetNative(), msg, title, MB_YESNO | MB_ICONQUESTION) == IDYES)
                    needShowLogin_ = true;
                break;
            }
            Sleep(500);
            continue;
        }

        if(!WeChatLogic::Get().LoadMsgContent(msgs))
        {
            Sleep(500);
            continue;
        }

        int result = SendUiProc(MakeDelegate(this, &MainUi::OnMsgArrived), reinterpret_cast<void*>(&msgs));
    }
    if(needShowLogin_ && window_ != NULL)
    {
        window_->Destroy();
    }
}

LRESULT MainUi::OnMsgArrived(void* param)
{
    WeChatMsgList* listMsgs = reinterpret_cast<WeChatMsgList*>(param);
    UserInfo* lastUserInfo = AddMsgs(*listMsgs);
    if(lastUserInfo != NULL)
    {
        NFrame* frame = reinterpret_cast<NFrame*>(lastUserInfo->data);
        NLayout* layout = window_->GetRootFrame()->GetChildById<NLayout*>(_T("contactList"));
        layout->GetChildById<NFrame*>(_NUI_INNER_FRAME_ID_)->SetChildZOrder(frame, 0);
        layout->RelayoutChilds();
    }
    return 0;
}

