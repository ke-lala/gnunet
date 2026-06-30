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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/contact/ShareAttributesFragment.kt
 */

package org.gnunet.gnunetmessenger.ui.contact

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.LinearLayoutManager
import android.widget.Toast
import androidx.navigation.fragment.navArgs
import androidx.recyclerview.widget.RecyclerView
import org.gnunet.gnunetmessenger.MainActivity
import org.gnunet.gnunetmessenger.R
import org.gnunet.gnunetmessenger.ui.adapters.ShareAttributesAdapter

class ShareAttributesFragment : Fragment() {

    private val args: ShareAttributesFragmentArgs by navArgs()
    private lateinit var adapter: ShareAttributesAdapter
    private val selectedAttributes = mutableSetOf<Pair<String, String>>()

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ): View {
        val view = inflater.inflate(R.layout.fragment_share_attribute, container, false)
        val recycler = view.findViewById<RecyclerView>(R.id.shareAttributeList)
        val button = view.findViewById<Button>(R.id.btn_confirm_share)

        val activity = activity as MainActivity
        val gnunetChat = activity.getGnunetChatInstance()
        val handle = activity.getChatHandle()
        val attributes = mutableListOf<Pair<String, String>>()

        adapter = ShareAttributesAdapter(attributes, selectedAttributes)
        recycler.layoutManager = LinearLayoutManager(requireContext())
        recycler.adapter = adapter

        gnunetChat.getAttributes(handle) { key, value ->
            attributes.add(key to value)
            adapter.notifyItemInserted(attributes.size - 1)
        }

        button.setOnClickListener {
            Toast.makeText(requireContext(),
                "Share ${selectedAttributes.size} Attribute", Toast.LENGTH_SHORT).show()

        }

        return view
    }
}