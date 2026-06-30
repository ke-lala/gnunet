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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/chat/MemberListFragment.kt
 */

package org.gnunet.gnunetmessenger.ui.chat

import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
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
import org.gnunet.gnunetmessenger.ui.adapters.ContactListAdapter
import org.gnunet.gnunetmessenger.viewmodel.ContactListViewModel

class MemberListFragment : Fragment(R.layout.fragment_member_list) {

    private val args: MemberListFragmentArgs by navArgs()
    private lateinit var adapter: ContactListAdapter
    private val viewModel: ContactListViewModel by activityViewModels()

    private var existingMemberKeys: Set<String> = emptySet()
    private var latestContacts: List<ChatContact> = emptyList()

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        val chatGroup = args.chatGroup
        Log.d("MemberListFragment", "Opened for group '${chatGroup.name}' userPointer=${chatGroup.userPointer}")
        val mainActivity = requireActivity() as MainActivity
        val gnunetChat = mainActivity.getGnunetChatInstance()

        val recyclerView = view.findViewById<RecyclerView>(R.id.memberRecyclerView)
        val inviteButton = view.findViewById<Button>(R.id.confirmButton)
        val hint = view.findViewById<TextView>(R.id.memberListHint)

        recyclerView.layoutManager = LinearLayoutManager(requireContext())
        adapter = ContactListAdapter()
        recyclerView.adapter = adapter

        fun renderList() {
            val invitable = latestContacts.filter {
                it.key.isBlank() || it.key !in existingMemberKeys
            }
            adapter.submitList(invitable)
            hint.text = if (invitable.isEmpty()) {
                "No contacts available to invite. Share your identity via a lobby to add contacts first."
            } else {
                "Select contacts to invite"
            }
        }

        viewModel.contacts.observe(viewLifecycleOwner, Observer { contactList ->
            latestContacts = contactList ?: emptyList()
            renderList()
        })

        // Refresh the contact list each time this screen opens so stale/empty
        // view-model state doesn't leave the list blank.
        mainActivity.loadChats()

        // Load existing members asynchronously so we can filter them out.
        viewLifecycleOwner.lifecycleScope.launch {
            existingMemberKeys = try {
                gnunetChat.listGroupContacts(chatGroup)
                    .mapNotNull { it.key.takeIf { k -> k.isNotBlank() } }
                    .toSet()
            } catch (t: Throwable) {
                Log.w("MemberListFragment", "listGroupContacts failed", t)
                emptySet()
            }
            renderList()
        }

        inviteButton.setOnClickListener {
            val selectedContacts = adapter.getSelectedContacts()
            Log.d("MemberListFragment", "Inviting ${selectedContacts.size} contacts to group '${chatGroup.name}' (groupPtr=${chatGroup.userPointer})")
            for (contact in selectedContacts) {
                Log.d("MemberListFragment", "  Inviting contact '${contact.name}' (contactPtr=${contact.userPointer}, key=${contact.key.take(8)})")
                gnunetChat.inviteContactToGroup(chatGroup, contact)
            }
            findNavController().popBackStack()
        }
    }
}