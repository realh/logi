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

}
