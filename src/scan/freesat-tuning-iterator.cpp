/*
    logi - A DVB DVR designed for web-based clients.
    Copyright (C) 2016 Tony Houghton <h@realh.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "freesat-tuning-iterator.h"

namespace logi
{

std::shared_ptr<TuningProperties> FreesatTuningIterator::next()
{
    auto s = presets_[iter_];
    if (!s)
        return nullptr;
    ++iter_;
    return std::make_shared<TuningProperties>(s);
}

void FreesatTuningIterator::reset()
{
    iter_ = 0;
}

char const *FreesatTuningIterator::presets_[] =
{
    "S2 11023250 H 23000000 2/3 25 8PSK",
    "S2 10935500 V 23000000 8/9 25 QPSK",
    "S2 10847000 V 23000000 2/3 25 8PSK",
    "S1 11307000 V 27500000 2/3 35 QPSK",
    "S1 11344500 H 27500000 2/3 35 QPSK",
    "S1 11223670 V 27500000 2/3 35 QPSK",
    "S1 11222170 H 27500000 2/3 35 QPSK",
    "S1 11426330 V 27500000 2/3 35 QPSK",
    "S1 11343000 V 27500000 2/3 35 QPSK",
    nullptr
};

}
