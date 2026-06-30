package org.gnunet.gnunetmessenger.ipc;

import org.gnunet.gnunetmessenger.ipc.ChatGroupDto;
import org.gnunet.gnunetmessenger.ipc.ChatContactDto;

interface IGroupContactCallback {
    void onGroupContact(in ChatGroupDto group, in ChatContactDto contact);
    void onDone();
    void onError(int code, String message);
}
