package org.gnunet.gnunetmessenger.ipc;

interface IAttributeCallback {
    void onAttribute(String key, String value);
    void onDone();
    void onError(int code, String message);
}
