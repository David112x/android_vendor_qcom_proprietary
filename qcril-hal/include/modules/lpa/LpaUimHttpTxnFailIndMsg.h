/******************************************************************************
#  Copyright (c) 2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/

#pragma once
#include "framework/GenericCallback.h"
#include "framework/UnSolicitedMessage.h"
#include "framework/Message.h"
#include "framework/add_message_id.h"


class LpaUimHttpTxnFailIndMsg : public UnSolicitedMessage,
                                    public add_message_id<LpaUimHttpTxnFailIndMsg>
{
  private:
    int                     mToken;

  public:
    static constexpr const char *MESSAGE_NAME = "com.qualcomm.qti.qcril.lpa.http_transaction_local_fail_ind_msg";
    LpaUimHttpTxnFailIndMsg() = delete;
    ~LpaUimHttpTxnFailIndMsg() = default;

    LpaUimHttpTxnFailIndMsg(int token):UnSolicitedMessage(get_class_message_id())
    {
      mName = MESSAGE_NAME;
      mToken = token;
    }

    inline std::shared_ptr<UnSolicitedMessage> clone()
    {
        return  std::static_pointer_cast<UnSolicitedMessage>(std::make_shared<LpaUimHttpTxnFailIndMsg>(mToken));
    }

    inline string dump()
    {
      return mName + "{}";
    }

    inline int getToken(void)
    {
      return mToken;
    }

};
