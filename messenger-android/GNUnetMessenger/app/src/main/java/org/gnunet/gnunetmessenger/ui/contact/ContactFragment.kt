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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/contact/ContactFragment.kt
 */

package org.gnunet.gnunetmessenger.ui.contact

import android.graphics.BitmapFactory
import android.net.Uri
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.navigation.fragment.findNavController
import androidx.navigation.fragment.navArgs
import org.gnunet.gnunetmessenger.MainActivity
import org.gnunet.gnunetmessenger.R
import org.gnunet.gnunetmessenger.model.ChatContact
import org.gnunet.gnunetmessenger.service.GnunetChat
import org.gnunet.gnunetmessenger.ui.BaseProfileFragment
import org.gnunet.gnunetmessenger.util.AvatarStorageUtil

class ContactFragment : BaseProfileFragment() {

    private val args: ContactFragmentArgs by navArgs()
    private lateinit var contact: ChatContact
    private lateinit var gnunetChat: GnunetChat
    private var selectedAvatarUri: Uri? = null

    private val avatarImageRequest =
        registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
            uri?.let {
                selectedAvatarUri = it
                val key = gnunetChat.getContactKey(contact)
                AvatarStorageUtil.saveAvatarFromUri(requireContext(), uri, key)
                avatarImage.setImageURI(uri)
            }
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val activity = requireActivity() as MainActivity
        gnunetChat = activity.getGnunetChatInstance()
        contact = args.contact
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        return inflater.inflate(R.layout.fragment_profile_base, container, false)
    }

    override fun setupUI() {
        nameEdit.visibility = View.GONE
        saveNameButton.visibility = View.GONE
        nameText.text = contact.name

        val publicKey = gnunetChat.getContactKey(contact)

        val avatar = AvatarStorageUtil.loadAvatar(requireContext(), publicKey)
        avatarImage.setImageBitmap(
            avatar ?: BitmapFactory.decodeResource(resources, R.drawable.ic_account_circle)
        )

        avatarImage.setOnClickListener {
            avatarImageRequest.launch("image/*")
        }

        publicKeyText.visibility = View.VISIBLE
        qrCodeImage.visibility = View.GONE
        publicKeyText.text = publicKey

        shareAttributesButton.visibility = View.VISIBLE
        blockContactButton.visibility = View.VISIBLE

        /*listAttributesButton.setOnClickListener {
            val action = ContactFragmentDirections
                .actionContactFragmentToAttributeListFragment(contact, editable = false)
            findNavController().navigate(action)
        }*/

        shareAttributesButton.setOnClickListener {
            val action = ContactFragmentDirections
                .actionContactFragmentToShareAttributesFragment(contact)
            findNavController().navigate(action)
        }

        blockContactButton.setOnClickListener {
            val blocked = gnunetChat.isContactBlocked(contact)
            gnunetChat.setContactBlocked(contact, !blocked)
            contact.blocked = !blocked
            val msg = if (blocked) "Unblocked" else "Blocked"
            Toast.makeText(requireContext(), msg, Toast.LENGTH_SHORT).show()
        }

        shareIdentityButton.setOnClickListener {
            val action = ContactFragmentDirections.actionContactFragmentToLobbyDisplayFragment(
                lobbyId = publicKey,
                lifetime = "0"
            )
            findNavController().navigate(action)
        }
    }
}