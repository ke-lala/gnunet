/*
   This file is part of GNUnet.
   Copyright (C) 2021--2025 GNUnet e.V.

   GNUnet is free software: you can redistribute it and/or modify it
   under the terms of the GNU Affero General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   GNUnet is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   SPDX-License-Identifier: AGPL3.0-or-later
 */
/*
 * @author t3sserakt
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/chat/ChatFragment.kt
 */

package org.gnunet.gnunetmessenger.ui.chat

import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.EditText
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.MenuHost
import androidx.core.view.MenuProvider
import androidx.fragment.app.Fragment
import androidx.lifecycle.Observer
import androidx.lifecycle.lifecycleScope
import androidx.navigation.fragment.findNavController
import androidx.navigation.fragment.navArgs
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import kotlinx.coroutines.launch
import org.gnunet.gnunetmessenger.MainActivity
import org.gnunet.gnunetmessenger.R
import org.gnunet.gnunetmessenger.model.ChatContact
import org.gnunet.gnunetmessenger.model.ChatContext
import org.gnunet.gnunetmessenger.viewmodel.ChatMenuViewModel
import org.gnunet.gnunetmessenger.model.ChatMessage
import org.gnunet.gnunetmessenger.model.ChatMessageType
import org.gnunet.gnunetmessenger.model.MessageKind
import org.gnunet.gnunetmessenger.service.GnunetChat
import org.gnunet.gnunetmessenger.ui.adapters.ChatMessageAdapter
import org.gnunet.gnunetmessenger.viewmodel.ChatViewModel

class ChatFragment : Fragment(R.layout.fragment_chat) {

    private lateinit var recyclerView: RecyclerView
    private lateinit var adapter: ChatMessageAdapter
    private val args: ChatFragmentArgs by navArgs()
    private lateinit var chatViewModel: ChatViewModel
    private lateinit var chatContext: ChatContext
    private lateinit var chatMenuViewModel: ChatMenuViewModel
    private lateinit var mainActivity: MainActivity
    private lateinit var gnunetChat: GnunetChat

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        val menuHost = requireActivity() as MenuHost


        menuHost.addMenuProvider(object : MenuProvider {
            override fun onCreateMenu(menu: Menu, menuInflater: MenuInflater) {


                menuInflater.inflate(R.menu.chat_menu, menu)

                val chatGroup = gnunetChat.getGroupFromContext(chatContext)

                if (null != chatGroup) {

                    gnunetChat.iterateGroupContacts(chatGroup) { _, contact ->
                        val itemId = View.generateViewId()
                        chatMenuViewModel.contactMenuIds[itemId] = contact
                        menu.add(Menu.NONE, itemId, 0, "👤 ${contact.name}")
                        0
                    }
                }
            }

            override fun onPrepareMenu(menu: Menu) {
                val contact = gnunetChat.getContextContact(chatContext)
                val isBlocked = gnunetChat.isContactBlocked(contact)
                val isGroup = gnunetChat.isGroup(chatContext)
                val isPlatform = gnunetChat.isPlatform(chatContext)
                val blockItem = menu.findItem(R.id.menu_block_contact)

                blockItem.isVisible = !isGroup || !isPlatform
                blockItem.title = if (isBlocked) "Unblock Contact" else "Block Contact"

                menu.findItem(R.id.menu_invite_members)?.isVisible = isGroup || isPlatform
            }

            override fun onMenuItemSelected(menuItem: MenuItem): Boolean {
                val mainActivity = requireActivity() as MainActivity
                val gnunetChat = mainActivity.getGnunetChatInstance()
                val handle = mainActivity.getChatHandle()

                val contact = chatMenuViewModel.contactMenuIds[menuItem.itemId]
                if (contact != null) {
                    val action = ChatFragmentDirections.actionChatFragmentToContactFragment(contact)
                    findNavController().navigate(action)
                    return true
                }

                return when (menuItem.itemId) {
                    R.id.menu_share_identity -> {
                        val action = ChatFragmentDirections.actionChatFragmentToLobbyDisplayFragment(lobbyId = gnunetChat.getProfileKey(handle),
                            // lifetime not used here
                            lifetime = "0")
                        findNavController().navigate(action)
                        true
                    }
                    R.id.menu_block_contact -> {
                        val contact = gnunetChat.getContextContact(chatContext)
                        val isBlocked = gnunetChat.isContactBlocked(contact)
                        gnunetChat.setContactBlocked(contact, !isBlocked)
                        contact.blocked = !isBlocked
                        requireActivity().invalidateOptionsMenu()
                        true
                    }
                    R.id.menu_leave_chat -> {
                        val contact = gnunetChat.getContextContact(chatContext)
                        gnunetChat.deleteContact(contact)
                        true
                    }
                    R.id.menu_invite_members -> {
                        val chatGroup = gnunetChat.getGroupFromContext(chatContext)
                        if (null != chatGroup) {
                            val action = ChatFragmentDirections.actionChatFragmentToMemberListFragment(chatGroup)
                            findNavController().navigate(action)
                        }
                        true
                    }
                    else -> false
                }
            }
        }, viewLifecycleOwner)
        val title = try {
            when {
                gnunetChat.isGroup(chatContext) || gnunetChat.isPlatform(chatContext) -> {
                    gnunetChat.getGroupFromContext(chatContext)?.name
                        ?: getString(R.string.placeholder_label_chat)
                }
                else -> {
                    gnunetChat.getContextContact(chatContext).name
                }
            }
        } catch (t: Throwable) {
            // The server died (or the binder hasn't reattached yet). Bounce
            // the user back to the chat list instead of letting the
            // IllegalStateException kill the whole app on the main thread.
            Log.w("ChatFragment", "Server unreachable while opening chat; popping back stack", t)
            Toast.makeText(
                requireContext(),
                getString(R.string.toast_server_unreachable),
                Toast.LENGTH_SHORT
            ).show()
            runCatching { findNavController().popBackStack() }
            return
        }
        (requireActivity() as AppCompatActivity).supportActionBar?.title = title
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        val view = inflater.inflate(R.layout.fragment_chat, container, false)
        mainActivity = activity as MainActivity
        gnunetChat = mainActivity.getGnunetChatInstance()

        chatContext = args.chatContext
        chatViewModel = mainActivity.getChatViewModel(chatContext)!!
        chatMenuViewModel = mainActivity.getChatMenuViewModel(chatContext)!!
        recyclerView = view.findViewById(R.id.chatRecyclerView)
        recyclerView.layoutManager = LinearLayoutManager(requireContext())

        adapter = ChatMessageAdapter()
        recyclerView.adapter = adapter

        adapter.registerAdapterDataObserver(object : RecyclerView.AdapterDataObserver() {
            override fun onItemRangeInserted(positionStart: Int, itemCount: Int) {
                recyclerView.scrollToPosition(adapter.itemCount - 1)
            }
        })

        chatViewModel.messages.observe(viewLifecycleOwner, Observer { messages ->
            Log.d(
                "ChatFragment",
                "observer: count=${messages.size} " +
                    "types=${messages.map { it.type }} " +
                    "texts=${messages.map { it.text.take(8) }}"
            )
            adapter.submitList(messages)
            recyclerView.scrollToPosition(messages.size - 1)
        })

        // If the ViewModel has no messages (e.g., after account switch cleared
        // state), reload history from libgnunetchat via native iteration.
        if (chatViewModel.messages.value.isNullOrEmpty()) {
            viewLifecycleOwner.lifecycleScope.launch {
                try {
                    val profileKey = gnunetChat.getProfileKey(mainActivity.getChatHandle())
                    val history = gnunetChat.iterateContextMessages(chatContext)
                    Log.d(
                        "ChatFragment",
                        "iterate: profileKey='${profileKey.take(16)}...' " +
                            "(len=${profileKey.length}) historySize=${history.size}"
                    )
                    for (msg in history) {
                        val sKey = msg.sender?.key ?: ""
                        val isOwn = sKey.isNotBlank() && sKey == profileKey
                        Log.d(
                            "ChatFragment",
                            "iterate.msg: senderKey='${sKey.take(16)}...' " +
                                "(len=${sKey.length}) isOwn=$isOwn text='${msg.text}'"
                        )
                        val typed = msg.copy(type = if (isOwn) ChatMessageType.OWN else ChatMessageType.OTHER)
                        chatViewModel.addMessage(typed)
                    }
                    Log.d("ChatFragment", "Loaded ${history.size} messages from native")
                } catch (t: Throwable) {
                    Log.w("ChatFragment", "iterateContextMessages failed", t)
                }
            }
        }

        view.findViewById<Button>(R.id.sendButton).setOnClickListener {
            val input = view.findViewById<EditText>(R.id.inputMessage)
            val text = input.text.toString()
            if (text.isNotBlank()) {
                val newMessage = ChatMessage(
                    chatContext = chatContext,
                    text = text,
                    timestamp = System.currentTimeMillis(),
                    sender = ChatContact(chatContext, mainActivity.currentAccount?.name ?: ""),
                    kind = MessageKind.TEXT,
                    type = ChatMessageType.OWN
                )
                chatViewModel.addMessage(newMessage)
                Log.d(
                    "ChatFragment",
                    "send: nativeCtxPtr=${chatContext.nativeContextPointer} " +
                        "userPtr=${chatContext.userPointer} isGroup=${chatContext.isGroup} " +
                        "textLen=${text.length}"
                )
                gnunetChat.sendText(chatContext, text)
                input.text.clear()
            }
        }

        return view
    }
}
