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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/adapters/ShareAttributesAdapter.kt
 */

package org.gnunet.gnunetmessenger.ui.adapters

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.CheckBox
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import org.gnunet.gnunetmessenger.R

class ShareAttributesAdapter(
    private val items: List<Pair<String, String>>,
    private val selected: MutableSet<Pair<String, String>>
) : RecyclerView.Adapter<ShareAttributesAdapter.ViewHolder>() {

    inner class ViewHolder(view: View) : RecyclerView.ViewHolder(view) {
        val checkbox: CheckBox = view.findViewById(R.id.checkbox)
        val keyValue: TextView = view.findViewById(R.id.keyValue)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_share_attribute, parent, false)
        return ViewHolder(view)
    }

    override fun getItemCount() = items.size

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val (key, value) = items[position]
        holder.keyValue.text = "$key: $value"
        holder.checkbox.isChecked = selected.contains(items[position])
        holder.checkbox.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) selected.add(items[position]) else selected.remove(items[position])
        }
    }
}