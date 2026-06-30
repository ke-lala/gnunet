package org.gnunet.gnunetmessenger.ipc;

import org.gnunet.gnunetmessenger.ipc.ChatContextDto;
import org.gnunet.gnunetmessenger.ipc.ChatMessageDto;

interface IChatCallback {
    void onMessage(in ChatContextDto context, in ChatMessageDto message);
}
