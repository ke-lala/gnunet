package org.gnunet.gnunetmessenger.ipc;

import org.gnunet.gnunetmessenger.ipc.ChatContactDto;

interface IContactCallback {
    void onContact(in ChatContactDto contact);
    void onDone();
    void onError(int code, String message);
}
