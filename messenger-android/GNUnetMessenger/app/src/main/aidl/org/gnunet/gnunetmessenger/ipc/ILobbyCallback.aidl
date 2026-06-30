package org.gnunet.gnunetmessenger.ipc;

interface ILobbyCallback {
    void onLobbyUri(String uri);
    void onError(int code, String message);
}
