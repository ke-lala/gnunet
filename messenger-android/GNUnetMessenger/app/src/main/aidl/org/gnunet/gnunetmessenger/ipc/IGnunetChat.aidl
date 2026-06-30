package org.gnunet.gnunetmessenger.ipc;

import org.gnunet.gnunetmessenger.ipc.ChatAccountDto;
import org.gnunet.gnunetmessenger.ipc.ChatContactDto;
import org.gnunet.gnunetmessenger.ipc.ChatGroupDto;
import org.gnunet.gnunetmessenger.ipc.ChatUriDto;
import org.gnunet.gnunetmessenger.ipc.IChatCallback;
import org.gnunet.gnunetmessenger.ipc.IAccountCallback;
import org.gnunet.gnunetmessenger.ipc.IContactCallback;
import org.gnunet.gnunetmessenger.ipc.IGroupCallback;
import org.gnunet.gnunetmessenger.ipc.IGroupContactCallback;
import org.gnunet.gnunetmessenger.ipc.IAttributeCallback;
import org.gnunet.gnunetmessenger.ipc.ILobbyCallback;
import org.gnunet.gnunetmessenger.ipc.IMessageIterateCallback;
import org.gnunet.gnunetmessenger.ipc.ChatContextDto;
import org.gnunet.gnunetmessenger.ipc.ChatMessageDto;

interface IGnunetChat {
    // Core chat operations
    int getApiVersion();
    long startChat(String messengerApp, IChatCallback cb);
    void reset();
    void iterateAccounts(long handle, IAccountCallback cb);
    int createAccount(long handle, String name);
    void connect(long handle, in ChatAccountDto account);
    void disconnect(long handle);
    void stopChat(long handle);
    String getProfileName(long handle);
    void setProfileName(long handle, String name);

    // Contact operations
    String getProfileKey(long handle);
    boolean isContactBlocked(in ChatContactDto contact);
    void setContactBlocked(in ChatContactDto contact, boolean isBlocked);
    void setAttribute(long handle, String key, String value);
    void getAttributes(long handle, IAttributeCallback cb);
    
    // Lobby operations
    void lobbyOpen(long handle, ILobbyCallback cb);
    void lobbyJoin(long handle, String uri);
    
    // Group operations
    void setGroupName(in ChatGroupDto group, String name);
    ChatGroupDto createGroup(long handle, String topic);
    
    // URI operations
    ChatUriDto parseUri(String uri);
    void destroyUri(in ChatUriDto uri);
    
    // Group contact operations
    void inviteContactToGroup(in ChatGroupDto group, in ChatContactDto contact);
    String getUserPointerForContext(in ChatContextDto context);
    void setUserPointerForContext(in ChatContextDto context, String userPointer);
    
    // Message operations
    ChatContactDto getSenderFromMessage(in ChatMessageDto message);
    ChatGroupDto getGroupFromContext(in ChatContextDto context);
    ChatMessageDto getMessageForGroupContact(in ChatGroupDto group, in ChatContactDto contact);
    int getMessageKind(in ChatMessageDto message);
    int isMessageRecent(in ChatMessageDto message);
    long getMessageTimestamp(in ChatMessageDto message);
    void setMessageForGroupContact(in ChatGroupDto group, in ChatContactDto contact, in ChatMessageDto message);
    
    // Iteration operations
    void iterateContacts(long handle, IContactCallback cb);
    void iterateGroups(long handle, IGroupCallback cb);
    
    // Context operations
    ChatContextDto getContactContext(in ChatContactDto chatContact);
    ChatContextDto getGroupContext(in ChatGroupDto chatGroup);
    String getContactUserPointer(in ChatContactDto chatContact);
    void setContactUserPointer(in ChatContactDto chatContact, String userPointer);
    String getGroupUserPointer(in ChatGroupDto chatGroup);
    void setGroupUserPointer(in ChatGroupDto chatGroup, String userPointer);
    
    // Messaging operations
    void sendText(in ChatContextDto chatContext, String text);
    String getContactKey(in ChatContactDto chatContact);
    ChatContactDto getContextContact(in ChatContextDto context);
    void deleteContact(in ChatContactDto chatContact);
    boolean isGroup(in ChatContextDto context);
    boolean isPlatform(in ChatContextDto context);
    
    // Group contact iteration
    void iterateGroupContacts(in ChatGroupDto chatGroup, IGroupContactCallback cb);
    
    // Utility operations
    String randomUUID();
    void getContactAttributes(in ChatContactDto contact, IAttributeCallback cb);
    void shareAttributes(long handle, in ChatContactDto contact, String key);
    void unshareAttributes(long handle, in ChatContactDto contact, String key);

    // Message iteration
    void iterateContextMessages(in ChatContextDto context, IMessageIterateCallback cb);
}
