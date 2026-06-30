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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/adapters/AttributeAdapter.kt
 */

package org.gnunet.gnunetmessenger.ui.adapters

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import org.gnunet.gnunetmessenger.databinding.AttributeItemBinding

class AttributeAdapter(private val attributes: MutableList<Pair<String, String>>) : RecyclerView.Adapter<AttributeAdapter.AttributeViewHolder>() {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): AttributeViewHolder {
        val binding = AttributeItemBinding.inflate(LayoutInflater.from(parent.context), parent, false)
        return AttributeViewHolder(binding)
    }

    override fun onBindViewHolder(holder: AttributeViewHolder, position: Int) {
        val attribute = attributes[position]
        holder.bind(attribute)
    }

    override fun getItemCount(): Int = attributes.size

    inner class AttributeViewHolder(private val binding: AttributeItemBinding) : RecyclerView.ViewHolder(binding.root) {
        fun bind(attribute: Pair<String, String>) {
            binding.keyName.text = attribute.first
            binding.valueName.text = attribute.second
        }
    }
}
