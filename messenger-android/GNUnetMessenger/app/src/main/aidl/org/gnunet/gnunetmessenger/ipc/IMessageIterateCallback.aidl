package org.gnunet.gnunetmessenger.ipc;

import org.gnunet.gnunetmessenger.ipc.ChatMessageDto;

interface IMessageIterateCallback {
    void onMessage(in ChatMessageDto message);
    void onDone();
    void onError(int code, String message);
}
