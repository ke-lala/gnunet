package org.gnunet.gnunetmessenger.ipc;

import org.gnunet.gnunetmessenger.ipc.ChatAccountDto;

interface IAccountCallback {
    void onAccount(in ChatAccountDto account);
    void onDone();
    void onError(int code, String message);
}
