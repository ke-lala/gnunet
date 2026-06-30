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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/account/CreateAccountFragment.kt
 */

package org.gnunet.gnunetmessenger.ui.account

import android.net.Uri
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.activity.result.contract.ActivityResultContracts
import androidx.lifecycle.lifecycleScope
import androidx.navigation.fragment.findNavController
import kotlinx.coroutines.launch
import org.gnunet.gnunetmessenger.MainActivity
import org.gnunet.gnunetmessenger.databinding.FragmentCreateAccountBinding
import org.gnunet.gnunetmessenger.model.ChatHandle
import org.gnunet.gnunetmessenger.service.GnunetChat
import org.gnunet.gnunetmessenger.model.GnunetReturnValue
import org.gnunet.gnunetmessenger.util.AvatarStorageUtil

class CreateAccountFragment : Fragment() {

    private lateinit var binding: FragmentCreateAccountBinding
    private lateinit var mainActivity: MainActivity
    private lateinit var gnunetChat: GnunetChat
    private lateinit var handle: ChatHandle
    private var selectedAvatarUri: Uri? = null

    private val avatarImageRequest = registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
        uri?.let {
            selectedAvatarUri = it
            binding.avatarPreview.setImageURI(it)
            AvatarStorageUtil.saveAvatarFromUri(requireContext(), uri, gnunetChat.getProfileKey(handle))
            binding.avatarPreview.setImageURI(it)
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        mainActivity = activity as MainActivity
        gnunetChat = mainActivity.getGnunetChatInstance()
        handle = mainActivity.getChatHandle()

        binding = FragmentCreateAccountBinding.inflate(inflater, container, false)

        binding.selectAvatarButton.setOnClickListener {
            avatarImageRequest.launch("image/*") // Start the image picker
        }

        val avatar = AvatarStorageUtil.loadAvatar(requireContext(), gnunetChat.getProfileKey(handle))
        avatar?.let {
            binding.avatarPreview.setImageBitmap(it)
        }


        binding.confirmButton.setOnClickListener {
            val accountName = binding.accountNameInput.text.toString()
            if (accountName.isNotEmpty()) {
                createAccount(accountName)
            }
        }

        binding.cancelButton.setOnClickListener {
            // Navigate back to the previous screen
            requireActivity().onBackPressed()
        }

        return binding.root
    }

    private fun createAccount(accountName: String) {
        viewLifecycleOwner.lifecycleScope.launch {
            try {
                val result = gnunetChat.createAccount(handle = handle, name = accountName)

                when (result) {
                    GnunetReturnValue.OK -> {
                        // Erfolgreich → zurück zur Account-Liste
                        requireActivity().runOnUiThread {
                            findNavController().popBackStack()
                        }
                    }
                    else -> {
                        // TODO: Fehler anzeigen (Toast/Snackbar)
                        println("createAccount failed: $result")
                    }
                }
            } catch (t: Throwable) {
                // TODO: Fehler anzeigen (Toast/Snackbar)
                t.printStackTrace()
            }
        }
    }
}
