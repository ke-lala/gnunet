package org.gnunet.gnunetmessenger.ipc;

import org.gnunet.gnunetmessenger.ipc.ChatGroupDto;

interface IGroupCallback {
    void onGroup(in ChatGroupDto group);
    void onDone();
    void onError(int code, String message);
}
