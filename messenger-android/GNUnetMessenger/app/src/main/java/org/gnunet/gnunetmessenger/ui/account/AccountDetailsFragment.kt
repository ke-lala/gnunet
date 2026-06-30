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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/account/AccountDetailsFragment.kt
 */

package org.gnunet.gnunetmessenger.ui.account

import android.os.Bundle
import android.graphics.BitmapFactory
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.lifecycle.lifecycleScope
import androidx.navigation.fragment.findNavController
import kotlinx.coroutines.launch
import org.gnunet.gnunetmessenger.MainActivity
import org.gnunet.gnunetmessenger.R
import org.gnunet.gnunetmessenger.ui.BaseProfileFragment
import org.gnunet.gnunetmessenger.util.AvatarStorageUtil

class AccountDetailsFragment : BaseProfileFragment() {

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        return inflater.inflate(R.layout.fragment_profile_base, container, false)
    }

    override fun setupUI() {
        val activity = requireActivity() as MainActivity
        val gnunetChat = activity.getGnunetChatInstance()
        val handle = activity.getChatHandle()


        viewLifecycleOwner.lifecycleScope.launch {
            try {
                nameEdit.setText(gnunetChat.getProfileName(handle))
            } catch (t: Throwable) {
                // optional: Fehlermeldung / Toast
            }
        }
        nameText.visibility = View.GONE
        nameEdit.visibility = View.VISIBLE
        saveNameButton.visibility = View.VISIBLE


        saveNameButton.setOnClickListener {
            val newName = nameEdit.text.toString()
            viewLifecycleOwner.lifecycleScope.launch {
                try {
                    gnunetChat.setProfileName(handle, newName)
                } catch (t: Throwable) {
                    // optional: Fehlermeldung / Toast
                }
            }
        }

        val avatar = AvatarStorageUtil.loadAvatar(requireContext(), gnunetChat.getProfileKey(handle))
        avatarImage.setImageBitmap(avatar ?: BitmapFactory.decodeResource(resources, R.drawable.ic_account_circle))

        shareIdentityButton.setOnClickListener {
            val action = AccountDetailsFragmentDirections.actionAccountDetailsFragmentToLobbyDisplayFragment(
                lobbyId = gnunetChat.getProfileKey(handle),
                lifetime = "0"
            )
            findNavController().navigate(action)
        }

        /*listAttributesButton.setOnClickListener {
            val action = AccountDetailsFragmentDirections
                .actionAccountDetailsFragmentToAttributeListFragment(null, editable = true)
            findNavController().navigate(action)
        }*/
    }
}