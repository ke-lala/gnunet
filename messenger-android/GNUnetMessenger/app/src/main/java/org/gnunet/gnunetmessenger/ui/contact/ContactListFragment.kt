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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/contact/ContactListFragment.kt
 */

package org.gnunet.gnunetmessenger.ui.contact

import android.os.Bundle
import android.view.View
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.lifecycle.Observer
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import org.gnunet.gnunetmessenger.R
import org.gnunet.gnunetmessenger.ui.adapters.ContactListAdapter
import org.gnunet.gnunetmessenger.viewmodel.ContactListViewModel

class ContactListFragment : Fragment(R.layout.fragment_contact_list) {

    private lateinit var recyclerView: RecyclerView
    private lateinit var adapter: ContactListAdapter
    private val viewModel: ContactListViewModel by activityViewModels()

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        recyclerView = view.findViewById(R.id.contactRecyclerView)
        recyclerView.layoutManager = LinearLayoutManager(requireContext())
        adapter = ContactListAdapter()
        recyclerView.adapter = adapter

        viewModel.contacts.observe(viewLifecycleOwner, Observer { contactList ->
            adapter.submitList(contactList)
        })
    }
}